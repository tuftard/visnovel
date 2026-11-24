#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

// Both classes draw in the 233x207 "pixel canvas" space and take any sf::RenderTarget.
// No images and no font files: digits are drawn as 3x5 pixel glyphs.

// ---- falling 0/1 code rain: light purple + pink, see-through ----
class CodeRain
{
public:
    float intensity = 1.f; // 0..1, lower = more transparent

    explicit CodeRain(float width = 233.f, float height = 207.f);
    void update(float dt);
    void draw(sf::RenderTarget &target);

private:
    struct Column { float x, head, speed; int len; int tint; };
    static constexpr float ROW = 7.f; // vertical spacing between digits

    float W, H, time = 0.f;
    std::vector<Column> cols;
    void respawn(Column &c, bool first);
};

// ---- player + boss health bars (like the battle screen) ----
class HealthBars
{
public:
    void set(int playerHP, int bossHP); // 0..100 each
    void update(float dt);
    void draw(sf::RenderTarget &target);

private:
    float time = 0.f;
    int playerHP = 100, bossHP = 100;
    float ghostP = 100.f, ghostB = 100.f; // pale trail that drains after a hit
};
