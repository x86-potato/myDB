#include "server.hpp"


void Server::start() {
    std::cout << "Listening on port 5432" << std::endl;

    std::thread ([this]() {
        admin_session.session_id = 0; // Admin session has a reserved ID of 0
        admin_session.run_admin(*this);
    }).detach();

    //std::cout << "Admin CLI ready. Type 'exit' to quit." << std::endl;


    //current_session_id = 1;
    server_fd = socket(AF_INET, SOCK_STREAM, 0); 
    if (server_fd == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    int opt = 1;
    if (setsockopt(
        server_fd, 
        SOL_SOCKET, 
        SO_REUSEADDR | SO_REUSEPORT, 
        &opt, 
        sizeof(opt)) == -1)
    {
        std::cerr << "Failed to set socket options" << std::endl;
        return;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(5432);

    if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        return;
    }

    if(listen(server_fd, 3) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        return;
    }
    
    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            std::cerr << "Failed to accept connection" << std::endl;
            continue;
        }

        add_session(client_fd);

    }
}


void Server::add_session(int client_fd) {
    std::lock_guard<std::mutex> lock(session_mutex);

    int new_session_id = session_id_counter.fetch_add(1);

    auto new_session = std::make_shared<Session>();
    new_session->session_id = new_session_id;
    new_session->client_fd = client_fd;
    active_sessions[new_session_id] = new_session;

    std::thread ([new_session, this]() {
        new_session->run(*this);
        remove_session(new_session->session_id);
    }).detach();

}


void Server::remove_session(int session_id) {
    std::lock_guard<std::mutex> lock(session_mutex);

    auto it = active_sessions.find(session_id);
    if (it != active_sessions.end()) {
        active_sessions.erase(it);
        std::cout << "Session with ID " << session_id << " removed." << std::endl;
    } else {
        std::cout << "Session with ID " << session_id << " not found." << std::endl;
    }
}