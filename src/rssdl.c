/*
 * RSSDL - Download RSS to folder.
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <sys/types.h>
#include <string.h>
#include <asm/errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "rssfs.h"
#include "http_fetcher.h"
#include "rss_parser.h"

// The linked list struct for our data
RssData * datalist;

int download_file(char * file, char * fileUrl){
    int res;
    long int file_size;
    char *file_content;
    FILE *ptr;

    printf("%s ", file);

    #ifdef PREFETCH
        RssData * current = getRecordByTitle(datalist, file);
        if (!current) {
            printf(" FAILED\n");
            fprintf(stderr, "Could fetch file from cache '%s'.\n", file);
            return EXIT_FAILURE;
        }
        file_size = current->size;
        file_content = current->file_content;
    #else
        file_size = fetch_url(fileUrl, &file_content);
    #endif
    if (file_size <= 0) {
        printf(" FAILED\n");
        fprintf(stderr, "Could not fetch file from '%s'.\n", fileUrl);
        return EXIT_FAILURE;
    }

    ptr = fopen(file, "wb");
    if(!ptr) {
        printf(" FAILED\n");
        fprintf(stderr, "Could not write file '%s', check path permissions and free space.\n", file);
        return EXIT_FAILURE;
    }

    res = fwrite(file_content, file_size, 1, ptr);

    if (!res) {
        printf(" FAILED\n");
        fprintf(stderr, "Writing to file '%s' failed, check free space.\n", file);
        return EXIT_FAILURE;
    }

    fclose(ptr);

    printf(" OK\n");

}

int download_main(char * rssUrl, char * downloadPath, int dryRun) {
    int res;

    printf("Loading...\n");

    datalist = loadRSS(rssUrl);

    if (datalist == NULL) {
        fprintf(stderr, "Could not open or parse: '%s'\n", rssUrl);
        return EXIT_FAILURE;
    } else {
        printf("RSS feed '%s' loaded.\n", rssUrl);
    }

    res = chdir(downloadPath);
    if (res) {
        fprintf(stderr, "Could not open path: '%s'\n", downloadPath);
        return EXIT_FAILURE;
    }

    printf("Downloading files...\n");

    RssData *current = datalist;
    if (current != NULL) {
        while (current != NULL) {
            if (dryRun) {
                printf("%s OK\n", current->title);
            } else {
                res = download_file(current->title, current->link);
            }
            current = current->next;
        }
    }

    printf("Download finished.\n");

    return EXIT_SUCCESS;
}

/* Print usage information on stderr */
void usage(char * argve) {
    fprintf(stderr, "Usage: %s [--dry-run] url download-dir\n", argve);
    fprintf(stderr, "Version: %s\n", VERSION);
}

/* Main function */
int main(int argc, char *argv[]) {
    char *url;
    char *path;
    int dryRun=0;

    if (argc < 3 | argc > 4) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    
    dryRun=(argc == 4 && strncmp(argv[1],"--dry-run", 9) == 0);
    url = argv[dryRun + 1];
    path = argv[dryRun + 2];
    
    return download_main(url, path, dryRun);
}
