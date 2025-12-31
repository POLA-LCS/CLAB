/*---------------------------------------------------------------*\
| CLAB - Command Line Arguments Builder                           |
|                                                                 |
| File: evaluation.hpp                                            |
| Description:                                                    |
|     Header file for the Evaluation class, responsible for       |
|     storing and querying the results of the parsing process.    |
|                                                                 |
| Minimum Standard: ISO C++17                                     |
| License: MIT (c) 2025                                           |
\*---------------------------------------------------------------*/

#pragma once

#include <unordered_map>
#include "types.hpp"

namespace clab {

    /*------------------------------*\
    | Evaluation class to store      |
    | the results of the parsing     |
    \*------------------------------*/
    class Evaluation {
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

} // namespace clab