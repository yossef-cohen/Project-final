#include <iostream>
#include <thread>
#include "CommonObject.h"
#include "DrawThread.h"
#include "DownloadThread.h"
#include "input_with_server.h"


int main()
{
    CommonObjects common;
    DrawThread draw;

    auto draw_th = std::jthread([&] {draw(common); });

    DownloadThread down;
    auto down_th = std::jthread([&] {down(common); });
    down.SetUrl("http://....");
    std::cout << "running...\n";
    down_th.join();
    draw_th.join();
}

