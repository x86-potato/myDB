import random
import string
from datetime import datetime, timedelta

# Number of users to generate
N = 100  # change as needed
filename = "users.sql"

# Xbox-style username components
adjectives = [
    "Cool", "Dark", "Epic", "Silent", "Pro", "Crazy", "Fast", "Red", "Blue",
    "Shadow", "Night", "Mega", "Ultra", "Stealth", "Fire", "Ice", "Ghost", "Wild"
]

nouns = [
    "Gamer", "Ninja", "Wizard", "Sniper", "Dragon", "Hunter", "Rider",
    "Warrior", "Slayer", "Ghost", "Assassin", "Knight", "Destroyer", "Shadow", "Blade"
]

# Helper functions
def xbox_username():
    """Generate an Xbox-style username with digits and optional underscores."""
    name = random.choice(adjectives) + random.choice(nouns)
    digits = str(random.randint(0, 9999))
    name += digits
    if random.choice([True, False]):
        name = "_" + name + "_"
    return name

def simple_password(length=12):
    """Password with only letters and digits (no special symbols)."""
    chars = string.ascii_letters + string.digits
    return ''.join(random.choices(chars, k=length))

def email_for_username(username):
    """Generate email with only letters, digits, underscores, @ and dots."""
    clean_username = username.replace("_", "")
    domains = ["gmail.com", "yahoo.com", "outlook.com", "example.com"]
    return f"{clean_username}@{random.choice(domains)}"

def random_dob(start_year=1950, end_year=2010):
    """Random date of birth in d/m/yyyy format."""
    start = datetime(year=start_year, month=1, day=1)
    end = datetime(year=end_year, month=12, day=31)
    delta = end - start
    random_days = random.randint(0, delta.days)
    dob = start + timedelta(days=random_days)
    return f"{dob.day}/{dob.month}/{dob.year}"

# Generate SQL inserts
with open(filename, "w") as f:
    # Table creation line
    f.write('create users (username=string, password=string, email=string, dob=string);')
    
    for _ in range(N):
        username = xbox_username()
        password = simple_password()
        email = email_for_username(username)
        dob = random_dob()
        f.write(f'\ninsert into users ("{username}", "{password}", "{email}", "{dob}");')
    
    # Exit at the end
    f.write('\nexit\n')

print(f"{N} users written to {filename}")
