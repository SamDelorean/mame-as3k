# Herramientas de ROM del AlphaSmart 3000

Dos aplicaciones independientes del emulador, con GUI GTK3 y CLI:

- `scripts/as3k-rom-builder`: constructor de una Flash nueva a partir de una selección de SmartApplets compiladas.
- `scripts/as3k-applet-test`: una applet, una ROM y una sesión de prueba desechables.

Ambas utilizan exclusivamente `backend.py`. Los perfiles `scripts/as3k-stable` y
`scripts/as3k-diag` añaden **Herramientas → Constructor de ROM… / Probar applet…**
a la interfaz GTK de AS2K reutilizada. Todos ejecutan el mismo `./alphasma3k`.
`scripts/as3k-install-menu` instala accesos en el menú de aplicaciones y
`~/.local/bin`; se niega a reemplazar archivos de otra instalación.

## Uso gráfico

Ejecute cualquiera de las dos aplicaciones sin argumentos. En el constructor:

1. Abra una Flash AS3000 System 3 / Writer de 1 MiB.
2. Abra la carpeta principal de applets o añada archivos `.OS3KApp`.
3. Marque las applets que desea incluir y ajuste el orden con Subir/Bajar.
4. Guarde opcionalmente el proyecto de selección.
5. Construya un archivo ROM nuevo; junto a él aparecen manifiesto y registro de operación.
6. Use **Abrir ROM generada** para ejecutarla.

La selección es el inventario final completo. Las applets ya presentes en la
ROM base aparecen seleccionadas inicialmente; el catálogo se añade sin
seleccionar. Para sustituir una versión, desmarque la anterior: dos imágenes con
el mismo `APPLET_ID` no pueden instalarse juntas.

En **Probar applet**, seleccione base y una applet, e inicie la prueba. Al cerrar
el emulador, el botón de cerrar prueba o la aplicación, se elimina toda su
sesión. Desde el menú AS3K, la sesión normal se cierra ordenadamente mientras
la ventana de pruebas está abierta y se vuelve a abrir al salir de ella.
Su selección original y su persistencia no se cambian.

La pantalla proviene de capturas PNG del LCD nativo de MAME; los botones y el
teclado actúan sobre las entradas físicas. En una memoria nueva, responda al
aviso de inicialización de datos del firmware con **Y**, espere y pulse Return.
Las sesiones nuevas seleccionan el modo existente de cable desconectado; el
modo histórico de D11 alimentado para diagnósticos no es el valor de estas aplicaciones.

Los menús Archivo, Máquina, Ver, Configuración y Ayuda de AS2K se conservan.
La DictROM externa y los payloads binarios de AS2K no son periféricos/programas
portables al AS3000 y aparecen deshabilitados con explicación. IrDA sigue OPEN
y no se presenta como funcional. Los scripts existentes de bootstrap AS3K,
impresión CUPS y pruebas R10 permanecen en el mismo árbol.

## CLI

Desde la raíz canónica:

```sh
scripts/as3k-rom-builder --base /ruta/base.bin --inspect

scripts/as3k-rom-builder \
  --base /ruta/base.bin \
  --applet /ruta/AlphaWordPlus.OS3KApp \
  --applet /ruta/Calculator.OS3KApp \
  --output /ruta/nueva.rom \
  --save-project /ruta/seleccion.as3k.json \
  --report /ruta/construccion.json

scripts/as3k-rom-builder --project /ruta/seleccion.as3k.json \
  --output /ruta/otra.rom --open

scripts/as3k-applet-test --base /ruta/base.bin \
  --applet /ruta/prueba.OS3KApp --report /ruta/prueba.json

scripts/as3k-stable --rom /ruta/nueva.rom
scripts/as3k-diag --rom /ruta/nueva.rom
```

`--retain-base` conserva primero las applets incluidas en la base al construir
por CLI. Sin esa opción, la lista `--applet` es la selección completa.
`--project` verifica las huellas guardadas y rechaza entradas cambiadas.
Las salidas y manifiestos existentes nunca se sobrescriben.

Para ensayos: `--headless --seconds 15 --observer /ruta/observacion.lua`.
`--rompath`, o la variable `AS3K_ROMPATH`, permite señalar los conjuntos
originales requeridos por MAME. No se distribuyen ROMs ni applets propietarias.
La entrada nativa del mismo emulador también puede usarse directamente:

```sh
./alphasma3k asma3ksw -rom /ruta/nueva.rom -rompath /ruta/conjuntos \
  -cfg_directory /ruta/config-exclusiva -nvram_directory /ruta/nvram-exclusiva
```

La CLI y la GUI crean manifiestos deterministas (sin fecha dentro del contenido
reproducible). Los registros de operación separados incluyen fecha UTC, origen
y opciones. El informe temporal registra identificador de sesión, huellas,
salida del proceso, archivos de estado por nombre/tamaño/huella y limpieza.
Un cierre por SIGKILL o un corte de energía no permite ejecutar limpieza: no se
reutilizan esas carpetas en la siguiente prueba. Cierre normal y SIGTERM sí se
han probado.

## Formato e integridad

El constructor reconoce el mapa Flash AS3000 System 3: área de applets
`0x008000..0x0BFFFF`, System desde `0x0C0000`, 31 slots además de System.
Resuelve la cabecera System por el puntero de su cabecera nativa. No busca la
firma a ciegas, porque también aparece dentro del código de System.

Valida firma C0FFEEAD, longitud, cabecera 1, nombre/ID, estado FF, entrada
relativa, extensión 0/1/2, límites de configuración, marcador final CAFEFEED,
IDs, Flash y SRAM disponibles y buffers de archivos. Rechaza arena desconocida
y requisito inalámbrico. Esta validación de contenedor no demuestra que todo
código compilado para otra máquina OS3K sea portable: una applet para AS3000
debe usar su hardware/ABI. La versión mínima ASM pertenece al Manager anfitrión,
no se interpreta como versión del firmware.

Las imágenes PIC se copian byte por byte, contiguas; no hay compilación,
reubicación ni parche del sistema. System 3 construye su tabla de slots al
arrancar. Los sectores de arranque y System de la base se preservan exactamente.
La entrada MAME `-rom` es un socket de imagen de solo lectura que inicializa
la región existente de Flash antes de la carga normal de NVRAM. No crea otra Flash.

La ejecución permanente separa estados por SHA-256 de la ROM y máquina, con
bloqueo para evitar escrituras simultáneas. Cada prueba temporal usa un
`TemporaryDirectory` nuevo, directorios nuevos de cfg/NVRAM/Flash/states/diff/input,
sin lectura de configuraciones generales ni autosave ni transportes heredados.
No se copia SRAM previa para iniciar pruebas temporales.

## Verificación reproducible

```sh
python3 -m unittest tools.as3k_rom_tools.test_backend -v
python3 work/AS3K_ROM_TOOLS_20261113/temporary_acceptance.py
GDK_BACKEND=broadway BROADWAY_DISPLAY=:9 \
  python3 work/AS3K_ROM_TOOLS_20261113/gui_acceptance_final.py
python3 work/AS3K_ROM_TOOLS_20261113/regressions.py
python3 work/AS3K_ROM_TOOLS_20261113/usb_r10.py
```

Los ensayos de aceptación conservan evidencia y crean salidas exclusivas:
para repetirlos, use una nueva carpeta de campaña y ajuste sus rutas, sin
borrar evidencias anteriores. Broadway se usó únicamente como display GTK
local de pruebas; el uso habitual requiere una sesión gráfica GTK3.

Informe y matriz: `docs/as3k/ROM_TOOLS_20261113.md`.
