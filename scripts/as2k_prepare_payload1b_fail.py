#!/usr/bin/env python3
"""Generate the Payload-1b FAIL-display diagnostic image for MAME.

The executable portion is the frozen G0 Payload-0 image. Only the final
8-byte message field is changed from "BOOT OK\0" to "FAIL PB\0".
"""

from pathlib import Path
import argparse
import hashlib
import zlib

PAYLOAD0 = bytes.fromhex(
    "0F8E00C3143C20150040863C97098600B720008604B74000"
    "86038D408D5286038D3A86038D3686028D3286288D218601"
    "8D1D8D3C86068D17860C8D138602B72000CE00B7A6002705"
    "8D050820F720FE16444444448D0617840F8D013948489708"
    "8605B740008604B7400086604A26FD39CE08000926FD39"
    "424F4F54204F4B00"
)

EXPECTED_DONOR_SHA256 = "54582354a972848c642b2c87893ae2cc693483eae3d161693edf8f10d26f634a"
EXPECTED_FAIL_CRC32 = "e0e98168"
EXPECTED_FAIL_SHA1 = "ff3136f95aff8f74c77eb968ecb684af2905ce40"
EXPECTED_FAIL_SHA256 = "7469f23dbe03b3a35cfac66fc4be5e107d15498542a892f947b70c900cc288a"

assert len(PAYLOAD0) == 127
assert PAYLOAD0[-8:] == b"BOOT OK\x00"
assert hashlib.sha256(PAYLOAD0).hexdigest() == EXPECTED_DONOR_SHA256

FAIL = PAYLOAD0[:-8] + b"FAIL PB\x00"

assert len(FAIL) == 127
assert "%08x" % (zlib.crc32(FAIL) & 0xffffffff) == EXPECTED_FAIL_CRC32
assert hashlib.sha1(FAIL).hexdigest() == EXPECTED_FAIL_SHA1
assert hashlib.sha256(FAIL).hexdigest() == EXPECTED_FAIL_SHA256

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--romroot", default="roms")
    args = ap.parse_args()

    outdir = Path(args.romroot) / "asma2k1bf"
    outdir.mkdir(parents=True, exist_ok=True)
    path = outdir / "as2k_payload1b_fail.bin"
    path.write_bytes(FAIL)

    print("Prepared", path)
    print("size=127 CRC32=%s SHA1=%s SHA256=%s" % (
        EXPECTED_FAIL_CRC32, EXPECTED_FAIL_SHA1, EXPECTED_FAIL_SHA256
    ))
    print('message="FAIL PB"')

if __name__ == "__main__":
    main()
