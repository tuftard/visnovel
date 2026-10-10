#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <map>

// =============================================================
// MAP DATA (shared by the editor and by your games)
// =============================================================
struct MapNote
{
    float time = 0.f; // seconds from song start = the moment the note is hit
    int col = 0;      // lanes mode: lane 0-3.  grid mode: column 0-2
    int row = 0;      // grid mode only: 0 = top, 2 = bottom
};

struct RhythmMap
{
    enum class Mode
    {
        Lanes, // 4-lane battle track (A S D F)
        Grid   // 3x3 aim game
    };

    Mode mode = Mode::Lanes;
    float bpm = 120.f;
    float offset = 0.f; // time of beat 0, in seconds
    std::vector<MapNote> notes;

    bool save(const std::string &path) const;
    bool load(const std::string &path);
};

// Loads every maps/*.txt into a registry keyed by file name without
// extension ("maps/shiro_boss.txt" -> "shiro_boss").
std::map<std::string, RhythmMap> loadAllMaps(const std::string &dir = "maps");

// =============================================================
// EDITOR
// =============================================================
// Same shape as your Minigame classes: handleEvent / update / draw / finished.
//   Ctrl+N  new map          Ctrl+O  open the map browser
//   Ctrl+S  save             F2      rename / save-as
class RhythmMapEditor
{
public:
    explicit RhythmMapEditor(std::string filePath = "maps/new_map.txt");

    void handleEvent(const sf::Event &e);
    void update(float dt);
    void draw(sf::RenderWindow &w, const sf::Font &font);
    bool finished() const { return quit; }

private:
    static constexpr float LANE_W = 140.f;
    static constexpr float TOP = 50.f;
    static constexpr float BOTTOM = 700.f;
    static constexpr float PLAY_Y = 560.f;
    static constexpr float PANEL_X = 700.f;
    static constexpr float PAD_X = 830.f;
    static constexpr float PAD_Y = 380.f;
    static constexpr float PAD_CELL = 100.f;
    static constexpr int DIVS[8] = {1, 2, 3, 4, 6, 8, 12, 16};

    // map browser overlay geometry
    static constexpr float BR_X = 340.f;
    static constexpr float BR_Y = 110.f;
    static constexpr float BR_W = 600.f;
    static constexpr float BR_ROW0 = 168.f;
    static constexpr float BR_ROW_H = 28.f;
    static constexpr int BR_VISIBLE = 14;

    struct FileEntry
    {
        std::string path;  // "maps/shiro_boss.txt"
        std::string label; // "shiro_boss"
        std::string info;  // "4-lane, 42 notes"
    };

    RhythmMap maps[2]; // [0] = lanes, [1] = grid
    int cur = 0;

    std::vector<std::vector<MapNote>> undo;

    std::string path;
    bool editingName = false;

    // browser state
    bool browsing = false;
    std::vector<FileEntry> files;
    int browseSel = 0;
    int browseScroll = 0;

    float bpm = 120.f;
    float offset = 0.f;
    int divIdx = 1; // notes per beat = DIVS[divIdx]

    float cursorTime = 0.f;
    float pxPerSec = 220.f;
    bool playing = false;
    int lastBeat = -1;

    int selCol = 1;
    int selRow = 1;

    float laneFlash[4] = {0.f, 0.f, 0.f, 0.f};
    float cellFlash[9] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
    float pulse = 0.f;

    std::string status;
    float statusTimer = 0.f;

    bool quit = false;

    // geometry / time helpers
    int cols() const { return cur == 0 ? 4 : 3; }
    float x0() const { return 90.f + (4 - cols()) * LANE_W / 2.f; }
    float yOf(float t) const { return PLAY_Y - (t - cursorTime) * pxPerSec; }
    float tOf(float y) const { return cursorTime + (PLAY_Y - y) / pxPerSec; }
    float beatLen() const { return 60.f / bpm; }
    float step() const { return beatLen() / static_cast<float>(DIVS[divIdx]); }
    float snap(float t) const;
    bool inTimeline(float mx, float my) const;

    // editing
    void pushUndo();
    void undoStep();
    void toggleNote(float t, int col, int row);
    void removeNearest(float mx, float my);
    void clearAt(float t);
    void flashNote(const MapNote &n);
    void placeKey(sf::Keyboard::Key code);
    void doSave();
    bool loadFile(const std::string &p);
    void setStatus(const std::string &s);

    // many-maps support
    void newMap();
    void finishName();
    void openBrowser();
    void refreshFiles();
    void browserKey(const sf::Event::KeyPressed &k);
    void browserClick(float mx, float my);
    void browserPick();
    void drawBrowser(sf::RenderWindow &w, const sf::Font &font);

    // input
    void onKey(const sf::Event::KeyPressed &k);
    void onMouse(const sf::Event::MouseButtonPressed &m);
    void onWheel(const sf::Event::MouseWheelScrolled &wh);
};