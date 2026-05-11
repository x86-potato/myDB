import time
import threading
import random
from db import DBConnection

HOST = "localhost"
PORT = 5432

ID_MIN = 1
ID_MAX = 1_999_000_000

THREAD_COUNT = 10
REPORT_INTERVAL = 1.0

stop_flag = False

counter_lock = threading.Lock()
total_queries = 0
total_misses = 0
total_errors = 0

QUERIES = [
    lambda i: f"select * from users where uid == {i};",
    lambda i: f"select * from products where pid == {i};",
    lambda i: f"select * from messages where mid == {i};",
]


def worker(thread_id: int):
    global total_queries, total_misses, total_errors, stop_flag

    local_q = 0
    local_m = 0
    local_e = 0
    last_flush = time.time()
    randint = random.randint
    randrange = random.randrange

    try:
        with DBConnection(HOST, PORT) as db:
            execute = db.execute

            while not stop_flag:
                row_id = randint(ID_MIN, ID_MAX)
                query_fn = QUERIES[randrange(3)]

                try:
                    res = execute(query_fn(row_id))
                    if not res.ok:
                        local_e += 1
                    elif not res.rows:
                        local_m += 1  # valid lookup, key just not present
                    else:
                        local_q += 1
                except Exception:
                    local_e += 1

                now = time.time()
                if now - last_flush >= 0.25:
                    with counter_lock:
                        total_queries += local_q
                        total_misses  += local_m
                        total_errors  += local_e
                    local_q = local_m = local_e = 0
                    last_flush = now

    finally:
        with counter_lock:
            total_queries += local_q
            total_misses  += local_m
            total_errors  += local_e


if __name__ == "__main__":
    print(f"Random point-lookup reader | threads={THREAD_COUNT}")
    print("Tables: users (uid) | products (pid) | messages (mid)")
    print("Press Ctrl+C to stop.\n")
    print(f"{'Time':>6} | {'Hits':>10} | {'Misses':>10} | {'Errors':>8} | {'QPS':>8} | {'Avg QPS':>8}")
    print("-" * 70)

    threads = []
    start_time = time.time()
    last_q = 0
    last_t = start_time

    for i in range(THREAD_COUNT):
        t = threading.Thread(target=worker, args=(i,))
        t.daemon = True
        t.start()
        threads.append(t)

    try:
        while True:
            time.sleep(REPORT_INTERVAL)
            now = time.time()

            with counter_lock:
                q = total_queries
                m = total_misses
                e = total_errors

            elapsed = now - start_time
            interval_qps = (q - last_q) / (now - last_t) if now > last_t else 0.0
            avg_qps = (q + m) / elapsed

            print(
                f"{elapsed:>5.1f}s | {q:>10} | {m:>10} | {e:>8} | "
                f"{interval_qps:>8.0f} | {avg_qps:>8.0f}"
            )

            last_q = q
            last_t = now

    except KeyboardInterrupt:
        print("\nStopping...")
        stop_flag = True

    for t in threads:
        t.join(timeout=2)

    elapsed = time.time() - start_time
    print("\n--- FINAL RESULTS ---")
    print(f"Hits:   {total_queries}")
    print(f"Misses: {total_misses}")
    print(f"Errors: {total_errors}")
    print(f"Elapsed: {elapsed:.2f}s | Avg QPS: {(total_queries + total_misses) / elapsed:.0f}")