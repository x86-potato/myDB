import socket

def main():
    host, port = "localhost", 5432
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((host, port))
        print(f"Connected to {host}:{port}")
        
        while True:
            query = input("db> ")
            if query.lower() in ("exit", "quit"):
                break
            s.sendall((query + "\n").encode())
            #response = s.recv(4096).decode().strip()
            #print(response)

if __name__ == "__main__":
    main()