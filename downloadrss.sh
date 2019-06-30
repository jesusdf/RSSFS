#!/bin/bash
# Crontab rss downloader script.
# Requires a parameter for the url search.
# That parameter is used with a hash as a unique key.

if [ "$#" -ne 1 ]; then
  echo "Not enough parameters, example: $0 \"anything+1080p\""
  exit -1
fi

# Settings
RSS_URL="https://your-server-url.com/search?page=rss&q=$1"
INDEX_PATH=~/rssindex
OUTPUT_PATH=~/Downloads
TMP_PATH=/tmp/rssdl

INDEX_NAME=$( echo "$1" | md5sum | cut -f1 -d" " )
INDEX_FILE=${INDEX_PATH}/${INDEX_NAME}
TMP_INDEX_FILE=${TMP_PATH}/${INDEX_NAME}

if [ ! -d "${INDEX_PATH}" ]; then
  mkdir "${INDEX_PATH}"
fi

if [ ! -f "${INDEX_FILE}" ]; then
  touch "${INDEX_FILE}"
fi

if [ -d "${TMP_PATH}" ]; then
  rm -rf "${TMP_PATH}"
fi

mkdir "${TMP_PATH}"
cd "${TMP_PATH}"

# Download the file list
/usr/local/bin/rssdl --dry-run "${RSS_URL}" "${TMP_PATH}" | grep " OK" | sed -e 's/ OK//' > "${TMP_INDEX_FILE}"

# And check if there's any addition
NEW_FILE=$( diff "${INDEX_FILE}" "${TMP_INDEX_FILE}" | grep \> | cut -d\  -f2- | tail -n1 )

if [ -z "${NEW_FILE}" ]; then
  echo "Nothing new."
else
  echo "Downloading file '${NEW_FILE}'..."
  /usr/local/bin/rssdl "${RSS_URL}" "${TMP_PATH}" "${NEW_FILE}"
  cp "${NEW_FILE}" "${OUTPUT_PATH}"
  cat "${TMP_INDEX_FILE}" > "${INDEX_FILE}"
fi

# Cleanup
rm -f "${OUTPUT_PATH}/*.added"

rm -rf "${TMP_PATH}"

