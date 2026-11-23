#pragma once
// Retro (Frutiger / Win98-ish) UI helpers, same palette as the Sora editor.
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <string>

namespace RetroUI
{
    inline const sf::Color Desktop(30, 130, 225);
    inline const sf::Color Frame(190, 225, 250);
    inline const sf::Color TitleTop(240, 250, 255);
    inline const sf::Color TitleBot(150, 200, 240);
    inline const sf::Color Panel(215, 235, 250);
    inline const sf::Color Navy(20, 30, 90);
    inline const sf::Color Royal(45, 65, 185);
    inline const sf::Color Mint(165, 240, 190);
    inline const sf::Color Close(235, 90, 120);
    inline const sf::Color Hi(255, 255, 255);
    inline const sf::Color Lo(90, 120, 170);

    // ---------------------------------------------------------
    // primitives
    // ---------------------------------------------------------
    inline void bevel(sf::RenderTarget &t, sf::FloatRect r, sf::Color fill, bool raised = true)
    {
        sf::RectangleShape box(r.size);
        box.setPosition(r.position);
        box.setFillColor(fill);
        t.draw(box);

        sf::Color tl = raised ? Hi : Lo;
        sf::Color br = raised ? Lo : Hi;

        sf::RectangleShape e;
        e.setFillColor(tl);
        e.setSize({r.size.x, 2.f});
        e.setPosition(r.position);
        t.draw(e);
        e.setSize({2.f, r.size.y});
        e.setPosition(r.position);
        t.draw(e);
        e.setFillColor(br);
        e.setSize({r.size.x, 2.f});
        e.setPosition({r.position.x, r.position.y + r.size.y - 2.f});
        t.draw(e);
        e.setSize({2.f, r.size.y});
        e.setPosition({r.position.x + r.size.x - 2.f, r.position.y});
        t.draw(e);
    }

    inline void gradient(sf::RenderTarget &t, sf::FloatRect r, sf::Color top, sf::Color bot)
    {
        sf::VertexArray v(sf::PrimitiveType::TriangleStrip, 4);
        v[0] = sf::Vertex(r.position, top);
        v[1] = sf::Vertex({r.position.x + r.size.x, r.position.y}, top);
        v[2] = sf::Vertex({r.position.x, r.position.y + r.size.y}, bot);
        v[3] = sf::Vertex({r.position.x + r.size.x, r.position.y + r.size.y}, bot);
        t.draw(v);
    }

    inline void text(sf::RenderTarget &t, const sf::Font &f, const std::string &s,
                     sf::Vector2f p, unsigned size, sf::Color c)
    {
        sf::Text x(f, s, size);
        x.setPosition(p);
        x.setFillColor(c);
        t.draw(x);
    }

    // scrolling two-colour strip
    inline void checkerStrip(sf::RenderTarget &t, float x, float y, float width, float h,
                             sf::Color a, sf::Color b, float time)
    {
        const float cell = h;
        float off = std::fmod(time * 20.f, cell * 2.f);
        sf::RectangleShape r({cell, h});

        for (float cx = -off; cx < width; cx += cell)
        {
            int idx = static_cast<int>(std::floor((cx + off) / cell));
            r.setPosition({x + cx, y});
            r.setFillColor(idx % 2 == 0 ? a : b);
            t.draw(r);
        }
    }

    // retro button (set `hover` from the mapped mouse position)
    inline void button(sf::RenderTarget &t, sf::FloatRect r, const std::string &label,
                       const sf::Font &f, bool hover, unsigned size = 22)
    {
        bool down = hover && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
        bevel(t, r, hover ? sf::Color(200, 250, 215) : Mint, !down);

        sf::Text x(f, label, size);
        sf::FloatRect b = x.getLocalBounds();
        x.setOrigin({b.position.x + b.size.x / 2.f, b.position.y + b.size.y / 2.f});
        x.setPosition({r.position.x + r.size.x / 2.f + (down ? 1.f : 0.f),
                       r.position.y + r.size.y / 2.f + (down ? 1.f : 0.f)});
        x.setFillColor(Navy);
        t.draw(x);
    }

    // ---------------------------------------------------------
    // backgrounds
    // ---------------------------------------------------------

    // checker wallpaper for the inside of the window (1280x720 scene space)
    inline void sceneBackdrop(sf::RenderTarget &t, float time)
    {
        const float r = 40.f;                          // half the width of one diamond
        const float shift = std::fmod(time * 14.f, r * 2.f); // pattern repeats every 2r, so it loops seamlessly

        // base colour = the "gaps"; the lighter diamonds are drawn on top
        sf::RectangleShape base({1280.f, 720.f});
        base.setFillColor(sf::Color(30, 130, 225));
        t.draw(base);

        // a circle with 4 points is a diamond
        sf::CircleShape d(r, 4);
        d.setOrigin({r, r});
        d.setFillColor(sf::Color(55, 155, 240));

        for (int k = -2; k < 720.f / r + 3; k += 2)        // every other row
            for (int m = -2; m < 1280.f / r + 3; m++)
            {
                if ((k + m) & 1)                           // diamonds sit on a staggered grid
                    continue;
                d.setPosition({m * r + shift, k * r + shift});
                t.draw(d);
            }
    }

    // full-screen desktop + taskbar (default view, 1280x720)
    inline void desktop(sf::RenderWindow &w, const sf::Font &f, const std::string &taskTitle, float time)
    {
        w.setView(w.getDefaultView());
        gradient(w, {{0.f, 0.f}, {1280.f, 688.f}}, sf::Color(90, 185, 250), Desktop);

        const char *names[] = {"My Code", "Trash"};
        for (int i = 0; i < 2; i++)
        {
            sf::FloatRect ic({24.f, 24.f + i * 92.f}, {48.f, 48.f});
            bevel(w, ic, i == 0 ? Mint : sf::Color(245, 232, 140), true);
            text(w, f, names[i], {ic.position.x - 2.f, ic.position.y + 54.f}, 12, sf::Color::White);
        }

        checkerStrip(w, 0.f, 672.f, 1280.f, 16.f, Royal, Mint, time);

        bevel(w, {{0.f, 688.f}, {1280.f, 32.f}}, Frame, true);
        bevel(w, {{4.f, 692.f}, {84.f, 24.f}}, Mint, true);
        text(w, f, "Start", {22.f, 696.f}, 14, Navy);
        bevel(w, {{96.f, 692.f}, {240.f, 24.f}}, Panel, false);
        text(w, f, taskTitle, {106.f, 696.f}, 13, Navy);
    }

    // ---------------------------------------------------------
    // the window itself
    // ---------------------------------------------------------
    struct Layout
    {
        sf::FloatRect content; // where the 1280x720 scene is shown
        sf::FloatRect closeBtn;
    };

    // frame 976x584 -> content 960x540 (exactly 16:9)
    inline Layout layoutFor(sf::FloatRect frame)
    {
        Layout l;
        l.content = {{frame.position.x + 8.f, frame.position.y + 36.f},
                     {frame.size.x - 16.f, frame.size.y - 44.f}};
        l.closeBtn = {{frame.position.x + frame.size.x - 28.f, frame.position.y + 7.f}, {18.f, 18.f}};
        return l;
    }

    class AppWindow
    {
    public:
        void open() { state_ = State::Opening; }

        void close()
        {
            if (state_ == State::Open || state_ == State::Opening)
                state_ = State::Closing;
        }

        bool interactive() const { return state_ == State::Open; }

        // true once, the frame a close animation finishes
        bool takeClosed()
        {
            bool c = closed_;
            closed_ = false;
            return c;
        }

        void update(float dt)
        {
            const float speed = 7.f; // ~0.14 s pop
            if (state_ == State::Opening)
            {
                t_ += dt * speed;
                if (t_ >= 1.f)
                {
                    t_ = 1.f;
                    state_ = State::Open;
                }
            }
            else if (state_ == State::Closing)
            {
                t_ -= dt * speed;
                if (t_ <= 0.f)
                {
                    t_ = 0.f;
                    state_ = State::Closed;
                    closed_ = true;
                }
            }
        }

        // view that maps the 1280x720 scene into the (scaled) content area; viewport clips it
        sf::View sceneView(sf::FloatRect content, sf::FloatRect frame) const
        {
            float s = scale();
            sf::Vector2f c = frame.position + frame.size / 2.f;
            sf::Vector2f pos = c + (content.position - c) * s;
            sf::Vector2f size = content.size * s;

            sf::View v(sf::FloatRect({0.f, 0.f}, {1280.f, 720.f}));
            v.setViewport({{pos.x / 1280.f, pos.y / 720.f}, {size.x / 1280.f, size.y / 720.f}});
            return v;
        }

        void drawChrome(sf::RenderWindow &w, const sf::Font &f, const std::string &title,
                        sf::FloatRect frame, const Layout &l, float /*time*/) const
        {
            if (state_ == State::Closed)
                return;

            float s = scale();
            const sf::Vector2f S(1280.f, 720.f);
            sf::Vector2f c = frame.position + frame.size / 2.f;

            // pop animation: scale everything about the window centre
            sf::View v(sf::FloatRect({0.f, 0.f}, S));
            v.setSize(S / s);
            v.setCenter((S / 2.f - c * (1.f - s)) / s);
            w.setView(v);

            sf::RectangleShape shadow(frame.size);
            shadow.setPosition(frame.position + sf::Vector2f(7.f, 7.f));
            shadow.setFillColor(sf::Color(0, 20, 60, 90));
            w.draw(shadow);

            bevel(w, frame, Frame, true);
            gradient(w, {{frame.position.x + 2.f, frame.position.y + 2.f}, {frame.size.x - 4.f, 28.f}},
                     TitleTop, TitleBot);
            text(w, f, title, {frame.position.x + 12.f, frame.position.y + 8.f}, 13, Navy);

            bevel(w, l.closeBtn, Close, true);
            text(w, f, "x", {l.closeBtn.position.x + 5.f, l.closeBtn.position.y + 1.f}, 13, sf::Color::White);

            bevel(w, l.content, Panel, false);

            w.setView(w.getDefaultView());
        }

    private:
        enum class State
        {
            Closed,
            Opening,
            Open,
            Closing
        } state_ = State::Closed;

        float t_ = 0.f;
        bool closed_ = false;

        float scale() const
        {
            float e = t_ * t_ * (3.f - 2.f * t_); // smoothstep
            return std::max(e, 0.02f);
        }
    };
}
