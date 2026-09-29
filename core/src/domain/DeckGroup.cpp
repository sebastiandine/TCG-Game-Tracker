#include "tracker/domain/DeckGroup.hpp"

#include <algorithm>
#include <cctype>

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

}  // namespace

std::vector<DeckGroup> groupDecksByName(const std::vector<Deck>& decks) {
    // Sort a working copy by lowercase name then lowercase variant.
    auto sorted = decks;
    std::sort(sorted.begin(), sorted.end(),
              [](const Deck& a, const Deck& b) {
                  const auto aName = toLowerAscii(a.name);
                  const auto bName = toLowerAscii(b.name);
                  if (aName != bName) return aName < bName;
                  return toLowerAscii(a.variant) < toLowerAscii(b.variant);
              });

    std::vector<DeckGroup> groups;
    for (const auto& deck : sorted) {
        const auto key = toLowerAscii(deck.name);
        if (groups.empty() || toLowerAscii(groups.back().name) != key) {
            groups.push_back(DeckGroup{deck.name, {}});
        }
        groups.back().variants.push_back(deck);
    }
    return groups;
}

}  // namespace tracker
