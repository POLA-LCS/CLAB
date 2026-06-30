#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <initializer_list>
#include <iostream>
#include <tuple>

#include "exceptions_decl.hpp"
#include "evaluation_decl.hpp"

namespace clab {

    // CLAB

    class CLAB {
    public:
        struct TagInfo {
            std::string prefix;
            bool toggle_val;
        };

        struct FlagEssential {
            std::string tag{};
            std::string prefix{ "-" };
        };

        struct HelpEntry {
            std::string id;
            std::string short_flag;
            std::string description;
            bool is_flag = true;
            std::string default_val;
            std::vector<std::string> allowed_vals;
        };

        struct VersionInfo {
            std::string name;
            std::string version;
            std::string extra;
        };

        enum class ArgType { String, Int, Double };

        struct FlagConfig {
            using Action = std::function<void(const std::string&)>;
            using HelpAction = std::function<void(const std::string&, const std::vector<HelpEntry>&)>;

            std::unordered_map<std::string, TagInfo> tags{};
            std::unordered_set<std::string> allowed_params{};
            std::vector<std::string> default_params{};
            std::string id{};
            Action action{};
            HelpAction help_action{};
            std::vector<HelpEntry> help_entries{};
            VersionInfo version_info{};
            ArgType arg_type = ArgType::String;
            bool is_version = false;
            size_t consumed_args = 0;
            bool is_required = false;
            bool is_multiple = false;
            bool is_abort = false;
            bool is_over = false;
            bool default_toggle = false;
        };

        struct FlagConfigurator {
            std::shared_ptr<FlagConfig> data;
            CLAB& parent;

            template<typename Fn, typename... BindArgs>
            FlagConfigurator& action(Fn&& fn, BindArgs&&... bindargs) {
                data->action = [fn = std::forward<Fn>(fn),
                    bindargs = std::make_tuple(std::forward<BindArgs>(bindargs)...)]
                    (const std::string& val) mutable {
                    std::apply([&fn, &val](auto&&... args) {
                        fn(val, std::forward<decltype(args)>(args)...);
                    }, std::move(bindargs));
                };
                return *this;
            }

            FlagConfigurator& flag(std::string tag, std::string pref = "-", bool toggle = true);
            FlagConfigurator& flag(const std::vector<FlagEssential>& tags, bool toggle = true);

            template<typename StringOrBool>
            FlagConfigurator& initial(StringOrBool&& val) {
                if constexpr (std::is_convertible_v<StringOrBool, std::string>) {
                    data->default_params.clear();
                    data->default_params.push_back(std::forward<StringOrBool>(val));
                }
                return *this;
            }

            FlagConfigurator& initial(bool val) {
                data->default_toggle = val;
                return *this;
            }

            FlagConfigurator& initial(std::initializer_list<std::string> vals);
            FlagConfigurator& initial(int val);
            FlagConfigurator& initial(double val);

            FlagConfigurator& Int() noexcept;
            FlagConfigurator& Float() noexcept;
            FlagConfigurator& consume(size_t n);
            FlagConfigurator& consume(size_t n, std::initializer_list<std::string> allowed);
            FlagConfigurator& required() noexcept;
            FlagConfigurator& multiple() noexcept;
            FlagConfigurator& abort() noexcept;
            FlagConfigurator& over() noexcept;
            FlagConfigurator& auto_help(std::string name, std::vector<HelpEntry> entries) noexcept;
            FlagConfigurator& auto_version(std::string name, std::string version, std::string extra = "") noexcept;
            CLAB& end();
        };

        CLAB() = default;
        CLAB(const std::string& path_id);
        ~CLAB() = default;

        FlagConfigurator start(std::string id = "");
        Evaluation evaluate(int argc, char* argv[]) const;
        Evaluation evaluate(const std::vector<std::string>& args) const;

    private:
        std::vector<std::shared_ptr<FlagConfig>> flags_vector;

        struct MatchCandidate {
            std::shared_ptr<FlagConfig> flag;
            std::string full_tag;
            bool toggle;
        };

        void validate_type(ArgType type, const std::string& val, const std::string& id) const;
        void initialize_defaults(Evaluation& out_eval) const;
        bool check_for_abort(const std::vector<std::string>& args, Evaluation& out_eval) const;
        void validate_and_store(std::shared_ptr<FlagConfig> flag, const std::string& val, Evaluation& eval) const;
        void handle_tagged_token(std::shared_ptr<FlagConfig> flag, bool toggle, const std::vector<std::string>& args,
            size_t& idx, Evaluation& eval, std::unordered_set<std::string>& ids) const;
        bool handle_positional_token(const std::vector<std::string>& args, size_t& idx,
            Evaluation& eval, std::unordered_set<std::string>& ids) const;
        void verify_required_flags(const std::unordered_set<std::string>& provided_ids) const;
        std::shared_ptr<FlagConfig> find_match(const std::string& arg, bool& out_toggle) const;
    };

} // namespace clab
