#pragma once

#include <unordered_map>
#include <string>
#include <mutex>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <../GL/glew.h>
#include "stb_image.h"

class ImageCache {
public:
    ImageCache() {}
    ~ImageCache() {}

    GLuint GetTexture(const std::string& movieTitle) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = cache.find(movieTitle);
        if (it != cache.end()) {
            return it->second;
        }

        // Check if the image exists in the "posters" folder
        std::filesystem::path localImagePath = std::filesystem::path("posters") / (movieTitle + ".jpg");
        if (std::filesystem::exists(localImagePath)) {
            GLuint textureID = LoadTextureFromFile(localImagePath.string());
            if (textureID != 0) {
                cache[movieTitle] = textureID;
                return textureID;
            }
        }

        // If the image doesn't exist locally, return 0
        return 0;
    }

private:
    std::unordered_map<std::string, GLuint> cache;
    std::mutex mutex_;

    GLuint LoadTextureFromFile(const std::string& filepath) {
        int texWidth, texHeight, channels;
        unsigned char* imgData = stbi_load(filepath.c_str(), &texWidth, &texHeight, &channels, 4); // Force RGBA

        if (imgData == nullptr) {
            std::cerr << "Failed to load image from file: " << filepath << std::endl;
            return 0;
        }

        GLuint texture = CreateTextureFromData(imgData, texWidth, texHeight);
        stbi_image_free(imgData);
        return texture;
    }

    GLuint CreateTextureFromData(unsigned char* imgData, int texWidth, int texHeight) {
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenerateMipmap(GL_TEXTURE_2D);

        return texture;
    }
};