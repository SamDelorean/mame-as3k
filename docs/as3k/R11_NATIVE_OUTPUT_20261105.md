# R11 — ejecución diagnóstica incompleta / OPEN / FAIL
Fecha de referencia del paquete y del cliente: 2026-11-05.

**No se completó R11.** Se ejecutaron inspección de reutilización, dos ensayos
nativos y las regresiones. No se implementaron los interlocutores PS/2/ADB,
la ruta óptica IrDA ni la integración Print/PDF. Este informe documenta el
resultado fallido de la intervención; no acredita una entrega funcional parcial.
No hay bloqueo de acceso: el árbol, paquete y ocho referencias están disponibles.
Falta desarrollo de hardware/interlocutores y su acreditación; no se atribuye
la falta de cierre a una autorización pendiente ni a archivos ausentes.

## Matriz de aceptación

| Condición | Estado observado |
|---|---|
| Send PC PS/2 exacto | OPEN / NOT TESTED positivo; Send sin host ejecutado, exit 0 |
| Send Macintosh ADB exacto | OPEN / NOT TESTED; no captura ADB realizada |
| IrDA bidireccional útil | OPEN / FAIL aceptación; se observa actividad durante Print/Send, sin intercambio útil de ida y vuelta |
| Print original -> RAW -> CUPS -> PDF fiel | FAIL; 173 bytes observados en PTY, helper existente los rechaza con exit 2; ningún PDF AS3K acreditado |
| Regresiones R3–R10 | PASS: 16 controles y aceptación USB completa reejecutados en este turno |

## Paquete e inventario

ZIP de entrada Drive y T160 SHA-256:
`b44675f30936746be1d5dc31dda2b966682d7a701ca52b7befde5e75178eda75`.
Cinco entradas: manifiesto y cuatro documentos. Los cuatro tamaños y SHA-256
coinciden. Las ocho referencias del manifiesto existen; no están incluidas como
código en el ZIP. El manifiesto declara existencia, no hashes ni acreditación
funcional de esas ocho rutas. Se leyeron orden, prompt, referencias y README.

Árbol único: `/home/spc/Projects/alphasmart/mame-as3k-dev`.
Rama `as3k-mame0289-dev`, HEAD `3ba23b4713f9a1eceb7135bdf9622bdadb43b599`.
Único ejecutable `./alphasma3k`, SHA-256:
`d963fff7e7d39111f67b30e9a71f48d3a5c04ddaba9f2bcdf2e1605ffa5bca27`.
No se recompiló ni cambió código del emulador. Fuentes R10 y script de aceptación
conservan exactamente los hashes registrados al cerrar R10.

## Matriz de reutilización contrastada

| AS2K origen | AS3K destino físico nativo | Verificación y límite |
|---|---|---|
| AS2K_STABLE_HOST_SOURCE.cpp, pc_clock_rising/pc_set2_byte, Send sink | PS2Assembly: PBDATA, PB4 clock out, PB5 data out; PB3/PB6 sense | El driver AS3K no conecta PB4/PB5 a un receptor. No se adaptó todavía. No se copiaron PD0/PD1 del HC11 |
| Referencia F46 adb_command_pass y selector estable | ADBAssembly: atención/sync por PB6, salida PB5, Timer1 | El estable dice ADB no implementado. F46 registra P1_HOST_PC_WRITE_BYTE, sink Set-2 y ABC 123: no acredita Send ADB. No se ejecutó ese harness HC11 ni se reinterpretó como ADB |
| F46 ir_bridge_20261027_h12 | Irda68328EZ/UartModule: PE4/PE5, PA7/PG2, UARTMISC y UARTNIPR | summary.json dice FAIL_IR_DIRECTED, exit 27; result optical_pass=false, received_hex vacío. No existe PASS óptico en esa referencia concreta |
| as2k_print_to_host.py, CUPS AlphaSmart_PDF, pdf_receiver.py | PrintModule nativo RS232/UART y perfiles de impresora | Helper reutilizado sin modificar sobre captura real: rechaza formato. No se sustituyó por PDF escrito directamente |

Las diferencias HC11/EZ328 son materiales. El fuente estable acreditado fue
identificado desde la bitácora (AS2K_STABLE_HOST_SOURCE.cpp), sin compilar su
checkout histórico sucio. No se ejecutó ni modificó ningún emulador AS2K.
Las conclusiones sobre ADB/IrDA se limitan a las referencias indicadas; no se
ha demostrado que no exista otra implementación válida en todo el proyecto.

## Ensayo nativo Print y evidencia de hardware

Prueba por tecla física original `:COL9` máscara 10, presionada entre segundos
3 y 3.2. ROM original WRITER, copia diagnóstica del NVRAM acreditado de r8proof,
UART al dispositivo MAME PTY. Un proceso independiente leyó el PTY, sin leer
la memoria del documento para fabricar salida. MAME exit 0, sin error Lua.
Se conservaron exactamente 173 bytes observados en `native_print_probe/uart.raw`:

`ffffffffc0ff3f0100000003ffffffff050000332fc1ffffffffc0ff3f0100000003ffffffff0101008a55c1ffffffffc0ff3f0100000003ffffffff010200e27fc1ffffffffc0ff3f0100000003ffffffff0103003a66c1ffffffffc0ff3f0100000003ffffffff010400322bc1ffffffffc0ff3f0100000003ffffffff010500ea32c1ffffffffc0ff3f0100000003ffffffff01ff00820400416c706861536d6172742b09c1808a4dfaaaf4`

Estos son bytes del PTY durante el ensayo, **no un trabajo Print RAW validado
ni prueba de modulación óptica**. Hay delimitadores C0/C1 y la cadena AlphaSmart,
compatibles con descubrimiento IR; esto no demuestra IrLAP/IrCOMM bidireccional.
La traza CPU registra escritura original en `FFFFF90A=0220`, PC `004D015E`,
que el núcleo informa como unmapped. UartModule.c identifica esa escritura
como UARTNIPR/prescaler para IrDA y selecciona UARTMISC bit 20, PA7 y PG2.
El modelo actual UART emite 8N1 por RS232 y el driver conecta TXD a RS232 sin
interlocutor óptico separado. Estas ausencias se verificaron en código/traza.
No se ha aislado cuál causa por sí sola cada fallo del Print nativo; no se
presenta la escritura unmapped como causa única demostrada del PDF ausente.

Se reutilizó el helper exacto existente:
`python3 /home/spc/Projects/alphasmart/mame-as2k-stable-pr1/scripts/as2k_print_to_host.py <native_print_probe/uart.raw>`.
Resultado real exit 2: `AS2K_PRINT_HOST REJECTED Incomplete or unsupported HP/PCL job`.
No se enviaron textos fabricados a lp ni se generó un PDF alternativo.
`lpstat -p -d` exit 0: AlphaSmart_PDF idle, destino predeterminado.
La consulta systemctl --user falló por ausencia de DBUS_SESSION_BUS_ADDRESS y
XDG_RUNTIME_DIR; eso no acredita que el receptor esté apagado ni se atribuye
como causa del fallo, pues el helper rechazó el formato antes de llamar CUPS.

## Send sin anfitrión

Se accionó la tecla física original `:COL7` máscara 10 entre segundos 3 y 3.2,
sin importar un anfitrión USB ni conectar receptor PS/2/ADB. Exit MAME 0.
Final de observación: PC `004CE44C`, PB `48`, PBDIR `30`, PF `00`.
También aparece la escritura UARTNIPR no mapeada. No hay receptor PS/2/ADB
instalado en este ensayo: no se afirma una captura positiva ni una validación
negativa completa de esos receptores. El resultado es un control sin anfitrión.

## Regresiones realmente reejecutadas

`regressions/summary.json` registra 16 controles PASS y `usb_original` PASS.
Incluyen validate, auditoría de cuatro ROM, D11 BOOT/WRITER, UART FIFO y PTY,
R3 D0=0, escritura nativa r8proofk10, capturas persistentes SRAM/Flash iguales
píxel a píxel y comandos/borrados Intel Flash exactos. Se reutilizaron los Lua
corregidos de R10 con machine:exit(), sin cambios a los originales privados.
USB: cliente independiente y MAME exit 0; descriptor auténtico
`12010001000000081e0804bd010001020001`, SET_ADDRESS/SET_CONFIGURATION,
Get 47 bytes TX=RX, concurrencia y UNLINK. Evidencia en usb_regression/.
No se extrapolan estos PASS a las cuatro interfaces R11 no acreditadas.

## Reproducción y preservación

Los comandos exactos y Lua están en
`work/r11_20261105/native_print_probe/{command.json,probe.lua}`,
`negative_send/{command.json,probe.lua}` y cada carpeta de regresión.
La captura PTY utiliza el mismo mecanismo de lectura independiente de
`/proc/<pid>/fdinfo`/tty-index probado en el harness UART R10.
Se emplearon copias privadas de NVRAM; las ROM originales y datos del usuario
no se alteraron. No hubo commit, push, reset, stash, pull, rebase, clones,
workers, subagentes ni nuevas automatizaciones.
Sólo se modifican STATUS.md, CODEX_RESULT.md y esta bitácora R11. El chequeo
propio de espacios se exige sin errores; el aviso previo AGENTS.md:44 se conserva.
No se declara R11 CLOSED, ni se reemplaza esta falta de implementación por un
plan de trabajo o por resultados parciales presentados como cumplimiento.


## R11 correctivo — 2026-11-10 UTC — esta intervención: OPEN / FAIL

Resultado comprobado y límites en R11_CORRECTIVO_RESULT_20261110.md.
IrDA: ida/vuelta nativa de 32 bytes idénticos, consulta 0x90 = FB0020.
Print: RAW HP/PCL completo de 55 bytes -> CUPS AlphaSmart_PDF-11 -> PDF
nuevo legible, una página. Se guardaron RAW, tramas, comandos y hashes.
PS/2 con espacios/CR y ADB fiel: FAIL / no acreditados integralmente.
Regresiones propias sobre binario 4623e9dd...: 14/16 PASS; FIFO y PTY UART
FAIL. No se proyecta ningún PASS sobre el binario recompilado después.
Se preservan las notas de otras intervenciones, incluido su R10 histórico,
pero esta intervención no reproduce su nuevo USB después del último build.
Bloqueo comprobado: dos cambios concurrentes ajenos en el driver; la guardia
de hash detuvo el parche antes de escribir. No hubo rollback ni sobrescritura.
Se solicitó aclarar la escritura exclusiva del árbol. R11 NO TERMINADO.

Entrega documental OPEN/FAIL verificada en Drive: https://drive.google.com/file/d/1oc_qutzmPwWOnfJmRjoOMNR8kf_PEXw_/view?usp=drivesdk
ZIP 164794 bytes, 169 entradas. SHA-256: 27a1586d409656906f72ee9f15d1d30ced223da6a4c6e86acc0d0a8f682129c5.
El ZIP es una instantánea de evidencias; no certifica R11 terminado ni el binario concurrente posterior.
