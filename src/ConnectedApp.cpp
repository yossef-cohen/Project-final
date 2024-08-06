#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <vector>
#include <thread>
#include "ServerSide/ServerWithInput.h"
#include <CommonObject.h>
#include <CommonObject.h>
class ConnectedApp {
private:
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    }

    static size_t WriteImageCallback(void* contents, size_t size, size_t nmemb, std::vector<unsigned char>* buffer) {
        size_t totalSize = size * nmemb;
        buffer->insert(buffer->end(), (unsigned char*)contents, (unsigned char*)contents + totalSize);
        return totalSize;
    }

    bool fetchMovieData() {
        CURL* curl;
        CURLcode res;
        std::string readBuffer;

        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8080/");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

            res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
                curl_easy_cleanup(curl);
                return false;
            }

            curl_easy_cleanup(curl);

            try {
                nlohmann::json jsonResponse = nlohmann::json::parse(readBuffer);

                if (jsonResponse.is_array()) {
                    m_movies.clear();
                    for (const auto& movieData : jsonResponse) {
                        Movie movie;
                        movie.Title = movieData.value("Title", "");
                        movie.Year = movieData.value("Year", "");
                        movie.Duration = movieData.value("Duration", "");
                        movie.Rating = movieData.value("Rating", "");
                        movie.Genre = movieData.value("Genre", "");
                        movie.Description = movieData.value("Description", "");
                        movie.Poster = movieData.value("Poster", "");

                        m_movies.push_back(movie);
                    }
                    return true;
                }
                else {
                    std::cerr << "Expected JSON array" << std::endl;
                    return false;
                }
            }
            catch (const nlohmann::json::exception& e) {
                std::cerr << "JSON error: " << e.what() << std::endl;
                return false;
            }
        }
        return false;
    }

    bool downloadAndSaveImage(const Movie& movie) {
        CURL* curl;
        CURLcode res;
        std::vector<unsigned char> buffer;

        curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, movie.Poster.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteImageCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
            curl_easy_setopt(curl, CURLOPT_CAINFO, "HttpSrc/cacert.pem");

            res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
                curl_easy_cleanup(curl);
                return false;
            }

            curl_easy_cleanup(curl);

            // Create the "posters" directory if it does not exist
            std::filesystem::path dir = "posters";
            if (!std::filesystem::exists(dir)) {
                std::filesystem::create_directory(dir);
            }

            // Define the path to save the image
            std::filesystem::path localImagePath = dir / (movie.Title + ".jpg");
            std::ofstream outfile(localImagePath, std::ios::binary);
            outfile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
            outfile.close();

            std::cout << "Image saved successfully as '" << localImagePath.string() << "'." << std::endl;
            return true;
        }
        return false;
    }

    std::vector<Movie> m_movies;

public:
    void Run() {
        if (fetchMovieData()) {
            for (const auto& movie : m_movies) {
                // Display movie information
                std::cout << "Title: " << movie.Title << std::endl;
                std::cout << "Year: " << movie.Year << std::endl;
                std::cout << "Duration: " << movie.Duration << std::endl;
                std::cout << "Rating: " << movie.Rating << std::endl;
                std::cout << "Genre: " << movie.Genre << std::endl;
                std::cout << "Description: " << movie.Description << std::endl;
                std::cout << "Poster URL: " << movie.Poster << std::endl;

                if (downloadAndSaveImage(movie)) {
                    std::cout << "Image downloaded and saved successfully." << std::endl;
                }
                else {
                    std::cerr << "Failed to download and save image for " << movie.Title << std::endl;
                }

                std::cout << "------------------------" << std::endl;
            }
        }
        else {
            std::cerr << "Failed to fetch movie data" << std::endl;
        }
    }
};


int main() {
    curl_global_init(CURL_GLOBAL_ALL);

    try {
        ServerWithInput server;
        std::thread server_thread(&ServerWithInput::httpServer, &server);

        // Check if the thread is joinable
        if (!server_thread.joinable()) {
            throw std::runtime_error("Failed to start server thread.");
        }

        ConnectedApp app;
        app.Run();

        if (server_thread.joinable()) {
            server_thread.join();
        }
        else {
            std::cerr << "Server thread was not joinable." << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        return 1;
    }
    curl_global_cleanup();
    return 0;
}
