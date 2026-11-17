#include "GradeScene.h"
#include "MiniChess.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <vector>

using namespace std;

namespace
{
    // board placement (matches wx + 12, wy + 114 in draw())
    constexpr float kBoardX = 312.f;
    constexpr float kBoardY = 154.f;
    constexpr float kSq = 56.f;

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
}

void GradeScene::open(const BattleStats &s, int score, char grade)
{
    stats_ = s;
    score_ = score;
    grade_ = grade;
    t_ = 0.f;
    active_ = true;
    chess_.reset();

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
    if (!active_)
        return;

    t_ += dt;
    chess_.update(dt);
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

    // mouse: play chess
    if (const auto *m = e.getIf<sf::Event::MouseButtonPressed>())
    {
        if (m->button == sf::Mouse::Button::Left)
        {
            int c = static_cast<int>(std::floor((m->position.x - kBoardX) / kSq));
            int r = static_cast<int>(std::floor((m->position.y - kBoardY) / kSq));
            if (r >= 0 && r < 8 && c >= 0 && c < 8)
                chess_.click(r, c);
        }
        return false;
    }

    const auto *k = e.getIf<sf::Event::KeyPressed>();
    if (!k)
        return false;

    if (k->code == sf::Keyboard::Key::R)
    {
        chess_.reset();
        return false;
    }

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
    // File menu dropdown
    box(w, wx + 8.f, wy + 48.f, 80.f, 16.f, sf::Color(240, 240, 255), sf::Color(20, 40, 100), 1.f);
    txt(w, mono, "New Game", wx + 12.f, wy + 50.f, 11, sf::Color(20, 40, 100));
    box(w, wx + 8.f, wy + 64.f, 80.f, 16.f, sf::Color(240, 240, 255), sf::Color(20, 40, 100), 1.f);
    txt(w, mono, "Load Game", wx + 12.f, wy + 66.f, 11, sf::Color(20, 40, 100));
    // when typing is done, show the grade and comment
    if (t_ > textStart)
    {
        size_t n = shown();
        string comment = comment_.substr(0, n);

        txt(w, display, "GRADE  " + string(1, grade_), wx + 30.f, wy + 60.f, 32,
            sf::Color(240, 200, 110));
        txt(w, display, comment, wx + 30.f, wy + 120.f, 20,
            sf::Color(20, 40, 100));
    }
    // scanlines
    for (int y = 0; y < 720; y += 2)
        box(w, 0.f, static_cast<float>(y), 1280.f, 1.f,
            sf::Color(255, 255, 255, 10));
    // interactive exit with red button already seen in the window title bar, but also allow space/enter to exit
    txt(w, mono, "Press SPACE to continue", wx + 30.f, wy + 580.f, 14, sf::Color(20, 40, 100));
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
    const float bx = kBoardX, by = kBoardY, sq = kSq;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
        {
            bool dark = (r + c) % 2 == 1;
            box(w, bx + c * sq, by + r * sq, sq, sq,
                dark ? sf::Color(50, 80, 200) : sf::Color(150, 232, 190));
        }

    // selection + legal-move hints
    if (chess_.hasSelection())
    {
        box(w, bx + chess_.selCol() * sq, by + chess_.selRow() * sq, sq, sq,
            sf::Color(240, 200, 110, 140));

        for (const auto &m : chess_.movesFrom(chess_.selRow(), chess_.selCol()))
        {
            sf::CircleShape dot(8.f);
            dot.setOrigin({8.f, 8.f});
            dot.setPosition({bx + m.tc * sq + sq / 2.f, by + m.tr * sq + sq / 2.f});
            dot.setFillColor(sf::Color(240, 200, 110, 210));
            w.draw(dot);
        }
    }

    // pieces (letters with an outline so they read on both square colours)
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
        {
            char p = chess_.at(r, c);
            if (p == '.')
                continue;

            bool white = MiniChess::isWhite(p);
            string ch(1, static_cast<char>(std::toupper(static_cast<unsigned char>(p))));

            sf::Text t(mono, ch, 34);
            t.setFillColor(white ? sf::Color(250, 250, 255) : sf::Color(15, 25, 70));
            t.setOutlineColor(white ? sf::Color(20, 40, 120) : sf::Color(230, 240, 255));
            t.setOutlineThickness(2.f);

            sf::FloatRect tb = t.getLocalBounds();
            t.setOrigin({tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f});
            t.setPosition({bx + c * sq + sq / 2.f, by + r * sq + sq / 2.f});
            w.draw(t);
        }

    // status line under the board
    string status;
    switch (chess_.winner())
    {
    case 1:
        status = "CHECKMATE - YOU WIN  (R = new game)";
        break;
    case 2:
        status = "CHECKMATE - SHIRO WINS  (R = new game)";
        break;
    case 3:
        status = "STALEMATE - DRAW  (R = new game)";
        break;
    default:
        status = chess_.playerTurn() ? (chess_.inCheck() ? "CHECK! YOUR MOVE" : "YOUR MOVE (WHITE)")
                                     : "SHIRO IS THINKING...";
        break;
    }
    txt(w, mono, status, bx, by + 8 * sq + 4.f, 12, sf::Color(20, 40, 100));

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
