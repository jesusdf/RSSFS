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
 * HTTP handler
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include <curl/curl.h>
#include <curl/easy.h>

struct MemoryStruct {
    unsigned char * memory;
    size_t size;
};

void *myrealloc(void *ptr, size_t size) {
    /* There might be a realloc() out there that doesn't like reallocing
       NULL pointers, so we take care of it here */
    if(ptr)
        return realloc(ptr, size);
    else
        return malloc(size);
}

size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data) {
    // The number of bytes, times the number of elements its sending. (i.e, 4 chunks of 10 bytes)
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)data;

    mem->memory = (unsigned char *)myrealloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory) {
        memcpy(&(mem->memory[mem->size]), ptr, realsize);
        mem->size += realsize;
        mem->memory[mem->size] = 0;
    }
    return realsize;
}

// Fetch a URL, returns the file size in int, or NULL if there is an error. Pass a char pointer reference to get the data.
long int fetch_url_size(char *url) {
    CURL *cu ;
    CURLcode c;
    double cl = 0;
    char *cc;

    cu = curl_easy_init(); 
    curl_easy_setopt(cu,CURLOPT_URL, url);
    curl_easy_setopt(cu,CURLOPT_HEADER,1);
    curl_easy_setopt(cu,CURLOPT_NOBODY,1);

    /* skip ssl verification */
    curl_easy_setopt(cu, CURLOPT_SSL_VERIFYPEER, 0L);

    /* skip hostname verification */
    curl_easy_setopt(cu, CURLOPT_SSL_VERIFYHOST, 0L);

    /* some servers don't like requests that are made without a user-agent
    field, so we provide one */
    curl_easy_setopt(cu, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    /* Cookie test */
    curl_easy_setopt(cu, CURLOPT_COOKIE, "uid=xxxx; pass=xxx;");

    c = curl_easy_perform(cu);
    if(c) {
        #ifdef DEBUG
            syslog(LOG_INFO, "Could not get url info!");
        #endif
    } else {
        c = curl_easy_getinfo(cu, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &cl); 
        if(c) {
            #ifdef DEBUG
                syslog(LOG_INFO, "Could not get url file size!");
            #endif
        }
    }
    curl_easy_cleanup(cu); 
    curl_global_cleanup();

    return cl;

}

// Fetch a URL, returns the file size in int, or NULL if there is an error. Pass a char pointer reference to get the data.
long int fetch_url(char *url, char **fileBuf) {
    CURL *curl_handle;

    struct MemoryStruct chunk;
    CURLcode res;
    long int r = -1;

    chunk.memory=NULL; /* we expect realloc(NULL, size) to work */
    chunk.size = 0;    /* no data at this point */

    curl_global_init(CURL_GLOBAL_ALL);

    /* init the curl session */
    curl_handle = curl_easy_init();

    if(curl_handle) {

        /* specify URL to get */
        curl_easy_setopt(curl_handle, CURLOPT_URL, url);

        /* skip ssl verification */
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L);

        /* skip hostname verification */
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);

        /* send all data to this function  */
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

        /* we pass our 'chunk' struct to the callback function */
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

        /* some servers don't like requests that are made without a user-agent
        field, so we provide one */
        curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        /* Cookie test */
        curl_easy_setopt(curl_handle, CURLOPT_COOKIE, "uid=xxxx; pass=xxx;");

        /* get it! */ 
        res = curl_easy_perform(curl_handle);

        /* check for errors */ 
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
        }
        else
        {
            r = (long int)chunk.size;

            *fileBuf = (char *)chunk.memory;

            /* cleanup curl stuff */
            curl_easy_cleanup(curl_handle);
        }
    }
    
    curl_global_cleanup();

    return r;
}

