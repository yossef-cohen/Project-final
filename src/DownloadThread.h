#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <vector>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "ServerSide/ServerWithInput.h"
#include <random>


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
        std::filesystem::path dir = "posters";
        if (!std::filesystem::exists(dir)) {
            std::filesystem::create_directory(dir);
        }

        std::filesystem::path localImagePath = dir / (movie.Title + ".jpg");

        if (std::filesystem::exists(localImagePath)) {
            std::cout << "Image for '" << movie.Title << "' already exists. Skipping download." << std::endl;
            return true;
        }

        CURL* curl = curl_easy_init();
        if (curl) {
            std::vector<unsigned char> buffer;
            curl_easy_setopt(curl, CURLOPT_URL, movie.Poster.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteImageCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
            curl_easy_setopt(curl, CURLOPT_CAINFO, "HttpSrc/cacert.pem");

            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            if (res == CURLE_OK) {
                std::ofstream outfile(localImagePath, std::ios::binary);
                outfile.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
                outfile.close();
                std::cout << "Image saved successfully as '" << localImagePath.string() << "'." << std::endl;
                return true;
            }

            std::cerr << "Failed to download image for " << movie.Title << ": " << curl_easy_strerror(res) << std::endl;
        }
        return false;
    }

    bool downloadBatch(const std::vector<Movie>& batch) {
        CURLM* multi_handle = curl_multi_init();
        if (!multi_handle) {
            std::cerr << "Failed to initialize CURL multi handle" << std::endl;
            return false;
        }

        std::vector<CURL*> easy_handles(batch.size());
        std::vector<std::vector<unsigned char>> buffers(batch.size());

        for (size_t i = 0; i < batch.size(); ++i) {
            const auto& movie = batch[i];
            CURL* easy_handle = curl_easy_init();
            if (easy_handle) {
                easy_handles[i] = easy_handle;

                curl_easy_setopt(easy_handle, CURLOPT_URL, movie.Poster.c_str());
                curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, WriteImageCallback);
                curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, &buffers[i]);
                curl_easy_setopt(easy_handle, CURLOPT_CAINFO, "HttpSrc/cacert.pem");

                CURLMcode mc = curl_multi_add_handle(multi_handle, easy_handle);
                if (mc != CURLM_OK) {
                    std::cerr << "Failed to add handle: " << curl_multi_strerror(mc) << std::endl;
                }
            }
        }

        int still_running;
        CURLMcode mc;
        do {
            mc = curl_multi_perform(multi_handle, &still_running);
            if (mc != CURLM_OK) {
                std::cerr << "curl_multi_perform() failed: " << curl_multi_strerror(mc) << std::endl;
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Yield CPU time
        } while (still_running);

        // Process completed downloads
        for (size_t i = 0; i < batch.size(); ++i) {
            long response_code;
            curl_easy_getinfo(easy_handles[i], CURLINFO_RESPONSE_CODE, &response_code);

            std::filesystem::path localImagePath = std::filesystem::path("posters") / (batch[i].Title + ".jpg");
            if (response_code == 200 && !buffers[i].empty()) {
                std::ofstream outfile(localImagePath, std::ios::binary);
                outfile.write(reinterpret_cast<const char*>(buffers[i].data()), buffers[i].size());
                outfile.close();
                std::cout << "Image saved successfully as '" << localImagePath.string() << "'." << std::endl;
            }
            else {
                std::cerr << "Failed to download or empty buffer for: " << batch[i].Title << std::endl;
            }

            curl_multi_remove_handle(multi_handle, easy_handles[i]);
            curl_easy_cleanup(easy_handles[i]);
        }

        curl_multi_cleanup(multi_handle);
        return true;
    }

public:
    void operator()(CommonObjects& common) {
        ServerWithInput server;
        std::jthread server_thread(&ServerWithInput::httpServer, &server);

        if (fetchMovieData(common)) {
            const size_t min_batch_size = 100;
            const size_t max_batch_size = 200;
            int successful_downloads = 0;
            int failed_downloads = 0;

            // Random number generator setup
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(min_batch_size, max_batch_size);

            for (size_t i = 0; i < common.Movies.size(); /* increment inside loop */) {
                size_t batch_size = dis(gen);  // Random batch size between min_batch_size and max_batch_size
                size_t batch_end = i + batch_size;
                if (batch_end > common.Movies.size()) {
                    batch_end = common.Movies.size();
                }

                std::vector<Movie> batch(common.Movies.begin() + i, common.Movies.begin() + batch_end);

                if (downloadBatch(batch)) {
                    successful_downloads += batch.size();
                }
                else {
                    failed_downloads += batch.size();
                }

                common.loaded_movies_count += batch.size();

                float progress = static_cast<float>(common.loaded_movies_count) / common.Movies.size();
                std::cout << "\rProgress: " << std::fixed << std::setprecision(2) << (progress * 100) << "% "
                    << "(" << common.loaded_movies_count << "/" << common.Movies.size() << ") "
                    << "Downloaded: " << successful_downloads << " Failed: " << failed_downloads << std::flush;

                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                i = batch_end;  // Move to the next batch
            }

            std::cout << "\nDownload complete. " << successful_downloads << " images downloaded, "
                << failed_downloads << " failed." << std::endl;

            common.loading_complete = true;
        }
        else {
            std::cerr << "Failed to fetch movie data" << std::endl;
        }
    }};
