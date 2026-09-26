# AS2K IR emulation — technical continuity dossier
Updated: 2026-09-26
Purpose: preserve enough verified technical context to resume and potentially complete the AlphaSmart 2000 IR emulation later without reconstructing the investigation.

## 1. Current purpose and scope
The IR implementation was developed primarily as dynamic instrumentation for Gate 1A/1B, not as an end-user complete IrDA implementation. Its present job is to stimulate real stock-ROM IR paths faithfully enough to observe PCs, callbacks, IRQ/PAI activity, protocol state and relevant RAM while avoiding forced PC/RAM/firmware state.

The implementation is nevertheless mature enough to demonstrate three end-to-end application paths:
- Send IR -> peer/text: FUNCTIONAL_COMPLETE.
- Print IR -> peer -> PCL/PDF: FUNCTIONAL_COMPLETE.
- Receive File peer -> AlphaSmart: PROVEN_AND_EXECUTABLE_IN_DIAG.

Future work may promote this instrumentation into an officially complete emulator feature. The evidence and boundaries below must be retained for that possibility.

## 2. Platform and implementation location
Primary source tree: /home/spc/Projects/alphasmart/mame-as3k
Current diagnostic target: as2kdiag
Relevant driver: src/mame/skeleton/alphasma.cpp
Current observed repository HEAD at preservation time: refs/heads/as2k-send-publication.
Do not use scripts/as2k/codex-next.sh in this checkout while it requires branch as2k-mame0289-dev unless repository context is explicitly changed/authorized.

Stock ROM must remain unmodified. The verified stock CODE image used by Gate analysis is:
private/validation/as2k-wired-print-prep-20260922/AS2K-v314-CODE-8000-FFFF.PRIVATE.bin
SHA-256 efdf350cb67e4fe5023312a259c220320ad4ca5b24c1c884cff01bfc7a080beb.

## 3. Low-level ROM map relevant to IR
D099: low-level IR/timer initialization.
D0E0: RX input-capture ISR / 8-bit sampler -> circular queue 05A9..05B8.
D166: RX queue drain/dispatch.
D194-D1E9: TX output-compare bit engine.
D1EA: queue one TX byte in B.
D1F6: TX setup.
D220: wait for TX idle/completion.
D2DC: Send-key IR entry.
D3F3: optical presence/probe using PA7.
E625-E76F: framed link TX / stuffing / checksum / timeout handling.
E770-E7C5: IR link/session continuation and state-management helpers; see E770_E7C5_OWNERSHIP_RESOLUTION_20260926.md.
E7C6+: higher-level protocol/session state.
FDB0-FDF0: checksum/CRC-like transform.

The DIAG peer must use the existing emulated physical PA7/PAI path. Do not replace this with direct writes into ROM-owned queues or state if faithful execution is required.

## 4. Verified Receive File protocol/application path
Static and dynamic interpretation:
- BC87 -> E468: receive-file setup, sets $0489=1.
- $0489 is proven receive-side state.
- E1FB: Receiving File UI branch while receive state is active.
- E33E: consumes received application bytes.
- E33E calls 9F74 per character; 9F74..9F87 is live editor insertion/storage code.
- 0xBB is the stock application terminator observed for the tested transfer.
- BC92 -> E4AD: closes the session and sets $0599=1.
- ROM returns normally to editor at $87D7 / normal route.
- BC90 -> E484 is NOT receive-start; it is associated with $0124-$0128 -> $0501.
- BC84 -> E492 is send-side setup and sets $048A=1.
- $048A is send-side state.

Verified DIAG Receive File sequence:
1. Normal Send action enters stock $9716 -> D2DC.
2. Normal IrDA setup occurs.
3. Peer waits for/uses the observed AlphaSmart IR v1.0 exchange and sends BC87.
4. ROM responds FB.
5. Peer sends payload RX IR TEST\rSECOND LINE followed by stock terminator 0xBB.
6. ROM responds FB and deactivates $0489.
7. Peer sends BC92.
8. ROM returns to editor through the stock path.

No emulator forcing of PC, RAM or firmware-owned protocol state is permitted in this proof.

## 5. Verified RX physical/runtime route
Dynamic evidence during the complete receive session reached:
D0E0 — RX timed/input-capture start/sampling.
D107 — bit reconstruction.
D166 — RX queue drain.
E58C — stack/delivery path.
FD22-FD77 — receiver/callback loader/path.
E33E — application receive callback.
E468 — BC87 / receive-state setup.
E1FB — Receiving File branch.
9F74 — per-byte editor insertion/storage.
E4AD — BC92 close.
9719 — normal return path.

FD22-FD77 invokes a callback stored at $0586 according to the preserved Run 6 analysis.
FF02-FF18 is a protected indirect dispatch helper ending in JMP 0,IX.

## 6. Verified receive result
The end-to-end test stored the supplied payload in NVRAM:
RX IR TEST at NVRAM offset 0x069D.
SECOND LINE at NVRAM offset 0x06A8, with an auxiliary occurrence at 0x04C6.
The firmware completed the transfer and returned normally.

The complete RX trace contains exactly 2382 unique executed PCs.

## 7. Send and Print state
Send IR -> text and Print IR -> PCL/PDF were completed in the preceding runs 1-3 and independently audited before the RX block. Do not repeat these tests merely to prove already established functionality. They remain available as regression paths and as dynamic stimulation when a specific Gate hypothesis requires them.

The current project policy is to preserve those working paths rather than expand protocol breadth without a concrete reason.

## 8. Gate instrumentation produced by IR
The RX trace directly executed ten historical UNDEFINED intervals:
9F74-9F87
E33E-E4CF
E770-E7C5
E8F1-E8FB
EB43-F2FE
F34E-F3C9
F80A-FAD0
FB34-FD81
FE9A-FED8
FF02-FF18

This evidence belongs to Gate classification, not to the feature count of the IR emulator. Gate has subsequently contextualized/protected all ten intervals. The IR subsystem's value here is that it generated faithful dynamic execution evidence.

## 9. Instrumentation/evidence artifacts
RUN4_RX_COVERAGE_20260926.md — complete Receive File experiment and coverage summary.
run4-rx-bc87-round1/ — first bounded BC87 verification.
run4-rx-bc87-round2/ — second bounded BC87/full transfer verification.
run4-rx-trace/coverage-pcs.txt — 2382 unique PCs.
run4-rx-trace/coverage-vs-unclassified.csv — dynamic coverage against historical undefined ranges.
run4-rx-trace/error.log — detailed execution/coverage log.
RUN5_GATE1B_RX_OWNERSHIP_20260926.md — Gate use of IR-derived evidence.
RUN6_RX_DYNAMIC_AUDIT_20260926.md — audit/consolidation.
AUDIT_RUNS4_6_20260926.md — independent closure audit.
E770_E7C5_OWNERSHIP_RESOLUTION_20260926.md — later exact static ownership resolution.
IR_EMULATION_STATE.md — concise current IR state.
GATE1AB_STATE.md — separate Gate state.

## 10. Design rules established by the investigation
- Preserve stock ROM behavior; no patched ROM is required for IR instrumentation.
- Prefer real peripheral/interrupt paths over synthetic firmware-state injection.
- Do not force PC, RAM or ROM-owned state to make a test pass.
- Keep the peer minimal and faithful to observed protocol behavior.
- At most two reasonable verification rounds per new hypothesis before documenting a block and moving on.
- Reuse proven Send/Print/Receive paths for regression or targeted coverage; avoid blind repetition.
- Treat protocol interpretation as evidence-driven. Do not infer command meaning solely from address adjacency.
- Separate emulator functionality from Gate conclusions.

## 11. Current limitations / intentionally unfinished areas
The peer/DIAG implementation is not claimed to be a complete generic IrDA stack.
Not every possible AlphaSmart IR command, session state, error path, timeout, retry, discovery scenario or peer implementation has been catalogued/exercised.
Coverage completeness of E7C6+ higher-level state machinery has not been defined as a product requirement.
Interoperability with arbitrary real IrDA devices is not established by the DIAG tests.
The current test peer is engineering instrumentation first; product-quality UI/configuration, generalized peer abstraction and comprehensive automated regression are outside the completed scope.

These are not defects in the Gate instrumentation objective; they are the likely work items if official IR emulation is resumed.

## 12. Suggested path if official/full IR emulation is resumed
Phase A — freeze/reproduce baseline:
- Re-run the existing Send, Print and Receive regression suite against the same stock ROM.
- Preserve expected protocol transcripts, NVRAM result and coverage signatures.
- Convert currently manual/diagnostic assertions into automated regression checks.

Phase B — protocol inventory:
- Enumerate all higher-level BC commands and state transitions from E7C6+ using static ROM analysis plus targeted traces.
- Build a command/state table with direction, preconditions, responses, timeout/retry behavior and application owner.
- Explicitly map discovery/handshake, connection, data, close/error and callback paths.

Phase C — faithful peer model:
- Refactor the minimal DIAG peer into a stateful peer abstraction while retaining the existing PA7/PAI physical route.
- Keep timing/queue behavior faithful enough that the stock ROM continues to drive its own state.
- Add negative/error cases without synthetic state injection.

Phase D — completeness/interoperability:
- Exercise all known command/state branches.
- Compare with captures or real hardware where available.
- Decide whether the target is AlphaSmart-to-AlphaSmart compatibility, printer compatibility, generic IrDA interoperability, or all three; these are different completion criteria.

Phase E — upstream/product quality:
- Separate diagnostic tracing from normal emulation behavior.
- Add deterministic automated tests and documentation.
- Review code structure, timing assumptions and MAME integration conventions before any publication/upstream proposal.

## 13. Resume checkpoint
For Gate work: IR remains an available dynamic instrument. Gate should first identify a specific unresolved ownership/consumer hypothesis; only then should IR instrumentation be extended to stimulate that path.

For future official IR work: begin from this dossier plus IR_EMULATION_STATE.md and the preserved Run 1-4 artifacts. Do not restart protocol discovery from scratch.