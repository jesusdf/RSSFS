CC = gcc
FUSE_CFLAGS ?= `pkg-config --cflags --libs fuse`
CURL_CFLAGS ?= `curl-config --cflags --libs`
XML_CFLAGS ?= `xml2-config --cflags --libs`

rssfs: 
	@$(CC) $(FUSE_CFLAGS) $(CURL_CFLAGS) $(XML_CFLAGS) src/http_fetcher.c src/rss_parser.c src/rssfs.c `pkg-config fuse --cflags --libs` -o rssfs -lcurl -lxml2 -lpthread -Wno-incompatible-pointer-types

all: rssfs
clean:
	@rm -f rssfs
