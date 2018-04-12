output="_build"
echo "Downloading readline library"
brew install readline

echo "Building mac-client in ./$output/mac-client"
xcodebuild -scheme mac-client build CONFIGURATION_BUILD_DIR=./$output/
