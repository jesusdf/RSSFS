CC = gcc
PROGRAMS=rssfs rssdl
FUSE_CFLAGS ?= `pkg-config --cflags --libs fuse`
CURL_CFLAGS ?= `curl-config --cflags --libs`
XML_CFLAGS ?= `xml2-config --cflags --libs`
RSSDL_FLAGS=-DNO_PREFETCH -DDELAY_METADATA_LOADING
OPTIMIZE_FLAGS=-O3 -march=native -mtune=native -pipe

rssfs: 
	@$(CC) $(FUSE_CFLAGS) $(CURL_CFLAGS) $(XML_CFLAGS) $(OPTIMIZE_FLAGS) src/http_fetcher.c src/rss_parser.c src/rssfs.c `pkg-config fuse --cflags --libs` -o rssfs -lcurl -lxml2 -lpthread -Wno-incompatible-pointer-types
rssdl: 
	@$(CC) $(CURL_CFLAGS) $(XML_CFLAGS) $(RSSDL_FLAGS) $(OPTIMIZE_FLAGS) src/http_fetcher.c src/rss_parser.c src/rssdl.c -o rssdl -lcurl -lxml2 -lpthread -Wno-incompatible-pointer-types

all: $(PROGRAMS)
clean:
	@rm -f rssfs
	@rm -f rssdl
install:
	@cp rss* /usr/local/bin
