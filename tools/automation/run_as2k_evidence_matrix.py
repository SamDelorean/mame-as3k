#!/usr/bin/env python3
"""Standalone AS2000 evidence-matrix planner/runner.

This tool is deliberately NOT wired into run_as2k_emulator_cycle.sh yet.
It converts the evidence matrix into deterministic local work and refuses to
turn missing harnesses or external evidence into emulator failures.

A validator becomes executable by adding a command in a local JSON registry:
  $XDG_CONFIG_HOME/as2k-emulator/evidence-harness.json
or:
  ~/.config/as2k-emulator/evidence-harness.json

Example registry entry:
{
  "BOOT_EDITOR": ["/path/to/test_boot_editor.sh"],
  "LCD_THREE_LINE": ["python3", "/path/to/test_lcd.py"]
}

Validator command contract:
  exit 0  -> VALIDATED
  exit 10 -> reproducible mismatch against expected behavior
  exit 20 -> BLOCKED infrastructure/private-input problem
  exit 30 -> HARNESS_REQUIRED / observation unavailable
  other   -> BLOCKED (unexpected harness failure)

The runner never invokes Codex.
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Any

VALIDATED = "VALIDATED"
REPAIR_REQUIRED = "REPAIR_REQUIRED"
EVIDENCE_REQUIRED = "EVIDENCE_REQUIRED"
HARNESS_REQUIRED = "HARNESS_REQUIRED"
BLOCKED = "BLOCKED"


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8") as fh:
        return json.load(fh)


def repo_root() -> Path:
    here = Path(__file__).resolve()
    return here.parents[2]


def default_registry() -> Path:
    base = Path(os.environ.get("XDG_CONFIG_HOME", Path.home() / ".config"))
    return base / "as2k-emulator" / "evidence-harness.json"


def classify_unavailable(test: dict[str, Any]) -> str:
    automation = test.get("automation")
    if automation == "external":
        return EVIDENCE_REQUIRED
    return test.get("unavailable", HARNESS_REQUIRED)


def load_registry(path: Path) -> dict[str, list[str]]:
    if not path.exists():
        return {}
    data = load_json(path)
    if not isinstance(data, dict):
        raise ValueError("harness registry must be a JSON object")
    result: dict[str, list[str]] = {}
    for key, value in data.items():
        if not isinstance(key, str) or not isinstance(value, list) or not value:
            raise ValueError(f"invalid harness registry entry: {key!r}")
        if not all(isinstance(part, str) and part for part in value):
            raise ValueError(f"invalid command for harness: {key}")
        result[key] = value
    return result


def run_harness(command: list[str], timeout: int) -> tuple[int, str]:
    try:
        proc = subprocess.run(
            command,
            cwd=repo_root(),
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            timeout=timeout,
            check=False,
        )
        output = proc.stdout[-4000:].strip()
        return proc.returncode, output
    except subprocess.TimeoutExpired as exc:
        out = exc.stdout or ""
        if isinstance(out, bytes):
            out = out.decode(errors="replace")
        return 20, (str(out)[-4000:] + "\nTIMEOUT").strip()
    except OSError as exc:
        return 20, f"EXEC_ERROR {exc}"


def result_from_rc(test: dict[str, Any], rc: int) -> str:
    if rc == 0:
        return test.get("on_pass", VALIDATED)
    if rc == 10:
        return test.get("on_fail", REPAIR_REQUIRED)
    if rc == 20:
        return BLOCKED
    if rc == 30:
        return classify_unavailable(test)
    return BLOCKED


def dependency_state(test: dict[str, Any], results: dict[str, dict[str, Any]]) -> str | None:
    for dep in test.get("depends", []):
        observed = results.get(dep, {}).get("outcome")
        if observed != VALIDATED:
            return dep
    return None


def main() -> int:
    root = repo_root()
    parser = argparse.ArgumentParser()
    parser.add_argument("--matrix", type=Path, default=root / "docs/as2k/EMULATOR_EVIDENCE_MATRIX.json")
    parser.add_argument("--registry", type=Path, default=default_registry())
    parser.add_argument("--state", type=Path, default=Path(os.environ.get("XDG_STATE_HOME", Path.home() / ".local/state")) / "as2k-emulator-automation" / "evidence-state.json")
    parser.add_argument("--timeout", type=int, default=120)
    parser.add_argument("--test", help="Run/inspect only one matrix test ID")
    parser.add_argument("--plan", action="store_true", help="Print classifications without executing harness commands")
    args = parser.parse_args()

    matrix = load_json(args.matrix)
    if matrix.get("schema") != 2:
        print(f"BLOCKED unsupported_matrix_schema={matrix.get('schema')}")
        return 20

    try:
        registry = load_registry(args.registry)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"BLOCKED registry={exc}")
        return 20

    tests = sorted(matrix.get("tests", []), key=lambda item: item.get("order", 0))
    if args.test:
        tests = [test for test in tests if test.get("id") == args.test]
        if not tests:
            print(f"BLOCKED unknown_test={args.test}")
            return 20

    results: dict[str, dict[str, Any]] = {}
    if args.state.exists() and not args.plan:
        try:
            previous = load_json(args.state)
            if isinstance(previous, dict) and previous.get("schema") == 1:
                # Previous results are informational only. Dependencies are validated
                # by this campaign, avoiding stale PASS decisions after code changes.
                pass
        except (OSError, json.JSONDecodeError):
            pass

    worst = 0
    for test in tests:
        tid = test["id"]
        automation = test.get("automation", "needs_harness")
        confidence = test.get("confidence", "hypothesis")

        if args.plan:
            outcome = EVIDENCE_REQUIRED if automation == "external" else ("EXECUTABLE" if tid in registry else HARNESS_REQUIRED)
            print(f"{tid}\tautomation={automation}\tconfidence={confidence}\tstatus={outcome}")
            continue

        blocked_dep = dependency_state(test, results)
        if blocked_dep:
            outcome = BLOCKED
            detail = f"dependency_not_validated={blocked_dep}"
        elif automation == "external":
            outcome = EVIDENCE_REQUIRED
            detail = test.get("evidence_request", "external evidence required")
        elif test.get("validator") == "derived_from_dependency":
            outcome = VALIDATED
            detail = "derived_from_validated_dependency"
        elif tid not in registry:
            outcome = classify_unavailable(test)
            detail = "no_local_harness_registered"
        else:
            rc, detail = run_harness(registry[tid], args.timeout)
            outcome = result_from_rc(test, rc)

        results[tid] = {
            "outcome": outcome,
            "automation": automation,
            "confidence": confidence,
            "detail": detail,
        }
        print(f"{tid}\t{outcome}\t{detail}")

        if outcome == REPAIR_REQUIRED:
            worst = max(worst, 10)
        elif outcome == BLOCKED:
            worst = max(worst, 20)
        elif outcome == HARNESS_REQUIRED:
            worst = max(worst, 30)

    if not args.plan:
        args.state.parent.mkdir(parents=True, exist_ok=True)
        payload = {
            "schema": 1,
            "matrix_schema": matrix.get("schema"),
            "machine": matrix.get("machine"),
            "results": results,
        }
        args.state.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    return worst


if __name__ == "__main__":
    sys.exit(main())
