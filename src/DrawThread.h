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
    static int ProgressCallback(void* clientp, double t, double d, double u, double f);

    void operator()(CommonObjects& common);
    static void DrawFunction(void* common_ptr);
    static std::vector<unsigned char> DownloadImage(const char* url);
    static GLuint LoadTextureFromMemory(const unsigned char* data, int dataSize, int* width, int* height);
    static void SortMovies(std::vector<Movie>& movies, int sortOption);


private:

    static void Draw(CommonObjects* common, const std::vector<Movie>& filteredMovies);
    static std::string toLower(const std::string& str);

};