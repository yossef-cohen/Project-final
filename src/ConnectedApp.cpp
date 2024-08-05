#include "ConnectedApp.h"
#include "GuiMain.h"
#include "DrawThread.h"
#include <iostream>
#include <DownloadThread.h>
#include <thread>
#include <curl/system.h>

ConnectedApp::ConnectedApp() {
    Initialize();
}

ConnectedApp::~ConnectedApp() {
    Cleanup();
}

void ConnectedApp::Initialize() {
    // Perform any necessary initialization for CommonObjects
    commonObjects.data_ready = false;
}

void ConnectedApp::Cleanup() {
    // Perform any necessary cleanup for CommonObjects
}

void ConnectedApp::Run() {
    // Initialize and start the download thread
    DownloadThread downloadThread;
    downloadThread.SetUrl("http://example.com/movies"); // Set your URL
    std::thread downloadThreadInstance([&]() {
        downloadThread(commonObjects);
        });

    // Run the GUI
    int guiResult = GuiMain(DrawThread::DrawFunction, &commonObjects);
    if (guiResult != 0) {
        std::cerr << "GUI Main failed with result code: " << guiResult << std::endl;
    }

    // Wait for download thread to complete
    downloadThreadInstance.join();
}


int main() {
 
    ConnectedApp app;
    app.Run();
    return 0;
}
