NEKOSTACK 0.5.2 - NEKOSETTINGS: SELECTOR DE PROGRAMAS
============================

Esta version agrega NekoSettings, un pequeno editor grafico para administrar los
accesos de NekoLauncher sin abrir Applications.ini ni recompilar NekoStack.
Parte de la version 0.4, cuya funcionalidad de NekoLauncher fue probada por el
usuario (cargar, renombrar, ocultar, restaurar y agregar accesos desde INI).

CORRECCIONES DE COMPILACION
---------------------------
- Se cualifico Application como NekoLauncher::Application en el cargador INI.
- Las macros UNICODE y _UNICODE ahora se definen solo si aun no existen,
  evitando las advertencias de redefinicion del compilador.

NOVEDADES
---------
- Nuevo modulo Modules\NekoSettings.cpp / NekoSettings.h.
- En la pagina Aplicaciones aparece el boton ADMINISTRAR.
- El editor permite cambiar nombre, descripcion, distintivo, comando, argumentos,
  color y estado activo de cada acceso.
- Permite agregar y eliminar entradas.
- Conserva tambien las entradas desactivadas, para poder reactivarlas despues.
- Limite de 9 entradas, igual que NekoLauncher 0.4.
- Guarda los cambios directamente en Config\Applications.ini.
- Al guardar, NekoLauncher actualiza su lista en memoria; no hace falta cerrar
  NekoStack para que los cambios se reflejen al volver a la pagina Aplicaciones.
- Se mantiene la compilacion estatica y no se agregan DLL externas de MinGW.
- Se actualiza la identificacion de la interfaz a NekoStack 0.5.

ACTUALIZACION
-------------
1. Cierra NekoStack.
2. Extrae el ZIP en la raiz existente, por ejemplo:
   C:\Users\Public\NekoStack
3. Reemplaza los archivos cuando Windows lo solicite. No elimines ni reemplaces
   tu carpeta Config: se conservan Settings.ini, Modules.ini y Applications.ini.
4. Abre Config\Modules.ini y verifica que exista:

   [Modules]
   Launcher=1
   NekoSettings=1

   Si no existe NekoSettings, esta version lo considera activo por defecto.
   Si aparece NekoSettings=0, cambia el valor a 1.
5. Cierra cualquier NekoStack.exe que este abierto.
6. Ejecuta Build_NekoStack.bat.
7. Al terminar, abre NekoStack.exe y entra en Aplicaciones.
8. Pulsa ADMINISTRAR.

RESPALDOS
---------
Antes de reemplazar el ejecutable, el BAT conserva:
- Builds\NekoStack_0.2_Baseline.exe (si aun no existe).
- Builds\NekoStack_0.4_Launcher.exe (si aun no existe).
- Builds\NekoStack_0.5.1_Settings.exe (si aun no existe).

No sobrescribe los respaldos existentes. Si la compilacion falla, el ejecutable
anterior se conserva porque el BAT solo copia el temporal al nombre final despues
de una compilacion correcta.

USAR NEKOSETTINGS
-----------------
- La lista de la izquierda muestra todas las entradas, incluidas las ocultas.
- Selecciona una entrada para editar sus detalles.
- Activa o desactiva la casilla para mostrar u ocultar el acceso en NekoLauncher.
- Agregar crea una nueva entrada; completa Nombre y Comando antes de guardarla.
- Eliminar quita la entrada seleccionada, previa confirmacion.
- Guardar cambios escribe Applications.ini y refresca NekoLauncher.
- Puedes tener hasta 9 entradas entre activas y ocultas.

CAMPOS
------
- Nombre: texto de la tarjeta.
- Descripcion: subtitulo de la tarjeta.
- Distintivo: texto corto mostrado en el icono (EX, PS, APP, etc.).
- Color: Blue, Purple, Green u Orange.
- Programa / ejecutable: pulsa Buscar... y selecciona el .exe o .lnk del programa.
  Tambien puedes escribir manualmente comandos conocidos de Windows como explorer.exe.
- Argumentos: parametros opcionales; normalmente se dejan vacios. Para la raiz de
  NekoStack usa %NEKOSTACK%.
- Aplicacion activa: si esta marcada, aparece en NekoLauncher.

LIMITACIONES DE ESTA PRIMERA VERSION
------------------------------------
- El editor admite hasta 9 aplicaciones porque ese es el limite visual actual de
  NekoLauncher (cuadricula de 3 columnas por 3 filas).
- Los cambios se guardan al pulsar Guardar cambios. Cerrar sin guardar descarta
  las modificaciones pendientes en esa sesion del editor.
- El campo Programa / ejecutable incluye un boton Buscar... para seleccionar
  archivos .exe o accesos directos .lnk desde una ventana de Windows.
- Al elegir un archivo, su ruta completa se coloca automaticamente en Command.
  Si el nombre aun es "Nueva aplicacion", se completa con el nombre del archivo.
- Los argumentos quedan como opcion avanzada y normalmente pueden dejarse vacios.
- NekoSettings administra Applications.ini. La configuracion general, los temas,
  los fondos y la distribucion del escritorio se trabajaran en etapas posteriores.

COMPILACION MANUAL DESDE POWERSHELL
-----------------------------------
Abre PowerShell en la raiz del proyecto y ejecuta en una sola linea:

& "C:\WinLibs\mingw64\bin\g++.exe" -std=c++17 -O2 -Wall -Wextra -static -static-libgcc -static-libstdc++ -mwindows -municode -I. NekoStack.cpp Core\NekoCore.cpp Modules\NekoDesktop.cpp Modules\NekoLauncher.cpp Modules\NekoSettings.cpp -o NekoStack.exe -luser32 -lgdi32 -lshell32 -lcomdlg32

RECUPERACION
------------
Si NekoStack 0.5 no inicia, cierra el programa y puedes restaurar:
Builds\NekoStack_0.4_Launcher.exe -> NekoStack.exe

El proyecto sigue ejecutandose como una aplicacion Win32 normal. No reemplaza
Explorer, no modifica Winlogon ni requiere cambiar el Registro de Windows.

FLUJO SENCILLO PARA AGREGAR UNA APLICACION
------------------------------------------
1. Pulsa Agregar y selecciona la nueva entrada en la lista.
2. Escribe el Nombre y, si quieres, una Descripcion.
3. Pulsa Buscar... junto a Programa / ejecutable.
4. Busca el archivo .exe del programa o su acceso directo .lnk y pulsa Abrir.
5. Deja Argumentos vacio salvo que el programa necesite parametros especiales.
6. Pulsa Guardar cambios.

NOTA SOBRE PROGRAMAS QUE NO ABREN
---------------------------------
El nombre visible (por ejemplo, "MediBang") no es necesariamente el comando
ejecutable. Usa Buscar... para elegir el archivo real del programa. Esto guarda
la ruta completa y evita depender de que Windows encuentre el nombre en PATH.
