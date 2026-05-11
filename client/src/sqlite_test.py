import sqlite3
import time
import random
import threading

DB_FILE = "benchmark.db"

THREADS = 3
REPORT_INTERVAL = 1.0
AVERAGE_WINDOW_UPDATES = 10

ID_MIN = 1
ID_MAX = 1_999_000_000

# One table definition per worker thread
TABLES = [
    {
        "name": "users",
        "ddl": "CREATE TABLE IF NOT EXISTS users (uid INTEGER PRIMARY KEY, name TEXT NOT NULL);",
        "insert": "INSERT INTO users (uid, name) VALUES (?, ?);",
        "make_row": lambda i: (i, f"user_{i}"),
    },
    {
        "name": "products",
        "ddl": "CREATE TABLE IF NOT EXISTS products (pid INTEGER PRIMARY KEY, name TEXT NOT NULL);",
        "insert": "INSERT INTO products (pid, name) VALUES (?, ?);",
        "make_row": lambda i: (i, f"product_{i}"),
    },
    {
        "name": "messages",
        "ddl": "CREATE TABLE IF NOT EXISTS messages (mid INTEGER PRIMARY KEY, text TEXT NOT NULL, sender_id INTEGER NOT NULL);",
        "insert": "INSERT INTO messages (mid, text, sender_id) VALUES (?, ?, ?);",
        "make_row": lambda i: (i, f"msg_{i}", random.randint(ID_MIN, ID_MAX)),
    },
]

total_success = 0
total_errors = 0

counter_lock = threading.Lock()


def setup_database():
    conn = sqlite3.connect(DB_FILE)
    cur = conn.cursor()

    # WAL mode + group commit (NORMAL = fsync only at checkpoint, not every commit)
    cur.execute("PRAGMA journal_mode=WAL;")
    cur.execute("PRAGMA synchronous=NORMAL;")
    cur.execute("PRAGMA temp_store=MEMORY;")

    for t in TABLES:
        cur.execute(t["ddl"])

    conn.commit()
    conn.close()


def insert_worker(table_def: dict, stop_event: threading.Event):
    global total_success, total_errors

    local_success = 0
    local_errors = 0
    last_flush = time.time()

    conn = sqlite3.connect(
        DB_FILE,
        isolation_level=None,  # autocommit mode
        check_same_thread=False
    )
    cur = conn.cursor()

    # Re-apply per-connection settings
    cur.execute("PRAGMA journal_mode=WAL;")
    cur.execute("PRAGMA synchronous=NORMAL;")

    sql = table_def["insert"]
    make_row = table_def["make_row"]

    while not stop_event.is_set():
        row_id = random.randint(ID_MIN, ID_MAX)
        try:
            cur.execute("BEGIN;")
            cur.execute(sql, make_row(row_id))
            cur.execute("COMMIT;")
            local_success += 1
        except Exception:
            local_errors += 1
            try:
                cur.execute("ROLLBACK;")
            except Exception:
                pass

        now = time.time()
        if now - last_flush >= 0.25:
            with counter_lock:
                total_success += local_success
                total_errors += local_errors
            local_success = 0
            local_errors = 0
            last_flush = now

    conn.close()
    with counter_lock:
        total_success += local_success
        total_errors += local_errors


if __name__ == "__main__":
    setup_database()

    stop_event = threading.Event()

    print(f"Starting SQLite WAL group-commit benchmark — {THREADS} thread(s), one per table.")
    print("synchronous=NORMAL  (fsync at checkpoint only — group commit)")
    print("Tables: users | products | messages")
    print("Each INSERT = its own transaction commit.\n")

    start_time = time.time()
    last_success = 0
    last_time = start_time
    window_start_success = 0
    window_start_time = start_time
    updates_in_window = 0

    workers = [
        threading.Thread(target=insert_worker, args=(TABLES[i], stop_event))
        for i in range(THREADS)
    ]
    for w in workers:
        w.start()

    try:
        while True:
            time.sleep(REPORT_INTERVAL)

            now = time.time()

            with counter_lock:
                success = total_success
                errors = total_errors

            interval_qps = (success - last_success) / (now - last_time)

            updates_in_window += 1
            window_avg_qps = (
                (success - window_start_success) / (now - window_start_time)
            )

            print(
                f"Total Writes: {success} | "
                f"Errors: {errors} | "
                f"Interval QPS: {interval_qps:.2f} | "
                f"Avg QPS ({updates_in_window}/{AVERAGE_WINDOW_UPDATES}): "
                f"{window_avg_qps:.2f}"
            )

            last_success = success
            last_time = now

            if updates_in_window == AVERAGE_WINDOW_UPDATES:
                window_start_success = success
                window_start_time = now
                updates_in_window = 0

    except KeyboardInterrupt:
        print("\nStopping benchmark...")
        stop_event.set()

    for w in workers:
        w.join()

    duration = time.time() - start_time

    print("\n--- SQLITE WAL GROUP COMMIT RESULTS ---")
    print(f"Successfully committed {total_success} rows.")
    print(f"Errors: {total_errors}")
    print(f"Duration: {duration:.2f} seconds")
    print(f"Avg Write Throughput: {total_success / duration:.2f} commits/sec")