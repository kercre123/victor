# Change the IP address to match your board
IP=192.168.1.125
OUT_FILENAME=a7_out.txt
VERBOSE_AUTOVECTORIZE=1

#THIS_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

mkdir build
cd build

# Delete executable
if [ -f "./Unix Makefiles/bin/Debug/run_pc_embeddedTests" ]
then
  cd Unix\ Makefiles/bin/Debug/ 
  rm run_pc_embeddedTests
  cd ../../..
fi

# Move output file
mv ../${OUT_FILENAME} ../${OUT_FILENAME}.old

# Compile and run
AR=arm-linux-gnueabihf-gcc-ar-4.9 AS=arm-linux-gnueabihf-gcc-as-4.9 CC=arm-linux-gnueabihf-gcc-4.9 CXX=arm-linux-gnueabihf-g++-4.9 cmake .. -DCMAKE_BUILD_TYPE=Debug -DEMBEDDED_USE_GTEST=0 -DEMBEDDED_USE_MATLAB=0 -DEMBEDDED_USE_OPENCV=0 -DCPU_A7=1 -DVERBOSE_AUTOVECTORIZE=${VERBOSE_AUTOVECTORIZE}
make -j8 

# Start python in background, and run the other stuff
python ../python/addSourceToGccAssembly.py . &

# Upload the executable, run it, and get the results
if [ -f "./Unix Makefiles/bin/Debug/run_pc_embeddedTests" ]
then
  cd Unix\ Makefiles/bin/Debug/ 
  scp run_pc_embeddedTests linaro@$IP:/home/linaro/
  cd ../../../..
  ssh linaro@192.168.1.125 "rm ${OUT_FILENAME} ; nice -n 0 taskset -c 0 /home/linaro/run_pc_embeddedTests > ${OUT_FILENAME}"
  scp "linaro@192.168.1.125:/home/linaro/${OUT_FILENAME}" .
  echo "Compile succeded"
else
  echo "Compile failed"
fi

# Wait for the python to complete
fg
