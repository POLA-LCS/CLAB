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
            bool default_toggle = false;
            Vector<String> default_values;
        };

    private:
        Vector<Shared<FlagData>> flags_vector;

        struct MatchCandidate {
            Shared<FlagData> flag;
            String full_tag;
            bool toggle;
        };

        inline Shared<FlagData> find_match(const String& arg, bool& out_toggle) const {
            Vector<MatchCandidate> candidates;
            Vector<Shared<FlagData>>::const_iterator flag_it;
            for(flag_it = flags_vector.begin(); flag_it != flags_vector.end(); ++flag_it) {
                const Shared<FlagData>& flag = *flag_it;
                std::unordered_map<String, TagInfo>::const_iterator tag_it;
                for(tag_it = flag->tags.begin(); tag_it != flag->tags.end(); ++tag_it) {
                    const TagInfo& info = tag_it->second;
                    candidates.push_back({ flag, info.prefix + tag_it->first, info.toggle_val });
                }
            }

            std::sort(candidates.begin(), candidates.end(),
                [](const MatchCandidate& a, const MatchCandidate& b) noexcept {
                return a.full_tag.length() > b.full_tag.length();
            });

            Vector<MatchCandidate>::const_iterator cand_it;
            for(cand_it = candidates.begin(); cand_it != candidates.end(); ++cand_it) {
                if(arg != cand_it->full_tag) continue;
                out_toggle = cand_it->toggle;
                return cand_it->flag;
            }
            return nullptr;
        }

    public:
        CLAB() = default;
        ~CLAB() = default;

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

            inline FlagConfigurator& initial(bool val) noexcept {
                data->default_toggle = val;
                return *this;
            }

            inline FlagConfigurator& initial(String val) {
                data->default_values.clear();
                data->default_values.push_back(std::move(val));
                return *this;
            }

            inline FlagConfigurator& initial(std::initializer_list<String> vals) {
                data->default_values = vals;
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
            Vector<String> args;
            for(int i = 0; i < argc; ++i) args.push_back(String(argv[i]));
            return evaluate(args);
        }

        inline Evaluation evaluate(const Vector<String>& args) const {
            Evaluation eval;
            std::unordered_set<String> user_provided_ids;
            size_t arg_idx = 0;

            Vector<Shared<FlagData>>::const_iterator init_it;
            for(init_it = flags_vector.begin(); init_it != flags_vector.end(); ++init_it) {
                const Shared<FlagData>& flag = *init_it;
                eval.set_found(flag->id, flag->default_toggle);
                Vector<String>::const_iterator val_it;
                for(val_it = flag->default_values.begin(); val_it != flag->default_values.end(); ++val_it) {
                    eval.add_value(flag->id, *val_it);
                }
            }

            Vector<String>::const_iterator pre_it;
            for(pre_it = args.begin(); pre_it != args.end(); ++pre_it) {
                bool dummy;
                Shared<FlagData> flag = find_match(*pre_it, dummy);
                if(!flag || !flag->is_abort) continue;
                eval.set_abort(flag->id);
                eval.set_found(flag->id, dummy);
                if(flag->action_cb) flag->action_cb("");
                return eval;
            }

            while(arg_idx < args.size()) {
                const String& current_token = args[arg_idx];
                bool toggle_val = true;
                Shared<FlagData> matched_flag = find_match(current_token, toggle_val);

                if(!matched_flag) {
                    bool consumed = false;
                    Vector<Shared<FlagData>>::const_iterator pos_it;
                    for(pos_it = flags_vector.begin(); pos_it != flags_vector.end(); ++pos_it) {
                        const Shared<FlagData>& flag = *pos_it;
                        bool is_first_time = user_provided_ids.find(flag->id) == user_provided_ids.end();
                        if(!flag->tags.empty()) continue;
                        if(!is_first_time && !flag->is_multiple) continue;

                        if(is_first_time && (flag->is_multiple || flag->consumed_args > 0)) {
                            eval.clear_values(flag->id);
                        }

                        user_provided_ids.insert(flag->id);
                        eval.set_found(flag->id, true);

                        if(flag->is_multiple) {
                            while(arg_idx < args.size()) {
                                bool d;
                                if(find_match(args[arg_idx], d)) break;
                                String val = args[arg_idx++];
                                if(!flag->allowed_values.empty() && flag->allowed_values.find(val) == flag->allowed_values.end())
                                    throw InvalidValue(val);
                                eval.add_value(flag->id, val);
                                if(flag->action_cb) flag->action_cb(val);
                            }
                        } else {
                            for(size_t i = 0; i < flag->consumed_args; ++i) {
                                if(arg_idx >= args.size())
                                    throw MissingValue(flag->id);
                                String val = args[arg_idx++];
                                if(!flag->allowed_values.empty() && flag->allowed_values.find(val) == flag->allowed_values.end())
                                    throw InvalidValue(val);
                                eval.add_value(flag->id, val);
                                if(flag->action_cb) flag->action_cb(val);
                            }
                        }
                        consumed = true; break;
                    }
                    if(!consumed)
                        throw UnexpectedArgument(current_token);
                    continue;
                }

                bool already_seen = user_provided_ids.find(matched_flag->id) != user_provided_ids.end();
                if(already_seen && !matched_flag->is_multiple)
                    throw RedundantArgument(matched_flag->id);

                if(!already_seen && matched_flag->consumed_args > 0) {
                    eval.clear_values(matched_flag->id);
                }

                user_provided_ids.insert(matched_flag->id);
                eval.set_found(matched_flag->id, toggle_val);
                arg_idx++;

                for(size_t i = 0; i < matched_flag->consumed_args; ++i) {
                    if(arg_idx >= args.size())
                        throw MissingValue(matched_flag->id);

                    String val = args[arg_idx++];
                    bool d;
                    if(find_match(val, d))
                        throw TokenMismatch(val);
                        
                    if(!matched_flag->allowed_values.empty() && matched_flag->allowed_values.find(val) == matched_flag->allowed_values.end())
                        throw InvalidValue(val);
                        
                    eval.add_value(matched_flag->id, val);
                    if(matched_flag->action_cb) matched_flag->action_cb(val);
                }
            }

            Vector<Shared<FlagData>>::const_iterator req_it;
            for(req_it = flags_vector.begin(); req_it != flags_vector.end(); ++req_it) {
                const Shared<FlagData>& flag = *req_it;
                if(flag->is_required && user_provided_ids.find(flag->id) == user_provided_ids.end())
                    throw MissingArgument(flag->id);
            }
            return eval;
        }
    };

} // namespace clab