from __future__ import annotations

import argparse
import filecmp
from pathlib import Path, PurePosixPath
import shutil


def _destination(root: Path, relative: str) -> Path:
    path = PurePosixPath(relative)
    if path.is_absolute() or not path.parts or ".." in path.parts:
        raise ValueError(f"unsafe package path: {relative}")
    return root.joinpath(*path.parts)


def _copy(source: Path, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        if not filecmp.cmp(source, destination, shallow=False):
            raise ValueError(f"two different files target {destination}")
        return
    shutil.copyfile(source, destination)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    parser.add_argument("--root-file", action="append", default=[], type=Path)
    parser.add_argument("--named-file", action="append", default=[])
    args = parser.parse_args()

    copies: list[tuple[str, Path]] = [
        (source.name, source) for source in args.root_file
    ]
    for specification in args.named_file:
        relative, separator, source = specification.partition("=")
        if not separator:
            parser.error(f"invalid --named-file value: {specification}")
        copies.append((relative, Path(source)))

    args.output.mkdir(parents=True, exist_ok=True)
    try:
        for relative, source in sorted(copies, key=lambda item: (item[0], str(item[1]))):
            if not source.is_file():
                raise ValueError(f"package input is not a file: {source}")
            _copy(source, _destination(args.output, relative))
    except ValueError as error:
        parser.error(str(error))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
