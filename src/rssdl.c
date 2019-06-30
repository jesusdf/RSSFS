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

#define true -1
#define false 0

// The linked list struct for our data
RssData * datalist;

// Function that matches input str with 
// given wildcard pattern 
// https://www.geeksforgeeks.org/wildcard-pattern-matching/
int strmatch(char str[], char pattern[], 
              int n, int m) 
{ 
    // empty pattern can only match with 
    // empty string 
    if (m == 0) 
        return (n == 0); 
  
    // lookup table for storing results of 
    // subproblems 
    int lookup[n + 1][m + 1]; 
  
    // initailze lookup table to false 
    memset(lookup, false, sizeof(lookup)); 
  
    // empty pattern can match with empty string 
    lookup[0][0] = true;
  
    // Only '*' can match with empty string 
    for (int j = 1; j <= m; j++) 
        if (pattern[j - 1] == '*') 
            lookup[0][j] = lookup[0][j - 1]; 
  
    // fill the table in bottom-up fashion 
    for (int i = 1; i <= n; i++) 
    { 
        for (int j = 1; j <= m; j++) 
        { 
            // Two cases if we see a '*' 
            // a) We ignore ‘*’ character and move 
            //    to next  character in the pattern, 
            //     i.e., ‘*’ indicates an empty sequence. 
            // b) '*' character matches with ith 
            //     character in input 
            if (pattern[j - 1] == '*') 
                lookup[i][j] = lookup[i][j - 1] || 
                               lookup[i - 1][j]; 
  
            // Current characters are considered as 
            // matching in two cases 
            // (a) current character of pattern is '?' 
            // (b) characters actually match 
            else if (pattern[j - 1] == '?' || 
                    str[i - 1] == pattern[j - 1]) 
                lookup[i][j] = lookup[i - 1][j - 1]; 
  
            // If characters don't match 
            else lookup[i][j] = false; 
        } 
    } 
  
    return lookup[n][m]; 
} 


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

int download_main(char * rssUrl, char * downloadPath, char * pattern, int dryRun) {
    int res;
    int counter = 0;

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

    if (pattern != NULL)
        printf("Looking for files that match the '%s' pattern.\n", pattern);

    if (dryRun)
        printf("Dry run, no files will be written to disk.\n");

    printf("Downloading files...\n");

    RssData *current = datalist;
    if (current != NULL) {
        while (current != NULL) {

            if (pattern != NULL && 
                !strmatch(current->title, pattern, strlen(current->title), strlen(pattern))) {
                // filename doesn't match, skip it.
                current = current->next;
                continue;
            }
            
            counter++;

            if (dryRun) {
                printf("%s OK\n", current->title);
            } else {
                res = download_file(current->title, current->link);
            }
            
            current = current->next;
        }
    }

    printf("Downloaded %d files.\n", counter);

    return EXIT_SUCCESS;
}

/* Print usage information on stderr */
void usage(char * argve) {
    fprintf(stderr, "Version: %s %s\n", argve, VERSION);
    fprintf(stderr, "Usage: %s [--dry-run] url download-dir [filename-wildcard]\n", argve);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "   --dry-run:\n");
    fprintf(stderr, "       Don't actually write anything to disk.\n");
    fprintf(stderr, "   filename-wildcard:\n");
    fprintf(stderr, "       Look only for files that match the pattern.\n");
    fprintf(stderr, "       Simple wildcard expressions with * and ? for anything/any char.\n");
    fprintf(stderr, "       Example: MyFilename*1080p*201?\n");
}

/* Main function */
int main(int argc, char *argv[]) {
    char *url;
    char *path;
    char *pattern = NULL;
    int dryRun=0;

    if (argc < 3 | argc > 5) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    
    dryRun=(argc >= 4 && strncmp(argv[1], "--dry-run", 9) == 0) ? 1 : 0;
    url = argv[dryRun + 1];
    path = argv[dryRun + 2];
    if (argc = (dryRun + 4)) {
        pattern = argv[dryRun + 3];
    }
    
    return download_main(url, path, pattern, dryRun);
}
