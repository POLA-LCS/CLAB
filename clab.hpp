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

public:
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

    inline bool state(const String& id) const {
        auto it = toggles.find(id);
        return it != toggles.end() ? it->second : false;
    }

    inline bool aborted() const noexcept {
        return !aborter_id.empty();
    }

    inline const String& aborted_by() const noexcept {
        return aborter_id;
    }

    inline const Vector<String>& params(const String& id) const noexcept {
        static const Vector<String> empty;
        auto it = values.find(id);
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
            for(auto const& tag_pair : flag->tags) {
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

        for(const auto& arg : args) {
            bool dummy_toggle;
            auto flag = find_match(arg, dummy_toggle);
            if(flag && flag->is_abort) {
                eval.set_abort(flag->id);
                eval.set_found(flag->id, dummy_toggle);
                return eval;
            }
        }

        for(size_t args_i = 0; args_i < args.size(); args_i++) {
            bool toggle_val = true;
            Shared<FlagData> matched_flag = find_match(args[args_i], toggle_val);

            if(matched_flag == nullptr) {
                for(const Shared<FlagData>& flag : flags_vector) {
                    if(flag->tags.empty()) {
                        found_ids.insert(flag->id);
                        if(flag->action_cb) {
                            flag->action_cb(args[args_i]);
                        }
                        eval.add_value(flag->id, args[args_i]);
                    }
                }
                continue;
            }

            if(found_ids.find(matched_flag->id) != found_ids.end() && !matched_flag->is_multiple) {
                throw std::runtime_error("Flag '" + matched_flag->id + "' is not multiple.");
            }

            if(matched_flag->is_overwritable) {
                eval.clear_values(matched_flag->id);
            }

            found_ids.insert(matched_flag->id);
            eval.set_found(matched_flag->id, toggle_val);

            for(size_t i = 0; i < matched_flag->consumed_args; ++i) {
                if((args_i + 1) >= args.size()) {
                    throw MissingArgument("Flag '" + matched_flag->id + "' missing args.");
                }

                String val = args[++args_i];
                if(matched_flag->action_cb) {
                    matched_flag->action_cb(val);
                }
                eval.add_value(matched_flag->id, val);
            }
        }

        for(const auto& flag : flags_vector) {
            if(flag->is_required && found_ids.find(flag->id) == found_ids.end()) {
                throw MissingArgument("Required flag '" + flag->id + "' missing.");
            }
        }

        return eval;
    }
};