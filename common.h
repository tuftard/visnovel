#pragma once
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cstdint>
#include <string>

struct AttackParams
{
    int bullets = 5;
    float speed = 8.f;
    float spread = 20.f;

    bool operator==(const AttackParams &o) const
    {
        return bullets == o.bullets && speed == o.speed && spread == o.spread;
    }
};

namespace Syntax
{
    inline const sf::Color bg(32, 32, 31);
    inline const sf::Color keyword(190, 116, 227);
    inline const sf::Color orange(230, 164, 92);
    inline const sf::Color green(160, 200, 80);
    inline const sf::Color comment(102, 112, 133);
    inline const sf::Color text(208, 210, 216);
}

// declarations only; bodies are in Common.cpp
bool isKeyword(const std::string &token);
bool isBuiltin(const std::string &token);
std::string getClipboardText();

inline char scoreLetter(int score)
{
    return static_cast<char>('A' + std::clamp(score / 1000, 0, 25));
}

inline sf::Color letterColor(char c)
{
    static const sf::Color stops[6] = {
        {0, 229, 255}, {80, 255, 120}, {255, 220, 60}, {255, 140, 40}, {255, 60, 160}, {170, 90, 255}};

    float f = std::clamp((c - 'A') / 25.f, 0.f, 1.f) * 5.f;
    int a = static_cast<int>(f);
    int b = std::min(a + 1, 5);
    float k = f - a;

    auto m = [&](int x, int y)
    { return static_cast<std::uint8_t>(x + (y - x) * k); };

    return sf::Color(m(stops[a].r, stops[b].r),
                     m(stops[a].g, stops[b].g),
                     m(stops[a].b, stops[b].b));
}