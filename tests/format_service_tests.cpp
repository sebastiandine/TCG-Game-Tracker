#include <doctest/doctest.h>

#include "fakes/InMemoryFormatRepository.hpp"
#include "fakes/InMemoryRotationRepository.hpp"
#include "tracker/domain/Rotation.hpp"
#include "tracker/services/FormatService.hpp"

using namespace tracker;

TEST_SUITE("FormatService") {
    TEST_CASE("create trims whitespace") {
        testing::InMemoryFormatRepository repo;
        testing::InMemoryRotationRepository rotations;
        FormatService svc(repo, rotations);

        auto result = svc.create(1, "  Standard  ");
        REQUIRE(result.isOk());
        CHECK(result.value().name == "Standard");
        CHECK(result.value().gameId == 1);
    }

    TEST_CASE("create rejects missing game") {
        testing::InMemoryFormatRepository repo;
        testing::InMemoryRotationRepository rotations;
        FormatService svc(repo, rotations);

        auto result = svc.create(0, "Standard");
        CHECK(result.isErr());
        CHECK(result.error() == "No game selected.");
    }

    TEST_CASE("create rejects empty name") {
        testing::InMemoryFormatRepository repo;
        testing::InMemoryRotationRepository rotations;
        FormatService svc(repo, rotations);

        auto result = svc.create(1, "");
        CHECK(result.isErr());
        CHECK(result.error() == "Format name must not be empty.");
    }

    TEST_CASE("create rejects whitespace-only name") {
        testing::InMemoryFormatRepository repo;
        testing::InMemoryRotationRepository rotations;
        FormatService svc(repo, rotations);

        auto result = svc.create(1, "   ");
        CHECK(result.isErr());
        CHECK(result.error() == "Format name must not be empty.");
    }

    TEST_CASE("create delegates to repository") {
        testing::InMemoryFormatRepository repo;
        testing::InMemoryRotationRepository rotations;
        FormatService svc(repo, rotations);

        auto r1 = svc.create(1, "Standard");
        REQUIRE(r1.isOk());
        CHECK(r1.value().id == 1);

        auto r2 = svc.create(1, "Commander");
        REQUIRE(r2.isOk());
        CHECK(r2.value().id == 2);

        auto list = svc.listByGame(1);
        REQUIRE(list.isOk());
        CHECK(list.value().size() == 2);
        CHECK(list.value()[0].name == "Standard");
        CHECK(list.value()[1].name == "Commander");
    }

    TEST_CASE("create rejects duplicate name in the same game") {
        testing::InMemoryFormatRepository repo;
        testing::InMemoryRotationRepository rotations;
        FormatService svc(repo, rotations);

        auto r1 = svc.create(1, "Standard");
        REQUIRE(r1.isOk());

        auto r2 = svc.create(1, "standard");
        CHECK(r2.isErr());
    }

    TEST_CASE("same format name is allowed in different games") {
        testing::InMemoryFormatRepository repo;
        testing::InMemoryRotationRepository rotations;
        FormatService svc(repo, rotations);

        auto r1 = svc.create(1, "Standard");
        REQUIRE(r1.isOk());

        auto r2 = svc.create(2, "Standard");
        REQUIRE(r2.isOk());
        CHECK(r2.value().gameId == 2);

        auto g1 = svc.listByGame(1);
        REQUIRE(g1.isOk());
        CHECK(g1.value().size() == 1);

        auto g2 = svc.listByGame(2);
        REQUIRE(g2.isOk());
        CHECK(g2.value().size() == 1);
        CHECK(g2.value()[0].name == "Standard");
    }

    TEST_CASE("non-rotating create stores no rotation") {
        testing::InMemoryFormatRepository repo;
        testing::InMemoryRotationRepository rotations;
        FormatService svc(repo, rotations);

        auto created = svc.create(1, "Standard");
        REQUIRE(created.isOk());
        CHECK_FALSE(created.value().rotating);
        auto listed = svc.listRotations(created.value().id);
        REQUIRE(listed.isOk());
        CHECK(listed.value().empty());
    }

    TEST_CASE("rotating create stores the first rotation") {
        testing::InMemoryFormatRepository repo;
        testing::InMemoryRotationRepository rotations;
        FormatService svc(repo, rotations);

        auto created = svc.createRotating(1, " Standard ", "  Bloomburrow  ",
                                          2024, 7, 2024, 10);
        REQUIRE(created.isOk());
        CHECK(created.value().format.rotating);
        CHECK(created.value().format.name == "Standard");
        CHECK(created.value().rotation.name == "Bloomburrow");
        CHECK(created.value().rotation.formatId == created.value().format.id);
        CHECK(created.value().rotation.startMonth == 7);
        CHECK(created.value().rotation.endMonth == 10);

        auto listed = svc.listRotations(created.value().format.id);
        REQUIRE(listed.isOk());
        REQUIRE(listed.value().size() == 1);
        CHECK(listed.value()[0].name == "Bloomburrow");
    }

    TEST_CASE("create rotation rejects a non-rotating format") {
        testing::InMemoryFormatRepository repo;
        testing::InMemoryRotationRepository rotations;
        FormatService svc(repo, rotations);

        auto format = svc.create(1, "Commander");
        REQUIRE(format.isOk());
        auto rotation = svc.createRotation(format.value().id, "Next",
                                           2025, 1, 2025, 3);
        CHECK(rotation.isErr());
        CHECK(rotation.error() ==
              "Rotations can only be added to a rotating format.");
    }

    TEST_CASE("create rotation rejects a duplicate name and an inverted span") {
        testing::InMemoryFormatRepository repo;
        testing::InMemoryRotationRepository rotations;
        FormatService svc(repo, rotations);

        auto created = svc.createRotating(1, "Standard", "First",
                                          2024, 1, 2024, 3);
        REQUIRE(created.isOk());
        auto duplicate = svc.createRotation(created.value().format.id, "first",
                                            2024, 4, 2024, 6);
        CHECK(duplicate.isErr());
        CHECK(duplicate.error() == "A rotation named \"first\" already exists.");

        auto inverted = svc.createRotation(created.value().format.id, "Second",
                                           2024, 8, 2024, 4);
        CHECK(inverted.isErr());
        CHECK(inverted.error() == "Rotation end is before the start.");
    }

    TEST_CASE("rotations list by start date and latest follows that order") {
        testing::InMemoryFormatRepository repo;
        testing::InMemoryRotationRepository rotations;
        FormatService svc(repo, rotations);

        auto created = svc.createRotating(1, "Standard", "Middle",
                                          2024, 6, 2024, 8);
        REQUIRE(created.isOk());
        auto early = svc.createRotation(created.value().format.id, "Early",
                                        2024, 1, 2024, 3);
        REQUIRE(early.isOk());
        auto late = svc.createRotation(created.value().format.id, "Late",
                                       2025, 1, 2025, 3);
        REQUIRE(late.isOk());

        auto listed = svc.listRotations(created.value().format.id);
        REQUIRE(listed.isOk());
        REQUIRE(listed.value().size() == 3);
        CHECK(listed.value()[0].name == "Early");
        CHECK(listed.value()[1].name == "Middle");
        CHECK(listed.value()[2].name == "Late");

        const auto latest = latestRotation(listed.value());
        REQUIRE(latest.has_value());
        CHECK(latest->name == "Late");
        CHECK(rotationEnded(*latest, 2025, 4));
        CHECK_FALSE(rotationEnded(*latest, 2025, 3));
    }

    TEST_CASE("markExpiryPrompted is remembered") {
        testing::InMemoryFormatRepository repo;
        testing::InMemoryRotationRepository rotations;
        FormatService svc(repo, rotations);

        auto created = svc.createRotating(1, "Standard", "First",
                                          2024, 1, 2024, 3);
        REQUIRE(created.isOk());
        REQUIRE(svc.markExpiryPrompted(created.value().rotation.id).isOk());
        auto listed = svc.listRotations(created.value().format.id);
        REQUIRE(listed.isOk());
        CHECK(listed.value()[0].expiryPrompted);
    }

    TEST_CASE("rotation end may be unknown and can be edited later") {
        testing::InMemoryFormatRepository repo;
        testing::InMemoryRotationRepository rotations;
        FormatService svc(repo, rotations);

        auto created = svc.createRotating(1, "Standard", " Current ",
                                          2025, 1, std::nullopt, std::nullopt);
        REQUIRE(created.isOk());
        CHECK_FALSE(created.value().rotation.endYear.has_value());
        CHECK_FALSE(created.value().rotation.endMonth.has_value());
        CHECK_FALSE(rotationEnded(created.value().rotation, 2026, 6));

        const auto formatId = created.value().format.id;
        const auto rotationId = created.value().rotation.id;
        auto updated = svc.updateRotation(formatId, rotationId, "Current",
                                          2025, 1, 2025, 9);
        REQUIRE(updated.isOk());
        CHECK(updated.value().endYear == 2025);
        CHECK(updated.value().endMonth == 9);
        CHECK(rotationEnded(updated.value(), 2025, 10));
        CHECK_FALSE(rotationEnded(updated.value(), 2025, 9));

        auto cleared = svc.updateRotation(formatId, rotationId, " Renamed ",
                                          2025, 2, std::nullopt, std::nullopt);
        REQUIRE(cleared.isOk());
        CHECK(cleared.value().name == "Renamed");
        CHECK(cleared.value().startMonth == 2);
        CHECK_FALSE(cleared.value().endYear.has_value());
        CHECK_FALSE(rotationEnded(cleared.value(), 2030, 1));
    }

    TEST_CASE("update rotation rejects a duplicate name and an inverted span") {
        testing::InMemoryFormatRepository repo;
        testing::InMemoryRotationRepository rotations;
        FormatService svc(repo, rotations);

        auto created = svc.createRotating(1, "Standard", "First",
                                          2024, 1, 2024, 3);
        REQUIRE(created.isOk());
        auto second = svc.createRotation(created.value().format.id, "Second",
                                         2024, 4, 2024, 6);
        REQUIRE(second.isOk());

        auto duplicate = svc.updateRotation(
            created.value().format.id, second.value().id, "first",
            2024, 4, 2024, 6);
        CHECK(duplicate.isErr());
        CHECK(duplicate.error() == "A rotation named \"first\" already exists.");

        auto inverted = svc.updateRotation(
            created.value().format.id, second.value().id, "Second",
            2024, 8, 2024, 4);
        CHECK(inverted.isErr());
        CHECK(inverted.error() == "Rotation end is before the start.");

        auto half = svc.createRotation(created.value().format.id, "Half",
                                       2024, 7, 2024, std::nullopt);
        CHECK(half.isErr());
        CHECK(half.error() ==
              "Rotation end month and year must both be set or both be unknown.");
    }

    TEST_CASE("changing the end clears the expiry prompt") {
        testing::InMemoryFormatRepository repo;
        testing::InMemoryRotationRepository rotations;
        FormatService svc(repo, rotations);

        auto created = svc.createRotating(1, "Standard", "First",
                                          2024, 1, 2024, 3);
        REQUIRE(created.isOk());
        const auto formatId = created.value().format.id;
        const auto rotationId = created.value().rotation.id;
        REQUIRE(svc.markExpiryPrompted(rotationId).isOk());

        auto renamed = svc.updateRotation(formatId, rotationId, "Opening",
                                          2024, 1, 2024, 3);
        REQUIRE(renamed.isOk());
        CHECK(renamed.value().expiryPrompted);

        auto extended = svc.updateRotation(formatId, rotationId, "Opening",
                                           2024, 1, 2024, 6);
        REQUIRE(extended.isOk());
        CHECK_FALSE(extended.value().expiryPrompted);
    }

    TEST_CASE("update rotation rejects a missing row and a non-rotating format") {
        testing::InMemoryFormatRepository repo;
        testing::InMemoryRotationRepository rotations;
        FormatService svc(repo, rotations);

        auto format = svc.create(1, "Commander");
        REQUIRE(format.isOk());
        auto blocked = svc.updateRotation(format.value().id, 1, "Nope",
                                          2024, 1, 2024, 2);
        CHECK(blocked.isErr());
        CHECK(blocked.error() ==
              "Rotations can only be changed on a rotating format.");

        auto created = svc.createRotating(1, "Standard", "First",
                                          2024, 1, 2024, 3);
        REQUIRE(created.isOk());
        auto missing = svc.updateRotation(created.value().format.id, 999, "Nope",
                                          2024, 1, 2024, 2);
        CHECK(missing.isErr());
        CHECK(missing.error() == "Rotation not found.");
    }
}
