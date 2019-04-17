#include "robot_to_arf_converter.h"


int main(int argc, char** argv) {
    ARF::RobotToArfConverter converter;

    bool connected = converter.Connect();

    if (connected) {
        std::cout << "Successfully connected." << std::endl;
    } else {
        std::cout << "Did not connect." << std::endl;
    }
    converter.Disconnect();
}
