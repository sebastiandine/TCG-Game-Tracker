#include <doctest/doctest.h>

#include "tracker/domain/DeckGroup.hpp"

using namespace tracker;

TEST_SUITE("groupDecksByName") {
    TEST_CASE("empty input returns empty output") {
        auto groups = groupDecksByName({});
        CHECK(groups.empty());
    }

    TEST_CASE("single deck produces one group with one variant") {
        Deck d;
        d.id = 1; d.formatId = 1; d.archetypeId = 1;
        d.name = "Jund"; d.variant = ""; d.variantNote = "";

        auto groups = groupDecksByName({d});
        REQUIRE(groups.size() == 1);
        CHECK(groups[0].name == "Jund");
        REQUIRE(groups[0].variants.size() == 1);
        CHECK(groups[0].variants[0].id == 1);
    }

    TEST_CASE("same name collapses into one group") {
        Deck d1{1, 1, 3, "Landstill", "", ""};
        Deck d2{2, 1, 3, "Landstill", "Glaciers", "ice"};
        Deck d3{3, 1, 3, "Landstill", "Mage/Angel", ""};

        auto groups = groupDecksByName({d3, d1, d2});
        REQUIRE(groups.size() == 1);
        CHECK(groups[0].name == "Landstill");
        REQUIRE(groups[0].variants.size() == 3);
        // Sorted by variant: "" < "Glaciers" < "Mage/Angel"
        CHECK(groups[0].variants[0].variant.empty());
        CHECK(groups[0].variants[1].variant == "Glaciers");
        CHECK(groups[0].variants[2].variant == "Mage/Angel");
    }

    TEST_CASE("different names stay separate and are sorted") {
        Deck d1{1, 1, 1, "Shops", "", ""};
        Deck d2{2, 1, 3, "Landstill", "", ""};
        Deck d3{3, 1, 4, "PO Storm", "", ""};

        auto groups = groupDecksByName({d1, d3, d2});
        REQUIRE(groups.size() == 3);
        CHECK(groups[0].name == "Landstill");
        CHECK(groups[1].name == "PO Storm");
        CHECK(groups[2].name == "Shops");
    }

    TEST_CASE("grouping is case-insensitive") {
        Deck d1{1, 1, 1, "Jund", "", ""};
        Deck d2{2, 1, 1, "jund", "Saga", ""};

        auto groups = groupDecksByName({d2, d1});
        REQUIRE(groups.size() == 1);
        REQUIRE(groups[0].variants.size() == 2);
        CHECK(groups[0].variants[0].variant.empty());
        CHECK(groups[0].variants[1].variant == "Saga");
    }
}
