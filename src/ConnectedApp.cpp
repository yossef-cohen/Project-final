#include <iostream>
#include <curl/curl.h>
#include <ServerSide/nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <vector>
#include <thread>
#include "ServerSide/ServerWithInput.h"
#include <CommonObject.h>
#include <GuiMain.h>
#include <DrawThread.h>
#include "DownloadThread.h"
#include <Windows.h>


int main() {
    CommonObjects commonObjects; //object
    try{
 STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    // Unicode strings for server path and working directory
    std::wstring serverPath = L"src\\ServerSide\\build\\Release\\server.exe";
    std::wstring workingDirectory = L"src\\ServerSide\\build\\Release";

    // Try to start the server process
    BOOL result = CreateProcessW(
        NULL,                          // No module name (use command line)
        const_cast<LPWSTR>(serverPath.c_str()), // Command line (LPWSTR is used here)
        NULL,                          // Process handle not inheritable
        NULL,                          // Thread handle not inheritable
        FALSE,                         // Set handle inheritance to FALSE
        0,                             // No creation flags
        NULL,                          // Use parent's environment block
        workingDirectory.c_str(),      // Set working directory
        &si,                           // Pointer to STARTUPINFO structure
        &pi                            // Pointer to PROCESS_INFORMATION structure
    );

    if (!result) {
        std::cerr << "CreateProcess failed (" << GetLastError() << ").\n";
        return 1;
    }

    std::cout << "Server started successfully.\n";
    Sleep(100);

    curl_global_init(CURL_GLOBAL_ALL);

    try {
        CommonObjects commonObjects;
        DownloadThread downloadThread;
        std::thread download_thread(std::ref(downloadThread), std::ref(commonObjects));
        std::thread gui_thread(GuiMain, &DrawThread::DrawFunction, &commonObjects);

        if (download_thread.joinable()) {
            download_thread.join();
        }
        if (gui_thread.joinable()) {
            gui_thread.join();
        }
        else {
            std::cerr << "GUI Main thread was not joinable." << std::endl;
        }
    }
    catch (const std::length_error& e) {
        std::cerr << "std::length_error caught in main execution: " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception caught in main execution: " << e.what() << std::endl;
    }

    curl_global_cleanup();

 // Wait until child process exits
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Close process and thread handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    std::cout << "Server has stopped.\n";

}
    catch (const std::exception& e) {
    std::cerr << "Exception caught in main function: " << e.what() << std::endl;
}
    return 0;
}
