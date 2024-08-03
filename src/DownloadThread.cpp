#include "DownloadThread.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"
#include "nlohmann/json.hpp"
#include "input_with_server.h"

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Movie, Title, Year, Duration, Rating, Genre, Poster, Description);

void DownloadThread::operator()(CommonObjects& common)
{
    ServerWithInput server;
    std::jthread server_thread(&ServerWithInput::httpServer, &server);
    std::jthread http(&ServerWithInput::getLinesFromJSON, &server);
    std::this_thread::sleep_for(std::chrono::seconds(10));
    httplib::Client cli("http://127.0.0.1:8080");
    auto res = cli.Get("/");

    if (res && res->status == 200)
    {
        try {
            //std::cout << "Received JSON: " << res->body << std::endl; // Debug output
            auto json_result = nlohmann::json::parse(res->body);

            // Ensure json_result is an array
            if (json_result.is_array())
            {
                std::vector<Movie> movies;
                for (const auto& item : json_result)
                {
                    Movie movie;

                    // Safely fetch and assign fields with type checks

                    movie.Title = item.value("Title", "");

                    if (item.contains("Year") && item["Year"].is_string()) {
         
                        movie.Year = item["Year"].get<std::string>();
                    }
                    else
                        movie.Year = "";

                    if (item.contains("Duration") && item["Duration"].is_string())
                        movie.Duration = item["Duration"].get<std::string>();
                    else
                        movie.Duration = "";

                    if (item.contains("Rating") && item["Rating"].is_string())
                        movie.Rating = item["Rating"].get<std::string>();
                    else
                        movie.Rating = "";

                    if (item.contains("Genre") && item["Genre"].is_string())
                        movie.Genre = item["Genre"].get<std::string>();
                    else
                        movie.Genre = "";

                    if (item.contains("Poster") && item["Poster"].is_string())
                        movie.Poster = item["Poster"].get<std::string>();
                    else
                        movie.Poster = "";

                    if (item.contains("Description") && item["Description"].is_string())
                        movie.Description = item["Description"].get<std::string>();
                    else
                        movie.Description = "";

                    movies.push_back(movie);
                }

                common.Movies = std::move(movies);
                if (!common.Movies.empty())
                    common.data_ready = true;
            }
            else
            {
                std::cerr << "Expected an array of movies, but got: " << json_result.dump() << std::endl;
            }
        }
        catch (const nlohmann::json::type_error& e) {
            std::cerr << "JSON type error: " << e.what() << std::endl;
        }
        catch (const nlohmann::json::parse_error& e) {
            std::cerr << "JSON parsing error: " << e.what() << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
        }
    }
    else
    {
        std::cerr << "HTTP request failed or returned non-200 status" << std::endl;
    }
    //common.data_ready = true;
}

void DownloadThread::SetUrl(std::string_view new_url)
{
    _download_url = new_url;
}
