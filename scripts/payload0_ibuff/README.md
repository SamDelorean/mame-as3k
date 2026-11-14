# AS3K payload-0 — BOOT OK por bootstrap sin SRAM externa

## Objetivo original
Acreditar que el bootstrap nativo del MC68EZ328 ejecuta instrucciones y escribe `BOOT OK` en el LCD del AlphaSmart 3000 **sin leer, escribir ni ejecutar desde los 256 KiB de SRAM externa** y sin modificar Flash. No es un programa de diagnóstico.

## Archivos
- `as3k_payload0_ibuff.py`: fuente/generador de 93 instrucciones `MOVE.B #imm,($FFFFF4xx).L` (8 bytes cada una); únicamente escribe PCSEL, PCDIR y PCDATA internos.
- `payload0_ibuff_boot_ok.b`: secuencia exacta de 186 B-records verificada en AS3K-DIAG el 13 de noviembre de 2026.
- `send_brecords.py`: emisor UART para el flujo B-record, incluido para reproducibilidad. Hardware físico no probado.

## Reproducir los B-records desde la fuente

```sh
python3 as3k_payload0_ibuff.py --out payload0_ibuff_boot_ok.b --audit audit.json
```

**SHA-256 del payload verificado:** `9986657541fd4d8eddeb479fefdd1af63a5babbcdb91d061c69c14228d1428a0`.

## Alcance de validación
**PASS en el único AS3K-DIAG (modelo emulado de bootstrap Motorola)**. En la corrida aceptada hubo 93 escrituras GPIO, 30 pulsos LCD, captura de `BOOT OK`, 0 escrituras observadas a SRAM y SHA-256 de los 262,144 bytes de SRAM antes/después idéntico. **No está validado en el aparato físico**, y el mapeo de pines de la placa original deberá comprobarse antes de usar el procedimiento en hardware real.

Evidencias completas (incluyen captura, JSON, traza y arnés):
https://drive.google.com/file/d/1P5y1wVc9hjAijG3gK1eyx88tuFAcAl38/view

Estos archivos son parte del **único proyecto AS3K-DIAG**, no una versión independiente del emulador. No sustituyen `scripts/as3k_payload0.s`, que corresponde al anterior ensayo de SRAM externa.
