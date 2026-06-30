#include "decl/clab_decl.hpp"
#include "impl/exceptions.hpp"
#include "impl/evaluation.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace clab {

    // FlagConfigurator

    inline CLAB::FlagConfigurator& CLAB::FlagConfigurator::flag(std::string tag, std::string pref, bool toggle) {
        data->tags[tag] = { std::move(pref), toggle };
        data->default_toggle = !toggle;
        return *this;
    }

    inline CLAB::FlagConfigurator& CLAB::FlagConfigurator::flag(const std::vector<FlagEssential>& tags, bool toggle) {
        for (const auto& ft : tags) {
            if (!ft.tag.empty())
                data->tags[ft.tag] = { ft.prefix.empty() ? "-" : ft.prefix, toggle };
        }
        return *this;
    }

    inline CLAB::FlagConfigurator& CLAB::FlagConfigurator::initial(std::initializer_list<std::string> vals) {
        data->default_params = vals;
        return *this;
    }

    inline CLAB::FlagConfigurator& CLAB::FlagConfigurator::initial(int val) {
        data->arg_type = ArgType::Int;
        data->default_params.clear();
        data->default_params.push_back(std::to_string(val));
        return *this;
    }

    inline CLAB::FlagConfigurator& CLAB::FlagConfigurator::initial(double val) {
        data->arg_type = ArgType::Double;
        data->default_params.clear();
        data->default_params.push_back(std::to_string(val));
        return *this;
    }

    inline CLAB::FlagConfigurator& CLAB::FlagConfigurator::Int() noexcept {
        data->arg_type = ArgType::Int;
        return *this;
    }

    inline CLAB::FlagConfigurator& CLAB::FlagConfigurator::Float() noexcept {
        data->arg_type = ArgType::Double;
        return *this;
    }

    inline CLAB::FlagConfigurator& CLAB::FlagConfigurator::consume(size_t n) {
        data->consumed_args = n;
        data->allowed_params.clear();
        return *this;
    }

    inline CLAB::FlagConfigurator& CLAB::FlagConfigurator::consume(size_t n, std::initializer_list<std::string> allowed) {
        data->consumed_args = n;
        data->allowed_params = allowed;
        return *this;
    }

    inline CLAB::FlagConfigurator& CLAB::FlagConfigurator::required() noexcept {
        data->is_required = true;
        return *this;
    }

    inline CLAB::FlagConfigurator& CLAB::FlagConfigurator::multiple() noexcept {
        data->is_multiple = true;
        return *this;
    }

    inline CLAB::FlagConfigurator& CLAB::FlagConfigurator::abort() noexcept {
        data->is_abort = true;
        return *this;
    }

    inline CLAB::FlagConfigurator& CLAB::FlagConfigurator::over() noexcept {
        data->is_over = true;
        data->is_multiple = true;
        return *this;
    }

    inline CLAB::FlagConfigurator& CLAB::FlagConfigurator::auto_help(std::string name, std::vector<HelpEntry> entries) noexcept {
        data->is_abort = true;
        data->help_entries = std::move(entries);
        data->help_action = [name = std::move(name)](const std::string& /*id*/, const std::vector<HelpEntry>& entries) {
            // Find widest short_flag for column alignment
            std::size_t flag_w = 0;
            for (const auto& e : entries)
                if (e.short_flag.size() > flag_w) flag_w = e.short_flag.size();
            std::size_t flag_col = flag_w + 6;

            std::cout << "Usage: " << name << " [options]\n";
            for (const auto& entry : entries) {
                std::cout << "  " << entry.short_flag;
                for (std::size_t i = entry.short_flag.size(); i < flag_col; ++i)
                    std::cout << ' ';
                std::cout << entry.description;
                if (!entry.default_val.empty())
                    std::cout << "  (default: " << entry.default_val << ")";
                if (!entry.allowed_vals.empty()) {
                    if (entry.default_val.empty()) std::cout << "  (";
                    else std::cout << " (";
                    for (std::size_t i = 0; i < entry.allowed_vals.size(); ++i) {
                        if (i) std::cout << ", ";
                        std::cout << entry.allowed_vals[i];
                    }
                    std::cout << ")";
                }
                std::cout << "\n";
            }
        };
        return *this;
    }

    inline CLAB::FlagConfigurator& CLAB::FlagConfigurator::auto_version(std::string name, std::string version,
        std::string extra) noexcept {
        data->is_version = true;
        data->version_info = { std::move(name), std::move(version), std::move(extra) };
        return *this;
    }

    inline CLAB& CLAB::FlagConfigurator::end() {
        if (data->tags.empty() && data->is_multiple && data->consumed_args > 0)
            throw InvalidBuilding("Positional argument '" + data->id + "' cannot have both .consume() and .multiple().");
        return parent;
    }

    // CLAB

    inline CLAB::CLAB(const std::string& path_id) {
        start(path_id).required().consume(1u).end();
    }

    inline CLAB::FlagConfigurator CLAB::start(std::string id) {
        auto cfg = std::make_shared<FlagConfig>();
        cfg->id = std::move(id);
        flags_vector.push_back(cfg);
        return { cfg, *this };
    }

    inline Evaluation CLAB::evaluate(int argc, char* argv[]) const {
        std::vector<std::string> args(static_cast<std::size_t>(argc));
        for (int i = 0; i < argc; ++i)
            args[static_cast<std::size_t>(i)] = argv[i];
        return evaluate(args);
    }

    inline Evaluation CLAB::evaluate(const std::vector<std::string>& args) const {
        Evaluation out_eval;
        std::unordered_set<std::string> provided_ids;

        initialize_defaults(out_eval);

        if (check_for_abort(args, out_eval))
            return out_eval;

        std::size_t idx = 0;
        while (idx < args.size()) {
            const std::string& arg = args[idx];

            bool toggle{};
            auto flag = find_match(arg, toggle);

            if (flag) {
                handle_tagged_token(flag, toggle, args, idx, out_eval, provided_ids);
            } else {
                if (!handle_positional_token(args, idx, out_eval, provided_ids))
                    throw UnexpectedArgument("unexpected argument: " + arg);
            }
        }

        verify_required_flags(provided_ids);
        return out_eval;
    }

    inline void CLAB::validate_type(ArgType type, const std::string& val, const std::string& id) const {
        if (type == ArgType::String) return;
        std::size_t pos{};
        try {
            if (type == ArgType::Int) {
                std::stoi(val, &pos);
                if (pos != val.size())
                    throw TypeConversion("'" + id + "' expects an int, got \"" + val + "\"");
            } else {
                std::stod(val, &pos);
                if (pos != val.size())
                    throw TypeConversion("'" + id + "' expects a float, got \"" + val + "\"");
            }
        } catch (const TypeConversion&) {
            throw;
        } catch (...) {
            throw TypeConversion("'" + id + "' expects a " +
                (type == ArgType::Int ? "int" : "float") + ", got \"" + val + "\"");
        }
    }

    inline void CLAB::initialize_defaults(Evaluation& out_eval) const {
        for (const auto& flag : flags_vector) {
            if (!flag) continue;
            out_eval.set_state(flag->id, flag->default_toggle);
            for (const auto& param : flag->default_params)
                out_eval.add_param(flag->id, param);
        }
    }

    inline bool CLAB::check_for_abort(const std::vector<std::string>& args, Evaluation& out_eval) const {
        for (std::size_t i = 0; i < args.size(); ++i) {
            bool toggle{};
            auto flag = find_match(args[i], toggle);
            if (flag && flag->is_abort) {
                out_eval.set_aborted_by(flag->id);
                out_eval.set_state(flag->id, toggle);
                if (flag->help_action)
                    flag->help_action("", flag->help_entries);
                if (flag->action)
                    flag->action("");
                return true;
            }
        }
        return false;
    }

    inline void CLAB::validate_and_store(std::shared_ptr<FlagConfig> flag, const std::string& val,
        Evaluation& eval) const {
        if (!flag->allowed_params.empty() &&
            flag->allowed_params.find(val) == flag->allowed_params.end())
            throw InvalidValue("invalid value for '" + flag->id + "': \"" + val + "\"");
        validate_type(flag->arg_type, val, flag->id);
        eval.add_param(flag->id, val);
        if (flag->action)
            flag->action(val);
    }

    inline void CLAB::handle_tagged_token(std::shared_ptr<FlagConfig> flag, bool toggle,
        const std::vector<std::string>& args,
        size_t& idx, Evaluation& eval,
        std::unordered_set<std::string>& ids) const {
        bool already_seen = ids.find(flag->id) != ids.end();
        if (already_seen && !flag->is_multiple)
            throw RedundantArgument("redundant argument: " + flag->id);

        if (!already_seen && flag->consumed_args > 0 && !flag->is_over)
            eval.clear_params(flag->id);

        ids.insert(flag->id);
        eval.set_state(flag->id, toggle);

        ++idx;

        if (flag->consumed_args == 0) {
            if (flag->is_version) {
                const auto& v = flag->version_info;
                std::cout << v.name << " " << v.version;
                if (!v.extra.empty())
                    std::cout << " (" << v.extra << ")";
                std::cout << "\n";
            } else if (flag->help_action) {
                flag->help_action(flag->id, flag->help_entries);
            }
            return;
        }

        for (std::size_t i = 0; i < flag->consumed_args; ++i) {
            if (idx >= args.size())
                throw MissingValue("missing value for '" + flag->id + "'");

            const std::string& val = args[idx++];
            bool d = false;
            if (find_match(val, d))
                throw TokenMismatch("unexpected flag-like value: " + val);

            validate_and_store(flag, val, eval);
        }
    }

    inline bool CLAB::handle_positional_token(const std::vector<std::string>& args, size_t& idx,
        Evaluation& eval,
        std::unordered_set<std::string>& ids) const {
        for (const auto& flag : flags_vector) {
            if (!flag || !flag->tags.empty()) continue;

            bool is_first = ids.find(flag->id) == ids.end();
            if (!is_first && !flag->is_multiple) continue;

            if (is_first && (flag->is_multiple || flag->consumed_args > 0) && !flag->is_over)
                eval.clear_params(flag->id);

            ids.insert(flag->id);
            eval.set_state(flag->id, true);

            if (flag->is_multiple) {
                while (idx < args.size()) {
                    bool d = false;
                    if (find_match(args[idx], d))
                        break;
                    validate_and_store(flag, args[idx], eval);
                    ++idx;
                }
            } else {
                for (std::size_t i = 0; i < flag->consumed_args; ++i) {
                    if (idx >= args.size())
                        throw MissingValue("missing value for '" + flag->id + "'");
                    validate_and_store(flag, args[idx], eval);
                    ++idx;
                }
            }
            return true;
        }
        return false;
    }

    inline void CLAB::verify_required_flags(const std::unordered_set<std::string>& provided_ids) const {
        for (const auto& flag : flags_vector) {
            if (!flag) continue;
            bool provided = provided_ids.find(flag->id) != provided_ids.end();
            bool has_default = !flag->default_params.empty();
            if (flag->is_required && !provided && !has_default)
                throw MissingArgument("required argument missing: " + flag->id);
        }
    }

    inline std::shared_ptr<CLAB::FlagConfig> CLAB::find_match(const std::string& arg, bool& out_toggle) const {
        for (const auto& flag : flags_vector) {
            if (!flag) continue;
            for (const auto& [tag, info] : flag->tags) {
                if (arg.rfind(info.prefix + tag, 0) == 0) {
                    std::string rest = arg.substr(info.prefix.size() + tag.size());
                    if (rest.empty()) {
                        out_toggle = info.toggle_val;
                        return flag;
                    }
                    if (rest == "+" || rest == "true" || rest == "on") {
                        out_toggle = true;
                        return flag;
                    }
                    if (rest == "-" || rest == "false" || rest == "off") {
                        out_toggle = false;
                        return flag;
                    }
                }
            }
        }
        return nullptr;
    }

} // namespace clab
