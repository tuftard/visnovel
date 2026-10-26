#define NOMINMAX
#include "Common.h"
#include <vector>
#include <windows.h>
#include <string>
#include <algorithm>
#include <initializer_list>

bool isKeyword(const std::string &token)
{
    static const std::vector<std::string> keywords = {
        "include", "int", "bool", "float", "char", "double", "void", "string",
        "using", "namespace", "if", "else", "while", "for", "return", "true", "false",
        "const", "static", "auto", "continue", "break", "class", "struct"};

    for (const auto &k : keywords)
        if (token == k)
            return true;
    return false;
}

bool isBuiltin(const std::string &t)
{
    static const std::vector<std::string> b = {
        "cout", "cin", "endl", "std", "vector", "printf", "scanf", "main"};

    for (const auto &x : b)
        if (t == x)
            return true;
    return false;
}

std::string getClipboardText()
{
    if (!OpenClipboard(nullptr))
        return "";

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData)
    {
        CloseClipboard();
        return "";
    }

    wchar_t *text = static_cast<wchar_t *>(GlobalLock(hData));
    if (!text)
    {
        CloseClipboard();
        return "";
    }

    int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);

    std::string result;
    if (size > 1)
    {
        result.resize(size - 1);
        WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), size, nullptr, nullptr);
    }

    GlobalUnlock(hData);
    CloseClipboard();
    return result;
}
struct FightResult
{
    bool won;
    float accuracy;
    float timeSec;
};

class Fight
{
public:
    virtual ~Fight() = default;
    virtual void start() = 0;
    virtual void update(float dt) = 0;
    virtual void draw() = 0;
    virtual bool finished() const = 0;
    virtual FightResult result() const = 0;
};
static std::string stripComments(const std::string &s)
{
    std::string out;
    for (size_t i = 0; i < s.size(); i++)
    {
        if (s[i] == '/' && i + 1 < s.size() && s[i + 1] == '/')
        {
            while (i < s.size() && s[i] != '\n')
                i++;
            out += '\n';
        }
        else if (s[i] == '/' && i + 1 < s.size() && s[i + 1] == '*')
        {
            size_t e = s.find("*/", i + 2);
            i = (e == std::string::npos) ? s.size() : e + 1;
        }
        else
            out += s[i];
    }
    return out;
}

static int countOf(const std::string &s, const std::string &key)
{
    int n = 0;
    for (size_t p = s.find(key); p != std::string::npos; p = s.find(key, p + key.size()))
        n++;
    return n;
}

static int countOutputs(const std::string &s)
{
    return countOf(s, "cout") + countOf(s, "printf") + countOf(s, "puts");
}

static size_t firstInput(const std::string &s)
{
    size_t best = std::string::npos;
    for (const char *k : {"cin", "getline", "scanf"})
        best = std::min(best, s.find(k));
    return best;
}

char gradeCode(const std::string &raw)
{
    std::string c = stripComments(raw);

    size_t in = firstInput(c);
    if (in == std::string::npos)
        return 'F'; // no character selection

    if (countOutputs(c.substr(0, in)) < 2)
        return 'F'; // no intro

    std::string after = c.substr(in);
    int branches = countOf(after, "case ") + countOf(after, "if (") + countOf(after, "if(");
    if (branches < 2)
        return 'F'; // fewer than 2 characters

    int lines = countOutputs(after);
    if (lines < branches)
        return 'F'; // missing lines per character

    if (branches >= 4 && lines >= branches * 2)
        return 'A';
    if (branches >= 3)
        return 'B';
    return 'C';
}