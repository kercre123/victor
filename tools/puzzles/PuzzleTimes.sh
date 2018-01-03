echo "Starting Simulated Maze Runs"
OUTPUT_DIR=$(pwd)
cd $OUTPUT_DIR/../../_build/mac/Debug/test
GTEST_ALSO_RUN_DISABLED_TESTS=1 GTEST_FILTER="PuzzleMazeBehavior.*" ctest -V > $OUTPUT_DIR/output.txt
echo "Completed please check output.txt"
