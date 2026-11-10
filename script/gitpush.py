#!/usr/bin/env python3

from pathlib import Path
import subprocess
import sys


def get_arg(i, default=None, cast=str):
    try:
        return cast(sys.argv[i])
    except (IndexError, ValueError):
        return default


path = get_arg(1)
comment = get_arg(2)


def run_git(directory, *args, check=True):
    return subprocess.run(["git", *args], cwd=directory, check=check)


def main():
    if path is None or comment is None or not comment.strip():
        print('使い方: python3 gitpush.py <path> "コメント内容"')
        return 1

    directory = Path(path).expanduser().resolve()
    if not directory.is_dir():
        print(f"ディレクトリが存在しません: {directory}")
        return 1

    committed = False
    try:
        run_git(directory, "pull", "--ff-only")
        run_git(directory, "add", "-A", "--", ".")

        result = run_git(directory, "diff", "--cached", "--quiet", "--", ".", check=False)
        if result.returncode == 0:
            return 0
        if result.returncode != 1:
            raise subprocess.CalledProcessError(result.returncode, result.args)

        run_git(directory, "commit", "--only", "-m", comment, "--", ".")
        committed = True
        run_git(directory, "push")
    except (subprocess.CalledProcessError, OSError) as error:
        print(f"Gitの処理に失敗しました: {error}", file=sys.stderr)
        if committed:
            print(
                file=sys.stderr,
            )
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
