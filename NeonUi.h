#pragma once
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Neon
{
    inline const sf::Color bgDeep(5, 11, 20), bgPanel(9, 19, 34),
        cyan(0, 240, 255), pink(255, 42, 117), purple(157, 0, 255),
        orange(255, 119, 0), white(255, 255, 255), dim(97, 123, 148);

    constexpr float W = 1280.f, H = 720.f;
    constexpr float PANEL_X = 980.f;

    inline sf::Color alpha(sf::Color c, float a)
    {
        c.a = static_cast<std::uint8_t>(255.f * std::clamp(a, 0.f, 1.f));
        return c;
    }

    inline char rank(float acc)
    {
        return acc >= .95f ? 'S' : acc >= .85f ? 'A'
                               : acc >= .70f   ? 'B'
                               : acc >= .50f   ? 'C'
                                               : 'D';
    }

    using CodeLines = std::vector<std::pair<std::string, sf::Color>>;
    using Rows = std::vector<std::pair<std::string, std::string>>;

    void rect(sf::RenderWindow &w, float x, float y, float wd, float h,
              sf::Color fill, sf::Color outline = sf::Color::Transparent, float thick = 0.f);
    void quad(sf::RenderWindow &w, sf::Vector2f a, sf::Vector2f b, sf::Vector2f c,
              sf::Vector2f d, sf::Color ca, sf::Color cb, sf::Color cc, sf::Color cd);
    void circle(sf::RenderWindow &w, sf::Vector2f c, float r, sf::Color fill,
                sf::Color outline = sf::Color::Transparent, float thick = 0.f,
                size_t pts = 24, float rotDeg = 0.f);
    // align: 0 left, 1 centre
    void text(sf::RenderWindow &w, const sf::Font &f, const std::string &s,
              float x, float y, unsigned size, sf::Color c, bool centered = false);

    void glowBox(sf::RenderWindow &w, float x, float y, float wd, float h, sf::Color c);
    void backdrop(sf::RenderWindow &w, float time, sf::Vector2f portal);
    void header(sf::RenderWindow &w, const sf::Font &f, const std::string &left,
                const std::string &right);
    void hudRow(sf::RenderWindow &w, const sf::Font &f, int score, int combo, char rank);
    void terminalPanel(sf::RenderWindow &w, const sf::Font &f, const std::string &title,
                       const std::string &file, const CodeLines &lines);
    void telemetry(sf::RenderWindow &w, const sf::Font &f, const Rows &rows);
    void crosshair(sf::RenderWindow &w, sf::Vector2f p);
}