#pragma once

#include <string>
#include <vector>
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

    // compiles test.cpp with g++; if launch is true, also starts test.exe
    // in its own console. returns true on success.
    bool build(bool launch = true);
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
    void drawScanlines(sf::RenderWindow &window);

private:
    // ---------------- menu bar ----------------
    enum class Cmd
    {
        None,
        New,
        Save,
        CloseEditor,
        Undo,
        Redo,
        Paste,
        CopyLine,
        CutLine,
        DeleteLine,
        DuplicateLine,
        BuildRun,
        BuildOnly,
        RunOnly,
        ClearOutput,
        ToggleScanlines,
        ToggleIntroBlack,
        GoTop,
        GoBottom
    };

    struct MenuItem
    {
        std::string label;
        std::string hint;
        Cmd cmd;
        bool sep;
    };

    struct Snapshot
    {
        std::string code;
        std::vector<char> black;
        size_t cursor;
    };

    std::vector<std::string> menuNames_;
    std::vector<std::vector<MenuItem>> menus_;
    int menuOpen_ = -1;
    sf::Vector2f mouse_{0.f, 0.f};

    std::vector<Snapshot> undo_;
    std::vector<Snapshot> redo_;

    bool showScanlines_ = true;
    bool showIntroBlack_ = true;

    void buildMenus();
    sf::FloatRect headerRect(int m) const;
    sf::FloatRect dropRect(int m) const;
    sf::FloatRect itemRect(int m, int j) const;
    void drawMenus(sf::RenderWindow &window);
    void execute(Cmd c);

    void undo();
    void redo();
    void runExe();
    size_t lineEndOf(size_t pos) const;

    // ---------------- editor state ----------------
    const sf::Font *font_;

    bool open_ = false;
    std::string code_;
    std::vector<char> black_; // 1 = original intro character (drawn black), 0 = normal syntax colour
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

    sf::VertexArray scanlines_{sf::PrimitiveType::Lines};
    float cachedTop_ = 0.f, cachedBottom_ = 0.f;
    float cachedLeft_ = 0.f, cachedRight_ = 0.f;

    static constexpr int maxColumns = 130;
    static constexpr float codeLeft = 45.f;
    static constexpr float codeRight = 1210.f;
    static constexpr float codeTop = 55.f;
    static constexpr float codeBottom = 480.f;
    static constexpr float lineHeight = 17.f;

    static constexpr float menuX = 300.f; // first header x
    static constexpr float menuY = 24.f;  // header y
    static constexpr float menuHeaderW = 52.f;
    static constexpr float menuHeaderH = 22.f;
    static constexpr float menuGap = 4.f;
    static constexpr float menuW = 240.f; // dropdown width

    void rebuildScanlines();
    void insertText(size_t pos, const std::string &s); // keeps black_ in sync
    void eraseText(size_t pos, size_t n);              // keeps black_ in sync
    size_t lineStart(size_t pos) const;
    void placeCursor(sf::Vector2f mouse);
    void moveUp();
    void moveDown();
    void paste();
    void clampScroll();
    void onText(char32_t unicode);
    bool onKey(const sf::Event::KeyPressed &k);
};
void drawStamps(sf::RenderWindow &window, const sf::Font &font);
void clearStamps();
void applyGradeToStamps(char rank);