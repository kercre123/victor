# Change the IP address to match your board
IP=192.168.1.125

OUT_FILENAME=a7_out.txt

mkdir build
cd build

# Compile and run
AR=arm-linux-gnueabihf-gcc-ar-4.9 AS=arm-linux-gnueabihf-gcc-as-4.9 CC=arm-linux-gnueabihf-gcc-4.9 CXX=arm-linux-gnueabihf-g++-4.9 cmake .. -DCMAKE_BUILD_TYPE=Release -DEMBEDDED_USE_GTEST=0 -DEMBEDDED_USE_MATLAB=0 -DEMBEDDED_USE_OPENCV=0
make -j8 

# Start python in background, and run the other stuff
python ../python/addSourceToGccAssembly.py . &

# Upload the executable, run it, and get the results
cd Unix\ Makefiles/bin/Release/ 
scp run_pc_embeddedTests linaro@$IP:/home/linaro/
cd ../../../..
ssh linaro@192.168.1.125 "/home/linaro/run_pc_embeddedTests > $OUT_FILENAME"
scp "linaro@192.168.1.125:/home/linaro/$OUT_FILENAME" .

# Wait for the python to complete
fg
