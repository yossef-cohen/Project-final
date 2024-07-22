#include "DrawThread.h"
#include "../vendor/ImGui/imgui.h"
#include "GuiMain.h"
#include <iostream>
#include <unordered_map>
#include <curl/curl.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


struct ImageData {
    unsigned char* data;
    int width;
    int height;
    int channels;
    GLuint texture;
    std::atomic<bool> isLoading{ false };
    std::atomic<bool> isLoaded{ false };
};

std::unordered_map<std::string, std::shared_ptr<ImageData>> g_TextureCache;
std::mutex g_TextureCacheMutex;
std::queue<std::string> g_ImageLoadQueue;
std::mutex g_ImageLoadQueueMutex;
std::atomic<bool> g_ImageLoaderRunning{ true };

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void LoadTextureFromURL(const std::string& url, std::shared_ptr<ImageData> imageData) {
    CURL* curl = curl_easy_init();
    if (curl) {
        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            int width, height, channels;
            unsigned char* data = stbi_load_from_memory(
                (unsigned char*)response.c_str(), response.size(),
                &width, &height, &channels, 4);

            if (data) {
                imageData->data = data;
                imageData->width = width;
                imageData->height = height;
                imageData->channels = 4;
                imageData->isLoaded = true;
            }
        }
    }
    imageData->isLoading = false;
}

void ImageLoaderThread() {
    while (g_ImageLoaderRunning) {
        std::string url;
        {
            std::lock_guard<std::mutex> lock(g_ImageLoadQueueMutex);
            if (!g_ImageLoadQueue.empty()) {
                url = g_ImageLoadQueue.front();
                g_ImageLoadQueue.pop();
            }
        }

        if (!url.empty()) {
            std::shared_ptr<ImageData> imageData;
            {
                std::lock_guard<std::mutex> lock(g_TextureCacheMutex);
                imageData = g_TextureCache[url];
            }
            LoadTextureFromURL(url, imageData);
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void DrawAppWindow(void* common_ptr) {
    auto common = reinterpret_cast<CommonObjects*>(common_ptr);
    if (!common) {
        std::cerr << "CommonObjects pointer is null" << std::endl;
        return;
    }

    ImGui::Begin("Movie Catalog", nullptr, ImGuiWindowFlags_NoScrollbar);

    static char buff[200];
    ImGui::InputText("Search", buff, sizeof(buff));
    ImGui::SameLine();
    if (ImGui::Button("Get")) {
        common->url = buff;
    }

    if (common->data_ready) {
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        int movies_per_row = 6;
        float poster_width = ImGui::GetContentRegionAvail().x / movies_per_row - 10;
        float poster_height = poster_width * 1.5f;

        for (int i = 0; i < common->Movies.size(); i++) {
            const auto& movie = common->Movies[i];

            ImGui::PushID(i);
            if (i % movies_per_row != 0) {
                ImGui::SameLine();
            }

            ImGui::BeginGroup();

            std::shared_ptr<ImageData> imageData;
            {
                std::lock_guard<std::mutex> lock(g_TextureCacheMutex);
                if (g_TextureCache.find(movie.Poster) == g_TextureCache.end()) {
                    g_TextureCache[movie.Poster] = std::make_shared<ImageData>();
                    std::lock_guard<std::mutex> queueLock(g_ImageLoadQueueMutex);
                    g_ImageLoadQueue.push(movie.Poster);
                }
                imageData = g_TextureCache[movie.Poster];
            }

            ImTextureID tex_id = reinterpret_cast<ImTextureID>(0);
            if (imageData->isLoaded && imageData->texture == 0) {
                glGenTextures(1, &imageData->texture);
                glBindTexture(GL_TEXTURE_2D, imageData->texture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageData->width, imageData->height, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, imageData->data);
                glBindTexture(GL_TEXTURE_2D, 0);
                stbi_image_free(imageData->data);
                imageData->data = nullptr;
            }

            if (imageData->isLoaded) {
                tex_id = reinterpret_cast<ImTextureID>(static_cast<intptr_t>(imageData->texture));
            }

            if (ImGui::ImageButton(tex_id, ImVec2(poster_width, poster_height), ImVec2(0, 0), ImVec2(1, 1), 0)) {
                ImGui::OpenPopup("MovieDescription");
            }
            ImGui::TextWrapped("%s", movie.Title.c_str());
            ImGui::Text("%s | %s", movie.Rating.c_str(), movie.Duration.c_str());
            ImGui::Text("%s", movie.Genre.c_str());
            ImGui::EndGroup();

            if (ImGui::BeginPopup("MovieDescription")) {
                ImGui::Text("%s", movie.Title.c_str());
                ImGui::Separator();
                ImGui::TextWrapped("%s", movie.Description.c_str());
                ImGui::EndPopup();
            }

            ImGui::PopID();

            float last_button_x2 = ImGui::GetItemRectMax().x;
            float next_button_x2 = last_button_x2 + ImGui::GetStyle().ItemSpacing.x + poster_width;
            if (i + 1 < common->Movies.size() && next_button_x2 < window_visible_x2) {
                ImGui::SameLine();
            }
        }

        ImGui::EndChild();
    }

    ImGui::End();
}

void DrawThread::operator()(CommonObjects& common) {
    std::thread imageLoaderThread(ImageLoaderThread);
    GuiMain(DrawAppWindow, &common);
    g_ImageLoaderRunning = false;
    imageLoaderThread.join();
    common.exit_flag = true;
}