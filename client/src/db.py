import socket
import struct
from typing import Callable, List, Optional

STATUS_OK       = 0
STATUS_ERROR    = 1
STATUS_METADATA = 2
STATUS_ROW      = 3
STATUS_AGGREGATE = 4

FLAG_LAST = 0
FLAG_MORE = 1


def _recv_exact(sock, n):
    data = b""
    while len(data) < n:
        chunk = sock.recv(n - len(data))
        if not chunk:
            raise ConnectionError("Connection closed")
        data += chunk
    return data


def _recv_packet(sock):
    header = _recv_exact(sock, 6)
    status = header[0]
    flag = header[1]
    payload_length = struct.unpack_from("<I", header, 2)[0]
    payload = _recv_exact(sock, payload_length) if payload_length > 0 else b""
    return status, flag, payload


def _parse_metadata(payload):
    offset = 0
    columns = []
    table_count = struct.unpack_from("<I", payload, offset)[0]
    offset += 4
    for _ in range(table_count):
        name_len = struct.unpack_from("<I", payload, offset)[0]
        offset += 4
        table_name = payload[offset:offset + name_len].decode()
        offset += name_len
        col_count = struct.unpack_from("<I", payload, offset)[0]
        offset += 4
        for _ in range(col_count):
            col_name_len = struct.unpack_from("<I", payload, offset)[0]
            offset += 4
            col_name = payload[offset:offset + col_name_len].decode()
            offset += col_name_len
            offset += 1  # column type byte
            columns.append(f"{table_name}.{col_name}")
    return columns


def _parse_row(payload):
    offset = 0
    col_count = struct.unpack_from("<I", payload, offset)[0]
    offset += 4
    values = []
    for _ in range(col_count):
        val_len = struct.unpack_from("<I", payload, offset)[0]
        offset += 4
        val = payload[offset:offset + val_len].decode()
        offset += val_len
        values.append(val)
    return values


def _parse_aggregate(payload):
    offset = 0
    # aggregate_kind byte (unused by client, display only)
    offset += 1
    label_len = struct.unpack_from("<I", payload, offset)[0]
    offset += 4
    label = payload[offset:offset + label_len].decode()
    offset += label_len
    val_len = struct.unpack_from("<I", payload, offset)[0]
    offset += 4
    value = payload[offset:offset + val_len].decode()
    return label, value


class QueryResult:
    """Holds the result of a single query execution."""

    def __init__(self, ok: bool, error: Optional[str] = None,
                 columns: Optional[List[str]] = None, rows: Optional[List[List[str]]] = None,
                 row_count: Optional[int] = None):
        self.ok = ok
        self.error = error
        self.columns = columns or []
        self.rows = rows or []
        self.row_count = len(self.rows) if row_count is None else row_count

    def __repr__(self):
        if not self.ok:
            return "QueryResult(error={!r})".format(self.error)
        if self.columns:
            return "QueryResult({} row(s), columns={})".format(self.row_count, self.columns)
        return "QueryResult(OK)"


class DBConnection:
    """
    A connection to the database server.

    Usage:
        with DBConnection("localhost", 5432) as db:
            result = db.execute("SELECT * FROM users;")
    """

    def __init__(self, host: str = "localhost", port: int = 5432):
        self.host = host
        self.port = port
        self._sock: Optional[socket.socket] = None

    def connect(self):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.connect((self.host, self.port))

    def disconnect(self):
        if self._sock:
            self._sock.close()
            self._sock = None

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, *_):
        self.disconnect()

    def execute(self, query: str) -> QueryResult:
        """Send a SQL query and return a QueryResult."""
        if not self._sock:
            raise RuntimeError("Not connected. Call connect() first.")
        self._sock.sendall((query + "\n").encode())
        return self._read_response()

    def execute_stream(
        self,
        query: str,
        on_metadata: Optional[Callable[[List[str]], None]] = None,
        on_row: Optional[Callable[[List[str]], None]] = None,
    ) -> QueryResult:
        """Send a SQL query and stream metadata and rows via callbacks."""
        if not self._sock:
            raise RuntimeError("Not connected. Call connect() first.")
        self._sock.sendall((query + "\n").encode())
        return self._read_response(on_metadata=on_metadata, on_row=on_row)

    def execute_file(self, path: str) -> list[QueryResult]:
        """Execute every statement in a .sql file, returning one result per statement."""
        results = []
        with open(path, "r") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("--"):
                    continue
                results.append(self.execute(line))
        return results

    # ------------------------------------------------------------------
    # Internal response parsing
    # ------------------------------------------------------------------

    def _read_response(
        self,
        on_metadata: Optional[Callable[[List[str]], None]] = None,
        on_row: Optional[Callable[[List[str]], None]] = None,
    ) -> QueryResult:
        status, flag, payload = _recv_packet(self._sock)

        if status == STATUS_OK:
            return QueryResult(ok=True)

        if status == STATUS_ERROR:
            return QueryResult(ok=False, error=payload.decode())

        if status == STATUS_METADATA:
            columns = _parse_metadata(payload)
            if on_metadata is not None:
                on_metadata(columns)

            rows = [] if on_row is None else None
            row_count = 0

            if flag == FLAG_LAST:
                return QueryResult(ok=True, columns=columns, rows=rows or [], row_count=row_count)

            while True:
                status, flag, payload = _recv_packet(self._sock)
                if status == STATUS_ROW:
                    row = _parse_row(payload)
                    row_count += 1
                    if on_row is None:
                        rows.append(row)
                    else:
                        on_row(row)
                if flag == FLAG_LAST:
                    break
            return QueryResult(ok=True, columns=columns, rows=rows or [], row_count=row_count)

        if status == STATUS_AGGREGATE:
            aggregates = []
            label, value = _parse_aggregate(payload)
            aggregates.append((label, value))
            while flag == FLAG_MORE:
                status, flag, payload = _recv_packet(self._sock)
                if status == STATUS_AGGREGATE:
                    label, value = _parse_aggregate(payload)
                    aggregates.append((label, value))
            columns = [a[0] for a in aggregates]
            rows = [[a[1] for a in aggregates]]
            return QueryResult(ok=True, columns=columns, rows=rows, row_count=1)

        return QueryResult(ok=False, error=f"Unexpected status code: {status}")
