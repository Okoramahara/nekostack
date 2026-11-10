#ifndef NEKO_CORE_H
#define NEKO_CORE_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>

namespace NekoCore {

struct Paths {
    std::wstring executable;
    std::wstring root;
    std::wstring core;
    std::wstring modules;
    std::wstring config;
    std::wstring resources;
    std::wstring scripts;
    std::wstring builds;
    std::wstring settingsFile;
    std::wstring modulesFile;
};

// Prepara las rutas de NekoStack a partir de la ubicacion del ejecutable.
// No cambia el directorio de trabajo ni modifica el registro de Windows.
bool Initialize(HINSTANCE instance = nullptr);

bool IsInitialized();
const Paths& GetPaths();

std::wstring ReadString(const wchar_t* section,
                        const wchar_t* key,
                        const wchar_t* defaultValue = L"");

int ReadInt(const wchar_t* section,
            const wchar_t* key,
            int defaultValue = 0);

bool WriteString(const wchar_t* section,
                 const wchar_t* key,
                 const wchar_t* value);

bool WriteInt(const wchar_t* section,
              const wchar_t* key,
              int value);

// Lee Modules.ini. Un valor distinto de cero significa que el modulo esta activo.
bool IsModuleEnabled(const wchar_t* module,
                     bool defaultValue = true);

} // namespace NekoCore

#endif // NEKO_CORE_H
