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
	std::atomic_bool exit_flag = false;
	std::atomic_bool start_download = false;
	std::atomic_bool data_ready = false;
	std::string url;
	std::vector<Movie> Movies;
};