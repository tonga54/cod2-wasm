#!/usr/bin/env python3
"""Align generated data-blob declarations with compiled WebAssembly symbols.

The reconstructed data blobs encode addresses as untyped pointers. On wasm,
function-table symbols and linear-memory symbols have different kinds. This
pass uses the already compiled object graph to correct generated extern
declarations before the client link. It deliberately does not guess
for names absent from the object graph or resolve names defined as both kinds.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path


DECLARATION = re.compile(
    r"^(?P<prefix>\s*extern\s+)(?:char|unsigned char|void)\s+"
    r"(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*(?P<shape>\[\]|\(void\))\s*;",
    re.MULTILINE,
)
FUNCTION_KINDS = set("TtWwIi")
OBJECT_KINDS = set("BbDdGgRrSsVvCc")
GENERATED_OBJECTS = {"data32.c.o", "literals32.c.o", "import_pointers_native.c.o"}
GLOBAL_INITIALIZER = re.compile(
    r"^(?P<indent>\s*)(?!static\b|extern\b)(?P<declaration>.*?)"
    r"\b(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*"
    r"(?:__attribute__\s*\(\([^\n]*?\)\)\s*)?=\s*",
    re.MULTILINE,
)


def symbol_kinds(nm: str, object_root: Path) -> dict[str, set[str]]:
    objects = sorted(object_root.rglob("*.o"))
    if not objects:
        raise RuntimeError(f"no compiled object files found under {object_root}")

    kinds: dict[str, set[str]] = {}
    objects = [obj for obj in objects if obj.name not in GENERATED_OBJECTS]
    # llvm-nm accepts multiple objects. Batch them to avoid hundreds of
    # compiler-tool startups, particularly costly in the amd64 Docker image.
    for offset in range(0, len(objects), 64):
        batch = objects[offset:offset + 64]
        result = subprocess.run(
            [nm, "--format=posix", "--extern-only", "--defined-only",
             *map(str, batch)],
            check=False,
            capture_output=True,
            text=True,
        )
        if result.returncode:
            detail = result.stderr.strip() or result.stdout.strip()
            raise RuntimeError(f"{nm} failed for object batch at {batch[0]}: {detail}")
        for line in result.stdout.splitlines():
            columns = line.split()
            if len(columns) < 2:
                continue
            # POSIX nm emits: name type value size. Ignore any non-symbol rows.
            symbol, kind = columns[0], columns[1]
            if len(kind) != 1:
                continue
            if kind in FUNCTION_KINDS:
                kinds.setdefault(symbol, set()).add("function")
            elif kind in OBJECT_KINDS:
                kinds.setdefault(symbol, set()).add("object")
    return kinds


def rewrite_source(path: Path, kinds: dict[str, set[str]]) -> tuple[str, int, list[str]]:
    original = path.read_text()
    ambiguous: list[str] = []
    changes = 0

    def replace(match: re.Match[str]) -> str:
        nonlocal changes
        name = match.group("name")
        found = kinds.get(name, set())
        if len(found) > 1:
            ambiguous.append(name)
            return match.group(0)
        if not found:
            return match.group(0)
        desired = "(void)" if "function" in found else "[]"
        if match.group("shape") == desired:
            return match.group(0)
        ctype = "void" if desired == "(void)" else "char"
        changes += 1
        return f"{match.group('prefix')}{ctype} {name}{desired};"

    updated = DECLARATION.sub(replace, original)
    if ambiguous:
        return original, 0, sorted(set(ambiguous))
    if path.name == "data32.c":
        updated, removed = remove_duplicate_initializers(updated, kinds)
        changes += removed
    return updated, changes, []


def remove_duplicate_initializers(source: str, kinds: dict[str, set[str]]) -> tuple[str, int]:
    """Prefer typed C definitions over duplicate copies in the generated blob."""
    removals: list[tuple[int, int]] = []
    for match in GLOBAL_INITIALIZER.finditer(source):
        if not kinds.get(match.group("name")):
            continue
        start = source.rfind("\n", 0, match.start()) + 1
        brace = source.find("{", match.end())
        semicolon = source.find(";", match.end())
        if semicolon < 0:
            raise RuntimeError(f"unterminated generated initializer for {match.group('name')}")
        if brace >= 0 and brace < semicolon:
            depth = 0
            end = brace
            while end < len(source):
                if source[end] == "{":
                    depth += 1
                elif source[end] == "}":
                    depth -= 1
                    if depth == 0:
                        semicolon = source.find(";", end)
                        if semicolon < 0:
                            raise RuntimeError(f"unterminated generated initializer for {match.group('name')}")
                        break
                end += 1
        removals.append((start, semicolon + 1))

    if not removals:
        return source, 0
    for start, end in reversed(removals):
        source = source[:start] + source[end:]
    return source, len(removals)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--nm", required=True, help="llvm-nm or emnm executable")
    parser.add_argument("--object-root", required=True, type=Path)
    parser.add_argument("sources", nargs="+", type=Path)
    args = parser.parse_args()

    try:
        kinds = symbol_kinds(args.nm, args.object_root)
        total = 0
        updates: list[tuple[Path, str]] = []
        for source in args.sources:
            updated, count, ambiguous = rewrite_source(source, kinds)
            if ambiguous:
                names = ", ".join(ambiguous[:12])
                suffix = " …" if len(ambiguous) > 12 else ""
                raise RuntimeError(
                    f"{source} has symbols defined as both functions and objects: {names}{suffix}"
                )
            total += count
            if count:
                updates.append((source, updated))
        for source, updated in updates:
            source.write_text(updated)
        print(f"Reclassified {total} generated symbol declaration(s) from {len(kinds)} object symbols.")
        return 0
    except (OSError, RuntimeError) as exc:
        print(f"wasm symbol classification failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
