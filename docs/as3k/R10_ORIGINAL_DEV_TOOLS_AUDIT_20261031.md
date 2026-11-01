# R10 — Aprovechamiento del depurador y utilidades originales (auditoría de fuentes)

Fecha: 2026-10-31. Estado: **AUDITORÍA ESTÁTICA CONCLUIDA; R10 DE COMUNICACIONES ABIERTO**.
Ámbito único: mame-as3k-dev / AS3K-DIAG / MAME 0.289 reducido. No se crea ejecutable, rama o emulador auxiliar. No se incorporan archivos propietarios de los fabricantes en Git.

## Procedencia verificada
Fuente de desarrollo original en el Drive montado:
`/home/spc/GoogleDrive/ALPHASMART/Soruce code/Software/ModuleSources/`.
Los ficheros fueron inspeccionados desde la T160. Procedencia de firmware, no una API pública moderna. El simulador original es Win32; no sustituye a MAME.

## 1. DebugModule.c/.h — monitor, trazas y comprobación de memoria
- `DEBUG_BBUG_ON` activa `Debug_SendBbugMessage()`: espera el bit de ocupación UART 0x04 en 0xFFFFF906; escribe caracteres en 0xFFFFF907 y termina el mensaje con un byte NUL. Esta semántica se puede usar como **vector exacto TX/status** para el nuevo UART EZ y un backend serie/archivo del anfitrión. No usar solo una escritura: demostrar bytes, ocupación y el terminador.
- `Debug_GoToBootstrapMonitor()` invoca la dirección 0xFFFFFF44, bajo opción de compilación; es una **referencia para seguimiento de arranque/monitor**, no algo que debamos invocar a ciegas ni un controlador USB.
- `DEBUG_SIMPLE_TRACE_ON` habilita una traza circular de 100 KiB y `Debug_FindSimpleTrace`; se puede correlacionar con MAME `trace`, `logerror`, watchpoints, registros y RAM, sin insertar este búfer en la ROM del usuario.
- `DEBUG_BREAK_ON` ofrece aserciones `Debug_DoBreakIfTrue(__FILE__,__LINE__)` y una pantalla de fallo; útil para localizar rutinas y cadenas compiladas en firmware de desarrollo. El código fuente aclara que el depurador externo SDS/SingleStep es **otra herramienta**: no se encontró su implementación en `DebugModule.c`.
- `DEBUG_STACK_CHECKER_ON` define el patrón 0xAA, intervalo 0x3A700–0x3FF00, tope real 0x3FFF0 y comprobación de 16 bytes; es una **referencia de auditoría** de crecimiento de stack para ejecuciones reales del firmware. No sobrescribir esa RAM en el firmware en uso.

**Resultado estático**: estructura e interfaces originales identificadas. **NO PASS** de ejecución del BBUG original en la ROM actual: esos módulos están condicionados por flags y no se demostró que estén habilitados.

## 2. As_sim.c / As_scrn.c / WinSim — simulador y utilidades de escritorio
- `As_sim.c` es una interfaz de simulación **Win32** que incluye `windows.h`, VxD de teclado y un mapa de 92 posiciones `ubaPCScanToIcode`. Conviene utilizar esa tabla como segundo oráculo independiente para comparar las 77 asignaciones de teclado de R6, además de `KeyboardModule.c`; discrepancias se resolverán con fuente y trazas del firmware.
- `As_scrn.c` y `LCDModule.c` sirven para contrastar caracteres, cursor, selección de filas y formato de pantalla contra el LCD doble KS0066 ya validado; evitar portar sus ventanas Win32.
- `WinSim.dsp`, `WinSim.mak`, `WinSim.exe` y carpetas Debug/Release documentan construcción original MS Visual C++/Win32, **no** una versión más fiel del emulador de CPU. No crear segunda aplicación ni ejecutar binarios antiguos por defecto.
- `AS3000InitCode.s`, `SystemAssembly.s`, `InterruptModule.c`, `SystemModule.c` permiten relacionar vectores, chip selects, detectores de conexión USB/PS2/ADB, reposo y arranque con MAME.

## 3. USB — comandos auténticos y camino físico, no simulación de resultados
- `UsbAsmModule.s`: `USB_SendByte` y `USB_GetByte` manejan **Port E PE1/PE2**, dirección, reloj y liberación de línea; se debe observar esa señalización en la CPU original.
- `UsbChip.c/.h` identifica comandos del puente **D11 y su controlador D12**: prefijos D11 command-read 0x37, command-write 0x36, data-read 0x35, data-write 0x34; comandos D12 set-address 0xD1, initialize 0xD0, interrupt-register 0xF4, read/write-buffer 0xF0, validate-buffer 0xFA y acknowledge-setup 0xF1. Estos valores son vectores para pruebas de máquina de estados, NO sustitutos de las transferencias reales.
- `UsbModule.c`, `UsbIsr.c`, `UsbMain.c`, `UsbRequestResponse.c`: rutas de configuración, setup, endpoints e interrupciones. Comparar el tráfico eléctrico PE y los estados internos con sus decisiones originales.
- `UsbToAS.c` y `UsbGet.h`: protocolo Get Utility/AlphaAdmin con comando GET_FILE_SIZE 0x01, SWITCH_FILE 0x05, SEND_FILE_TO_AS 0x06, GET_FILE_FROM_AS 0x07, FILE_BUFFER 0x08, GET_VERSION 0x18. `USB_Ep1RxDoneITimeProc` consume esos comandos, responde con máscara 0x40 y error con máscara 0x80. Permite elaborar comprobaciones **entrada/resultado del firmware** y exportación real de archivos R8, sin traducirlos artificialmente.
- **No confundir Get Utility con actualización Flash**: la ruta del actualizador está en `UpdaterCommands.h` y `UpdaterInfo.h`.

## 4. Actualizador original — prueba pendiente de R9 mediante transporte real
- `UpdaterCommands.h`: el protocolo para USB o RS232 admite bloques de hasta 16 KiB. Comandos: versión 1; estado 20; envío a memoria 30; descompresión 40; programación Flash 50; borrado 60; ejecución 70; reinicio 80; envío/descompresión/programación 85; lectura 90; nombre Flash 100 y tamaño 110. Los bloques llevan direcciones/tamaños de 32 bits big-endian.
- El protocolo distingue latencia/respuestas: por RS232 se reciben tras completar la operación; por USB el anfitrión debe consultar repetidamente para operaciones largas. **No inventar** transferencia directa a Flash equivalente al script de R9.
- `UpdaterInfo.h`: Machine ID 0x01 para original con AMD/ST 512 KiB/1 MiB; 0x02 para Intel/Micron 1 MiB. El modelo y protocolo deben conservar esta distinción, independientemente de la ROM usada.
- `RS232Framer.h`: tramas de actualización con BOF 0xC0, EOF 0xC1, checksums y escape. Reutilizar exactamente su algoritmo; no redefinir un protocolo de paquetes.
- Resultado R9 sigue válido: programación del original AlphaWordPlus por bus Flash. **NO equivale** a una instalación mediante AlphaSmart Manager ni prueba de updater nativo.

## 5. UART, IrDA, impresión y periféricos
- `UartModule.c`: UART MC68EZ328 en PE4 RXD, PE5 TXD, PE6 RTS, PE7 CTS; estados/fifos, baud rates 4800/9600/19200/38400/57600/115200 y control de flujo. `DebugModule.c` aporta un segundo testigo independiente de registros TX/status.
- `Irda68328EZ.c`, `IrdaFram.c`, `IrdaLap.c`, `IrdaLmp.c`: reutilizar contratos y rutinas del protocolo sobre el UART; enlazar al backend host ya probado para AS2K solo donde coincidan los datos, nunca sustituir la electrónica AS3K.
- `PrintModule.c`, `PrintLookupTables.c`, `PrintStyleWriter.c`, `ADBModule.c`, `PS2Module.c`: referentes de envío, impresión, teclado de anfitrión y selección de protocolo; la salida PDF/archivo podrá aprovechar el backend AS2K conservando el formato transmitido por firmware.

## Regla para las pruebas de R10, sin fragmentación
1. Auditar pines/registros y rutinas exactas en estas fuentes antes de cambiar core o driver. MAME breakpoints y watchpoints contra las ROM originales para localizar el camino ejecutado.
2. UART real: acreditar bit de estado, salida byte-a-byte y recepción/IRQ, usando BBUG o `UartModule.c` como vectores; no contar estática como dinámica.
3. USB real: PE1/PE2 -> D11 -> D12 -> setup/endpoints -> Get Utility/actualizador -> memoria/Flash de R8/R9; cada salto con prueba del firmware.
4. RS232 / IrDA / Print: reutilizar protocolos de fuentes originales y backends compatibles de AS2K, sin reinterpretar datos.
5. Compilación **minimalista del MISMO** `alphasma3k`, auditorías y regresiones R3–R9; ninguna ROM específica fija en el código.

**Acreditado con esta auditoría**: existencia, estructura y semántica estática de los módulos enumerados. **No acreditado aún**: funcionamiento real USB, UART, BBUG, IrDA, RS232, Print o instalación desde AlphaSmart Manager. Mantener R10 ABIERTO. Esta auditoría NO es un incremento nuevo ni autoriza emulador paralelo.
