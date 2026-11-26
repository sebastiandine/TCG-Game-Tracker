#pragma once

// SqliteRotationRepository: IRotationRepository backed by a SqliteDatabase.

#include "tracker/ports/IRotationRepository.hpp"

namespace tracker {

class SqliteDatabase;

class SqliteRotationRepository final : public IRotationRepository {
public:
    explicit SqliteRotationRepository(SqliteDatabase& db);

    [[nodiscard]] Result<Rotation> create(std::int64_t formatId,
                                          const std::string& name,
                                          int startYear,
                                          int startMonth,
                                          int endYear,
                                          int endMonth) override;

    [[nodiscard]] Result<std::vector<Rotation>> listByFormat(
        std::int64_t formatId) override;

    [[nodiscard]] Result<void> markExpiryPrompted(
        std::int64_t rotationId) override;

private:
    SqliteDatabase& db_;
};

}  // namespace tracker
