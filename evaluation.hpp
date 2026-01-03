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

namespace clab {

    class Evaluation {
    private:
        std::unordered_map<String, Vector<String>> _params;
        std::unordered_map<String, bool> _states;
        String _abort_id;

    public:
        Evaluation() = default;
        ~Evaluation() = default;

        /** @brief Sets the found state (boolean) for a given flag ID. */
        inline void set_found(const String& id, bool v) {
            _states[id] = v;
        }

        /** @brief Adds a string value to the parameter list of a flag ID. */
        inline void add_value(const String& id, const String& v) {
            _params[id].push_back(v);
        }

        /** @brief Removes all stored values for a specific flag ID. */
        inline void clear_values(const String& id) {
            _params[id].clear();
        }

        /** @brief Sets the ID of the flag that triggered a parsing abort. */
        inline void set_abort(const String& id) {
            _abort_id = id;
        }

        /** @brief Returns the boolean state of a flag. Returns false if not found. */
        inline bool state(const String& id) const {
            if(_states.find(id) == _states.end())
                return false;

            return _states.at(id);
        }

        /** @brief Returns all values associated with an ID. Returns an empty vector if none. */
        inline const Vector<String>& params(const String& id) const {
            static const Vector<String> empty_vec;

            if(_params.find(id) == _params.end())
                return empty_vec;

            return _params.at(id);
        }

        /** @brief Returns the last value added to a flag. Returns empty string if none. */
        inline String value(const String& id) const {
            if(_params.find(id) == _params.end())
                return "";

            const Vector<String>& vec = _params.at(id);

            if(vec.empty())
                return "";

            return vec.back();
        }

        /** @brief Checks if the parsing was aborted by a specific flag. */
        inline bool aborted() const {
            return !_abort_id.empty();
        }

        /** @brief Returns the ID of the flag that caused the abort. */
        inline String aborted_by() const {
            return _abort_id;
        }
    };

} // namespace clab