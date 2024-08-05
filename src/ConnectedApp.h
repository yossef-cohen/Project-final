#ifndef CONNECTEDAPP_H
#define CONNECTEDAPP_H
#include "CommonObject.h"


class ConnectedApp {
public:
    ConnectedApp();
    ~ConnectedApp();

    void Run();

private:
    CommonObjects commonObjects;
    void Initialize();
    void Cleanup();
};

#endif // CONNECTEDAPP_H
