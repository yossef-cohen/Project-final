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
    try {
        auto common = static_cast<CommonObjects*>(common_ptr);
        if (!common) {
            std::cerr << "CommonObjects pointer is null" << std::endl;
            return;
        }
        std::cout << "DrawFunction started" << std::endl;

        ImVec2 screenSize = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(screenSize);
        ImGui::Begin("Movie Catalog", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);

        static char buff[200] = "";
        static std::vector<Movie> filteredMovies;
        static int sortOption = 0;
        static bool showRadioButtons = false;
        static bool showFilterPopup = false;
        static int currentPage = 0;
        static const int itemsPerPage = 54;
        static const int totalPages = 175; // Fixed number of pages
        static std::unordered_map<std::string, bool> categoryFilters;

        // Search and sort movies
        auto performSearchAndSort = [&]() {
            std::cout << "Performing search and sort" << std::endl;
            std::string searchStr = toLower(std::string(buff));
            std::vector<Movie> tempMovies;
            {
                std::lock_guard<std::mutex> lock(common->movies_mutex);
                tempMovies = common->Movies; // Create a copy to work with
            }

            filteredMovies.clear();
            for (const auto& movie : tempMovies) {
                std::string movieTitleLower = toLower(movie.Title);
                if (movieTitleLower.find(searchStr) != std::string::npos) {
                    if (categoryFilters.empty() || categoryFilters[movie.Genre]) {
                        filteredMovies.push_back(movie);
                    }
                }
            }
            SortMovies(filteredMovies, sortOption);
            currentPage = 0;
            std::cout << "Search and sort completed. Filtered movies: " << filteredMovies.size() << std::endl;
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
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 5.0f);
        }

        // Filter button
        ImGui::SameLine();
        if (ImGui::Button("Filter", ImVec2(button_width, ImGui::GetFrameHeight() + 10.0f))) {
            showFilterPopup = !showFilterPopup; // Toggle the popup visibility
        }

        // Get button position for setting popup position
        ImVec2 filterButtonPos = ImGui::GetItemRectMin();

        // Filter pop-up window
        if (showFilterPopup) {
            ImGui::SetNextWindowPos(ImVec2(filterButtonPos.x, filterButtonPos.y + ImGui::GetFrameHeight() + 10.0f), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Always); // Fixed size
            ImGui::OpenPopup("Filter Categories");
            if (ImGui::BeginPopup("Filter Categories", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) { // Fixed, non-resizable, non-movable
                // Demo categories, these should be dynamic based on your data
                static const std::vector<std::string> categories = { "Action", "Comedy", "Drama" };
                for (const auto& category : categories) {
                    ImGui::Checkbox(category.c_str(), &categoryFilters[category]);
                }
                if (ImGui::Button("Apply Filters")) {
                    performSearchAndSort();
                    showFilterPopup = false; // Close after applying filters
                }
                ImGui::EndPopup();
            }
            else {
                // Close the pop-up if it is not open anymore
                showFilterPopup = false;
            }
        }

        // Check if the user clicked outside the pop-up to close it
        if (showFilterPopup && ImGui::IsMouseClicked(0)) {
            ImVec2 mousePos = ImGui::GetMousePos();
            ImVec2 popupMin = filterButtonPos;
            ImVec2 popupMax = ImVec2(popupMin.x + 300, popupMin.y + 200); // Match the size of the popup
            if (mousePos.x < popupMin.x || mousePos.x > popupMax.x || mousePos.y < popupMin.y || mousePos.y > popupMax.y) {
                showFilterPopup = false;
            }
        }
		float width = ImGui::GetWindowWidth() * 0.46;
        // Pagination UI
        ImGui::Spacing();
        ImGui::SetCursorPosX(width);
        ImGui::Text("Page %d of %d", currentPage + 1, totalPages);
        ImGui::SetCursorPosX(width-5);
        if (ImGui::Button("Previous") && currentPage > 0) currentPage--;
        ImGui::SetCursorPosX(width);
        ImGui::SameLine();
        if (ImGui::Button("Next") && currentPage < totalPages - 1) currentPage++;
        // Prepare page movies
        std::vector<Movie> pageMovies;
        std::vector<Movie> sourceMovies;
        {
            std::lock_guard<std::mutex> lock(common->movies_mutex);
            sourceMovies = filteredMovies.empty() ? common->Movies : filteredMovies;
        }

        size_t startIdx = static_cast<size_t>(currentPage) * itemsPerPage;
        size_t endIdx = std::min<size_t>(startIdx + itemsPerPage, sourceMovies.size());

        if (!sourceMovies.empty() && startIdx < sourceMovies.size()) {
            pageMovies.assign(sourceMovies.begin() + startIdx, sourceMovies.begin() + endIdx);
        }

        std::cout << "Page movies prepared. Size: " << pageMovies.size()
            << " (from " << startIdx << " to " << (startIdx + pageMovies.size())
            << " out of " << sourceMovies.size() << ")" << std::endl;
        //std::cout << "Page movies prepared. Size: " << pageMovies.size() << std::endl;

        //std::cout << "Calling Draw function" << std::endl;
        Draw(common, pageMovies);

        ImGui::End();
        //std::cout << "DrawFunction completed" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Unhandled exception in DrawFunction: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown exception in DrawFunction" << std::endl;
    }
}


void DrawThread::Draw(CommonObjects* common, const std::vector<Movie>& filteredMovies) {
    static ImageCache imageCache;

    ImGui::GetIO().FontGlobalScale = 1.2f;
    ImVec2 screenSize = ImGui::GetIO().DisplaySize;
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, screenSize.y - 100), false, ImGuiWindowFlags_HorizontalScrollbar);

    float window_visible_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
	int movies_per_row = 5;
    //int movies_per_row = (float)(ImGui::GetWindowWidth() * 0.0025f);
    float poster_width = ImGui::GetContentRegionAvail().x / movies_per_row - 10;
    float poster_height = poster_width * 1.5f;

    const auto& movies_to_display = filteredMovies.empty() ? common->Movies : filteredMovies;
    int movies_to_show = std::min<int>(static_cast<int>(movies_to_display.size()), common->loaded_movies_count.load());

    if (movies_to_show == 0) {
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.45f);
        ImGui::Text("No movies to display.");
        ImGui::EndChild();
        return;
    }
    if (!common->loading_complete) {
        float progress = static_cast<float>(common->loaded_movies_count) / common->Movies.size();
        ImGui::ProgressBar(progress, ImVec2(-1, 0));
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.44f);
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
    // Print the original string for debugging
    std::cout << "toLower input string (first 100 characters): " << str.substr(0, 100) << "..." << std::endl;
    std::cout << "toLower input string length: " << str.length() << std::endl;

    // Check if the string is too large to process
    if (str.length() > 1000000) { // You can adjust this limit as needed
        std::cerr << "Warning: String too large in toLower function. Truncating." << std::endl;
        return str.substr(0, 1000000); // Return a truncated version of the string
    }

    std::string lowerStr;
    lowerStr.reserve(str.length()); // Pre-allocate memory to avoid reallocations

    for (char c : str) {
        lowerStr += std::tolower(static_cast<unsigned char>(c));
    }

    // Print the result string for debugging
    std::cout << "toLower output string (first 100 characters): " << lowerStr.substr(0, 100) << "..." << std::endl;
    std::cout << "toLower output string length: " << lowerStr.length() << std::endl;

    return lowerStr;
}

// Callback function to write data into a vector
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::vector<unsigned char>*)userp)->insert(((std::vector<unsigned char>*)userp)->end(), (unsigned char*)contents, (unsigned char*)contents + size * nmemb);
    return size * nmemb;
}




