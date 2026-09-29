#include "tracker/domain/DeckStatistics.hpp"

#include "tracker/domain/DeckGroup.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace tracker {

namespace {

std::string toLowerAscii(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s)
        out.push_back(
            static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return out;
}

void applyResult(MatchRecord& rec, MatchResult result) {
    switch (result) {
        case MatchResult::Win:
            ++rec.wins;
            break;
        case MatchResult::Loss:
            ++rec.losses;
            break;
        case MatchResult::Draw:
            ++rec.draws;
            break;
    }
}

std::vector<std::int64_t> playerDeckIds(const std::vector<Deck>& decks,
                                        const DeckStatsKey& key,
                                        std::string& title) {
    if (key.scope == DeckStatsScope::Variant) {
        for (const auto& deck : decks) {
            if (deck.id != key.variantDeckId) continue;
            title = deck.variant.empty()
                        ? deck.name
                        : deck.name + " \xE2\x80\x94 " + deck.variant;
            return {deck.id};
        }
        return {};
    }

    const auto nameKey = toLowerAscii(key.deckName);
    for (const auto& group : groupDecksByName(decks)) {
        if (toLowerAscii(group.name) != nameKey) continue;
        title = group.name;
        std::vector<std::int64_t> ids;
        ids.reserve(group.variants.size());
        for (const auto& deck : group.variants) ids.push_back(deck.id);
        return ids;
    }
    return {};
}

bool matchupBetter(const MatchupStats& a, const MatchupStats& b) {
    const double pa = a.record.winPercentage().value_or(-1.0);
    const double pb = b.record.winPercentage().value_or(-1.0);
    if (pa != pb) return pa > pb;
    if (a.record.total() != b.record.total())
        return a.record.total() > b.record.total();
    return a.label < b.label;
}

bool matchupWorse(const MatchupStats& a, const MatchupStats& b) {
    const double pa = a.record.winPercentage().value_or(101.0);
    const double pb = b.record.winPercentage().value_or(101.0);
    if (pa != pb) return pa < pb;
    if (a.record.total() != b.record.total())
        return a.record.total() > b.record.total();
    return a.label < b.label;
}

bool morePlayed(const MatchupStats& a, const MatchupStats& b) {
    if (a.record.total() != b.record.total())
        return a.record.total() > b.record.total();
    return a.label < b.label;
}

int currentStreak(const std::vector<Game>& games,
                  const std::unordered_set<std::int64_t>& playerIds) {
    std::vector<const Game*> ordered;
    ordered.reserve(games.size());
    for (const auto& game : games) {
        if (playerIds.find(game.deckId) == playerIds.end()) continue;
        ordered.push_back(&game);
    }
    std::sort(ordered.begin(), ordered.end(), [](const Game* a, const Game* b) {
        if (a->playedOn != b->playedOn) return a->playedOn > b->playedOn;
        return a->id > b->id;
    });
    if (ordered.empty()) return 0;
    const auto first = ordered.front()->result;
    if (first == MatchResult::Draw) return 0;
    int n = 0;
    for (const Game* game : ordered) {
        if (game->result != first) break;
        ++n;
    }
    return first == MatchResult::Win ? n : -n;
}

std::vector<MatchupStats> takeTop(std::vector<MatchupStats> rows,
                                  bool (*less)(const MatchupStats&,
                                               const MatchupStats&)) {
    std::sort(rows.begin(), rows.end(), less);
    const auto n = std::min<std::size_t>(5, rows.size());
    rows.resize(n);
    return rows;
}

}  // namespace

std::vector<DeckNameStats> computeDeckStatistics(
    const std::vector<Deck>& decks, const std::vector<Game>& games) {
    std::unordered_map<std::int64_t, MatchRecord> byDeckId;
    for (const auto& game : games) {
        applyResult(byDeckId[game.deckId], game.result);
    }

    std::vector<DeckNameStats> out;
    for (const auto& group : groupDecksByName(decks)) {
        DeckNameStats row;
        row.name = group.name;
        row.variants.reserve(group.variants.size());
        for (const auto& deck : group.variants) {
            MatchRecord rec{};
            if (const auto it = byDeckId.find(deck.id); it != byDeckId.end()) {
                rec = it->second;
            }
            row.aggregate.wins += rec.wins;
            row.aggregate.losses += rec.losses;
            row.aggregate.draws += rec.draws;
            row.variants.push_back(DeckVariantStats{deck, rec});
        }
        if (row.aggregate.total() == 0) continue;
        out.push_back(std::move(row));
    }
    return out;
}

DeckDetailStatistics computeDeckDetailStatistics(
    const std::vector<Deck>& decks,
    const std::vector<DeckArchetype>& archetypes,
    const std::vector<Game>& games,
    const DeckStatsKey& key) {
    DeckDetailStatistics out;
    const auto ids = playerDeckIds(decks, key, out.title);
    if (ids.empty()) return out;

    std::unordered_set<std::int64_t> playerIds(ids.begin(), ids.end());

    std::unordered_map<std::int64_t, const Deck*> byId;
    byId.reserve(decks.size());
    for (const auto& deck : decks) byId.emplace(deck.id, &deck);

    std::unordered_map<std::int64_t, std::string> archetypeName;
    for (const auto& arch : archetypes) archetypeName.emplace(arch.id, arch.name);

    struct NamedRecord {
        std::string label;
        MatchRecord record;
    };
    std::unordered_map<std::string, NamedRecord> vsDecksByKey;
    std::unordered_map<std::int64_t, NamedRecord> vsArchetypesById;

    for (const auto& game : games) {
        if (playerIds.find(game.deckId) == playerIds.end()) continue;
        applyResult(out.overview, game.result);

        const auto oppIt = byId.find(game.opponentDeckId);
        if (!game.notes.empty()) {
            OpenNote note;
            note.game = game;
            note.opponentDeck =
                (oppIt != byId.end()) ? oppIt->second->name : "Unknown deck";
            out.openNotes.push_back(std::move(note));
        }
        if (oppIt == byId.end()) continue;
        const Deck& opponent = *oppIt->second;

        const auto deckKey = toLowerAscii(opponent.name);
        auto& vsDeck = vsDecksByKey[deckKey];
        if (vsDeck.label.empty()) vsDeck.label = opponent.name;
        applyResult(vsDeck.record, game.result);

        const auto archIt = archetypeName.find(opponent.archetypeId);
        if (archIt == archetypeName.end()) continue;
        auto& vsArch = vsArchetypesById[opponent.archetypeId];
        if (vsArch.label.empty()) vsArch.label = archIt->second;
        applyResult(vsArch.record, game.result);
    }

    out.vsDecks.reserve(vsDecksByKey.size());
    for (auto& entry : vsDecksByKey) {
        out.vsDecks.push_back(
            MatchupStats{std::move(entry.second.label), entry.second.record});
    }
    std::sort(out.vsDecks.begin(), out.vsDecks.end(),
              [](const MatchupStats& a, const MatchupStats& b) {
                  return toLowerAscii(a.label) < toLowerAscii(b.label);
              });

    out.vsArchetypes.reserve(vsArchetypesById.size());
    for (auto& entry : vsArchetypesById) {
        out.vsArchetypes.push_back(MatchupStats{
            std::move(entry.second.label), entry.second.record});
    }
    std::sort(out.vsArchetypes.begin(), out.vsArchetypes.end(),
              [](const MatchupStats& a, const MatchupStats& b) {
                  return toLowerAscii(a.label) < toLowerAscii(b.label);
              });

    out.bestMatchups = takeTop(out.vsDecks, matchupBetter);
    out.worstMatchups = takeTop(out.vsDecks, matchupWorse);

    std::sort(out.openNotes.begin(), out.openNotes.end(),
              [](const OpenNote& a, const OpenNote& b) {
                  if (a.game.playedOn != b.game.playedOn)
                      return a.game.playedOn > b.game.playedOn;
                  return a.game.id > b.game.id;
              });
    return out;
}

FormatStatistics computeFormatStatistics(const std::vector<Deck>& decks,
                                         const std::vector<Game>& games) {
    FormatStatistics out;
    const auto rows = computeDeckStatistics(decks, games);
    out.decksPlayed = static_cast<int>(rows.size());

    std::vector<MatchupStats> deckRows;
    deckRows.reserve(rows.size());
    for (const auto& row : rows) {
        out.overview.wins += row.aggregate.wins;
        out.overview.losses += row.aggregate.losses;
        out.overview.draws += row.aggregate.draws;
        deckRows.push_back(MatchupStats{row.name, row.aggregate});
    }
    out.bestDecks = takeTop(deckRows, matchupBetter);
    out.worstDecks = takeTop(deckRows, matchupWorse);
    out.mostPlayed = takeTop(std::move(deckRows), morePlayed);

    std::unordered_set<std::int64_t> playerIds;
    playerIds.reserve(decks.size());
    for (const auto& deck : decks) playerIds.insert(deck.id);

    std::unordered_set<std::string> opponents;
    for (const auto& game : games) {
        if (playerIds.find(game.deckId) == playerIds.end()) continue;
        if (game.opponent.empty()) continue;
        opponents.insert(toLowerAscii(game.opponent));
    }
    out.uniqueOpponents = static_cast<int>(opponents.size());
    out.streak = currentStreak(games, playerIds);
    return out;
}

}  // namespace tracker
