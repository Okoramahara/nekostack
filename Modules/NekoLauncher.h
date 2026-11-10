#ifndef NEKO_LAUNCHER_H
#define NEKO_LAUNCHER_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstddef>
#include <string>
#include <vector>

namespace NekoLauncher {

struct Application {
    std::wstring name;
    std::wstring description;
    std::wstring glyph;
    std::wstring command;
    std::wstring arguments;
    std::wstring accent;
    bool enabled = true;
};

// Carga Config\\Applications.ini. Si no existe, crea una lista inicial de ejemplo.
bool Initialize(const std::wstring& configDirectory);
void Shutdown();
const std::vector<Application>& GetApplications();

// Devuelve todas las entradas, incluidas las desactivadas, para administrarlas.
const std::vector<Application>& GetAllApplications();

// Guarda la lista completa (maximo 9) y actualiza la lista visible en memoria.
bool SaveApplications(const std::vector<Application>& applications);

bool IsInitialized();

// Abre una aplicacion de la lista cargada. El indice corresponde a GetApplications().
bool Launch(std::size_t index, HWND owner);

} // namespace NekoLauncher

#endif // NEKO_LAUNCHER_H
