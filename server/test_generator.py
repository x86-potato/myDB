import random
import string

# Configuration
output_file = "users.sql"
num_inserts = 10000000  # number of random insert lines

# Sample word list for Xbox-style usernames
words = [
    "Shadow", "Dragon", "Pixel", "Ghost", "Thunder", "Blade", "Frost", 
    "Fire", "Storm", "Wolf", "Knight", "Hunter", "Rogue", "Titan", "Nova",
    "Smart", "Ranger","Vortex", "Venom", "Eclipse", "Phantom", "Blaze", 
    "Reaper", "Raptor",
    "Specter", "Inferno", "Ember", "Striker", "Falcon", "Havoc", "Rift",
    "Razor", "Bolt", "Echo", "Surge", "Matrix", "Cipher", "Wraith", "Obsidian",
    "Tempest", "Onyx", "Comet", "Quantum", "Crimson", "Viper", "Talon",
    "Zero", "Archer", "Blitz", "Sentinel", "Glitch", "Orbit", "Ace",
    "Forge", "Pulse", "Vertex", "Neon", "Raven", "Spectra", "NovaX", 
    "Terror", "Grim", "Flux", "Delta", "Omega", "Phoenix", "Hollow",
    "Ion", "Nitro", "Stealth", "Core", "Zephyr", "Rune", "Solar",
    "Arctic", "Chaos", "Dread", "EchoX", "Jet", "Venus", "Mars",
    "Lancer", "Plasma", "Drift", "Fury", "Tundra", "QuantumX"
]
words += [
    "NovaCore", "ShadowX", "Nightfall", "Ironclad", "PulseX", "Cryo", "Spectral",
    "Graviton", "Oblivion", "Phaser", "Neutrino", "Valkyrie", "Apex", "Blizzard",
    "Circuit", "GhostX", "Titanium", "Darkstar", "Revenant", "Overdrive", "Mirage",
    "VenomX", "Crytek", "Skyfire", "Hex", "Infernal", "Lumina", "Stratos", "Nebula",
    "Onslaught", "Sentrix", "Warp", "Volt", "Havok", "OmegaX", "Shadowborn", "Cryon",
    "Ironfang", "Pulsefire", "Nightshade", "QuantumZ", "Void", "RogueX", "Blight",
    "Stormbreaker", "EclipseX", "Frostbite", "Grimlock", "Exodus", "PhantomX", "Drakon",
    "VortexX", "Solaris", "ThunderX", "SpectreX", "ChaosX", "FuryX", "NovaStorm"
]




# Helper functions
def xbox_username():
    return random.choice(words) + random.choice(words) + str(random.randint(10, 9999))

def random_password(length=10):
    chars = string.ascii_letters + string.digits + "!@#$%^&*"
    return ''.join(random.choices(chars, k=length))

def random_email():
    name = ''.join(random.choices(string.ascii_lowercase, k=random.randint(5,10)))
    domain = random.choice(["gmail.com","yahoo.com","hotmail.com"])
    return f"{name}@{domain}"

def random_dob():
    day = random.randint(1,28)
    month = random.randint(1,12)
    year = random.randint(1970,2010)
    return f"{month}/{day}/{year}"

# Generate file
with open(output_file, "w") as f:
    # First line: table creation
    f.write("create users (username=string, password=string, email=string, dob=string);\n")
    
    # First fixed insert
    f.write('insert users (username="smartpotato", password="password", email="sp666@gmail.com", dob="2/8/1991");\n')
    
    # Random inserts
    for _ in range(num_inserts):
        uname = xbox_username()
        pwd = random_password()
        email = random_email()
        dob = random_dob()
        f.write(f'insert users (username="{uname}", password="{pwd}", email="{email}", dob="{dob}");\n')
    
    # End with exit
    f.write("exit\n")

print(f"File '{output_file}' created with {num_inserts + 1} insert statements.")
