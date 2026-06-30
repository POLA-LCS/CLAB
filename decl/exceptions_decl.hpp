#pragma once

#include <stdexcept>
#include <string>

namespace clab {

    class Exception : public std::runtime_error {
    public:
        explicit Exception(const std::string& msg);
    };

    class MissingArgument : public Exception { public: explicit MissingArgument(const std::string& msg); };
    class InvalidBuilding : public Exception { public: explicit InvalidBuilding(const std::string& msg); };
    class InvalidValue : public Exception { public: explicit InvalidValue(const std::string& msg); };
    class UnexpectedArgument : public Exception { public: explicit UnexpectedArgument(const std::string& msg); };
    class RedundantArgument : public Exception { public: explicit RedundantArgument(const std::string& msg); };
    class TokenMismatch : public Exception { public: explicit TokenMismatch(const std::string& msg); };
    class MissingValue : public Exception { public: explicit MissingValue(const std::string& msg); };
    class TypeConversion : public Exception { public: explicit TypeConversion(const std::string& msg); };

} // namespace clab
