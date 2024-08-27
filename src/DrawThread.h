#pragma once
#include "CommonObject.h"
#include <vector>
#include <string>
#include <mutex>
#include <../GL/glew.h>
#include <GL.h>
#include <stb_image.h>

class DrawThread
{
public:
    void operator()(CommonObjects& common);
    static void DrawFunction(void* common_ptr);
    static void SortMovies(std::vector<Movie>& movies, int sortOption);
    static void Draw(CommonObjects* common, const std::vector<Movie>& filteredMovies);

private:
    static std::string toLower(const std::string& str);

};