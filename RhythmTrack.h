#pragma once
#include "Common.h"
#include <string>
#include <vector>

namespace Theme
{
    inline const sf::Color bgDeep(10, 10, 26);
    inline const sf::Color bgPanel(16, 16, 40);
    inline const sf::Color steel(74, 90, 130);
    inline const sf::Color steelLight(130, 150, 200);
    inline const sf::Color gold(240, 200, 110);
    inline const sf::Color amber(200, 140, 80);
    inline const sf::Color violet(190, 110, 235);
    inline const sf::Color teal(90, 235, 235);
    inline const sf::Color rust(160, 90, 80);
    inline const sf::Color text(220, 225, 245);
    inline const sf::Color textDim(110, 120, 160);
}

struct RhythmNote
{
    int lane;
    float z;
    bool active = true;
};

struct BattleHud
{
    int playerHP = 100;
    int bossHP = 100;
    int combo = 0;
    int score = 0;
    int playerLane = 1;
    int bullets = 5;
    float spread = 20.f;
};

class RhythmTrack
{
public:
    static constexpr unsigned CW = 233;
    static constexpr unsigned CH = 207;
    static constexpr float PX = 3.f;

    sf::Vector2f origin{18.f, 60.f};

    RhythmTrack();

    void update(float dt, int playerLane);
    void flash(int lane);
    void miss() { hurtFlash = 1.f; }
    sf::Vector2f playerScreenPos() const;

    void draw(sf::RenderWindow &window,
              const std::vector<RhythmNote> &notes,
              const BattleHud &hud,
              const sf::Font &font);

private:
    struct Spark
    {
        sf::Vector2f pos;
        sf::Vector2f vel;
        float life = 0.f;
        float maxLife = 1.f;
        sf::Color color;
    };

    sf::RenderTexture canvas;
    float time = 0.f;
    float laneGlow[4] = {0.f, 0.f, 0.f, 0.f};
    float playerLaneF = 1.f;
    float hurtFlash = 0.f;
    std::vector<Spark> sparks;

    static constexpr float fW = 233.f;
    static constexpr float fH = 207.f;
    static constexpr float zHit = 150.f;
    static constexpr float zFar = 850.f;
    static constexpr float vanishX = 118.f;
    static constexpr float vanishY = 36.f;
    static constexpr float hitY = 159.f;
    static constexpr float halfHit = 92.f;

    static inline const sf::Color laneBright[4] = {
        sf::Color(100, 240, 240), sf::Color(195, 115, 240),
        sf::Color(240, 125, 95), sf::Color(250, 210, 115)};
    static inline const sf::Color laneMid[4] = {
        sf::Color(30, 140, 150), sf::Color(110, 60, 170),
        sf::Color(160, 70, 60), sf::Color(200, 150, 70)};
    static inline const sf::Color laneDark[4] = {
        sf::Color(8, 46, 56), sf::Color(40, 24, 78),
        sf::Color(70, 28, 40), sf::Color(96, 64, 36)};

    // helpers
    static sf::Color col(sf::Color c, float a);
    static sf::Color mix(sf::Color a, sf::Color b, float t);
    static float laneCenter(float lane) { return -0.75f + 0.5f * lane; }
    static sf::Vector2f pS(float lanePos, float s);
    static sf::Vector2f pZ(float lanePos, float z);

    void rect(float x, float y, float w, float h, sf::Color fill,
              sf::Color outline = sf::Color::Transparent, float thick = 0.f);
    void quad(sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Vector2f d,
              sf::Color ca, sf::Color cb, sf::Color cc, sf::Color cd);
    void quad(sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Vector2f d,
              sf::Color all);
    void tri(sf::Vector2f a, sf::Vector2f b, sf::Vector2f c, sf::Color color);
    void line(sf::Vector2f a, sf::Vector2f b, sf::Color c);
    void circle(sf::Vector2f c, float r, sf::Color fill,
                sf::Color outline = sf::Color::Transparent, float thick = 0.f,
                size_t points = 24, float rotDeg = 0.f);
    void spiral(sf::Vector2f c, float rMin, float rMax, int arms, float twist,
                float spin, float squash, sf::Color color);
    void frame(float x, float y, float w, float h, sf::Color fill, sf::Color border);
    void bar(float x, float y, float w, float h, float frac, sf::Color fill,
             sf::Color hi, bool rightAnchored, sf::Color border);
    void magicCircle(sf::Vector2f c, float r, sf::Color a, sf::Color b, float dir);

    // scene
    void drawBackdrop();
    void drawPortal();
    void drawTrack();
    void drawNotes(const std::vector<RhythmNote> &notes);
    void drawHitZone();
    void drawCrackle();
    void drawSparks();
    void drawPlayer();
    void drawHud(const BattleHud &h);

    void text(sf::RenderWindow &w, const sf::Font &f, const std::string &s,
              sf::Vector2f cpos, unsigned size, sf::Color c, int align = 0);
    void drawOverlayText(sf::RenderWindow &w, const BattleHud &h, const sf::Font &f);
};