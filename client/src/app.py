import socket
import struct

STATUS_OK       = 0
STATUS_ERROR    = 1
STATUS_METADATA = 2
STATUS_ROW      = 3

def recv_exact(sock, n):
    """Receive exactly n bytes from socket."""
    data = b""
    while len(data) < n:
        chunk = sock.recv(n - len(data))
        if not chunk:
            raise ConnectionError("Connection closed")
        data += chunk
    return data

def recv_packet(sock):
    """Read one packet: 1 byte status, 1 byte flag, 4 bytes payload_length, N bytes payload."""
    header = recv_exact(sock, 6)
    status = header[0]
    flag = header[1]
    payload_length = struct.unpack_from("<I", header, 2)[0]

    payload = recv_exact(sock, payload_length) if payload_length > 0 else b""
    return status, flag, payload

def handle_response(sock):
    status, flag, payload = recv_packet(sock)

    if status == STATUS_OK:
        print("OK")
    elif status == STATUS_ERROR:
        print(f"ERROR: {payload.decode()}")
    else:
        print(f"Unexpected status code: {status}")

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
            try:
                handle_response(s)
            except ConnectionError as e:
                print(f"Connection lost: {e}")
                break

if __name__ == "__main__":
    main()