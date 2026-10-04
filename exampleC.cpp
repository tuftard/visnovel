#include <iostream>
#include <string>
using namespace std;

void say(const string &who, const string &line)
{
    cout << "\n[" << who << "] " << line << "\n";
    cout << "   (press Enter)";
    cin.get();
}

int main()
{
    cout << "=== THE ONLY WAY ===\n";
    cout << "Type 1 to PLAY: ";

    int play = 0;
    cin >> play;
    cin.ignore(1000, '\n');

    if (play != 1)
        return 0;

    cout << "\nChoose a character:\n";
    cout << " 1) Shiro\n";
    cout << " 2) Sora\n";
    cout << "> ";

    int pick = 0;
    cin >> pick;
    cin.ignore(1000, '\n');

    if (pick == 1)
    {
        say("SHIRO", "...");
        say("SHIRO", "Pick Sora. She's the one with the question.");
        return 0;
    }

    say("SORA", "Hey. You picked me.");
    say("SORA", "What's up, wanna code...");
    say("SORA", "first tell me how you coded this without error");

    cout << "\n-- END OF SCENE --\n";
    cin.get();
    return 0;
}