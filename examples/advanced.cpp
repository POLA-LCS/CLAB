// Advanced CLAB features: toggles, allowed values, multiple flags, callbacks
//
// Try:
//   ./advanced --no-color -l warn -I src/ -I lib/ main.cpp util.cpp
//   ./advanced --color -l debug main.cpp
//   ./advanced -l invalid main.cpp          (throws InvalidValue)

#include "../clab.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    using namespace clab;

    CLAB cli;

    // Toggle: --color / --no-color
    cli.start("color")
        .toggle(true, "color", "--")
        .toggle(false, "no-color", "--")
        .initial(true)
    .end();

    // Allowed values enumeration
    cli.start("level")
        .flag("l").flag("level", "--")
        .consume(1, {"debug", "info", "warn", "error"})
        .initial("info")
    .end();

    // Multiple: -I can appear many times
    cli.start("include")
        .flag("I").flag("include", "--")
        .consume(1).multiple()
    .end();

    // Positional multiple: remaining args are source files
    cli.start("sources")
        .multiple().required()
    .end();

    try {
        Evaluation eval = cli.evaluate(argc - 1, argv + 1);

        std::cout << "Color:    " << (eval.state("color") ? "on" : "off") << "\n"
                  << "Level:    " << eval.value("level") << "\n";

        std::cout << "Includes: ";
        for (const auto& inc : eval.list("include"))
            std::cout << inc << " ";
        std::cout << "\n";

        std::cout << "Sources:  ";
        for (const auto& src : eval.list("sources"))
            std::cout << src << " ";
        std::cout << "\n";

    } catch (const Exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
