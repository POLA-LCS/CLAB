#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../clab.hpp"

using namespace clab;

// ---------------------------------------------------------------------------
// Helper: build a Vector<String> from initializer list (simulates argv)
// ---------------------------------------------------------------------------
static Vector<String> args(std::initializer_list<String> list) {
    return Vector<String>(list);
}

// ===========================================================================
// 1. Basic tagged flags
// ===========================================================================

TEST_CASE("Single tagged flag with value") {
    CLAB cli;
    cli.start("output")
        .flag("o").flag("output", "--")
        .consume(1)
    .end();

    SUBCASE("-o path") {
        auto eval = cli.evaluate(args({"-o", "build/"}));
        CHECK(eval.state("output") == true);
        CHECK(eval.value("output") == "build/");
    }

    SUBCASE("--output path") {
        auto eval = cli.evaluate(args({"--output", "dist/"}));
        CHECK(eval.state("output") == true);
        CHECK(eval.value("output") == "dist/");
    }
}

TEST_CASE("Flag without value (boolean toggle)") {
    CLAB cli;
    cli.start("verbose")
        .flag("v").flag("verbose", "--")
    .end();

    auto eval = cli.evaluate(args({"-v"}));
    CHECK(eval.state("verbose") == true);
}

TEST_CASE("Unknown flag is not set") {
    CLAB cli;
    cli.start("verbose")
        .flag("v")
    .end();

    auto eval = cli.evaluate(args({}));
    CHECK(eval.state("verbose") == false);
}

// ===========================================================================
// 2. Required flags
// ===========================================================================

TEST_CASE("Required flag missing throws MissingArgument") {
    CLAB cli;
    cli.start("input")
        .flag("i")
        .consume(1).required()
    .end();

    CHECK_THROWS_AS(cli.evaluate(args({})), MissingArgument);
}

TEST_CASE("Required flag present does not throw") {
    CLAB cli;
    cli.start("input")
        .flag("i")
        .consume(1).required()
    .end();

    CHECK_NOTHROW(cli.evaluate(args({"-i", "file.txt"})));
}

// ===========================================================================
// 3. Default values
// ===========================================================================

TEST_CASE("Default value is used when flag is absent") {
    CLAB cli;
    // NOTE: initial("release") would match initial(bool) due to const char* -> bool
    // implicit conversion ranking higher than const char* -> std::string.
    // Workaround: wrap in String() explicitly.
    cli.start("mode")
        .flag("m")
        .consume(1)
        .initial(String("release"))
    .end();

    auto eval = cli.evaluate(args({}));
    CHECK(eval.value("mode") == "release");
}

TEST_CASE("Default value is overridden when flag is provided") {
    CLAB cli;
    cli.start("mode")
        .flag("m")
        .consume(1)
        .initial(String("release"))
    .end();

    auto eval = cli.evaluate(args({"-m", "debug"}));
    CHECK(eval.value("mode") == "debug");
}

TEST_CASE("Default toggle state") {
    CLAB cli;
    cli.start("opt")
        .flag("o")
        .initial(true)
    .end();

    auto eval = cli.evaluate(args({}));
    CHECK(eval.state("opt") == true);
}

// ===========================================================================
// 4. Abort flags
// ===========================================================================

TEST_CASE("Abort flag stops parsing early") {
    CLAB cli;
    cli.start("input")
        .flag("i")
        .consume(1).required()
    .end();
    cli.start("help")
        .flag("h").flag("help", "--")
        .abort()
    .end();

    auto eval = cli.evaluate(args({"-h"}));
    CHECK(eval.aborted() == true);
    CHECK(eval.aborted_id() == "help");
    // Required "input" is NOT enforced when abort fires
}

TEST_CASE("Abort flag triggers action callback") {
    bool called = false;
    CLAB cli;
    cli.start("help")
        .flag("h")
        .abort()
        .action([&](const String&) { called = true; })
    .end();

    cli.evaluate(args({"-h"}));
    CHECK(called == true);
}

// ===========================================================================
// 5. Allowed values (enumeration)
// ===========================================================================

TEST_CASE("Allowed values accepted") {
    CLAB cli;
    cli.start("level")
        .flag("l")
        .consume(1, {"debug", "info", "warn", "error"})
    .end();

    auto eval = cli.evaluate(args({"-l", "info"}));
    CHECK(eval.value("level") == "info");
}

TEST_CASE("Disallowed value throws InvalidValue") {
    CLAB cli;
    cli.start("level")
        .flag("l")
        .consume(1, {"debug", "info", "warn", "error"})
    .end();

    CHECK_THROWS_AS(cli.evaluate(args({"-l", "trace"})), InvalidValue);
}

// ===========================================================================
// 6. Multiple occurrences
// ===========================================================================

TEST_CASE("Multiple flag collects all values") {
    CLAB cli;
    cli.start("include")
        .flag("I")
        .consume(1).multiple()
    .end();

    auto eval = cli.evaluate(args({"-I", "src/", "-I", "lib/"}));
    CHECK(eval.list("include").size() == 2);
    CHECK(eval.list("include")[0] == "src/");
    CHECK(eval.list("include")[1] == "lib/");
}

TEST_CASE("Duplicate non-multiple flag throws RedundantArgument") {
    CLAB cli;
    cli.start("output")
        .flag("o")
        .consume(1)
    .end();

    CHECK_THROWS_AS(cli.evaluate(args({"-o", "a", "-o", "b"})), RedundantArgument);
}

// ===========================================================================
// 7. Positional arguments
// ===========================================================================

TEST_CASE("Positional argument captures value") {
    CLAB cli;
    cli.start("file")
        .consume(1)
    .end();

    auto eval = cli.evaluate(args({"hello.txt"}));
    CHECK(eval.state("file") == true);
    CHECK(eval.value("file") == "hello.txt");
}

TEST_CASE("Positional multiple captures remaining args") {
    CLAB cli;
    cli.start("files")
        .multiple()
    .end();

    auto eval = cli.evaluate(args({"a.txt", "b.txt", "c.txt"}));
    CHECK(eval.list("files").size() == 3);
}

// ===========================================================================
// 8. Action callbacks
// ===========================================================================

TEST_CASE("Action callback is invoked with parsed value") {
    String captured;
    CLAB cli;
    cli.start("name")
        .flag("n")
        .consume(1)
        .action([&](const String& v) { captured = v; })
    .end();

    cli.evaluate(args({"-n", "pola"}));
    CHECK(captured == "pola");
}

// ===========================================================================
// 9. Toggle states
// ===========================================================================

TEST_CASE("Toggle with explicit true/false tags") {
    CLAB cli;
    cli.start("color")
        .toggle(true, "color", "--")
        .toggle(false, "no-color", "--")
    .end();

    SUBCASE("--color sets true") {
        auto eval = cli.evaluate(args({"--color"}));
        CHECK(eval.state("color") == true);
    }

    SUBCASE("--no-color sets false") {
        auto eval = cli.evaluate(args({"--no-color"}));
        CHECK(eval.state("color") == false);
    }
}

// ===========================================================================
// 10. Custom prefixes
// ===========================================================================

TEST_CASE("Custom prefix works") {
    CLAB cli;
    cli.start("help")
        .flag("help", "/")
    .end();

    auto eval = cli.evaluate(args({"/help"}));
    CHECK(eval.state("help") == true);
}

// ===========================================================================
// 11. Error conditions
// ===========================================================================

TEST_CASE("Unexpected argument throws") {
    CLAB cli;
    cli.start("known")
        .flag("k")
    .end();

    CHECK_THROWS_AS(cli.evaluate(args({"unknown"})), UnexpectedArgument);
}

TEST_CASE("Missing consumed value throws MissingValue") {
    CLAB cli;
    cli.start("output")
        .flag("o")
        .consume(1)
    .end();

    CHECK_THROWS_AS(cli.evaluate(args({"-o"})), MissingValue);
}

TEST_CASE("Flag where value expected throws TokenMismatch") {
    CLAB cli;
    cli.start("output")
        .flag("o")
        .consume(1)
    .end();
    cli.start("verbose")
        .flag("v")
    .end();

    CHECK_THROWS_AS(cli.evaluate(args({"-o", "-v"})), TokenMismatch);
}

// ===========================================================================
// 12. Builder validation
// ===========================================================================

TEST_CASE("Positional with consume + multiple throws InvalidBuilding") {
    CLAB cli;
    CHECK_THROWS_AS(
        cli.start("bad")
            .consume(2).multiple()
        .end(),
        InvalidBuilding
    );
}

// ===========================================================================
// 13. Shorthand constructor
// ===========================================================================

TEST_CASE("CLAB(path_id) shorthand creates required consume(1)") {
    CLAB cli("path");

    auto eval = cli.evaluate(args({"myfile.txt"}));
    CHECK(eval.value("path") == "myfile.txt");

    CLAB cli2("path");
    CHECK_THROWS_AS(cli2.evaluate(args({})), MissingArgument);
}

// ===========================================================================
// 14. Over (override) mode
// ===========================================================================

TEST_CASE("Over mode allows re-specifying and keeps last value") {
    CLAB cli;
    cli.start("output")
        .flag("o")
        .consume(1).over()
    .end();

    auto eval = cli.evaluate(args({"-o", "first", "-o", "second"}));
    CHECK(eval.value("output") == "second");
}

// ===========================================================================
// 15. Mixed tagged + positional
// ===========================================================================

TEST_CASE("Tagged and positional arguments together") {
    CLAB cli;
    cli.start("verbose")
        .flag("v")
    .end();
    cli.start("file")
        .consume(1)
    .end();

    auto eval = cli.evaluate(args({"-v", "main.cpp"}));
    CHECK(eval.state("verbose") == true);
    CHECK(eval.value("file") == "main.cpp");
}

// ===========================================================================
// 16. Empty input
// ===========================================================================

TEST_CASE("Empty args with no required flags succeeds") {
    CLAB cli;
    cli.start("opt")
        .flag("o")
    .end();

    auto eval = cli.evaluate(args({}));
    CHECK(eval.state("opt") == false);
    CHECK(eval.aborted() == false);
}

// ===========================================================================
// 17. Evaluation::list for missing id
// ===========================================================================

TEST_CASE("list() for unknown id returns empty vector") {
    CLAB cli;
    auto eval = cli.evaluate(args({}));
    CHECK(eval.list("nonexistent").empty());
}

// ===========================================================================
// 18. Evaluation::handle
// ===========================================================================

TEST_CASE("handle() returns nullptr for unknown id") {
    CLAB cli;
    auto eval = cli.evaluate(args({}));
    CHECK(eval.handle("nope") == nullptr);
}

TEST_CASE("handle() returns valid shared_ptr for known id") {
    CLAB cli;
    cli.start("flag")
        .flag("f")
    .end();

    auto eval = cli.evaluate(args({"-f"}));
    auto h = eval.handle("flag");
    REQUIRE(h != nullptr);
    CHECK(h->state == true);
}
