import time
import random
import threading
import argparse
from concurrent.futures import ThreadPoolExecutor
from db import DBConnection

HOST = "localhost"
PORT = 5432

ID_MIN = 1
ID_MAX = 1_999_000_000

# One query-builder per known table
def make_query(table: str, randint) -> str:
    if table == "users":
        uid = randint(ID_MIN, ID_MAX)
        return f'insert into users ({uid}, "u{uid}");'
    elif table == "products":
        pid = randint(ID_MIN, ID_MAX)
        qty = randint(1, 9999)
        name = f"prod{pid}"[:32]
        return f'insert into products ({pid}, "{name}", {qty});'
    elif table == "messages":
        mid = randint(ID_MIN, ID_MAX)
        sender = randint(ID_MIN, ID_MAX)
        text = f"msg{mid}"[:32]
        return f'insert into messages ({mid}, "{text}", {sender});'
    else:
        raise ValueError(f"Unknown table: {table}")

total_success = 0
counter_lock = threading.Lock()


def insert_worker(stop_event: threading.Event, table: str):
    global total_success

    local_success = 0
    randint = random.randint

    try:
        with DBConnection(HOST, PORT) as db:
            execute = db.execute

            while not stop_event.is_set():
                res = execute(make_query(table, randint))
                if res.ok:
                    local_success += 1
                    if local_success % 500 == 0:
                        with counter_lock:
                            total_success += local_success
                        local_success = 0

    except Exception:
        pass
    finally:
        with counter_lock:
            total_success += local_success


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Fast single-table insert benchmark")
    parser.add_argument("--threads", "-t", type=int, default=10, help="Number of writer threads (default: 10)")
    parser.add_argument("--table", "-n", type=str, default="users", help="Table name to insert into (default: users)")
    parser.add_argument("--limit", "-l", type=int, default=100_000, help="Stop after this many successful inserts (default: 100000)")
    args = parser.parse_args()

    THREADS = args.threads
    TABLE = args.table
    MAX_INSERTS = args.limit

    if TABLE not in ("users", "products", "messages"):
        print(f"Error: unknown table '{TABLE}'. Choose from: users, products, messages")
        raise SystemExit(1)

    stop_event = threading.Event()

    print(f"Fast writer | table={TABLE} | threads={THREADS} | limit={MAX_INSERTS}\n")

    start = time.time()
    last_count = 0
    last_time = start

    with ThreadPoolExecutor(max_workers=THREADS) as executor:
        for _ in range(THREADS):
            executor.submit(insert_worker, stop_event, TABLE)

        try:
            while True:
                time.sleep(1.0)
                now = time.time()

                with counter_lock:
                    count = total_success

                elapsed = now - start
                interval_qps = (count - last_count) / (now - last_time)
                avg_qps = count / elapsed

                print(f"Writes: {count:>10} | QPS: {interval_qps:>8.0f} | Avg: {avg_qps:>8.0f}")

                last_count = count
                last_time = now

                if count >= MAX_INSERTS:
                    stop_event.set()
                    break

        except KeyboardInterrupt:
            stop_event.set()

    elapsed = time.time() - start
    with counter_lock:
        count = total_success

    print(f"\nDone: {count} rows in {elapsed:.2f}s | Avg {count/elapsed:.0f} QPS")
