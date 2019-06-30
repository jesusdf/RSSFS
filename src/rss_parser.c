/*
 * RSSFS - RSS in userspace.
 * http://www.jardinpresente.com.ar/wiki/index.php/RSSFS
 *
 * Copyright (C) 2007 Marc E. <santusmarc@users.sourceforge.net>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * RSS parser functions and storage handlers
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "rssfs.h"
#include "rss_parser.h"
#include "http_fetcher.h"

// Invalid characters in filenames
char invalid_char[10] = {'/', '\\', '?', '%', '*', ':', '|', '"', '<', '>'};

// Checks filename, and replaces invalid chars the invalid_char character array.
char *checkFilename(char *filename) {
    if (strcspn(filename, invalid_char) != strlen(filename)) {
        int i;
        char * stringptr;
        // Needed
        char * filenamecopy = strdup(filename);

        for (i = 0; i < sizeof(invalid_char); i++) {
            if ((stringptr = strchr(filenamecopy, invalid_char[i])) != NULL) {
                *stringptr = '.';
            } 
        }
        return filenamecopy;
    } else {
        return filename;
    }
}

static long int singleFetch(char * link, char * file_content) {
    #ifdef PREFETCH
        return fetch_url(link, &file_content);
    #else
        return fetch_url_size(link);
    #endif
}

#ifdef MULTITHREADS

pthread_mutex_t urlMutex;

static void * threadGetSize(void *arg) {
    RssData *datalist = (RssData *) arg;
    
    if (datalist != NULL) {
        // enabling this makes listing files asynchronous, but file size is delayed
        //pthread_detach(datalist->thread);
        pthread_mutex_lock(&urlMutex);
        datalist->size = singleFetch(datalist->link, datalist->file_content);
        pthread_mutex_unlock(&urlMutex);
        #ifdef DEBUG
            syslog(LOG_INFO, "Size is %ld for '%s'", datalist->size, datalist->link);
        #endif 
    }
    
    pthread_exit(NULL);
    return 0;
}

void waitForData(RssData *datalist) {
    
    RssData *current = datalist;
    
    #ifdef DEBUG
        syslog(LOG_INFO, "Waiting for threads...");
    #endif 

    while (current != NULL) {
        if (current->thread) {
            pthread_join(current->thread, NULL);
        }
        current = current->next;
    }

    #ifdef DEBUG
        syslog(LOG_INFO, "All threads finished.");
    #endif
}

#endif

long int QueryFileSize(RssData * current) {
    #ifdef MULTITHREADS
        /* get file size on a separate thread */
        pthread_create(&current->thread, NULL, &threadGetSize, current);
    #else
        return singleFetch(new->link, new->file_content);
    #endif
}

// Adds a record to a RssData struct
RssData * addRecord(RssData *datalist, int counter, const xmlChar *title, const xmlChar *link, long int size) {
    //printf("Add %d: %s - %s!\n", counter, (char *)title, (char *)link);

    // Clean out invalid chars
    char *titleclean = checkFilename((char *)title);

    RssData * new = malloc(sizeof(RssData));
    if (new == NULL) {
        fprintf(stderr, "Could not allocate memory\n");
    }
    new->number = counter;
#ifdef RSSEXT
    sprintf(new->title, "%s%s", titleclean, RSSEXT);
#else
    sprintf(new->title, "%s", titleclean);
#endif
    sprintf(new->link, "%s", link);
    #ifdef DELAY_METADATA_LOADING
        new->size = -1;
    #else
        new->size = 0;
        if (size == -1) {
            new->size = QueryFileSize(new);
        } else {
            new->size = size;
        }
    #endif
    new->next = datalist;
    datalist = new;
    return datalist;
}

void printRecord(RssData *datalist) {
    printf("Number: %d\n", datalist->number);
    printf("Title: %s\n", datalist->title);
    printf("Link: %s\n", datalist->link);
}

void printAllRecords(RssData *datalist) {
    if (datalist != NULL) {
        while (datalist != NULL) {
            printRecord(datalist);
            datalist = datalist->next;
        }
    } else {
        printf("Error: No records have been entered yet!\n");
    }
}

// Returns a pointer to the record if found, NULL otherwise.
RssData * getRecordByTitle(RssData *datalist, const char *title) {
    int found = 0;
    RssData *current = datalist;
    while ((current != NULL) && !found) {
        if (!strcmp(current->title, title)) {
            return current;
        } else {
            current = current->next;
        }
    }
    return NULL;
}

// Returns -1 for not found, and 0 for found.
int findRecordByTitle(RssData *datalist, const char *title) {
    return getRecordByTitle(datalist, title) == NULL ? -1 : 0;
}

// Returns the url by title, or (char *)-1 if it cant be found.
char * getRecordUrlByTitle(RssData *datalist, const char *title) {
    int found = 0;
    RssData *current = datalist;
    while ((current != NULL) && !found) {
        if (!strcmp(current->title, title)) {
            found = 1;
            return (char *)current->link;
        } else {
            current = current->next;
        }
    }
    // FIXME
    return (char *)-1;
}

// Returns the file size as a long int, or -1 if it can't be found
long int getRecordFileSizeByTitle(RssData *datalist, const char *title) {
    int found = 0;
    RssData *current = datalist;
    while ((current != NULL) && !found) {
        if (!strcmp(current->title, title)) {
            long int s = -1;
            found = 1;
            s = current->size;
            return s;
        } else {
            current = current->next;
        }
    }
    return -1;
}

void startIteration(void) {
    #ifdef MULTITHREADS
        pthread_mutex_init(&urlMutex, NULL);
    #endif
}

void ensureIterationFinished(RssData * datalist) {
    #ifndef DELAY_METADATA_LOADING
        #ifdef MULTITHREADS
            waitForData(datalist);
            pthread_mutex_destroy(&urlMutex);
        #endif
    #endif
}

static RssData * iterate_xml(xmlNode *root_node) {
    xmlNode * root_node_children;
    xmlNode * channel_node_children;
    xmlNode * item_node_children;
    xmlChar * link;
    xmlChar * title;
    RssData * datalist = NULL;
    int counter = 0;
    long int size = -1;

    startIteration();

    // Top level (<rss>)
    for (; root_node; root_node = root_node->next) {
        if(root_node->type != XML_ELEMENT_NODE) {
            continue;
        }

        // Sub level (<channel>)
        for (root_node_children = root_node->children; root_node_children; root_node_children = root_node_children->next) {
            if (root_node_children->type != XML_ELEMENT_NODE) {
                continue;
            }

            // Channel sub level (<title>, <item>, etc)
            for (channel_node_children = root_node_children->children; channel_node_children; channel_node_children = channel_node_children->next) {
                // We only care about <item>
                if ((channel_node_children->type != XML_ELEMENT_NODE) || (strcmp((const char *)channel_node_children->name, "item") != 0)) {
                    continue;
                }

                // Each <item> is a recordset counted
                counter++;

                // Item sub level (<title>, <link>, <description>)
                for (item_node_children = channel_node_children->children; item_node_children; item_node_children = item_node_children->next) {
                    if (item_node_children->type != XML_ELEMENT_NODE)
                        continue;

                    if (strcmp((const char *)item_node_children->name, "title") == 0) {
                        title = item_node_children->children->content;
                    }

                    // Set our link and titles
                    if (strcmp((const char *)item_node_children->name, "link") == 0) {
                        link = item_node_children->children->content;
                    }
                }

                // Add the item's data to a linked list
                datalist = addRecord(datalist, counter, title, link, size);
                //printf("%d %s %s\n", counter, link, title);

            }
        }
    }

    ensureIterationFinished(datalist);

    printf("Found %d items.\n", counter);
    #ifdef DEBUG
        syslog(LOG_INFO, "%d items found.", counter);
    #endif 

    return datalist;
}

#ifdef LIBXML_TREE_ENABLED
RssData * loadRSS(char *url) {
    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;
    char * file_content;
    long int size;

    LIBXML_TEST_VERSION

    #ifdef DEBUG
        syslog(LOG_INFO, "Reading XML...");
        #ifdef PREFETCH
            syslog(LOG_INFO, "Prefetching enabled.");
        #else
            syslog(LOG_INFO, "Prefetching disabled.");
        #endif
        #ifdef DELAY_METADATA_LOADING
            syslog(LOG_INFO, "Delaying metadata loading...");
        #else
            syslog(LOG_INFO, "Metadata loading on the fly...");
        #endif
    #endif

    size = fetch_url(url, &file_content);

    if (size == -1) {
        #ifdef DEBUG
            syslog(LOG_INFO, "Download failed.");
        #endif 
        size = 0;
    }

    if (size >0) 
    {

        /* doc = xmlReadFile(url, NULL, 0); */
        doc = xmlReadDoc((xmlChar *)(file_content), NULL, NULL, XML_PARSE_NOBLANKS | XML_PARSE_OLD10);

        if (doc == NULL) {
            #ifdef DEBUG
                syslog(LOG_INFO, "No content.");
            #endif
            return NULL;
        }
        root_element = xmlDocGetRootElement(doc);


        RssData * datalist = iterate_xml(root_element);

        xmlFreeDoc(doc);
        xmlCleanupParser();

        return datalist;

    }

    return NULL;

}

#else
fprintf(stderr, "Tree support not compiled in\n");
exit(1);
#endif
