#include "tracker/domain/Enums.hpp"

#include <stdexcept>
#include <string>

namespace tracker {

// ── Theme ───────────────────────────────────────────────────────────────

std::string_view to_string(Theme t) noexcept {
    switch (t) {
        case Theme::Light: return "Light";
        case Theme::Dark:  return "Dark";
    }
    return "Light";
}

std::optional<Theme> themeFromString(std::string_view s) noexcept {
    if (s == "Light") return Theme::Light;
    if (s == "Dark")  return Theme::Dark;
    return std::nullopt;
}

const std::array<Theme, 2>& allThemes() noexcept {
    static constexpr std::array<Theme, 2> v{Theme::Light, Theme::Dark};
    return v;
}

void to_json(nlohmann::json& j, Theme v) {
    j = std::string(to_string(v));
}

void from_json(const nlohmann::json& j, Theme& v) {
    auto parsed = themeFromString(j.get<std::string>());
    if (!parsed) throw std::invalid_argument("Unknown Theme value: " + j.get<std::string>());
    v = *parsed;
}

// ── Competitiveness ─────────────────────────────────────────────────────

std::string_view to_string(Competitiveness c) noexcept {
    switch (c) {
        case Competitiveness::Competitive:    return "Competitive";
        case Competitiveness::NonCompetitive: return "Non-Competitive";
    }
    return "Non-Competitive";
}

std::optional<Competitiveness> competitivenessFromString(std::string_view s) noexcept {
    if (s == "Competitive")     return Competitiveness::Competitive;
    if (s == "Non-Competitive") return Competitiveness::NonCompetitive;
    return std::nullopt;
}

const std::array<Competitiveness, 2>& allCompetitiveness() noexcept {
    static constexpr std::array<Competitiveness, 2> v{
        Competitiveness::Competitive, Competitiveness::NonCompetitive};
    return v;
}

void to_json(nlohmann::json& j, Competitiveness v) {
    j = std::string(to_string(v));
}

void from_json(const nlohmann::json& j, Competitiveness& v) {
    auto parsed = competitivenessFromString(j.get<std::string>());
    if (!parsed) throw std::invalid_argument("Unknown Competitiveness value: " + j.get<std::string>());
    v = *parsed;
}

// ── PlayMedium ──────────────────────────────────────────────────────────

std::string_view to_string(PlayMedium m) noexcept {
    switch (m) {
        case PlayMedium::Paper:  return "Paper";
        case PlayMedium::Online: return "Online";
    }
    return "Paper";
}

std::optional<PlayMedium> playMediumFromString(std::string_view s) noexcept {
    if (s == "Paper")  return PlayMedium::Paper;
    if (s == "Online") return PlayMedium::Online;
    return std::nullopt;
}

const std::array<PlayMedium, 2>& allPlayMediums() noexcept {
    static constexpr std::array<PlayMedium, 2> v{PlayMedium::Paper, PlayMedium::Online};
    return v;
}

void to_json(nlohmann::json& j, PlayMedium v) {
    j = std::string(to_string(v));
}

void from_json(const nlohmann::json& j, PlayMedium& v) {
    auto parsed = playMediumFromString(j.get<std::string>());
    if (!parsed) throw std::invalid_argument("Unknown PlayMedium value: " + j.get<std::string>());
    v = *parsed;
}

// ── MatchResult ─────────────────────────────────────────────────────────

std::string_view to_string(MatchResult r) noexcept {
    switch (r) {
        case MatchResult::Win:  return "Win";
        case MatchResult::Loss: return "Loss";
        case MatchResult::Draw: return "Draw";
    }
    return "Win";
}

std::optional<MatchResult> matchResultFromString(std::string_view s) noexcept {
    if (s == "Win")  return MatchResult::Win;
    if (s == "Loss") return MatchResult::Loss;
    if (s == "Draw") return MatchResult::Draw;
    return std::nullopt;
}

const std::array<MatchResult, 3>& allMatchResults() noexcept {
    static constexpr std::array<MatchResult, 3> v{
        MatchResult::Win, MatchResult::Loss, MatchResult::Draw};
    return v;
}

void to_json(nlohmann::json& j, MatchResult v) {
    j = std::string(to_string(v));
}

void from_json(const nlohmann::json& j, MatchResult& v) {
    auto parsed = matchResultFromString(j.get<std::string>());
    if (!parsed) throw std::invalid_argument("Unknown MatchResult value: " + j.get<std::string>());
    v = *parsed;
}

// ── MatchScore ──────────────────────────────────────────────────────────

std::string_view to_string(MatchScore s) noexcept {
    switch (s) {
        case MatchScore::TwoOne:  return "2-1";
        case MatchScore::TwoZero: return "2-0";
        case MatchScore::OneTwo:  return "1-2";
        case MatchScore::ZeroTwo: return "0-2";
        case MatchScore::OneOne:  return "1-1";
    }
    return "2-1";
}

std::optional<MatchScore> matchScoreFromString(std::string_view s) noexcept {
    if (s == "2-1") return MatchScore::TwoOne;
    if (s == "2-0") return MatchScore::TwoZero;
    if (s == "1-2") return MatchScore::OneTwo;
    if (s == "0-2") return MatchScore::ZeroTwo;
    if (s == "1-1") return MatchScore::OneOne;
    return std::nullopt;
}

const std::array<MatchScore, 5>& allMatchScores() noexcept {
    static constexpr std::array<MatchScore, 5> v{
        MatchScore::TwoOne, MatchScore::TwoZero,
        MatchScore::OneTwo, MatchScore::ZeroTwo,
        MatchScore::OneOne};
    return v;
}

MatchResult resultForScore(MatchScore s) noexcept {
    switch (s) {
        case MatchScore::TwoOne:
        case MatchScore::TwoZero:
            return MatchResult::Win;
        case MatchScore::OneTwo:
        case MatchScore::ZeroTwo:
            return MatchResult::Loss;
        case MatchScore::OneOne:
            return MatchResult::Draw;
    }
    return MatchResult::Win;
}

void to_json(nlohmann::json& j, MatchScore v) {
    j = std::string(to_string(v));
}

void from_json(const nlohmann::json& j, MatchScore& v) {
    auto parsed = matchScoreFromString(j.get<std::string>());
    if (!parsed) throw std::invalid_argument("Unknown MatchScore value: " + j.get<std::string>());
    v = *parsed;
}

}  // namespace tracker
