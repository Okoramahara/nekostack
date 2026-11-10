#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include "NekoDesktop.h"
#include "../Core/NekoCore.h"
#include "NekoLauncher.h"
#include "NekoSettings.h"

#include <windowsx.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstdint>

// Neko Desktop 0.2 integrado como modulo grafico de NekoStack.
// Conserva la interfaz original y mantiene Explorer como herramienta auxiliar.

namespace {

constexpr int TITLE_H = 48;
constexpr int SIDEBAR_W = 214;
constexpr int BORDER = 7;
constexpr UINT TIMER_ID = 1;
constexpr int ACT_LAUNCH_APP_BASE = 1000;
constexpr int MAX_LAUNCHER_ACTIONS = 9;

const COLORREF C_BG       = RGB(13, 11, 19);
const COLORREF C_PANEL    = RGB(20, 17, 29);
const COLORREF C_PANEL2   = RGB(27, 22, 39);
const COLORREF C_PANEL3   = RGB(34, 27, 49);
const COLORREF C_PURPLE   = RGB(153, 105, 255);
const COLORREF C_PURPLE2  = RGB(105, 70, 180);
const COLORREF C_BORDER   = RGB(75, 51, 111);
const COLORREF C_TEXT     = RGB(235, 230, 245);
const COLORREF C_MUTED    = RGB(160, 151, 178);
const COLORREF C_GREEN    = RGB(119, 219, 166);
const COLORREF C_BLUE     = RGB(116, 178, 255);
const COLORREF C_ORANGE   = RGB(238, 177, 112);

HFONT g_fontSmall = nullptr;
HFONT g_fontBody = nullptr;
HFONT g_fontBold = nullptr;
HFONT g_fontTitle = nullptr;
HFONT g_fontBig = nullptr;
HFONT g_fontLogo = nullptr;

int g_page = 0; // 0 Inicio, 1 Aplicaciones, 2 Sistema, 3 Acerca de
int g_hoverAction = -1;
bool g_trackingMouse = false;
bool g_isMaximized = false;
RECT g_restoreRect{};
HWND g_hwnd = nullptr;
int g_refreshMs = 1000;
bool g_launcherEnabled = false;
bool g_launcherReady = false;
bool g_settingsEnabled = true;

struct HitItem { RECT rect; int action; };
std::vector<HitItem> g_hits;

enum Action {
    ACT_NONE = -1,
    ACT_HOME = 1,
    ACT_APPS,
    ACT_SYSTEM,
    ACT_ABOUT,
    ACT_EXPLORER,
    ACT_TERMINAL,
    ACT_NOTEPAD,
    ACT_TASKMGR,
    ACT_CONTROL,
    ACT_SETTINGS,
    ACT_NEKO_FOLDER,
    ACT_MINIMIZE,
    ACT_MAXRESTORE,
    ACT_CLOSE,
    ACT_MANAGE_APPS
};

ULONGLONG g_prevIdle = 0, g_prevKernel = 0, g_prevUser = 0;
bool g_haveCpuSample = false;
double g_cpuPercent = 0.0;
DWORDLONG g_memTotal = 0, g_memUsed = 0;
std::wstring g_userName = L"Usuario";
std::wstring g_computerName = L"PC";

ULONGLONG FileTimeValue(const FILETIME& ft) {
    ULARGE_INTEGER v{};
    v.LowPart = ft.dwLowDateTime;
    v.HighPart = ft.dwHighDateTime;
    return v.QuadPart;
}

void UpdateMetrics() {
    FILETIME idle{}, kernel{}, user{};
    if (GetSystemTimes(&idle, &kernel, &user)) {
        ULONGLONG i = FileTimeValue(idle);
        ULONGLONG k = FileTimeValue(kernel);
        ULONGLONG u = FileTimeValue(user);
        if (g_haveCpuSample) {
            ULONGLONG di = i - g_prevIdle;
            ULONGLONG dk = k - g_prevKernel;
            ULONGLONG du = u - g_prevUser;
            ULONGLONG total = dk + du;
            if (total > 0) {
                double busy = 1.0 - (static_cast<double>(di) / static_cast<double>(total));
                if (busy < 0.0) busy = 0.0;
                if (busy > 1.0) busy = 1.0;
                g_cpuPercent = busy * 100.0;
            }
        }
        g_prevIdle = i; g_prevKernel = k; g_prevUser = u;
        g_haveCpuSample = true;
    }

    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        g_memTotal = mem.ullTotalPhys;
        g_memUsed = mem.ullTotalPhys - mem.ullAvailPhys;
    }
}

void InitIdentity() {
    wchar_t name[128]{};
    DWORD n = GetEnvironmentVariableW(L"USERNAME", name, 127);
    if (n > 0 && n < 127) g_userName = name;
    n = 127;
    if (GetComputerNameW(name, &n)) g_computerName = name;
}

HFONT MakeFont(int height, int weight = FW_NORMAL) {
    return CreateFontW(height, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

void Text(HDC dc, const std::wstring& value, RECT r, COLORREF color,
          HFONT font, UINT flags = DT_LEFT | DT_VCENTER | DT_SINGLELINE) {
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    HGDIOBJ old = SelectObject(dc, font ? font : g_fontBody);
    DrawTextW(dc, value.c_str(), static_cast<int>(value.size()), &r, flags);
    SelectObject(dc, old);
}

void FillRectColor(HDC dc, const RECT& r, COLORREF color) {
    HBRUSH b = CreateSolidBrush(color);
    FillRect(dc, &r, b);
    DeleteObject(b);
}

void RoundCard(HDC dc, RECT r, COLORREF fill, COLORREF outline = C_BORDER, int radius = 14) {
    HBRUSH b = CreateSolidBrush(fill);
    HPEN p = CreatePen(PS_SOLID, 1, outline);
    HGDIOBJ oldB = SelectObject(dc, b);
    HGDIOBJ oldP = SelectObject(dc, p);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, radius, radius);
    SelectObject(dc, oldB);
    SelectObject(dc, oldP);
    DeleteObject(b);
    DeleteObject(p);
}

void AddHit(RECT r, int action) { g_hits.push_back({r, action}); }

void Button(HDC dc, RECT r, const std::wstring& label, int action,
            bool primary = false, int labelSize = 0) {
    bool hover = (g_hoverAction == action);
    COLORREF fill = primary ? (hover ? RGB(122, 83, 211) : C_PURPLE2)
                            : (hover ? C_PANEL3 : C_PANEL2);
    COLORREF edge = primary ? C_PURPLE : (hover ? C_PURPLE2 : C_BORDER);
    RoundCard(dc, r, fill, edge, 10);
    RECT t = r;
    t.left += 12; t.right -= 12;
    HFONT customFont = labelSize ? MakeFont(labelSize, FW_SEMIBOLD) : nullptr;
    Text(dc, label, t, C_TEXT, customFont ? customFont : g_fontBold,
         DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (customFont) DeleteObject(customFont);
    AddHit(r, action);
}

void SmallIcon(HDC dc, RECT r, const wchar_t* glyph, COLORREF accent) {
    RoundCard(dc, r, C_PANEL3, C_BORDER, 9);
    RECT t = r;
    Text(dc, glyph, t, accent, g_fontBold, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

std::wstring FormatBytes(DWORDLONG bytes) {
    double gb = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
    std::wostringstream out;
    out << std::fixed << std::setprecision(1) << gb << L" GB";
    return out.str();
}

std::wstring ClockText() {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t buf[64]{};
    wsprintfW(buf, L"%02u:%02u", st.wHour, st.wMinute);
    return buf;
}

std::wstring DateText() {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t buf[80]{};
    wsprintfW(buf, L"%02u/%02u/%04u", st.wDay, st.wMonth, st.wYear);
    return buf;
}

void DrawTopBar(HDC dc, int width) {
    RECT top{0, 0, width, TITLE_H};
    FillRectColor(dc, top, RGB(17, 14, 25));
    RECT line{0, TITLE_H - 1, width, TITLE_H};
    FillRectColor(dc, line, C_BORDER);

    RECT logo{18, 10, 47, 39};
    RoundCard(dc, logo, C_PURPLE2, C_PURPLE, 9);
    Text(dc, L"N", logo, C_TEXT, g_fontLogo, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    RECT brand{58, 5, 260, 27};
    Text(dc, L"NEKO DESKTOP", brand, C_TEXT, g_fontBold);
    RECT sub{58, 24, 280, 43};
    Text(dc, L"ENTORNO EXPERIMENTAL", sub, C_MUTED, g_fontSmall);

    RECT clock{width - 218, 7, width - 143, 28};
    Text(dc, ClockText(), clock, C_TEXT, g_fontBold, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    RECT date{width - 218, 25, width - 143, 43};
    Text(dc, DateText(), date, C_MUTED, g_fontSmall, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    int x = width - 132;
    RECT min{x, 0, x + 42, TITLE_H};
    RECT max{x + 42, 0, x + 84, TITLE_H};
    RECT close{x + 84, 0, x + 126, TITLE_H};
    if (g_hoverAction == ACT_MINIMIZE) FillRectColor(dc, min, C_PANEL3);
    if (g_hoverAction == ACT_MAXRESTORE) FillRectColor(dc, max, C_PANEL3);
    if (g_hoverAction == ACT_CLOSE) FillRectColor(dc, close, RGB(125, 42, 67));
    Text(dc, L"−", min, C_TEXT, g_fontTitle, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    Text(dc, g_isMaximized ? L"❐" : L"□", max, C_TEXT, g_fontBody, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    Text(dc, L"×", close, C_TEXT, g_fontTitle, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    AddHit(min, ACT_MINIMIZE);
    AddHit(max, ACT_MAXRESTORE);
    AddHit(close, ACT_CLOSE);
}

void DrawSidebar(HDC dc, int height) {
    RECT side{0, TITLE_H, SIDEBAR_W, height};
    FillRectColor(dc, side, RGB(16, 13, 23));
    RECT divider{SIDEBAR_W - 1, TITLE_H, SIDEBAR_W, height};
    FillRectColor(dc, divider, C_BORDER);

    RECT caption{22, 70, SIDEBAR_W - 18, 95};
    Text(dc, L"ESPACIO DE TRABAJO", caption, C_MUTED, g_fontSmall);

    struct Nav { const wchar_t* label; int action; int page; const wchar_t* mark; };
    const Nav nav[] = {
        {L"Inicio", ACT_HOME, 0, L"◆"},
        {L"Aplicaciones", ACT_APPS, 1, L"▦"},
        {L"Sistema", ACT_SYSTEM, 2, L"◈"},
        {L"Acerca de Neko", ACT_ABOUT, 3, L"●"}
    };
    int y = 108;
    for (const auto& item : nav) {
        RECT r{12, y, SIDEBAR_W - 12, y + 43};
        bool active = (g_page == item.page);
        if (active || g_hoverAction == item.action)
            RoundCard(dc, r, active ? RGB(43, 31, 61) : C_PANEL2,
                      active ? C_PURPLE2 : C_BORDER, 9);
        if (active) {
            RECT accent{12, y + 9, 15, y + 34};
            FillRectColor(dc, accent, C_PURPLE);
        }
        RECT mark{25, y + 2, 57, y + 41};
        Text(dc, item.mark, mark, active ? C_PURPLE : C_MUTED, g_fontBold,
             DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        RECT label{62, y + 2, SIDEBAR_W - 20, y + 41};
        Text(dc, item.label, label, active ? C_TEXT : C_MUTED, g_fontBody);
        AddHit(r, item.action);
        y += 51;
    }

    RECT info{15, height - 104, SIDEBAR_W - 15, height - 18};
    RoundCard(dc, info, C_PANEL, C_BORDER, 11);
    RECT dot{27, height - 87, 37, height - 77};
    HBRUSH b = CreateSolidBrush(C_GREEN);
    FillRect(dc, &dot, b); DeleteObject(b);
    RECT status{45, height - 94, SIDEBAR_W - 25, height - 72};
    Text(dc, L"SESION ACTIVA", status, C_GREEN, g_fontSmall);
    RECT usr{26, height - 67, SIDEBAR_W - 25, height - 44};
    Text(dc, g_userName, usr, C_TEXT, g_fontBold);
    RECT machine{26, height - 45, SIDEBAR_W - 25, height - 25};
    Text(dc, g_computerName, machine, C_MUTED, g_fontSmall);
}

void DrawMetricCard(HDC dc, RECT r, const wchar_t* label, const std::wstring& value,
                    const std::wstring& detail, COLORREF accent) {
    RoundCard(dc, r, C_PANEL, C_BORDER, 13);
    RECT stripe{r.left, r.top + 15, r.left + 3, r.bottom - 15};
    FillRectColor(dc, stripe, accent);
    RECT l{r.left + 17, r.top + 12, r.right - 12, r.top + 34};
    Text(dc, label, l, C_MUTED, g_fontSmall);
    RECT v{r.left + 17, r.top + 34, r.right - 12, r.top + 70};
    Text(dc, value, v, C_TEXT, g_fontTitle);
    RECT d{r.left + 17, r.bottom - 29, r.right - 12, r.bottom - 9};
    Text(dc, detail, d, accent, g_fontSmall);
}

void DrawAppCard(HDC dc, RECT r, const wchar_t* glyph, const wchar_t* title,
                 const wchar_t* desc, int action, COLORREF accent) {
    bool hover = (g_hoverAction == action);
    RoundCard(dc, r, hover ? C_PANEL3 : C_PANEL, hover ? C_PURPLE2 : C_BORDER, 13);
    RECT icon{r.left + 15, r.top + 16, r.left + 57, r.top + 58};
    SmallIcon(dc, icon, glyph, accent);
    RECT titleR{r.left + 69, r.top + 15, r.right - 12, r.top + 40};
    Text(dc, title, titleR, C_TEXT, g_fontBold);
    RECT descR{r.left + 15, r.top + 67, r.right - 12, r.bottom - 10};
    Text(dc, desc, descR, C_MUTED, g_fontSmall, DT_LEFT | DT_TOP | DT_WORDBREAK);
    AddHit(r, action);
}

void DrawHome(HDC dc, int x, int y, int width) {
    RECT heading{x, y, x + width, y + 34};
    Text(dc, L"Tu espacio, a tu manera.", heading, C_TEXT, g_fontTitle);
    RECT sub{x, y + 34, x + width, y + 58};
    Text(dc, L"Un entorno oscuro, ligero y bajo tu control.", sub, C_MUTED, g_fontBody);

    RECT hero{x, y + 78, x + width, y + 232};
    RoundCard(dc, hero, RGB(29, 22, 43), RGB(92, 62, 137), 16);
    RECT accent{x + 1, hero.top + 20, x + 5, hero.bottom - 20};
    FillRectColor(dc, accent, C_PURPLE);
    RECT eyebrow{x + 24, hero.top + 18, x + width - 210, hero.top + 43};
    Text(dc, L"NEKOSTACK  /  ENTORNO 0.5", eyebrow, C_PURPLE, g_fontSmall);
    RECT title{x + 24, hero.top + 48, x + width - 210, hero.top + 92};
    Text(dc, L"Bienvenido a tu nuevo espacio.", title, C_TEXT, g_fontBig);
    RECT caption{x + 24, hero.top + 94, x + width - 210, hero.top + 125};
    Text(dc, L"Prueba aplicaciones, consulta el estado del equipo y personaliza tu flujo.",
         caption, C_MUTED, g_fontBody, DT_LEFT | DT_VCENTER | DT_WORDBREAK);
    RECT open{x + width - 190, hero.top + 57, x + width - 24, hero.top + 103};
    Button(dc, open, L"ABRIR ARCHIVOS", ACT_EXPLORER, true);

    RECT section{x, y + 250, x + width, y + 278};
    Text(dc, L"ESTADO DEL EQUIPO", section, C_TEXT, g_fontBold);
    int gap = 12;
    int cardW = (width - gap * 2) / 3;
    int top = y + 286;
    DrawMetricCard(dc, RECT{x, top, x + cardW, top + 102}, L"USO DE CPU",
        std::to_wstring(static_cast<int>(g_cpuPercent + 0.5)) + L"%", L"Medición aproximada", C_PURPLE);
    std::wstring memValue = L"--";
    std::wstring memDetail = L"Memoria física";
    if (g_memTotal) {
        int pct = static_cast<int>((static_cast<double>(g_memUsed) / static_cast<double>(g_memTotal)) * 100.0 + 0.5);
        memValue = std::to_wstring(pct) + L"%";
        memDetail = FormatBytes(g_memUsed) + L" / " + FormatBytes(g_memTotal);
    }
    DrawMetricCard(dc, RECT{x + cardW + gap, top, x + cardW * 2 + gap, top + 102},
        L"MEMORIA RAM", memValue, memDetail, C_BLUE);
    DrawMetricCard(dc, RECT{x + cardW * 2 + gap * 2, top, x + width, top + 102},
        L"SESION", g_userName, L"Usuario actual", C_GREEN);

    RECT appHead{x, y + 405, x + width, y + 432};
    Text(dc, L"ACCESOS RAPIDOS", appHead, C_TEXT, g_fontBold);
    int appGap = 10;
    int appW = (width - appGap * 3) / 4;
    int appTop = y + 442;
    DrawAppCard(dc, RECT{x, appTop, x + appW, appTop + 100}, L"EX", L"Archivos", L"Abrir el Explorador", ACT_EXPLORER, C_BLUE);
    DrawAppCard(dc, RECT{x + appW + appGap, appTop, x + (appW + appGap) * 2 - appGap, appTop + 100}, L"PS", L"PowerShell", L"Abrir una consola", ACT_TERMINAL, C_PURPLE);
    DrawAppCard(dc, RECT{x + (appW + appGap) * 2, appTop, x + (appW + appGap) * 3 - appGap, appTop + 100}, L"TXT", L"Bloc de notas", L"Editar archivos de texto", ACT_NOTEPAD, C_ORANGE);
    DrawAppCard(dc, RECT{x + (appW + appGap) * 3, appTop, x + width, appTop + 100}, L"TM", L"Administrador", L"Procesos y rendimiento", ACT_TASKMGR, C_GREEN);

    RECT footer{x, y + 555, x + width, y + 580};
    Text(dc, L"NekoStack se ejecuta como una aplicacion normal. Explorer permanece intacto.",
         footer, C_MUTED, g_fontSmall);
}

COLORREF LauncherAccent(const std::wstring& name) {
    if (CompareStringOrdinal(name.c_str(), -1, L"Purple", -1, TRUE) == CSTR_EQUAL) return C_PURPLE;
    if (CompareStringOrdinal(name.c_str(), -1, L"Green", -1, TRUE) == CSTR_EQUAL) return C_GREEN;
    if (CompareStringOrdinal(name.c_str(), -1, L"Orange", -1, TRUE) == CSTR_EQUAL) return C_ORANGE;
    if (CompareStringOrdinal(name.c_str(), -1, L"Blue", -1, TRUE) == CSTR_EQUAL) return C_BLUE;
    return C_PURPLE;
}

void DrawApps(HDC dc, int x, int y, int width) {
    const int manageWidth = 174;
    const int textRight = (g_settingsEnabled && g_launcherEnabled && g_launcherReady)
        ? x + width - manageWidth - 14 : x + width;
    RECT heading{x, y, textRight, y + 36};
    Text(dc, L"Aplicaciones", heading, C_TEXT, g_fontTitle);
    RECT sub{x, y + 35, textRight, y + 60};
    Text(dc, L"Accesos configurables: puedes modificar la lista sin recompilar.", sub, C_MUTED, g_fontBody);
    if (g_settingsEnabled && g_launcherEnabled && g_launcherReady) {
        RECT manage{x + width - manageWidth, y + 3, x + width, y + 45};
        Button(dc, manage, L"ADMINISTRAR", ACT_MANAGE_APPS, true);
    }

    if (!g_launcherEnabled) {
        RECT panel{x, y + 88, x + width, y + 220};
        RoundCard(dc, panel, C_PANEL, C_BORDER, 13);
        RECT title{x + 20, panel.top + 20, x + width - 20, panel.top + 52};
        Text(dc, L"NekoLauncher esta desactivado", title, C_TEXT, g_fontBold);
        RECT detail{x + 20, panel.top + 58, x + width - 20, panel.top + 104};
        Text(dc, L"Abre Config\\Modules.ini y cambia Launcher=0 por Launcher=1.\nReinicia NekoStack para cargar el modulo.", detail, C_MUTED, g_fontBody, DT_LEFT | DT_TOP | DT_WORDBREAK);
        return;
    }

    if (!g_launcherReady) {
        RECT panel{x, y + 88, x + width, y + 220};
        RoundCard(dc, panel, C_PANEL, C_BORDER, 13);
        RECT title{x + 20, panel.top + 20, x + width - 20, panel.top + 52};
        Text(dc, L"No se pudo cargar NekoLauncher", title, C_ORANGE, g_fontBold);
        RECT detail{x + 20, panel.top + 58, x + width - 20, panel.top + 104};
        Text(dc, L"Comprueba Config\\Applications.ini y los permisos de la carpeta.", detail, C_MUTED, g_fontBody, DT_LEFT | DT_TOP | DT_WORDBREAK);
        return;
    }

    const std::vector<NekoLauncher::Application>& apps = NekoLauncher::GetApplications();
    if (apps.empty()) {
        RECT panel{x, y + 88, x + width, y + 220};
        RoundCard(dc, panel, C_PANEL, C_BORDER, 13);
        RECT title{x + 20, panel.top + 20, x + width - 20, panel.top + 52};
        Text(dc, L"No hay aplicaciones disponibles", title, C_TEXT, g_fontBold);
        RECT detail{x + 20, panel.top + 58, x + width - 20, panel.top + 104};
        Text(dc, L"Agrega entradas habilitadas en Config\\Applications.ini.", detail, C_MUTED, g_fontBody);
        return;
    }

    const int columns = 3;
    const int gap = 14;
    const int cardHeight = 126;
    const int cardWidth = (width - gap * (columns - 1)) / columns;
    const int top = y + 85;
    const std::size_t maxCount = static_cast<std::size_t>(MAX_LAUNCHER_ACTIONS);
    const std::size_t count = apps.size() < maxCount ? apps.size() : maxCount;

    for (std::size_t i = 0; i < count; ++i) {
        const int column = static_cast<int>(i % columns);
        const int row = static_cast<int>(i / columns);
        const int left = x + column * (cardWidth + gap);
        const int cardTop = top + row * (cardHeight + gap);
        RECT card{left, cardTop, left + cardWidth, cardTop + cardHeight};
        DrawAppCard(dc, card, apps[i].glyph.c_str(), apps[i].name.c_str(),
                    apps[i].description.c_str(), ACT_LAUNCH_APP_BASE + static_cast<int>(i),
                    LauncherAccent(apps[i].accent));
    }
}

void DrawSystem(HDC dc, int x, int y, int width) {
    RECT heading{x, y, x + width, y + 36};
    Text(dc, L"Sistema", heading, C_TEXT, g_fontTitle);
    RECT sub{x, y + 35, x + width, y + 60};
    Text(dc, L"Lecturas basicas del equipo actual; no se modifica ninguna configuracion.", sub, C_MUTED, g_fontBody);

    int gap = 14;
    int cw = (width - gap) / 2;
    int top = y + 88;
    std::wstring mem = L"No disponible";
    if (g_memTotal) mem = FormatBytes(g_memUsed) + L" usados de " + FormatBytes(g_memTotal);
    DrawMetricCard(dc, RECT{x, top, x + cw, top + 112}, L"PROCESADOR", std::to_wstring(static_cast<int>(g_cpuPercent + 0.5)) + L"%", L"Uso total aproximado", C_PURPLE);
    int pct = g_memTotal ? static_cast<int>((static_cast<double>(g_memUsed) / static_cast<double>(g_memTotal)) * 100.0 + 0.5) : 0;
    DrawMetricCard(dc, RECT{x + cw + gap, top, x + width, top + 112}, L"MEMORIA FISICA", g_memTotal ? std::to_wstring(pct) + L"%" : L"--", mem, C_BLUE);

    RECT info{x, top + 132, x + width, top + 278};
    RoundCard(dc, info, C_PANEL, C_BORDER, 13);
    RECT h{x + 18, info.top + 13, x + width - 18, info.top + 38};
    Text(dc, L"INFORMACION DE LA SESION", h, C_PURPLE, g_fontSmall);
    RECT u{x + 18, info.top + 49, x + width - 18, info.top + 75};
    Text(dc, L"Usuario: " + g_userName, u, C_TEXT, g_fontBody);
    RECT c{x + 18, info.top + 78, x + width - 18, info.top + 104};
    Text(dc, L"Equipo: " + g_computerName, c, C_TEXT, g_fontBody);
    RECT p{x + 18, info.top + 107, x + width - 18, info.top + 132};
    Text(dc, L"Modo: aplicacion de escritorio; Explorer sigue siendo el shell de Windows.", p, C_MUTED, g_fontSmall);

    RECT actions{x, info.bottom + 22, x + width, info.bottom + 49};
    Text(dc, L"HERRAMIENTAS", actions, C_TEXT, g_fontBold);
    RECT b1{x, info.bottom + 58, x + 190, info.bottom + 102};
    RECT b2{x + 204, info.bottom + 58, x + 394, info.bottom + 102};
    RECT b3{x + 408, info.bottom + 58, x + 598, info.bottom + 102};
    Button(dc, b1, L"Panel de control", ACT_CONTROL);
    Button(dc, b2, L"Configuracion", ACT_SETTINGS);
    Button(dc, b3, L"Administrador", ACT_TASKMGR);
}

void DrawAbout(HDC dc, int x, int y, int width) {
    RECT heading{x, y, x + width, y + 38};
    Text(dc, L"Acerca de NekoStack", heading, C_TEXT, g_fontTitle);
    RECT hero{x, y + 62, x + width, y + 248};
    RoundCard(dc, hero, RGB(29, 22, 43), RGB(92, 62, 137), 16);
    RECT logo{x + 24, hero.top + 25, x + 88, hero.top + 89};
    RoundCard(dc, logo, C_PURPLE2, C_PURPLE, 16);
    Text(dc, L"N", logo, C_TEXT, g_fontBig, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    RECT title{x + 108, hero.top + 25, x + width - 25, hero.top + 65};
    Text(dc, L"NEKOSTACK 0.5", title, C_TEXT, g_fontBig);
    RECT desc{x + 108, hero.top + 68, x + width - 25, hero.top + 118};
    Text(dc, L"Entorno Win32 modular para equipos modestos.\nInterfaz ligera, aplicaciones configurables y herramientas integradas.", desc, C_MUTED, g_fontBody, DT_LEFT | DT_TOP | DT_WORDBREAK);
    RECT note{x + 24, hero.bottom - 47, x + width - 24, hero.bottom - 18};
    Text(dc, L"No reemplaza Explorer, no instala servicios y no modifica el Registro.", note, C_GREEN, g_fontSmall);

    RECT details{x, y + 274, x + width, y + 390};
    RoundCard(dc, details, C_PANEL, C_BORDER, 13);
    RECT d1{x + 18, details.top + 14, x + width - 18, details.top + 40};
    Text(dc, L"TECNOLOGIA", d1, C_PURPLE, g_fontSmall);
    RECT d2{x + 18, details.top + 43, x + width - 18, details.top + 68};
    Text(dc, L"C++17  /  Win32 API  /  GDI  /  MinGW-w64", d2, C_TEXT, g_fontBody);
    RECT d3{x + 18, details.top + 72, x + width - 18, details.top + 97};
    Text(dc, L"Pensado para Windows 10 1607 y procesadores x64 sin AVX2.", d3, C_MUTED, g_fontBody);
}

void DrawDesktop(HDC dc, RECT client) {
    int width = client.right - client.left;
    int height = client.bottom - client.top;
    FillRectColor(dc, client, C_BG);

    HPEN borderPen = CreatePen(PS_SOLID, 2, C_PURPLE2);
    HGDIOBJ oldPen = SelectObject(dc, borderPen);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
    Rectangle(dc, 1, 1, width - 1, height - 1);
    SelectObject(dc, oldBrush); SelectObject(dc, oldPen); DeleteObject(borderPen);

    DrawTopBar(dc, width);
    DrawSidebar(dc, height);

    int x = SIDEBAR_W + 28;
    int y = TITLE_H + 22;
    int contentW = width - x - 28;
    if (contentW < 350) contentW = 350;

    if (g_page == 0) DrawHome(dc, x, y, contentW);
    else if (g_page == 1) DrawApps(dc, x, y, contentW);
    else if (g_page == 2) DrawSystem(dc, x, y, contentW);
    else DrawAbout(dc, x, y, contentW);
}

void Launch(const wchar_t* target, const wchar_t* args = nullptr) {
    HINSTANCE result = ShellExecuteW(g_hwnd, L"open", target, args, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        MessageBoxW(g_hwnd, L"No se pudo abrir esta aplicacion.", L"NekoStack", MB_OK | MB_ICONWARNING);
    }
}

void MaxRestore(HWND hwnd) {
    if (!g_isMaximized) {
        GetWindowRect(hwnd, &g_restoreRect);
        RECT work{};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
        SetWindowPos(hwnd, HWND_TOP, work.left, work.top,
            work.right - work.left, work.bottom - work.top,
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        g_isMaximized = true;
    } else {
        SetWindowPos(hwnd, HWND_TOP, g_restoreRect.left, g_restoreRect.top,
            g_restoreRect.right - g_restoreRect.left,
            g_restoreRect.bottom - g_restoreRect.top,
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        g_isMaximized = false;
    }
    InvalidateRect(hwnd, nullptr, TRUE);
}

void DoAction(HWND hwnd, int action) {
    if (action >= ACT_LAUNCH_APP_BASE && action < ACT_LAUNCH_APP_BASE + MAX_LAUNCHER_ACTIONS) {
        const std::size_t index = static_cast<std::size_t>(action - ACT_LAUNCH_APP_BASE);
        NekoLauncher::Launch(index, hwnd);
        return;
    }

    switch (action) {
    case ACT_HOME: g_page = 0; InvalidateRect(hwnd, nullptr, TRUE); break;
    case ACT_APPS: g_page = 1; InvalidateRect(hwnd, nullptr, TRUE); break;
    case ACT_SYSTEM: g_page = 2; InvalidateRect(hwnd, nullptr, TRUE); break;
    case ACT_ABOUT: g_page = 3; InvalidateRect(hwnd, nullptr, TRUE); break;
    case ACT_MANAGE_APPS:
        if (g_settingsEnabled && g_launcherReady) NekoSettings::OpenApplications(hwnd);
        g_hoverAction = ACT_NONE;
        InvalidateRect(hwnd, nullptr, TRUE);
        break;
    case ACT_EXPLORER: Launch(L"explorer.exe"); break;
    case ACT_TERMINAL: Launch(L"powershell.exe"); break;
    case ACT_NOTEPAD: Launch(L"notepad.exe"); break;
    case ACT_TASKMGR: Launch(L"taskmgr.exe"); break;
    case ACT_CONTROL: Launch(L"control.exe"); break;
    case ACT_SETTINGS: Launch(L"ms-settings:"); break;
    case ACT_NEKO_FOLDER: Launch(L"explorer.exe", NekoCore::GetPaths().root.c_str()); break;
    case ACT_MINIMIZE: ShowWindow(hwnd, SW_MINIMIZE); break;
    case ACT_MAXRESTORE: MaxRestore(hwnd); break;
    case ACT_CLOSE: DestroyWindow(hwnd); break;
    default: break;
    }
}

int FindHit(POINT p) {
    for (auto it = g_hits.rbegin(); it != g_hits.rend(); ++it) {
        if (PtInRect(&it->rect, p)) return it->action;
    }
    return ACT_NONE;
}

void CreateFonts() {
    g_fontSmall = MakeFont(15, FW_NORMAL);
    g_fontBody = MakeFont(18, FW_NORMAL);
    g_fontBold = MakeFont(18, FW_SEMIBOLD);
    g_fontTitle = MakeFont(25, FW_SEMIBOLD);
    g_fontBig = MakeFont(30, FW_BOLD);
    g_fontLogo = MakeFont(22, FW_BOLD);
}

void DestroyFonts() {
    HFONT* fonts[] = {&g_fontSmall, &g_fontBody, &g_fontBold, &g_fontTitle, &g_fontBig, &g_fontLogo};
    for (HFONT* f : fonts) { if (*f) { DeleteObject(*f); *f = nullptr; } }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        g_hwnd = hwnd;
        CreateFonts();
        InitIdentity();
        UpdateMetrics();
        g_refreshMs = NekoCore::ReadInt(L"Metrics", L"RefreshMs", 1000);
        if (g_refreshMs < 250) g_refreshMs = 250;
        if (g_refreshMs > 60000) g_refreshMs = 60000;
        g_launcherEnabled = NekoCore::IsModuleEnabled(L"Launcher", false);
        g_settingsEnabled = NekoCore::IsModuleEnabled(L"NekoSettings", true);
        g_launcherReady = g_launcherEnabled && NekoLauncher::Initialize(NekoCore::GetPaths().config);
        SetTimer(hwnd, TIMER_ID, static_cast<UINT>(g_refreshMs), nullptr);
        return 0;

    case WM_TIMER:
        if (wp == TIMER_ID) { UpdateMetrics(); InvalidateRect(hwnd, nullptr, FALSE); }
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC screen = BeginPaint(hwnd, &ps);
        RECT rc{}; GetClientRect(hwnd, &rc);
        int w = rc.right - rc.left, h = rc.bottom - rc.top;
        if (w > 0 && h > 0) {
            HDC mem = CreateCompatibleDC(screen);
            HBITMAP bmp = CreateCompatibleBitmap(screen, w, h);
            if (mem && bmp) {
                HGDIOBJ old = SelectObject(mem, bmp);
                g_hits.clear();
                DrawDesktop(mem, rc);
                BitBlt(screen, 0, 0, w, h, mem, 0, 0, SRCCOPY);
                SelectObject(mem, old);
            }
            if (bmp) DeleteObject(bmp);
            if (mem) DeleteDC(mem);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (!g_trackingMouse) {
            TRACKMOUSEEVENT tme{}; tme.cbSize = sizeof(tme); tme.dwFlags = TME_LEAVE; tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme); g_trackingMouse = true;
        }
        POINT p{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        int hit = FindHit(p);
        if (hit != g_hoverAction) { g_hoverAction = hit; InvalidateRect(hwnd, nullptr, FALSE); }
        return 0;
    }

    case WM_MOUSELEAVE:
        g_trackingMouse = false;
        if (g_hoverAction != ACT_NONE) { g_hoverAction = ACT_NONE; InvalidateRect(hwnd, nullptr, FALSE); }
        return 0;

    case WM_LBUTTONUP: {
        POINT p{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        int action = FindHit(p);
        if (action != ACT_NONE) DoAction(hwnd, action);
        return 0;
    }

    case WM_NCHITTEST: {
        if (g_isMaximized) {
            POINT maxPoint{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
            ScreenToClient(hwnd, &maxPoint);
            RECT maxRc{}; GetClientRect(hwnd, &maxRc);
            if (maxPoint.y >= 0 && maxPoint.y < TITLE_H && maxPoint.x < maxRc.right - 132) return HTCAPTION;
            return HTCLIENT;
        }
        POINT p{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        ScreenToClient(hwnd, &p);
        RECT rc{}; GetClientRect(hwnd, &rc);
        const int b = BORDER;
        bool left = p.x < b, right = p.x >= rc.right - b;
        bool top = p.y < b, bottom = p.y >= rc.bottom - b;
        if (top && left) return HTTOPLEFT;
        if (top && right) return HTTOPRIGHT;
        if (bottom && left) return HTBOTTOMLEFT;
        if (bottom && right) return HTBOTTOMRIGHT;
        if (left) return HTLEFT;
        if (right) return HTRIGHT;
        if (top) return HTTOP;
        if (bottom) return HTBOTTOM;
        if (p.y >= 0 && p.y < TITLE_H && p.x < rc.right - 132) return HTCAPTION;
        return HTCLIENT;
    }

    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lp);
        mmi->ptMinTrackSize.x = 820;
        mmi->ptMinTrackSize.y = 600;
        return 0;
    }

    case WM_SIZE:
        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ID);
        NekoLauncher::Shutdown();
        DestroyFonts();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

} // namespace

namespace NekoDesktop {

int Run(HINSTANCE instance, int showCommand, const wchar_t* windowTitle) {
    const wchar_t CLASS_NAME[] = L"NekoStackDesktopWindow";
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = CLASS_NAME;
    if (!RegisterClassExW(&wc)) {
        MessageBoxW(nullptr, L"No se pudo registrar la ventana de NekoStack.", L"NekoStack", MB_OK | MB_ICONERROR);
        return 1;
    }

    RECT work{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    int workW = work.right - work.left;
    int workH = work.bottom - work.top;
    int width = (workW * 92) / 100;
    int height = (workH * 88) / 100;
    if (width < 900) width = (workW > 900 ? 900 : workW);
    if (height < 620) height = (workH > 620 ? 620 : workH);
    int x = work.left + (workW - width) / 2;
    int y = work.top + (workH - height) / 2;

    HWND hwnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        CLASS_NAME,
        (windowTitle && *windowTitle) ? windowTitle : L"NekoStack",
        WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU,
        x, y, width, height,
        nullptr, nullptr, instance, nullptr);

    if (!hwnd) {
        MessageBoxW(nullptr, L"No se pudo crear la ventana de NekoStack.", L"NekoStack", MB_OK | MB_ICONERROR);
        return 2;
    }

    ShowWindow(hwnd, showCommand ? showCommand : SW_SHOWNORMAL);
    UpdateWindow(hwnd);

    MSG msg{};
    while (true) {
        BOOL result = GetMessageW(&msg, nullptr, 0, 0);
        if (result == -1) return 3;
        if (result == 0) break;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

} // namespace NekoDesktop
