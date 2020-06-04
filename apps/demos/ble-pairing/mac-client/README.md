# Mac Client - Terminal application

## Building the client

You can build the client using the build_client.sh file. To run it use the following command

`./build_client.sh`

While building if you might encounter this error

```
xcode-select: error: tool 'xcodebuild' requires Xcode, but active developer directory '/Library/Developer/CommandLineTools' is a command line tools instance
```

To remove the error run the following commands.

```
xcode-select — install
sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
sudo xcodebuild -license accept
```

## Running the client

You can run the mac client using the following command

`./_build/mac_client`
