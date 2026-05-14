#include "session.hpp"
#include "server.hpp"
#include <atomic>
#include <iomanip>
#include <sstream>

std::string Session::read_query()
{
    std::string line;
    char c;
    while (read(client_fd, &c, 1) > 0) {
        if (c == '\n') {
            break;
        }
        line += c;
    }
    return line;
}

void Session::run(Server& server)
{
    while (true) {
        std::string query = read_query();
        if (query.empty()) {
            break; // Client disconnected
        }
        //std::cout << "Received query from session " << session_id << ": " << query << "\n";
        //current_transaction_id = server.database.create_transaction();
        server.executor.execute(query, *this);
    }

    // Client disconnected. If a BEGIN transaction was never committed,
    // roll it back now to release all held page locks.
    if (current_transaction_id != -1) {
        server.database.rollback_transaction(current_transaction_id);
        current_transaction_id = -1;
    }

    if(client_fd != -1) {
        close(client_fd);
        client_fd = -1;
    }

}

inline void push_u32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back(val & 0xFF);
    buf.push_back((val >> 8) & 0xFF);
    buf.push_back((val >> 16) & 0xFF);
    buf.push_back((val >> 24) & 0xFF);
}

inline void push_str(std::vector<uint8_t>& buf, const std::string& s) {
    buf.insert(buf.end(), s.begin(), s.end());
}

void Session::send_ok() 
{
    uint8_t header[6] = {
        static_cast<uint8_t>(StatusCode::OK),
        static_cast<uint8_t>(PacketFlag::LAST_PACKET),
        0, 0, 0, 0  // payload_length = 0
    };
    write(client_fd, header, sizeof(header));

}
void Session::send_error(const std::string& error_message)
{
    if (client_fd == -1) {
        if (!silent) std::cerr << error_message << "\n";
        return;
    }

    std::vector<uint8_t> payload;
    payload.reserve(6 + error_message.size());
    payload.push_back(static_cast<uint8_t>(StatusCode::ERROR));
    payload.push_back(static_cast<uint8_t>(PacketFlag::LAST_PACKET));
    push_u32(payload, static_cast<uint32_t>(error_message.size()));
    push_str(payload, error_message);
    write(client_fd, payload.data(), payload.size());
}

void Session::send_metadata(std::vector<const Table*> tables, bool more_packets)
{
    std::vector<uint8_t> payload;


    //status code
    payload.push_back(static_cast<uint8_t>(StatusCode::METADATA));

    //flag
    payload.push_back(static_cast<uint8_t>(more_packets ? PacketFlag::MORE_PACKETS : PacketFlag::LAST_PACKET));

    //payload length (placeholder, will fill in later)
    size_t payload_length_pos = payload.size();
    int final_payload_length = 0; // will calculate as we build the payload
    push_u32(payload, 0); // placeholder for payload length


    //payload

    //table_count
    final_payload_length += 4; // for the table count
    push_u32(payload, tables.size());

    //for each table:
    for (const auto& table : tables) {
        final_payload_length += 4 + table->name.length(); // for name length and name
        push_u32(payload, table->name.length());     //push name len
        push_str(payload, table->name);              //push table name

        final_payload_length += 4; // for the column count
        push_u32(payload, table->columns.size());    //push # of columns
        //for each column:
        for (const auto& col : table->columns) {
            final_payload_length += 4 + col.name.length() + 1; // for name length, name, and type
            push_u32(payload, col.name.length());
            push_str(payload, col.name);
            payload.push_back(static_cast<uint8_t>(col.type));
        }
    }
    payload[payload_length_pos] = final_payload_length & 0xFF;
    payload[payload_length_pos + 1] = (final_payload_length >> 8) & 0xFF;
    payload[payload_length_pos + 2] = (final_payload_length >> 16) & 0xFF;
    payload[payload_length_pos + 3] = (final_payload_length >> 24) & 0xFF;

    write(client_fd, payload.data(), payload.size());
}

void Session::send_row(std::vector<OutputTuple> &tuples, bool last_packet)
{
    std::vector<uint8_t> buf;
    buf.reserve(128);

    buf.push_back(static_cast<uint8_t>(StatusCode::ROW));
    buf.push_back(last_packet ? static_cast<uint8_t>(PacketFlag::LAST_PACKET) 
                              : static_cast<uint8_t>(PacketFlag::MORE_PACKETS));
    push_u32(buf, 0); // payload length placeholder

    // column count (total across all tuples)
    uint32_t column_count = 0;
    for (const auto& tuple : tuples)
        column_count += tuple.table_->columns.size();
    push_u32(buf, column_count);

    // column values
    for (const auto& tuple : tuples) {
        for (size_t i = 0; i < tuple.table_->columns.size(); i++) {
            std::string token = tuple.record.get_token(i, *tuple.table_);
            push_u32(buf, token.length());
            push_str(buf, token);
        }
    }

    // backfill payload length
    uint32_t payload_len = static_cast<uint32_t>(buf.size() - 6);
    memcpy(buf.data() + 2, &payload_len, 4);

    write(client_fd, buf.data(), buf.size());
}

void Session::send_affected(int count)
{
    (void)count;
}

void Session::send_metadata(const std::vector<TableDescriptor>& descriptors, bool more_packets)
{
    std::vector<uint8_t> buf;
    buf.reserve(128);

    buf.push_back(static_cast<uint8_t>(StatusCode::METADATA));
    buf.push_back(static_cast<uint8_t>(more_packets ? PacketFlag::MORE_PACKETS : PacketFlag::LAST_PACKET));
    push_u32(buf, 0); // payload length placeholder

    push_u32(buf, static_cast<uint32_t>(descriptors.size()));
    for (const auto& td : descriptors) {
        push_u32(buf, static_cast<uint32_t>(td.name.size()));
        push_str(buf, td.name);
        push_u32(buf, static_cast<uint32_t>(td.columns.size()));
        for (const auto& col : td.columns) {
            push_u32(buf, static_cast<uint32_t>(col.name.size()));
            push_str(buf, col.name);
            buf.push_back(static_cast<uint8_t>(col.type));
        }
    }

    uint32_t payload_len = static_cast<uint32_t>(buf.size() - 6);
    memcpy(buf.data() + 2, &payload_len, 4);
    write(client_fd, buf.data(), buf.size());
}

void Session::send_row_values(const std::vector<std::string>& values, bool last_packet)
{
    std::vector<uint8_t> buf;
    buf.reserve(128);

    buf.push_back(static_cast<uint8_t>(StatusCode::ROW));
    buf.push_back(last_packet ? static_cast<uint8_t>(PacketFlag::LAST_PACKET)
                              : static_cast<uint8_t>(PacketFlag::MORE_PACKETS));
    push_u32(buf, 0); // payload length placeholder

    push_u32(buf, static_cast<uint32_t>(values.size()));
    for (const auto& v : values) {
        push_u32(buf, static_cast<uint32_t>(v.size()));
        push_str(buf, v);
    }

    uint32_t payload_len = static_cast<uint32_t>(buf.size() - 6);
    memcpy(buf.data() + 2, &payload_len, 4);
    write(client_fd, buf.data(), buf.size());
}

void Session::send_aggregate(AggregateKind kind, const std::string& label, const std::string& value, bool last_packet)
{
    std::vector<uint8_t> buf;
    buf.reserve(64);

    buf.push_back(static_cast<uint8_t>(StatusCode::AGGREGATE));
    buf.push_back(static_cast<uint8_t>(last_packet ? PacketFlag::LAST_PACKET : PacketFlag::MORE_PACKETS));
    push_u32(buf, 0); // payload length placeholder

    buf.push_back(static_cast<uint8_t>(kind));
    push_u32(buf, static_cast<uint32_t>(label.size()));
    push_str(buf, label);
    push_u32(buf, static_cast<uint32_t>(value.size()));
    push_str(buf, value);

    uint32_t payload_len = static_cast<uint32_t>(buf.size() - 6);
    memcpy(buf.data() + 2, &payload_len, 4);
    write(client_fd, buf.data(), buf.size());
}

void Session::close_session(Server &server) {
    (void)server;
    if(client_fd != -1) {
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
        client_fd = -1;

    }
}








// ---------------------------------------------------------------------------
// In-process benchmark helpers
// ---------------------------------------------------------------------------

static unsigned int bench_seed(int offset = 0)
{
    auto ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    return static_cast<unsigned int>(ns) + static_cast<unsigned int>(offset);
}

static void run_bench(Executor& executor, int n_tables, int inserts_per_table)
{
    struct TableDef { int session_id; std::string name; int col_count; };
    const TableDef tables[3] = {
        {1, "users",    2},
        {2, "products", 3},
        {3, "messages", 3},
    };

    struct WorkerState {
        std::atomic<int>  done{0};
        unsigned int      seed = 0;
        Session           s;
        std::string       name;
        int               col_count = 0;
    };

    const int nt = (n_tables < 1) ? 1 : (n_tables > 3) ? 3 : n_tables;

    std::vector<std::unique_ptr<WorkerState>> states(nt);
    for (int i = 0; i < nt; i++) {
        states[i] = std::make_unique<WorkerState>();
        states[i]->seed      = bench_seed(i * 1000000);
        states[i]->s.session_id = tables[i].session_id;
        states[i]->s.client_fd  = -1;
        states[i]->s.silent     = true;
        states[i]->name      = tables[i].name;
        states[i]->col_count = tables[i].col_count;
    }

    std::atomic<bool> stop{false};

    auto make_query = [](const std::string& tbl, int /*cols*/, unsigned int seed) -> std::string {
        if (tbl == "users") {
            unsigned int uid = seed % 1'999'000'000u + 1u;
            return "insert into users (" + std::to_string(uid)
                 + ", \"u" + std::to_string(uid) + "\");";
        }
        if (tbl == "products") {
            unsigned int pid = seed % 1'999'000'000u + 1u;
            unsigned int qty = pid % 9999 + 1;
            std::string  nm  = "prod" + std::to_string(pid);
            if (nm.size() > 32) nm.resize(32);
            return "insert into products (" + std::to_string(pid)
                 + ", \"" + nm + "\", " + std::to_string(qty) + ");";
        }
        // messages
        unsigned int mid = seed % 1'999'000'000u + 1u;
        unsigned int sid = (seed ^ (seed >> 13)) % 1'999'000'000u + 1u;
        std::string  txt = "msg" + std::to_string(mid);
        if (txt.size() > 32) txt.resize(32);
        return "insert into messages (" + std::to_string(mid)
             + ", \"" + txt + "\", " + std::to_string(sid) + ");";
    };

    std::vector<std::thread> workers;
    workers.reserve(nt);
    for (int i = 0; i < nt; i++) {
        workers.emplace_back([&, i]() {
            WorkerState& ws = *states[i];
            while (!stop) {
                ws.seed = ws.seed * 1664525u + 1013904223u;
                executor.execute(make_query(ws.name, ws.col_count, ws.seed), ws.s);
                if (++ws.done >= inserts_per_table) stop = true;
            }
        });
    }

    // Print header
    std::cout << std::right << std::setw(6) << "Time" << " |";
    for (int i = 0; i < nt; i++)
        std::cout << std::setw(9) << states[i]->name << " |" << std::setw(7) << "QPS" << " |";
    std::cout << std::setw(9) << "Total" << " |" << std::setw(8) << "Avg QPS" << "\n";
    std::cout << std::string(14 + nt * 18, '-') << "\n";

    auto t0      = std::chrono::steady_clock::now();
    auto last_ts = t0;
    std::vector<int> last(nt, 0);

    while (!stop) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto   now      = std::chrono::steady_clock::now();
        double elapsed  = std::chrono::duration<double>(now - t0).count();
        double interval = std::chrono::duration<double>(now - last_ts).count();

        int total = 0;
        std::cout << std::fixed << std::setprecision(1) << std::setw(6) << elapsed << "s|";
        for (int i = 0; i < nt; i++) {
            int d    = states[i]->done.load();
            int qps  = (int)((d - last[i]) / interval);
            total   += d;
            std::cout << std::setw(9) << d << " |" << std::setw(7) << qps << " |";
            last[i] = d;
        }
        int avg = elapsed > 0 ? (int)(total / elapsed) : 0;
        std::cout << std::setw(9) << total << " |" << std::setw(8) << avg << "\n";
        last_ts = now;
    }

    for (auto& t : workers) t.join();

    double total_time = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - t0).count();
    int grand = 0;
    for (int i = 0; i < nt; i++) grand += states[i]->done.load();
    std::cout << "\n[bench] tables=" << nt << " total=" << grand
              << " time=" << std::fixed << std::setprecision(2) << total_time << "s"
              << " avg_qps=" << (int)(grand / total_time) << "\n";
}

// ---------------------------------------------------------------------------

void Session::run_admin(Server& server) {
    std::string line;
    
    // 1. THE INITIAL PROMPT: Print this before the loop ever starts
    std::cout << "Admin CLI ready. Type 'exit' to quit.\n" << "\n";
    std::cout << "admin>";

    while (std::getline(std::cin, line)) {
        if (line == "exit") {
            break;
        }

        if (!line.empty()) {
            // bench <tables> <rows_per_table>
            // e.g.  bench 1 50000   bench 2 100000   bench 3 300000
            //server.database.wal->sync_every_commit = true; // Ensure durability for admin-run benchmarks
            std::istringstream ss(line);
            std::string cmd;
            ss >> cmd;
            if (cmd == "bench") {
                int tables = 3, rows = 50000;
                ss >> tables >> rows;
                std::cout << "[bench] starting: tables=" << tables
                          << " rows_per_table=" << rows << "\n";
                run_bench(server.executor, tables, rows);
                std::cout << "admin> ";
                continue;
            }

            auto start_time = std::chrono::high_resolution_clock::now();
            
            server.executor.execute(line, *this);
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

            std::cout << "\nExecution time: " << elapsed.count() << " µs\nadmin> " << "\n";
        }
    }
}

