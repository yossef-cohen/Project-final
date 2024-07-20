#include "DownloadThread.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include "nlohmann/json.hpp"

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Recipe, name, cuisine, difficulty, cookTimeMinutes ,image)

void DownloadThread::operator()(CommonObjects& common)
{
	httplib::Client cli("https://dummyjson.com");

	auto res = cli.Get("/recipes");
	if (res->status == 200)
	{
		auto json_result = nlohmann::json::parse(res->body);
		common.recipies = json_result["recipes"];
		if (common.recipies.size() > 0)
			common.data_ready = true;
	}
}

void DownloadThread::SetUrl(std::string_view new_url)
{
	_download_url = new_url;
}
