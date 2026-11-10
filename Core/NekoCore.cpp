#include "NekoCore.h"

#include <vector>

namespace {

NekoCore::Paths g_paths;
bool g_initialized = false;

std::wstring JoinPath(const std::wstring& base, const wchar_t* child) {
    if (base.empty()) return std::wstring(child ? child : L"");

    std::wstring result = base;
    if (result.back() != L'\\' && result.back() != L'/') {
        result += L'\\';
    }
    if (child) result += child;
    return result;
}

bool EnsureDirectory(const std::wstring& path) {
    if (CreateDirectoryW(path.c_str(), nullptr)) return true;

    const DWORD error = GetLastError();
    if (error != ERROR_ALREADY_EXISTS) return false;

    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool FileExists(const std::wstring& path) {
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES &&
           (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool CreateDefaultSettings(const std::wstring& path) {
    if (FileExists(path)) return true;

    if (!WritePrivateProfileStringW(L"General", L"Name", L"NekoStack", path.c_str())) return false;
    if (!WritePrivateProfileStringW(L"General", L"Version", L"0.5", path.c_str())) return false;
    if (!WritePrivateProfileStringW(L"General", L"Theme", L"Dark", path.c_str())) return false;
    if (!WritePrivateProfileStringW(L"Desktop", L"Enabled", L"1", path.c_str())) return false;
    if (!WritePrivateProfileStringW(L"Metrics", L"RefreshMs", L"1000", path.c_str())) return false;

    WritePrivateProfileStringW(nullptr, nullptr, nullptr, path.c_str());
    return true;
}

bool CreateDefaultModules(const std::wstring& path) {
    if (FileExists(path)) return true;

    if (!WritePrivateProfileStringW(L"Modules", L"Desktop", L"1", path.c_str())) return false;
    if (!WritePrivateProfileStringW(L"Modules", L"Taskbar", L"0", path.c_str())) return false;
    if (!WritePrivateProfileStringW(L"Modules", L"Launcher", L"1", path.c_str())) return false;
    if (!WritePrivateProfileStringW(L"Modules", L"NekoSettings", L"1", path.c_str())) return false;
    if (!WritePrivateProfileStringW(L"Modules", L"Notifications", L"0", path.c_str())) return false;
    if (!WritePrivateProfileStringW(L"Modules", L"Files", L"0", path.c_str())) return false;
    if (!WritePrivateProfileStringW(L"Modules", L"System", L"0", path.c_str())) return false;

    WritePrivateProfileStringW(nullptr, nullptr, nullptr, path.c_str());
    return true;
}

} // namespace

namespace NekoCore {

bool Initialize(HINSTANCE instance) {
    if (g_initialized) return true;
    if (!instance) instance = GetModuleHandleW(nullptr);
    if (!instance) return false;

    std::vector<wchar_t> buffer(32768, L'\0');
    const DWORD length = GetModuleFileNameW(instance, buffer.data(),
                                             static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) return false;

    Paths paths;
    paths.executable.assign(buffer.data(), length);

    const std::wstring::size_type slash = paths.executable.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return false;

    if (slash == 2 && paths.executable.size() >= 3 && paths.executable[1] == L':') {
        paths.root = paths.executable.substr(0, 3);
    } else {
        paths.root = paths.executable.substr(0, slash);
    }

    paths.core = JoinPath(paths.root, L"Core");
    paths.modules = JoinPath(paths.root, L"Modules");
    paths.config = JoinPath(paths.root, L"Config");
    paths.resources = JoinPath(paths.root, L"Resources");
    paths.scripts = JoinPath(paths.root, L"Scripts");
    paths.builds = JoinPath(paths.root, L"Builds");
    paths.settingsFile = JoinPath(paths.config, L"Settings.ini");
    paths.modulesFile = JoinPath(paths.config, L"Modules.ini");

    if (!EnsureDirectory(paths.core)) return false;
    if (!EnsureDirectory(paths.config)) return false;
    if (!EnsureDirectory(paths.modules)) return false;
    if (!EnsureDirectory(paths.resources)) return false;
    if (!EnsureDirectory(JoinPath(paths.resources, L"Icons"))) return false;
    if (!EnsureDirectory(JoinPath(paths.resources, L"Themes"))) return false;
    if (!EnsureDirectory(paths.scripts)) return false;
    if (!EnsureDirectory(paths.builds)) return false;

    if (!CreateDefaultSettings(paths.settingsFile)) return false;
    if (!CreateDefaultModules(paths.modulesFile)) return false;

    g_paths = paths;
    g_initialized = true;
    return true;
}

bool IsInitialized() {
    return g_initialized;
}

const Paths& GetPaths() {
    return g_paths;
}

std::wstring ReadString(const wchar_t* section,
                        const wchar_t* key,
                        const wchar_t* defaultValue) {
    if (!g_initialized || !section || !key) return defaultValue ? defaultValue : L"";

    std::vector<wchar_t> buffer(4096, L'\0');
    GetPrivateProfileStringW(section, key,
                             defaultValue ? defaultValue : L"",
                             buffer.data(),
                             static_cast<DWORD>(buffer.size()),
                             g_paths.settingsFile.c_str());
    return std::wstring(buffer.data());
}

int ReadInt(const wchar_t* section, const wchar_t* key, int defaultValue) {
    if (!g_initialized || !section || !key) return defaultValue;
    return static_cast<int>(GetPrivateProfileIntW(section, key, defaultValue,
                                                    g_paths.settingsFile.c_str()));
}

bool WriteString(const wchar_t* section, const wchar_t* key, const wchar_t* value) {
    if (!g_initialized || !section || !key || !value) return false;
    return WritePrivateProfileStringW(section, key, value,
                                      g_paths.settingsFile.c_str()) != FALSE;
}

bool WriteInt(const wchar_t* section, const wchar_t* key, int value) {
    if (!g_initialized || !section || !key) return false;
    const std::wstring text = std::to_wstring(value);
    return WriteString(section, key, text.c_str());
}

bool IsModuleEnabled(const wchar_t* module, bool defaultValue) {
    if (!g_initialized || !module) return defaultValue;
    const int fallback = defaultValue ? 1 : 0;
    return GetPrivateProfileIntW(L"Modules", module, fallback,
                                 g_paths.modulesFile.c_str()) != 0;
}

} // namespace NekoCore
