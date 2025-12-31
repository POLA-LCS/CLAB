/*---------------------------------------------------------------*\
| CLAB - Command Line Arguments Builder                           |
|                                                                 |
| File: types.hpp                                                 |
| Description:                                                    |
|     Centralized type aliases and utility definitions used       |
|     across the CLAB library.                                    |
|                                                                 |
| Minimum Standard: ISO C++17                                     |
| License: MIT (c) 2025                                           |
\*---------------------------------------------------------------*/

#pragma once

#include <string>
#include <vector>
#include <memory>

namespace clab {
    using String = std::string;

    template<class T>
    using Vector = std::vector<T>;

    template<class T>
    using Shared = std::shared_ptr<T>;
} // namespace clab