#include "ImageDownloader.h"
#include <curl/curl.h>
#include <iostream>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::vector<unsigned char>*)userp)->insert(
        ((std::vector<unsigned char>*)userp)->end(),
        (unsigned char*)contents,
        (unsigned char*)contents + size * nmemb
    );
    return size * nmemb;
}

std::vector<unsigned char> DownloadImage(const char* url) {
    CURL* curl;
    CURLcode res;
    std::vector<unsigned char> buffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        curl_easy_setopt(curl, CURLOPT_CAINFO, "HttpSrc/cacert.pem");

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        else {
            std::cout << "Image downloaded successfully. Size: " << buffer.size() << " bytes." << std::endl;
        }

        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    return buffer;
}
