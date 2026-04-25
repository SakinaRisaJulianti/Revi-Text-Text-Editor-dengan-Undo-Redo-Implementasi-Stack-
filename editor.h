#pragma once
#include <vector>
#include <string>
#include <stack>
#include <algorithm>
#include <windows.h>
#include "buffer.h"
#include "snapshot.h"
using namespace std;

class Editor {
    vector<string> lines = {""};
    int row = 0, col = 0;

    int topLine = 0;
    const int viewHeight = 20;

    stack<Snapshot> undoS, redoS;
    DWORD lastInputTime = 0;

    void saveStateSmart(){
        DWORD now  = GetTickCount();
        DWORD diff = now - lastInputTime;
        if(diff > 50){
            undoS.push({lines, row, col});
            while(!redoS.empty()) redoS.pop();
        }
        lastInputTime = now;
    }

public:
    int getVisibleLines(){ return viewHeight; }

    // ===== NAVIGASI =====
    void left(){
        if(col > 0) col--;
        else if(row > 0){ row--; col = lines[row].size(); }
    }

    void right(){
        if(col < (int)lines[row].size()) col++;
        else if(row < (int)lines.size() - 1){ row++; col = 0; }
    }

    void up(){
        if(row > 0){ row--; col = min(col, (int)lines[row].size()); }
    }

    void down(){
        if(row < (int)lines.size() - 1){ row++; col = min(col, (int)lines[row].size()); }
    }

    void home(){ col = 0; }
    void end() { col = lines[row].size(); }

    // ===== EDIT =====
    void insert(char c){
        saveStateSmart();
        lines[row].insert(col, 1, c);
        col++;
    }

    void enter(){
        saveStateSmart();
        string newLine  = lines[row].substr(col);
        lines[row]      = lines[row].substr(0, col);
        lines.insert(lines.begin() + row + 1, newLine);
        row++; col = 0;
    }

    void backspace(){
        if(row == 0 && col == 0) return;
        saveStateSmart();
        if(col > 0){
            lines[row].erase(col - 1, 1);
            col--;
        } else {
            col = lines[row - 1].size();
            lines[row - 1] += lines[row];
            lines.erase(lines.begin() + row);
            row--;
        }
    }

    // ===== UNDO / REDO =====
    void undo(){
        if(undoS.empty()) return;
        redoS.push({lines, row, col});
        auto s = undoS.top(); undoS.pop();
        lines = s.lines; row = s.row; col = s.col;
    }

    void redo(){
        if(redoS.empty()) return;
        undoS.push({lines, row, col});
        auto s = redoS.top(); redoS.pop();
        lines = s.lines; row = s.row; col = s.col;
    }

    // ===== TAMPILAN =====
    void adjustScroll(){
        if(row < topLine) topLine = row;
        if(row >= topLine + viewHeight) topLine = row - viewHeight + 1;
    }

    void display(ConsoleBuffer& buf){
        adjustScroll();
        buf.clear();

        // Menu bar
        buf.fillLine(0, 0, SCREEN_WIDTH, ' ', 240);
        buf.writeString(1, 0, "File  Edit  View  Help", 240);

        // Garis pemisah atas
        buf.fillLine(0, 1, SCREEN_WIDTH, '-', 7);

        // Isi teks
        for(int i = 0; i < viewHeight; i++){
            int idx = topLine + i;
            if(idx < (int)lines.size())
                buf.writeString(0, 2 + i, lines[idx], 7);
        }

        // Garis pemisah bawah
        buf.fillLine(0, 22, SCREEN_WIDTH, '-', 7);

        // Status bar
        string status = "Ln " + to_string(row + 1) + ", Col " + to_string(col + 1)
                      + " | Ctrl+Z Undo | Ctrl+Y Redo";
        buf.fillLine(0, 23, SCREEN_WIDTH, ' ', 112);
        buf.writeString(0, 23, status, 112);

        setCursor(col, 2 + (row - topLine));
        buf.flush();
    }
};
