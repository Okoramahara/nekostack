#ifndef NEKO_BAR_H
#define NEKO_BAR_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string>

namespace NekoBar {
bool Initialize(HINSTANCE instance);
void PollCursor();
void NotifyOpened(const std::wstring& title, const std::wstring& command,
                  const std::wstring& arguments = L"");
void Shutdown();
}

#endif
