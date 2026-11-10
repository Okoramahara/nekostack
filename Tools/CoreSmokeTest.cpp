#include "../Core/NekoCore.h"
#include <iostream>

int main() {
    if (!NekoCore::Initialize()) {
        std::wcerr << L"NekoCore no pudo inicializarse. Codigo Win32: "
                   << GetLastError() << L"\n";
        return 1;
    }

    const NekoCore::Paths& paths = NekoCore::GetPaths();
    std::wcout << L"NekoCore inicializado correctamente.\n";
    std::wcout << L"Raiz: " << paths.root << L"\n";
    std::wcout << L"Configuracion: " << paths.settingsFile << L"\n";
    std::wcout << L"Nombre: " << NekoCore::ReadString(L"General", L"Name", L"NekoStack") << L"\n";
    std::wcout << L"Tema: " << NekoCore::ReadString(L"General", L"Theme", L"Dark") << L"\n";
    std::wcout << L"Refresco: " << NekoCore::ReadInt(L"Metrics", L"RefreshMs", 1000) << L" ms\n";
    return 0;
}
