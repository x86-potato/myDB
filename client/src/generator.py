"""
generator.py — generate N random INSERT statements for the users table.

Usage:
    python3 generator.py <N>                   # prints to stdout
    python3 generator.py <N> -o output.sql     # writes to a file
"""

import argparse
import random

FIRST_NAMES = [
    "alice", "bob", "carol", "dave", "eve", "frank", "grace", "hank",
    "iris", "jack", "karen", "leo", "mia", "ned", "olivia", "paul",
    "quinn", "rose", "sam", "tina", "uma", "victor", "wendy", "xander",
    "yara", "zoe", "vlad", "john", "doe", "anna", "liam", "noah",
    "emma", "olivia", "ava", "sophia", "james", "mason", "ethan",
]

START_ID = 400001


def random_name() -> str:
    return random.choice(FIRST_NAMES)


def generate(n: int, start_id: int = START_ID) -> list[str]:
    statements = []
    for i in range(n):
        uid = start_id + i
        name = random_name()
        statements.append(f'insert into users ({uid}, "{name}");')
    return statements


def main():
    parser = argparse.ArgumentParser(description="Generate random INSERT statements for users table.")
    parser.add_argument("n", type=int, help="Number of INSERT statements to generate")
    parser.add_argument("-o", "--output", metavar="FILE",
                        help="Write output to FILE instead of stdout")
    parser.add_argument("--start-id", type=int, default=START_ID,
                        help=f"Starting user ID (default: {START_ID})")
    args = parser.parse_args()

    if args.n <= 0:
        parser.error("N must be a positive integer")

    statements = generate(args.n, args.start_id)
    output = "\n".join(statements) + "\n"

    if args.output:
        with open(args.output, "w") as f:
            f.write(output)
        print(f"Wrote {args.n} statements to {args.output}")
    else:
        print(output, end="")


if __name__ == "__main__":
    main()
