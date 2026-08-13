#include "systems/GameSystem.hpp"

#include "persistence/Persistence.hpp"
#include "systems/ScreenSystem.hpp"

#include <charconv>
#include <stdexcept>
#include <string>

GameSystem::GameSystem() : System("Game") {}

GameSystem::~GameSystem() {
    if (persistence_ != nullptr) {
        saveState();
    }
}

void GameSystem::init() {
    if (renderTarget_ == nullptr || persistence_ == nullptr) {
        throw std::runtime_error(
            "GameSystem requires registered rendering and persistence"
        );
    }
    if (const auto savedCount = persistence_->get("game.update_count")) {
        const auto [end, error] = std::from_chars(
            savedCount->data(),
            savedCount->data() + savedCount->size(),
            updateCount_
        );
        if (error != std::errc{} || end != savedCount->data() + savedCount->size()) {
            updateCount_ = 0;
        }
    }
    System::init();
}

void GameSystem::registerRenderTarget(RenderTarget& renderTarget) {
    renderTarget_ = &renderTarget;
}

void GameSystem::registerPersistence(Persistence& persistence) {
    persistence_ = &persistence;
}

void GameSystem::update() {
    ++updateCount_;
    constexpr auto checkpointInterval = std::chrono::seconds(5);
    const auto now = std::chrono::steady_clock::now();
    if (now - lastCheckpoint_ >= checkpointInterval) {
        saveState();
        persistence_->checkpoint();
        lastCheckpoint_ = now;
    }
}

void GameSystem::render() {
    renderTarget_->render();
}

void GameSystem::saveState() {
    persistence_->set("game.update_count", std::to_string(updateCount_));
}
