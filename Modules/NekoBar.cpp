#include "NekoBar.h"

#include <windowsx.h>
#include <shellapi.h>
#include <vector>
#include <string>

namespace {
struct Item {
    std::wstring title, command, arguments;
    bool Empty() const { return command.empty(); }
};
std::vector<Item> g_tabs;
Item g_left, g_center, g_right;
HINSTANCE g_instance = nullptr;
HWND g_pill = nullptr, g_panel = nullptr;
bool g_open = false, g_manager = false;
int g_dragTab = -1, g_dragSlot = -1;
const COLORREF BG = RGB(25, 20, 35), PANEL = RGB(39, 31, 53);
const COLORREF EDGE = RGB(91, 65, 125), PURPLE = RGB(153, 105, 255);
const COLORREF TEXT = RGB(239, 233, 247), MUTED = RGB(174, 163, 190);

void Fill(HDC dc, RECT r, COLORREF c) {
    HBRUSH b = CreateSolidBrush(c); FillRect(dc, &r, b); DeleteObject(b);
}
void Round(HDC dc, RECT r, COLORREF c, COLORREF edge, int radius) {
    HBRUSH b=CreateSolidBrush(c); HPEN p=CreatePen(PS_SOLID,1,edge);
    HGDIOBJ ob=SelectObject(dc,b), op=SelectObject(dc,p);
    RoundRect(dc,r.left,r.top,r.right,r.bottom,radius,radius);
    SelectObject(dc,ob); SelectObject(dc,op); DeleteObject(b); DeleteObject(p);
}
void Label(HDC dc, const std::wstring& s, RECT r, COLORREF c, int size=16, bool bold=false, UINT flags=DT_CENTER|DT_VCENTER|DT_SINGLELINE) {
    HFONT f=CreateFontW(size,0,0,0,bold?FW_SEMIBOLD:FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH|FF_DONTCARE,L"Segoe UI");
    HGDIOBJ old=SelectObject(dc,f); SetBkMode(dc,TRANSPARENT); SetTextColor(dc,c);
    DrawTextW(dc,s.c_str(),(int)s.size(),&r,flags); SelectObject(dc,old); DeleteObject(f);
}
std::wstring ShortName(const Item& item) {
    if(item.title.empty()) return L"Arrastra aquí";
    return item.title;
}
void DrawSlot(HDC dc, RECT r, const Item& item, bool center, bool selected) {
    Round(dc,r,center?RGB(53,39,77):PANEL,center?PURPLE:EDGE,12);
    RECT preview{r.left+8,r.top+8,r.right-8,r.bottom-30};
    Round(dc,preview,RGB(31,26,43),RGB(65,53,82),7);
    RECT ptxt=preview; Label(dc,item.Empty()?L"Sin elemento":L"Vista previa",ptxt,MUTED,12,false);
    RECT logo{preview.left+5,preview.bottom-25,preview.left+30,preview.bottom};
    Round(dc,logo,PURPLE,PURPLE,6);
    std::wstring glyph=item.Empty()?L"+":item.title.substr(0,1);
    Label(dc,glyph,logo,TEXT,13,true);
    RECT title{r.left+8,r.bottom-25,r.right-8,r.bottom-5};
    Label(dc,ShortName(item),title,TEXT,13,true,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
    if(center && selected) {
        RECT mark{r.right-53,r.top+5,r.right-7,r.top+22};
        Round(dc,mark,RGB(72,49,105),PURPLE,8);
        Label(dc,L"CENTRO",mark,TEXT,9,true);
    }
}
void LayoutSlots(int w, RECT& l, RECT& c, RECT& r) {
    int cardW=145, gap=14, total=cardW*3+gap*2, x=(w-total)/2, y=14;
    l={x,y,x+cardW,y+112}; c={x+cardW+gap,y,x+cardW*2+gap,y+112};
    r={x+cardW*2+gap*2,y,x+cardW*3+gap*2,y+112};
}
void ShowPanel(bool show) {
    g_open=show;
    if(!g_panel) return;
    if(show) {
        RECT s{}; GetWindowRect(g_panel,&s);
        int h=g_manager?286:154;
        int sw=GetSystemMetrics(SM_CXSCREEN), sh=GetSystemMetrics(SM_CYSCREEN);
        SetWindowPos(g_panel,HWND_TOPMOST,(sw-520)/2,sh-h-42,520,h,SWP_NOACTIVATE|SWP_SHOWWINDOW);
    } else ShowWindow(g_panel,SW_HIDE);
    InvalidateRect(g_pill,nullptr,TRUE);
}
void ToggleManager() {
    g_manager=!g_manager;
    ShowPanel(true);
}
void SwapCenterWithTab(int i) {
    if(i<0 || i>=(int)g_tabs.size()) return;
    Item old=g_center; g_center=g_tabs[(size_t)i]; g_tabs[(size_t)i]=old;
}
void SwapCenterWithSlot(int slot) {
    if(slot==0) { Item t=g_center; g_center=g_left; g_left=t; }
    else if(slot==2) { Item t=g_center; g_center=g_right; g_right=t; }
}
void RunItem(const Item& item, HWND owner) {
    if(item.Empty()) return;
    const wchar_t* args=item.arguments.empty()?nullptr:item.arguments.c_str();
    ShellExecuteW(owner,L"open",item.command.c_str(),args,nullptr,SW_SHOWNORMAL);
}
void AddToCenter(const Item& item) {
    if(item.Empty()) return;
    if(g_center.Empty()) { g_center=item; return; }
    if(g_center.command==item.command && g_center.arguments==item.arguments) return;
    for(const auto& t:g_tabs) if(t.command==item.command && t.arguments==item.arguments) return;
    g_tabs.push_back(g_center); g_center=item;
}
void DrawPanel(HWND hwnd,HDC dc) {
    RECT rc{}; GetClientRect(hwnd,&rc); int w=rc.right;
    Fill(dc,rc,BG);
    RECT handle{w/2-34,5,w/2+34,8}; Round(dc,handle,RGB(104,83,130),RGB(104,83,130),3);
    RECT l{},c{},r{}; LayoutSlots(w,l,c,r);
    DrawSlot(dc,l,g_left,false,false); DrawSlot(dc,c,g_center,true,!g_center.Empty()); DrawSlot(dc,r,g_right,false,false);
    RECT menu{w/2-34,130,w/2+34,151};
    Round(dc,menu,RGB(48,37,66),EDGE,8);
    Label(dc,L"☰",menu,TEXT,17,true);
    if(g_manager) {
        RECT head{18,158,w-18,181}; Label(dc,L"GESTOR DE PESTAÑAS",head,MUTED,11,true,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
        int y=184;
        for(size_t i=0;i<g_tabs.size() && i<3;i++) {
            RECT row{18,y,w-18,y+29};
            Round(dc,row,(int)i==g_dragTab?RGB(65,48,89):PANEL,EDGE,7);
            RECT icon{row.left+6,row.top+4,row.left+27,row.bottom-4}; Round(dc,icon,PURPLE,PURPLE,5);
            std::wstring glyph=g_tabs[i].title.empty()?L"•":g_tabs[i].title.substr(0,1);
            Label(dc,glyph,icon,TEXT,11,true);
            RECT txt{row.left+35,row.top,row.right-8,row.bottom};
            Label(dc,g_tabs[i].title,txt,TEXT,12,false,DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
            y+=33;
        }
        if(g_tabs.empty()) { RECT empty{18,185,w-18,214}; Label(dc,L"Las aplicaciones abiertas aparecerán aquí.",empty,MUTED,12,false,DT_LEFT|DT_VCENTER|DT_SINGLELINE); }
    }
}
LRESULT CALLBACK PillProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    if(msg==WM_PAINT) {
        PAINTSTRUCT ps{}; HDC dc=BeginPaint(hwnd,&ps); RECT rc{}; GetClientRect(hwnd,&rc);
        Round(dc,rc,RGB(48,37,66),EDGE,18);
        Label(dc,g_open?L"Ocultar carrusel":L"Ver el carrusel",rc,TEXT,13,true);
        EndPaint(hwnd,&ps); return 0;
    }
    if(msg==WM_LBUTTONUP) { ShowPanel(!g_open); return 0; }
    return DefWindowProcW(hwnd,msg,wp,lp);
}
LRESULT CALLBACK PanelProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    switch(msg) {
    case WM_PAINT: { PAINTSTRUCT ps{}; HDC dc=BeginPaint(hwnd,&ps); DrawPanel(hwnd,dc); EndPaint(hwnd,&ps); return 0; }
    case WM_LBUTTONDOWN: {
        POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};
        RECT l{},c{},r{}; LayoutSlots(520,l,c,r);
        if(g_manager && p.y>=184 && p.y<283 && !g_tabs.empty()) {
            int i=(p.y-184)/33; if(i<(int)g_tabs.size() && i<3) { g_dragTab=i; SetCapture(hwnd); }
        } else if(PtInRect(&l,p)) g_dragSlot=0;
        else if(PtInRect(&c,p)) g_dragSlot=1;
        else if(PtInRect(&r,p)) g_dragSlot=2;
        return 0;
    }
    case WM_LBUTTONUP: {
        POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};
        RECT l{},c{},r{}; LayoutSlots(520,l,c,r);
        if(g_dragTab>=0) {
            if(PtInRect(&c,p)) SwapCenterWithTab(g_dragTab);
            else if(PtInRect(&l,p)) { Item t=g_tabs[(size_t)g_dragTab]; g_tabs[(size_t)g_dragTab]=g_left; g_left=t; }
            else if(PtInRect(&r,p)) { Item t=g_tabs[(size_t)g_dragTab]; g_tabs[(size_t)g_dragTab]=g_right; g_right=t; }
            g_dragTab=-1; ReleaseCapture(); InvalidateRect(hwnd,nullptr,TRUE); return 0;
        }
        if(g_dragSlot>=0) {
            if(g_dragSlot==0 || g_dragSlot==2) SwapCenterWithSlot(g_dragSlot);
            else RunItem(g_center,hwnd);
            g_dragSlot=-1; InvalidateRect(hwnd,nullptr,TRUE); return 0;
        }
        RECT menu{520/2-34,130,520/2+34,151};
        if(PtInRect(&menu,p)) ToggleManager();
        return 0;
    }
    case WM_DROPFILES: {
        HDROP drop=(HDROP)wp; wchar_t path[32768]{};
        if(DragQueryFileW(drop,0,path,32768)) {
            POINT dropPoint{}; DragQueryPoint(drop,&dropPoint);
            RECT l{},c{},r{}; LayoutSlots(520,l,c,r);
            std::wstring full=path, name=full.substr(full.find_last_of(L"\\/")+1);
            Item item{name,full,L""};
            if(PtInRect(&c,dropPoint)) AddToCenter(item);
            else if(PtInRect(&l,dropPoint)) { Item old=g_left; g_left=item; if(!old.Empty()) g_tabs.push_back(old); }
            else if(PtInRect(&r,dropPoint)) { Item old=g_right; g_right=item; if(!old.Empty()) g_tabs.push_back(old); }
            ShowPanel(true); InvalidateRect(hwnd,nullptr,TRUE);
        }
        DragFinish(drop); return 0;
    }
    case WM_RBUTTONUP: ShowPanel(false); return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}
}
namespace NekoBar {
bool Initialize(HINSTANCE instance) {
    g_instance=instance;
    WNDCLASSEXW p{}; p.cbSize=sizeof(p); p.lpfnWndProc=PillProc; p.hInstance=instance;
    p.hCursor=LoadCursorW(nullptr,IDC_HAND); p.hbrBackground=nullptr; p.lpszClassName=L"NekoBarPill01";
    if(!RegisterClassExW(&p) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) return false;
    WNDCLASSEXW b{}; b.cbSize=sizeof(b); b.lpfnWndProc=PanelProc; b.hInstance=instance;
    b.hCursor=LoadCursorW(nullptr,IDC_ARROW); b.hbrBackground=nullptr; b.lpszClassName=L"NekoBarPanel01";
    if(!RegisterClassExW(&b) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) return false;
    int sw=GetSystemMetrics(SM_CXSCREEN), sh=GetSystemMetrics(SM_CYSCREEN);
    g_pill=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,L"NekoBarPill01",L"",WS_POPUP,
        (sw-190)/2,sh-36,190,27,nullptr,nullptr,instance,nullptr);
    g_panel=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,L"NekoBarPanel01",L"",WS_POPUP,
        (sw-520)/2,sh-196,520,154,nullptr,nullptr,instance,nullptr);
    if(!g_pill||!g_panel) { Shutdown(); return false; }
    DragAcceptFiles(g_panel,TRUE); ShowWindow(g_pill,SW_HIDE); ShowWindow(g_panel,SW_HIDE);
    return true;
}
void PollCursor() {
    if(!g_pill||!g_panel) return;
    POINT p{}; GetCursorPos(&p);
    int sw=GetSystemMetrics(SM_CXSCREEN), sh=GetSystemMetrics(SM_CYSCREEN);
    bool nearBottom=(p.y>=sh-48 && p.x>=sw/2-180 && p.x<=sw/2+180);
    RECT pr{},br{}; GetWindowRect(g_pill,&pr); GetWindowRect(g_panel,&br);
    bool overPill=PtInRect(&pr,p)!=FALSE, overPanel=g_open && PtInRect(&br,p)!=FALSE;
    if(nearBottom||overPill||overPanel||g_open) {
        SetWindowPos(g_pill,HWND_TOPMOST,(sw-190)/2,sh-36,190,27,SWP_NOACTIVATE|SWP_SHOWWINDOW);
    } else ShowWindow(g_pill,SW_HIDE);
}
void NotifyOpened(const std::wstring& title,const std::wstring& command,const std::wstring& arguments) {
    Item item{title,command,arguments};
    if(g_center.Empty()) g_center=item;
    else if(g_center.command!=item.command || g_center.arguments!=item.arguments) {
        bool exists=false;
        for(const auto& tab:g_tabs) if(tab.command==item.command && tab.arguments==item.arguments) { exists=true; break; }
        if(!exists) g_tabs.push_back(item);
    }
    if(g_panel) InvalidateRect(g_panel,nullptr,TRUE);
}
void Shutdown() {
    if(g_panel) { DestroyWindow(g_panel); g_panel=nullptr; }
    if(g_pill) { DestroyWindow(g_pill); g_pill=nullptr; }
    g_tabs.clear(); g_left=Item{}; g_center=Item{}; g_right=Item{};
    g_open=false; g_manager=false; g_dragTab=-1; g_dragSlot=-1;
}
}
