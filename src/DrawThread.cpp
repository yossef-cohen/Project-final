#include "DrawThread.h"
#include "../vendor/ImGui/imgui.h"
#include "GuiMain.h"
#include <iostream>

void DrawAppWindow(void* common_ptr)
{
    // Cast the common pointer and check for null
    auto common = reinterpret_cast<CommonObjects*>(common_ptr);
    if (!common) {
        std::cerr << "CommonObjects pointer is null" << std::endl;
        return;
    }

    ImGui::Begin("Connected!");

    static char buff[200];
    // Uncomment to add URL input functionality
    // ImGui::InputText("URL", buff, sizeof(buff));
    // ImGui::SameLine();
    // if (ImGui::Button("Set"))
    //     common->url = buff;

    if (common->data_ready)
    {
        if (ImGui::BeginTable("Movies", 8))
        {
            // Setup columns
            ImGui::TableSetupColumn("Title");
            ImGui::TableSetupColumn("Year");
            ImGui::TableSetupColumn("Duration");
            ImGui::TableSetupColumn("Rating");
            ImGui::TableSetupColumn("Genre");
            ImGui::TableSetupColumn("Poster");
            ImGui::TableSetupColumn("Description");
            ImGui::TableSetupColumn("");  // Empty column for buttons
            ImGui::TableHeadersRow();

            // Iterate through movies
            for (const auto& movie : common->Movies)
            {
                ImGui::TableNextRow();

                // Set column indices and display movie data
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%s", movie.Title.c_str());

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", movie.Year.c_str());

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", movie.Duration.c_str());

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%s", movie.Rating.c_str());

                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%s", movie.Genre.c_str());

                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%s", movie.Poster.c_str());

                ImGui::TableSetColumnIndex(6);
                ImGui::Text("%s", movie.Description.c_str());

                ImGui::TableSetColumnIndex(7);
                if (ImGui::Button("Show"))
                {
                    // Implement show details functionality here
                }
            }

            ImGui::EndTable();
        }
    }

    ImGui::End();
}


void DrawThread::operator()(CommonObjects& common)
{
	GuiMain(DrawAppWindow, &common);
	common.exit_flag = true;
}

