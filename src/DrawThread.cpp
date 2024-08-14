#include "imgui.h"        
#include "ImageCache.h"
#include "DrawThread.h"
#include "GuiMain.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <curl/curl.h>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include <../GL/glew.h>
#include <../GL/GL.h>
#include <stb_image.h>


void DrawThread::operator()(CommonObjects& common) {
    GuiMain(DrawFunction, &common);
    common.exit_flag = true;
}


void DrawThread::SortMovies(std::vector<Movie>& movies, int sortOption) {
    if (sortOption == 0) {
        std::sort(movies.begin(), movies.end(), [](const Movie& a, const Movie& b) {
            return a.Title < b.Title;
            });
    }
    else if (sortOption == 1) {
        std::sort(movies.begin(), movies.end(), [](const Movie& a, const Movie& b) {
            return std::stoi(a.Year) > std::stoi(b.Year);
            });
    }
    else if (sortOption == 2) {
        std::sort(movies.begin(), movies.end(), [](const Movie& a, const Movie& b) {
            return std::stof(a.Rating) > std::stof(b.Rating);
            });
    }
    else if (sortOption == 3) {
        std::sort(movies.begin(), movies.end(), [](const Movie& a, const Movie& b) {
            return std::stof(a.Duration) > std::stof(b.Duration);
            });
    }
}

void DrawThread::DrawFunction(void* common_ptr) {
    auto common = static_cast<CommonObjects*>(common_ptr);
    if (!common) {
        std::cerr << "CommonObjects pointer is null" << std::endl;
        return;
    }

    ImGui::Begin("Movie Catalog", nullptr, ImGuiWindowFlags_NoScrollbar);

    static char buff[200] = "";
    static std::vector<Movie> filteredMovies;
    static int sortOption = 0; // 0: Sort by Title, 1: Sort by Year, 2: Sort by Rating
    static bool showRadioButtons = false;
    static int currentPage = 0;
    static const int itemsPerPage = 54; // Changed to 54

    // Search and sort movies
    auto performSearchAndSort = [&]() {
        std::string searchStr = toLower(std::string(buff));
        filteredMovies.clear();
        for (const auto& movie : common->Movies) {
            std::string movieTitleLower = toLower(movie.Title);
            if (movieTitleLower.find(searchStr) != std::string::npos) {
                filteredMovies.push_back(movie);
            }
        }
        SortMovies(filteredMovies, sortOption);
        currentPage = 0; // Reset to first page after search/sort
        };

    // Adjust search bar and button widths
    float window_width = ImGui::GetWindowWidth();
    float search_bar_width = 400.0f;
    float button_width = 70.0f;
    float total_width = search_bar_width + button_width + ImGui::GetStyle().ItemSpacing.x;
    float indent = (window_width - total_width) * 0.5f;
    ImGui::SetCursorPosX(indent);

    // Search bar and button
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    ImGui::PushItemWidth(search_bar_width);
    bool search_triggered = ImGui::InputText("##SearchInput", buff, sizeof(buff), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopItemWidth();
    ImGui::PopStyleVar(2);
    ImGui::SameLine();
    if (ImGui::Button("Search", ImVec2(button_width, ImGui::GetFrameHeight() + 10.0f)) || search_triggered) {
        performSearchAndSort();
    }

    // Sort button
    ImGui::SameLine();
    if (ImGui::Button("Sort", ImVec2(button_width, ImGui::GetFrameHeight() + 10.0f))) {
        showRadioButtons = !showRadioButtons;
    }

    // Radio buttons for sorting
    if (showRadioButtons) {
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);
        if (ImGui::RadioButton("Title", &sortOption, 0)) performSearchAndSort();
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);
        if (ImGui::RadioButton("Year", &sortOption, 1)) performSearchAndSort();
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);
        if (ImGui::RadioButton("Rating", &sortOption, 2)) performSearchAndSort();
        ImGui::SameLine();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5.0f);
        if (ImGui::RadioButton("Duration", &sortOption, 3)) performSearchAndSort();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5.0f);
    }
    // Pagination
    const int totalItems = filteredMovies.empty() ? common->Movies.size() : filteredMovies.size();
    const int totalPages = (totalItems + itemsPerPage - 1) / itemsPerPage;

    // Ensure currentPage is within valid range
    if (currentPage >= totalPages) {
        currentPage = totalPages > 0 ? totalPages - 1 : 0;
    }

    ImGui::Spacing();
    ImGui::Text("Page %d of %d", totalPages > 0 ? currentPage + 1 : 1, std::max<int>(1, totalPages));

    if (ImGui::Button("Previous") && currentPage > 0) currentPage--;
    ImGui::SameLine();
    if (ImGui::Button("Next") && currentPage < totalPages - 1) currentPage++;

    // Render movies for current page
    const int startIdx = currentPage * itemsPerPage;
    const int endIdx = std::min<int>(startIdx + itemsPerPage, totalItems);

    std::vector<Movie> pageMovies;
    if (filteredMovies.empty()) {
        pageMovies.assign(common->Movies.begin() + startIdx, common->Movies.begin() + endIdx);
    }
    else {
        pageMovies.assign(filteredMovies.begin() + startIdx, filteredMovies.begin() + endIdx);
    }

    Draw(common, pageMovies);

    ImGui::End();
}


void DrawThread::Draw(CommonObjects* common, const std::vector<Movie>& filteredMovies) {
    static ImageCache imageCache;

    ImGui::GetIO().FontGlobalScale = 1.4f;

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

        GLuint textureID = imageCache.GetTexture(movie.Title);
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
        ImGui::Text("Rating:%s | Duration:%s | Year:%s", movie.Rating.c_str(), movie.Duration.c_str(), movie.Year.c_str());
        ImGui::Text("Genre:%s", movie.Genre.c_str());

        if (ImGui::BeginPopup("MovieDescription")) {
            ImGui::Text("%s", movie.Title.c_str());
            ImGui::Separator();

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_Button));
            ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));

            ImGui::BeginChild(("child_" + movie.Title).c_str(), ImVec2(poster_width, poster_height / 4), true, ImGuiWindowFlags_NoScrollbar);
            ImGui::TextWrapped("%s", movie.Description.c_str());

            ImGui::EndChild();
            ImGui::PopStyleColor(2);

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
//


