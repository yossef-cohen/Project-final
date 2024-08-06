#pragma once
#include <atomic>
#include <string>
#include <vector>

struct Movie
{
    std::string Title;
    std::string Year;
    std::string Duration;
    std::string Rating;
    std::string Genre;
    std::string Poster;
    std::string Description;
};

struct CommonObjects
{
    std::atomic<int> loaded_movies_count{ 0 };
    std::atomic<bool> loading_complete{ false };
    std::atomic_bool data_ready = false;  // Use atomic_bool for thread-safe operations
    std::atomic_bool exit_flag = false;
    std::atomic_bool start_download = false;
    std::string url;
    std::vector<Movie> Movies;
};
