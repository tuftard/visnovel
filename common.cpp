#define NOMINMAX
#include "Common.h"
#include <vector>
#include <windows.h>

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