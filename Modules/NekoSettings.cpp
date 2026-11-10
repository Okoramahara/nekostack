#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include "NekoSettings.h"
#include "NekoLauncher.h"

#include <commdlg.h>
#include <string>
#include <vector>

namespace {

const wchar_t WINDOW_CLASS[] = L"NekoSettingsApplicationsWindow";
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 560;
const std::size_t MAX_APPS = 9;

const COLORREF DLG_BG = RGB(18, 15, 25);
const COLORREF DLG_TEXT = RGB(235, 230, 245);

const int IDC_APP_LIST = 1001;
const int IDC_NAME = 1002;
const int IDC_DESCRIPTION = 1003;
const int IDC_GLYPH = 1004;
const int IDC_COMMAND = 1005;
const int IDC_ARGUMENTS = 1006;
const int IDC_ACCENT = 1007;
const int IDC_ENABLED = 1008;
const int IDC_ADD = 1010;
const int IDC_REMOVE = 1011;
const int IDC_SAVE = 1012;
const int IDC_CLOSE = 1013;
const int IDC_BROWSE = 1014;

struct DialogState {
    HWND window = nullptr;
    HWND list = nullptr;
    HWND name = nullptr;
    HWND description = nullptr;
    HWND glyph = nullptr;
    HWND command = nullptr;
    HWND arguments = nullptr;
    HWND accent = nullptr;
    HWND enabled = nullptr;
    HFONT font = nullptr;
    HBRUSH background = nullptr;
    std::vector<NekoLauncher::Application> apps;
    int selected = -1;
};

std::wstring ReadControl(HWND control) {
    if (!control) return L"";
    const int length = GetWindowTextLengthW(control);
    std::vector<wchar_t> buffer(static_cast<std::size_t>(length) + 1, L'\0');
    GetWindowTextW(control, buffer.data(), length + 1);
    return std::wstring(buffer.data());
}

void SetControlText(HWND control, const std::wstring& value) {
    if (control) SetWindowTextW(control, value.c_str());
}

HWND AddControl(HWND parent, const wchar_t* className, const wchar_t* text,
                DWORD style, int x, int y, int width, int height, int id,
                HFONT font, DWORD exStyle = 0) {
    HWND control = CreateWindowExW(exStyle, className, text,
        WS_CHILD | WS_VISIBLE | style,
        x, y, width, height, parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        GetModuleHandleW(nullptr), nullptr);
    if (control && font) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return control;
}

void AddLabel(DialogState* state, const wchar_t* text, int x, int y, int width) {
    AddControl(state->window, L"STATIC", text, SS_LEFT | SS_CENTERIMAGE,
               x, y, width, 23, 0, state->font);
}

void SetEditorEnabled(DialogState* state, bool enabled) {
    HWND controls[] = {state->name, state->description, state->glyph,
                       state->command, state->arguments, state->accent,
                       state->enabled, GetDlgItem(state->window, IDC_REMOVE),
                       GetDlgItem(state->window, IDC_BROWSE)};
    for (HWND control : controls) if (control) EnableWindow(control, enabled);
}

void FillList(DialogState* state) {
    SendMessageW(state->list, LB_RESETCONTENT, 0, 0);
    for (std::size_t i = 0; i < state->apps.size(); ++i) {
        const NekoLauncher::Application& app = state->apps[i];
        std::wstring label = std::to_wstring(i + 1) + L". ";
        label += app.name.empty() ? L"(Sin nombre)" : app.name;
        label += app.enabled ? L"  [ACTIVA]" : L"  [OCULTA]";
        SendMessageW(state->list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }
    if (state->selected >= 0 && state->selected < static_cast<int>(state->apps.size())) {
        SendMessageW(state->list, LB_SETCURSEL, static_cast<WPARAM>(state->selected), 0);
    }
}

void LoadCurrent(DialogState* state) {
    const bool valid = state->selected >= 0 && state->selected < static_cast<int>(state->apps.size());
    SetEditorEnabled(state, valid);
    if (!valid) {
        SetControlText(state->name, L"");
        SetControlText(state->description, L"");
        SetControlText(state->glyph, L"");
        SetControlText(state->command, L"");
        SetControlText(state->arguments, L"");
        SendMessageW(state->accent, CB_SETCURSEL, 1, 0);
        SendMessageW(state->enabled, BM_SETCHECK, BST_UNCHECKED, 0);
        return;
    }

    const NekoLauncher::Application& app = state->apps[static_cast<std::size_t>(state->selected)];
    SetControlText(state->name, app.name);
    SetControlText(state->description, app.description);
    SetControlText(state->glyph, app.glyph);
    SetControlText(state->command, app.command);
    SetControlText(state->arguments, app.arguments);
    SendMessageW(state->enabled, BM_SETCHECK, app.enabled ? BST_CHECKED : BST_UNCHECKED, 0);

    int accentIndex = 1;
    if (CompareStringOrdinal(app.accent.c_str(), -1, L"Blue", -1, TRUE) == CSTR_EQUAL) accentIndex = 0;
    else if (CompareStringOrdinal(app.accent.c_str(), -1, L"Green", -1, TRUE) == CSTR_EQUAL) accentIndex = 2;
    else if (CompareStringOrdinal(app.accent.c_str(), -1, L"Orange", -1, TRUE) == CSTR_EQUAL) accentIndex = 3;
    SendMessageW(state->accent, CB_SETCURSEL, static_cast<WPARAM>(accentIndex), 0);
}

void CommitCurrent(DialogState* state) {
    if (state->selected < 0 || state->selected >= static_cast<int>(state->apps.size())) return;

    NekoLauncher::Application& app = state->apps[static_cast<std::size_t>(state->selected)];
    app.name = ReadControl(state->name);
    app.description = ReadControl(state->description);
    app.glyph = ReadControl(state->glyph);
    app.command = ReadControl(state->command);
    app.arguments = ReadControl(state->arguments);
    app.enabled = SendMessageW(state->enabled, BM_GETCHECK, 0, 0) == BST_CHECKED;

    const int accentIndex = static_cast<int>(SendMessageW(state->accent, CB_GETCURSEL, 0, 0));
    static const wchar_t* accents[] = {L"Blue", L"Purple", L"Green", L"Orange"};
    if (accentIndex >= 0 && accentIndex < 4) app.accent = accents[accentIndex];
}

void RefreshList(DialogState* state) {
    FillList(state);
    LoadCurrent(state);
}

std::wstring FileNameWithoutExtension(const std::wstring& path) {
    const std::wstring::size_type slash = path.find_last_of(L"\\/");
    std::wstring name = slash == std::wstring::npos ? path : path.substr(slash + 1);
    const std::wstring::size_type dot = name.find_last_of(L'.');
    if (dot != std::wstring::npos && dot > 0) name.resize(dot);
    return name;
}

void BrowseExecutable(DialogState* state) {
    if (state->selected < 0 || state->selected >= static_cast<int>(state->apps.size())) return;

    wchar_t path[32768]{};
    const std::wstring current = ReadControl(state->command);
    if (!current.empty() && current.size() < (sizeof(path) / sizeof(path[0]))) {
        DWORD attributes = GetFileAttributesW(current.c_str());
        if (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
            lstrcpynW(path, current.c_str(), static_cast<int>(sizeof(path) / sizeof(path[0])));
        }
    }

    const wchar_t filter[] =
        L"Aplicaciones (*.exe)\0*.exe\0"
        L"Accesos directos (*.lnk)\0*.lnk\0"
        L"Todos los archivos (*.*)\0*.*\0\0";
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = state->window;
    dialog.lpstrFilter = filter;
    dialog.nFilterIndex = 1;
    dialog.lpstrFile = path;
    dialog.nMaxFile = static_cast<DWORD>(sizeof(path) / sizeof(path[0]));
    dialog.lpstrTitle = L"Seleccionar programa o acceso directo";
    dialog.lpstrDefExt = L"exe";
    dialog.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
                   OFN_NOCHANGEDIR | OFN_HIDEREADONLY;

    if (!GetOpenFileNameW(&dialog)) return;

    SetControlText(state->command, path);

    const std::wstring currentName = ReadControl(state->name);
    if (currentName.empty() || currentName == L"Nueva aplicacion") {
        SetControlText(state->name, FileNameWithoutExtension(path));
    }
    SetFocus(state->arguments);
}

void AddNew(DialogState* state) {
    CommitCurrent(state);
    if (state->apps.size() >= MAX_APPS) {
        MessageBoxW(state->window, L"NekoLauncher admite hasta 9 aplicaciones.",
                    L"NekoSettings", MB_OK | MB_ICONINFORMATION);
        return;
    }

    NekoLauncher::Application app;
    app.name = L"Nueva aplicacion";
    app.description = L"Acceso personalizado";
    app.glyph = L"APP";
    app.command = L"";
    app.arguments = L"";
    app.accent = L"Purple";
    app.enabled = true;
    state->apps.push_back(app);
    state->selected = static_cast<int>(state->apps.size()) - 1;
    RefreshList(state);
    SetFocus(state->command);
}

void RemoveCurrent(DialogState* state) {
    if (state->selected < 0 || state->selected >= static_cast<int>(state->apps.size())) return;
    const int answer = MessageBoxW(state->window,
        L"Se eliminara esta entrada de Applications.ini. ¿Continuar?",
        L"Eliminar aplicacion", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
    if (answer != IDYES) return;

    state->apps.erase(state->apps.begin() + state->selected);
    if (state->apps.empty()) state->selected = -1;
    else if (state->selected >= static_cast<int>(state->apps.size())) state->selected = static_cast<int>(state->apps.size()) - 1;
    RefreshList(state);
}

void SaveCurrent(DialogState* state) {
    CommitCurrent(state);

    for (std::size_t i = 0; i < state->apps.size(); ++i) {
        const NekoLauncher::Application& app = state->apps[i];
        if (app.enabled && (app.name.find_first_not_of(L" \t") == std::wstring::npos ||
                            app.command.find_first_not_of(L" \t") == std::wstring::npos)) {
            std::wstring message = L"La aplicacion " + std::to_wstring(i + 1) +
                L" esta activa, pero le falta el nombre o el comando.\n\nCompleta esos campos o desactiva la entrada.";
            MessageBoxW(state->window, message.c_str(), L"NekoSettings", MB_OK | MB_ICONWARNING);
            state->selected = static_cast<int>(i);
            RefreshList(state);
            return;
        }
    }

    if (!NekoLauncher::SaveApplications(state->apps)) {
        MessageBoxW(state->window,
            L"No se pudieron guardar los cambios en Applications.ini. Comprueba los permisos de Config.",
            L"NekoSettings", MB_OK | MB_ICONERROR);
        return;
    }

    RefreshList(state);
    MessageBoxW(state->window,
        L"Aplicaciones guardadas correctamente. NekoLauncher ya tiene la nueva lista en memoria.",
        L"NekoSettings", MB_OK | MB_ICONINFORMATION);
}

void CreateUi(DialogState* state) {
    state->font = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    state->background = CreateSolidBrush(DLG_BG);

    AddLabel(state, L"APLICACIONES CONFIGURADAS", 20, 16, 245);
    AddLabel(state, L"DETALLES DEL ACCESO", 286, 16, 460);

    state->list = AddControl(state->window, L"LISTBOX", L"",
        LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_HASSTRINGS | WS_VSCROLL | WS_BORDER | WS_TABSTOP,
        20, 46, 245, 350, IDC_APP_LIST, state->font, WS_EX_CLIENTEDGE);

    AddControl(state->window, L"BUTTON", L"Agregar", BS_PUSHBUTTON | WS_TABSTOP,
        20, 410, 115, 34, IDC_ADD, state->font);
    AddControl(state->window, L"BUTTON", L"Eliminar", BS_PUSHBUTTON | WS_TABSTOP,
        150, 410, 115, 34, IDC_REMOVE, state->font);

    AddLabel(state, L"Nombre", 286, 54, 110);
    state->name = AddControl(state->window, L"EDIT", L"",
        ES_AUTOHSCROLL | WS_TABSTOP | WS_BORDER,
        402, 52, 355, 26, IDC_NAME, state->font, WS_EX_CLIENTEDGE);
    AddLabel(state, L"Descripcion", 286, 94, 110);
    state->description = AddControl(state->window, L"EDIT", L"",
        ES_AUTOHSCROLL | WS_TABSTOP | WS_BORDER,
        402, 92, 355, 26, IDC_DESCRIPTION, state->font, WS_EX_CLIENTEDGE);

    AddLabel(state, L"Distintivo", 286, 134, 110);
    state->glyph = AddControl(state->window, L"EDIT", L"",
        ES_AUTOHSCROLL | WS_TABSTOP | WS_BORDER,
        402, 132, 105, 26, IDC_GLYPH, state->font, WS_EX_CLIENTEDGE);
    AddLabel(state, L"Color", 520, 134, 64);
    state->accent = AddControl(state->window, L"COMBOBOX", L"",
        CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
        584, 132, 173, 180, IDC_ACCENT, state->font);
    SendMessageW(state->accent, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Blue"));
    SendMessageW(state->accent, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Purple"));
    SendMessageW(state->accent, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Green"));
    SendMessageW(state->accent, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Orange"));

    AddLabel(state, L"Programa / ejecutable", 286, 174, 220);
    state->command = AddControl(state->window, L"EDIT", L"",
        ES_AUTOHSCROLL | WS_TABSTOP | WS_BORDER,
        286, 198, 372, 26, IDC_COMMAND, state->font, WS_EX_CLIENTEDGE);
    AddControl(state->window, L"BUTTON", L"Buscar...", BS_PUSHBUTTON | WS_TABSTOP,
        670, 198, 87, 26, IDC_BROWSE, state->font);
    AddLabel(state, L"Argumentos (opcional; normalmente vacío)", 286, 238, 340);
    state->arguments = AddControl(state->window, L"EDIT", L"",
        ES_AUTOHSCROLL | WS_TABSTOP | WS_BORDER,
        286, 262, 471, 26, IDC_ARGUMENTS, state->font, WS_EX_CLIENTEDGE);

    state->enabled = AddControl(state->window, L"BUTTON", L"Aplicacion activa (visible en NekoLauncher)",
        BS_AUTOCHECKBOX | WS_TABSTOP,
        286, 310, 450, 28, IDC_ENABLED, state->font);

    AddControl(state->window, L"STATIC",
        L"Para agregar un programa, pulsa Buscar... y selecciona su archivo .exe o un acceso directo .lnk. Los argumentos son opcionales; normalmente puedes dejarlos vacíos.",
        SS_LEFT, 286, 355, 470, 52, 0, state->font);
    AddControl(state->window, L"BUTTON", L"Guardar cambios", BS_DEFPUSHBUTTON | WS_TABSTOP,
        445, 454, 145, 38, IDC_SAVE, state->font);
    AddControl(state->window, L"BUTTON", L"Cerrar", BS_PUSHBUTTON | WS_TABSTOP,
        610, 454, 145, 38, IDC_CLOSE, state->font);

    state->selected = state->apps.empty() ? -1 : 0;
    RefreshList(state);
}

LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    DialogState* state = reinterpret_cast<DialogState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        CREATESTRUCTW* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        state = static_cast<DialogState*>(create->lpCreateParams);
        state->window = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }
    if (!state) return DefWindowProcW(hwnd, message, wParam, lParam);

    switch (message) {
    case WM_CREATE:
        CreateUi(state);
        return 0;

    case WM_ERASEBKGND: {
        RECT rc{};
        GetClientRect(hwnd, &rc);
        FillRect(reinterpret_cast<HDC>(wParam), &rc, state->background);
        return 1;
    }

    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, DLG_TEXT);
        SetBkMode(dc, TRANSPARENT);
        return reinterpret_cast<LRESULT>(state->background);
    }

    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        const int code = HIWORD(wParam);
        if (id == IDC_APP_LIST && code == LBN_SELCHANGE) {
            CommitCurrent(state);
            const LRESULT selection = SendMessageW(state->list, LB_GETCURSEL, 0, 0);
            state->selected = selection == LB_ERR ? -1 : static_cast<int>(selection);
            LoadCurrent(state);
            return 0;
        }
        if (id == IDC_ADD && code == BN_CLICKED) { AddNew(state); return 0; }
        if (id == IDC_BROWSE && code == BN_CLICKED) { BrowseExecutable(state); return 0; }
        if (id == IDC_REMOVE && code == BN_CLICKED) { RemoveCurrent(state); return 0; }
        if (id == IDC_SAVE && code == BN_CLICKED) { SaveCurrent(state); return 0; }
        if (id == IDC_CLOSE && code == BN_CLICKED) { DestroyWindow(hwnd); return 0; }
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        if (state->font) { DeleteObject(state->font); state->font = nullptr; }
        if (state->background) { DeleteObject(state->background); state->background = nullptr; }
        state->window = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

bool RegisterWindowClass(HINSTANCE instance) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = SettingsWndProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = WINDOW_CLASS;
    if (RegisterClassExW(&wc)) return true;
    return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

} // namespace

namespace NekoSettings {

bool OpenApplications(HWND owner) {
    if (!NekoLauncher::IsInitialized()) {
        MessageBoxW(owner, L"NekoLauncher no esta inicializado. Activalo en Modules.ini.",
                    L"NekoSettings", MB_OK | MB_ICONWARNING);
        return false;
    }

    HINSTANCE instance = GetModuleHandleW(nullptr);
    if (!RegisterWindowClass(instance)) {
        MessageBoxW(owner, L"No se pudo registrar la ventana de NekoSettings.",
                    L"NekoSettings", MB_OK | MB_ICONERROR);
        return false;
    }

    DialogState state;
    state.apps = NekoLauncher::GetAllApplications();

    RECT parentRect{};
    if (owner && GetWindowRect(owner, &parentRect)) {
        // El centrado se calcula con la posicion actual de NekoStack.
    } else {
        parentRect.left = 0;
        parentRect.top = 0;
        parentRect.right = GetSystemMetrics(SM_CXSCREEN);
        parentRect.bottom = GetSystemMetrics(SM_CYSCREEN);
    }
    const int x = parentRect.left + ((parentRect.right - parentRect.left) - WINDOW_WIDTH) / 2;
    const int y = parentRect.top + ((parentRect.bottom - parentRect.top) - WINDOW_HEIGHT) / 2;

    if (owner) EnableWindow(owner, FALSE);
    HWND dialog = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT,
        WINDOW_CLASS, L"NekoSettings  |  Administrar aplicaciones",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN,
        x, y, WINDOW_WIDTH, WINDOW_HEIGHT, owner, nullptr, instance, &state);
    if (!dialog) {
        if (owner && IsWindow(owner)) EnableWindow(owner, TRUE);
        MessageBoxW(owner, L"No se pudo crear la ventana de NekoSettings.",
                    L"NekoSettings", MB_OK | MB_ICONERROR);
        return false;
    }

    ShowWindow(dialog, SW_SHOW);
    UpdateWindow(dialog);
    if (state.list) SetFocus(state.list);

    bool quitReceived = false;
    int quitCode = 0;
    MSG msg{};
    while (IsWindow(dialog)) {
        const BOOL result = GetMessageW(&msg, nullptr, 0, 0);
        if (result == -1) break;
        if (result == 0) {
            quitReceived = true;
            quitCode = static_cast<int>(msg.wParam);
            break;
        }
        if (!IsDialogMessageW(dialog, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (owner && IsWindow(owner)) {
        EnableWindow(owner, TRUE);
        SetActiveWindow(owner);
        SetForegroundWindow(owner);
        InvalidateRect(owner, nullptr, TRUE);
    }
    if (quitReceived) PostQuitMessage(quitCode);
    return true;
}

} // namespace NekoSettings
