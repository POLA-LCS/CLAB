/*---------------------------------------------------------------*\
| CLAB - Command Line Arguments Builder                           |
|                                                                 |
| File: exceptions.hpp                                            |
| Description:                                                    |
|     Custom exception classes for error handling during building |
|     and evaluation of command line arguments.                   |
|                                                                 |
| Minimum Standard: ISO C++17                                     |
| License: MIT (c) 2025                                           |
\*---------------------------------------------------------------*/

#pragma once

#include <stdexcept>
#include "types.hpp"

namespace clab {

    /*------------------------------*\
    | MissingArgument:               |
    | Thrown when a required flag or |
    | positional is not provided.    |
    \*------------------------------*/
    class MissingArgument : public std::runtime_error {
    public:
        explicit MissingArgument(const String& msg) : std::runtime_error(msg) {}
    };

    /*----------------------------*\
    | InvalidBuilding:             |
    | Thrown when the CLAB config  |
    | has logical inconsistencies. |
    \*----------------------------*/
    class InvalidBuilding : public std::runtime_error {
    public:
        explicit InvalidBuilding(const String& msg) : std::runtime_error(msg) {}
    };

    /*------------------------------*\
    | InvalidValue:                  |
    | Thrown when a provided value   |
    | is not in the allowed list.    |
    \*------------------------------*/
    class InvalidValue : public std::runtime_error {
    public:
        explicit InvalidValue(const String& msg) : std::runtime_error(msg) {}
    };

    /*------------------------------*\
    | UnexpectedArgument:            |
    | Thrown when more arguments are |
    | provided than expected.        |
    \*------------------------------*/
    class UnexpectedArgument : public std::runtime_error {
    public:
        explicit UnexpectedArgument(const String& msg) : std::runtime_error(msg) {}
    };

    /*--------------------------*\
    | RedundantArgument:         |
    | Thrown when a non-multiple |
    | flag is provided twice.    |
    \*--------------------------*/
    class RedundantArgument : public std::runtime_error {
    public:
        explicit RedundantArgument(const String& msg) : std::runtime_error(msg) {}
    };

    /*--------------------------------*\
    | TokenMismatch:                   |
    | Thrown when a flag prefix is     |
    | found where a value was expected |
    \*--------------------------------*/
    class TokenMismatch : public std::runtime_error {
    public:
        explicit TokenMismatch(const String& msg) : std::runtime_error(msg) {}
    };

    /*--------------------------------*\
    | MissingValue:                    |
    | Thrown when a flag or positional |
    | expects a value but none is      |
    | provided.                        |
    \*--------------------------------*/
    class MissingValue : public std::runtime_error {
    public:
        explicit MissingValue(const String& msg) : std::runtime_error(msg) {}
    };
} // namespace clab