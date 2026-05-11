import os
from db import DBConnection, QueryResult

HOST = "localhost"
PORT = 5432


def print_result(result: QueryResult):
    if not result.ok:
        print(f"ERROR: {result.error}")
        return

    if result.columns:
        col_widths = [max(len(c), 6) for c in result.columns]
        header = " | ".join(c.ljust(col_widths[i]) for i, c in enumerate(result.columns))
        separator = "-+-".join("-" * w for w in col_widths)
        print(header)
        print(separator)
        for row in result.rows:
            print(" | ".join(v.ljust(col_widths[i]) for i, v in enumerate(row)))
        n = result.row_count
        print(f"({n} row{'s' if n != 1 else ''})")
    else:
        print("OK")


def print_streamed_result(db: DBConnection, query: str):
    state = {
        "printed_header": False,
        "columns": [],
        "widths": [],
    }

    def on_metadata(columns):
        state["columns"] = columns
        state["widths"] = [max(len(column), 6) for column in columns]

        if not columns:
            return

        header = " | ".join(
            column.ljust(state["widths"][i]) for i, column in enumerate(columns)
        )
        separator = "-+-".join("-" * width for width in state["widths"])
        print(header)
        print(separator)
        state["printed_header"] = True

    def on_row(row):
        widths = state["widths"]
        print(" | ".join(value.ljust(widths[i]) for i, value in enumerate(row)))

    result = db.execute_stream(query, on_metadata=on_metadata, on_row=on_row)

    if not result.ok:
        print(f"ERROR: {result.error}")
        return

    if result.columns:
        if not state["printed_header"]:
            on_metadata(result.columns)
        n = result.row_count
        print(f"({n} row{'s' if n != 1 else ''})")
    else:
        print("OK")


def handle_meta_command(cmd: str, db: DBConnection) -> bool:
    """
    Handle dot-commands (like SQLite's .read).
    Returns True if the input was a meta-command, False otherwise.
    """
    parts = cmd.strip().split(None, 1)
    command = parts[0].lower()

    if command == ".source" or command == ".read":
        if len(parts) < 2:
            print("Usage: .source <file.sql>")
            return True
        path = parts[1].strip()
        if not os.path.isfile(path):
            print(f"ERROR: file not found: {path}")
            return True
        results = db.execute_file(path)
        for r in results:
            print_result(r)
        return True

    if command == ".help":
        print("Meta-commands:")
        print("  .source <file>   Execute SQL statements from a file")
        print("  .read   <file>   Alias for .source")
        print("  .help            Show this help message")
        print("  exit / quit      Disconnect and exit")
        return True

    return False


def main():
    with DBConnection(HOST, PORT) as db:
        print(f"Connected to {HOST}:{PORT}")
        print('Type SQL queries, ".source <file>" to run a file, or "exit" to quit.')

        while True:
            try:
                query = input("db> ").strip()
            except (EOFError, KeyboardInterrupt):
                print()
                break

            if not query:
                continue

            if query.lower() in ("exit", "quit"):
                break

            if query.startswith("."):
                handle_meta_command(query, db)
                continue

            try:
                print_streamed_result(db, query)
            except ConnectionError as e:
                print(f"Connection lost: {e}")
                break


if __name__ == "__main__":
    main()
