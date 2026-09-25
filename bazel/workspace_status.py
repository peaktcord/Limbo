import subprocess

FALLBACK = "unknown"


def git(*args: str) -> subprocess.CompletedProcess:
    return subprocess.run(
        ("git",) + args,
        capture_output=True,
        text=True,
        check=False,
    )


def revision() -> str:
    try:
        result = git("rev-parse", "--short", "HEAD")
    except OSError:
        return FALLBACK
    if result.returncode != 0:
        return FALLBACK
    revision = result.stdout.strip()
    if not revision:
        return FALLBACK

    try:
        dirty = git("diff", "--quiet", "HEAD")
    except OSError:
        return revision
    if dirty.returncode == 1:
        revision += "-dirty"
    return revision


def main() -> int:
    print("STABLE_VOYAGE_COMMIT %s" % revision())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
