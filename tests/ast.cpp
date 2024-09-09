#include <stdexcept>
#include <variant>

#include <catch_amalgamated.hpp>

#include <parser/ast.h>

TEST_CASE("AST tests", "[AST]") {
    try {
        constexpr std::string_view input = "select \"bar\" from \"foo\"";
        AST ast{input};
        auto root = ast.getRoot();
    } catch (std::runtime_error& error) {
        FAIL(error.what());
    } catch (std::bad_variant_access& error) {
        FAIL(error.what());
    }
}
