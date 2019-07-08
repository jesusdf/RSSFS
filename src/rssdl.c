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
#include <malloc.h>
#include <ctype.h>

#include "rssfs.h"
#include "http_fetcher.h"
#include "rss_parser.h"

// The linked list struct for our data
RssData * datalist;

typedef int bool;
#define false 0
#define true !false
#define null false

#define SKIP(text, c) while((int)*text == c && *text != null) text++;
#define SKIP_UNTIL(text, c) while((int)*text != c && (int)*text != null) text++;
#define CLONE(a, b) a=calloc(strlen(b), sizeof(char)); { char *p=b; while(*p != null) { *(char *)(a + (p - b))=tolower(*p); p++; }}
#define COMPARE_WORD(a, b, equal) while (*a != null && *b != null && equal && *b != '?' && *b != '*' && *a == *b) { a++; b++; } equal=(*a == *b || *b == '?' || *b == '*');

bool matches(char * text, char * pattern, bool ignoreCase) {
	bool areEqual = true;
	bool anyChar = false;

	/* No string to compare */
	if (*text == null && *pattern == null)
		return true;
	/* No pattern is an error */
	if (*pattern == null)
		return false;

	char * current;
	char * challenge;
	char * a;
	char * b;
	if (ignoreCase) {
		/* make a lowercase copy to ignore case */
		CLONE(a, text);
		CLONE(b, pattern);
		current = a;
		challenge = b;
	} else {
		current = text;
		challenge = pattern;
	}
	
	char * next;
	do {
		next = (char *)(challenge + 1);
		switch( *challenge ) {
			case '*':
				/* Skip redundant characters */
				SKIP(challenge, '*');
				anyChar = true;
				break;
			case '?':
				while (*challenge == '?' && *current != null) {
					if(anyChar && (*next == null)) {
						SKIP_UNTIL(current, null);
						current--;
						anyChar = false;
					}
					challenge++;
					current++;
					next = (char *)(challenge + 1);
				}
				areEqual = ((*current == null && *challenge == null) ||
							(*current != null && *challenge != null));
				break;
			default:
				if (anyChar) {
					/* Is it the last character? */
					if (*next == null) {
						SKIP_UNTIL(current, null);
						current--;
					} else {
						/* Find a potential matching word */
						SKIP_UNTIL(current, *challenge);
					}
				}
				/* Compare the whole word */
				COMPARE_WORD(current, challenge, areEqual);
				if (areEqual) {
					/* The next char is a * or ? or we finished */
					anyChar = false;
				} else {
					if (anyChar) {
						/* 
						* If the previous challenge char was an asterisk,
						* then we found a similar word, but not the exact one 
						* that we were looking for.
						*/
						int wordSize = challenge - (next - 1);
						/* Rewind + 1 and keep searching if there's text left. */
						if (wordSize > 0) {
							current = (char *)(current - wordSize + 1);
							challenge = (next - 1);
							if (*current == null) {
								anyChar = false;
							} else {
								areEqual = true;
							}
						}
					}
				}
				break;
		}
	} while (areEqual && *current != null && *challenge != null);

	if (ignoreCase) {
		free(a);
		free(b);
	}

	return areEqual;

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
                !matches(current->title, pattern, true)) {
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
