#include "PledgeScreen.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

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
               float x, float y, unsigned size, sf::Color c, bool centre = false)
    {
        sf::Text t(f, s, size);
        t.setFillColor(c);
        if (centre)
        {
            sf::FloatRect b = t.getLocalBounds();
            t.setOrigin({b.position.x + b.size.x / 2.f, 0.f});
        }
        t.setPosition({x, y});
        w.draw(t);
    }
}

void PledgeScreen::open(const string &opponent, const string &wager,
                        const vector<string> &rules)
{
    opponent_ = opponent;
    wager_ = wager;
    rules_ = rules;
    t_ = 0.f;
    active_ = true;
}

void PledgeScreen::update(float dt)
{
    if (active_)
        t_ += dt;
}

PledgeScreen::Result PledgeScreen::handleEvent(const sf::Event &e)
{
    if (!active_)
        return Result::None;

    const auto *k = e.getIf<sf::Event::KeyPressed>();
    if (!k)
        return Result::None;

    if (k->code == sf::Keyboard::Key::Escape)
    {
        active_ = false;
        return Result::Declined;
    }

    bool confirm = k->code == sf::Keyboard::Key::E ||
                   k->code == sf::Keyboard::Key::Enter ||
                   k->code == sf::Keyboard::Key::Space;

    // only accept once every rule has appeared
    if (confirm && t_ > revealEnd())
    {
        active_ = false;
        return Result::Accepted;
    }

    return Result::None;
}

void PledgeScreen::draw(sf::RenderWindow &w, const sf::Font &font) const
{
    if (!active_)
        return;

    w.setView(w.getDefaultView());

    box(w, 0.f, 0.f, 1280.f, 720.f, sf::Color(0, 0, 0, 200));
    box(w, 240.f, 100.f, 800.f, 520.f, sf::Color(10, 10, 25, 245),
        sf::Color(190, 110, 235), 2.f);

    label(w, font, "TERMS OF THE MATCH", 640.f, 130.f, 30,
          sf::Color(240, 200, 110), true);
    label(w, font, "Opponent: " + opponent_, 640.f, 180.f, 20,
          sf::Color(220, 225, 245), true);

    for (int i = 0; i < static_cast<int>(rules_.size()); i++)
    {
        float a = clamp((t_ - (0.5f + i * 0.6f)) / 0.4f, 0.f, 1.f);
        if (a <= 0.f)
            continue;

        label(w, font, to_string(i + 1) + ".  " + rules_[i], 290.f,
              240.f + i * 50.f, 20,
              sf::Color(220, 225, 245, static_cast<uint8_t>(255 * a)));
    }

    float wa = clamp((t_ - (0.5f + rules_.size() * 0.6f)) / 0.4f, 0.f, 1.f);
    if (wa > 0.f)
        label(w, font, "STAKES:  " + wager_, 290.f, 250.f + rules_.size() * 50.f,
              20, sf::Color(240, 125, 95, static_cast<uint8_t>(255 * wa)));

    if (t_ > revealEnd() && fmod(t_, 1.f) < 0.7f)
        label(w, font, "[E] Accept        [Esc] Walk away", 640.f, 570.f, 18,
              sf::Color(90, 235, 235), true);
}