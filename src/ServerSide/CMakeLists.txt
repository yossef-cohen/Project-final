cmake_minimum_required(VERSION 3.10)
project(ServerSide)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify the paths to CURL
set(CURL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/curl-8.8.0_3-win64-mingw/include")
set(CURL_LIBRARY "${CMAKE_SOURCE_DIR}/curl-8.8.0_3-win64-mingw/lib/libcurl.dll.a")  # Ensure this is the correct file for your build type

# Include directories
include_directories(${CURL_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/httplib)
include_directories(${CMAKE_SOURCE_DIR}/nlohmann)

# Link directories
link_directories(${CMAKE_SOURCE_DIR}/curl-8.8.0_3-win64-mingw/lib)

# Add source files
set(SOURCE_FILES
    ServerWithInput.cpp
    ImageDownloader.cpp
    main.cpp
)

# Create executable
add_executable(server ${SOURCE_FILES})

# Link libraries
target_link_libraries(server ${CURL_LIBRARY})

# Optional: Print the build configuration
message(STATUS "CURL Include Directory: ${CURL_INCLUDE_DIR}")
message(STATUS "CURL Library: ${CURL_LIBRARY}")
