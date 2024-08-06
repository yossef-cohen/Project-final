#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include "ServerSide/ServerWithInput.h"

class DownloadThread {
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

    bool fetchMovieData(CommonObjects& common) {
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
                    common.Movies.clear();
                    const int batch_size = 100;  // Process 100 movies at a time
                    int count = 0;
                    for (const auto& movieData : jsonResponse) {
                        Movie movie;
                        movie.Title = movieData.value("Title", "");
                        movie.Year = movieData.value("Year", "");
                        movie.Duration = movieData.value("Duration", "");
                        movie.Rating = movieData.value("Rating", "");
                        movie.Genre = movieData.value("Genre", "");
                        movie.Description = movieData.value("Description", "");
                        movie.Poster = movieData.value("Poster", "");

                        common.Movies.push_back(movie);

                        ++count;
                        if (count % batch_size == 0) {
                            // Add a small delay after processing each batch
                            std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        }
                            // Update the UI to show progress
                         //   common.loaded_movies_count = count;
                        
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
        // Create the "posters" directory if it does not exist
        std::filesystem::path dir = "posters";
        if (!std::filesystem::exists(dir)) {
            std::filesystem::create_directory(dir);
        }

        // Define the path where the image would be saved
        std::filesystem::path localImagePath = dir / (movie.Title + ".jpg");

        // Check if the file already exists
        if (std::filesystem::exists(localImagePath)) {
            std::cout << "Image for '" << movie.Title << "' already exists. Skipping download." << std::endl;
            return true;
        }

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

            std::ofstream outfile(localImagePath, std::ios::binary);
            outfile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
            outfile.close();

            std::cout << "Image saved successfully as '" << localImagePath.string() << "'." << std::endl;
            return true;
        }
        return false;
    }

public:
    void operator()(CommonObjects& common) {
        ServerWithInput server;
        std::jthread server_thread(&ServerWithInput::httpServer, &server);

        if (fetchMovieData(common)) {
            const int batch_size = 50;
            for (int i = 0; i < common.Movies.size(); ++i) {
                const auto& movie = common.Movies[i];

                std::filesystem::path localImagePath = std::filesystem::path("posters") / (movie.Title + ".jpg");
                if (!std::filesystem::exists(localImagePath)) {
                    if (downloadAndSaveImage(movie)) {
                        std::cout << "Image downloaded and saved successfully for " << movie.Title << std::endl;
                    }
                    else {
                        std::cerr << "Failed to download and save image for " << movie.Title << std::endl;
                    }
                }

                common.loaded_movies_count++;

                // Add a small delay after each request
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                // After each batch, give the UI thread a chance to update
                if ((i + 1) % batch_size == 0 || i == common.Movies.size() - 1) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            common.loading_complete = true;
        }
        else {
            std::cerr << "Failed to fetch movie data" << std::endl;
        }
    }
};