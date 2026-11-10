#ifndef NEKO_SETTINGS_H
#define NEKO_SETTINGS_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace NekoSettings {

// Abre el editor modal de Applications.ini. Requiere NekoLauncher inicializado.
bool OpenApplications(HWND owner);

} // namespace NekoSettings

#endif // NEKO_SETTINGS_H
