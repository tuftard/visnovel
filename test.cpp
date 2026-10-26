#include <iostream>
#include <string>
using namespace std;

int main()
{
    // intro
    cout << "=== THE ONLY WAY ===" << endl;
    cout << "Welcome to the academy." << endl;
    cout << "Choose your character:" << endl;
    cout << "1. Shiro  2. Sora  3. Izuna  4. Tet" << endl;

    int choice = 0;
    cin >> choice;

    if (choice == 1)
    {
        cout << "Shiro: ..." << endl;
        cout << "Shiro: Don't slow me down." << endl;
        cout << "Shiro: Let's go." << endl;
    }
    else if (choice == 2)
    {
        cout << "Sora: Yo!" << endl;
        cout << "Sora: Ready to code?" << endl;
        cout << "Sora: Let's go." << endl;
    }
    else if (choice == 3)
    {
        cout << "Izuna: Welcome." << endl;
        cout << "Izuna: Mind the tags." << endl;
        cout << "Izuna: Let's go." << endl;
    }
    else if (choice == 4)
    {
        cout << "Tet: Hello." << endl;
        cout << "Tet: Shall we play?" << endl;
        cout << "Tet: Let's go." << endl;
    }

    return 0;
}