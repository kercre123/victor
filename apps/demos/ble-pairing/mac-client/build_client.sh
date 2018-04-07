output="_build"
echo "Building mac-client in ./$output/mac-client"
xcodebuild -scheme mac-client build CONFIGURATION_BUILD_DIR=./$output/
