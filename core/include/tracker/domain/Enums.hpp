#pragma once

// Enums shared across the domain layer.

#include <nlohmann/json.hpp>

#include <array>
#include <optional>
#include <string_view>

namespace tracker {

// ── Theme (light / dark UI mode) ────────────────────────────────────────

enum class Theme {
    Light,
    Dark,
};

std::string_view to_string(Theme t) noexcept;
std::optional<Theme> themeFromString(std::string_view s) noexcept;
const std::array<Theme, 2>& allThemes() noexcept;

void to_json(nlohmann::json& j, Theme v);
void from_json(const nlohmann::json& j, Theme& v);

// ── Competitiveness ─────────────────────────────────────────────────────

enum class Competitiveness {
    Competitive,
    NonCompetitive,
};

std::string_view to_string(Competitiveness c) noexcept;
std::optional<Competitiveness> competitivenessFromString(std::string_view s) noexcept;
const std::array<Competitiveness, 2>& allCompetitiveness() noexcept;

void to_json(nlohmann::json& j, Competitiveness v);
void from_json(const nlohmann::json& j, Competitiveness& v);

// ── PlayMedium ──────────────────────────────────────────────────────────

enum class PlayMedium {
    Paper,
    Online,
};

std::string_view to_string(PlayMedium m) noexcept;
std::optional<PlayMedium> playMediumFromString(std::string_view s) noexcept;
const std::array<PlayMedium, 2>& allPlayMediums() noexcept;

void to_json(nlohmann::json& j, PlayMedium v);
void from_json(const nlohmann::json& j, PlayMedium& v);

// ── MatchResult ─────────────────────────────────────────────────────────

enum class MatchResult {
    Win,
    Loss,
    Draw,
};

std::string_view to_string(MatchResult r) noexcept;
std::optional<MatchResult> matchResultFromString(std::string_view s) noexcept;
const std::array<MatchResult, 3>& allMatchResults() noexcept;

void to_json(nlohmann::json& j, MatchResult v);
void from_json(const nlohmann::json& j, MatchResult& v);

// ── MatchScore ──────────────────────────────────────────────────────────

enum class MatchScore {
    TwoOne,
    TwoZero,
    OneTwo,
    ZeroTwo,
    OneOne,
};

std::string_view to_string(MatchScore s) noexcept;
std::optional<MatchScore> matchScoreFromString(std::string_view s) noexcept;
const std::array<MatchScore, 5>& allMatchScores() noexcept;

// Derive the most natural MatchResult from a score.
MatchResult resultForScore(MatchScore s) noexcept;

void to_json(nlohmann::json& j, MatchScore v);
void from_json(const nlohmann::json& j, MatchScore& v);

}  // namespace tracker
