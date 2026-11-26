#pragma once

// InMemoryFormatRepository: test fake backed by a std::vector.

#include "tracker/ports/IFormatRepository.hpp"

#include <algorithm>
#include <cctype>
#include <vector>

namespace tracker::testing {

class InMemoryFormatRepository final : public IFormatRepository {
public:
    Result<Format> create(std::int64_t gameId, const std::string& name,
                          bool rotating = false) override {
        auto it = std::find_if(formats_.begin(), formats_.end(),
            [&](const Format& f) {
                if (f.gameId != gameId) return false;
                if (f.name.size() != name.size()) return false;
                for (std::size_t i = 0; i < name.size(); ++i) {
                    if (std::tolower(static_cast<unsigned char>(f.name[i])) !=
                        std::tolower(static_cast<unsigned char>(name[i])))
                        return false;
                }
                return true;
            });
        if (it != formats_.end()) {
            return Result<Format>::err(
                "A format named \"" + name + "\" already exists.");
        }
        Format f;
        f.id = nextId_++;
        f.gameId = gameId;
        f.name = name;
        f.rotating = rotating;
        formats_.push_back(f);
        return Result<Format>::ok(f);
    }

    Result<Format> findById(std::int64_t formatId) override {
        for (const auto& format : formats_) {
            if (format.id == formatId) return Result<Format>::ok(format);
        }
        return Result<Format>::err("Format not found.");
    }

    Result<void> remove(std::int64_t formatId) override {
        const auto it = std::find_if(
            formats_.begin(), formats_.end(),
            [formatId](const Format& format) { return format.id == formatId; });
        if (it == formats_.end()) return Result<void>::err("Format not found.");
        formats_.erase(it);
        return Result<void>::ok();
    }

    Result<std::vector<Format>> listByGame(std::int64_t gameId) override {
        std::vector<Format> out;
        for (const auto& f : formats_) {
            if (f.gameId == gameId) out.push_back(f);
        }
        return Result<std::vector<Format>>::ok(std::move(out));
    }

private:
    std::int64_t nextId_{1};
    std::vector<Format> formats_;
};

}  // namespace tracker::testing
