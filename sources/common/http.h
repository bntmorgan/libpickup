#ifndef __HTTP_H__
#define __HTTP_H__

#include <curl/curl.h>

#define HTTP_HEADER_USER_AGENT_MAC "User-Agent: Tinder/3.0.4 "\
  "(iPhone; iOS 7.1; Scale/2.00)"

struct context {
  // Error core
  int error_code;
  // Response buffer
  char *buf;
  // Size
  size_t size;
};

int http_curl_prepare(CURL **curl, struct curl_slist **headers,
    struct context *ctx);
int http_curl_perform(CURL *curl, struct curl_slist *headers);
int http_download_file(const char *url, char **out, size_t *count);

#endif//__HTTP_H__
