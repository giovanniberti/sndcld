#define _POSIX_SOURCE

#include "sndcld.h"
#include <stdlib.h>     /* malloc(), free() */
#include <string.h>     /* strncpy() */
#include <stdio.h>      /* printf() */
#include <curl/curl.h>  /* libcurl */

static char* sndcld_getid(char* url);
static size_t header_callback(char* buf, size_t size, size_t nitems, void* userdata);
static size_t curl_null_callback(char* buf, size_t size, size_t nitems, void* userdata);
static size_t write_callback(char* buf, size_t size, size_t nitems, FILE* file);

static size_t header_callback(char* buf, size_t size, size_t nitems, void* userdata) {
    char* location = strstr(buf, "Location: ");
    char* udata = userdata;
    char* p = NULL;

    if(location) {
        location += strlen("Location: ");
        /* Puts a '\0' instead of '\r', thus effectively truncating the string at that point */

        p = strchr(location, '\r');
        *p = '\0';

        strcpy(udata, location);

        printf("Location: %s\n", udata);
    }

    return nitems * size;
}

static size_t curl_null_callback(char* buf, size_t size, size_t nitems, void* userdata) {
    /* To avoid compiler warnings */
    buf = userdata;
    buf++;

    return size * nitems;
}

static size_t write_callback(char* buf, size_t size, size_t nitems, FILE* file) {
    return fwrite(buf, size, nitems, file);
}

static char* sndcld_getid(char* url) {
    CURL* curl = NULL;
    char* resolve_url = NULL;
    char* id = NULL;
    char* location = malloc(sizeof(char) * CURL_MAX_HTTP_HEADER);
    char* tmp = NULL;
    char* dot = NULL;
    size_t len = 0;

    if(!url) return NULL;

    resolve_url = malloc(sizeof(char) * (strlen("http://api.soundcloud.com/resolve.json?url=&client_id=YOUR_CLIENT_ID") + strlen(url) + 1));
    sprintf(resolve_url, "http://api.soundcloud.com/resolve.json?url=%s&client_id=YOUR_CLIENT_ID", url);

    printf("DBG: resolve_url = \"%s\"\n", resolve_url);

    curl = curl_easy_init();
    if(!curl) return NULL;

    curl_easy_setopt(curl, CURLOPT_URL, resolve_url);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, location);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_null_callback);
    curl_easy_perform(curl);

    tmp = location + (sizeof(char) * strlen("https://api.soundcloud.com/tracks/"));

    dot = strchr(tmp, '.');
    len = dot - tmp;

    id = malloc(len + sizeof(char));
    memcpy(id, tmp, len);

    id[len + sizeof(char)] = '\0';

    curl_easy_cleanup(curl);
    free(location);

    return id;
}

void get_song(char* url) {
    FILE* song = NULL;
    CURL* curl = NULL;
    char* id = NULL;
    char* streamurl = NULL;

    printf("Getting song id...\n");

    id = sndcld_getid(url);
    if(!id) goto error;

    curl = curl_easy_init();
    if(!curl) goto error;

    /*
     * Path to get song:
     * api.soundcloud.com/resolve.json?url=[url]&client_id=[id] -> api.soundcloud.com/tracks/[id].json?...
     * api.soundcloud.com/tracks/[id]/stream?client_id=[id]
     */

    printf("DBG: song id = %s\n", id);

    streamurl = malloc((strlen("http://api.soundcloud.com/tracks//stream?client_id=YOUR_CLIENT_ID") + strlen(id) + 1) * sizeof(char));
    if(!streamurl) goto error;

    sprintf(streamurl, "http://api.soundcloud.com/tracks/%s/stream?client_id=YOUR_CLIENT_ID", id);

    song = fopen("song.mp3", "w");
    if(!song) goto error;

    printf("Downloading song from %s\n", streamurl);

    curl_easy_setopt(curl, CURLOPT_URL, streamurl);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, song);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_perform(curl);

    fflush(song);
    free(id);
    curl_easy_cleanup(curl);
    free(streamurl);
    fclose(song);

    return;

error:
    free(id);
    curl_easy_cleanup(curl);
    free(streamurl);
}
