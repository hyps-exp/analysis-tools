#!/usr/bin/env python3
"""Print a greeting (template for the scripts in script/).

Copy this file to start a new script. Replace this text with what the
script does, the files it reads and writes, and the environment
variables it uses.

Coding notes:
    Indentation: 4 spaces.
    Arguments: a name without a leading '-' is a positional argument
    and is required; a name with a leading '-' is an option and can be
    omitted.

Examples:
    python3 script/sample.py World
    python3 script/sample.py HYPS --repeat 3
    python3 script/sample.py World --dry-run
"""

import argparse
import sys
import time
from datetime import datetime
from typing import Optional


# _____________________________________________________________________________
def parse_args(argv: Optional[list[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    # required
    parser.add_argument('name',
                        help='name to greet')
    # optional
    parser.add_argument('-n', '--repeat', type=int, default=1,
                        help='number of greetings')
    parser.add_argument('--dry-run', action='store_true',
                        help='show what would be printed '
                             'instead of printing it')
    return parser.parse_args(argv)


# _____________________________________________________________________________
def say_hello(name: str) -> None:
    """Print a greeting to name."""
    print(f'Hello {name}.')


# _____________________________________________________________________________
def main(argv: Optional[list[str]] = None) -> int:
    args = parse_args(argv)
    if args.repeat < 1:
        print(f'error: --repeat must be 1 or more (got {args.repeat})',
              file=sys.stderr)
        return 1

    start = datetime.now()
    t0 = time.perf_counter()

    for _ in range(args.repeat):
        if args.dry_run:
            print(f'[dry-run] would print: Hello {args.name}.')
        else:
            say_hello(args.name)

    elapsed = time.perf_counter() - t0
    end = datetime.now()
    print(f'Started at : {start:%Y-%m-%d %H:%M:%S}')
    print(f'Finished at: {end:%Y-%m-%d %H:%M:%S}')
    print(f'Elapsed    : {elapsed:.3f} s')
    return 0


# _____________________________________________________________________________
if __name__ == '__main__':
    sys.exit(main())
