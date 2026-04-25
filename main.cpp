#include <iostream>
#include <vector>
#include <string>
#include <stack>
#include <windows.h>
#include <fstream>
#include <algorithm>
#include <cstring>

using namespace std;

// ===== CONFIG =====
const int SCREEN_WIDTH = 70;
const int SCREEN_HEIGHT = 25;

// ===== UTIL =====
void setCursor(int x, int y){
    COORD c = {(SHORT)x, (SHORT)y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

// ===== DOUBLE BUFFER =====
struct ConsoleBuffer {
    CHAR_INFO buffer[SCREEN_HEIGHT][SCREEN_WIDTH];
    HANDLE hOut;

    ConsoleBuffer(HANDLE h) : hOut(h){
        memset(buffer,0,sizeof(buffer));
    }

    void clear(){
        for(int y=0;y<SCREEN_HEIGHT;y++)
            for(int x=0;x<SCREEN_WIDTH;x++){
                buffer[y][x].Char.AsciiChar=' ';
                buffer[y][x].Attributes=7;
            }
    }

    void writeChar(int x,int y,char c,int color){
        if(x>=0 && x<SCREEN_WIDTH && y>=0 && y<SCREEN_HEIGHT){
            buffer[y][x].Char.AsciiChar=c;
            buffer[y][x].Attributes=color;
        }
    }

    void writeString(int x,int y,const string& s,int color){
        for(int i=0;i<(int)s.size() && x+i<SCREEN_WIDTH;i++){
            writeChar(x+i,y,s[i],color);
        }
    }

    void fillLine(int x,int y,int w,char c,int color){
        for(int i=x;i<x+w && i<SCREEN_WIDTH;i++){
            writeChar(i,y,c,color);
        }
    }

    void flush(){
        COORD size={SCREEN_WIDTH,SCREEN_HEIGHT};
        COORD pos={0,0};
        SMALL_RECT rect={0,0,SCREEN_WIDTH-1,SCREEN_HEIGHT-1};
        WriteConsoleOutput(hOut,(CHAR_INFO*)buffer,size,pos,&rect);
    }
};

// ===== SNAPSHOT =====
struct Snapshot{
    vector<string> lines;
    int row,col;
};

// ===== EDITOR =====
class Editor{
    vector<string> lines={""};
    int row=0,col=0;

    int topLine=0;
    const int viewHeight=20;

    stack<Snapshot> undoS,redoS;

    // 🔥 SMART GROUPING
    DWORD lastInputTime = 0;

    void saveStateSmart(){
        DWORD now = GetTickCount();
        DWORD diff = now - lastInputTime;

        if(diff > 50){ // 👉 batas cepat/paste
            undoS.push({lines,row,col});
            while(!redoS.empty()) redoS.pop();
        }

        lastInputTime = now;
    }

public:
    int getVisibleLines(){ return viewHeight; }

    // ===== NAVIGATION =====
    void left(){
        if(col>0) col--;
        else if(row>0){ row--; col=lines[row].size(); }
    }

    void right(){
        if(col < (int)lines[row].size()) col++;
        else if(row < (int)lines.size()-1){ row++; col=0; }
    }

    void up(){
        if(row>0){
            row--;
            col=min(col,(int)lines[row].size());
        }
    }

    void down(){
        if(row < (int)lines.size()-1){
            row++;
            col=min(col,(int)lines[row].size());
        }
    }

    void home(){ col=0; }
    void end(){ col=lines[row].size(); }

    // ===== EDIT =====
    void insert(char c){
        saveStateSmart();
        lines[row].insert(col,1,c);
        col++;
    }

    void enter(){
        saveStateSmart();
        string newLine=lines[row].substr(col);
        lines[row]=lines[row].substr(0,col);
        lines.insert(lines.begin()+row+1,newLine);
        row++; col=0;
    }

    void backspace(){
        if(row==0 && col==0) return;
        saveStateSmart();

        if(col>0){
            lines[row].erase(col-1,1);
            col--;
        }else{
            col=lines[row-1].size();
            lines[row-1]+=lines[row];
            lines.erase(lines.begin()+row);
            row--;
        }
    }

    // ===== UNDO REDO =====
    void undo(){
        if(undoS.empty()) return;
        redoS.push({lines,row,col});
        auto s=undoS.top(); undoS.pop();
        lines=s.lines; row=s.row; col=s.col;
    }

    void redo(){
        if(redoS.empty()) return;
        undoS.push({lines,row,col});
        auto s=redoS.top(); redoS.pop();
        lines=s.lines; row=s.row; col=s.col;
    }

    // ===== DISPLAY (TIDAK DIUBAH) =====
    void adjustScroll(){
        if(row < topLine) topLine=row;
        if(row >= topLine+viewHeight) topLine=row-viewHeight+1;
    }

    void display(ConsoleBuffer& buf){
        adjustScroll();
        buf.clear();

        buf.fillLine(0,0,SCREEN_WIDTH,' ',240);
        buf.writeString(1,0,"File  Edit  View  Help",240);

        buf.fillLine(0,1,SCREEN_WIDTH,'-',7);

        for(int i=0;i<viewHeight;i++){
            int idx=topLine+i;
            if(idx < (int)lines.size()){
                buf.writeString(0,2+i,lines[idx],7);
            }
        }

        buf.fillLine(0,22,SCREEN_WIDTH,'-',7);

        string status="Ln "+to_string(row+1)+", Col "+to_string(col+1)
        +" | Ctrl+Z Undo | Ctrl+Y Redo";

        buf.fillLine(0,23,SCREEN_WIDTH,' ',112);
        buf.writeString(0,23,status,112);

        setCursor(col,2+(row-topLine));
        buf.flush();
    }
};

// ===== MAIN =====
int main(){

    HANDLE hIn=GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOut=GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD mode;
    GetConsoleMode(hIn,&mode);
    SetConsoleMode(hIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT);

    system("cls");

    Editor e;
    ConsoleBuffer buf(hOut);

    INPUT_RECORD rec;
    DWORD cnt;

    while(true){

        e.display(buf);

        ReadConsoleInput(hIn,&rec,1,&cnt);

        if(rec.EventType==KEY_EVENT){

            auto key=rec.Event.KeyEvent;
            if(!key.bKeyDown) continue;

            char ch=key.uChar.AsciiChar;

            if(key.wVirtualKeyCode==VK_ESCAPE) break;

            if(key.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)){
                if(key.wVirtualKeyCode=='Z') e.undo();
                else if(key.wVirtualKeyCode=='Y') e.redo();
                continue;
            }

            switch(key.wVirtualKeyCode){
                case VK_LEFT: e.left(); break;
                case VK_RIGHT: e.right(); break;
                case VK_UP: e.up(); break;
                case VK_DOWN: e.down(); break;
                case VK_HOME: e.home(); break;
                case VK_END: e.end(); break;
                case VK_BACK: e.backspace(); break;
                case VK_RETURN: e.enter(); break;
                default:
                    if(ch>=32 && ch<=126)
                        e.insert(ch);
            }
        }
    }

    setCursor(0, SCREEN_HEIGHT-1);
    cout<<"Program selesai.\n";
}
