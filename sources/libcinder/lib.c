/*
Copyright (C) 2016  Benoît Morgan

This file is part of libcinder.

libcinder is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

libcinder is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with libcinder.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "api.h"

#define TMP_TEMPLATE "/tmp/cinder_XXXXXX"

static const char *at;

void cinder_set_credentials(const char *access_token) {
  at = access_token;
}

static size_t __attribute__ ((unused)) cinder_updates_write(void *ptr, size_t
    size, size_t nmemb, void *stream) {
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

void test(void) {
  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_ALL);

  /* get a curl handle */
  curl = curl_easy_init();
  if(curl) {
    struct curl_slist *headers=NULL;

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers,
        "User-Agent: Tinder/3.0.4 (iPhone; iOS 7.1; Scale/2.00)");

    char b[256];
    sprintf(b, "X-Auth-Token: %s", at);
    headers = curl_slist_append(headers, b);

    /* the DEBUGFUNCTION has no effect until we enable VERBOSE */ 
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);

    /* First set the URL that is about to receive our POST. This URL can
       just as well be a https:// URL if that is what should receive the
       data. */
    curl_easy_setopt(curl, CURLOPT_URL, API_HOST API_UPDATES);
    /* Now specify the POST data */
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,
        "{\"last_activity_date\": \"\"}");

    /* pass our list of custom made headers */
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    /* Perform the request, res will get the return code */
    res = curl_easy_perform(curl);

    /* open the file */
    char tmpname[] = TMP_TEMPLATE;
    FILE *tmpfile = NULL;
    int tmpfilefd = mkstemp(tmpname);
    if (tmpfilefd == -1) {
      perror("open tmp file");
    } else {
      tmpfile = fdopen(tmpfilefd, "a");
      if (tmpfile == NULL) {
        perror("fdopen tmp file");
      } else {
        /* write the page body to this file handle */
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, tmpfile);
        /* send all data to this function  */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cinder_set_credentials);
        /* get it! */
        curl_easy_perform(curl);
        /* close the header file */
        fclose(tmpfile);
      }
    }

    /* Check for errors */
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
          curl_easy_strerror(res));

    curl_slist_free_all(headers); /* free the header list */

    /* always cleanup */
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();
}
