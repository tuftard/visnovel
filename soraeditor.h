#pragma once

#include <string>
#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

#include "Common.h" // Syntax colours, isKeyword, isBuiltin, getClipboardText

// =============================================================
// SORA EDITOR
// =============================================================

class SoraEditor
{
public:
    explicit SoraEditor(const sf::Font &monoFont);

    // call once: fills the editor with the // comment lines of a source file
    void loadCommentsFrom(const std::string &fileName);

    void open();
    void close();
    bool isOpen() const { return open_; }

    const std::string &code() const { return code_; }

    // writes the editor text to test.cpp
    bool save() const;

    // compiles test.cpp with g++ and launches test.exe in its own console.
    // returns true on success; compiler output is stored in lastOutput()
    bool build();
    const std::string &lastOutput() const { return buildOutput_; }
    bool lastBuildOk() const { return buildOk_; }

    // true exactly once after each build (use it to react to a finished build)
    bool takeBuildEvent()
    {
        bool b = builtFlag_;
        builtFlag_ = false;
        return b;
    }

    // Returns true if the event was consumed (editor is modal while open).
    bool handleEvent(const sf::Event &e);

    void draw(sf::RenderWindow &window);

private:
    const sf::Font *font_;

    bool open_ = false;
    std::string code_;
    size_t cursorPos_ = 0;
    float scrollOffset_ = 0.f;
    size_t lastCursor_ = static_cast<size_t>(-1);
    float cw_ = 8.4f;

    std::string buildOutput_ = "Press BUILD (or F5) to compile.";
    bool buildOk_ = true;
    bool builtFlag_ = false;

    sf::RectangleShape background_;
    sf::RectangleShape titleBar_;
    sf::RectangleShape closeBtn_;
    sf::RectangleShape buildBtn_;
    sf::RectangleShape cursor_;
    sf::Clock cursorClock_;

    static constexpr int maxColumns = 130;
    static constexpr float codeLeft = 45.f;
    static constexpr float codeRight = 1210.f;
    static constexpr float codeTop = 55.f;
    static constexpr float codeBottom = 480.f;
    static constexpr float lineHeight = 17.f;

    size_t lineStart(size_t pos) const;
    void placeCursor(sf::Vector2f mouse);
    void moveUp();
    void moveDown();
    void paste();
    void clampScroll();
    void onText(char32_t unicode);
    bool onKey(const sf::Event::KeyPressed &k);
};