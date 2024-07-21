#include "DrawThread.h"
#include "../vendor/ImGui/imgui.h"
#include "GuiMain.h"
#include <iostream>
#include <unordered_map>
#include <curl/curl.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GL/gl.h>
#include <GL/glu.h>


// Structure to hold image data
struct ImageData {
    unsigned char* data;
    int width;
    int height;
    int channels;
    GLuint texture;
};

// Global map to store loaded textures
std::unordered_map<std::string, ImageData> g_TextureCache;

// Function to write data received from CURL
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to load image from URL
bool LoadTextureFromURL(const std::string& url, ImageData& imageData) {
    CURL* curl = curl_easy_init();
    if (curl) {
        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK) {
            imageData.data = stbi_load_from_memory(
                (unsigned char*)response.c_str(), response.size(),
                &imageData.width, &imageData.height, &imageData.channels, 0);

            if (imageData.data) {
                glGenTextures(1, &imageData.texture);
                glBindTexture(GL_TEXTURE_2D, imageData.texture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageData.width, imageData.height, 0,
                    GL_RGBA, GL_UNSIGNED_BYTE, imageData.data);
                glBindTexture(GL_TEXTURE_2D, 0);
                return true;
            }
        }
    }
    return false;
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

            // Load and cache the texture if not already loaded
            if (g_TextureCache.find(movie.Poster) == g_TextureCache.end()) {
                ImageData imageData;
                if (LoadTextureFromURL(movie.Poster, imageData)) {
                    g_TextureCache[movie.Poster] = imageData;
                }
            }

            // Use the cached texture
            ImTextureID tex_id = g_TextureCache.count(movie.Poster) ?
                reinterpret_cast<ImTextureID>(static_cast<intptr_t>(g_TextureCache[movie.Poster].texture)) :
                reinterpret_cast<ImTextureID>(0);

            if (ImGui::ImageButton(tex_id, ImVec2(poster_width, poster_height), ImVec2(0, 0), ImVec2(1, 1), 0)) {
                ImGui::OpenPopup("MovieDescription");
            }
            ImGui::TextWrapped("%s", movie.Title.c_str());
            ImGui::Text("%s | %s", movie.Rating.c_str(), movie.Duration.c_str());
            ImGui::Text("%s", movie.Genre.c_str());
            ImGui::EndGroup();

            // Movie description popup
            if (ImGui::BeginPopup("MovieDescription")) {
                ImGui::Text("%s", movie.Title.c_str());
                ImGui::Separator();
                ImGui::TextWrapped("%s", movie.Description.c_str());
                ImGui::EndPopup();
            }

            ImGui::PopID();

            // Wrap to next row
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
    GuiMain(DrawAppWindow, &common);
    common.exit_flag = true;
}