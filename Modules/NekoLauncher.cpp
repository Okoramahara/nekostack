#include "NekoLauncher.h"
#include "../Core/NekoCore.h"

#include <shellapi.h>
#include <vector>

namespace {

const int MAX_APPS = 9;
std::vector<NekoLauncher::Application> g_allApps;
std::vector<NekoLauncher::Application> g_apps;
std::wstring g_iniPath;

std::wstring JoinPath(const std::wstring& base, const wchar_t* leaf) {
    std::wstring path = base;
    if (!path.empty() && path.back() != L'\\' && path.back() != L'/') path += L'\\';
    path += leaf;
    return path;
}

bool FileExists(const std::wstring& path) {
    const DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool WriteValue(const wchar_t* section, const wchar_t* key, const wchar_t* value) {
    return WritePrivateProfileStringW(section, key, value, g_iniPath.c_str()) != FALSE;
}

bool CreateDefaultFile() {
    if (!WriteValue(L"General", L"Count", L"6")) return false;

    struct DefaultApp {
        const wchar_t* name;
        const wchar_t* description;
        const wchar_t* glyph;
        const wchar_t* command;
        const wchar_t* arguments;
        const wchar_t* accent;
    };
    const DefaultApp defaults[] = {
        {L"Explorador de archivos", L"Carpetas, unidades y documentos", L"EX", L"explorer.exe", L"", L"Blue"},
        {L"PowerShell", L"Consola de comandos", L"PS", L"powershell.exe", L"-NoLogo", L"Purple"},
        {L"Bloc de notas", L"Editor de texto ligero", L"TXT", L"notepad.exe", L"", L"Orange"},
        {L"Administrador", L"Procesos y rendimiento", L"TM", L"taskmgr.exe", L"", L"Green"},
        {L"Panel de control", L"Configuracion clasica de Windows", L"CP", L"control.exe", L"", L"Blue"},
        {L"Carpeta NekoStack", L"Archivos del entorno", L"NK", L"explorer.exe", L"%NEKOSTACK%", L"Purple"}
    };

    for (int i = 0; i < 6; ++i) {
        const std::wstring section = L"App" + std::to_wstring(i + 1);
        if (!WritePrivateProfileStringW(section.c_str(), L"Enabled", L"1", g_iniPath.c_str())) return false;
        if (!WritePrivateProfileStringW(section.c_str(), L"Name", defaults[i].name, g_iniPath.c_str())) return false;
        if (!WritePrivateProfileStringW(section.c_str(), L"Description", defaults[i].description, g_iniPath.c_str())) return false;
        if (!WritePrivateProfileStringW(section.c_str(), L"Glyph", defaults[i].glyph, g_iniPath.c_str())) return false;
        if (!WritePrivateProfileStringW(section.c_str(), L"Command", defaults[i].command, g_iniPath.c_str())) return false;
        if (!WritePrivateProfileStringW(section.c_str(), L"Arguments", defaults[i].arguments, g_iniPath.c_str())) return false;
        if (!WritePrivateProfileStringW(section.c_str(), L"Accent", defaults[i].accent, g_iniPath.c_str())) return false;
    }

    WritePrivateProfileStringW(nullptr, nullptr, nullptr, g_iniPath.c_str());
    return true;
}

std::wstring ReadString(const wchar_t* section, const wchar_t* key, const wchar_t* fallback) {
    std::vector<wchar_t> buffer(1024, L'\0');
    GetPrivateProfileStringW(section, key, fallback ? fallback : L"", buffer.data(),
                             static_cast<DWORD>(buffer.size()), g_iniPath.c_str());
    return std::wstring(buffer.data());
}

bool LoadFromIni() {
    g_allApps.clear();
    g_apps.clear();

    int count = static_cast<int>(GetPrivateProfileIntW(L"General", L"Count", 0, g_iniPath.c_str()));
    if (count < 0) count = 0;
    if (count > MAX_APPS) count = MAX_APPS;

    for (int i = 1; i <= count; ++i) {
        const std::wstring section = L"App" + std::to_wstring(i);
        NekoLauncher::Application app;
        app.enabled = GetPrivateProfileIntW(section.c_str(), L"Enabled", 1, g_iniPath.c_str()) != 0;
        app.name = ReadString(section.c_str(), L"Name", L"");
        app.description = ReadString(section.c_str(), L"Description", L"");
        app.glyph = ReadString(section.c_str(), L"Glyph", L"APP");
        app.command = ReadString(section.c_str(), L"Command", L"");
        app.arguments = ReadString(section.c_str(), L"Arguments", L"");
        app.accent = ReadString(section.c_str(), L"Accent", L"Purple");
        g_allApps.push_back(app);

        if (app.enabled && !app.name.empty() && !app.command.empty()) {
            g_apps.push_back(app);
        }
    }
    return true;
}

bool ReplaceRootToken(std::wstring& arguments) {
    const std::wstring token = L"%NEKOSTACK%";
    const std::wstring::size_type pos = arguments.find(token);
    if (pos == std::wstring::npos) return true;
    if (!NekoCore::IsInitialized()) return false;

    std::wstring quotedRoot = L"\"" + NekoCore::GetPaths().root + L"\"";
    arguments.replace(pos, token.size(), quotedRoot);
    return true;
}

bool WriteApp(int index, const NekoLauncher::Application& app) {
    const std::wstring section = L"App" + std::to_wstring(index + 1);
    const wchar_t* enabled = app.enabled ? L"1" : L"0";
    return WriteValue(section.c_str(), L"Enabled", enabled) &&
           WriteValue(section.c_str(), L"Name", app.name.c_str()) &&
           WriteValue(section.c_str(), L"Description", app.description.c_str()) &&
           WriteValue(section.c_str(), L"Glyph", app.glyph.c_str()) &&
           WriteValue(section.c_str(), L"Command", app.command.c_str()) &&
           WriteValue(section.c_str(), L"Arguments", app.arguments.c_str()) &&
           WriteValue(section.c_str(), L"Accent", app.accent.c_str());
}

} // namespace

namespace NekoLauncher {

bool Initialize(const std::wstring& configDirectory) {
    g_allApps.clear();
    g_apps.clear();
    g_iniPath = JoinPath(configDirectory, L"Applications.ini");

    if (!FileExists(g_iniPath) && !CreateDefaultFile()) return false;
    return LoadFromIni();
}

void Shutdown() {
    g_allApps.clear();
    g_apps.clear();
    g_iniPath.clear();
}

bool IsInitialized() {
    return !g_iniPath.empty();
}

const std::vector<Application>& GetApplications() {
    return g_apps;
}

const std::vector<Application>& GetAllApplications() {
    return g_allApps;
}

bool SaveApplications(const std::vector<Application>& applications) {
    if (g_iniPath.empty() || applications.size() > static_cast<std::size_t>(MAX_APPS)) return false;

    for (std::size_t i = 0; i < applications.size(); ++i) {
        if (!WriteApp(static_cast<int>(i), applications[i])) return false;
    }

    const std::wstring count = std::to_wstring(applications.size());
    if (!WriteValue(L"General", L"Count", count.c_str())) return false;
    WritePrivateProfileStringW(nullptr, nullptr, nullptr, g_iniPath.c_str());
    return LoadFromIni();
}

bool Launch(std::size_t index, HWND owner) {
    if (index >= g_apps.size()) return false;

    const Application& app = g_apps[index];
    std::wstring arguments = app.arguments;
    if (!ReplaceRootToken(arguments)) {
        MessageBoxW(owner, L"No se pudo resolver la carpeta de NekoStack.", L"NekoLauncher", MB_OK | MB_ICONWARNING);
        return false;
    }

    const wchar_t* args = arguments.empty() ? nullptr : arguments.c_str();
    HINSTANCE result = ShellExecuteW(owner, L"open", app.command.c_str(), args, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        std::wstring message = L"No se pudo iniciar: " + app.name +
            L".\n\nEn Programa / ejecutable debe estar la ruta del archivo .exe o .lnk, no solo el nombre visible.\nAbre NekoSettings y utiliza Buscar... para seleccionar el programa.";
        MessageBoxW(owner, message.c_str(), L"NekoLauncher", MB_OK | MB_ICONWARNING);
        return false;
    }
    return true;
}

} // namespace NekoLauncher
