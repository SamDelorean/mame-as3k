#!/usr/bin/env python3
# license:BSD-3-Clause
"""AlphaSmart 3000: observed original PCL text -> CUPS AlphaSmart_PDF.

Adapted from the proven AS2K as3k_print_to_host.py receiver. AS3K uses
PC-850 symbol set ESC(12U; this minimal adapter accepts its ASCII subset.
All printable text comes exclusively from the captured printer wire bytes.

No PDF export or printer-specific driver. Unsupported/incomplete printer
language is rejected; raw spool bytes are retained by the MAME driver.
"""
from __future__ import annotations
import pathlib
import subprocess
import sys

PCL_COMMANDS = (
    b"\x1b&k3G", b"\x1b&s0C", b"\x1b&a08L", b"\x1b&a70M",
    b"\x1b&l05E", b"\x1b&l3D", b"\x1b&l0H", b"\x1b(8U", b"\x1b(12U", b"\x1bE",
)


def parse_observed_hp_text(raw: bytes) -> list[list[str]]:
    if not raw.startswith(b"\x1bE\x1b&k3G"):
        raise ValueError("Not the verified HP/PCL-text profile; keep raw trace")
    pages: list[list[str]] = [[]]
    line: list[str] = []
    cr_to_lf = False
    saw_text = False
    saw_reset = False
    i = 0
    def end_line(force: bool = False) -> None:
        if line or force:
            pages[-1].append("".join(line))
            line.clear()
    while i < len(raw):
        if raw[i] == 0x1b:
            cmd = next((c for c in PCL_COMMANDS if raw.startswith(c, i)), None)
            if cmd is None:
                raise ValueError(f"Unsupported PCL escape at decoded offset {i}")
            if cmd == b"\x1b&k3G":
                cr_to_lf = True
            if cmd == b"\x1bE":
                saw_reset = True
            if cmd == b"\x1b&l0H":
                end_line()
            i += len(cmd)
            continue
        b = raw[i]
        i += 1
        if 0x20 <= b <= 0x7e:
            line.append(chr(b))
            saw_text = True
        elif b == 0x09:
            line.extend(" " * (8 - len(line) % 8))
        elif b == 0x0d:
            if cr_to_lf:
                end_line(True)
            else:
                line.clear()
        elif b == 0x0a:
            end_line(True)
        elif b == 0x0c:
            end_line()
            if pages[-1]:
                pages.append([])
        else:
            raise ValueError(f"Unsupported PCL control or non-ASCII byte 0x{b:02x} at {i-1}")
        if len(line) > 120 or sum(map(len, pages[-1])) > 3000:
            raise ValueError("Unsupported width/height for minimal text-only renderer")
    end_line()
    if not saw_text or not saw_reset or not any(page for page in pages):
        raise ValueError("No complete printable HP/PCL-text content")
    return [page for page in pages if page]



def main() -> int:
    if len(sys.argv) != 2:
        print("Usage: as3k_print_to_host.py /path/as3k-printer-pcl-<id>.bin",file=sys.stderr)
        return 2
    path=pathlib.Path(sys.argv[1])
    try:
        if not path.is_file() or path.stat().st_size>200_000:
            raise ValueError("Missing or overlong PCL job")
        raw=path.read_bytes()
        if not raw.startswith(b"\x1bE\x1b&k3G") or not raw.endswith(b"\x1b&l0H\x1bE"):
            raise ValueError("Incomplete or unsupported HP/PCL job")
        pages=parse_observed_hp_text(raw)
        payload="\f".join("\n".join(lines)+"\n" for lines in pages).encode("ascii")
        if len(payload)>200_000:
            raise ValueError("Printer text exceeds 200000 bytes")
        result=subprocess.run(["lp","-d","AlphaSmart_PDF","-t","AlphaSmart 3000 R11"],input=payload,
                              capture_output=True,timeout=15,check=False)
    except (OSError,ValueError,UnicodeError,subprocess.TimeoutExpired) as exc:
        print(f"AS3K_PRINT_HOST REJECTED {exc}: raw retained {path}",file=sys.stderr)
        return 2
    if result.returncode:
        msg=(result.stderr or result.stdout).decode("utf-8","replace").strip()[:300]
        print(f"AS3K_PRINT_HOST CUPS_REJECTED rc={result.returncode}: {msg}; raw {path}",
              file=sys.stderr)
        return 3
    print("AS3K_PRINT_HOST SUBMITTED "+
          result.stdout.decode("utf-8","replace").strip()[:250],flush=True)
    return 0

if __name__=="__main__":
    sys.exit(main())
