cmake_minimum_required(VERSION 3.10)
project(MainProgram)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Specify the paths to CURL and other dependencies
set(CURL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/ServerSide/curl-8.8.0_3-win64-mingw/include")
set(CURL_LIBRARY "${CMAKE_SOURCE_DIR}/ServerSide/curl-8.8.0_3-win64-mingw/lib/libcurl.dll.a")
set(HTTPLIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/ServerSide/httplib")
set(NLOHMANN_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/ServerSide/nlohmann")

# Include directories
include_directories(${CURL_INCLUDE_DIR})
include_directories(${HTTPLIB_INCLUDE_DIR})
include_directories(${NLOHMANN_INCLUDE_DIR})

# Link directories
link_directories(${CMAKE_SOURCE_DIR}/ServerSide/curl-8.8.0_3-win64-mingw/lib)

# Add source files
set(SOURCE_FILES
    ConnectedApp.cpp        # Replace with your actual main program file
    ImageDownloader.cpp    # Include the downloader if needed
    ServerWithInput.cpp    # Include the server if part of the same project
)

# Create executable
add_executable(MainProgram ${SOURCE_FILES})

# Link libraries
target_link_libraries(MainProgram ${CURL_LIBRARY})

# Optionally, you can set up the runtime path for DLLs if needed
# set_target_properties(MainProgram PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
