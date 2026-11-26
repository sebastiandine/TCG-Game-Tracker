#include "tracker/services/FormatService.hpp"

namespace tracker {

FormatService::FormatService(IFormatRepository& formats,
                             IRotationRepository& rotations)
    : formats_(formats), rotations_(rotations) {}

static std::string trim(const std::string& s) {
    const auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

Result<std::string> FormatService::trimmedName(
        const std::string& name, const char* emptyMessage) const {
    const std::string trimmed = trim(name);
    if (trimmed.empty()) return Result<std::string>::err(emptyMessage);
    return Result<std::string>::ok(trimmed);
}

Result<Format> FormatService::create(std::int64_t gameId,
                                     const std::string& name) {
    if (gameId <= 0) {
        return Result<Format>::err("No game selected.");
    }
    auto trimmed = trimmedName(name, "Format name must not be empty.");
    if (!trimmed) return Result<Format>::err(trimmed.error());
    return formats_.create(gameId, trimmed.value(), false);
}

Result<CreatedRotatingFormat> FormatService::createRotating(
        std::int64_t gameId,
        const std::string& formatName,
        const std::string& rotationName,
        int startYear,
        int startMonth,
        int endYear,
        int endMonth) {
    if (gameId <= 0) {
        return Result<CreatedRotatingFormat>::err("No game selected.");
    }
    auto formatTrimmed =
        trimmedName(formatName, "Format name must not be empty.");
    if (!formatTrimmed) {
        return Result<CreatedRotatingFormat>::err(formatTrimmed.error());
    }
    auto rotationTrimmed =
        trimmedName(rotationName, "Rotation name must not be empty.");
    if (!rotationTrimmed) {
        return Result<CreatedRotatingFormat>::err(rotationTrimmed.error());
    }
    if (const char* spanError =
            rotationSpanError(startYear, startMonth, endYear, endMonth)) {
        return Result<CreatedRotatingFormat>::err(spanError);
    }

    auto created = formats_.create(gameId, formatTrimmed.value(), true);
    if (!created) return Result<CreatedRotatingFormat>::err(created.error());

    auto rotation = rotations_.create(
        created.value().id, rotationTrimmed.value(),
        startYear, startMonth, endYear, endMonth);
    if (!rotation) {
        const auto removed = formats_.remove(created.value().id);
        (void)removed;
        return Result<CreatedRotatingFormat>::err(rotation.error());
    }

    CreatedRotatingFormat result;
    result.format = std::move(created.value());
    result.rotation = std::move(rotation.value());
    return Result<CreatedRotatingFormat>::ok(std::move(result));
}

Result<Rotation> FormatService::createRotation(
        std::int64_t formatId,
        const std::string& name,
        int startYear,
        int startMonth,
        int endYear,
        int endMonth) {
    if (formatId <= 0) {
        return Result<Rotation>::err("No format selected.");
    }
    auto format = formats_.findById(formatId);
    if (!format) return Result<Rotation>::err(format.error());
    if (!format.value().rotating) {
        return Result<Rotation>::err(
            "Rotations can only be added to a rotating format.");
    }
    auto trimmed = trimmedName(name, "Rotation name must not be empty.");
    if (!trimmed) return Result<Rotation>::err(trimmed.error());
    if (const char* spanError =
            rotationSpanError(startYear, startMonth, endYear, endMonth)) {
        return Result<Rotation>::err(spanError);
    }
    return rotations_.create(formatId, trimmed.value(), startYear, startMonth,
                             endYear, endMonth);
}

Result<std::vector<Format>> FormatService::listByGame(std::int64_t gameId) {
    return formats_.listByGame(gameId);
}

Result<std::vector<Rotation>> FormatService::listRotations(
        std::int64_t formatId) {
    return rotations_.listByFormat(formatId);
}

Result<void> FormatService::markExpiryPrompted(std::int64_t rotationId) {
    return rotations_.markExpiryPrompted(rotationId);
}

}  // namespace tracker
