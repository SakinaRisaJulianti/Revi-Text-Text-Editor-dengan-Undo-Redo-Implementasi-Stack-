#pragma once
#include <windows.h>
#include <string>
#include <cstring>
using namespace std;

const int SCREEN_WIDTH  = 70;
const int SCREEN_HEIGHT = 25;

void setCursor(int x, int y){
    COORD c = {(SHORT)x, (SHORT)y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

struct ConsoleBuffer {
    CHAR_INFO buffer[SCREEN_HEIGHT][SCREEN_WIDTH];
    HANDLE hOut;

    ConsoleBuffer(HANDLE h) : hOut(h){
        memset(buffer, 0, sizeof(buffer));
    }

    void clear(){
        for(int y = 0; y < SCREEN_HEIGHT; y++)
            for(int x = 0; x < SCREEN_WIDTH; x++){
                buffer[y][x].Char.AsciiChar = ' ';
                buffer[y][x].Attributes    = 7;
            }
    }

    void writeChar(int x, int y, char c, int color){
        if(x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT){
            buffer[y][x].Char.AsciiChar = c;
            buffer[y][x].Attributes    = color;
        }
    }

    void writeString(int x, int y, const string& s, int color){
        for(int i = 0; i < (int)s.size() && x + i < SCREEN_WIDTH; i++)
            writeChar(x + i, y, s[i], color);
    }

    void fillLine(int x, int y, int w, char c, int color){
        for(int i = x; i < x + w && i < SCREEN_WIDTH; i++)
            writeChar(i, y, c, color);
    }

    void flush(){
        COORD      size = {SCREEN_WIDTH, SCREEN_HEIGHT};
        COORD      pos  = {0, 0};
        SMALL_RECT rect = {0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1};
        WriteConsoleOutput(hOut, (CHAR_INFO*)buffer, size, pos, &rect);
    }
};
