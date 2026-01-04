import random
import csv
from faker import Faker

fake = Faker()

# Realistic product name components
PRODUCT_BASES = [
    "iPhone", "Galaxy S", "Pixel", "MacBook Pro", "MacBook Air", "Dell XPS", "HP Pavilion",
    "Surface Pro", "Surface Laptop", "ThinkPad X1", "Yoga", "AirPods", "Galaxy Buds",
    "Apple Watch", "Fitbit Charge", "Kindle", "Echo Dot", "Nest Hub", "PlayStation",
    "Xbox Series X", "Nintendo Switch", "Wireless Mouse", "Mechanical Keyboard",
    "USB-C Hub", "Portable SSD", "4K Monitor", "Gaming Headset", "Bluetooth Speaker",
    "Smart Plug", "Ring Doorbell", "Security Camera", "Coffee Maker", "Air Fryer"
]

SUFFIXES = ["Pro", "Max", "Plus", "Lite", "Ultra", "SE", "Mini", "Air", "2024", "2025", "Black", "White", "Silver", "Space Gray", "Blue", "Red", "Edition"]

# === Table Definitions ===
tables = {
    "users": [
        ("uid", "int", lambda p: p['start_id'] + p['counter'] - 1),
        ("name", "char32", lambda p: fake.name()[:32]),
        ("state", "char8", lambda p: fake.state_abbr()[:8]),
        ("age", "int", lambda p: random.randint(18, 85))
    ],
    "products": [
        ("pid", "int", lambda p: p['start_id'] + p['counter'] - 1),
        ("name", "char32", lambda p: generate_product_name()[:32]),
        ("price", "int", lambda p: random.randint(50, 5000)),
        ("stock", "int", lambda p: random.randint(0, 1000))
    ],
    "orders": [
        ("oid", "int", lambda p: p['start_id'] + p['counter'] - 1),
        ("uid", "int", lambda p: random.randint(p.get('min_uid', 1), p['max_uid'])),
        ("pid", "int", lambda p: random.randint(1, p['max_pid'])),
        ("total", "int", lambda p: random.randint(100, 100000))
    ]
}

def generate_product_name():
    base = random.choice(PRODUCT_BASES)
    name = base
    
    if random.random() < 0.5:  # 50% chance to add suffix
        name += " " + random.choice(SUFFIXES)
    
    # Add model numbers for phones/watches
    if base.startswith("iPhone"):
        name += f" {random.choice([13, 14, 15, 16])}"
    elif base.startswith("Galaxy S"):
        name += f"{random.randint(21, 25)}"
    elif "Watch" in base:
        name += f" Series {random.randint(7, 10)}"
    elif "Kindle" in base:
        name += " Paperwhite"
    
    return name

# === Input Helpers ===
def get_positive_int(prompt, allow_zero=False):
    while True:
        try:
            value = int(input(prompt))
            if value > 0 or (allow_zero and value >= 0):
                return value
            print("Please enter a valid positive integer.")
        except ValueError:
            print("Invalid input. Enter a number.")

# === Core CSV Generation ===
def generate_csv(table_name, num_rows, start_id=1, max_uid=None, max_pid=None, min_uid=None):
    if table_name not in tables:
        print(f"Error: Table '{table_name}' not defined.")
        return

    columns = tables[table_name]
    filename = f"{table_name}.csv"
    
    print(f"Generating {num_rows:,} rows for {filename} (starting ID: {start_id}) ...")

    with open(filename, "w", newline="", encoding="utf-8") as f:
        writer = csv.writer(f, quoting=csv.QUOTE_NONNUMERIC)
        
        # Header
        header = [col[0] for col in columns]
        writer.writerow(header)
        
        # Rows
        for counter in range(1, num_rows + 1):
            row = []
            params = {
                'counter': counter,
                'start_id': start_id,
                'max_uid': max_uid,
                'max_pid': max_pid,
                'min_uid': min_uid or 1
            }
            
            for col_name, col_type, generator in columns:
                value = generator(params)
                
                # Truncate strings
                if col_type.startswith("char"):
                    max_len = int(col_type[4:])
                    if isinstance(value, str):
                        value = value[:max_len]
                
                # Keep integers unquoted
                if col_type == "int":
                    value = int(value)
                
                row.append(value)
            
            writer.writerow(row)
    
    last_id = start_id + num_rows - 1
    print(f"✓ {num_rows:,} rows written to {filename} (IDs: {start_id} → {last_id})")

# === Main Program ===
if __name__ == "__main__":
    print("=== CSV Data Generator with Starting ID Support ===")
    print("Tables: users, products, orders, all\n")
    
    mode = input("Enter mode (users/products/orders/all): ").strip().lower()
    
    if mode == "all":
        num_users = get_positive_int("Enter number of users to generate: ")
        start_uid = get_positive_int("Starting UID (e.g., 1 or 1001 to continue): ")
        
        num_products = get_positive_int("Enter number of products to generate: ")
        start_pid = get_positive_int("Starting PID: ")
        
        num_orders = get_positive_int("Enter number of orders to generate: ")
        start_oid = get_positive_int("Starting OID: ")
        
        max_uid = start_uid + num_users - 1
        max_pid = start_pid + num_products - 1
        
        generate_csv("users", num_users, start_id=start_uid)
        generate_csv("products", num_products, start_id=start_pid)
        generate_csv("orders", num_orders, start_id=start_oid, max_uid=max_uid, max_pid=max_pid)
        
    elif mode in ["users", "products", "orders"]:
        num_rows = get_positive_int(f"Enter number of {mode} to generate: ")
        start_id = get_positive_int(f"Starting {mode[:-1].upper()}ID (e.g., 1 or 5001): ")
        
        if mode == "orders":
            max_uid = get_positive_int("Max existing UID (highest user ID): ")
            max_pid = get_positive_int("Max existing PID (highest product ID): ")
            generate_csv(mode, num_rows, start_id=start_id, max_uid=max_uid, max_pid=max_pid)
        else:
            generate_csv(mode, num_rows, start_id=start_id)
            
    else:
        print("Invalid mode. Choose: users, products, orders, all")
        exit()
    
    print("\nGeneration complete!")
    print("Tip: Use starting IDs to append new data without overlapping.")
    print("Requires: pip install faker")