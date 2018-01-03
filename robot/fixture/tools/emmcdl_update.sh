#!/bin/sh

#get server info/creds (sets local env vars)
. ./setcredentials.sh
dbpasslen=${#dbpass}

#clean old files
rm ${foldername}_w.zip
rm ${foldername}_c.zip
rm -rf ${foldername}_w
rm -rf ${foldername}_c

#fetch latest and unpack (pwd protected)
if [ "$dbpasslen" -gt "0" ]; then #password length > 0
  echo "wget -O ${foldername}_w.zip --password=pwd $dblink"
  wget -O ${foldername}_w.zip --password=$dbpass $dblink
  echo "curl -L -o ${foldername}_c.zip --pass pwd $dblink"
  curl -L -o ${foldername}_c.zip --pass $dbpass $dblink
elif [ "$dbpasslen" -eq "0" ]; then #no password
  echo "wget -O ${foldername}_w.zip $dblink"
  wget -O ${foldername}_w.zip $dblink
  echo "curl -L -o ${foldername}_c.zip $dblink"
  curl -L -o ${foldername}_c.zip $dblink
else
  echo unknown plen=$dbpasslen
fi
unzip ${foldername}_w.zip -d ${foldername}_w
unzip ${foldername}_c.zip -d ${foldername}_c

#cleanup temp files
#rm ${foldername}_w.zip
#rm ${foldername}_c.zip

#fetch dropbox CLI
#mkdir -p ~/bin
#wget -O ~/bin/dropbox.py "https://www.dropbox.com/download?dl=packages/dropbox.py"
#chmod +x ~/bin/dropbox.py
#~/bin/dropbox.py help
