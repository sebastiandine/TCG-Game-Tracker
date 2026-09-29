#pragma once

// DeckStatistics: per-deck-name win/loss/draw records, plus per-deck detail
// matchup tables. Produced from a format's decks, archetypes, and games.

#include "tracker/domain/Deck.hpp"
#include "tracker/domain/DeckArchetype.hpp"
#include "tracker/domain/Game.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace tracker {

struct MatchRecord {
    int wins{0};
    int losses{0};
    int draws{0};

    [[nodiscard]] int total() const noexcept { return wins + losses + draws; }

    // Win percentage including draws as non-wins. Empty when total == 0.
    [[nodiscard]] std::optional<double> winPercentage() const noexcept {
        const int n = total();
        if (n == 0) return std::nullopt;
        return 100.0 * static_cast<double>(wins) / static_cast<double>(n);
    }

    friend bool operator==(const MatchRecord&, const MatchRecord&) = default;
};

struct DeckVariantStats {
    Deck        deck;
    MatchRecord record;

    friend bool operator==(const DeckVariantStats&,
                           const DeckVariantStats&) = default;
};

struct DeckNameStats {
    std::string                    name;
    MatchRecord                    aggregate;
    std::vector<DeckVariantStats>  variants;

    friend bool operator==(const DeckNameStats&, const DeckNameStats&) = default;
};

enum class DeckStatsScope { NameAggregate, Variant };

struct DeckStatsKey {
    DeckStatsScope scope{DeckStatsScope::NameAggregate};
    std::string    deckName;
    std::int64_t   variantDeckId{0};
    std::string    variant;  // display-only; identity is scope + name/id

    friend bool operator==(const DeckStatsKey&, const DeckStatsKey&) = default;
};

struct MatchupStats {
    std::string label;
    MatchRecord record;

    friend bool operator==(const MatchupStats&, const MatchupStats&) = default;
};

// One player-side game with a non-empty note, for the deck overview list.
// `opponentDeck` is the resolved opponent name, or "Unknown deck".
struct OpenNote {
    Game        game;
    std::string opponentDeck;

    friend bool operator==(const OpenNote&, const OpenNote&) = default;
};

struct DeckDetailStatistics {
    std::string                title;
    MatchRecord                overview;
    std::vector<MatchupStats>  vsArchetypes;
    std::vector<MatchupStats>  vsDecks;
    std::vector<MatchupStats>  bestMatchups;
    std::vector<MatchupStats>  worstMatchups;
    std::vector<OpenNote>      openNotes;

    friend bool operator==(const DeckDetailStatistics&,
                           const DeckDetailStatistics&) = default;
};

// Format-wide overview derived from player-side games. `streak` is consecutive
// wins from the most recent game (>0), consecutive losses (<0), or 0 if the
// latest result is a draw or there are no games.
struct FormatStatistics {
    MatchRecord                overview;
    int                        decksPlayed{0};
    int                        uniqueOpponents{0};
    int                        streak{0};
    std::vector<MatchupStats>  bestDecks;
    std::vector<MatchupStats>  worstDecks;
    std::vector<MatchupStats>  mostPlayed;

    friend bool operator==(const FormatStatistics&,
                           const FormatStatistics&) = default;
};

// Group decks by name and tally player-side games (Game::deckId) into
// per-variant records plus a parent aggregate. Names with zero games are
// omitted; unplayed siblings of a played name are kept with a zero record.
std::vector<DeckNameStats> computeDeckStatistics(
    const std::vector<Deck>& decks, const std::vector<Game>& games);

// Player-side W/L/D plus vs-archetype / vs-deck matchups for one family
// (NameAggregate) or one variant row. Unresolved opponents are omitted from
// matchup tables but still counted in overview. Best/worst are the top 5
// vs-deck rows by win % (tie: more games, then name). Open notes are
// player-side games with non-empty notes (newest first); unresolved
// opponents are labeled "Unknown deck".
DeckDetailStatistics computeDeckDetailStatistics(
    const std::vector<Deck>& decks,
    const std::vector<DeckArchetype>& archetypes,
    const std::vector<Game>& games,
    const DeckStatsKey& key);

// Format-wide W/L/D, unique opponents, current streak, and top/bottom/most
// played deck families. Deck rows follow computeDeckStatistics grouping.
FormatStatistics computeFormatStatistics(const std::vector<Deck>& decks,
                                         const std::vector<Game>& games);

}  // namespace tracker
