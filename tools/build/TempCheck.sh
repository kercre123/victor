#!/bin/bash

# Brad's command to search for TEMP in code and output a warning. This
# must be run from the root directory. if the -fail argument is
# present, it will return a nonzero exit code if it finds a temp,
# otherwise it just prints the warnings

if [ "$1" = "-fail" ]
then
    ERROR_STR="error"
else
    ERROR_STR="warning"
fi

echo "Brads TEMP check running..."
RES=$(find ./source/anki ./resources/config  \( \
    -iname '*.cpp' \
    -or -iname '*.h' \
    -or -iname '*.m' \
    -or -iname '*.mm' \
    -or -iname '*.json' \
    -or -iname '*.cfg' \) \
    -and -not -path '*/lib/*' \
    -exec fgrep -wnH 'TEMP' {} \+)

# find returns nonzero if no TEMPs were found
NOTFOUND=$?

FILT=$(echo "$RES" | sed "s;^\([^ ]*:\)\(.*\)$;${PWD}/\1 $ERROR_STR: found temp: \2;")

if [ "$1" = "-fail" -a $NOTFOUND -eq 0 ] # -a "$FILT" -ne "" ]
then
    echo "$FILT"
    exit 1
fi

echo "all clear!"

# otherwise just reutrn 0
exit 0
