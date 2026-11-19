# AlphaSmart Pro (`asmapro`) — avances de emulación sobre MAME 0.289

Rama: `aspro-mame0289-dev`. Fuente canónica del desarrollo:
[SamDelorean/ASPRO-emu](https://github.com/SamDelorean/ASPRO-emu) (`main`, AP-10, 22/23-oct-2026).
Corte de esta publicación: 19-nov-2026.

## Contenido publicado

| Ruta | Origen en ASPRO-emu | Descripción |
| --- | --- | --- |
| `src/mame/skeleton/alphasma.cpp` | `stable/alphasma.cpp` (blob `75e2d5f`) | Driver `asmapro` (`aspro_send_state`): matriz de teclado Pro derivada de `INPUT_PORTS_START(alphasmart)`, LCD 240×36, NVRAM, PC conectado/Send. `asma2k` se conserva en el mismo archivo. |
| `src/devices/cpu/mc68hc11/mc68hc11.{cpp,h}` | `stable/proven_hc11/` | MC68HC11D0: registro HPRIO y soporte acotado de Special Bootstrap SCI. Desactivado salvo que la máquina lo habilite (`set_diag_bootstrap`). |
| `scripts/aspro/aspro_ui.py`, `aspro_session.py` | `stable/` | Frontend GTK3 (LCD virtual, teclado, menús) y gestor de sesión que lanza `asmapro`. |
| `docs/aspro/*.md` | `docs/`, `stable/RELEASE_MANIFEST.md` | Mapa de hardware, auditoría de periféricos, Send TXT, bootstrap D0, contrato LCD, tiempos frío/caliente, procedencia y manifiesto. |

Compilación limitada (patrón MAME 0.289):

```
make SUBTARGET=aspro SOURCES=src/mame/skeleton/alphasma.cpp REGENIE=1
```

## Estado verificado

| Ámbito | Resultado |
| --- | --- |
| Vontar, Armbian aarch64 | Compilación, validación e instalación **PASS** (`EXIT=0`), 1 driver `asmapro` |
| Surface RT, postmarketOS ARMv7/musl | Compilación nativa, smoke con ROM e instalación **PASS** |
| Uso interactivo posinstalación (pantalla, teclado, PC, SCI) | **Pendiente** |

Validación funcional en el emulador ASPRO-DIAG (binarios de diagnóstico HC11 adaptados de AS2K):

| Prueba | Resultado |
| --- | --- |
| L2.0 A→B→C→B→A, L2.0B, L2.1, L2.2 | PASS MAME |
| L2.3 banco 0 `$0100–$7FFF` (32 512 B) | PASS MAME, snapshots iguales |
| L2.4 selección/restauración 4 bancos | PASS MAME (MV09) |
| L2.5 firma ROM Pro 2.1.2 | PASS de firma ROM emulada |
| L3 Escape, LCD0 (6 etapas) | PASS MAME con estímulo por debugger |

Hallazgos de hardware relevantes para el driver:

- Selección de banco RAM por puerto A: `((PA>>3)&1) | ((PA>>4)&2)` (PA3/PA5); la máscara AS2K `$CF` pasa a `$D7` en el Pro.
- El Pro no tiene el diagnóstico ZPSD (L2.6) de AS2K.
- Puertos físicos PC AT/PS2 y ADB confirmados; aún no hay host ADB/PS2 en el driver.

**No comprobado:** SCI del host tras bootstrap real, RAM completa de 128 KiB, LCD1, teclado/LCD físicos.
El driver sigue marcado `MACHINE_NOT_WORKING`.

## Privacidad

No se incluyen ROMs, payloads binarios, NVRAM ni trazas privadas. Las huellas SHA
de los insumos privados figuran en `RELEASE_MANIFEST.md` solo para comprobación local.
