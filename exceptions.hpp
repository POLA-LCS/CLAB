/*------------------------------*\
| This exceptions header file    |
| for the library CLAB           |
| Command Line Arguments Builder |
\*------------------------------*/

#pragma once

#include <stdexcept>
#include <string>

class MissingArgument : public std::runtime_error {
public:
    explicit MissingArgument(const std::string& msg) : std::runtime_error(msg) {}
};

class InvalidBuilding : public std::runtime_error {
public:
    explicit InvalidBuilding(const std::string& msg) : std::runtime_error(msg) {}
};

class InvalidValue : public std::runtime_error {
public:
    explicit InvalidValue(const std::string& msg) : std::runtime_error(msg) {}
};