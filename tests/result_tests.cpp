#include <doctest/doctest.h>

#include "tracker/util/Result.hpp"

using namespace tracker;

TEST_SUITE("Result") {
    TEST_CASE("ok result holds value") {
        auto r = Result<int>::ok(42);
        CHECK(r.isOk());
        CHECK_FALSE(r.isErr());
        CHECK(static_cast<bool>(r));
        CHECK(r.value() == 42);
    }

    TEST_CASE("err result holds error") {
        auto r = Result<int>::err("bad");
        CHECK(r.isErr());
        CHECK_FALSE(r.isOk());
        CHECK_FALSE(static_cast<bool>(r));
        CHECK(r.error() == "bad");
    }

    TEST_CASE("value on err throws") {
        auto r = Result<int>::err("oops");
        CHECK_THROWS_AS(r.value(), std::logic_error);
    }

    TEST_CASE("error on ok throws") {
        auto r = Result<int>::ok(1);
        CHECK_THROWS_AS(r.error(), std::logic_error);
    }

    TEST_CASE("valueOr returns fallback on err") {
        auto r = Result<int>::err("fail");
        CHECK(r.valueOr(99) == 99);
    }

    TEST_CASE("valueOr returns value on ok") {
        auto r = Result<int>::ok(7);
        CHECK(r.valueOr(99) == 7);
    }

    TEST_CASE("copy semantics") {
        auto a = Result<std::string>::ok("hello");
        auto b = a;  // NOLINT(performance-unnecessary-copy-initialization)
        CHECK(b.value() == "hello");
        CHECK(a.value() == "hello");
    }

    TEST_CASE("move semantics") {
        auto a = Result<std::string>::ok("hello");
        auto b = std::move(a);
        CHECK(b.value() == "hello");
    }

    TEST_CASE("void ok result") {
        auto r = Result<void>::ok();
        CHECK(r.isOk());
        CHECK_FALSE(r.isErr());
    }

    TEST_CASE("void err result") {
        auto r = Result<void>::err("problem");
        CHECK(r.isErr());
        CHECK(r.error() == "problem");
    }

    TEST_CASE("void error on ok throws") {
        auto r = Result<void>::ok();
        CHECK_THROWS_AS(r.error(), std::logic_error);
    }
}
