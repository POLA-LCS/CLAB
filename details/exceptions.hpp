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
    | Base Exception                 |
    | Common base for all CLAB errs  |
    \*------------------------------*/
    class Exception : public std::runtime_error {
    public:
        explicit Exception(const String& msg) : std::runtime_error(msg) {}
    };

    /*------------------------------*\
    | MissingArgument:               |
    | Thrown when a required flag or |
    | positional is not provided.    |
    \*------------------------------*/
    class MissingArgument : public Exception {
    public:
        explicit MissingArgument(const String& msg) : Exception(msg) {}
    };

    /*----------------------------*\
    | InvalidBuilding:             |
    | Thrown when the CLAB config  |
    | has logical inconsistencies. |
    \*----------------------------*/
    class InvalidBuilding : public Exception {
    public:
        explicit InvalidBuilding(const String& msg) : Exception(msg) {}
    };

    /*------------------------------*\
    | InvalidValue:                  |
    | Thrown when a provided value   |
    | is not in the allowed list.    |
    \*------------------------------*/
    class InvalidValue : public Exception {
    public:
        explicit InvalidValue(const String& msg) : Exception(msg) {}
    };

    /*------------------------------*\
    | UnexpectedArgument:            |
    | Thrown when more arguments are |
    | provided than expected.        |
    \*------------------------------*/
    class UnexpectedArgument : public Exception {
    public:
        explicit UnexpectedArgument(const String& msg) : Exception(msg) {}
    };

    /*--------------------------*\
    | RedundantArgument:         |
    | Thrown when a non-multiple |
    | flag is provided twice.    |
    \*--------------------------*/
    class RedundantArgument : public Exception {
    public:
        explicit RedundantArgument(const String& msg) : Exception(msg) {}
    };

    /*--------------------------------*\
    | TokenMismatch:                   |
    | Thrown when a flag prefix is     |
    | found where a value was expected |
    \*--------------------------------*/
    class TokenMismatch : public Exception {
    public:
        explicit TokenMismatch(const String& msg) : Exception(msg) {}
    };

    /*--------------------------------*\
    | MissingValue:                    |
    | Thrown when a flag or positional |
    | expects a value but none is      |
    | provided.                        |
    \*--------------------------------*/
    class MissingValue : public Exception {
    public:
        explicit MissingValue(const String& msg) : Exception(msg) {}
    };
} // namespace clab