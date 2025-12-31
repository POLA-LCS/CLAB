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
#include "types.hpp"
#include "exceptions.hpp"
#include "evaluation.hpp"

namespace clab {

/*------------------------------*\
| CLAB class: Main builder and   |
| engine for argument parsing    |
\*------------------------------*/
class CLAB {
public:
    using Action = std::function<void(String)>;

    struct TagInfo {
        String prefix;
        bool toggle_val;
    };

    struct FlagData {
        String id;
        std::unordered_map<String, TagInfo> tags;
        size_t consumed_args = 0;
        Action action_cb;
        bool is_required = false;
        bool is_multiple = false;
        bool is_abort = false;
        bool is_overwritable = false;
        std::unordered_set<String> allowed_values;
    };

private:
    CLAB() = default;
    Vector<Shared<FlagData>> flags_vector;

    struct MatchCandidate {
        Shared<FlagData> flag;
        String full_tag;
        bool toggle;
    };

    inline Shared<FlagData> find_match(const String& arg, bool& out_toggle) const {
        Vector<MatchCandidate> candidates;
        for(const Shared<FlagData>& flag : flags_vector) {
            std::unordered_map<String, TagInfo>::const_iterator it;
            for(it = flag->tags.begin(); it != flag->tags.end(); ++it) {
                const TagInfo& info = it->second;
                candidates.push_back({ flag, info.prefix + it->first, info.toggle_val });
            }
        }

        std::sort(candidates.begin(), candidates.end(),
            [](const MatchCandidate& a, const MatchCandidate& b) noexcept {
            return a.full_tag.length() > b.full_tag.length();
        });

        for(const MatchCandidate& c : candidates) {
            if(arg == c.full_tag) {
                out_toggle = c.toggle;
                return c.flag;
            }
        }
        return nullptr;
    }

public:
    inline static CLAB& get() noexcept {
        static CLAB instance;
        return instance;
    }

    struct FlagConfigurator {
        Shared<FlagData> data;
        CLAB& parent;

        inline FlagConfigurator& action(Action fn) noexcept {
            data->action_cb = std::move(fn);
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

        inline FlagConfigurator& consume(size_t n) noexcept {
            data->consumed_args = n;
            return *this;
        }

        inline FlagConfigurator& consume(size_t n, std::initializer_list<String> allowed) {
            data->consumed_args = n;
            for(const String& s : allowed) data->allowed_values.insert(s);
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
            data->is_overwritable = true;
            data->is_multiple = true;
            return *this;
        }

        inline CLAB& end() {
            if(data->tags.empty() && data->is_multiple && data->consumed_args > 0) {
                throw InvalidBuilding("Positional argument '" + data->id + "' cannot have both .consume() and .multiple().");
            }
            return parent;
        }
    };

    inline FlagConfigurator start(String id) {
        Shared<FlagData> flag = std::make_shared<FlagData>();
        flag->id = id;
        flags_vector.push_back(flag);
        return { flag, *this };
    }

    inline Evaluation evaluate(int argc, char* argv[]) const {
        return evaluate(Vector<String>(argv, argv + argc));
    }

    inline Evaluation evaluate(const Vector<String>& args) const {
        Evaluation eval;
        std::unordered_set<String> found_ids;
        size_t arg_idx = 0;

        for(const String& arg : args) {
            bool dummy;
            Shared<FlagData> flag = find_match(arg, dummy);
            if(flag && flag->is_abort) {
                eval.set_abort(flag->id);
                eval.set_found(flag->id, dummy);
                if(flag->action_cb) flag->action_cb("");
                return eval;
            }
        }

        while(arg_idx < args.size()) {
            bool toggle_val = true;
            Shared<FlagData> matched_flag = find_match(args[arg_idx], toggle_val);

            if(!matched_flag) {
                bool consumed = false;
                for(const Shared<FlagData>& flag : flags_vector) {
                    if(flag->tags.empty() && (found_ids.find(flag->id) == found_ids.end() || flag->is_multiple)) {
                        found_ids.insert(flag->id);
                        eval.set_found(flag->id, true);
                        if(flag->is_multiple) {
                            while(arg_idx < args.size()) {
                                bool d;
                                if(find_match(args[arg_idx], d)) break;
                                String val = args[arg_idx++];
                                if(!flag->allowed_values.empty() && flag->allowed_values.find(val) == flag->allowed_values.end())
                                    throw InvalidValue("Value '" + val + "' not allowed for '" + flag->id + "'.");
                                if(flag->action_cb) flag->action_cb(val);
                                eval.add_value(flag->id, val);
                            }
                        } else {
                            for(size_t i = 0; i < flag->consumed_args; ++i) {
                                if(arg_idx >= args.size()) throw MissingArgument("Positional '" + flag->id + "' missing args.");
                                String val = args[arg_idx++];
                                if(!flag->allowed_values.empty() && flag->allowed_values.find(val) == flag->allowed_values.end())
                                    throw InvalidValue("Value '" + val + "' not allowed for '" + flag->id + "'.");
                                if(flag->action_cb) flag->action_cb(val);
                                eval.add_value(flag->id, val);
                            }
                        }
                        consumed = true; break;
                    }
                }
                if(!consumed) arg_idx++;
                continue;
            }

            if(found_ids.find(matched_flag->id) != found_ids.end() && !matched_flag->is_multiple)
                throw std::runtime_error("Flag '" + matched_flag->id + "' is not multiple.");

            if(matched_flag->is_overwritable) eval.clear_values(matched_flag->id);
            found_ids.insert(matched_flag->id);
            eval.set_found(matched_flag->id, toggle_val);
            arg_idx++;

            for(size_t i = 0; i < matched_flag->consumed_args; ++i) {
                if(arg_idx >= args.size()) throw MissingArgument("Flag '" + matched_flag->id + "' missing args.");
                String val = args[arg_idx++];
                if(!matched_flag->allowed_values.empty() && matched_flag->allowed_values.find(val) == matched_flag->allowed_values.end())
                    throw InvalidValue("Value '" + val + "' not allowed for '" + matched_flag->id + "'.");
                if(matched_flag->action_cb) matched_flag->action_cb(val);
                eval.add_value(matched_flag->id, val);
            }
        }

        for(const Shared<FlagData>& flag : flags_vector) {
            if(flag->is_required && found_ids.find(flag->id) == found_ids.end())
                throw MissingArgument("Required flag/positional '" + flag->id + "' missing.");
        }
        return eval;
    }
};

} // namespace clab