import re, shutil, sys

SRC = sys.argv[1] if len(sys.argv) > 1 else "anime.cpp"

with open(SRC, encoding="utf-8") as f:
    t = f.read()

orig = t


def fail(msg):
    print("PATCH FAILED:", msg)
    print("Nothing was written. Your file is untouched.")
    sys.exit(1)


def ind(text, n):
    pad = " " * n
    return "\n".join((pad + l) if l.strip() else l for l in text.split("\n"))


# helper: regex replace exactly once (literal replacement text)
def sub_once(pattern, repl, label, flags=re.S):
    global t
    m = re.search(pattern, t, flags)
    if not m:
        fail(label + " not found (already patched, or the code was edited)")
    t = t[: m.start()] + repl + t[m.end():]


# ---------------------------------------------------------------
# 1. remove the duplicate arrow-key / scroll block
# ---------------------------------------------------------------
i90 = t.find("scrollOffset -= scrollEvent->delta * 90.f;")
if i90 >= 0:
    start = t.rfind("\n            {\n", 0, i90)
    end_marker = ("        }\n\n        // =========================================================\n"
                  "        // RHYTHM UPDATE")
    end = t.find(end_marker, i90)
    if start < 0 or end < 0:
        fail("duplicate input block boundaries not found")
    t = t[: start + 1] + t[end:]
    print("1. removed duplicate arrow-key block")
else:
    print("1. duplicate block already gone")

# ---------------------------------------------------------------
# 2. drawRun: advance by fixed charW (same as the cursor)
# ---------------------------------------------------------------
new_drawrun = ind(
    """auto drawRun =
    [&](const string &textString, sf::Color color)
{
    if (textString.empty())
        return;

    if (drawY >= codeTop &&
        drawY + lineHeight <= codeBottom &&
        drawX < codeRight)
    {
        sf::Text t(editorFont, textString, 14);
        t.setFillColor(color);
        t.setPosition({drawX, drawY});
        window.draw(t);
    }

    drawX += static_cast<float>(textString.size()) * charW;
};""", 20).lstrip()
sub_once(r"auto drawRun =.*?drawX \+= width;\s*\};", new_drawrun, "drawRun lambda")
print("2. drawRun uses fixed character width")

# ---------------------------------------------------------------
# 3. unclosed quote must not run past the end of the line
# ---------------------------------------------------------------
new_str = ind(
    """size_t eol = code.find('\\n', i);
if (eol == string::npos)
    eol = code.size();

size_t end = code.find('"', i + 1);
if (end == string::npos || end > eol)
    end = (eol > i) ? eol - 1 : i;""", 28).lstrip()
sub_once(
    r"size_t end = code\.find\('\"', i \+ 1\);\s*if \(end == string::npos\)\s*\{\s*end = code\.size\(\) - 1;\s*\}",
    new_str, "string branch")
print("3. unclosed quotes handled")

# ---------------------------------------------------------------
# 4. cursor drawing: same math as the text, blink reset
# ---------------------------------------------------------------
new_cursor = ind(
    """size_t curLs = lineStartOf(code, cursorPosition);

float cursorX = codeLeft + static_cast<float>(cursorPosition - curLs) * charW;
float cursorY = codeTop +
                static_cast<float>(lineIndexOf(code, cursorPosition)) * lineHeight -
                scrollOffset;

if (cursorY >= codeTop &&
    cursorY + lineHeight <= codeBottom &&
    fmod(cursorClock.getElapsedTime().asSeconds(), 1.f) < 0.6f)
{
    cursor.setPosition({cursorX, cursorY});
    window.draw(cursor);
}""", 20).lstrip()
sub_once(r"float cursorX = codeLeft;.*?window\.draw\(cursor\);\s*\}", new_cursor, "cursor section")
print("4. cursor drawn with the same math as the text")

# ---------------------------------------------------------------
# 5. auto-scroll to keep the cursor visible
# ---------------------------------------------------------------
anchor = "float drawY = codeTop - scrollOffset;"
if t.count(anchor) != 1:
    fail("drawY anchor not found exactly once")
auto = ind(
    """

cursorPosition = min(cursorPosition, code.size());

if (cursorPosition != lastCursor)
{
    int cl = lineIndexOf(code, cursorPosition);
    float top = cl * lineHeight;
    float viewH = codeBottom - codeTop;

    if (top < scrollOffset)
        scrollOffset = top;
    else if (top + lineHeight > scrollOffset + viewH)
        scrollOffset = top + lineHeight - viewH;

    cursorClock.restart();
    lastCursor = cursorPosition;
    drawY = codeTop - scrollOffset;
}""", 20)
t = t.replace(anchor, anchor + auto)
print("5. auto-scroll added")

# ---------------------------------------------------------------
# 6. every rfind-based line start -> lineStartOf (fixes underflow)
# ---------------------------------------------------------------
pat = (r"size_t (\w+) =\s*code\.rfind\(\s*'\\n',\s*cursorPosition == 0 \? 0 : cursorPosition - 1\);"
       r"\s*if \(\1 == string::npos\)\s*(?:\{\s*\1 = 0;\s*\}|\1 = 0;)"
       r"\s*else\s*(?:\{\s*\1\+\+;\s*\}|\1\+\+;)")
t, n = re.subn(pat, r"size_t \1 = lineStartOf(code, cursorPosition);", t, flags=re.S)
if n == 0:
    fail("no rfind line-start patterns found")
print("6. replaced %d line-start lookups" % n)

# ---------------------------------------------------------------
# 7. Up / Down (+ Home, End, Delete)
# ---------------------------------------------------------------
new_up = ind(
    """else if (keyEvent->code == sf::Keyboard::Key::Up)
{
    size_t ls = lineStartOf(code, cursorPosition);
    size_t col = cursorPosition - ls;

    if (ls > 0)
    {
        size_t pls = lineStartOf(code, ls - 1);
        size_t plen = (ls - 1) - pls;
        cursorPosition = pls + min(col, plen);
    }
}
""", 20)
new_down = ind(
    """else if (keyEvent->code == sf::Keyboard::Key::Down)
{
    size_t ls = lineStartOf(code, cursorPosition);
    size_t col = cursorPosition - ls;
    size_t le = lineEndOf(code, cursorPosition);

    if (le < code.size())
    {
        size_t nls = le + 1;
        size_t nle = lineEndOf(code, nls);
        cursorPosition = nls + min(col, nle - nls);
    }
}
else if (keyEvent->code == sf::Keyboard::Key::Home)
{
    cursorPosition = lineStartOf(code, cursorPosition);
}
else if (keyEvent->code == sf::Keyboard::Key::End)
{
    cursorPosition = lineEndOf(code, cursorPosition);
}
else if (keyEvent->code == sf::Keyboard::Key::Delete)
{
    if (cursorPosition < code.size())
        code.erase(cursorPosition, 1);
}
""", 20)

sub_once(r" {20}else if \(keyEvent->code == sf::Keyboard::Key::Up\)\n {20}\{.*?\n {20}\}\n",
         new_up, "Up branch")
sub_once(r" {20}else if \(keyEvent->code == sf::Keyboard::Key::Down\)\n {20}\{.*?\n {20}\}\n",
         new_down, "Down branch")
print("7. Up/Down rewritten, Home/End/Delete added")

# ---------------------------------------------------------------
# 8. click to place the cursor
# ---------------------------------------------------------------
new_click_body = ind(
    """const float codeLeft = 45.f; // keep equal to the render section
const float lineHeight = 17.f;

int targetLine = max(0, static_cast<int>((mouseY - 55.f + scrollOffset) / lineHeight));

size_t ls = 0;

for (int l = 0; l < targetLine; l++)
{
    size_t nl = code.find('\\n', ls);

    if (nl == string::npos)
    {
        ls = string::npos;
        break;
    }

    ls = nl + 1;
}

if (ls == string::npos)
{
    cursorPosition = code.size();
}
else
{
    size_t le = lineEndOf(code, ls);
    int col = max(0, static_cast<int>(lround((mouseX - codeLeft) / charW)));
    cursorPosition = ls + min(static_cast<size_t>(col), le - ls);
}""", 24)
m = re.search(r"(mouseY <= 480\.f\)\s*\{)\s*const float codeLeft = 45\.f;.*?cursorPosition = targetPosition;(\s*\})",
              t, re.S)
if not m:
    fail("click handler not found")
t = t[: m.start()] + m.group(1) + "\n" + new_click_body + m.group(2) + t[m.end():]
print("8. click-to-place rewritten")

# ---------------------------------------------------------------
# 9. reset scroll/blink tracking when the editor opens
# ---------------------------------------------------------------
if t.count("editorOpen = true;") != 1:
    fail("editorOpen = true; not found exactly once")
t = t.replace("editorOpen = true;",
              "editorOpen = true;\n\n                        lastCursor = static_cast<size_t>(-1);")
print("9. editor open resets cursor tracking")

# ---------------------------------------------------------------
# sanity checks, then write
# ---------------------------------------------------------------
for needle in ("lineStartOf(code, cursorPosition)", "charW", "lastCursor"):
    if needle not in t:
        fail("sanity check failed: " + needle)

if t.count("* 8.4f") != 0:
    print("note: a hardcoded 8.4f is still present somewhere (probably the Shiro terminal, which is fine)")

shutil.copyfile(SRC, "anime_backup_cursor.cpp")
with open(SRC, "w", encoding="utf-8") as f:
    f.write(t)

print("\nPatched OK. Backup saved as anime_backup_cursor.cpp")
