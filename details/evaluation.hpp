/*---------------------------------------------------------------*\
| CLAB - Command Line Arguments Builder                           |
|                                                                 |
| File: evaluation.hpp                                            |
| Description:                                                    |
|     Result container for the parsing process. Stores states      |
|     and parameters for each flag/positional.                    |
|                                                                 |
| Minimum Standard: ISO C++17                                     |
| License: MIT (c) 2025                                           |
\*---------------------------------------------------------------*/

#pragma once

#include <unordered_map>
#include <string>
#include "types.hpp"
#include <optional>

namespace clab {
    class Evaluation {
    public:
        struct Flag {
            Vector<String> list{};
            bool state{};
        };
    private:
        std::unordered_map<String, Flag> _flags_info;
        std::optional<String> _abort_id = std::nullopt;

    public:
        Evaluation() = default;
        ~Evaluation() = default;


        /** @brief Sets the found state (boolean) for a given flag ID. */
        inline void set_state(const String& id, bool v) {
            _flags_info[id].state = v;
        }

        /** @brief Adds a string value to the parameter list of a flag ID. */
        inline void add_param(const String& id, const String& v) {
            _flags_info[id].list.push_back(v);
        }

        /** @brief Removes all stored values for a specific flag ID. */
        inline void clear_params(const String& id) {
            _flags_info[id].list.clear();
        }

        /** @brief Sets the ID of the flag that triggered a parsing abort. */
        inline void set_aborted_by(const String& id) {
            _abort_id = id;
        }

        /** @brief Returns the boolean state of a flag. Returns false if not found. */
        inline bool state(const String& id) const {
            auto it = _flags_info.find(id);
            if(it == _flags_info.end())
                return false;

            return it->second.state;
        }

        /** @brief Returns all values associated with an ID. Returns an empty vector if none. */
        inline const Vector<String>& list(const String& id) const {
            static const Vector<String> empty;

            auto it = _flags_info.find(id);
            if(it == _flags_info.end())
                return empty;

            return it->second.list;
        }

        inline Shared<Flag> handle(const String& id) const {
            auto it = _flags_info.find(id);
            if(it == _flags_info.end())
                return nullptr;
            return std::make_shared<Flag>(it->second);
        }

        /** @brief Returns the last value added to a flag. Returns empty string if none. */
        inline const String& value(const String& id) const {
            static const String empty;

            auto it = _flags_info.find(id);
            if(it->second.list.empty() || it == _flags_info.end())
                return empty;

            return it->second.list.back();
        }

        /** @brief Checks if the parsing was aborted by a specific flag. */
        inline bool aborted() const {
            return _abort_id.has_value();
        }

        /** @brief Returns the ID of the flag that caused the abort. */
        inline String aborted_id() const {
            return _abort_id.value();
        }
    };

} // namespace clab