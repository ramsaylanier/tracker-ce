#!/usr/bin/env python3
"""Fail if a feature module includes headers outside the Module SDK allowlist."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<"]([^">]+)[>"]')

# Allowed include prefixes / exact names for feature-modules sources.
ALLOWED_EXACT = {
    "FeatureModule.h",
    "ModuleSidecar.h",
    "SampleInstrumentFxQuery.h",
    "Platform.h",
    "cr_section_macros.h",
    "polyassert.h",
}

ALLOWED_PREFIXES = (
    "sdk/",
)

ALLOWED_SYSTEM = {
    "stdint.h",
    "stddef.h",
    "stdio.h",
    "cstring",
    "cstdio",
    "cstdlib",
    "cmath",
    "algorithm",
    "optional",
    "span",
    "string",
    "string.h",
    "memory",
    "new",
    "utility",
    "type_traits",
}


def is_allowed(header: str) -> bool:
    if header in ALLOWED_EXACT or header in ALLOWED_SYSTEM:
        return True
    if any(header.startswith(p) for p in ALLOWED_PREFIXES):
        return True
    # Same-directory module headers (MyModule.h)
    if "/" not in header and header.endswith((".h", ".hpp")):
        return True
    return False


def check_file(path: Path) -> list[str]:
    errors: list[str] = []
    for lineno, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
        match = INCLUDE_RE.match(line)
        if not match:
            continue
        header = match.group(1)
        if not is_allowed(header):
            errors.append(f"{path}:{lineno}: disallowed include '{header}'")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "roots",
        nargs="*",
        default=["feature-modules"],
        help="Directories to scan (default: feature-modules)",
    )
    args = parser.parse_args()

    errors: list[str] = []
    for root in args.roots:
        root_path = Path(root)
        if not root_path.exists():
            continue
        for path in root_path.rglob("*"):
            if path.suffix not in {".h", ".hpp", ".cpp", ".cc", ".cxx"}:
                continue
            # Skip READMEs etc.; only sources.
            errors.extend(check_file(path))

    if errors:
        print("Module include check failed:", file=sys.stderr)
        for err in errors:
            print(f"  {err}", file=sys.stderr)
        print(
            "\nModules may only include SDK headers (FeatureModule.h / sdk/*), "
            "ModuleSidecar.h, SampleInstrumentFxQuery.h, Platform.h, "
            "cr_section_macros.h, and standard C/C++ headers.",
            file=sys.stderr,
        )
        return 1

    print("Module include check passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
