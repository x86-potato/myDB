import random
import string

# All possible fields and their types
all_fields = [
    ("username", "string"),
    ("password", "string"),
    ("email", "string"),
    ("age", "int"),
    ("dob", "string"),
    ("uid", "int")
]

# Shorter keywords for username construction
short_keywords = [
    "sh", "dr", "x", "pr", "gm", "nj", "gh", "wf", "kg", "fr",
    "st", "bl", "ht", "sn", "dk", "lt", "rg", "dm", "sk", "ph", "zr"
]

# --- Data generators ---
def random_username():
    # Pick 2-3 short keywords
    parts = [random.choice(short_keywords) for _ in range(random.randint(2, 3))]
    # Fill the rest with digits up to 16 chars
    base = ''.join(parts)
    max_digits = 16 - len(base)
    digits = ''.join(random.choice(string.digits) for _ in range(max_digits))
    return (base + digits)[:16]  # enforce max length 16

def random_password(length=32):
    chars = string.ascii_letters + string.digits
    return ''.join(random.choice(chars) for _ in range(length))

def random_email(username):
    domains = ['gmail.com', 'yahoo.com', 'hotmail.com']
    email = username + "@" + random.choice(domains)
    return email[:32]  # max 32 chars

def random_int(min_val=1, max_val=100000000):
    return str(random.randint(min_val, max_val))

def random_age():
    return str(random.randint(13, 80))

def random_dob():
    month = random.randint(1, 12)
    day = random.randint(1, 28)
    year = random.randint(1970, 2010)
    return f"{month:02d}{day:02d}{year}"

generators = {
    "username": random_username,
    "password": random_password,
    "email": lambda: "",  # placeholder, email depends on username
    "age": random_age,
    "dob": random_dob,
    "uid": random_int
}

# --- CLI: number of rows ---
while True:
    num_rows_input = input("Enter number of rows to generate: ")
    if num_rows_input.isdigit() and int(num_rows_input) > 0:
        num_rows = int(num_rows_input)
        break
    print("Invalid input. Enter a positive integer.")

# --- CLI for picking fields ---
print("\nAvailable fields:")
for i, (name, _) in enumerate(all_fields):
    print(f"{i+1}. {name}")

picked_fields = []
while True:
    selection = input("Enter a field number to pick (or press Enter to finish): ")
    if selection == "":
        break
    if not selection.isdigit() or int(selection) < 1 or int(selection) > len(all_fields):
        print("Invalid choice, try again.")
        continue
    field_name = all_fields[int(selection)-1][0]
    if field_name in picked_fields:
        print("Field already picked.")
        continue
    picked_fields.append(field_name)

if not picked_fields:
    print("No fields selected. Exiting.")
    exit()

output_file = "users.csv"

# --- Write data to file ---
with open(output_file, "w") as f:
    f.write(",".join(picked_fields) + "\n")
    for uid_counter in range(1, num_rows + 1):
        row = []
        username_val = ""
        for field in picked_fields:
            if field == "email":
                val = random_email(username_val)
            elif field == "username":
                username_val = generators["username"]()
                val = username_val
            elif field == "uid":
                val = str(uid_counter)
            else:
                val = generators[field]()
            if dict(all_fields)[field] == "string":
                val = f'"{val}"'
            row.append(val)
        f.write(",".join(row) + "\n")

print(f"{num_rows} rows written to {output_file}")
