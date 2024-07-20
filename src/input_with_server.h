#pragma once

#include "httplib.h"
#include <iostream>
#include <string>
#include <mutex>
#include <fstream>
#include <filesystem>

#ifndef INPUT_WITH_SERVER_H
#define INPUT_WITH_SERVER_H

extern std::atomic<bool> server_running;

class ServerWithInput {
private:
    httplib::Server svr_;
    std::string server_content_;
    std::mutex content_mutex_;
    void getLinesFromJSON();
    void httpServer();
public:
    void start();
};

#endif // INPUT_WITH_SERVER_H
