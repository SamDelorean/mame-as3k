## Correctivo integrado: payload-0 con BOOT OK en LCD — CLOSED / NATIVE PASS

Referencia: AS3K_CODEX_CORRECTIVO_PAYLOAD0_LCD_20261112.md. Campaña registrada
en work/AS3K_PAYLOAD0_LCD_20261113 (fecha UTC; 12 de noviembre en Matamoros).
La aceptación anterior de 138 bytes acreditaba UART; faltaba la función visual.
El MISMO scripts/as3k_payload0.s ahora inicializa el LCD existente, escribe
BOOT OK y conserva íntegro su contrato UART. No se crea otra variante.

### Resultado observado

- LCD_BOOT_OK_PASS: PNG nativo de MAME 240×36, LCD 40×4, BOOT OK legible
  en la primera fila. Revisado visualmente. BOOT, WRITER y ciclo normal ->
  bootstrap visual -> normal generan imágenes idénticas píxel por píxel.
- Captura: SHA-256 8213d41a2a71d49c441506a03be129243f9c089bde9aea4eb2d7f9a768c29597.
- Payload: 338 bytes, rango 00030000–00030151; SHA-256
  ebc5bc5652c14cdf9f236c3bf05be04daa7f9aa9230b98c62da4105bd45cd0d3.
- Checksum calculado por instrucciones del payload: 00006B14, contrastado con
  la suma de su nueva imagen. No se reutiliza el oráculo histórico 0000323F.
- Identidad UART intacta: AS3K:P0:v1:BOOT OK CR LF.
  Desafío y eco: 00 A5 FF 5A. Confirmación: 4F 4B 0D 0A.
- B-records segmentados de 16 bytes, COUNT FF de 255 bytes e IBUFF D7=55
  pasan. La imagen observada en RAM coincide con los 338 bytes ensamblados.
  JMP FFFFFF44 permite otra carga REENTER_OK.
- BUILD PASS: make reducido PRECOMPILE=0 REGENIE=1 -j2, exit 0.
  validate exit 0. El ejecutable conserva SHA-256
  f8d8e11ea7bf6d3ef3fd6a9ff1a42a8774781bc896575ca0dcdcd7b53b1218e4.
- Las 16 regresiones R3–R10 pasan nuevamente: cuatro auditorías, D11,
  UART FIFO/PTY, teclado, BOOT/WRITER, Flash y SRAM. R8/R9 mantienen LCD
  idéntico píxel por píxel a las referencias congeladas.
- USB R10 completo: MAME y anfitrión exit 0, descriptores auténticos,
  SET_ADDRESS/SET_CONFIGURATION y los 47 bytes de Get idénticos TX/RX.

### Reutilización y prueba del origen de la imagen

Se adaptaron LCD_BYTE, LCD_NIBBLE, DELAY_LONG y MESSAGE del donante local
ASPRO-emu/src/payload0_pro.asm (SHA-256
311ea89900b07ce7af2ba4ff21448bd8dcaf3cea9ccc9e9ac3284af05856f444).
Se trasladó el algoritmo de inicialización 3,3,3,2 y escritura de nibbles;
no se trasladaron registros, pines ni instrucciones HC11.

El programa 68000 configura PCSEL FFFFF413=FF y PCDIR FFFFF410=FF.
PCDATA FFFFF411 entrega DB4–DB7 por PC0–3, RW por PC4, RS por PC5,
E1 por PC6 y E2 por PC7. Reutiliza lcd_port_c_commit, los dos KS0066 y
screen_update ya existentes. No hay cambios de driver, core ni interfaz.

La captura procede de machine.screens[":screen"]:snapshot. Lua solamente
observa MMIO/memoria, captura el framebuffer y selecciona el strap/reset;
no escribe DDRAM, texto, GPIO ni PC/RAM para fabricar el resultado.

Traza CPU independiente: 94 escrituras a los tres registros, 30 flancos de
bajada E1 y 14 E2. Ambos controladores reciben nibbles 3,3,3,2 y comandos
28,08,01,06,0C; E1 recibe además 80 y datos 42 4F 4F 54 20 4F 4B.
Su decodificación da literalmente BOOT OK. Se conservan los tiempos
emulados, dirección MMIO, valor y PC real de cada instrucción.
Los retardos DBRA dan unos 20,5 ms en inicialización/borrado y unos
160 us entre nibbles con el reloj MAME de 16 MHz.

### Reset y protección de datos

Ciclo final en un único proceso: EMUBRK alto/normal -> bajo/bootstrap ->
alto/normal; estados (1,0),(0,1),(1,0). El mismo host existente carga
y ejecuta el mismo payload y comprueba checksum/eco/reentrada dentro del ciclo.

La prueba copia el fixture R8 a una sesión privada. Tras capturar BOOT OK,
restaura por B-records UART las regiones de ensayo de código, scratch,
marcador y stack a sus bytes anteriores; una lectura CPU independiente
comprueba íntegra la restauración antes del reset normal. No se modifica la
NVRAM original del propietario. Stack descendente desde 0003F000, mínimo
0003EFF4 (12 bytes); sin solapar código ni buffers. Las regiones de código
y stack elegidas estaban a cero en el fixture antes de la prueba.

System 3/AlphaWord original vuelve al editor y conserva r8proof; Flash queda
idéntica. Capturas normal antes/después revisadas: el texto coincide y sólo
cambian 35 píxeles del cursor parpadeante (x=42..46, y=0..6). No se afirma
identidad total de estas dos capturas. Las regresiones R8/R9 independientes
sí conservan identidad total con sus referencias.

En las sesiones bootstrap limpias BOOT/WRITER, la primera página SRAM queda
intacta y no hay escrituras fuera de los rangos de ensayo y los 12 bytes de
stack. Ese chequeo de SRAM inicialmente cero no aplica al ciclo con System 3.

### Instrumentación y conservación

El autorrecargado del autoboot tras soft reset dejaba observadores UART
duplicados hasta su recolección. Se observaron 355 filas TX y 356 RX repetidas
con igual tiempo/valor/máscara. Se retiran explícitamente los tres taps Lua
antes de reinstalarlos y se repite el ciclo completo. La traza final coincide
íntegramente con el PTY, sin deduplicar ni corregir bytes para lograr PASS.
No se modificó la UART ni el bootstrap para resolver ese fallo de observación.

Cambios funcionales: scripts/as3k_payload0.s.
Pruebas reutilizadas/adaptadas: scripts/as3k_bootstrap_test.py y
scripts/as3k_bootstrap_reset_test.py; este último invoca el mismo host UART.
scripts/as3k_bootstrap_build.py conserva su hash y calcula tamaño/suma actuales.
Todo mc68328.cpp/.h, alphasma3k.cpp, KS0066, USB/IP, ADB congelado, Flash,
ROM y modelo bootstrap permanecen intactos, comprobados por hashes.

Reproducción con ROMPATH privado y directorios nuevos:
  python3 scripts/as3k_bootstrap_build.py --output-directory work/ensayo/assembled
  python3 scripts/as3k_bootstrap_test.py --rompath "$ROMPATH" --payload work/ensayo/assembled/payload0.bin --output-directory work/ensayo/writer
  python3 scripts/as3k_bootstrap_test.py --system asma3ksb --rompath "$ROMPATH" --payload work/ensayo/assembled/payload0.bin --output-directory work/ensayo/boot
  python3 scripts/as3k_bootstrap_reset_test.py --fixture "$AS3K_FIXTURE_R8" --rompath "$ROMPATH" --payload work/ensayo/assembled/payload0.bin --output-directory work/ensayo/ciclo

GIT_SYNC_BLOCKED: rama as3k-mame0289-dev, HEAD 3ba23b4713f,
ahead 12 / behind 2 (refs locales observadas). Sin commit, push ni
reconciliación destructiva. Se conservan los cambios locales anteriores.
R11 ADB sigue congelado/OPEN; este correctivo no declara cerrar R11.
Excel retirado del alcance por el propietario; no se modifica ningún libro.

El cargador sigue siendo el modelo documentado ya acreditado, no una máscara
Motorola original; hardware físico NOT TESTED. La aceptación aquí es nativa
dentro del mismo AS3K-DIAG. Fuentes, parches, binarios propios y trazas privadas
permanecen en T160; la entrega Drive contiene sólo evidencias publicables.

---

# AS3K-DIAG — bootstrap MC68EZ328 y payload-0 — 2026-11-12

**Resultado funcional: NATIVE PASS, en el único `./alphasma3k`.** El bootstrap
usa un **modelo de servicios ejecutable propio**, permitido por el paquete;
no es una ROM enmascarada original de Motorola ni una verificación física.
Git: **GIT_SYNC_BLOCKED**, rama existente ahead 12 / behind 2; sin commit,
push, pull, reset, clean, stash, rebase, clon, rama ni emulador adicional.

Binario final: `f8d8e11ea7bf6d3ef3fd6a9ff1a42a8774781bc896575ca0dcdcd7b53b1218e4`.
Árbol: `/home/spc/Projects/alphasmart/mame-as3k-dev`, rama `as3k-mame0289-dev`.
HEAD conservado: `3ba23b4713f9a1eceb7135bdf9622bdadb43b599`.
Evidencia local: `work/AS3K_BOOTSTRAP_INTEGRAL_20261112/`.

Entrada verificada: ZIP de 13.901 bytes, 10 entradas; nueve referencias internas SHA-256 correctas. ZIP SHA-256: `4d712e0e31f290bdf1eabe0151db7ee1e4a03a352d0482b00d58171cc1f04491`.

## Aceptación comprobada

| Gate | Resultado | Evidencia |
| --- | --- | --- |
| B0 | BUILD PASS | PRECOMPILE=0 REGENIE=1 -j2; mismo ejecutable; no limpieza global |
| B1 | PASS | -validate exit 0, cuatro auditorías ROM exit 0 |
| B2 | NATIVE PASS | System 3/AlphaWord original antes y después de bootstrap; vectores normales SSP=0003FFF0, PC=0040042A |
| B3 | NATIVE PASS | CPU leyó 00000000=FFFC, 00000002=FFFC, 00000004=FFFF, 00000006=FF00; máscara FFFF; fetch real FFFFFF00 |
| B4 | NATIVE PASS (modelo) | PTY independiente, UART 8N1, enlace B -> @, eco byte por byte |
| B5 | NATIVE PASS | COUNT=FF: 255 bytes exactos en 00032000; fuente recibida por UART; sin inyección host de RAM |
| B6 | NATIVE PASS | 138 bytes en 00030000, entrada CPU real, identidad, checksum y eco binario |
| B7 | NATIVE PASS | 8 bytes en FFFFFFAA; instrucción MOVEQ #55,D7 ejecutada; retorno interno al loader; D7=00000055 |
| B8 | NATIVE PASS | 16/16 R3–R10, USB original 47 bytes bidireccionales; LCD R8/R9 píxel idéntico |
| B9 | Documentación local actualizada; entrega registrada aparte | Excel retirado del alcance por aclaración expresa del propietario; Git sincronización bloqueada |

Reset en un mismo proceso, cambiando la configuración física: normal alto ->
bootstrap bajo -> normal alto. Modos reales registrados: `(1,0), (0,1), (1,0)`.
Flash idéntica y documento original `r8proof` conservado. La primera página de
SRAM no se copia del IPL bajo bootstrap; prueba limpia: 256 bytes intactos.
En BOOT se verificaron además todas las escrituras SRAM: únicamente payload,
255 bytes de ensayo, `REENTER_OK` y cuatro bytes de stack; cero cambios fuera
de esas regiones. Flash de ensayo idéntica al original seleccionado.

## Bytes y procedencia

Payload propio ensamblado para 68000 con `m68k-elf-as`, 138 bytes:
SHA-256 `7ad4e0935e98e6b24df2d35c694a2ab5d505850d3254c9ce2c944f7459cf2fa2`.
La imagen observada en RAM después de B-records coincide byte por byte y por
SHA-256; no se escribió RAM ni PC desde Lua, debugger o el anfitrión.

- Identidad emitida por instrucciones del payload: `AS3K:P0:v1:BOOT OK\r\n`.
- Hex: `4153334b3a50303a76313a424f4f54204f4b0d0a`.
- Suma aditiva de su imagen, calculada por el payload: `00 00 32 3F`.
- Desafío host y eco real: `00 A5 FF 5A`.
- Estado posterior del payload: `4F 4B 0D 0A` (`OK\r\n`).
- El payload ejecuta `JMP $FFFFFF44`; otro B-record escribió `REENTER_OK`.
- Buffer interno: `7E 55 4E 71 4E 71 4E 71`; ejecución nativa D7=55.

La traza MMIO UART con tiempos emulados se contrastó íntegramente con el PTY:
994 bytes recibidos por la UART y 1022 escritos por la CPU para transmisión,
idénticos a host TX/RX. RX: 1121147–3442710 us; TX: 1121148–3442714 us.
SHA RX: `0b2b9e55dd50edf6ea46369ae97216a06253ccf6f02389107c679fc7ffb07ce4`.
SHA TX: `495885ebf72980c16ade4b0d029235314219d92e1ee3b8a77fe0e44556035e1d`.

USB R10 final: ambos procesos exit 0; descriptores originales de 18 bytes,
SET_ADDRESS y SET_CONFIGURATION, protocolo Get 2 y los 47 bytes TX=RX:
`52313020555342206f726967696e616c3a20323032362d31312d3035202f20416c706861536d61727420333030300d`.
Las transacciones completas están en `usb_r10_final/host/transactions.json`.

## Implementación y referencias

Se sustituyó únicamente el intento previo de bootstrap del driver por el
muestreo de EMUBRK en el dispositivo MC68EZ328. Un callback con pull-up por
defecto mantiene normales los otros usuarios del core. El tap de reset usa
direcciones de byte absolutas y datos/máscara de 16 bits; afecta cuatro lecturas
CPU de reset, sin cambiar los ocho bytes almacenados en SRAM. Modo, contador,
prescaler e IBUFF se registran para save-state. No se alteró el core 68000.

El modelo propio ocupa 234 bytes dentro de FFFFFF00–FFFFFFFF, incluye la
reentrada FFFFFF44 y ocho bytes escribibles en FFFFFFAA. El 68000 ejecuta
instrucciones reales para leer URX, convertir ASCII, escribir registros/RAM,
devolver ecos y hacer JMP al destino. No hay parser externo, segunda UART,
`PC=payload` ni copia directa del payload desde el driver. Modelo ensamblado:
SHA-256 `17560522f9763492bb2f3b1678975dccf53df649bd00ada95049969085e2b0a9`.

MC68EZ328UM §§16.1.1–16.6, Bbug.c (send, líneas 581–619; ejecución y buffer)
y WinBugIo.c se consultaron en sus rutas originales. El COUNT cero se ejecuta
al completar el décimo carácter, como exige BBUG: este recorta el terminador
del GO. No se inventaron mensajes de error o ACK adicionales. CR y caracteres
ajenos al hexadecimal mayúsculo reinician el ensamblaje del registro.

AS2K y ASPRO aportaron selección explícita, límites, verificación del eco,
identidad BOOT OK y evidencias aisladas. Se inspeccionaron fuentes locales:
AS2K actual conserva un punto de inyección HC11; ASPRO identifica su payload
visual como candidato no probado. Esas rutas de inyección, los registros y
opcodes HC11 no se copiaron a AS3K. Se conserva la interfaz de configuración
MAME existente: BOOT/EMUBRK y reset, sin rediseñar UI ni simular el PC.

El payload reside en SRAM 00030000–00030089; stack descendente desde 0003F000.
El loader no usa el SSP interno FFFCFFFC para llamadas de stack. Las pruebas
usan una SRAM descartable de 256 KiB sin System 3 ejecutándose bajo bootstrap;
no se selecciona ni altera una sesión persistente del propietario.

El adaptador serie de diagnóstico se conecta a los pines UART del MCU bajo
bootstrap. En modo normal se mantiene el control PG2 del SP3223. La UART R10
(temporización, FIFO, registros, IRQ) permanece byte idéntica al estado previo.
También se verificaron intactos USB/IP, ADB congelado y los receptores Send.

## Reproducción

Definir `AS3K_TREE` como el árbol canónico y `ROMPATH` como las rutas privadas
ya existentes; usar directorios de salida nuevos en cada ejecución.

```sh
cd "$AS3K_TREE"
TMPDIR=/dev/shm make SUBTARGET=alphasma3k SOURCES=src/mame/skeleton/alphasma3k.cpp OSD=sdl PRECOMPILE=0 REGENIE=1 -j2
./alphasma3k -validate
python3 scripts/as3k_bootstrap_build.py --output-directory work/bootstrap_ensayo/assembled
python3 scripts/as3k_bootstrap_test.py --system asma3ksw --rompath "$ROMPATH" --payload work/bootstrap_ensayo/assembled/payload0.bin --output-directory work/bootstrap_ensayo/writer
python3 scripts/as3k_bootstrap_test.py --system asma3ksb --rompath "$ROMPATH" --payload work/bootstrap_ensayo/assembled/payload0.bin --output-directory work/bootstrap_ensayo/boot
```

Para una sesión manual, iniciar el mismo `alphasma3k` con `-rs232 pty`, elegir
«EZ328 EMUBRK/PG5 during reset» bajo en Machine Configuration y reiniciar.
Enviar un carácter a 9600 8N1; esperar `@`; enviar B-records con eco comprobado.
Para salir, seleccionar alto y volver a reiniciar. En Lua las configuraciones
se cambian mediante `field.user_value`, no mediante `field:set_value`.

El ciclo se reproduce con `python3 scripts/as3k_bootstrap_reset_test.py --output-directory work/bootstrap_ensayo/resets --fixture "$AS3K_FIXTURE_R8" --rompath "$ROMPATH"`. Las regresiones reutilizadas quedan en el expediente local;
cada subprueba conserva `command.json`, Lua/host, salidas y estado NVRAM aislado.
En los bancos UART se habilitó físicamente PG2 antes de USTCNT, como requiere
el SP3223 integrado en R11. La comprobación PTY acepta OLD_DATA manteniendo
obligatorios DATA_READY, dato 5A y ausencia de errores; no se forzó el MCU
para arreglar un resultado. Los scripts originales permanecen intactos.

## Límites y fallos previos conservados

**HW-FÍSICO: NOT TESTED. ROM enmascarada original: NOT AVAILABLE / no acreditada.**
Se acredita el modelo documentado permitido por la orden, no identidad de
ciclos ni bytes con el silicio Motorola. El reloj AS3K está configurado a
16 MHz; UBAUD=010D produce aproximadamente 9615 baud, compatible con el
anfitrión 9600. El modelo escoge el divisor más próximo; no se acredita
autobaud con otras velocidades, cambios de baud en vivo, ni todos los casos
de comentarios malformados o vectores reservados/A-line. El buffer se pone
a cero al reset por elección determinista, no por medición física.
Save/load está registrado pero su recuperación dinámica queda NOT TESTED.

La primera prueba se detuvo por API Lua `register_stop` inexistente. La segunda
realizó el intercambio completo pero su aserción de breakpoint inicial era
tardía. Se añadió observación del fetch real del modelo. Dos pruebas de reset
usaron incorrectamente la API digital para una opción de configuración; la
fuente MAME demostró su semántica booleana. La prueba final usa user_value y
pasa los tres resets. No se corrigió hardware a ciegas para esos errores.

R11 ADB permanece congelado/OPEN: esta intervención no declara su cierre ni
reacredita impresión, PS/2 o IrDA completos. Las regiones de implementación
protegidas están intactas; se acreditan las regresiones finales enumeradas.
El único aviso global de `git diff --check` continúa siendo el preexistente
AGENTS.md:44; cambios propios sin errores. Excel: retirado por el propietario,
sin modificar inventario OS3K, libros AS2K/ASPRO/DANA ni crear un sustituto.

La revisión automática rechazó exportar el script de implementación a Drive:
consideró insuficiente la autorización específica del artefacto y no verificó
la propiedad del destino. No se burló ese rechazo mediante rclone ni codificación.
Las fuentes y el parche completo permanecen locales. La entrega documental
de evidencias, sin fuentes ni rutas privadas, se registra por separado y solo
se declara disponible si fue efectivamente subida y verificada.


Entrega documental bootstrap 2026-11-12: DRIVE_EVIDENCE_DELIVERY_PASS.
ZIP https://drive.google.com/file/d/1gD3X0ZZlAlzz3Ip0KnpK5mJuuK_oQBF1/view?usp=drivesdk
16960 bytes; SHA-256 remoto descargado 79187a62edaef499bc802b64e9e24fb79ff41b67f07f65264b4acfda22f9e291.
Bitacora permanente actualizada y releida: https://docs.google.com/document/d/1VsmwfH4bTQd_zE_RLb7DcExKVSIP1j5gql-2tz7edvo
El ZIP contiene 22 evidencias mas manifiesto; excluye fuentes por rechazo
de revision automatica. Fuentes y parche: expediente local LOCAL_SOURCES.
GIT_SYNC_BLOCKED: ahead 12 / behind 2, sin commit/push. Excel retirado por el propietario.

Entrega LCD verificada en Drive: https://drive.google.com/file/d/1GHPhuBc_7L3Gk6i42NM1fiZ9l9nrGBOF/view?usp=drivesdk
Captura PNG original: https://drive.google.com/file/d/15c3e_9kmsuEHW0amUrPk0Si0X5pPmtTL/view?usp=drivesdk
ZIP: 20201 bytes, SHA-256 remoto 9c074dfe57cbe868300d7b32c0f9e0b9573d022306ef1f0fd5a03eb3bea394d3.
PNG: 383 bytes, SHA-256 remoto 8213d41a2a71d49c441506a03be129243f9c089bde9aea4eb2d7f9a768c29597.
26 archivos publicables mas manifiesto. Fuentes y parche locales; sin ROM ni NVRAM en Drive.
Bitacora permanente del correctivo actualizada y releida: https://docs.google.com/document/d/1VsmwfH4bTQd_zE_RLb7DcExKVSIP1j5gql-2tz7edvo
