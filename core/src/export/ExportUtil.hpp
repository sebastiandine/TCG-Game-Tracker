#pragma once

#include "tracker/domain/Enums.hpp"
#include "tracker/export/GameExportWrite.hpp"

#include <array>
#include <string>
#include <string_view>

namespace tracker::export_detail {

inline constexpr std::array<std::string_view, 8> kGameExportHeaders{
    "Date", "Deck", "Variante", "Opponent", "Result",
    "Event", "Opponent Player", "Notes"};

inline std::array<std::string, 8> cellsForRow(const GameExportRow& row) {
    return {
        row.playedOn,
        row.deckName,
        row.variant,
        row.opponentDeckName,
        std::string(to_string(row.score)),
        row.eventName,
        row.opponent,
        row.notes,
    };
}

}  // namespace tracker::export_detail
