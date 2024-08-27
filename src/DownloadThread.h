#include <curl/curl.h>
#include <../src/ServerSide/nlohmann/json.hpp>
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
        size_t total_size = size * nmemb;
        if (userp->size() + total_size > userp->max_size() - 1) {
            std::cerr << "WriteCallback: Potential buffer overflow detected" << std::endl;
            return 0; // Signal error to libcurl
        }
        userp->append((char*)contents, total_size);
        return total_size;
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
                    std::vector<Movie> tempMovies;
                    for (const auto& movieData : jsonResponse) {
                        Movie movie;
                        movie.Title = movieData.value("Title", "");
                        movie.Year = movieData.value("Year", "");
                        movie.Duration = movieData.value("Duration", "");
                        movie.Rating = movieData.value("Rating", "");
                        movie.Genre = movieData.value("Genre", "");
                        movie.Description = movieData.value("Description", "");
                        movie.Poster = movieData.value("Poster", "");

                        tempMovies.push_back(movie);
                    }

                    {
                        std::lock_guard<std::mutex> lock(common.movies_mutex);
                        common.Movies = std::move(tempMovies);
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
    }

    bool downloadAndSaveImage(const Movie& movie) {
        std::filesystem::path dir = "posters";
        if (!std::filesystem::exists(dir)) {
            std::filesystem::create_directory(dir);
        }

        std::filesystem::path localImagePath = dir / (movie.Title + ".jpg");

        if (std::filesystem::exists(localImagePath)) {
           // std::cout << "Image for '" << movie.Title << "' already exists. Skipping download." << std::endl;
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
                //std::cout << "Image saved successfully as '" << localImagePath.string() << "'." << std::endl;
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
                //std::cout << "Image saved successfully as '" << localImagePath.string() << "'." << std::endl;
            }
            else {
               // std::cerr << "Failed to download or empty buffer for: " << batch[i].Title << std::endl;
            }

            curl_multi_remove_handle(multi_handle, easy_handles[i]);
            curl_easy_cleanup(easy_handles[i]);
        }

        curl_multi_cleanup(multi_handle);
        return true;
    }

public:
    void operator()(CommonObjects& common) {
        if (fetchMovieData(common)) {
            const size_t min_batch_size = 100;
            const size_t max_batch_size = 200;
            int successful_downloads = 0;
            int failed_downloads = 0;

            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(min_batch_size, max_batch_size);

            size_t total_movies;
            std::vector<Movie> all_movies;
            {
                std::lock_guard<std::mutex> lock(common.movies_mutex);
                total_movies = common.Movies.size();
                all_movies = common.Movies; // Create a copy to work with
            }

            for (size_t i = 0; i < total_movies; /* increment inside loop */) {
                size_t batch_size = dis(gen);
                size_t batch_end = std::min<size_t>(i + batch_size, total_movies);

                std::vector<Movie> batch(all_movies.begin() + i, all_movies.begin() + batch_end);

                if (downloadBatch(batch)) {
                    successful_downloads += batch.size();
                }
                else {
                    failed_downloads += batch.size();
                }

                {
                    std::lock_guard<std::mutex> lock(common.movies_mutex);
                    common.loaded_movies_count += batch.size();
                }
                i = batch_end;

                // Add a small delay to prevent CPU hogging
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            common.loading_complete = true;
        }
        else {
            std::cerr << "Failed to fetch movie data" << std::endl;
        }
    }
};
