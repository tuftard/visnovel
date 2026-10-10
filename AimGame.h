#pragma once
#include "Minigame.h"
#include <vector>
#include <string>

struct RhythmMap; // from RhythmMapEditor.h

struct AimNote
{
    int col;
    int row;
    float time; // when it should be hit, in song seconds
    bool done = false;
};

// Rhythia-style: notes fly out of the distance onto a 3x3 grid,
// aim the mouse at the cell and click (or press Z / X / Space) on time.
class AimGame : public Minigame
{
    static constexpr float PERSPECTIVE = 400.f;
    static constexpr float Z_HIT = 400.f; // scale 1.0 at the grid
    static constexpr float Z_FAR = 1400.f;
    static constexpr float APPROACH = 1.2f; // seconds a note is in flight
    static constexpr float CELL = 110.f;
    static constexpr float HIT_WINDOW = 0.14f;

    const sf::Vector2f center{640.f, 360.f}; // 1280x720 window

    std::vector<AimNote> notes;
    float songTime = -1.f; // lead-in
    float timeTracker = 0.f;
    float endTime = 0.f;
    sf::Vector2f cursor{}; // relative to center
    int score = 0;
    int combo = 0;
    int hits = 0;
    int misses = 0;
    bool quit = false;
    struct Spark
    {
        float x, y, vx, vy, born;
        sf::Color color;
    };
    std::vector<Spark> sparks;
    int prevHits = 0, prevMisses = 0;
    std::string fbText;
    sf::Color fbColor{255, 255, 255};
    float fbUntil = -1.f;

    sf::Vector2f cellPos(int col, int row) const;
    sf::Vector2f toScreen(sf::Vector2f p, float z) const;
    void tryHit();

public:
    // map == nullptr (or a lane-mode map) -> built-in pattern
    explicit AimGame(const RhythmMap *map = nullptr);

    void handleEvent(const sf::Event &e) override;
    void update(float dt) override;
    void draw(sf::RenderWindow &w, const sf::Font &font) override;

    bool finished() const override { return quit || songTime > endTime; }
    int result() const override { return score; }
};