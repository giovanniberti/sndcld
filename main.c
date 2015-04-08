/*
 * Soundcloud song downloader
 *
 * This project is for learning purposes only, and is not intended to be used illegally.
 * The author doesn't hold any responsibility for the usage of this software.
 * For further information, see LICENSE.
 *
 */

#include <stdio.h>      /* printf */
#include <string.h>     /* strstr */
#include <curl/curl.h>  /* curl */
#include "sndcld.h"

int main(int argc, char** argv) {
    int i = 0;
    if (argc < 2) {
        printf("Warning: too few arguments\n"
                "Usage: %s [url] [-n filename]\n", argv[0]);
        return -1;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    if (strstr(argv[1], "/sets/")) {
        printf("Playlist detected: sorry, no playlist support available right now.\n");
        /* TODO: Handle playlist */
    } else {

        if (argc == 4) {
            if (strcmp(argv[2], "-n") == 0) {
                get_song(argv[1], argv[3]);

            } else {
                fprintf(stderr, "Warning: wrong command line argument\n");
                get_song(argv[1], NULL);
            }

        } else {
            get_song(argv[1], NULL);
        }
    }

    curl_global_cleanup();

    return 0;
}
