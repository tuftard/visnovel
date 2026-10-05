#include "GradeScene.h"

#include <algorithm>
#include <cstdint>
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

    // align: 0 left, 1 centre, 2 right
    void txt(sf::RenderWindow &w, const sf::Font &f, const string &s,
             float x, float y, unsigned size, sf::Color c, int align = 0)
    {
        sf::Text t(f, s, size);
        t.setFillColor(c);
        sf::FloatRect b = t.getLocalBounds();
        float ox = b.position.x;
        if (align == 1)
            ox += b.size.x / 2.f;
        else if (align == 2)
            ox += b.size.x;
        t.setOrigin({ox, 0.f});
        t.setPosition({x, y});
        w.draw(t);
    }

    // simple chess piece: base, body, head
    void piece(sf::RenderWindow &w, float x, float y, float sq,
               sf::Color c, float h)
    {
        box(w, x + sq * 0.25f, y + sq - 12.f, sq * 0.5f, 6.f, c);
        box(w, x + sq * 0.36f, y + sq - 12.f - h, sq * 0.28f, h, c);
        sf::CircleShape head(sq * 0.12f);
        head.setOrigin({sq * 0.12f, sq * 0.12f});
        head.setPosition({x + sq * 0.5f, y + sq - 14.f - h});
        head.setFillColor(c);
        w.draw(head);
    }
}

void GradeScene::open(const BattleStats &s, int score, char grade)
{
    stats_ = s;
    score_ = score;
    grade_ = grade;
    t_ = 0.f;
    active_ = true;

    vector<AttackParams> uniq;
    for (const auto &b : stats_.builds)
    {
        bool found = false;
        for (const auto &u : uniq)
            if (u == b)
                found = true;
        if (!found)
            uniq.push_back(b);
    }
    unique_ = static_cast<int>(uniq.size());

    if (stats_.fullCombo())
        comment_ = "...Zero hits taken.\nI'd call it luck. Almost.";
    else if (stats_.hitsTaken <= 3)
        comment_ = "Clean enough.\nYou dodge better than you type.";
    else if (stats_.hitsTaken <= 8)
        comment_ = "Passable. Sloppy.\nBut the code actually ran.";
    else
        comment_ = "You won by accident.\nDon't make it a habit.";
}

void GradeScene::update(float dt)
{
    if (active_)
        t_ += dt;
}

size_t GradeScene::shown() const
{
    float n = (t_ - textStart) * cps;
    if (n <= 0.f)
        return 0;
    return min(comment_.size(), static_cast<size_t>(n));
}

bool GradeScene::handleEvent(const sf::Event &e)
{
    if (!active_ || t_ < 0.8f)
        return false;

    const auto *k = e.getIf<sf::Event::KeyPressed>();
    if (!k)
        return false;

    bool confirm = k->code == sf::Keyboard::Key::E ||
                   k->code == sf::Keyboard::Key::Enter ||
                   k->code == sf::Keyboard::Key::Space;
    if (!confirm)
        return false;

    if (shown() < comment_.size())
    {
        t_ = textStart + static_cast<float>(comment_.size()) / cps; // finish typing
        return false;
    }

    active_ = false;
    return true;
}

void GradeScene::draw(sf::RenderWindow &w, const sf::Font &display, const sf::Font &mono) const
{
    if (!active_)
        return;

    w.setView(w.getDefaultView());

    // ---- desktop ----
    for (int y = 0; y < 720; y += 8)
    {
        float k = y / 720.f;
        box(w, 0.f, static_cast<float>(y), 1280.f, 8.f,
            sf::Color(static_cast<uint8_t>(10 + 20 * k),
                      static_cast<uint8_t>(120 - 50 * k),
                      static_cast<uint8_t>(210 - 40 * k)));
    }

    const char *icons[4] = {"CHESS", "MAHJONG", "CALC", "MUSIC"};
    for (int i = 0; i < 4; i++)
    {
        box(w, 40.f, 60.f + i * 100.f, 44.f, 44.f, sf::Color(20, 40, 110),
            sf::Color(200, 225, 255), 2.f);
        txt(w, mono, icons[i], 62.f, 110.f + i * 100.f, 11, sf::Color::White, 1);
    }

    // ---- window ----
    const float wx = 300.f, wy = 40.f;
    box(w, wx, wy, 680.f, 620.f, sf::Color(175, 220, 245), sf::Color(225, 240, 252), 2.f);
    box(w, wx, wy, 680.f, 28.f, sf::Color(110, 175, 230));
    txt(w, mono, "ChessGame", wx + 30.f, wy + 6.f, 14, sf::Color(20, 40, 100));
    box(w, wx + 570.f, wy + 6.f, 28.f, 16.f, sf::Color(130, 190, 240), sf::Color::White, 1.f);
    box(w, wx + 602.f, wy + 6.f, 28.f, 16.f, sf::Color(130, 190, 240), sf::Color::White, 1.f);
    box(w, wx + 634.f, wy + 6.f, 28.f, 16.f, sf::Color(240, 80, 150), sf::Color::White, 1.f);

    txt(w, mono, "File   Mode Action        Step       Options      Help",
        wx + 8.f, wy + 32.f, 11, sf::Color(20, 40, 100));

    // White / Black name bars
    box(w, wx + 8.f, wy + 50.f, 326.f, 38.f, sf::Color(235, 245, 255), sf::Color(20, 40, 100), 1.f);
    txt(w, display, "WHITE", wx + 16.f, wy + 52.f, 26, sf::Color(30, 50, 130));
    box(w, wx + 340.f, wy + 50.f, 332.f, 38.f, sf::Color(18, 48, 66), sf::Color(20, 40, 100), 1.f);
    txt(w, display, "SORA", wx + 348.f, wy + 52.f, 26, sf::Color::White);

    // nav buttons
    const char *nav[5] = {"<<", "<", "P", ">", ">>"};
    for (int i = 0; i < 5; i++)
    {
        box(w, wx + 8.f + i * 90.f, wy + 94.f, 86.f, 16.f, sf::Color(200, 235, 250),
            sf::Color(20, 40, 100), 1.f);
        txt(w, mono, nav[i], wx + 8.f + i * 90.f + 43.f, wy + 94.f, 11,
            sf::Color(20, 40, 100), 1);
    }

    // ---- board ----
    const float bx = wx + 12.f, by = wy + 114.f, sq = 56.f;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
        {
            bool dark = (r + c) % 2 == 1;
            box(w, bx + c * sq, by + r * sq, sq, sq,
                dark ? sf::Color(50, 80, 200) : sf::Color(150, 232, 190));
        }

    for (int c = 0; c < 8; c++)
    {
        piece(w, bx + c * sq, by + 0 * sq, sq, sf::Color(25, 45, 120), 14.f + (c % 3) * 4.f);
        piece(w, bx + c * sq, by + 1 * sq, sq, sf::Color(25, 45, 120), 8.f);
        piece(w, bx + c * sq, by + 6 * sq, sq, sf::Color(250, 250, 255), 8.f);
        piece(w, bx + c * sq, by + 7 * sq, sq, sf::Color(250, 250, 255), 14.f + (c % 3) * 4.f);
    }

    // ---- yellow score sheet ----
    const float px = bx + 8 * sq + 4.f;
    box(w, px, by, 204.f, 8 * sq, sf::Color(250, 232, 130));
    box(w, px, by, 204.f, 4.f, sf::Color(60, 60, 90));
    txt(w, mono, "SCORE SHEET", px + 10.f, by + 12.f, 13, sf::Color(40, 40, 90));

    struct Row
    {
        const char *name;
        int value;
    };
    Row rows[6] = {{"NOTES HIT", stats_.notesHit},
                   {"BEST COMBO", stats_.longestCombo},
                   {"CODES BUILT", static_cast<int>(stats_.builds.size())},
                   {"DIFFERENT", unique_},
                   {"HITS TAKEN", stats_.hitsTaken},
                   {"SCORE", score_}};

    for (int i = 0; i < 6; i++)
    {
        float a = clamp((t_ - (1.0f + i * 0.35f)) / 0.3f, 0.f, 1.f);
        if (a <= 0.f)
            continue;

        auto a8 = static_cast<uint8_t>(255 * a);
        float y = by + 50.f + i * 34.f;
        txt(w, mono, to_string(i + 1) + ". " + rows[i].name, px + 10.f, y, 12,
            sf::Color(40, 40, 90, a8));
        txt(w, mono, to_string(static_cast<int>(rows[i].value * a)), px + 194.f,
            y + 14.f, 14, sf::Color(150, 40, 105, a8), 2);
    }
}