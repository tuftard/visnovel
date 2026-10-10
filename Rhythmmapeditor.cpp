#include "RhythmMapEditor.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

using std::string;
using std::to_string;
namespace fs = std::filesystem;

// =============================================================
// MAP FILE FORMAT
// =============================================================
//   # rhythm map v1
//   mode lanes            (or: mode grid)
//   bpm 120.000
//   offset 0.000
//   note 1.250 2          lanes: time lane
//   note 1.500 1 0        grid:  time col row
bool RhythmMap::save(const string &path) const
{
    std::ofstream f(path);

    if (!f.is_open())
        return false;

    std::vector<MapNote> sorted = notes;

    std::sort(sorted.begin(), sorted.end(),
              [](const MapNote &a, const MapNote &b)
              {
                  if (a.time != b.time)
                      return a.time < b.time;
                  if (a.col != b.col)
                      return a.col < b.col;
                  return a.row < b.row;
              });

    f.setf(std::ios::fixed);
    f.precision(3);

    f << "# rhythm map v1\n";
    f << "mode " << (mode == Mode::Lanes ? "lanes" : "grid") << "\n";
    f << "bpm " << bpm << "\n";
    f << "offset " << offset << "\n";

    for (const MapNote &n : sorted)
    {
        f << "note " << n.time << " " << n.col;

        if (mode == Mode::Grid)
            f << " " << n.row;

        f << "\n";
    }

    return f.good();
}

bool RhythmMap::load(const string &path)
{
    std::ifstream f(path);

    if (!f.is_open())
        return false;

    RhythmMap m;
    string line;

    while (std::getline(f, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::istringstream ss(line);
        string key;
        ss >> key;

        if (key == "mode")
        {
            string v;
            ss >> v;
            m.mode = (v == "grid") ? Mode::Grid : Mode::Lanes;
        }
        else if (key == "bpm")
        {
            ss >> m.bpm;
        }
        else if (key == "offset")
        {
            ss >> m.offset;
        }
        else if (key == "note")
        {
            MapNote n;
            ss >> n.time >> n.col;

            if (ss.fail())
                continue; // malformed note line

            if (!(ss >> n.row))
                n.row = 0;

            m.notes.push_back(n);
        }
    }

    if (m.bpm < 30.f)
        m.bpm = 30.f;

    const int maxCol = (m.mode == Mode::Lanes) ? 3 : 2;

    for (MapNote &n : m.notes)
    {
        n.col = std::clamp(n.col, 0, maxCol);
        n.row = (m.mode == Mode::Lanes) ? 0 : std::clamp(n.row, 0, 2);
        n.time = std::max(0.f, n.time);
    }

    *this = std::move(m);
    return true;
}

// =============================================================
// MAP REGISTRY
// =============================================================
std::map<string, RhythmMap> loadAllMaps(const string &dir)
{
    std::map<string, RhythmMap> out;
    std::error_code ec;

    if (!fs::exists(dir, ec))
        return out;

    for (const auto &e : fs::directory_iterator(dir, ec))
    {
        if (e.path().extension() != ".txt")
            continue;

        RhythmMap m;

        if (m.load(e.path().string()))
            out[e.path().stem().string()] = std::move(m); // key = "shiro_boss"
    }

    return out;
}

// =============================================================
// DRAWING HELPERS
// =============================================================
namespace
{
    const sf::Color BG(5, 11, 20);
    const sf::Color PANEL(9, 19, 34);
    const sf::Color BAR(3, 7, 13);
    const sf::Color CYAN(0, 240, 255);
    const sf::Color PINK(255, 42, 117);
    const sf::Color PURPLE(157, 0, 255);
    const sf::Color ORANGE(255, 119, 0);
    const sf::Color GOLD(250, 210, 115);
    const sf::Color WHITE(255, 255, 255);
    const sf::Color DIM(97, 123, 148);

    const sf::Color LANE_COL[4] = {CYAN, PURPLE, ORANGE, GOLD};
    const char *ROW_NAME[3] = {"TOP", "MID", "BOT"};

    sf::Color withA(sf::Color c, float a)
    {
        c.a = static_cast<std::uint8_t>(255.f * std::clamp(a, 0.f, 1.f));
        return c;
    }

    void rect(sf::RenderWindow &w, float x, float y, float wd, float h,
              sf::Color fill, sf::Color outline = sf::Color::Transparent,
              float thick = 0.f)
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

    string timeString(float t)
    {
        int mins = static_cast<int>(t) / 60;
        float secs = t - mins * 60.f;
        char buf[32];
        std::snprintf(buf, sizeof buf, "%d:%06.3f", mins, secs);
        return buf;
    }
}

// =============================================================
// EDITOR
// =============================================================
RhythmMapEditor::RhythmMapEditor(string filePath) : path(std::move(filePath))
{
    maps[0].mode = RhythmMap::Mode::Lanes;
    maps[1].mode = RhythmMap::Mode::Grid;

    std::error_code ec;
    fs::create_directories("maps", ec); // make sure the folder exists

    // if the starting file already exists, open it
    if (fs::exists(path, ec))
        loadFile(path);
}

float RhythmMapEditor::snap(float t) const
{
    float s = step();
    return std::max(0.f, std::round((t - offset) / s) * s + offset);
}

bool RhythmMapEditor::inTimeline(float mx, float my) const
{
    return mx >= x0() && mx < x0() + cols() * LANE_W && my >= TOP && my < BOTTOM;
}

void RhythmMapEditor::setStatus(const string &s)
{
    status = s;
    statusTimer = 3.f;
}

// ---------------- editing ----------------
void RhythmMapEditor::pushUndo()
{
    undo.push_back(maps[cur].notes);

    if (undo.size() > 100)
        undo.erase(undo.begin());
}

void RhythmMapEditor::undoStep()
{
    if (undo.empty())
    {
        setStatus("Nothing to undo");
        return;
    }

    maps[cur].notes = undo.back();
    undo.pop_back();
    setStatus("Undo");
}

void RhythmMapEditor::flashNote(const MapNote &n)
{
    if (cur == 0)
    {
        if (n.col >= 0 && n.col < 4)
            laneFlash[n.col] = 1.f;
    }
    else if (n.col >= 0 && n.col < 3 && n.row >= 0 && n.row < 3)
    {
        cellFlash[n.row * 3 + n.col] = 1.f;
    }
}

void RhythmMapEditor::toggleNote(float t, int col, int row)
{
    if (col < 0 || col >= cols())
        return;

    row = (cur == 0) ? 0 : std::clamp(row, 0, 2);

    pushUndo();

    auto &v = maps[cur].notes;

    auto it = std::find_if(v.begin(), v.end(),
                           [&](const MapNote &n)
                           {
                               return std::fabs(n.time - t) < 0.0005f &&
                                      n.col == col && n.row == row;
                           });

    MapNote probe{t, col, row};

    if (it != v.end())
    {
        v.erase(it);
    }
    else
    {
        v.push_back(probe);
        flashNote(probe);
    }
}

void RhythmMapEditor::removeNearest(float mx, float my)
{
    auto &v = maps[cur].notes;
    int best = -1;
    float bestD = 28.f * 28.f;

    for (int i = 0; i < static_cast<int>(v.size()); i++)
    {
        float cx = x0() + v[i].col * LANE_W + LANE_W / 2.f;
        float cy = yOf(v[i].time);
        float dx = mx - cx;
        float dy = my - cy;
        float d = dx * dx + dy * dy;

        if (d < bestD)
        {
            bestD = d;
            best = i;
        }
    }

    if (best >= 0)
    {
        pushUndo();
        v.erase(v.begin() + best);
    }
}

void RhythmMapEditor::clearAt(float t)
{
    auto &v = maps[cur].notes;

    auto it = std::remove_if(v.begin(), v.end(),
                             [&](const MapNote &n)
                             { return std::fabs(n.time - t) < 0.0005f; });

    if (it != v.end())
    {
        pushUndo();
        v.erase(it, v.end());
    }
}

void RhythmMapEditor::placeKey(sf::Keyboard::Key code)
{
    using Key = sf::Keyboard::Key;
    float t = snap(cursorTime);

    if (cur == 0)
    {
        int lane = -1;

        if (code == Key::A)
            lane = 0;
        else if (code == Key::S)
            lane = 1;
        else if (code == Key::D)
            lane = 2;
        else if (code == Key::F)
            lane = 3;

        if (lane >= 0)
            toggleNote(t, lane, 0);

        return;
    }

    int n = -1;

    if (code >= Key::Num1 && code <= Key::Num9)
        n = static_cast<int>(code) - static_cast<int>(Key::Num1);
    else if (code >= Key::Numpad1 && code <= Key::Numpad9)
        n = static_cast<int>(code) - static_cast<int>(Key::Numpad1);

    if (n >= 0)
    {
        // numpad layout: 7 8 9 on the top row, 1 2 3 on the bottom
        selCol = n % 3;
        selRow = 2 - n / 3;
        toggleNote(t, selCol, selRow);
    }
}

void RhythmMapEditor::doSave()
{
    RhythmMap &m = maps[cur];
    m.bpm = bpm;
    m.offset = offset;

    std::error_code ec;
    fs::path parent = fs::path(path).parent_path();

    if (!parent.empty())
        fs::create_directories(parent, ec);

    if (m.save(path))
        setStatus("Saved " + to_string(m.notes.size()) + " notes to " + path);
    else
        setStatus("SAVE FAILED: " + path);
}

bool RhythmMapEditor::loadFile(const string &p)
{
    RhythmMap tmp;

    if (!tmp.load(p))
    {
        setStatus("Could not open " + p);
        return false;
    }

    cur = (tmp.mode == RhythmMap::Mode::Grid) ? 1 : 0;
    bpm = tmp.bpm;
    offset = tmp.offset;
    maps[cur] = tmp;
    path = p;

    undo.clear();
    cursorTime = 0.f;
    playing = false;

    setStatus("Loaded " + to_string(tmp.notes.size()) + " notes from " + p);
    return true;
}

// ---------------- many maps: new / rename / browse ----------------
void RhythmMapEditor::newMap()
{
    maps[cur].notes.clear();
    undo.clear();

    bpm = 120.f;
    offset = 0.f;
    cursorTime = 0.f;
    playing = false;

    path = "maps/"; // type the name, then Enter
    editingName = true;
    browsing = false;

    setStatus("New map: type a name and press Enter");
}

// turns what was typed into a real path: maps/<name>.txt
void RhythmMapEditor::finishName()
{
    editingName = false;

    if (path.find('/') == string::npos && path.find('\\') == string::npos)
        path = "maps/" + path;

    string name = fs::path(path).filename().string();

    if (name.empty() || name == ".txt") // nothing typed after "maps/"
    {
        int i = 1;

        while (fs::exists("maps/new_map_" + to_string(i) + ".txt"))
            i++;

        path = "maps/new_map_" + to_string(i) + ".txt";
    }
    else if (fs::path(path).extension() != ".txt")
    {
        path += ".txt";
    }
}

void RhythmMapEditor::refreshFiles()
{
    files.clear();
    std::error_code ec;

    if (fs::exists("maps", ec))
    {
        for (const auto &e : fs::directory_iterator("maps", ec))
        {
            if (e.path().extension() != ".txt")
                continue;

            FileEntry fe;
            fe.path = "maps/" + e.path().filename().string();
            fe.label = e.path().stem().string();

            RhythmMap m;

            if (m.load(fe.path))
                fe.info = string(m.mode == RhythmMap::Mode::Grid ? "aim grid" : "4-lane") +
                          ", " + to_string(m.notes.size()) + " notes";
            else
                fe.info = "unreadable";

            files.push_back(fe);
        }
    }

    std::sort(files.begin(), files.end(),
              [](const FileEntry &a, const FileEntry &b)
              { return a.label < b.label; });

    browseSel = std::clamp(browseSel, 0, std::max(0, static_cast<int>(files.size()) - 1));
}

void RhythmMapEditor::openBrowser()
{
    browsing = true;
    playing = false;
    browseSel = 0;
    browseScroll = 0;
    refreshFiles();

    // start on the map currently being edited
    for (int i = 0; i < static_cast<int>(files.size()); i++)
        if (files[i].path == path)
            browseSel = i;

    if (browseSel >= BR_VISIBLE)
        browseScroll = browseSel - BR_VISIBLE + 1;
}

void RhythmMapEditor::browserPick()
{
    if (files.empty())
        return;

    if (loadFile(files[browseSel].path))
        browsing = false;
}

void RhythmMapEditor::browserKey(const sf::Event::KeyPressed &k)
{
    using Key = sf::Keyboard::Key;
    int n = static_cast<int>(files.size());

    switch (k.code)
    {
    case Key::Escape:
        browsing = false;
        break;

    case Key::Up:
        browseSel = std::max(0, browseSel - 1);
        break;

    case Key::Down:
        browseSel = std::min(std::max(0, n - 1), browseSel + 1);
        break;

    case Key::Enter:
        browserPick();
        break;

    case Key::Delete:
        if (n > 0)
        {
            std::error_code ec;
            fs::remove(files[browseSel].path, ec);
            setStatus("Deleted " + files[browseSel].path);
            refreshFiles();
        }
        break;

    default:
        if (k.control && k.code == Key::N)
            newMap();
        break;
    }

    if (browseSel < browseScroll)
        browseScroll = browseSel;
    if (browseSel >= browseScroll + BR_VISIBLE)
        browseScroll = browseSel - BR_VISIBLE + 1;
}

void RhythmMapEditor::browserClick(float mx, float my)
{
    if (mx < BR_X || mx > BR_X + BR_W || my < BR_ROW0)
        return;

    int row = static_cast<int>((my - BR_ROW0) / BR_ROW_H);

    if (row < 0 || row >= BR_VISIBLE)
        return;

    int idx = browseScroll + row;

    if (idx >= 0 && idx < static_cast<int>(files.size()))
    {
        if (idx == browseSel)
            browserPick(); // second click opens it
        else
            browseSel = idx;
    }
}

// ---------------- input ----------------
void RhythmMapEditor::handleEvent(const sf::Event &e)
{
    if (const auto *t = e.getIf<sf::Event::TextEntered>())
    {
        if (editingName)
        {
            char32_t u = t->unicode;

            if (u == 8)
            {
                if (!path.empty())
                    path.pop_back();
            }
            else if (u >= 32 && u < 127)
            {
                path.push_back(static_cast<char>(u));
            }
        }
        return;
    }

    if (const auto *k = e.getIf<sf::Event::KeyPressed>())
    {
        onKey(*k);
        return;
    }

    if (const auto *m = e.getIf<sf::Event::MouseButtonPressed>())
    {
        onMouse(*m);
        return;
    }

    if (const auto *wh = e.getIf<sf::Event::MouseWheelScrolled>())
        onWheel(*wh);
}

void RhythmMapEditor::onKey(const sf::Event::KeyPressed &k)
{
    using Key = sf::Keyboard::Key;

    if (editingName)
    {
        if (k.code == Key::Enter || k.code == Key::Escape)
            finishName();
        return;
    }

    if (browsing)
    {
        browserKey(k);
        return;
    }

    if (k.control)
    {
        if (k.code == Key::S)
            doSave();
        else if (k.code == Key::O)
            openBrowser();
        else if (k.code == Key::N)
            newMap();
        else if (k.code == Key::Z)
            undoStep();
        return;
    }

    float shiftMul = k.shift ? 10.f : 1.f;

    switch (k.code)
    {
    case Key::Escape:
        quit = true;
        break;

    case Key::Space:
        playing = !playing;
        lastBeat = -1;
        break;

    case Key::Tab:
        cur = 1 - cur;
        undo.clear();
        playing = false;
        setStatus(cur == 0 ? "Mode: 4-lane rhythm (A S D F)"
                           : "Mode: 3x3 aim grid (keys 1-9)");
        break;

    case Key::F2:
        editingName = true;
        break;

    case Key::Up:
        cursorTime = snap(cursorTime) + step();
        break;

    case Key::Down:
        cursorTime = std::max(0.f, snap(cursorTime) - step());
        break;

    case Key::PageUp:
        cursorTime += beatLen() * 4.f;
        break;

    case Key::PageDown:
        cursorTime = std::max(0.f, cursorTime - beatLen() * 4.f);
        break;

    case Key::Home:
        cursorTime = 0.f;
        break;

    case Key::LBracket:
        divIdx = std::max(0, divIdx - 1);
        break;

    case Key::RBracket:
        divIdx = std::min(7, divIdx + 1);
        break;

    case Key::Hyphen:
        bpm = std::max(30.f, bpm - shiftMul);
        break;

    case Key::Equal:
        bpm = std::min(300.f, bpm + shiftMul);
        break;

    case Key::Comma:
        offset -= 0.01f * shiftMul;
        break;

    case Key::Period:
        offset += 0.01f * shiftMul;
        break;

    case Key::Delete:
        clearAt(snap(cursorTime));
        break;

    default:
        placeKey(k.code);
        break;
    }
}

void RhythmMapEditor::onMouse(const sf::Event::MouseButtonPressed &m)
{
    float mx = static_cast<float>(m.position.x);
    float my = static_cast<float>(m.position.y);

    if (browsing)
    {
        if (m.button == sf::Mouse::Button::Left)
            browserClick(mx, my);
        return;
    }

    if (editingName)
        return;

    if (inTimeline(mx, my))
    {
        if (m.button == sf::Mouse::Button::Left)
        {
            int col = static_cast<int>((mx - x0()) / LANE_W);
            toggleNote(snap(tOf(my)), col, selRow);
        }
        else if (m.button == sf::Mouse::Button::Right)
        {
            removeNearest(mx, my);
        }
        return;
    }

    if (cur == 1 && m.button == sf::Mouse::Button::Left)
    {
        float px = (mx - PAD_X) / PAD_CELL;
        float py = (my - PAD_Y) / PAD_CELL;

        if (px >= 0.f && px < 3.f && py >= 0.f && py < 3.f)
        {
            selCol = static_cast<int>(px);
            selRow = static_cast<int>(py);
        }
    }
}

void RhythmMapEditor::onWheel(const sf::Event::MouseWheelScrolled &wh)
{
    if (wh.wheel != sf::Mouse::Wheel::Vertical)
        return;

    if (browsing)
    {
        int n = static_cast<int>(files.size());
        browseScroll = std::clamp(browseScroll - static_cast<int>(wh.delta), 0,
                                  std::max(0, n - BR_VISIBLE));
        return;
    }

    bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) ||
                sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);

    if (ctrl)
        pxPerSec = std::clamp(pxPerSec * std::pow(1.12f, wh.delta), 60.f, 800.f);
    else
        cursorTime = std::max(0.f, cursorTime + wh.delta * step());
}

// ---------------- update ----------------
void RhythmMapEditor::update(float dt)
{
    dt = std::min(dt, 0.05f);

    for (float &f : laneFlash)
        f = std::max(0.f, f - dt * 4.f);

    for (float &f : cellFlash)
        f = std::max(0.f, f - dt * 4.f);

    pulse = std::max(0.f, pulse - dt * 4.f);
    statusTimer = std::max(0.f, statusTimer - dt);

    if (!playing)
        return;

    float prev = cursorTime;
    cursorTime += dt;

    float last = 0.f;

    for (const MapNote &n : maps[cur].notes)
    {
        last = std::max(last, n.time);

        if (n.time > prev && n.time <= cursorTime)
            flashNote(n);
    }

    int beat = static_cast<int>(std::floor((cursorTime - offset) / beatLen()));

    if (beat != lastBeat)
    {
        lastBeat = beat;
        pulse = 1.f;
    }

    if (cursorTime > last + 2.f)
        playing = false;
}

// ---------------- map browser overlay ----------------
void RhythmMapEditor::drawBrowser(sf::RenderWindow &w, const sf::Font &font)
{
    auto txt = [&](const string &s, float x, float y, unsigned size, sf::Color c)
    {
        sf::Text t(font, s, size);
        t.setFillColor(c);
        t.setPosition({x, y});
        w.draw(t);
    };

    rect(w, 0.f, 0.f, 1280.f, 720.f, sf::Color(0, 0, 0, 170));

    const float h = 90.f + BR_VISIBLE * BR_ROW_H + 40.f;
    rect(w, BR_X, BR_Y, BR_W, h, PANEL, withA(CYAN, 0.8f), 2.f);

    txt("OPEN MAP", BR_X + 20.f, BR_Y + 14.f, 18, CYAN);
    txt("Up/Down: select   Enter or double-click: open   Del: delete file   "
        "Ctrl+N: new   Esc: close",
        BR_X + 20.f, BR_Y + 42.f, 11, DIM);

    if (files.empty())
        txt("No maps in maps/ yet. Press Ctrl+N to make one.",
            BR_X + 20.f, BR_ROW0 + 8.f, 14, GOLD);

    for (int row = 0; row < BR_VISIBLE; row++)
    {
        int idx = browseScroll + row;

        if (idx >= static_cast<int>(files.size()))
            break;

        float y = BR_ROW0 + row * BR_ROW_H;
        bool sel = (idx == browseSel);
        bool open = (files[idx].path == path);

        if (sel)
            rect(w, BR_X + 10.f, y, BR_W - 20.f, BR_ROW_H - 2.f,
                 withA(PINK, 0.25f), withA(PINK, 0.9f), 1.f);

        txt(files[idx].label, BR_X + 22.f, y + 4.f, 14, open ? GOLD : WHITE);
        txt(files[idx].info, BR_X + 330.f, y + 6.f, 12, DIM);

        if (open)
            txt("editing", BR_X + 510.f, y + 6.f, 11, GOLD);
    }

    if (static_cast<int>(files.size()) > BR_VISIBLE)
        txt("scroll for more (" + to_string(files.size()) + " maps)",
            BR_X + 20.f, BR_Y + h - 26.f, 11, DIM);
}

// ---------------- draw ----------------
void RhythmMapEditor::draw(sf::RenderWindow &w, const sf::Font &font)
{
    using sf::Color;

    w.setView(w.getDefaultView());
    w.clear(BG);

    auto txt = [&](const string &s, float x, float y, unsigned size, Color c,
                   bool centered = false)
    {
        sf::Text t(font, s, size);
        t.setFillColor(c);

        if (centered)
        {
            sf::FloatRect b = t.getLocalBounds();
            t.setOrigin({b.position.x + b.size.x / 2.f,
                         b.position.y + b.size.y / 2.f});
        }

        t.setPosition({x, y});
        w.draw(t);
    };

    const RhythmMap &m = maps[cur];
    const int C = cols();
    const float X0 = x0();
    const float TW = C * LANE_W;

    sf::Vector2i mp = sf::Mouse::getPosition(w);
    const float mx = static_cast<float>(mp.x);
    const float my = static_cast<float>(mp.y);

    // ---------- timeline background ----------
    rect(w, X0, TOP, TW, BOTTOM - TOP, PANEL);

    for (int c = 0; c < C; c++)
        rect(w, X0 + c * LANE_W, TOP, LANE_W, BOTTOM - TOP,
             withA(LANE_COL[c], 0.05f + 0.25f * laneFlash[c]));

    // ---------- beat / snap lines ----------
    {
        const float tMin = tOf(BOTTOM);
        const float tMax = tOf(TOP);
        const float s = step();
        const long k0 = static_cast<long>(std::floor((tMin - offset) / s));
        const long k1 = static_cast<long>(std::ceil((tMax - offset) / s));
        const long perBeat = DIVS[divIdx];
        const long perMeasure = perBeat * 4;

        if (k1 - k0 < 3000)
        {
            for (long k = k0; k <= k1; k++)
            {
                float t = offset + static_cast<float>(k) * s;

                if (t < -0.0001f)
                    continue;

                float y = yOf(t);

                if (y < TOP || y > BOTTOM)
                    continue;

                long inBeat = ((k % perBeat) + perBeat) % perBeat;
                long inMeasure = ((k % perMeasure) + perMeasure) % perMeasure;

                if (inMeasure == 0)
                {
                    rect(w, X0 - 6.f, y - 1.f, TW + 12.f, 2.f, withA(WHITE, 0.55f));
                    txt("M" + to_string(k / perMeasure + 1), X0 - 52.f, y - 8.f, 12, DIM);
                }
                else if (inBeat == 0)
                    rect(w, X0, y, TW, 1.f, withA(WHITE, 0.22f));
                else
                    rect(w, X0, y, TW, 1.f, withA(WHITE, 0.07f));
            }
        }
    }

    for (int c = 0; c <= C; c++)
        rect(w, X0 + c * LANE_W, TOP, 1.f, BOTTOM - TOP, withA(CYAN, 0.3f));

    // ---------- notes ----------
    for (const MapNote &n : m.notes)
    {
        float y = yOf(n.time);

        if (y < TOP - 14.f || y > BOTTOM + 14.f || n.col < 0 || n.col >= C)
            continue;

        float a = (n.time < cursorTime - 0.001f) ? 0.45f : 1.f;
        Color lc = LANE_COL[n.col];
        float bx = X0 + n.col * LANE_W + 12.f;
        float bw = LANE_W - 24.f;

        rect(w, bx - 3.f, y - 14.f, bw + 6.f, 28.f, withA(lc, 0.15f * a));
        rect(w, bx, y - 10.f, bw, 20.f, withA(lc, 0.85f * a), withA(WHITE, a), 1.f);

        if (cur == 1)
            txt(ROW_NAME[std::clamp(n.row, 0, 2)], bx + bw / 2.f, y - 1.f, 12, BG, true);
    }

    // ---------- ghost note under the mouse ----------
    if (!editingName && !browsing && inTimeline(mx, my))
    {
        int col = static_cast<int>((mx - X0) / LANE_W);
        float tg = snap(tOf(my));
        float gy = yOf(tg);
        float bx = X0 + col * LANE_W + 12.f;

        rect(w, bx, gy - 10.f, LANE_W - 24.f, 20.f, withA(LANE_COL[col], 0.15f),
             withA(WHITE, 0.6f), 1.f);
        txt(timeString(tg), X0 + TW + 8.f, gy - 8.f, 11, DIM);
    }

    // ---------- playhead ----------
    for (int c = 0; c < C; c++)
        if (laneFlash[c] > 0.f)
            rect(w, X0 + c * LANE_W, PLAY_Y - 16.f, LANE_W, 32.f,
                 withA(LANE_COL[c], 0.5f * laneFlash[c]));

    rect(w, X0 - 20.f, PLAY_Y - 1.f, TW + 40.f, 3.f, PINK);

    // ---------- side panel ----------
    rect(w, PANEL_X, 40.f, 1280.f - PANEL_X, 660.f, PANEL);
    rect(w, PANEL_X, 40.f, 2.f, 660.f, PINK);

    {
        char buf[64];

        auto info = [&](int i, const string &label, const string &val, Color vc)
        {
            float y = 62.f + i * 26.f;
            txt(label, PANEL_X + 24.f, y, 14, DIM);
            txt(val, PANEL_X + 150.f, y, 14, vc);
        };

        info(0, "MODE", cur == 0 ? "4-LANE RHYTHM" : "3x3 AIM GRID", CYAN);

        std::snprintf(buf, sizeof buf, "%.1f", bpm);
        info(1, "BPM", buf, CYAN);

        std::snprintf(buf, sizeof buf, "%.2f s", offset);
        info(2, "OFFSET", buf, CYAN);

        std::snprintf(buf, sizeof buf, "%d per beat", DIVS[divIdx]);
        info(3, "SNAP", buf, CYAN);

        info(4, "TIME", timeString(cursorTime) + (playing ? "   PLAYING" : ""),
             playing ? PINK : CYAN);

        info(5, "NOTES", to_string(m.notes.size()), CYAN);

        info(6, "FILE", editingName ? path + "_" : path,
             editingName ? PINK : WHITE);

        const char *help[] = {
            "Click lane: place/remove    Right-click: delete",
            "A S D F  or  1-9: place at playhead    Del: clear row",
            "Up/Down: step    Wheel: scroll    Ctrl+Wheel: zoom",
            "[ ]: snap    - =: BPM (Shift x10)    , .: offset",
            "Space: play    Tab: switch mode    Home: restart",
            "Ctrl+N new   Ctrl+O open   Ctrl+S save   Ctrl+Z undo",
            "F2: rename / save as   Esc: exit"};

        for (int i = 0; i < 7; i++)
            txt(help[i], PANEL_X + 24.f, 250.f + i * 19.f, 12, DIM);
    }

    // ---------- pad / key preview ----------
    if (cur == 1)
    {
        txt("AIM PAD  (click a cell to pick the row for timeline clicks)",
            PAD_X, PAD_Y - 30.f, 12, DIM);

        for (int row = 0; row < 3; row++)
        {
            for (int col = 0; col < 3; col++)
            {
                float x = PAD_X + col * PAD_CELL;
                float y = PAD_Y + row * PAD_CELL;
                bool sel = (col == selCol && row == selRow);
                float f = cellFlash[row * 3 + col];

                rect(w, x + 3.f, y + 3.f, PAD_CELL - 6.f, PAD_CELL - 6.f,
                     withA(CYAN, 0.04f + 0.4f * f),
                     withA(sel ? PINK : CYAN, sel ? 0.95f : 0.3f), sel ? 2.f : 1.f);
            }
        }

        // approach rings for notes about to be hit
        for (const MapNote &n : m.notes)
        {
            float dt = n.time - cursorTime;

            if (dt < -0.15f || dt > 0.8f || n.col < 0 || n.col > 2 || n.row < 0 || n.row > 2)
                continue;

            float k = std::clamp(dt / 0.8f, 0.f, 1.f);
            float size = PAD_CELL * (0.85f - 0.55f * k);
            float a = (dt < 0.f) ? 1.f + dt / 0.15f : 1.f - 0.7f * k;
            float cx = PAD_X + n.col * PAD_CELL + PAD_CELL / 2.f;
            float cy = PAD_Y + n.row * PAD_CELL + PAD_CELL / 2.f;

            rect(w, cx - size / 2.f, cy - size / 2.f, size, size,
                 Color::Transparent, withA(LANE_COL[n.col], a), 2.f);
        }
    }
    else
    {
        txt("LANE KEYS", PAD_X, PAD_Y - 30.f, 12, DIM);

        const char *keys[4] = {"A", "S", "D", "F"};

        for (int i = 0; i < 4; i++)
        {
            float x = PAD_X + i * 80.f;

            rect(w, x, PAD_Y + 80.f, 66.f, 66.f,
                 withA(LANE_COL[i], 0.08f + 0.6f * laneFlash[i]),
                 withA(LANE_COL[i], 0.8f), 2.f);
            txt(keys[i], x + 33.f, PAD_Y + 112.f, 20, WHITE, true);
        }
    }

    // ---------- header + footer (drawn last to mask overflow) ----------
    rect(w, 0.f, 0.f, 1280.f, 40.f, BAR);
    rect(w, 0.f, 39.f, 1280.f, 1.f, withA(CYAN, 0.25f));
    txt("RHYTHM MAP EDITOR", 15.f, 10.f, 16, CYAN);
    txt(path, 290.f, 13.f, 13, editingName ? PINK : WHITE);
    rect(w, 250.f, 14.f, 12.f, 12.f, withA(PINK, 0.2f + 0.8f * pulse));

    rect(w, 0.f, BOTTOM, 1280.f, 20.f, BAR);

    if (statusTimer > 0.f)
        txt(status, 14.f, BOTTOM + 3.f, 12, GOLD);
    else if (editingName)
        txt("Type a name, Enter to confirm (saved as maps/<name>.txt)", 14.f,
            BOTTOM + 3.f, 12, PINK);
    else
        txt(cur == 0 ? "Tab: switch to aim grid" : "Tab: switch to 4-lane", 14.f,
            BOTTOM + 3.f, 12, DIM);

    // ---------- browser overlay ----------
    if (browsing)
        drawBrowser(w, font);
}