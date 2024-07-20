#pragma once

#include "httplib.h"
#include <iostream>
#include <string>
#include <mutex>
#include <fstream>
#include <filesystem>

#ifndef INPUT_WITH_SERVER_H
#define INPUT_WITH_SERVER_H

class ServerWithInput {
private:
    httplib::Server svr_;
    std::string server_content_;
    std::mutex content_mutex_;
 
public:
    void getLinesFromJSON();
    void httpServer();
    void start();
    void stopTheServer();
};

#endif // INPUT_WITH_SERVER_H
