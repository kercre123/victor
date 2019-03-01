#include <unistd.h>

#include "camera/cameraService.h"
#include "util/logging/logging.h"

int main(int argc, char** argv) {
    Anki::Vector::CameraService* camera_service = Anki::Vector::CameraService::getInstance();

    camera_service->InitCamera();

    for (int i = 0; i < 10; ++i) {
        usleep(100000);
        printf("Camera timestamp %lu\n", (unsigned long) camera_service->GetTimeStamp());
    }
    camera_service->DeleteCamera();
}