#ifndef ARB_UTIL_H
#define ARB_UTIL_H

#include <iostream>
#include <string>
#include <curl/curl.h>

#include "cryptopp/cryptlib.h"
#include "cryptopp/hmac.h"
#include "cryptopp/modes.h"
#include "cryptopp/sha.h"
#include "cryptopp/hex.h"
#include "cryptopp/filters.h"
#include "cryptopp/base64.h"

#define ARB_ERR_BAD_OPTION 20
#define ARB_ERR_INSUFFICIENT_FUNDS 30
#define ARB_ERR_TRADE_NOT_EX 31
#define ARB_ERR_UNEXPECTED_STR 41
#define ARB_ERR_UNKNOWN_PAIR 51
#define ARB_TRADE_EXECUTED 100

constexpr bool ARB_DEBUG = false;

using namespace std;

namespace
{
    std::size_t callback(
            const char* in,
            std::size_t size,
            std::size_t num,
            std::string* out)
    {
        const std::size_t totalBytes(size * num);
        out->append(in, totalBytes);
        return totalBytes;
    }
}

inline void print_ts(){
    cout << chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() << " ";
}

void set_curl_get_options(CURL* curl, const std::string url){

    // Set remote URL.
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    if(ARB_DEBUG){
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    }

    // Don't bother trying IPv6, which would increase DNS resolution time.
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

    //disable Nagle algorithm
    curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 0);

    // Don't wait forever, time out after 10 seconds.
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    // Follow HTTP redirects if necessary.
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, "redshift crypto trading automated trader");

    // Hook up data handling function.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
}

std::string curl_get(CURL* curl) {
    // Response information.
    long http_code;
    CURLcode result;
    const std::unique_ptr<std::string> http_data(new std::string());

    // Hook up data container (will be passed as the last parameter to the
    // callback handling function).  Can be any pointer type, since it will
    // internally be passed as a void pointer.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, http_data.get());

    // Run our HTTP GET command, capture the HTTP response code, and clean up.
    result = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if(result != CURLE_OK) {
          fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
          throw 20;
    }
    if (http_code == 200) {
        if(ARB_DEBUG){
            std::cout << "HTTP Response: " << *http_data.get() << std::endl;
        }
    } else {
        const std::unique_ptr<std::string> last_url;
        curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &last_url);
        std::cout << "Received " << http_code << " response code from " << *last_url.get() << endl;
        throw 30;
    }

    return *http_data;
}

//https://stackoverflow.com/questions/236129/the-most-elegant-way-to-iterate-the-words-of-a-string
std::vector<std::string> split(const std::string &text, char sep) {
  std::vector<std::string> tokens;
  std::size_t start = 0, end = 0;
  while ((end = text.find(sep, start)) != std::string::npos) {
    tokens.push_back(text.substr(start, end - start));
    start = end + 1;
  }
  tokens.push_back(text.substr(start));
  return tokens;
}

//https://stackoverflow.com/questions/39721005/how-to-create-a-hmac-256-using-the-crypto-library#answer-39742465
string hmac_512_sign(const char* key, string plain) {

    string sig;
    try {
        CryptoPP::HMAC<CryptoPP::SHA512 > hmac((byte*)key, strlen(key));

		CryptoPP::StringSource(plain, true,
			new CryptoPP::HashFilter(hmac,
                new CryptoPP::HexEncoder(
                    new CryptoPP::StringSink(sig)
                )
			) // HashFilter
		); // StringSource

	} catch(const CryptoPP::Exception& e) {

		cerr << e.what() << endl;
    }

    return sig;
};

#endif
