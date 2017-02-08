#!/bin/bash

# see https://docs.smartling.com/display/docs/Files+API#FilesAPI-/file/list(GET)
SUPPORTED_CONDITIONS=(haveAtLeastOneUnapproved haveAtLeastOneApproved haveAtLeastOneTranslated haveAllTranslated haveAllApproved haveAllUnapproved)

# see https://docs.smartling.com/display/docs/Files+API#FilesAPI-/file/upload(POST)
SUPPORTED_FILE_TYPES=(android ios gettext plaintext csv html xml json javaProperties yaml qt xliff docx pptx xlsx idml resx stringsdict)

# see https://docs.smartling.com/display/docs/Files+API#FilesAPI-/file/get(GET)
SUPPORTED_DOWNLOAD_TYPES=(original pending published pseudo contextMatchingInstrumented)

# supported URI prefix schemes
SUPPORTED_FILE_URI_SCHEMES=(fileName relativePath prefix)
DEFAULT_FILE_URI_SCHEME='fileName'
DEFAULT_FILE_PREFIX_SEPARATOR='/'

MAX_FILES_TO_DISPLAY=25

# see https://docs.smartling.com/display/docs/Glossary+Export+API
SUPPORTED_GLOSSARY_TYPES=(CSV TBX)

# see https://docs.smartling.com/display/docs/Translations+API
SUPPORTED_TMX_TYPES=(full published)

# curl
CURL_OPTIONS=()
# if enabled HTTP headers will be saved in a separate file in the same directory as the downloaded file
KEEP_HTTP_HEADERS=false

# regexes
RESPONSE_CODE_REGEX='"code":"([^"]+)"'
RESPONSE_MESSAGES_REGEX='"messages":\[(.*)\]'

LOCALE_REGEX='"locale":"([^"]+)"'
FILE_URI_REGEX='"fileUri":"(.+?)",'
FILE_COUNT_REGEX='"fileCount":([0-9]+)'
WORD_COUNT_REGEX='"wordCount":([0-9]+)'
STRING_COUNT_REGEX='"stringCount":([0-9]+)'
APPROVED_STRING_COUNT_REGEX='"approvedStringCount":([0-9]+)'
COMPLETED_STRING_COUNT_REGEX='"completedStringCount":([0-9]+)'
OVERWRITTEN_REGEX='"overWritten":(true|false)'

# global vars
SM_RESPONSE=""
SM_CODE=""
