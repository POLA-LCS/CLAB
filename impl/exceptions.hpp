#include "../decl/exceptions_decl.hpp"

namespace clab {

    inline Exception::Exception(const std::string& msg) : std::runtime_error(msg) {}

    inline MissingArgument::MissingArgument(const std::string& msg) : Exception(msg) {}
    inline InvalidBuilding::InvalidBuilding(const std::string& msg) : Exception(msg) {}
    inline InvalidValue::InvalidValue(const std::string& msg) : Exception(msg) {}
    inline UnexpectedArgument::UnexpectedArgument(const std::string& msg) : Exception(msg) {}
    inline RedundantArgument::RedundantArgument(const std::string& msg) : Exception(msg) {}
    inline TokenMismatch::TokenMismatch(const std::string& msg) : Exception(msg) {}
    inline MissingValue::MissingValue(const std::string& msg) : Exception(msg) {}
    inline TypeConversion::TypeConversion(const std::string& msg) : Exception(msg) {}

} // namespace clab
