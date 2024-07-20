#include "input_with_server.h"
#include <nlohmann/json.hpp>

std::atomic<bool> server_running(true);

void ServerWithInput::getLinesFromJSON() {
    std::string filename = "movie_data.json";
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        svr_.stop();
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json_string = buffer.str();

    {
        std::lock_guard<std::mutex> lock(content_mutex_);
        server_content_ = std::move(json_string);
    }

    std::cout << "JSON file loaded. Size: " << server_content_.size() << " bytes." << std::endl;
}

void ServerWithInput::httpServer() {
    // when some one get to the URL they will get the data that was writen in server_content_
    svr_.Get("/", [this](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(content_mutex_);
        res.set_content(server_content_, "text/plain");
        });

    std::cout << "Starting server on http://localhost:8080/\n";
    svr_.listen("127.0.0.1", 8080); // open the server
}

void ServerWithInput::start() {
    // threads 
    std::thread server_thread(&ServerWithInput::httpServer, this);
    std::thread input_thread(&ServerWithInput::getLinesFromJSON, this);
    server_thread.join();
    input_thread.join();
}

void ServerWithInput::stopTheServer() {
    svr_.stop();
}