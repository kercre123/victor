#!/bin/sh

# Uninstall Acapela TTS for Mac OS X.

echo "Uninstalling previous installation of Acapela TTS for Mac OS X..."
echo "You may have to enter your account password."
echo ""
echo "Note: Your licenses will not be deleted."

sudo rm -R /Applications/Acapela\ TTS\ for\ Mac > /dev/null
sudo rm -R /Library/Documentation/Applications/Acapela\ TTS\ for\ Mac > /dev/null
if [-f /Library/Speech/Synthesizers/AcapelaSynth.SpeechSynthesizer ]; then
    sudo rm -R /Library/Speech/Synthesizers/AcapelaSynth.SpeechSynthesizer > /dev/null
fi

for name in $(ls /Library/Receipts)
do 
	case "$name" in
    	*AcapelaTTS* ) sudo rm -r /Library/Receipts/$name > /dev/null
	esac
done

for id in $(/usr/sbin/pkgutil --pkgs=\\com.acapela*)
do 
	sudo /usr/sbin/pkgutil --forget $id > /dev/null
done

echo ""
echo "Uninstall succesfull"