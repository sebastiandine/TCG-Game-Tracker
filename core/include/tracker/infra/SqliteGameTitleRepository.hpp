#pragma once

// SqliteGameTitleRepository: IGameTitleRepository backed by a SqliteDatabase.

#include "tracker/ports/IGameTitleRepository.hpp"

namespace tracker {

class SqliteDatabase;

class SqliteGameTitleRepository final : public IGameTitleRepository {
public:
    explicit SqliteGameTitleRepository(SqliteDatabase& db);

    [[nodiscard]] Result<GameTitle> create(const std::string& name) override;
    [[nodiscard]] Result<std::vector<GameTitle>> listAll() override;

private:
    SqliteDatabase& db_;
};

}  // namespace tracker
