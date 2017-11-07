#!/bin/sh
#usage: >>. ./setcredentials.sh
#space after '.' keeps the exported vars in the caller's environment

#our "server" is Dropbox via password protected external link
#NOTE: change the link postfix from 'dl=0' to 'dl=1' to allow downloads via curl
dblink=https://www.dropbox.com/sh/ux5j7jheou0f0ql/AADBmD1LmdUe-P9m0dc5_yx8a?dl=1
dbpass=r8mCxHBysGTPiHNK
foldername=emmcdl

#DEBUG: test this out with a public-access folder, no password
#echo ========= DEBUG: test folder with stock password  ================
#dblink=https://www.dropbox.com/sh/17wazbx9xu5rjg0/AAC57fJwCusIs28x8-nL3zi9a?dl=1
#dbpass=""
#foldername=emmcdl_test

#DEBUG: test this out with a password protected file (not folder)
echo ========= DEBUG: test pass-protected file ================
dblink=https://www.dropbox.com/s/mge4v8dcnt2geiy/20171102%20emmcdl.zip?dl=1
dbpass=r8mCxHBysGTPiHNK
foldername=emmcdl_file

echo server/"$foldername" @ $dblink
export dblink
export dbpass
export foldername

#Usage: dl via curl or wget (dropbox will zip everything before sending):
#wget --max-redirect=20 -O download.zip https://www.dropbox.com/sh/igoku2mqsjqsmx1/AAAeF57DR2
#wget -O ${foldername}.zip --password=$dbpass $dblink
#curl -L -o output.zip https://www.dropbox.com/sh/[folderLink]?dl=1
#curl -L -o ${foldername}.zip --pass $dbpass $dblink
#unzip $foldername.zip -d $foldername
