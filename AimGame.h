#pragma once
#include "Minigame.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

struct RhythmMap; // from RhythmMapEditor.h

struct AimNote
{
    int col;
    int row;
    float time; // when it should be hit, in song seconds
    bool done = false;
};

// Pixel-art aim game: notes fly out of the portal onto a 3x3 grid.
// Aim with the mouse, click (or Z / X / Space) on time.
class AimGame : public Minigame
{
    // low-res canvas, upscaled 4x -> exactly 1280x720
    static constexpr unsigned CW = 320, CH = 180;
    static constexpr float FW = 320.f, FH = 180.f;
    static constexpr float PX = 4.f;

    static constexpr float Z_HIT = 400.f;
    static constexpr float Z_FAR = 1400.f;
    static constexpr float APPROACH = 1.2f;
    static constexpr float CELL = 30.f; // canvas units
    static constexpr float HIT_WINDOW = 0.14f;

    static constexpr float VX = 160.f, VY = 34.f;  // portal (vanishing point)
    static constexpr float GX = 160.f, GY = 96.f;  // grid centre

    struct Spark
    {
        sf::Vector2f pos, vel;
        float life = 0.f, maxLife = 1.f;
        sf::Color color;
    };

    sf::RenderTexture canvas;

    std::vector<AimNote> notes;
    std::vector<Spark> sparks;

    float songTime = -1.f; // lead-in
    float timeTracker = 0.f;
    float endTime = 0.f;
    float cellGlow[9] = {};
    float hurtFlash = 0.f;

    sf::Vector2f cursor{GX, GY}; // canvas coordinates

    int score = 0, combo = 0, hits = 0, misses = 0;
    bool quit = false;

    std::string fbText;
    sf::Color fbColor{255, 255, 255};
    float fbTime = 0.f;
    sf::Vector2f fbPos{GX, GY};

    sf::Vector2f cellPos(int col, int row) const;
    sf::Vector2f project(sf::Vector2f target, float s) const;
    float accuracy() const;
    void tryHit();
    void burst(sf::Vector2f p, sf::Color c, int n);

    void drawBackdrop();
    void drawTunnel();
    void drawGrid();
    void drawNotes();
    void drawSparks();
    void drawCrosshair();
    void drawHud();
    void drawOverlayText(sf::RenderWindow &w, const sf::Font &f);

public:
    explicit AimGame(const RhythmMap *map = nullptr);

    void handleEvent(const sf::Event &e) override;
    void update(float dt) override;
    void draw(sf::RenderWindow &w, const sf::Font &font) override;

    bool finished() const override { return quit || songTime > endTime; }
    int result() const override { return score; }
};