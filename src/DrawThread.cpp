#include "DrawThread.h"
#include "../vendor/ImGui/imgui.h"
#include "GuiMain.h"

void DrawAppWindow(void* common_ptr)
{
	auto common = (CommonObjects*)common_ptr;
	ImGui::Begin("Connected!");
	ImGui::Text("this is our draw callback");
	static char buff[200];
	ImGui::InputText("URL", buff, sizeof(buff));
	ImGui::SameLine();
	if(ImGui::Button("set"))
		common->url = buff;
	if (common->data_ready)
	{
		if (ImGui::BeginTable("Recipes", 5))  //, flags))
		{
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("Cuisine");
			ImGui::TableSetupColumn("Difficulty");
			ImGui::TableSetupColumn("Cook Time (Minutes)");
			ImGui::TableSetupColumn("");
			ImGui::TableHeadersRow();

			for (auto& rec : common->recipies)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text(rec.name.c_str());
				ImGui::TableSetColumnIndex(1);
				ImGui::Text(rec.cuisine.c_str());
				ImGui::TableSetColumnIndex(2);
				ImGui::Text(rec.difficulty.c_str());
				ImGui::TableSetColumnIndex(3);
				ImGui::Text("%d", rec.cookTimeMinutes);
				ImGui::TableSetColumnIndex(4);
				if (ImGui::Button("Show"))
				{
					//show details
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

