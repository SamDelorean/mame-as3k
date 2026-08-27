# Codex result — MC68EZ328 keyboard GPIO static audit

Date: 2026-08-26

Status: **complete; static stop condition met**. No system was executed, nothing was rebuilt, and no MAME source, driver list, ROM definition, fixture, binary, or diagnostic file was changed.

## Repository gate

- `git pull --ff-only as3k-project as3k-mame0289-dev`: already up to date.
- Branch: `as3k-mame0289-dev`; initial tracked status clean.
- Initial `git diff --check`: passed.
- The incoming `docs/as3k/CODEX_RESULT.md` recorded the successful `asma3kdv` Phase-1 return to `0x00420168` with `D0 = 0` and the exact 12 transactions.

## Evidence inspected

- MAME 0.289: `src/devices/machine/mc68328.cpp` and `src/devices/machine/mc68328.h`.
- Driver boundary check: `src/mame/skeleton/alphasma3k.cpp`.
- Local AlphaSmart ISO, extracted outside Git at `/private/tmp/as3k-keyboard.xJ1bRo`: `AS3000/Software/ModuleSources/M68328EZ.h`, `KeyboardModule.c`, and `AS3000/Software/BuiltApplet/KeyboardModule.o.lst`.
- Motorola/Freescale *MC68EZ328 Integrated Processor User's Manual*, revision 1, 11/98, especially the memory map and section 7.2.1/7.2.4. No MC68EZ328 processor manual is present in the local ISO, so the archived primary manual at `https://www.nxp.com/docs/en/reference-manual/MC68EZ328UM.pdf` was consulted read-only.

The AlphaSmart `M68328EZ.h` names `0xFFFFF403` as `M328_PASEL` and `0xFFFFF41E` as `M328_PKBDINT`. The processor manual instead marks `0xFFFFF403` reserved and names `0xFFFFF41E` PDKBEN (Port D Keyboard Enable). The linked AlphaSmart code nevertheless writes both addresses, so the source names describe its programming convention, not necessarily the silicon register names.

## Inheritance and implementation boundary

`mc68ez328_device::internal_map()` calls `base_internal_map(0xFFFFF000, map)`. Consequently the base entries at offsets `0x400`, `0x401`, `0x418`–`0x41A`, `0x41C`, `0x41D`, and `0x41F` are live for EZ. It does **not** inherit additions made by `mc68328_device::internal_map()`, including that original-DragonBall class's PASEL mapping at `0xFFF403`.

The base class owns `m_pasel`, but only `mc68328_device` maps `pasel_r/pasel_w` and registers `m_pasel` for save-state. On EZ, `mc68ez328_device::scr_w()` sets the internal `m_pasel = 0xFF` when `SCR.WDTH8` is written as 1. This is MAME's existing representation of the documented EZ rule: Port A is D[7:0] after reset and becomes GPIO in an 8-bit-only system through `SCR.WDTH8`. The EZ has no PASEL register to map.

## Register-by-register MC68EZ328 audit

“Reset” below is the value assigned by the MAME core. “Saved” means registered by the EZ save-state path.

| Register | EZ map and handler | State / reset / saved | Implemented data, callback, or interrupt behavior | Audit result |
| --- | --- | --- | --- | --- |
| PADIR `0xFFFFF400` | Yes, base `padir_r/w` | `m_padir`; `0x00`; yes | Write stores direction; read returns it. It does not itself drive callbacks. | Existing reusable implementation. |
| PADATA `0xFFFFF401` | Yes, base `padata_r/w` | `m_padata`; `0x00`; yes | Write stores data and calls each PA output callback only where `m_padir & m_pasel` is 1. Read returns the latch for selected outputs, selected input callbacks for inputs, and 0 for unselected pins or selected inputs with no callback. It reports the latch rather than the documented actual output pin level. | Existing implementation is sufficient for initial AS3K PA output columns once `SCR.WDTH8` has selected Port A and callbacks are connected; fidelity gaps remain. |
| PAPUEN `0xFFFFF402` | No handler or map | No `m_papuen`; no core reset/save | Documented per-bit pull-up enable, read/write, reset `0xFF`; MAME's PADATA input fallback has no Port-A pull-up state. | **Core state/behavior addition.** |
| PASEL `0xFFFFF403` | No EZ map; original-only `mc68328_device::pasel_r/w` | Shared internal `m_pasel`; base reset `0x00`; not saved by EZ | On EZ, `SCR.WDTH8=1` sets internal selection to `0xFF`. The manual marks `0x403` reserved and documents no PASEL. | The earlier “PASEL supported” classification was wrong, but adding PASEL to the EZ map would also be wrong. The AlphaSmart write is a harmless reserved-register write. |
| PDDIR `0xFFFFF418` | Yes, base `pddir_r/w` | `m_pddir`; `0x00`; yes | Stores/returns direction. Direction changes do not immediately emit outputs or reevaluate interrupts. | Existing state/handler, with fidelity caveats. |
| PDDATA `0xFFFFF419` | Yes, base `pddata_r/w` | `m_pddata`; `0x00`; yes | Outputs return the latch; inputs call per-bit callbacks, otherwise return the matching `m_pdpuen` bit. Writes drive output callbacks for output bits. Writes also clear modeled edge interrupt lines for bits 0–3 where `m_pdirqedge & data` is 1. | Existing scan-input path is reusable. Edge/interrupt details are incomplete. |
| PDPUEN `0xFFFFF41A` | Yes, base `pdpuen_r/w` | `m_pdpuen`; `0xFF`; yes | Stores/returns all eight pull-up enables; supplies the PDDATA fallback when an input callback is unset. | Existing reusable implementation. |
| PDSEL `0xFFFFF41B` | No handler or map | No `m_pdsel`; no core reset/save | Manual implements SEL7–SEL4 only, reset `0xF0`; 1 selects GPIO. PD3–PD0 have no SEL bits and are always usable as GPIO. Current MAME treats all PD bits as GPIO regardless of selection. | **Core state/behavior addition** for register fidelity and PD7–PD4 mux gating; not merely a map fix. |
| PDPOL `0xFFFFF41C` | Yes, base `pdpol_r/w` | `m_pdpol`; `0x00`; yes | `port_d_in_w()` uses bits as the comparison polarity for modeled individual interrupts. MAME stores all eight bits, although the EZ documents POL3–POL0 only. | Present but over-broad and not a complete EZ model. |
| PDIRQEN `0xFFFFF41D` | Yes, base `pdirqen_r/w` | `m_pdirqen`; `0x00`; yes | Handler stores/returns all eight bits. Critically, `port_d_in_w()` never consults `m_pdirqen`, so the enable register does not gate the modeled individual interrupt lines. EZ documents IQEN3–IQEN0 only. | Mapped state exists; interrupt semantics require later correction. |
| PKBDINT / PDKBEN `0xFFFFF41E` | No handler or map | No keyboard-enable state; no reset/save | Manual: KBEN7–KBEN0, reset `0x00`, read/write. Each 1 selects an input-configured PD pin into an active-low, level-sensitive OR that raises the keyboard interrupt; SEL/POL/IQEN/IQEG do not affect it, and only deasserting every selected low source clears it. | **Core state/interrupt addition**, not an acknowledge/status register. |
| PDIRQEDGE / PDIQEG `0xFFFFF41F` | Yes, base `pdirqedge_r/w` | `m_pdirqedge`; `0x00`; yes | `port_d_in_w()` selects level versus edge behavior from it; PDDATA writes can clear modeled edge lines. MAME stores all eight bits, although EZ implements IQEG3–IQEG0 only. | Present but over-broad/incomplete. |

Base save-state registration covers PADIR, PADATA, PDDIR, PDDATA, PDPUEN, PDPOL, PDIRQEN, PDIRQEDGE, and the non-mapped `m_pdindata`. It does not cover `m_pasel`; the original MC68328 subclass registers that field, but the EZ subclass does not. That is an independent EZ save-state defect because `SCR.WDTH8` changes this internal gating state.

## Explanation of the Phase-1 observations

The bus diagnostics and readbacks have more than one cause:

- PAPUEN is genuinely unmapped. PASEL occupies the other byte lane of the same aligned 16-bit bus word and is also unmapped for EZ (correctly reserved according to the manual). This explains the aligned `0xFFFFF402` unmapped messages with `0xFF00` for PAPUEN and `0x00FF` for the odd-byte PASEL access, and both zero readbacks.
- The external latch is genuinely absent from the AS3K driver map, explaining its unmapped `0x00600000` write.
- PDSEL is genuinely unmapped. The lack of an unmapped log line is not evidence of support; neither internal map contains `0xFFFFF41B`. Its zero debugger read is consistent with an open/unmapped byte, but source inspection alone does not establish why this particular bus lane emitted no diagnostic.
- PADATA readback of zero is explained by the handler: the EZ's internal `m_pasel` was zero at reset unless an earlier `SCR.WDTH8` write selected 8-bit Port A. `padata_r()` suppresses both stored output data and input callbacks wherever `m_pasel` is zero. Thus a watched write can store `m_padata = 0x7F` yet a later read can return zero. The prior trace did not record SCR or inspect `m_pasel`, so the exact selection history remains unproven.
- PADIR, PDDIR, and PDPUEN reads are direct returns of stored state and have no select, direction, callback, or input gating. Their Phase-1 writes should therefore have produced debugger reads `0x7F`, `0x00`, and `0x00`, respectively. PDDIR and PDPUEN happen to have been written to/read as zero, but PADIR's zero debugger read conflicts with the handler's expected retained `0x7F`. Static core source does not prove whether the prior debugger `b@` expressions used a view/space that bypassed or shadowed the CPU internal map, or whether another access changed state. “Debugger byte access behavior” is therefore unresolved rather than asserted.

A future narrow check, not executed here: on `asma3kdv`, stop immediately after the PADIR write and before any following instruction; compare (1) the debugger's explicit main-CPU program-space byte read, (2) a CPU-executed `MOVE.B 0xFFFFF400,Dn` observed at a temporary patched/controlled diagnostic point or debugger register/memory injection that preserves the program, and (3) GPIO log output from `padir_r()`. Also watch all subsequent writes to exact `0xFFFFF400` and SCR `0xFFFFF000`. Quit before Phase 2. This distinguishes handler state from debugger-space presentation without broad execution.

## Manual/source findings for the disputed registers

- **PASEL:** AlphaSmart's header says `0xFFFFF403`, select register; the EZ manual marks that byte reserved. EZ Port A selection is instead `SCR.WDTH8`: 0 connects D[7:0], 1 permits Port A GPIO in an 8-bit system. Reset selects the data bus. The original MC68328 PASEL handler is not an analogous EZ register and must not be mapped.
- **PAPUEN:** `0xFFFFF402`, eight independent read/write pull-up-enable bits; 1 enables, 0 disables; reset `0xFF`. There is no analogous Port-A state today. PDPUEN's simple state handlers and PDDATA fallback pattern can be adapted, but PADATA needs a Port-A pull-up fallback and the state must reset/save.
- **PDSEL:** `0xFFFFF41B`; only SEL7–SEL4 exist, reset `0xF0`; 1 selects GPIO and 0 selects the dedicated IRQ pin. PD3–PD0 are always GPIO-capable. Other base port-select handlers are structurally analogous for state/read/write, but correct EZ behavior needs a `0xF0` mask/default and mux-aware PD7–PD4 callback/data behavior. The current core is effectively hardwired GPIO for all eight bits.
- **PKBDINT/PDKBEN:** `0xFFFFF41E`, KBEN7–KBEN0, reset `0x00`, read/write enable bits. This is not a pending-bit clear or interrupt acknowledge register. A selected low external input contributes to the active-low keyboard OR interrupt (`INT_KB`, source bit 6 in MAME); the interrupt clears only when all selected sources deassert or their enable bits are cleared. AlphaSmart writes `0xFF` after driving every column low to enable wake-on-key and writes `0x00` in its handler/disable routine.

## Minimum implementation boundary and sequence

1. **First single stage — driver-only external latch.** Add an 8-bit saved latch in `alphasmart3k_state`, mapped at the board's `0x00600000` decoded location/window, with an explicit deterministic machine-reset value; Phase 1 will establish the operational `0xFF`. Validate only that the Phase-1 latch write is mapped/retained while `asma3kdi` and `asma3kdv` behavior remains intact. Do not add key mappings or run Phase 2 in that stage. The physical latch's power-on value was not established by this audit and should not be invented as a hardware claim.
2. **Core state/behavior addition — PAPUEN plus correct EZ Port-A selection/save.** Add `m_papuen` with reset `0xFF`, save-state, mapped read/write handlers, and input fallback. Preserve `SCR.WDTH8` as the only EZ selector; do not map `0x403`. Save the internal EZ Port-A selection state (or derive it reliably from saved SCR) and make SCR clearing behavior accurate. Validate independently.
3. **Driver-only matrix plumbing.** Connect PA0–PA6 output callbacks to seven saved/effective column levels; combine them with the eight latch bits; connect PD0–PD7 input callbacks to row evaluators. With no key mappings yet, each row must idle high from the board-level external pull-up model, because Phase 1 disables internal PD pull-ups. A row reads low when any currently-low selected column contains a pressed key. The structure must support multiple low columns, not assume exactly one.
4. **Core state/behavior addition — PDSEL.** Add saved `m_pdsel`, reset/read mask `0xF0`, handlers at `0x41B`, and gate PD7–PD4 GPIO callback/data behavior by SEL; PD3–PD0 remain GPIO regardless. Phase 1's `0xFF` then selects all available GPIO bits. This is fidelity-correct but not required merely for all eight AS3K row callbacks to function under the current hardwired model.
5. **Defer keyboard interrupt semantics.** Before executing `KeyboardEnableKeyboardInterrupt`, implement saved PDKBEN/PKBDINT state and its active-low OR into `INT_KB`, reevaluated when PDKBEN, PDDIR, or any PD input changes. At the same time audit/fix PDIRQEN/PDPOL/PDIQEG masking and the current individual-line logic rather than treating the new register as a storage-only byte. This can safely wait through Phase 2 because Phase 2 only installs the handler; it neither enables PDKBEN nor scans keys.

Direct answers:

1. Adding PASEL to the EZ map is not sufficient; it is incorrect because `0x403` is reserved. Correct Port-A work is PAPUEN plus faithful `SCR.WDTH8` gating/save behavior.
2. PAPUEN needs new state/handlers and PADATA input fallback. AS3K Phase 1 disables PA0–PA6 pull-ups and uses those pins as outputs, so it does not functionally depend on pull-ups beyond register fidelity in this stage.
3. PDSEL needs new state/handlers and mux behavior for PD7–PD4. Current MAME treats Port D as hardwired GPIO; PD3–PD0 really are always GPIO. AS3K's `0xFF` write selects GPIO for the implemented upper bits.
4. PKBDINT is PDKBEN: per-row membership in a level-sensitive, active-low keyboard OR interrupt. It can be deferred until immediately before `KeyboardEnableKeyboardInterrupt`; it is not needed for Phase 1 or Phase 2.
5. The `0x00600000` external component can be modeled entirely in the AS3K driver as a saved 8-bit write latch. Its outputs feed matrix column selection; no MC68EZ328 change belongs to that address.
6. The eventual driver matrix is 15 columns by 8 rows: latch bits are X1–X8, PA0–PA6 are X9–X15, and PD0–PD7 are row inputs. Column state and physical key state combine into eight active-low row results, with external idle-high behavior. Actual key/input-port mappings are a later driver task.

## Safe deferrals and next stage

Safe to defer: actual key mappings, Phase 2 execution, PDKBEN/keyboard interrupt generation, correction of individual Port-D interrupt semantics, PDSEL mux fidelity, and unrelated peripherals.

Proposed next single implementation/test stage: **driver-only saved external latch mapping**, followed by the same bounded Phase-1 diagnostic and existing `asma3kdi`/`asma3kdv` regression gates. Do not combine it with core changes, matrix inputs, Phase 2, or key mappings.

## Publication checks

- Final `git diff --check`: passed.
- Final tracked status before commit: ` M docs/as3k/CODEX_RESULT.md` and nothing else.
- Only `docs/as3k/CODEX_RESULT.md` is authorized for commit and push.
