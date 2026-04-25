#include <iostream>
#include <windows.h>
#include "editor.h"
using namespace std;

int main(){
    HANDLE hIn  = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD mode;
    GetConsoleMode(hIn, &mode);
    SetConsoleMode(hIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT);

    system("cls");

    Editor        e;
    ConsoleBuffer buf(hOut);

    INPUT_RECORD rec;
    DWORD        cnt;

    while(true){
        e.display(buf);
        ReadConsoleInput(hIn, &rec, 1, &cnt);

        if(rec.EventType == KEY_EVENT){
            auto key = rec.Event.KeyEvent;
            if(!key.bKeyDown) continue;

            char ch = key.uChar.AsciiChar;

            if(key.wVirtualKeyCode == VK_ESCAPE) break;

            // Ctrl+Z / Ctrl+Y
            if(key.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)){
                if(key.wVirtualKeyCode == 'Z') e.undo();
                else if(key.wVirtualKeyCode == 'Y') e.redo();
                continue;
            }

            switch(key.wVirtualKeyCode){
                case VK_LEFT:   e.left();      break;
                case VK_RIGHT:  e.right();     break;
                case VK_UP:     e.up();        break;
                case VK_DOWN:   e.down();      break;
                case VK_HOME:   e.home();      break;
                case VK_END:    e.end();       break;
                case VK_BACK:   e.backspace(); break;
                case VK_RETURN: e.enter();     break;
                default:
                    if(ch >= 32 && ch <= 126) e.insert(ch);
            }
        }
    }

    setCursor(0, SCREEN_HEIGHT - 1);
    cout << "Program selesai.\n";
    return 0;
}
