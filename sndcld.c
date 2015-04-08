#define _POSIX_SOURCE

#include "sndcld.h"
#include <stdlib.h>     /* malloc(), free() */
#include <string.h>     /* strncpy() */
#include <stdio.h>      /* printf() */
#include <curl/curl.h>  /* libcurl */

static char* sndcld_getid(char* url);
static char* sndcld_getname(char* url);
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
    }

    return nitems * size;
}

static size_t curl_null_callback(char* buf, size_t size, size_t nitems, void* userdata) {
    /* Avoid compiler warnings */
    buf = userdata;
    buf++;
    buf--;

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

    if(!location) return NULL;

    if(!url) goto error;

    resolve_url = malloc(sizeof(char) * (strlen("http://api.soundcloud.com/resolve.json?url=&client_id=YOUR_CLIENT_ID") + strlen(url) + 1));
    if(!resolve_url) goto error;

    sprintf(resolve_url, "http://api.soundcloud.com/resolve.json?url=%s&client_id=YOUR_CLIENT_ID", url);

    curl = curl_easy_init();
    if(!curl) goto error;

    curl_easy_setopt(curl, CURLOPT_URL, resolve_url);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, location);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_null_callback);
    curl_easy_perform(curl);

    /*
     * 'tmp' is a temporary value for the id.
     * The id is extrapolated as such:
     *
     * 'location' is in the format 'https://api.soundcloud.com/tracks/[id].json?[...]'
     * So we move the pointer after the third slash (right after "tracks/"),
     * then find the first dot (the one of the json extension)
     * and copy byte by byte from the new pointer until the dot.
     * Then we add the NUL byte.
     *
     * Another approach would be truncating the string at the dot position
     * replacing it with a NUL byte and then using strcpy()
     * This would be safer and feasible as long as we don't use the 'location' string after.
     */

    tmp = location + (sizeof(char) * strlen("https://api.soundcloud.com/tracks/"));

    dot = strchr(tmp, '.');
    len = dot - tmp;

    id = malloc(len + sizeof(char));
    memcpy(id, tmp, len);

    id[len + sizeof(char)] = '\0';

    curl_easy_cleanup(curl);
    free(location);
    free(resolve_url);

    return id;

error:
    free(location);
    free(resolve_url);
    return NULL;
}

char* sndcld_getname(char* url) {
    size_t len = 0;
    char* name = NULL;
    char* grbg = NULL;
    size_t offset = 0;
    int second = 0;
    if(!url) return NULL;

    len = strlen(url);

    /*
     * Assign to 'name' the part "user/track" of the soundcloud URL and prettify it
     * Example https://soundcloud.com/tinush/tinush-journey-original => tinush/tinush-journey-original
     */

    while(url[--len] != '/' || !second) {
        ++offset;
        if(url[len] == '/') second = 1;
    }

    name = malloc((offset + 1) * sizeof(char));

    memcpy(name, url + len + 1, offset + 1); /* include NUL byte */

    while((grbg = strchr(name, '/'))) { /* remove slashes */
        *grbg = ' ';
    }

    while((grbg = strchr(name, '-'))) { /* remove hyphens */
        *grbg = ' ';
    }

    return name;
}


void get_song(char* url, char* filename) {
    FILE* song = NULL;
    CURL* curl = NULL;
    char* id = NULL;
    char* name = NULL;
    char* streamurl = NULL;

    if(!url) {
        fprintf(stderr, "Error: no url supplied!\n");
        return;
    }

    id = sndcld_getid(url);
    if(!id) goto error;

    /*
     * Add space for ".mp3" extension
     * (4 characters plus NUL byte)
     */

    if(filename) {
        name = malloc((strlen(name) + 1) * sizeof(char) + 4 * sizeof(char));
        if(!name) goto error;

        strcpy(name, filename);
    } else {
        name = sndcld_getname(url);
        if(!name) goto error;

        name = realloc(name, (strlen(name) + 1) * sizeof(char) + 4 * sizeof(char));
    }

    strcat(name, ".mp3");

    curl = curl_easy_init();
    if(!curl) goto error;

    /*
     * Path to get song:
     * api.soundcloud.com/resolve.json?url=[url]&client_id=[client_id] -> api.soundcloud.com/tracks/[id].json?...
     * api.soundcloud.com/tracks/[id]/stream?client_id=[client_id] (raw file)
     */

    streamurl = malloc((strlen("http://api.soundcloud.com/tracks//stream?client_id=YOUR_CLIENT_ID") + strlen(id) + 1) * sizeof(char));
    if(!streamurl) goto error;

    sprintf(streamurl, "http://api.soundcloud.com/tracks/%s/stream?client_id=YOUR_CLIENT_ID", id);

    song = fopen(name, "w");
    if(!song) goto error;

    printf("Downloading song into \"%s\"\n", name);

    curl_easy_setopt(curl, CURLOPT_URL, streamurl);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, song);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_perform(curl);

    free(id);
    free(name);
    free(streamurl);
    curl_easy_cleanup(curl);
    fclose(song);

    return;

error:
    free(id);
    free(name);
    free(streamurl);
    curl_easy_cleanup(curl);
}
