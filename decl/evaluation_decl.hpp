#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>

namespace clab {

    class Evaluation {
    public:
        struct Flag {
            std::vector<std::string> list{};
            bool state{};
        };

        Evaluation() = default;
        ~Evaluation() = default;

        void set_state(const std::string& id, bool v);
        void add_param(const std::string& id, const std::string& v);
        void clear_params(const std::string& id);
        void set_aborted_by(const std::string& id);

        bool state(const std::string& id) const;
        const std::vector<std::string>& list(const std::string& id) const;
        std::shared_ptr<Flag> handle(const std::string& id) const;

        const std::string& value(const std::string& id) const;
        const std::string& String(const std::string& id) const;
        int Int(const std::string& id) const;
        double Float(const std::string& id) const;

        bool aborted() const;
        std::string aborted_id() const;

    private:
        std::unordered_map<std::string, Flag> _flags_info;
        std::optional<std::string> _abort_id = std::nullopt;
    };

} // namespace clab
