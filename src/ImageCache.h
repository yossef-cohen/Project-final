#pragma once
#include <unordered_map>
#include <../GL/glew.h>
#include <string>
#include <iostream>
#include <curl/curl.h>
#include "DrawThread.h"

class ImageCache {
public:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::vector<unsigned char>*)userp)->insert(
            ((std::vector<unsigned char>*)userp)->end(),
            (unsigned char*)contents,
            (unsigned char*)contents + size * nmemb
        );
        return size * nmemb;
    }

    GLuint GetTexture(const std::string& url) {
        auto it = cache.find(url);
        if (it != cache.end()) {
            return it->second;
        }

        std::vector<unsigned char> imageData = DownloadImage(url.c_str());
        if (imageData.empty()) {
            std::cerr << "Failed to download image: " << url << std::endl;
            return 0;
        }

        int texWidth, texHeight;
        GLuint textureID = LoadTextureFromMemory(imageData.data(), imageData.size(), &texWidth, &texHeight);
        if (textureID == 0) {
            std::cerr << "Failed to load texture from image data" << std::endl;
            return 0;
        }

        cache[url] = textureID;
        return textureID;
    }

private:
 
    std::unordered_map<std::string, GLuint> cache;

    std::vector<unsigned char> DownloadImage(const char* url) {
        CURL* curl;
        CURLcode res;
        std::vector<unsigned char> buffer;

        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
            curl_easy_setopt(curl, CURLOPT_CAINFO, "HttpSrc/cacert.pem");

            res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            }

            curl_easy_cleanup(curl);
        }
        curl_global_cleanup();

        return buffer;
    }

    GLuint LoadTextureFromMemory(const unsigned char* data, int dataSize, int* width, int* height) {
        int texWidth, texHeight, channels;
        unsigned char* imgData = stbi_load_from_memory(data, dataSize, &texWidth, &texHeight, &channels, 4); // Force RGBA

        if (imgData == nullptr) {
            std::cerr << "Failed to load image from memory: " << stbi_failure_reason() << std::endl;
            return 0;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgData);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(imgData);

        if (width) *width = texWidth;
        if (height) *height = texHeight;

        return texture;
    }
};
