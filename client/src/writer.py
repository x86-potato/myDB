import time
import random
import threading
from concurrent.futures import ThreadPoolExecutor
from db import DBConnection

HOST = "localhost"
PORT = 5432

CONNECTION_DELAY = 1
THREADS = 1
MAX_INSERTS = 100000

UID_MIN = 1
UID_MAX = 1_999_000_000

REPORT_INTERVAL = 1.0
AVERAGE_WINDOW_UPDATES = 10

counter_lock = threading.Lock()
total_success = 0
total_errors = 0
scheduled_inserts = 0


def reserve_insert_slot(stop_event: threading.Event) -> bool:
    global scheduled_inserts

    if MAX_INSERTS == -1:
        return True

    with counter_lock:
        if scheduled_inserts >= MAX_INSERTS:
            stop_event.set()
            return False

        scheduled_inserts += 1
        return True


def insert_worker(stop_event: threading.Event, connect_delay: float = 0.0):
    global total_success, total_errors

    local_success = 0
    local_errors = 0
    last_flush = time.time()

    # stagger connections slightly
    time.sleep(connect_delay)

    try:
        with DBConnection(HOST, PORT) as db:

            while not stop_event.is_set():
                if not reserve_insert_slot(stop_event):
                    break

                uid = random.randint(UID_MIN, UID_MAX)

                query = (
                    f'insert into users ({uid}, "stress_user_{uid}");'
                )

                try:
                    res = db.execute(query)

                    if res.ok:
                        local_success += 1
                    else:
                        local_errors += 1
                        print(f"\n[WRITE ERROR] UID {uid} failed: {res.error}")
                        # Give the slot back so MAX_INSERTS means MAX_INSERTS successes
                        with counter_lock:
                            scheduled_inserts -= 1

                except Exception as e:
                    local_errors += 1
                    #print(f"\n[EXEC ERROR] {e}")

                # periodically flush local counters
                now = time.time()

                if now - last_flush >= 0.25:
                    with counter_lock:
                        total_success += local_success
                        total_errors += local_errors

                    local_success = 0
                    local_errors = 0
                    last_flush = now

    except Exception as e:
        print(f"\n[THREAD CRASH] Connection dropped: {e}")

    finally:
        # final flush
        with counter_lock:
            total_success += local_success
            total_errors += local_errors


if __name__ == "__main__":
    stop_event = threading.Event()

    print(
        f"Starting nonstop random write stress test "
        f"with {THREADS} concurrent threads..."
    )
    if MAX_INSERTS == -1:
        print("Insert limit: unlimited")
    else:
        print(f"Insert limit: {MAX_INSERTS}")
    print("Press Ctrl+C to stop.\n")

    start_time = time.time()

    last_success = 0
    last_time = start_time
    window_start_success = 0
    window_start_time = start_time
    updates_in_window = 0

    try:
        with ThreadPoolExecutor(max_workers=THREADS) as executor:

            for i in range(THREADS):
                delay = i * CONNECTION_DELAY

                executor.submit(
                    insert_worker,
                    stop_event,
                    delay
                )

            while True:
                time.sleep(REPORT_INTERVAL)

                now = time.time()

                with counter_lock:
                    success = total_success
                    errors = total_errors

                interval_qps = (
                    (success - last_success)
                    / (now - last_time)
                )

                updates_in_window += 1
                window_avg_qps = (
                    (success - window_start_success)
                    / (now - window_start_time)
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
        print("\nStopping workers...")
        stop_event.set()

    duration = time.time() - start_time

    print("\n--- WRITER RESULTS ---")
    print(f"Successfully committed {total_success} rows.")
    print(f"Errors: {total_errors}")
    print(f"Duration: {duration:.2f} seconds")
    print(f"Avg Write Throughput: {total_success / duration:.2f} QPS")