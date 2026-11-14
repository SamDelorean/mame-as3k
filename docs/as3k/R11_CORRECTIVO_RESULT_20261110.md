# R11 correctivo — OPEN / FAIL — 2026-11-10 UTC

No se ha terminado R11. Los resultados siguientes son ensayos comprobados,
no cumplimiento parcial presentado como cierre de la orden integral.

## Árbol, integridad y alcance

Repositorio canónico: /home/spc/Projects/alphasmart/mame-as3k-dev.
Rama as3k-mame0289-dev; HEAD de entrada
3ba23b4713f9a1eceb7135bdf9622bdadb43b599. Único ejecutable ./alphasma3k.
Se leyeron las instrucciones y los cambios pendientes antes de editar.
Se conservaron los cambios preexistentes. No se realizaron operaciones
Git destructivas, commits, push, clones, ramas ni subagentes/workers.
Los procesos MAME, interlocutores de protocolo y pruebas se invocaron
explícitamente; no se instalaron automatizaciones o temporizadores externos.

Paquete aplicado: AS3K_R11_CORRECTIVO_CODEX_20261109.zip, Drive
https://drive.google.com/file/d/19scS-k5MpXT7LK_r5xQeGNIT5G84W9e0/view
7154 bytes, SHA-256
bb8a081c1e508c02df13c0873be676e24b9dbac9c7bd44286cd73a0629c9d628.
Se leyeron las cinco entradas y se comprobaron los cuatro documentos
referenciados por el manifiesto interno: todos coinciden en tamaño y hash.
Prompt correctivo:
https://drive.google.com/file/d/1NV1bdpR-IXXL9stdqPw_WfyE7jwWvBi0/view

La Flash de la copia de ensayo R8 y la ROM WRITER de referencia coinciden:
SHA-256 1d5002e2e54c732768ee1e296eb9ca868d06935e07afb0d0d9e01e9b5d7135bb.
No se alteraron sus bytes. Las modificaciones de documentos ocurrieron
solamente en copias privadas de SRAM por el firmware original.
Los contenedores privados para desensamblado, las ROM y las fuentes
históricas AlphaSmart NO están en esta entrega.

## Criterios conjuntos

| Criterio | Resultado comprobado en esta intervención |
|---|---|
| PS/2 Send fiel, espacios y CR; negativo desconectado | FAIL / incompleto. Tramas Set-2 auténticas y válidas; captura inicial r8proof y otra parcial r8proof r. No se acreditó el documento completo con CR. Negativo final NOT TESTED. |
| ADB auténtico Send fiel | FAIL. Host físico de atención/sync/Talk 0x2C incorporado; se observaron comandos 0x2C reconocidos por la ROM, pero no una respuesta ADB de texto aceptable. Los bordes emitidos por rutinas PS/2 no se acreditan como ADB. |
| IrDA bidireccional útil | PASS del ensayo ir_actual_protocol: 32 bytes escritos y recuperados a través de SIR, IrLAP, IAS, TinyTP/IrCOMM y el manejador AlphaWord original. No se extrapola a un binario posterior. |
| Print original -> RAW -> CUPS -> PDF | PASS del ensayo print_current: 55 bytes HP/PCL completos, CUPS AlphaSmart_PDF-11 y PDF nuevo legible de una página. No es un PDF fabricado desde RAM. |
| Regresiones R3-R10 sobre estado final | FAIL / pendiente. Instantánea estable: 14/16 pruebas históricas PASS; FIFO y PTY UART FAIL. El árbol/binario volvió a cambiar después. USB no se reprodujo por esta intervención tras la última recompilación. |

Por tanto R11 permanece OPEN / FAIL. El PASS histórico de R10 se conserva
como evidencia histórica; no se declara preservado en un estado final nuevo
que esta intervención no ha verificado.

## Ingeniería y evidencia positiva

Se adaptó el receptor PS/2 de 11 bits de la referencia AS2K probada a PB4/PB5
con su inversión externa y retorno PB3/PB6. Se añadió el interlocutor ADB
mínimo con temporización de atención/sincronización, sondeo y decodificación
por pulsos. No se usó el F46 Set-2 como prueba ADB.

Se implementó UARTNIPR FFFFF90A y el modo SIR en MC68EZ328, con pulsos de
cero de tres ciclos del prescaler no entero, recepción óptica, interrupciones
FIFO y OLD DATA; se separaron las rutas óptica y NRZ. Se contrastó con
MC68EZ328UM, Irda68328EZ, UartModule y las rutinas efectivas de la ROM.

Causa demostrada del corte inicial de Print: una escritura original de BYTE
a USTCNT en PC 004D04F6, máscara 00FF, borraba también el byte superior del
registro emulado y apagaba UART. Tras respetar mem_mask en USTCNT/UBAUD/
UMISC, el mismo camino original produjo el trabajo completo de 55 bytes.
La presencia del módulo IR opcional se contrastó con PB1 en IrdaModule.

IrDA: el callback original recibe datos en 004E44B4 y deriva al manejador
AlphaWord 00408094; su transporte IR efectivo está en 00410026. El código
histórico consultado ofrecía GET_DWNLD_SIZE 0x89, pero este manejador de ROM
no implementa ese caso. Su consulta 0x90 devuelve el tamaño del archivo.
No se atribuye el ACK FB de 0x89 a una avería del hardware.

Texto transmitido Y recuperado: R11 IrDA original bidirectional + CR.
Hex exacto, ambos sentidos:
5231312049724441206f726967696e616c206269646972656374696f6e616c0d
Respuesta nativa al tamaño: FB 00 20. Se conservaron tramas y FCS reales.
El ejecutable y los archivos C++ no cambiaron durante este ensayo.
Binario del ensayo IrDA y de la instantánea de 16 regresiones:
4623e9dd26ee70f68fd2e9be902e08e90bc80d054fbe021a6069f5604868c6de.

Print RAW original completo:
1b451b266b33471b267330431b266130384c1b266137304d1b266c3035451b266c33441b28313255723870726f6f660d1b266c30481b45
SHA-256 RAW: 0a24f0d01c220308c0cad9c233cefca1bf87d8b690c2db9764bee1dcffef424d.
El helper AS2K HP/PCL se adaptó solamente al ESC(12U observado y a la cola
AlphaSmart_PDF. Texto derivado EXCLUSIVAMENTE del RAW: r8proof, una página.
PDF nuevo: AlphaSmart_20261109_180858_2b58f92f.pdf.
SHA-256: b3eb590fc65f2b0e13ca34ce2e28ff66bb15ef60adf8e9477cf06dbc6af1be0b.
El raster de la página nueva coincide exactamente con la página revisada
visualmente. Limitación: el filtro CUPS texttopdf instalado tiene un mapa
ToUnicode defectuoso; se acredita lectura visual, no extracción de texto.
El perfil de impresión mínimo acepta el subconjunto ASCII observado.

## Regresiones reproducidas, sin reutilizar PASS antiguos

En regressions_snapshot_20261110 se ejecutaron validate, cuatro auditorías
ROM, D11 BOOT/WRITER, R3 teclado, escritura nativa r8proofk10, persistencia
WRITER/SRAM y BOOT/Flash/SRAM, y programa/borrados Intel Flash: 14 PASS.
Los dos ensayos UART históricos fallaron: FIFO recibió ocho ceros y el
interlocutor PTY no recibió A5; CPU RX=0000. La nueva puerta de alimentación
PG2 introducida en el enrutado es el punto a contrastar con el banco UART.
No se aplicó ni se acreditó una corrección de esos fallos.
Las ROM y copias NVRAM privadas no se incluyen en el ZIP.

## Bloqueo comprobado: escrituras concurrentes en el árbol

Esta intervención no añadió send_sink_begin/byte/end al C++ ni retiró IRQ5
del callback de conexión. Esos cambios aparecieron mientras se trabajaba
sobre el mismo archivo. Se preservaron. Una segunda modificación ajena
incorporó bus/rs232/printer.h y SERIAL_PRINTER al driver.

Driver en la primera instantánea de concurrencia:
30c8c99a5e1aa96431c260460e0ee03a49d85d88b824bd3bf3fdb1996f1a2456.
Driver al rechazarse el siguiente parche por su guardia de hash:
6be4d802d820114dda6dd478ea01d180f26ad43ff31eacd59e584fb9affb74d3.
La corrección de enrutado preparada NO SE APLICÓ: la guardia abortó antes
de escribir. El comando de shell continuó con make, que recompiló el árbol
concurrente; esto no aplica la corrección y no constituye una verificación.
La repetición posterior no se ejecutó: su script no llegó a crearse.
Binario observado después: 1fb3490e133c8b06b1f9fb263800e310bea1b4e53528ebe4c02d4f6a741baa78.
No se atribuye al último binario ninguno de los PASS de un binario anterior.
También se observaron nuevas notas y otra instancia MAME de esa intervención.
No se interrumpieron procesos ajenos ni se sobrescribieron sus modificaciones.

Se pidió identificar qué intervención mantiene la escritura exclusiva.
Hasta aclararlo, quedan detenidas las modificaciones que entren en conflicto
con la conexión PS/2/ADB y la certificación del estado final.
El acceso al entorno está disponible; el bloqueo NO es falta de archivos,
permisos generales, ROM ni herramientas. Las causas funcionales restantes
PS/2/ADB no se dan por resueltas ni se sustituyen por forzados de registros.

## Reproducción exacta y archivos

Comandos ya ejecutados desde el repositorio canónico:

    TMPDIR=/dev/shm make SUBTARGET=alphasma3k SOURCES=src/mame/skeleton/alphasma3k.cpp OSD=sdl REGENIE=1 PRECOMPILE=0 -j3
    python3 work/r11_correctivo_20261109/run_native.py ir --name ir_actual_protocol --seconds 18 --no-typing
    python3 work/r11_correctivo_20261109/run_native.py print --name print_current --seconds 18 --no-typing
    python3 scripts/as3k_print_to_host.py work/r11_correctivo_20261109/print_current/printer.raw
    python3 work/r11_correctivo_20261109/replay_regressions.py

Los nombres de carpeta de prueba son exclusivos y no se reutilizan: las
carpetas existentes contienen el resultado de la ejecución acreditada.
Cada prueba incluye command.json y Lua; la prueba IR incluye environment.json,
RAW de ambos sentidos, ir_transactions.json e ir_result.json. Se incluyen
los helpers propios y un diff de revisión contra la entrada R10; ese diff
contiene cambios mezclados de más de una intervención, no una versión final
aceptada ni una atribución de autoría de todos los cambios.

Entrega documental OPEN/FAIL verificada en Drive: https://drive.google.com/file/d/1oc_qutzmPwWOnfJmRjoOMNR8kf_PEXw_/view?usp=drivesdk
ZIP 164794 bytes, 169 entradas. SHA-256: 27a1586d409656906f72ee9f15d1d30ced223da6a4c6e86acc0d0a8f682129c5.
El ZIP es una instantánea de evidencias; no certifica R11 terminado ni el binario concurrente posterior.

## ADB 2026-11-10 — revalidacion focalizada; OPEN

No se reabrieron R10, IrDA, PS/2 ni Print/PDF. Se examinó directamente
la ROM original: el comando ADB Talk Reg 0 (0x2C) sí se decodifica, pero
el manejador sale al devolver 0x00 y reinicializa el búfer. Un sondeo
con breakpoints observó 38 comandos 0x2C, 80 reinicializaciones y
ocho entradas a la consulta de teclado sin datos pendientes.
Pulsación Send de 3 segundos: buffer 0, modo 0, salida PB5 sin
respuesta ADB. El temporizador del MCU y los requisitos de limpieza
de TSTAT se contrastaron con el manual oficial; no se aplicaron
ajustes de registro contrarios a sus reglas.
Un ensayo aislado redujo solo el tiempo de espera del interlocutor
de 6000 a 2000 us; ROM no transmitió datos. Se restauraron fuente y
único ejecutable canónicos, hashes idénticos antes/después.
No es lícito declarar ADB PASS ni R11 integral CLOSED/PASS.
Evidencia local acotada: work/r11_adb_connection_scan_20261110,
work/r11_adb_cadence_20261110 y work/r11_adb_rom_breaks_20261110.
La corrección de ADB permanece pendiente; no crear variantes
ni tocar componentes acreditados.

## 2026-11-10 — ADB original handler identificado; Send OPEN

Se corrigió la dirección del interlocutor ADB en el ÚNICO
src/mame/skeleton/alphasma3k.cpp: la ROM inicializa la dirección propia
gubADBCurrentAddress a 0x70 (dispositivo 7), y el handler a 0x5E.
Talk Register 3 correcto = 0x7F (no 0x2F); Talk Register 0 del teclado
Mac = 0x2C. Se observaron ocho comandos 0x7F válidos y 288 bordes
de respuesta PB5, con dieciséis bytes ADB (incluido ID estable 0x5E).
El anfitrión integrado ahora identifica una vez el dispositivo y
continúa con sondeos 0x2C; respuesta real a 0x7F acreditada.

NO se acreditó Send: después de once pulsaciones físicas reales,
la ROM no alimentó su búfer y no emitió códigos de teclado.
La matriz física sí responde (PA=BF, PDDATA=EF con Send presionado);
se había usado equivocadamente PFDATA para una medición anterior,
dato corregido. ADB sigue OPEN, R11 global OPEN.

Se probaron y revirtieron sin persistencia los cambios de atención
1100 us e interrupción IRQ5 sólo modo Macintosh; ambos empeoraron
o no mejoraron ADB. Fuente y binario quedaron exactamente restaurados
tras dichos ensayos. La nueva enumeración 0x7F sí produce
respuesta auténtica del firmware y queda integrada.
No se modificaron ROM, memoria Writer ni controladores PS/2, UART,
IrDA, impresión o USB. Sin commit ni push.

Evidencias:
work/r11_adb_correct_addr_20261110/result.json;
work/r11_adb_native_send_after_id_20261110/error.log;
work/r11_adb_real_scan_stats_20261110/error.log;
work/r11_adb_host_gpio_duty_20261110/error.log.
