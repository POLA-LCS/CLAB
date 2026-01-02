/*---------------------------------------------------------------*\
| CLAB - Command Line Arguments Builder                           |
|                                                                 |
| File: evaluation.hpp                                            |
| Description:                                                    |
|     Result container for the parsing process. Stores states     |
|     and parameters for each flag/positional.                    |
|                                                                 |
| Minimum Standard: ISO C++17                                     |
| License: MIT (c) 2025                                           |
\*---------------------------------------------------------------*/

#pragma once

#include <unordered_map>
#include "types.hpp"

namespace clab {

    class Evaluation {
    private:
        std::unordered_map<String, bool> _states;
        std::unordered_map<String, Vector<String>> _params;
        String _abort_id;

    public:
        inline void set_found(const String& id, bool v) {
            _states[id] = v;
        }

        inline void add_value(const String& id, const String& v) {
            _params[id].push_back(v);
        }

        inline void clear_values(const String& id) {
            _params[id].clear();
        }

        inline void set_abort(const String& id) {
            _abort_id = id;
        }

        inline bool state(const String& id) const {
            std::unordered_map<String, bool>::const_iterator it = _states.find(id);

            if(it == _states.end()) {
                return false;
            }

            return it->second;
        }

        inline const Vector<String>& params(const String& id) const {
            static const Vector<String> empty_vec;
            std::unordered_map<String, Vector<String>>::const_iterator it = _params.find(id);

            if(it == _params.end()) {
                return empty_vec;
            }

            return it->second;
        }

        inline String value(const String& id) const {
            std::unordered_map<String, Vector<String>>::const_iterator it = _params.find(id);

            if(it == _params.end()) {
                return "";
            }

            const Vector<String>& vec = it->second;

            if(vec.empty()) {
                return "";
            }

            return vec.back();
        }

        inline bool aborted() const {
            return !_abort_id.empty();
        }

        inline std::string aborted_by() const {
            return _abort_id;
        }
    };

} // namespace clab