#pragma once
#ifndef CONNECTEDAPP_H
#define CONNECTEDAPP_H
#include "CommonObject.h"


class ConnectedApp {
public:
    ConnectedApp();
    ~ConnectedApp();
    CommonObjects commonObjects;

    void Run();

private:
    void Initialize();
    void Cleanup();
};

#endif // CONNECTEDAPP_H
