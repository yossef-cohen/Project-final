#ifndef SERVERWITHINPUT_H
#define SERVERWITHINPUT_H

#include "httplib/httplib.h"
#include <string>
#include <mutex>
#include <atomic>

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

#endif // SERVERWITHINPUT_H
