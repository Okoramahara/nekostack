#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <string>

#include "Core/NekoCore.h"
#include "Modules/NekoDesktop.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    if (!NekoCore::Initialize(instance)) {
        MessageBoxW(nullptr,
                    L"No se pudo inicializar NekoStack. Comprueba que la carpeta tenga permisos de escritura.",
                    L"NekoStack", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (!NekoCore::IsModuleEnabled(L"Desktop", true) ||
        NekoCore::ReadInt(L"Desktop", L"Enabled", 1) == 0) {
        MessageBoxW(nullptr,
                    L"El modulo NekoDesktop esta desactivado en la configuracion.",
                    L"NekoStack", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    std::wstring appName = NekoCore::ReadString(L"General", L"Name", L"NekoStack");
    if (appName.empty()) appName = L"NekoStack";

    return NekoDesktop::Run(instance, showCommand, appName.c_str());
}
