#pragma once
#include "CommonObject.h"
#include <vector>

class DrawThread
{
public:
    void operator()(CommonObjects& common);
    static void DrawFunction(void* common_ptr);

private:
    static void Draw(CommonObjects* common, const std::vector<Movie>& filteredMovies);
    static std::string toLower(const std::string& str);
};