import random
import string

# All possible fields and their types
all_fields = [
    ("username", "string"),
    ("password", "string"),
    ("email", "string"),
    ("age", "int"),
    ("dob", "string"),
    ("id", "int")
]

# Keywords for Xbox-style usernames
keywords = ["shadow", "dragon", "x", "pro", "gamer", "ninja", "ghost", "wolf", "king", "fire"]

# --- Data generators ---
def random_username():
    first = random.choice(keywords)
    second = random.choice(keywords)
    digits = ''.join(random.choice(string.digits) for _ in range(random.randint(0, 4)))
    return first + second + digits

def random_password(length=6):
    chars = string.ascii_letters + string.digits
    return ''.join(random.choice(chars) for _ in range(length))

def random_email():
    names = ['sp', 'cool', 'fun', 'pro', 'x', 'gamer', 'player']
    domains = ['gmail.com', 'yahoo.com', 'hotmail.com']
    return random.choice(names) + str(random.randint(1,999)) + "@" + random.choice(domains)

def random_int(min_val=1, max_val=100):
    return str(random.randint(min_val, max_val))

def random_dob():
    year = random.randint(1970, 2010)
    month = random.randint(1,12)
    day = random.randint(1,28)
    return f"{year}-{month:02d}-{day:02d}"

generators = {
    "username": random_username,
    "password": random_password,
    "email": random_email,
    "age": random_int,
    "dob": random_dob,
    "id": random_int
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
    for _ in range(num_rows):
        row = []
        for field in picked_fields:
            val = generators[field]()
            if dict(all_fields)[field] == "string":
                val = f'"{val}"'
            row.append(val)
        f.write(",".join(row) + "\n")

print(f"{num_rows} rows written to {output_file}")
