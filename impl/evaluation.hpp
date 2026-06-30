#include "../decl/evaluation_decl.hpp"

#include <cctype>
#include <sstream>

namespace clab {

    inline void Evaluation::set_state(const std::string& id, bool v) {
        _flags_info[id].state = v;
    }

    inline void Evaluation::add_param(const std::string& id, const std::string& v) {
        _flags_info[id].list.push_back(v);
    }

    inline void Evaluation::clear_params(const std::string& id) {
        _flags_info[id].list.clear();
    }

    inline void Evaluation::set_aborted_by(const std::string& id) {
        _abort_id = id;
    }

    inline bool Evaluation::state(const std::string& id) const {
        auto it = _flags_info.find(id);
        return it != _flags_info.end() && it->second.state;
    }

    inline const std::vector<std::string>& Evaluation::list(const std::string& id) const {
        static const std::vector<std::string> empty;
        auto it = _flags_info.find(id);
        return it != _flags_info.end() ? it->second.list : empty;
    }

    inline std::shared_ptr<Evaluation::Flag> Evaluation::handle(const std::string& id) const {
        auto it = _flags_info.find(id);
        if (it == _flags_info.end()) return nullptr;
        return std::make_shared<Flag>(it->second);
    }

    inline const std::string& Evaluation::value(const std::string& id) const {
        static const std::string empty;
        auto it = _flags_info.find(id);
        if (it == _flags_info.end() || it->second.list.empty())
            return empty;
        return it->second.list.back();
    }

    inline const std::string& Evaluation::String(const std::string& id) const {
        return value(id);
    }

    inline int Evaluation::Int(const std::string& id) const {
        const std::string& v = value(id);
        if (v.empty() && !state(id))
            throw TypeConversion("flag '" + id + "' has no value to convert to int");
        try {
            return std::stoi(v);
        } catch (...) {
            throw TypeConversion("'" + v + "' cannot be parsed as int for '" + id + "'");
        }
    }

    inline double Evaluation::Float(const std::string& id) const {
        const std::string& v = value(id);
        if (v.empty() && !state(id))
            throw TypeConversion("flag '" + id + "' has no value to convert to double");
        try {
            return std::stod(v);
        } catch (...) {
            throw TypeConversion("'" + v + "' cannot be parsed as double for '" + id + "'");
        }
    }

    inline bool Evaluation::aborted() const {
        return _abort_id.has_value();
    }

    inline std::string Evaluation::aborted_id() const {
        return _abort_id.value_or("");
    }

} // namespace clab
