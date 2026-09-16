# AS2K Emulator Automation Inbox

This file is the Git control inbox for the unattended AS2K emulator worker.

## Current directive
Continue the active task in `EMULATOR_AUTOMATION_TASK.md` under the constraints in `EMULATOR_AUTOMATION_CONTROL.md`.

Newer explicit directives added here override older task wording only where they directly conflict. They do not override `AGENTS.md` safety/proprietary boundaries unless the human operator explicitly changes those rules.

## Operator notes
- Cadence: 10 minutes after each completed cycle.
- Maximum build parallelism on the t640: `-j3`.
- Keep this worker independent from the Gate 1A firmware worker.
- Prefer productive implementation/test cycles over documentation-only cycles.
