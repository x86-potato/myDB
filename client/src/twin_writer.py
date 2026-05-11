import time
import random
import threading
from concurrent.futures import ThreadPoolExecutor
from db import DBConnection

HOST = "localhost"
PORT = 5432

THREADS = 3
MAX_INSERTS = 100_000  # per table

UID_MIN = 1
UID_MAX = 1_999_000_000
PID_MIN = 1
PID_MAX = 1_999_000_000
MID_MIN = 1
MID_MAX = 1_999_000_000
QTY_MIN = 1
QTY_MAX = 9_999

counter_lock = threading.Lock()
users_success = 0
products_success = 0
messages_success = 0


def worker(stop_event: threading.Event):
    global users_success, products_success, messages_success

    local_users = 0
    local_products = 0
    local_messages = 0
    randint = random.randint

    try:
        with DBConnection(HOST, PORT) as db:
            execute = db.execute

            while not stop_event.is_set():
                choice = randint(0, 2)
                if choice == 0:
                    uid = randint(UID_MIN, UID_MAX)
                    res = execute(f'insert into users ({uid}, "u{uid}");')
                    if res.ok:
                        local_users += 1
                        if local_users % 500 == 0:
                            with counter_lock:
                                users_success += local_users
                            local_users = 0
                elif choice == 1:
                    pid = randint(PID_MIN, PID_MAX)
                    qty = randint(QTY_MIN, QTY_MAX)
                    name = f"prod{pid}"[:32]
                    res = execute(f'insert into products ({pid}, "{name}", {qty});')
                    if res.ok:
                        local_products += 1
                        if local_products % 500 == 0:
                            with counter_lock:
                                products_success += local_products
                            local_products = 0
                else:
                    mid = randint(MID_MIN, MID_MAX)
                    sender = randint(UID_MIN, UID_MAX)
                    text = f"msg{mid}"[:32]
                    res = execute(f'insert into messages ({mid}, "{text}", {sender});')
                    if res.ok:
                        local_messages += 1
                        if local_messages % 500 == 0:
                            with counter_lock:
                                messages_success += local_messages
                            local_messages = 0

    except Exception:
        pass
    finally:
        with counter_lock:
            users_success += local_users
            products_success += local_products
            messages_success += local_messages


if __name__ == "__main__":
    stop_event = threading.Event()

    print(f"Triplet writer | threads={THREADS} | limit={MAX_INSERTS} per table\n")
    print(f"{'Time':>6} | {'Users':>8} | {'U-QPS':>7} | {'Products':>8} | {'P-QPS':>7} | {'Messages':>8} | {'M-QPS':>7} | {'Total':>9} | {'Avg QPS':>8}")
    print("-" * 100)

    start = time.time()
    last_users = 0
    last_products = 0
    last_messages = 0
    last_time = start

    with ThreadPoolExecutor(max_workers=THREADS) as executor:
        for _ in range(THREADS):
            executor.submit(worker, stop_event)

        try:
            while True:
                time.sleep(1.0)
                now = time.time()

                with counter_lock:
                    u = users_success
                    p = products_success
                    m = messages_success

                elapsed = now - start
                dt = now - last_time

                u_qps = (u - last_users) / dt
                p_qps = (p - last_products) / dt
                m_qps = (m - last_messages) / dt
                total = u + p + m
                avg_qps = total / elapsed

                print(
                    f"{elapsed:>5.1f}s | {u:>8} | {u_qps:>7.0f} | {p:>8} | {p_qps:>7.0f} | {m:>8} | {m_qps:>7.0f} | {total:>9} | {avg_qps:>8.0f}"
                )

                last_users = u
                last_products = p
                last_messages = m
                last_time = now

                if u >= MAX_INSERTS and p >= MAX_INSERTS and m >= MAX_INSERTS:
                    stop_event.set()
                    break

        except KeyboardInterrupt:
            stop_event.set()

    elapsed = time.time() - start
    with counter_lock:
        u = users_success
        p = products_success
        m = messages_success

    print(f"\n--- TRIPLET WRITER RESULTS ---")
    print(f"Users:    {u} rows")
    print(f"Products: {p} rows")
    print(f"Messages: {m} rows")
    print(f"Total:    {u + p + m} rows in {elapsed:.2f}s | Avg {(u + p + m) / elapsed:.0f} QPS")
