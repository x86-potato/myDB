#include "session.hpp"
#include "server.hpp"

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
        std::cout << "Received query from session " << session_id << ": " << query << std::endl;
        //current_transaction_id = server.database.create_transaction();
        server.executor.execute(query, *this);
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

    uint8_t header[6] = {
        static_cast<uint8_t>(StatusCode::ERROR),
        static_cast<uint8_t>(PacketFlag::LAST_PACKET),
        0, 0, 0, 0  // payload_length = 0
    };
    write(client_fd, header, sizeof(header));
}

void Session::send_metadata(std::vector<const Table*> tables)
{
    std::vector<uint8_t> payload;


    //status code
    payload.push_back(static_cast<uint8_t>(StatusCode::METADATA));

    //flag
    payload.push_back(static_cast<uint8_t>(PacketFlag::LAST_PACKET));

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
{}

void Session::close_session(Server &server) {
    if(client_fd != -1) {
        shutdown(client_fd, SHUT_RDWR);
        close(client_fd);
        client_fd = -1;

    }
}








void Session::run_admin(Server& server) {
    std::string line;
    
    // 1. THE INITIAL PROMPT: Print this before the loop ever starts
    std::cout << "Admin CLI ready. Type 'exit' to quit.\n" << std::flush;
    std::cout << "admin>";

    while (std::getline(std::cin, line)) {
        if (line == "exit") {
            break;
        }

        if (!line.empty()) {  
            auto start_time = std::chrono::high_resolution_clock::now();
            
            server.executor.execute(line, *this);
            
            auto end_time = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

            std::cout << "\nExecution time: " << elapsed.count() << " µs\nadmin> " << std::flush;

        }
    }
}

