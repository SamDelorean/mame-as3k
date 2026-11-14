#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
"""Start the ONE AS3K-DIAG emulator with its native serial printer.
Save genuine RS-232 PCL jobs and forward completed ones to the proven
AS2K-derived CUPS/PDF adapter. No PC, printer firmware or ROM emulation.
Example:
 python3 scripts/as3k_print_run.py --output-directory ./impressions -- \
   asma3ksw -rompath /private/roms -video none
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parent.parent
MAME = ROOT / "alphasma3k"
CONVERTER = ROOT / "scripts" / "as3k_print_to_host.py"
PCL_START = b"\x1bE\x1b&k3G"
PCL_END = b"\x1b&l0H\x1bE"
MAX_BYTES = 200_000


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-directory", type=Path, required=True)
    parser.add_argument("mame_args", nargs=argparse.REMAINDER,
                        help="MAME ROM and options, following --")
    args = parser.parse_args()
    forwarded = list(args.mame_args)
    if forwarded and forwarded[0] == "--":
        forwarded.pop(0)
    if not forwarded:
        parser.error("Provide the ROM name and its arguments after --")
    if any(arg.lower() in ("-printout", "-rs232") for arg in forwarded):
        parser.error("-rs232 and -printout are owned by this native print bridge")

    base_output = args.output_directory.resolve()
    base_output.mkdir(parents=True, exist_ok=True)
    # Keep every print session, including repeated launches to the same folder.
    # Do not overwrite an earlier PCL job, PDF evidence or setup script.
    output = base_output / f"print-{time.strftime('%Y%m%d-%H%M%S')}-{os.getpid()}"
    output.mkdir(exist_ok=False)
    printer_raw = output / "as3k-serial-printer-stream.pcl"
    if printer_raw.exists():
        parser.error(f"Refusing to truncate an existing printer stream: {printer_raw}")
    if not MAME.is_file() or not CONVERTER.is_file():
        parser.error("Canonical ./alphasma3k binary or AS2K-derived converter missing")

    # AS3000 native HP printer output is 57600 baud, while MAME's generic
    # serial printer defaults to 9600. Configure only THIS emulated port
    # before the ROM issues Print, without altering global MAME devices.
    user_autoboot = None
    for i, value in enumerate(forwarded):
        if value.lower() == "-autoboot_script":
            if i + 1 >= len(forwarded):
                parser.error("-autoboot_script is missing a filename")
            user_autoboot = Path(forwarded[i + 1]).resolve()
            del forwarded[i:i + 2]
            break
    if user_autoboot is not None and not user_autoboot.is_file():
        parser.error(f"Original autoboot script not found: {user_autoboot}")
    setup_script = output / "printer-57600-setup.lua"
    if setup_script.exists():
        parser.error(f"Refusing to overwrite an existing setup script: {setup_script}")
    # The original user autoboot, when supplied, is executed unchanged.
    setup_script.write_text("""
local printer_baud_configured = false
emu.register_periodic(function()
    if printer_baud_configured then return end
    local f = manager.machine.ioport.ports[":rs232:printer:RS232_RXBAUD"]:field(1)
    for value, label in pairs(f.settings) do
        if label == "57600" then
            f.user_value = value
            printer_baud_configured = true
            manager.machine:logerror("AS3K_NATIVE_PRINTER_BAUD=57600")
            return
        end
    end
    error("AS3K native printer port does not offer 57600 baud")
end)
""" + (f"\ndofile({json.dumps(str(user_autoboot))})\n"
       if user_autoboot else ""))
    cmd = [str(MAME), *forwarded, "-autoboot_script", str(setup_script),
           "-rs232", "printer", "-printout", str(printer_raw)]
    (output / "launch.json").write_text(json.dumps(cmd, indent=2) + "\n")
    print("AS3K_PRINT_BRIDGE_START printer=serial output=" + str(output), flush=True)
    position = 0
    number = 0
    errors = 0
    with subprocess.Popen(cmd, cwd=ROOT, env=os.environ.copy()) as machine:
        while True:
            if printer_raw.is_file():
                content = printer_raw.read_bytes()
                if len(content) < position:
                    print("AS3K_PRINT_BRIDGE_FAIL printer stream unexpectedly truncated",
                          file=sys.stderr, flush=True)
                    errors += 1
                    break
                while len(content) >= position + len(PCL_START):
                    if content[position:position + len(PCL_START)] != PCL_START:
                        print(f"AS3K_PRINT_BRIDGE_FAIL unexpected data at byte {position}",
                              file=sys.stderr, flush=True)
                        errors += 1
                        position = len(content)
                        break
                    ending = content.find(PCL_END, position + len(PCL_START))
                    if ending == -1:
                        if len(content) - position > MAX_BYTES:
                            print("AS3K_PRINT_BRIDGE_FAIL oversized incomplete PCL",
                                  file=sys.stderr, flush=True)
                            errors += 1
                            position = len(content)
                        break
                    end = ending + len(PCL_END)
                    if end - position > MAX_BYTES:
                        print("AS3K_PRINT_BRIDGE_FAIL oversized PCL job",
                              file=sys.stderr, flush=True)
                        errors += 1
                        position = end
                        continue
                    number += 1
                    path = output / f"as3k-printer-pcl-{number:04d}.bin"
                    if path.exists():
                        raise RuntimeError(f"Refusing to overwrite {path}")
                    path.write_bytes(content[position:end])
                    position = end
                    result = subprocess.run(
                        [sys.executable, str(CONVERTER), str(path)],
                        capture_output=True, text=True, timeout=25,
                        check=False)
                    if result.returncode:
                        errors += 1
                    print(f"AS3K_PRINT_BRIDGE_JOB {number} bytes={path.stat().st_size} "
                          f"status={'PASS' if result.returncode == 0 else 'FAIL'} "
                          f"{(result.stdout or result.stderr).strip()}",
                          flush=True)
            if machine.poll() is not None:
                break
            time.sleep(0.1)
        rc = machine.wait()
    if printer_raw.exists() and printer_raw.stat().st_size > position:
        errors += 1
        print(f"AS3K_PRINT_BRIDGE_FAIL incomplete trailer "
              f"bytes={printer_raw.stat().st_size-position}", file=sys.stderr)
    print(f"AS3K_PRINT_BRIDGE_END emulator_exit={rc} jobs={number} errors={errors}",
          flush=True)
    return rc if rc else (2 if errors else 0)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as exc:
        print(f"AS3K_PRINT_BRIDGE_FAIL {type(exc).__name__}: {exc}",
              file=sys.stderr)
        raise SystemExit(2)
