#include "tracker/services/GameImportService.hpp"

#include "../import/ImportUtil.hpp"

namespace tracker {

using import_detail::toLower;

GameImportService::GameImportService(DeckService& decks,
                                     GameTypeService& gameTypes,
                                     GameService& games)
    : decks_(decks),
      gameTypes_(gameTypes),
      games_(games) {}

std::string GameImportService::deckKey(const std::string& name,
                                       const std::string& variant) {
    return toLower(name) + '\0' + toLower(variant);
}

std::optional<std::int64_t> GameImportService::inferArchetypeFromName(
        const std::string& name) const {
    const std::string haystack = toLower(name);
    std::optional<std::int64_t> best;
    std::size_t bestLen = 0;
    for (const auto& a : archetypes_) {
        if (a.name.empty()) continue;
        const std::string needle = toLower(a.name);
        if (haystack.find(needle) == std::string::npos) continue;
        if (needle.size() > bestLen) {
            bestLen = needle.size();
            best = a.id;
        }
    }
    return best;
}

Result<Deck> GameImportService::ensureCoreDeck(
        std::int64_t formatId,
        const std::string& name,
        IGameImportSink& sink,
        GameImportReport& report) {
    const std::string key = deckKey(name, "");
    auto it = deckByKey_.find(key);
    if (it != deckByKey_.end()) return Result<Deck>::ok(it->second);

    const std::string declinedKey = toLower(name);
    if (declinedCores_.count(declinedKey) != 0) {
        return Result<Deck>::err(
            "Skipped creating deck \"" + name + "\".");
    }

    auto inferred = inferArchetypeFromName(name);
    Result<std::int64_t> archetype = inferred
        ? Result<std::int64_t>::ok(*inferred)
        : sink.chooseArchetype(name);
    if (!archetype) {
        declinedCores_.insert(declinedKey);
        return Result<Deck>::err(archetype.error());
    }

    auto created = decks_.create(formatId, archetype.value(), name, "", "");
    if (!created) return created;
    ++report.decksCreated;
    deckByKey_.emplace(key, created.value());
    return created;
}

Result<Deck> GameImportService::ensurePlayerDeck(
        std::int64_t formatId,
        const std::string& name,
        const std::string& variant,
        IGameImportSink& sink,
        GameImportReport& report) {
    auto core = ensureCoreDeck(formatId, name, sink, report);
    if (!core) return core;
    if (variant.empty()) return core;

    const std::string key = deckKey(name, variant);
    auto it = deckByKey_.find(key);
    if (it != deckByKey_.end()) return Result<Deck>::ok(it->second);

    auto created = decks_.create(formatId, core.value().archetypeId, name,
                                 variant, "");
    if (!created) return created;
    ++report.decksCreated;
    deckByKey_.emplace(key, created.value());
    return created;
}

Result<GameType> GameImportService::ensureGameType(std::int64_t gameId,
                                                    const std::string& name,
                                                    IGameImportSink& sink,
                                                    GameImportReport& report) {
    const std::string key = toLower(name);
    auto it = gameTypeByName_.find(key);
    if (it != gameTypeByName_.end()) return Result<GameType>::ok(it->second);

    if (declinedGameTypes_.count(key) != 0) {
        return Result<GameType>::err(
            "Skipped creating event \"" + name + "\".");
    }

    auto choice = sink.chooseGameType(name);
    if (!choice) {
        declinedGameTypes_.insert(key);
        return Result<GameType>::err(choice.error());
    }

    auto created = gameTypes_.create(gameId, name,
                                     choice.value().competitiveness,
                                     choice.value().medium);
    if (!created) return created;
    ++report.gameTypesCreated;
    gameTypeByName_.emplace(key, created.value());
    return created;
}

Result<GameImportReport> GameImportService::import(
        std::int64_t formatId,
        std::int64_t gameId,
        const std::vector<GameImportRow>& rows,
        IGameImportSink& sink) {
    if (formatId <= 0) {
        return Result<GameImportReport>::err("No format selected.");
    }
    if (gameId <= 0) {
        return Result<GameImportReport>::err("No game selected.");
    }

    GameImportReport report;
    deckByKey_.clear();
    gameTypeByName_.clear();
    declinedCores_.clear();
    declinedGameTypes_.clear();
    archetypes_.clear();

    auto listedArchetypes = decks_.listByGame(gameId);
    if (!listedArchetypes) {
        return Result<GameImportReport>::err(listedArchetypes.error());
    }
    archetypes_ = std::move(listedArchetypes.value());

    auto existingDecks = decks_.listByFormat(formatId);
    if (!existingDecks) {
        return Result<GameImportReport>::err(existingDecks.error());
    }
    for (const auto& d : existingDecks.value()) {
        deckByKey_.emplace(deckKey(d.name, d.variant), d);
    }

    auto existingTypes = gameTypes_.listByGame(gameId);
    if (!existingTypes) {
        return Result<GameImportReport>::err(existingTypes.error());
    }
    for (const auto& gt : existingTypes.value()) {
        gameTypeByName_.emplace(toLower(gt.name), gt);
    }

    for (std::size_t i = 0; i < rows.size(); ++i) {
        if (!sink.onProgress(i, rows.size())) {
            report.aborted = true;
            break;
        }
        const auto& row = rows[i];

        auto player = ensurePlayerDeck(formatId, row.deckName, row.variant,
                                       sink, report);
        if (!player) {
            report.skipped.push_back({row.lineNumber, player.error()});
            continue;
        }

        auto opponent = ensureCoreDeck(formatId, row.opponentDeckName,
                                       sink, report);
        if (!opponent) {
            report.skipped.push_back({row.lineNumber, opponent.error()});
            continue;
        }

        auto gameType = ensureGameType(gameId, row.eventName, sink, report);
        if (!gameType) {
            report.skipped.push_back({row.lineNumber, gameType.error()});
            continue;
        }

        auto created = games_.create(formatId, row.playedOn, player.value().id,
                                     opponent.value().id, row.opponent,
                                     row.result, row.score, gameType.value().id,
                                     row.notes);
        if (!created) {
            report.skipped.push_back({row.lineNumber, created.error()});
            continue;
        }
        ++report.gamesImported;
    }

    return Result<GameImportReport>::ok(std::move(report));
}

}  // namespace tracker
