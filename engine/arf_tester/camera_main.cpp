#include <unistd.h>
#include <fstream>
#include <signal.h>

#include "camera/cameraService.h"
#include "coretech/vision/engine/camera.h"
#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/engine/compressedImage.h"
#include "util/logging/logging.h"
#include "json/json.h"

bool g_run = true;

void intHandler(int /*dummy*/) {
    g_run = false;
}

int main(int argc, char **argv)
{
    Anki::Vector::CameraService *camera_service = Anki::Vector::CameraService::getInstance();

    Anki::Vision::Camera camera;
    // From factory test
    const std::array<f32, 8> distortionCoeffs = {{-0.03822904514363595, -0.2964213946476391, -0.00181089972406104, 0.001866070303033584, 0.1803429725181202,
                                                  0, 0, 0}};
    auto calib = std::make_shared<Anki::Vision::CameraCalibration>(360,
                                                                   640,
                                                                   364.7223064012286,
                                                                   366.1693698832141,
                                                                   310.6264440545544,
                                                                   196.6729350209868,
                                                                   0,
                                                                   distortionCoeffs);

    camera.SetCalibration(calib);

    const std::string data_path = "/anki/data/assets/cozmo_resources/config/engine/vision/";

    const std::string jsonFilename = "/anki/data/assets/cozmo_resources/config/engine/vision_config.json";
    std::ifstream jsonFile(jsonFilename);
    Json::Reader reader;
    Json::Value jsonData;
    bool success = reader.parse(jsonFile, jsonData);
    if (!success)
    {
        printf("Problem\n");
    }
    jsonFile.close();

    Anki::Vision::FaceTracker face_tracker(camera, data_path, jsonData);

    Anki::Vision::ImageBuffer image_buffer;
    while (g_run) {
        usleep(100000);
        Anki::Result update_result = camera_service->Update();
        if (update_result != Anki::RESULT_OK) {
            printf("Couldn't update camera.\n");
            continue;
        }
        if (!camera_service->HaveGottenFrame()) {
            printf("Haven't gotten a frame\n");
        } else {
            printf("Got frame.\n");
        }
        bool got_frame = camera_service->CameraGetFrame(camera_service->GetTimeStamp(), image_buffer);
        if (!got_frame) {
            printf("Failed to get frame.\n");
            continue;
        }
        Anki::Vision::Image image;
        bool convert_to_gray = image_buffer.GetGray(image, Anki::Vision::ImageCacheSize::Full);
        if (!convert_to_gray) {
            printf("Couldn't convert to gray");
            camera_service->CameraReleaseFrame(image_buffer.GetImageId());
        }
        std::list<Anki::Vision::TrackedFace> face_list;
        std::list<Anki::Vision::UpdatedFaceID> updated_ids_list;
        Anki::Vision::DebugImageList<Anki::Vision::CompressedImage> debug_images;
        Anki::Result result = face_tracker.Update(image, 1.f, face_list, updated_ids_list, debug_images);
        if (result == 0) {
            printf("Result ok.  Num %zu\n", face_list.size());
        } else {
            printf("Result not ok: %d", result);
        }
        camera_service->CameraReleaseFrame(image_buffer.GetImageId());
    }
}