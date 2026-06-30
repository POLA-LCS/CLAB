# CLAB — Command Line Arguments Builder

**CLAB** is a header‑only C++17 library that makes command‑line parsing feel natural.  
You describe your interface with a fluent builder — flags, options, positional args, typed values, callbacks — and CLAB does the rest.

```cpp
#include "clab.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    clab::CLAB cli;

    cli.start("input")
        .flag("i").flag("input", "--")
        .consume(1).required()
        .end();

    cli.start("verbose")
        .flag("v").flag("verbose", "--")
        .end();

    cli.start("help")
        .flag("h").flag("help", "--")
        .abort()
        .end();

    try {
        auto eval = cli.evaluate(argc, argv);
        if (eval.aborted()) return 0;

        std::cout << "File: " << eval.value("input") << "\n";
        if (eval.state("verbose"))
            std::cout << "(verbose mode)\n";

    } catch (const clab::Exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
```

```
$ ./app -i data.txt -v
File: data.txt
(verbose mode)
```

---

## Quick Tour

### Flags with aliases

Give an option multiple names and prefixes:

```cpp
cli.start("port")
    .flag("p")            // -p
    .flag("port", "--")   // --port
    .consume(1)
    .end();
```

### Required options

Mandatory arguments produce a clear error when missing:

```cpp
.start("output")
    .flag("o").flag("output", "--")
    .consume(1)
    .required()
.end();
```

### Default values

Optional arguments fall back to a default when omitted:

```cpp
.start("threads")
    .flag("j")
    .consume(1)
    .initial(4)       // int default
.end();
```

### Boolean / toggle flags

Flags without a value act as booleans — query their state:

```cpp
cli.start("verbose").flag("v").end();

// later
if (eval.state("verbose")) { /* ... */ }
```

### Abort flags (`--help`, `--version`)

Flags that should stop parsing immediately:

```cpp
cli.start("help")
    .flag("h").flag("help", "--")
    .abort()
    .end();

// later
if (eval.aborted()) {
    if (eval.aborted_id() == "help") { /* show usage */ }
    return 0;
}
```

### Allowed values

Restrict an argument to a predefined set:

```cpp
cli.start("mode")
    .flag("m").flag("mode", "--")
    .consume(1, {"fast", "accurate", "balanced"})
    .initial("balanced")
    .end();
```

### Callbacks

Run custom logic the moment a flag is parsed:

```cpp
cli.start("config")
    .flag("c").flag("config", "--")
    .consume(1)
    .action([](std::string_view path) {
        std::clog << "Loading config: " << path << "\n";
    })
.end();
```

---

## Typed Parsing (Go‑Style)

Parse flags directly as `int` or `double` with validation at parse time:

```cpp
cli.start("port")
    .flag("p").flag("port", "--")
    .consume(1)
    .Int()
    .initial(8080)
    .end();

cli.start("rate")
    .flag("r").flag("rate", "--")
    .consume(1)
    .Float()
    .initial(1.0)
    .end();

cli.start("name")
    .flag("n").flag("name", "--")
    .consume(1)
    .initial(std::string("localhost"))
    .end();

// evaluate, then:
int    port = eval.Int("port");
double rate = eval.Float("rate");
std::string name = eval.as_string("name");
```

```
$ ./app --port 9090 --rate 2.5 --name myserver
9090 2.5 myserver

$ ./app                         # defaults
8080 1 localhost

$ ./app --port abc              # type error at parse time
Error: 'abc' cannot be parsed as int for 'port'
```

---

## Multi‑value & Repeatable Flags

### Collect multiple values

```cpp
cli.start("include")
    .flag("I")
    .multiple()       // allow -I/usr/include -I./src -I.
    .end();

// eval.list("include") → vector<string>
```

### Override mode

Keep the *last* value when the same flag appears more than once:

```cpp
cli.start("output")
    .flag("o")
    .consume(1)
    .over()           // -o a -o b → value is "b"
    .end();
```

---

## Error Handling

CLAB throws descriptive exceptions — all inherit from `clab::Exception`:

| Exception             | When                                                   |
|-----------------------|--------------------------------------------------------|
| `MissingArgument`     | A required argument was not provided                   |
| `InvalidValue`        | Value is not in the allowed set                        |
| `UnexpectedArgument`  | An unknown flag was encountered                        |
| `RedundantArgument`   | A non‑multiple flag was given more than once           |
| `MissingValue`        | A flag expected a value but none followed              |
| `TypeConversion`      | Value can't be parsed as the declared type (int/double)|
| `InvalidBuilding`     | Builder configuration is malformed                     |
| `TokenMismatch`       | A flag appeared where a value was expected             |

Wrap `evaluate()` in a `try`/`catch` block and you're covered.

---

## Getting Started

CLAB is **header‑only** — just grab `clab.hpp` and include it:

```cpp
#include "clab.hpp"
```

No build steps, no dependencies, no macros. Drop it in your project and go.

---

## License

[MIT](LICENSE)
