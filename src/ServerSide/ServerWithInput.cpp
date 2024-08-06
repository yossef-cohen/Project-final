#include "ServerWithInput.h"
#include <iostream>
#include <fstream>
#include <sstream>

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
    svr_.Post("/download", [](const httplib::Request& req, httplib::Response& res) {
        auto url = req.body;
        // Perform the download using the URL
        // This is a placeholder for downloading logic; replace with actual implementation
        std::string response_body = "Downloaded image data from URL: " + url;
        res.set_content(response_body, "text/plain");
        });

    svr_.Get("/", [this](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(content_mutex_);
        res.set_content(server_content_, "text/plain");
        });

    std::cout << "Starting server on http://localhost:8080/\n";
    svr_.listen("127.0.0.1", 8080); // Open the server
}

void ServerWithInput::start() {
    std::thread server_thread(&ServerWithInput::httpServer, this);
    std::thread input_thread(&ServerWithInput::getLinesFromJSON, this);
    input_thread.join();
    server_thread.join();
}

void ServerWithInput::stopTheServer() {
    svr_.stop();
}
