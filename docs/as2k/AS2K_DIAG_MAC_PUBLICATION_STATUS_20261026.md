# AS2000 MAME-DIAG — Macintosh/ADB capture, 2026-10-26

## Estado de publicación

- Parche fuente: `docs/as2k/patches/AS2K_DIAG_MAC_CAPTURE_20261026.patch`.
- Rama canónica: `as2k-mame0289-dev`.
- El parche está **publicado**, pero **no aplicado** a `src/mame/skeleton/alphasma.cpp` en esta rama. No se debe presentar esa rama como capturador Mac integrado.
- Motivo: el origen remoto avanzó 46 commits respecto de la copia T640, y la integración automática desde su ancestro común produjo 10 conflictos en `alphasma.cpp`, incluyendo la arquitectura ZPSD/diagnósticos de bootstrap. No sobrescribir ni forzar ese avance.
- El parche se genera sobre el contenido T640 justo anterior a Macintosh: SHA-256 `fba9b5c76d0f0d6d11cccb4f210a827306e2a849d7fcdcf0d66b631052db1eab`. Aplicarlo **solo a esa base exacta** o conciliarlo manualmente con cambios posteriores.

## Prueba real cerrada en T640 (MAME-DIAG canónico)

- Compilación/enlace MAME-DIAG: PASS (GCC 15.2.0).
- Entrada nueva `DIAG_MAC_ATTACHED` sin modificar `DIAG_PC_ATTACHED`.
- Captura de pulsos de salida HC11 `PORTD.1`; decodificación de 21 paquetes ADB; generación de `as2kdiag-adb.txt`.
- Captura exacta `ABC 123` (7 caracteres), cursor DynFS 7.
- Retorno de CPU normal; pila máxima 61/62 bytes; documento y portapapeles intactos.
- Repositorio F46 probado: `SamDelorean/AS2K-V3.14.x` commit `3bf6f6e9dff98ce21a03c3e8c6b4d9251d664cda`.
- Evidencia y manifiesto: https://drive.google.com/drive/folders/1oPVzQo7cxihjYnhMoBGwTqpaOgrRzOr8

## Alcance exacto

La prueba MAME utilizó un script que entregó al firmware comandos `Talk` **ya decodificados**. El receptor físico de comandos ADB del firmware F46 todavía no está implementado; el emulador no origina una secuencia física de solicitudes ADB del Macintosh. La recepción de texto demuestra la respuesta eléctrica del firmware a esas solicitudes de prueba, no una sesión autónoma Macintosh.

Este paquete no cambia PC/Set-2 ni IrDA y no autoriza marcar F46 CLOSED.

## Integración posterior obligatoria

Combinar selectivamente las declaraciones/entradas Macintosh y el sumidero ADB con el controlador remoto actual, preservando diagnósticos ZPSD, modos bootstrap y trabajo local anterior. Compilar y repetir una sola prueba de captura en el DIAG resultante antes de declarar integrado el parche.


## Corrección adicional: selección y registro Macintosh (26/oct/2026)

- Parche incremental publicado: `docs/as2k/patches/AS2K_DIAG_MAC_MENU_CAPTURE_FIX_20261026.patch`. Aplicar **después** del parche base `AS2K_DIAG_MAC_CAPTURE_20261026.patch`, sobre la copia T640 compatible, no sobre el remoto divergente sin conciliación.
- Selector normal **Pause/Break** recorre Desconectado → Impresora → PC → **Mac ADB keyboard (DIAG capture)**, sin usar `DIAG_MAC_ATTACHED` en la prueba de menú.
- El modo Mac mantiene **PA0 alto, PA2 bajo** en reposo, captura se arma al pulsar SEND y se desarma/cierra al desconectar.
- El modo Mac en el selector normal **no requiere** `AS2K_DIAG_ADB_SINK`. La entrada diagnóstica independiente sí conserva ese opt-in.
- Al cerrar se registra `AS2K_ADB_TEXT CLOSE packets=21 bytes=7` y se guarda `as2kdiag-adb.txt` con el contenido exacto `ABC 123`.
- **PASS**: compilación/enlace GCC 15.2.0; MAME-DIAG 21 paquetes físicos PD1, cursor DynFS=7, STACK 61/62 y documento/portapapeles invariantes. Captura verificada con variable ADB desactivada.
- Evidencia local T640: `/home/spc/Projects/alphasmart/F46_DIAG_RESULTS/as2k_writer_f46_mac_menu_noflag_20261026/results/`.
- **No es una sesión autónoma**: los comandos `Talk` continúan suministrados ya decodificados por el guion; receptor físico ADB F46 pendiente. F46 sigue abierta.

## IrDA — reproducción stock calificada, 26/oct/2026

- **Corrección real del DIAG en T640:** los hooks pasivos de IrDA en el núcleo HC11 (`D2DC` entrada, `D0D7` inicialización, `D1EF` dato, `D220` drenado, `9719` cierre y equivalentes Print) no deben depender de Gate 1A. El selector `set_as2k_gate1a_probe(false)` había desactivado también esos eventos; se añadió `set_as2k_ir_observer(true)` solo en DIAG, dejando Gate 1A deshabilitado.
- Parche verificable de tres fuentes (HC11 CPP/H y driver) `docs/as2k/patches/AS2K_DIAG_IR_OBSERVER_EOF_20261026.patch`. **Publicado como parche y aplicado/compilado en T640**, no fusionado al controlador remoto divergente. Incluye flush/close de captura al EOF TinyTP real.
- Prueba stock de envío **PASS** en MAME-DIAG canónico: ROM 3.1.4, NVRAM histórica con archivo F1 `IR TEST 123`, tecla SEND física y ningún cambio de PA7, CPU, RAM, ROM o estado de sesión. Traza: `AS2K_IR_PEER BEGIN role=primary/send`, `FILE_DATA bytes=11 total=11`, `AS2K_IR_SEND_CAPTURE EOF bytes=11 status=COMPLETE`. Archivo `irda-send-text-*.txt` = exactamente 11 bytes `IR TEST 123`.
- La polaridad stock de presencia `D3F3` quedó corregida en `SamDelorean/AS2K-V3.14.x/docs/WRITER_F46_STOCK_IR_PA7_FINDING_20261026.md`: `BRSET PA7` → `SEV` → `D2DC` continúa; cuatro muestras bajas → `CLV` → cancelación. Las interpretaciones anteriores que invertían esta condición están supersedidas.
- Evidencia Drive: carpeta `AS2K_TEMP/results/F46_STOCK_IR_RECOVERED_20261026`. **Importante:** este PASS califica el receptor del emulador **con ROM stock**, no F46/DynFS; el `P1_SendFile` actual no despacha IrDA y todavía devuelve `B=$63` cuando host=3.
