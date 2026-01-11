# CLAB | Command Line Arguments Builder

A header-only C++17 library for parsing command-line arguments using a fluent and expressive builder pattern.

## Features

- **Fluent Interface**: Chain methods to create complex configurations with ease.
- **Header-Only**: Just include `clab.hpp` and you're ready to go.
- **Tagged flags**: Supports flags with names such as `-o` (whole life flags).
- **Custom flag prefixes**: Supports custom prefixes for your flags (default in `-`).
- **Positional Arguments**: Supports values captured by position.
- **Required Arguments**: Enforce the presence of mandatory arguments.
- **Multiple Occurrences**: Allow arguments to be specified multiple times.
- **Abort Flags**: Define flags that stops the parsing when present.
- **Default Values**: Provide default values for your arguments.
- **Allowed Values**: Restrict argument values to a predefined set.
- **Callbacks**: Execute custom actions when an argument is parsed.
- **Custom Exceptions**: A comprehensive set of exceptions for robust error handling.

## Installation

`clab` is a header-only library. To use it, simply clone the repository and include the path of the `clab.hpp` file.

## Basic Usage

Here's a simple example of how to use `clab`:

```cpp
#include <iostream>
#include "clab.hpp"

int main(int argc, char* argv[]) {
    clab::CLAB builder;

    builder
        .start("input")
            .flag("i")
            .flag("input")
            .consume(1)
            .required()
        .end()
        .start("output")
            .flag("o")
            .flag("output")
            .consume(1)
            .initial("a.out")
        .end()
        .start("help")
            .flag("h")
            .flag("help")
            .abort()
        .end();

    try {
        clab::Evaluation eval = builder.evaluate(argc, argv);

        if(eval.aborted()) {
            if(eval.aborted_id() == "help") {
                std::cout << "This is a sample application." << std::endl;
                std::cout << "Usage: " << argv[0] << " -i <input_file> [-o <output_file>]" << std::endl;
            }
            return 0;
        }

        std::cout << "Input file: " << eval.value("input") << std::endl;
        std::cout << "Output file: " << eval.value("output") << std::endl;

    } catch(const clab::Exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
```

## API Documentation

### Builder Methods

- `start(id)`: Begins the configuration of a new argument with a unique ID.
- `flag(tag, prefix)`: Defines a tag for the argument (e.g., `-f`, `--file`).
- `toggle(value, tag, prefix)`: Defines a tag that, when present, sets the argument's state to the given boolean value.
- `consume(n)`: Specifies that the argument consumes `n` values from the command line.
- `consume(n, allowed_values)`: Specifies that the argument consumes `n` values, which must be from the `allowed_values` list.
- `required()`: Marks the argument as mandatory.
- `multiple()`: Allows the argument to be store multiple values when provided multiple times.
- `abort()`: If this argument is present, parsing is stopped immediately.
- `over()`: Allows the argument to be provided multiple times beign overrided each time.
- `initial(value)`: Sets a default value for the argument.
- `action(callback)`: Provides a function to be called when the argument is parsed.
- `end()`: Finalizes the configuration for the current argument.

### Evaluation Methods

The `evaluate()` method returns an `Evaluation` object with the following methods:

- `state(id)`: Returns the boolean state of an argument (true if present or toggled to true).
- `value(id)`: Returns the last value associated with an argument.
- `list(id)`: Returns a `Vector<String>` of all values associated with a multi-value argument.
- `aborted()`: Returns `true` if parsing was aborted by a flag.
- `aborted_id()`: Returns the ID of the flag that caused the abort.

## Error Handling

`clab` uses custom exceptions to report errors during parsing. All exceptions inherit from `clab::Exception`.

- `MissingArgument`: A required argument was not provided.
- `InvalidBuilding`: The builder configuration is invalid.
- `InvalidValue`: An argument's value is not in the allowed set.
- `UnexpectedArgument`: An unexpected argument was found.
- `RedundantArgument`: A non-multiple argument was provided more than once.
- `TokenMismatch`: A flag was found where a value was expected.
- `MissingValue`: An argument expected a value, but none was provided.

It's recommended to wrap the `evaluate()` call in a `try-catch` block to handle these exceptions.

## License

`clab` is licensed under the [MIT License](LICENSE).
