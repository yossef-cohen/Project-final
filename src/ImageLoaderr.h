#pragma once
#include <iostream>

class ImageLoader {
public:
    static void Initialize();
    static void Shutdown();
    static void QueueImageForLoading(const std::string& url);
    static ImTextureID GetLoadedTexture(const std::string& url);
    // ... other necessary methods
};