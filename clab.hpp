/*------------------------------*\
| This is the single header file |
| declaration and definition     |
| for the library CLAB           |
| Command Line Arguments Builder |
\*------------------------------*/

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <initializer_list>
#include "exceptions.hpp"

/*------------------------------*\
| Evaluation class to store      |
| the results of the parsing     |
\*------------------------------*/
class Evaluation {
    using String = std::string;
    template<class T>
    using Vector = std::vector<T>;

    std::unordered_map<String, bool> toggles;
    std::unordered_map<String, Vector<String>> values;
    String aborter_id;

    friend class CLAB;

    inline void set_found(const String& id, bool val) {
        toggles[id] = val;
    }

    inline void clear_values(const String& id) {
        values[id].clear();
    }

    inline void add_value(const String& id, String val) {
        values[id].push_back(std::move(val));
    }

    inline void set_abort(const String& id) {
        aborter_id = id;
    }

public:
    inline bool state(const String& id) const {
        std::unordered_map<String, bool>::const_iterator it = toggles.find(id);
        return it != toggles.end() ? it->second : false;
    }

    inline bool captured(const String& id) const noexcept {
        std::unordered_map<String, Vector<String>>::const_iterator it = values.find(id);
        return it != values.end() && !it->second.empty();
    }

    inline bool aborted() const noexcept {
        return !aborter_id.empty();
    }

    inline const String& aborted_by() const noexcept {
        return aborter_id;
    }

    inline const Vector<String>& params(const String& id) const noexcept {
        static const Vector<String> empty;
        std::unordered_map<String, Vector<String>>::const_iterator it = values.find(id);
        return it != values.end() ? it->second : empty;
    }
};

/*------------------------------*\
| CLAB class: Main builder and   |
| engine for argument parsing    |
\*------------------------------*/
class CLAB {
    using String = std::string;

    template<class T>
    using Shared = std::shared_ptr<T>;

    template<class T>
    using Vector = std::vector<T>;

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

    /*------------------------------*\
    | Internal match finder with     |
    | prefix length sorting          |
    \*------------------------------*/
    inline Shared<FlagData> find_match(const String& arg, bool& out_toggle) const {
        Vector<MatchCandidate> candidates;
        for(const Shared<FlagData>& flag : flags_vector) {
            for(const std::pair<const String, TagInfo>& tag_pair : flag->tags) {
                const String& tag = tag_pair.first;
                const TagInfo& info = tag_pair.second;
                candidates.push_back({
                    flag,
                    info.prefix + tag,
                    info.toggle_val
                    });
            }
        }

        std::sort(
            candidates.begin(),
            candidates.end(),
            [](const MatchCandidate& a, const MatchCandidate& b) noexcept {
            return a.full_tag.length() > b.full_tag.length();
        }
        );

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

    /*------------------------------*\
    | Fluent interface for flag      |
    | configuration                  |
    \*------------------------------*/
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
            data->allowed_values = allowed;
            return *this;
        }

        inline FlagConfigurator& required() noexcept {
            data->is_required = true;
            return *this;
        }

        inline FlagConfigurator& multiple() {
            if(data->is_overwritable) {
                throw InvalidBuilding(
                    "Flag '" + data->id + "' cannot be 'multiple' and 'over' at the same time."
                );
            }
            data->is_multiple = true;
            return *this;
        }

        inline FlagConfigurator& abort() noexcept {
            data->is_abort = true;
            return *this;
        }

        inline FlagConfigurator& over() {
            if(data->is_multiple && !data->is_overwritable) {
                throw InvalidBuilding(
                    "Flag '" + data->id + "' cannot be 'over' and 'multiple' at the same time."
                );
            }
            data->is_overwritable = true;
            data->is_multiple = true;
            return *this;
        }

        inline CLAB& end() noexcept {
            return parent;
        }
    };

    inline FlagConfigurator start(String id) {
        Shared<FlagData> flag = std::make_shared<FlagData>();
        flag->id = id;
        flags_vector.push_back(flag);
        return { flag, *this };
    }

    /*------------------------------*\
    | Main evaluation loop for       |
    | processing argv                |
    \*------------------------------*/
    inline Evaluation evaluate(int argc, char* argv[]) const {
        Evaluation eval;
        Vector<String> args;
        for(int i = 1; i < argc; ++i) {
            args.push_back(argv[i]);
        }

        std::unordered_set<String> found_ids;
        size_t arg_idx = 0;

        for(const String& arg : args) {
            bool dummy_toggle;
            Shared<FlagData> flag = find_match(arg, dummy_toggle);
            if(flag && flag->is_abort) {
                eval.set_abort(flag->id);
                eval.set_found(flag->id, dummy_toggle);
                if(flag->action_cb) flag->action_cb("");
                return eval;
            }
        }

        while(arg_idx < args.size()) {
            bool toggle_val = true;
            Shared<FlagData> matched_flag = find_match(args[arg_idx], toggle_val);

            if(matched_flag == nullptr) {
                bool consumed = false;
                for(const Shared<FlagData>& flag : flags_vector) {
                    if(flag->tags.empty() && found_ids.find(flag->id) == found_ids.end()) {
                        found_ids.insert(flag->id);
                        eval.set_found(flag->id, true);

                        for(size_t i = 0; i < flag->consumed_args; ++i) {
                            if(arg_idx >= args.size()) {
                                throw MissingArgument("Positional '" + flag->id + "' missing args.");
                            }

                            String val = args[arg_idx++];
                            if(!flag->allowed_values.empty() && flag->allowed_values.find(val) == flag->allowed_values.end()) {
                                throw InvalidValue("Value '" + val + "' not allowed for '" + flag->id + "'.");
                            }

                            if(flag->action_cb) flag->action_cb(val);
                            eval.add_value(flag->id, val);
                        }
                        consumed = true;
                        break;
                    }
                }
                if(!consumed) arg_idx++;
                continue;
            }

            if(found_ids.find(matched_flag->id) != found_ids.end() && !matched_flag->is_multiple) {
                throw std::runtime_error("Flag '" + matched_flag->id + "' is not multiple.");
            }

            if(matched_flag->is_overwritable) eval.clear_values(matched_flag->id);

            found_ids.insert(matched_flag->id);
            eval.set_found(matched_flag->id, toggle_val);
            arg_idx++;

            for(size_t i = 0; i < matched_flag->consumed_args; ++i) {
                if(arg_idx >= args.size()) {
                    throw MissingArgument("Flag '" + matched_flag->id + "' missing args.");
                }

                String val = args[arg_idx++];
                if(!matched_flag->allowed_values.empty() && matched_flag->allowed_values.find(val) == matched_flag->allowed_values.end()) {
                    throw std::runtime_error("Value '" + val + "' not allowed for '" + matched_flag->id + "'.");
                }

                if(matched_flag->action_cb) matched_flag->action_cb(val);
                eval.add_value(matched_flag->id, val);
            }
        }

        for(const Shared<FlagData>& flag : flags_vector) {
            if(flag->is_required && found_ids.find(flag->id) == found_ids.end()) {
                throw MissingArgument("Required flag/positional '" + flag->id + "' missing.");
            }
        }

        return eval;
    }
};