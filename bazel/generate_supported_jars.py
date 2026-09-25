import argparse
import json
import pathlib


def parse_manifest(path: pathlib.Path) -> dict[str, tuple[int, str]]:
    result: dict[str, tuple[int, str]] = {}
    for number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split(maxsplit=2)
        if len(parts) != 3:
            raise ValueError(f"{path}:{number}: expected SHA-256, size and filename")
        digest, size_text, name = parts
        if len(digest) != 64:
            raise ValueError(f"{path}:{number}: invalid SHA-256")
        result[digest] = (int(size_text), name)
    return result


def cpp_string(value: str) -> str:
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--inventory", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    manifest_path = pathlib.Path(args.manifest)
    inventory_path = pathlib.Path(args.inventory)
    manifest = parse_manifest(manifest_path)
    inventory = json.loads(inventory_path.read_text(encoding="utf-8"))
    if inventory.get("format_version") != 1:
        raise ValueError("unsupported game JAR inventory format")

    rows: list[str] = []
    seen: set[str] = set()
    for jar in inventory.get("jars", []):
        digest = jar["archive_sha256"]
        if digest not in manifest:
            raise ValueError(f"inventory hash {digest} is absent from {manifest_path}")
        expected_size, expected_name = manifest[digest]
        if jar["archive_size"] != expected_size or jar["canonical_name"] != expected_name:
            raise ValueError(f"inventory metadata disagrees for {expected_name}")
        seen.add(digest)
        rows.append(
            "    {"
            + ", ".join(
                [
                    cpp_string(expected_name),
                    cpp_string(digest),
                    str(expected_size) + "ULL",
                    cpp_string(jar["content_tree_sha256"]),
                    str(jar["entry_count"]) + "U",
                ]
            )
            + "},"
        )
    missing = set(manifest) - seen
    if missing:
        raise ValueError("manifest hashes absent from inventory: " + ", ".join(sorted(missing)))

    output = """// Generated from validation metadata; do not edit.
#ifndef VOYAGE_SUPPORTED_GAME_JARS_HPP
#define VOYAGE_SUPPORTED_GAME_JARS_HPP

#include <cstddef>
#include <cstdint>

namespace voyage::install::generated {

struct SupportedJar {
    const char *canonical_name;
    const char *archive_sha256;
    uint64_t archive_size;
    const char *content_tree_sha256;
    uint32_t entry_count;
};

inline constexpr SupportedJar kSupportedJars[] = {
""" + "\n".join(rows) + """
};
inline constexpr size_t kSupportedJarCount = sizeof(kSupportedJars) / sizeof(kSupportedJars[0]);

}  // namespace voyage::install::generated

#endif  // VOYAGE_SUPPORTED_GAME_JARS_HPP
"""
    pathlib.Path(args.output).write_text(output, encoding="utf-8", newline="\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
