#include "ResultsScreen.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <utility>
#include <vector>

using namespace std;

namespace
{
    void box(sf::RenderWindow &w, float x, float y, float wd, float h,
             sf::Color fill, sf::Color outline = sf::Color::Transparent,
             float thick = 0.f)
    {
        sf::RectangleShape r({wd, h});
        r.setPosition({x, y});
        r.setFillColor(fill);
        if (thick > 0.f)
        {
            r.setOutlineColor(outline);
            r.setOutlineThickness(thick);
        }
        w.draw(r);
    }

    void label(sf::RenderWindow &w, const sf::Font &f, const string &s,
               float x, float y, unsigned size, sf::Color c,
               bool centre = false, bool right = false)
    {
        sf::Text t(f, s, size);
        t.setFillColor(c);

        sf::FloatRect b = t.getLocalBounds();
        if (centre)
            t.setOrigin({b.position.x + b.size.x / 2.f, 0.f});
        else if (right)
            t.setOrigin({b.position.x + b.size.x, 0.f});

        t.setPosition({x, y});
        w.draw(t);
    }
}

void ResultsScreen::open(const BattleStats &s, int score, const string &subtitle)
{
    stats_ = s;
    score_ = score;
    sub_ = subtitle;
    t_ = 0.f;
    active_ = true;

    vector<pair<AttackParams, int>> counts;
    for (const auto &b : stats_.builds)
    {
        bool found = false;
        for (auto &c : counts)
            if (c.first == b)
            {
                c.second++;
                found = true;
                break;
            }
        if (!found)
            counts.push_back({b, 1});
    }

    unique_ = static_cast<int>(counts.size());
    favourite_ = "none";

    int best = 0;
    for (const auto &c : counts)
        if (c.second > best)
        {
            best = c.second;
            char buf[64];
            snprintf(buf, sizeof buf, "%d bullets / %d deg / spd %.1f",
                     c.first.bullets, static_cast<int>(c.first.spread),
                     c.first.speed);
            favourite_ = buf;
        }
}

void ResultsScreen::update(float dt)
{
    if (active_)
        t_ += dt;
}

bool ResultsScreen::handleEvent(const sf::Event &e)
{
    if (!active_)
        return false;

    const auto *k = e.getIf<sf::Event::KeyPressed>();
    if (!k)
        return false;

    bool confirm = k->code == sf::Keyboard::Key::E ||
                   k->code == sf::Keyboard::Key::Enter ||
                   k->code == sf::Keyboard::Key::Space;

    if (confirm && t_ > 4.2f)
    {
        active_ = false;
        return true;
    }
    return false;
}

void ResultsScreen::draw(sf::RenderWindow &w, const sf::Font &font) const
{
    if (!active_)
        return;

    w.setView(w.getDefaultView());

    box(w, 0.f, 0.f, 1280.f, 720.f, sf::Color(0, 0, 0, 215));
    box(w, 340.f, 40.f, 600.f, 640.f, sf::Color(10, 10, 25, 245),
        sf::Color(190, 110, 235), 2.f);

    label(w, font, "GRADE", 640.f, 58.f, 18, sf::Color(110, 120, 160), true);

    label(w, font, sub_, 640.f, 230.f, 16, sf::Color(240, 200, 110), true);
    label(w, font, "SCORE  " + to_string(score_), 640.f, 262.f, 20,
          sf::Color(220, 225, 245), true);

    struct Row
    {
        string name;
        int number;
        string text;
        bool useNumber;
    };

    vector<Row> rows = {
        {"NOTES HIT", stats_.notesHit, "", true},
        {"LONGEST COMBO", stats_.longestCombo, "", true},
        {"CODES BUILT", static_cast<int>(stats_.builds.size()), "", true},
        {"DIFFERENT CODES", unique_, "", true},
        {"MOST USED CODE", 0, favourite_, false},
    };

    float y = 320.f;

    for (size_t i = 0; i < rows.size(); i++)
    {
        float start = 0.8f + i * 0.6f;
        float a = clamp((t_ - start) / 0.5f, 0.f, 1.f);

        if (a > 0.f)
        {
            string v = rows[i].useNumber
                           ? to_string(static_cast<int>(rows[i].number * a))
                           : rows[i].text;

            sf::Color c(220, 225, 245, static_cast<uint8_t>(255 * a));

            label(w, font, rows[i].name, 380.f, y, 20,
                  sf::Color(110, 120, 160, static_cast<uint8_t>(255 * a)));
            label(w, font, v, 900.f, y, rows[i].useNumber ? 24 : 15, c, false, true);
        }

        y += 52.f;
    }

    float fcStart = 0.8f + rows.size() * 0.6f;
    float fa = clamp((t_ - fcStart) / 0.4f, 0.f, 1.f);

    if (fa > 0.f)
    {
        if (stats_.fullCombo())
        {
            float glow = 0.6f + 0.4f * sin(t_ * 8.f);
            sf::Color gold(240, 200, 110);

            for (int i = 4; i >= 1; i--)
            {
                sf::Text g(font, "FULL COMBO", 44);
                sf::FloatRect b = g.getLocalBounds();
                g.setOrigin({b.position.x + b.size.x / 2.f, 0.f});
                g.setPosition({640.f, 600.f});
                g.setFillColor(sf::Color::Transparent);
                g.setOutlineColor(sf::Color(gold.r, gold.g, gold.b,
                                            static_cast<uint8_t>(30 * glow)));
                g.setOutlineThickness(i * 3.f);
                w.draw(g);
            }
            label(w, font, "FULL COMBO", 640.f, 600.f, 44, gold, true);
        }
        else
        {
            label(w, font, "NO FULL COMBO  (" + to_string(stats_.hitsTaken) + " hits taken)",
                  640.f, 610.f, 18,
                  sf::Color(110, 120, 160, static_cast<uint8_t>(255 * fa)), true);
        }
    }

    if (t_ > 4.2f && fmod(t_, 1.f) < 0.7f)
        label(w, font, "[E] Continue", 640.f, 650.f, 16, sf::Color(90, 235, 235), true);
}