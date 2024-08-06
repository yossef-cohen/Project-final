#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <vector>
#include <thread>
#include "ServerSide/ServerWithInput.h"
#include <CommonObject.h>
#include <CommonObject.h>
#include <GuiMain.h>
#include <DrawThread.h>
#include <ConnectedApp.h>
#include "DownloadThread.h"

int main() {
    curl_global_init(CURL_GLOBAL_ALL);

    try {
        CommonObjects commonObjects;

        // Start the DownloadThread
        DownloadThread downloadThread;
        std::thread download_thread(std::ref(downloadThread), std::ref(commonObjects));

        // Run the GUI in a separate thread
        std::thread gui_thread(GuiMain, &DrawThread::DrawFunction, &commonObjects);

        // Wait for the download thread to complete
        if (download_thread.joinable()) {
            download_thread.join();
        }

        // Join the GUI thread
        if (gui_thread.joinable()) {
            gui_thread.join();
        }
        else {
            std::cerr << "GUI Main thread was not joinable." << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return 1;
    }

    curl_global_cleanup();
    return 0;
}
