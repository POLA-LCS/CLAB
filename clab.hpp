/*---------------------------------------------------------------*\
| CLAB - Command Line Arguments Builder                           |
|                                                                 |
| File: clab.hpp                                                  |
| Description:                                                    |
|     Main engine header file. Implements the fluent builder      |
|     interface and the core parsing logic for CLI arguments.     |
|                                                                 |
| Minimum Standard: ISO C++17                                     |
| License: MIT (c) 2025                                           |
\*---------------------------------------------------------------*/

#pragma once

#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <initializer_list>
#include <memory>
#include "types.hpp"
#include "exceptions.hpp"
#include "evaluation.hpp"

namespace clab {

    class CLAB {
    public:
        struct TagInfo {
            String prefix;
            bool toggle_val;
        };

        struct FlagConfig {
            using Action = std::function<void(const String&)>;

            std::unordered_map<String, TagInfo> tags{};
            std::unordered_set<String> allowed_params{};
            Vector<String> default_params{}; // defaults
            String id{};
            Action action{};
            size_t consumed_args = 0;
            bool is_required    = false; // from tigger
            bool is_multiple    = false; // from tigger
            bool is_abort       = false; // from tigger
            bool is_over        = false; // from tigger
            bool default_toggle = false; // defaults
        };

    private:
        Vector<Shared<FlagConfig>> flags_vector;

        struct MatchCandidate {
            Shared<FlagConfig> flag;
            String full_tag;
            bool toggle;
        };

        inline void initialize_defaults(Evaluation& out_eval) const {
            for(const Shared<FlagConfig>& flag : flags_vector) {
                out_eval.set_state(flag->id, flag->default_toggle);
                for(const String& val : flag->default_params)
                    out_eval.add_param(flag->id, val);
            }
        }

        inline bool check_for_abort(const Vector<String>& args, Evaluation& out_eval) const {
            for(const String& arg : args) {
                bool dummy = false;
                Shared<FlagConfig> flag = find_match(arg, dummy);

                if(!flag || !flag->is_abort)
                    continue;

                out_eval.set_aborted_by(flag->id);
                out_eval.set_state(flag->id, dummy);

                if(flag->action)
                    flag->action("");

                return true;
            }
            return false;
        }

        inline void validate_and_store(Shared<FlagConfig> flag, const String& val, Evaluation& eval) const {
            if(!flag->allowed_params.empty() && flag->allowed_params.find(val) == flag->allowed_params.end())
                throw InvalidValue(val);

            eval.add_param(flag->id, val);
            if(flag->action)
                flag->action(val);
        }

        inline void handle_tagged_token(Shared<FlagConfig> flag, bool toggle, const Vector<String>& args,
            size_t& idx, Evaluation& eval, std::unordered_set<String>& ids) const {
            bool already_seen = ids.find(flag->id) != ids.end();
            if(already_seen && !flag->is_multiple)
                throw RedundantArgument(flag->id);

            if(!already_seen && flag->consumed_args > 0 && !flag->is_over)
                eval.clear_params(flag->id);

            ids.insert(flag->id);
            eval.set_state(flag->id, toggle);
            idx++;

            for(size_t i = 0; i < flag->consumed_args; ++i) {
                if(idx >= args.size())
                    throw MissingValue(flag->id);

                String val = args[idx++];
                bool d = false;
                if(find_match(val, d))
                    throw TokenMismatch(val);

                validate_and_store(flag, val, eval);
            }
        }

        inline bool handle_positional_token(const Vector<String>& args, size_t& idx,
            Evaluation& eval, std::unordered_set<String>& ids) const {
            for(const Shared<FlagConfig>& flag : flags_vector) {
                if(!flag->tags.empty())
                    continue;

                bool is_first = ids.find(flag->id) == ids.end();
                if(!is_first && !flag->is_multiple)
                    continue;

                if(is_first && (flag->is_multiple || flag->consumed_args > 0) && !flag->is_over)
                    eval.clear_params(flag->id);

                ids.insert(flag->id);
                eval.set_state(flag->id, true);

                if(flag->is_multiple) {
                    while(idx < args.size()) {
                        bool d = false;
                        if(find_match(args[idx], d))
                            break;
                        validate_and_store(flag, args[idx++], eval);
                    }
                } else {
                    for(size_t i = 0; i < flag->consumed_args; ++i) {
                        if(idx >= args.size())
                            throw MissingValue(flag->id);
                        validate_and_store(flag, args[idx++], eval);
                    }
                }
                return true;
            }
            return false;
        }

        inline void verify_required_flags(const std::unordered_set<String>& provided_ids) const {
            for(const Shared<FlagConfig>& flag : flags_vector) {
                if(flag->is_required && provided_ids.find(flag->id) == provided_ids.end())
                    throw MissingArgument(flag->id);
            }
        }

        inline Shared<FlagConfig> find_match(const String& arg, bool& out_toggle) const {
            for(const Shared<FlagConfig>& flag : flags_vector) {
                for(const std::pair<const String, TagInfo>& tag_info : flag->tags) {
                    if(arg == (tag_info.second.prefix + tag_info.first)) {
                        out_toggle = tag_info.second.toggle_val;
                        return flag;
                    }
                }
            }
            return nullptr;
        }

    public:
        CLAB() = default;
        /*
        ** @brief Loads `start(path_id).required().consume(1)`
        */
        CLAB(const std::string& path_id) {
            this->start(path_id).required().consume(1).end();
        }
        ~CLAB() = default;

        struct FlagConfigurator {
            Shared<FlagConfig> data;
            CLAB& parent;

            inline FlagConfigurator& action(FlagConfig::Action fn) noexcept {
                data->action = std::move(fn);
                return *this;
            }

            inline FlagConfigurator& flag(String tag, String pref = "-") {
                data->tags[tag] = { pref, true };
                return *this;
            }

            inline FlagConfigurator& toggle(bool val, String tag, String pref = "-") {
                data->tags[tag] = { pref, val };
                return *this;
            }

            inline FlagConfigurator& initial(bool val) noexcept {
                data->default_toggle = val;
                return *this;
            }

            inline FlagConfigurator& initial(String val) {
                data->default_params.clear();
                data->default_params.push_back(std::move(val));
                return *this;
            }

            inline FlagConfigurator& initial(std::initializer_list<String> vals) {
                data->default_params = vals;
                return *this;
            }

            inline FlagConfigurator& consume(size_t n) noexcept {
                data->consumed_args = n;
                return *this;
            }

            inline FlagConfigurator& consume(size_t n, std::initializer_list<String> allowed) {
                data->consumed_args = n;
                for(const String& s : allowed)
                    data->allowed_params.insert(s);
                return *this;
            }

            inline FlagConfigurator& required() noexcept {
                data->is_required = true;
                return *this;
            }

            inline FlagConfigurator& multiple() noexcept {
                data->is_multiple = true;
                return *this;
            }

            inline FlagConfigurator& abort() noexcept {
                data->is_abort = true;
                return *this;
            }

            inline FlagConfigurator& over() noexcept {
                data->is_over = true;
                data->is_multiple = true;
                return *this;
            }

            inline CLAB& end() {
                if(data->tags.empty() && data->is_multiple && data->consumed_args > 0)
                    throw InvalidBuilding("Positional argument '" + data->id + "' cannot have both .consume() and .multiple().");
                return parent;
            }
        };

        inline FlagConfigurator start(String id = "") {
            Shared<FlagConfig> flag = std::make_shared<FlagConfig>();
            flag->id = id;
            flags_vector.push_back(flag);
            return { flag, *this };
        }

        inline Evaluation evaluate(int argc, char* argv[]) const {
            Vector<String> args;
            for(int i = 0; i < argc; ++i)
                args.push_back(String(argv[i]));
            return evaluate(args);
        }

        inline Evaluation evaluate(const Vector<String>& args) const {
            Evaluation eval;
            std::unordered_set<String> user_provided_ids;
            size_t arg_idx = 0;

            initialize_defaults(eval);

            if(check_for_abort(args, eval))
                return eval;

            while(arg_idx < args.size()) {
                bool toggle_val = true;
                Shared<FlagConfig> matched_flag = find_match(args[arg_idx], toggle_val);

                if(matched_flag) {
                    handle_tagged_token(matched_flag, toggle_val, args, arg_idx, eval, user_provided_ids);
                } else if(!handle_positional_token(args, arg_idx, eval, user_provided_ids)) {
                    throw UnexpectedArgument(args[arg_idx]);
                }
            }

            verify_required_flags(user_provided_ids);
            return eval;
        }
    };

} // namespace clab