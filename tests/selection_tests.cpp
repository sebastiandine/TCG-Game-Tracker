#include <doctest/doctest.h>

#include "tracker/domain/Selection.hpp"

using namespace tracker;

namespace {

struct Item {
    std::int64_t id{0};
};

}  // namespace

TEST_SUITE("resolveSavedOrSoleId") {
    TEST_CASE("empty list returns 0") {
        CHECK(resolveSavedOrSoleId(std::vector<Item>{}, 0) == 0);
        CHECK(resolveSavedOrSoleId(std::vector<Item>{}, 9) == 0);
    }

    TEST_CASE("saved id is kept when it is still present") {
        const std::vector<Item> items{{3}, {9}, {12}};
        CHECK(resolveSavedOrSoleId(items, 9) == 9);
    }

    TEST_CASE("missing saved id falls back to the only item") {
        const std::vector<Item> items{{7}};
        CHECK(resolveSavedOrSoleId(items, 9) == 7);
        CHECK(resolveSavedOrSoleId(items, 0) == 7);
    }

    TEST_CASE("missing saved id with several items returns 0") {
        const std::vector<Item> items{{3}, {9}};
        CHECK(resolveSavedOrSoleId(items, 12) == 0);
        CHECK(resolveSavedOrSoleId(items, 0) == 0);
    }
}
