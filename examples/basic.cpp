// Basic CLAB usage: a simple file compiler tool
//
// Try:
//   ./basic -o output.bin input.src
//   ./basic -h
//   ./basic input.src              (uses default output)

#include "../clab.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    using namespace clab;

    CLAB cli;

    cli.start("help")
        .flag("h").flag("help", "--")
        .abort()
        .action([](const String&) {
            std::cout << "Usage: basic [options] <input>\n"
                      << "  -o, --output <file>  Output file (default: a.out)\n"
                      << "  -v, --verbose        Enable verbose output\n"
                      << "  -h, --help           Show this help\n";
        })
    .end();

    cli.start("output")
        .flag("o").flag("output", "--")
        .consume(1)
        .initial("a.out")
    .end();

    cli.start("verbose")
        .flag("v").flag("verbose", "--")
    .end();

    cli.start("input")
        .consume(1).required()
    .end();

    try {
        Evaluation eval = cli.evaluate(argc - 1, argv + 1);

        if (eval.aborted())
            return 0;

        std::cout << "Input:   " << eval.value("input")  << "\n"
                  << "Output:  " << eval.value("output") << "\n"
                  << "Verbose: " << (eval.state("verbose") ? "yes" : "no") << "\n";

    } catch (const Exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
