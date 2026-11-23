#include "NeonUI.h"
#include <cmath>

namespace Neon
{
    void rect(sf::RenderWindow &w, float x, float y, float wd, float h,
              sf::Color fill, sf::Color outline, float thick)
    {
        sf::RectangleShape r({wd, h});
        r.setPosition({std::floor(x), std::floor(y)});
        r.setFillColor(fill);
        if (thick > 0.f)
        {
            r.setOutlineColor(outline);
            r.setOutlineThickness(thick);
        }
        w.draw(r);
    }

    void quad(sf::RenderWindow &w, sf::Vector2f a, sf::Vector2f b, sf::Vector2f c,
              sf::Vector2f d, sf::Color ca, sf::Color cb, sf::Color cc, sf::Color cd)
    {
        sf::Vertex v[6] = {
            sf::Vertex{a, ca}, sf::Vertex{b, cb}, sf::Vertex{c, cc},
            sf::Vertex{a, ca}, sf::Vertex{c, cc}, sf::Vertex{d, cd}};
        w.draw(v, 6, sf::PrimitiveType::Triangles);
    }

    void circle(sf::RenderWindow &w, sf::Vector2f c, float r, sf::Color fill,
                sf::Color outline, float thick, size_t pts, float rotDeg)
    {
        sf::CircleShape s(r, pts);
        s.setOrigin({r, r});
        s.setPosition(c);
        s.setRotation(sf::degrees(rotDeg));
        s.setFillColor(fill);
        if (thick > 0.f)
        {
            s.setOutlineColor(outline);
            s.setOutlineThickness(thick);
        }
        w.draw(s);
    }

    void text(sf::RenderWindow &w, const sf::Font &f, const std::string &s,
              float x, float y, unsigned size, sf::Color c, bool centered)
    {
        sf::Text t(f, s, size);
        t.setFillColor(c);
        if (centered)
        {
            sf::FloatRect b = t.getLocalBounds();
            t.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
        }
        t.setPosition({x, y});
        w.draw(t);
    }

    static void spiral(sf::RenderWindow &w, sf::Vector2f c, float rMin, float rMax,
                       int arms, float twist, float spin, float squash, sf::Color color)
    {
        sf::VertexArray pts(sf::PrimitiveType::Points);
        for (int a = 0; a < arms; a++)
            for (float r = rMin; r < rMax; r += 0.6f)
            {
                float t = (r - rMin) / (rMax - rMin);
                float ang = a * 6.2831853f / static_cast<float>(arms) + r * twist + spin;
                float x = std::floor(c.x + std::cos(ang) * r);
                float y = std::floor(c.y + std::sin(ang) * r * squash);
                sf::Color col = color;
                col.a = static_cast<std::uint8_t>(std::clamp(30.f + 200.f * (1.f - t), 0.f, 255.f));
                pts.append(sf::Vertex{{x + 0.5f, y + 0.5f}, col});
            }
        w.draw(pts);
    }

    void glowBox(sf::RenderWindow &w, float x, float y, float wd, float h, sf::Color c)
    {
        rect(w, x - 3, y - 3, wd + 6, h + 6, sf::Color::Transparent, alpha(c, 0.04f), 3.f);
        rect(w, x - 2, y - 2, wd + 4, h + 4, sf::Color::Transparent, alpha(c, 0.08f), 2.f);
        rect(w, x - 1, y - 1, wd + 2, h + 2, sf::Color::Transparent, alpha(c, 0.15f), 1.f);
        rect(w, x, y, wd, h, sf::Color::Transparent, c, 1.f);
    }

    void backdrop(sf::RenderWindow &w, float time, sf::Vector2f portal)
    {
        w.clear(bgDeep);
        quad(w, {0, 0}, {W, 0}, {W, H}, {0, H},
             sf::Color(10, 25, 48), sf::Color(10, 25, 48), bgDeep, bgDeep);

        spiral(w, portal, 10.f, 115.f, 5, 0.060f, -time * 0.45f, 0.8f, sf::Color(130, 70, 190));
        spiral(w, portal, 10.f, 80.f, 3, -0.090f, time * 0.30f, 0.8f, sf::Color(70, 50, 140));

        float pulse = 0.5f + 0.5f * std::sin(time * 3.f);
        circle(w, portal, 17.f + pulse * 2.f, alpha(purple, 0.1f));
        circle(w, portal, 10.5f, sf::Color(6, 4, 16), cyan, 2.f, 32);
    }

    void header(sf::RenderWindow &w, const sf::Font &f, const std::string &left,
                const std::string &right)
    {
        rect(w, 0, 0, W, 35.f, sf::Color(3, 7, 13));
        rect(w, 0, 35.f, W, 1.f, alpha(cyan, 0.23f));
        text(w, f, left, 15.f, 8.f, 13, cyan);
        text(w, f, right, 1190.f, 8.f, 13, cyan);
    }

    void hudRow(sf::RenderWindow &w, const sf::Font &f, int score, int combo, char rk)
    {
        auto panel = [&](float x, const std::string &label, const std::string &val, sf::Color vc)
        {
            rect(w, x, 50.f, 170.f, 36.f, sf::Color(5, 11, 20, 205), cyan, 1.f);
            text(w, f, label, x + 10.f, 59.f, 14, white);
            text(w, f, val, x + 92.f, 59.f, 14, vc);
        };
        panel(20.f, "SCORE:", std::to_string(score), cyan);
        panel(205.f, "COMBO:", std::to_string(combo) + "x", pink);
        panel(390.f, "RANK:", std::string(1, rk), purple);
    }

    void terminalPanel(sf::RenderWindow &w, const sf::Font &f, const std::string &title,
                       const std::string &file, const CodeLines &lines)
    {
        rect(w, PANEL_X, 36.f, W - PANEL_X, 684.f, bgPanel);
        rect(w, PANEL_X, 36.f, 2.f, 684.f, pink);
        rect(w, PANEL_X, 36.f, W - PANEL_X, 34.f, sf::Color(5, 11, 20));
        text(w, f, title, 1000.f, 45.f, 12, pink);
        text(w, f, file, 1205.f, 45.f, 12, purple);

        float y = 90.f;
        for (const auto &l : lines)
        {
            if (l.first.empty())
            {
                y += 10.f;
                continue;
            }
            text(w, f, l.first, 1000.f, y, 12, l.second);
            y += 20.f;
        }
    }

    void telemetry(sf::RenderWindow &w, const sf::Font &f, const Rows &rows)
    {
        rect(w, PANEL_X, 590.f, W - PANEL_X, 130.f, sf::Color(4, 8, 16));
        glowBox(w, PANEL_X, 590.f, W - PANEL_X, 130.f, cyan);
        text(w, f, "TELEMETRY", 1000.f, 602.f, 11, dim);

        float y = 630.f;
        for (const auto &r : rows)
        {
            text(w, f, r.first, 1000.f, y, 11, dim);
            text(w, f, r.second, 1200.f, y, 11, cyan);
            y += 24.f;
        }
    }

    void crosshair(sf::RenderWindow &w, sf::Vector2f p)
    {
        rect(w, p.x - 12.f, p.y - 1.f, 24.f, 2.f, white);
        rect(w, p.x - 1.f, p.y - 12.f, 2.f, 24.f, white);
        circle(w, p, 9.f, sf::Color::Transparent, cyan, 2.f);
    }
}
