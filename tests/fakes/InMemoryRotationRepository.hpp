#pragma once

// InMemoryRotationRepository: test fake backed by a std::vector.

#include "tracker/ports/IRotationRepository.hpp"

#include <algorithm>
#include <cctype>
#include <vector>

namespace tracker::testing {

class InMemoryRotationRepository final : public IRotationRepository {
public:
    Result<Rotation> create(std::int64_t formatId,
                            const std::string& name,
                            int startYear,
                            int startMonth,
                            int endYear,
                            int endMonth) override {
        for (const auto& rotation : rotations_) {
            if (rotation.formatId == formatId &&
                equalsIgnoreCase(rotation.name, name)) {
                return Result<Rotation>::err(
                    "A rotation named \"" + name + "\" already exists.");
            }
        }
        if (const char* error =
                rotationSpanError(startYear, startMonth, endYear, endMonth)) {
            return Result<Rotation>::err(error);
        }

        Rotation rotation;
        rotation.id = nextId_++;
        rotation.formatId = formatId;
        rotation.name = name;
        rotation.startYear = startYear;
        rotation.startMonth = startMonth;
        rotation.endYear = endYear;
        rotation.endMonth = endMonth;
        rotations_.push_back(rotation);
        return Result<Rotation>::ok(rotation);
    }

    Result<std::vector<Rotation>> listByFormat(std::int64_t formatId) override {
        std::vector<Rotation> out;
        for (const auto& rotation : rotations_) {
            if (rotation.formatId == formatId) out.push_back(rotation);
        }
        std::sort(out.begin(), out.end(), [](const Rotation& a, const Rotation& b) {
            if (a.startYear != b.startYear) return a.startYear < b.startYear;
            if (a.startMonth != b.startMonth) return a.startMonth < b.startMonth;
            return a.id < b.id;
        });
        return Result<std::vector<Rotation>>::ok(std::move(out));
    }

    Result<void> markExpiryPrompted(std::int64_t rotationId) override {
        for (auto& rotation : rotations_) {
            if (rotation.id == rotationId) {
                rotation.expiryPrompted = true;
                return Result<void>::ok();
            }
        }
        return Result<void>::err("Rotation not found.");
    }

private:
    static bool equalsIgnoreCase(const std::string& a, const std::string& b) {
        if (a.size() != b.size()) return false;
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(a[i])) !=
                std::tolower(static_cast<unsigned char>(b[i]))) {
                return false;
            }
        }
        return true;
    }

    std::int64_t nextId_{1};
    std::vector<Rotation> rotations_;
};

}  // namespace tracker::testing
