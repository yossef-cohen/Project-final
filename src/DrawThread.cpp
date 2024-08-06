
#include "ImageCache.h"
#include "DrawThread.h"
#include "../vendor/ImGui/imgui.h"
#include "GuiMain.h"
#include <iostream>
#include <algorithm>
#include <curl/curl.h>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include <../GL/glew.h>
#include <GL.h>
#include <stb_image.h>
#include <algorithm>


void DrawThread::operator()(CommonObjects& common) {
    GuiMain(DrawFunction, &common);
    common.exit_flag = true;
}




void DrawThread::DrawFunction(void* common_ptr) {
    auto common = static_cast<CommonObjects*>(common_ptr);
    if (!common) {
        std::cerr << "CommonObjects pointer is null" << std::endl;
        return;
    }

    ImGui::Begin("Movie Catalog", nullptr, ImGuiWindowFlags_NoScrollbar);

    static char buff[200];
    static std::vector<Movie> filteredMovies;

    ImGui::InputText("Search", buff, sizeof(buff));
    ImGui::SameLine();
    if (ImGui::Button("Get")) {
        std::string searchStr = toLower(std::string(buff));
        filteredMovies.clear();
        for (const auto& movie : common->Movies) {
            std::string movieTitleLower = toLower(movie.Title);
            if (movieTitleLower.find(searchStr) != std::string::npos) {
                filteredMovies.push_back(movie);
            }
        }
    }

    Draw(common, filteredMovies);

    ImGui::End();
}

void DrawThread::Draw(CommonObjects* common, const std::vector<Movie>& filteredMovies) {
    static ImageCache imageCache;

    ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    int movies_per_row = 6;
    float poster_width = ImGui::GetContentRegionAvail().x / movies_per_row - 10;
    float poster_height = poster_width * 1.5f;

    const auto& movies_to_display = filteredMovies.empty() ? common->Movies : filteredMovies;
    int movies_to_show = std::min<int>(static_cast<int>(movies_to_display.size()), common->loaded_movies_count.load());
    if (!common->loading_complete) {
        float progress = static_cast<float>(common->loaded_movies_count) / common->Movies.size();
        ImGui::ProgressBar(progress, ImVec2(-1, 0));
        ImGui::Text("Loading... %d / %d", common->loaded_movies_count.load(), common->Movies.size());
    }
    for (int i = 0; i < movies_to_show; i++) {
        const auto& movie = movies_to_display[i];

            ImGui::PushID(i);
            if (i % movies_per_row != 0) {
                ImGui::SameLine();
            }

            ImGui::BeginGroup();

            // Get the image from the cache
            GLuint textureID = imageCache.GetTexture(movie.Poster);
            if (textureID != 0) {
                if (ImGui::ImageButton((void*)(intptr_t)textureID, ImVec2(poster_width, poster_height))) {
                    // Handle button click
                }
            }
            else {
                ImGui::Button("Loading...", ImVec2(poster_width, poster_height));
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
                ImGui::OpenPopup("MovieDescription");
            }

            ImGui::TextWrapped("%s", movie.Title.c_str());
            ImGui::Text("Rating:%s | Duration:%s", movie.Rating.c_str(), movie.Duration.c_str());
            ImGui::Text("Genre:%s", movie.Genre.c_str());

            if (ImGui::BeginPopup("MovieDescription")) {
                ImGui::Text("%s", movie.Title.c_str());
                ImGui::Separator();
                ImGui::TextWrapped("%s", movie.Description.c_str());
                ImGui::EndPopup();
            }

            ImGui::EndGroup();
            ImGui::PopID();

            float last_button_x2 = ImGui::GetItemRectMax().x;
            float next_button_x2 = last_button_x2 + ImGui::GetStyle().ItemSpacing.x + poster_width;
            if (i + 1 < movies_to_display.size() && next_button_x2 < window_visible_x2) {
                ImGui::SameLine();
            }
    }



    ImGui::EndChild();
}

std::string DrawThread::toLower(const std::string& str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return lowerStr;
}

// Callback function to write data into a vector
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::vector<unsigned char>*)userp)->insert(((std::vector<unsigned char>*)userp)->end(), (unsigned char*)contents, (unsigned char*)contents + size * nmemb);
    return size * nmemb;
}

//std::vector<unsigned char> DrawThread::DownloadImage(const char* url) {
//    std::string server_url = "http://127.0.0.1:8080/image?url=";
//    server_url += url;  // Append the image URL to the server endpoint
//
//    CURL* curl;
//    CURLcode res;
//    std::vector<unsigned char> buffer;
//
//    curl_global_init(CURL_GLOBAL_DEFAULT);
//    curl = curl_easy_init();
//    if (curl) {
//        curl_easy_setopt(curl, CURLOPT_URL, server_url.c_str());
//        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
//        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
//
//        res = curl_easy_perform(curl);
//        if (res != CURLE_OK) {
//            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
//        }
//
//        curl_easy_cleanup(curl);
//    }
//    curl_global_cleanup();
//
//    return buffer;
//}

GLuint DrawThread::LoadTextureFromMemory(const unsigned char* data, int dataSize, int* width, int* height) {
    int texWidth, texHeight, channels;
    unsigned char* imgData = stbi_load_from_memory(data, dataSize, &texWidth, &texHeight, &channels, 4); // Force RGBA

    if (imgData == nullptr) {
        // Handle error
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

    stbi_image_free(imgData);

    if (width) *width = texWidth;
    if (height) *height = texHeight;

    return texture;
}


