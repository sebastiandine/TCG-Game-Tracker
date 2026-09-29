#pragma once

// InMemoryGameTitleRepository: test fake backed by a std::vector.

#include "tracker/ports/IGameTitleRepository.hpp"

#include <algorithm>
#include <cctype>
#include <vector>

namespace tracker::testing {

class InMemoryGameTitleRepository final : public IGameTitleRepository {
public:
    Result<GameTitle> create(const std::string& name) override {
        auto it = std::find_if(titles_.begin(), titles_.end(),
            [&](const GameTitle& g) {
                if (g.name.size() != name.size()) return false;
                for (std::size_t i = 0; i < name.size(); ++i) {
                    if (std::tolower(static_cast<unsigned char>(g.name[i])) !=
                        std::tolower(static_cast<unsigned char>(name[i])))
                        return false;
                }
                return true;
            });
        if (it != titles_.end()) {
            return Result<GameTitle>::err(
                "A game named \"" + name + "\" already exists.");
        }
        GameTitle g;
        g.id = nextId_++;
        g.name = name;
        titles_.push_back(g);
        return Result<GameTitle>::ok(g);
    }

    Result<std::vector<GameTitle>> listAll() override {
        return Result<std::vector<GameTitle>>::ok(titles_);
    }

private:
    std::int64_t nextId_{1};
    std::vector<GameTitle> titles_;
};

}  // namespace tracker::testing
