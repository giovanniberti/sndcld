/*
 * Soundcloud song downloader
 *
 * This project is for learning purposes only, and is not intended to be used illegally.
 * The author doesn't hold any responsibility for the usage of this software.
 * For further information, see LICENSE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <curl/curl.h>
#include "sndcld.h"

int main(int argc, char** argv) {
    if(argc < 2) {
        printf("Warning: too few arguments\n"
                "Usage: %s [url]\n", argv[0]);
        return -1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    get_song(argv[1]);

    curl_global_cleanup();

    return 0;
}
