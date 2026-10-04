#include "CodeEditorUtils.h"
#include <vector>
using namespace std;

// =============================================================
// C++ KEYWORD HELPER
// =============================================================

bool isKeyword(const string& token)
{
    static const vector<string> keywords = {
        "include", "int", "bool", "float", "char", "double", "void", "string",
        "using", "namespace", "if", "else", "while", "for", "return", "true", "false",
        "const", "static", "auto", "continue", "break", "class", "struct"
    };

    for (const auto& k : keywords)
    {
        if (token == k)
            return true;
    }

    return false;
}
