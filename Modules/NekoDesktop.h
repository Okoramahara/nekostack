#ifndef NEKO_DESKTOP_H
#define NEKO_DESKTOP_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace NekoDesktop {

// Ejecuta la interfaz Neko Desktop como modulo grafico de NekoStack.
int Run(HINSTANCE instance, int showCommand, const wchar_t* windowTitle);

} // namespace NekoDesktop

#endif // NEKO_DESKTOP_H
