#include <unistd.h>
#include <fstream>
#include <signal.h>

#include "camera/cameraService.h"
#include "coretech/vision/engine/camera.h"
#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/engine/compressedImage.h"
#include "coretech/common/shared/array2d_impl.h"
#include "coretech/vision/engine/image.h"
#include "arf/arf.h"
#include "arf/Intraprocess.h"
#include "arf/Nodes.h"
#include "arf/Threads.h"
#include "util/logging/logging.h"
#include "json/json.h"

#include "generated/proto/arf/ArfMessage.pb.h"
#include "generated/proto/arf/Image.pb.h"

bool g_run = true;

void intHandler(int /*dummy*/)
{
    g_run = false;
}

using ARF::DataEvent;
using ARF::InputPort;
using ARF::Logger;
using ARF::Node;
using ARF::OutputPort;
using ARF::TaggedItem;
using ARF::Task;
using ARF::Threadpool;

using TaggedImageBuffer = TaggedItem<std::shared_ptr<Anki::Vision::ImageBuffer>>;
using TaggedImage = TaggedItem<std::shared_ptr<Anki::Vision::Image>>;
using TrackedFacesAndImagePair = std::pair<std::shared_ptr<std::list<Anki::Vision::TrackedFace>>,
                                           std::shared_ptr<Anki::Vision::Image>>;
using TaggedTrackedFacesAndImage = TaggedItem<TrackedFacesAndImagePair>;

class CameraNode : public Node
{
  public:
    static constexpr const char *OUTPUT_CAMERA = "camera";
    CameraNode(const std::string &node_name) : Node(node_name), camera_service_(Anki::Vector::CameraService::getInstance()) {}

    bool Initialize()
    {
        OutputPort<TaggedImageBuffer> *output = Node::CreateOutputPort<TaggedImageBuffer>(OUTPUT_CAMERA);

        camera_task_ = [output, this]() {
            usleep(10000);
            Anki::Result update_result = camera_service_->Update();
            if (update_result != Anki::RESULT_OK)
            {
                printf("Couldn't update camera.\n");
                Threadpool::Inst().EnqueueTask(camera_task_);
                return;
            }

            std::shared_ptr<Anki::Vision::ImageBuffer> shared_buffer(
                new Anki::Vision::ImageBuffer(),
                [](Anki::Vision::ImageBuffer *buffer) {
                    Anki::Vector::CameraService *camera_service = Anki::Vector::CameraService::getInstance();
                    camera_service->CameraReleaseFrame(buffer->GetImageId());
                });
            bool got_frame = camera_service_->CameraGetFrame(camera_service_->GetTimeStamp(), *shared_buffer);
            if (!got_frame)
            {
                Threadpool::Inst().EnqueueTask(camera_task_);
                return;
            }
            TaggedImageBuffer tagged_image_buffer(shared_buffer);
            CreateAndLogEvent(tagged_image_buffer, {GetUUID()}, DataEvent::Type::CREATION);
            output->Write(tagged_image_buffer, GetUUID());
            Threadpool::Inst().EnqueueTask(camera_task_);
        };

        if (!output)
        {
            return false;
        }
        return true;
    }

    void Start()
    {
        Threadpool::Inst().EnqueueTask(camera_task_);
    }

  private:
    Anki::Vector::CameraService *camera_service_;
    Task camera_task_;
};

class ImageNode : public Node
{
  public:
    static constexpr const char *INPUT_BUFFER = "buffer";
    static constexpr const char *OUTPUT_IMAGE = "image";
    ImageNode(const std::string &node_name) : Node(node_name) {}

    bool Initialize()
    {
        InputPort<TaggedImageBuffer> *input = Node::CreateInputPort<TaggedImageBuffer>(1, INPUT_BUFFER);
        OutputPort<TaggedImage> *output = Node::CreateOutputPort<TaggedImage>(OUTPUT_IMAGE);

        InputPort<TaggedImageBuffer>::EventCallback cb = [input, output, this](InputPort<TaggedImageBuffer> *) {
            TaggedImageBuffer image_buffer;
            if (!input->Read(image_buffer, GetUUID()))
            {
                printf("Bad read\n");
            }
            Task get_image_task = [image_buffer, output, this]() {
                TaggedImage image(std::make_shared<Anki::Vision::Image>());
                bool convert_to_gray = image_buffer.item->GetGray(*image.item, Anki::Vision::ImageCacheSize::Half);
                if (!convert_to_gray)
                {
                    printf("Couldn't convert to gray\n");
                    return;
                }
                DataEvent event(image);
                event.type = DataEvent::Type::CREATION;
                event.AddParent(image_buffer);
                event.SetTimeToNow();
                Logger::Inst().LogDataEvent(event);

                output->Write(image, GetUUID());
            };

            Threadpool::Inst().EnqueueTask(get_image_task);
        };
        input->RegisterCallback(cb);
        return true;
    }
};

class FaceTrackerNode : public Node
{
  public:
    static constexpr const char *INPUT_IMAGE = "image";
    static constexpr const char *OUTPUT_TRACKED_FACES_AND_IMAGE = "tracked_faces_and_image";
    FaceTrackerNode(const std::string &node_name) : Node(node_name)
    {
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

        camera_.SetCalibration(calib);

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

        face_tracker_.reset(new Anki::Vision::FaceTracker(camera_, data_path, jsonData));
        face_tracker_->EnableRecognition(false);
    }

    bool Initialize()
    {
        InputPort<TaggedImage> *input = Node::CreateInputPort<TaggedImage>(1, INPUT_IMAGE);
        OutputPort<TaggedTrackedFacesAndImage> *output =
            Node::CreateOutputPort<TaggedTrackedFacesAndImage>(OUTPUT_TRACKED_FACES_AND_IMAGE);

        InputPort<TaggedImage>::EventCallback cb = [input, output, this](InputPort<TaggedImage> *) {
            Task get_faces_task = [input, output, this]() {
                std::lock_guard<ARF::Mutex> lock(mutex_);
                TaggedImage image;
                bool read_ok = input->Read(image, GetUUID());
                if (!read_ok)
                {
                    printf("Read failed\n");
                    return;
                }
                TaggedTrackedFacesAndImage faces_and_image;
                faces_and_image.item.first = std::make_shared<std::list<Anki::Vision::TrackedFace>>();
                faces_and_image.item.second = image.item;
                std::list<Anki::Vision::UpdatedFaceID> updated_ids_list;
                Anki::Vision::DebugImageList<Anki::Vision::CompressedImage> debug_images;
                Anki::Result result = face_tracker_->Update(*faces_and_image.item.second, 1.f, *faces_and_image.item.first, updated_ids_list, debug_images);
                if (result == 0)
                {
                    printf("Result ok.  Num %zu\n", faces_and_image.item.first->size());
                }
                else
                {
                    printf("Result not ok: %d", result);
                }
                DataEvent event(faces_and_image);
                event.type = DataEvent::Type::CREATION;
                event.AddParent(image);
                event.SetTimeToNow();
                Logger::Inst().LogDataEvent(event);

                output->Write(faces_and_image, GetUUID());
            };

            Threadpool::Inst().EnqueueTask(get_faces_task);
        };
        input->RegisterCallback(cb);
        return true;
    }

  private:
    ARF::Mutex mutex_; // To guard face tracker.
    std::unique_ptr<Anki::Vision::FaceTracker> face_tracker_;
    Anki::Vision::Camera camera_;
};

class FacePublishNode : public Node
{
  public:
    static constexpr const char *INPUT_TRACKED_FACES_AND_IMAGE = "tracked_faces_and_image";
    FacePublishNode(const std::string &node_name) : Node(node_name)
    {
        pub_handle_.reset(new ARF::PubHandle<std::string>(node_handle_.RegisterPublisher<std::string>("rgb")));
    }

    void Initialize()
    {
        InputPort<TaggedTrackedFacesAndImage> *input = Node::CreateInputPort<TaggedTrackedFacesAndImage>(5, INPUT_TRACKED_FACES_AND_IMAGE);

        InputPort<TaggedTrackedFacesAndImage>::EventCallback cb = [input, this](InputPort<TaggedTrackedFacesAndImage> *) {
            TaggedTrackedFacesAndImage tagged_tracked_faces_and_image;
            input->Read(tagged_tracked_faces_and_image, GetUUID());
            Task publish_faces_task = [tagged_tracked_faces_and_image, this]() {
                const auto &face_list = *tagged_tracked_faces_and_image.item.first;
                const auto &image = *tagged_tracked_faces_and_image.item.second;
                arf_proto::ArfMessage arf_message;
                arf_proto::TrackedFacesAndImage *tracked_faces_and_image = arf_message.mutable_tracked_faces_and_image();

                arf_proto::Image *arf_image = tracked_faces_and_image->mutable_image();
                arf_image->mutable_header()->set_time(image.GetTimestamp());
                arf_image->set_rows(image.GetNumRows());
                arf_image->set_cols(image.GetNumCols());
                arf_image->set_encoding("gray");
                arf_image->set_data(image.GetDataPointer(), image.GetNumRows() * image.GetNumCols());

                for (const auto &face : face_list)
                {
                    arf_proto::TrackedFace *face_proto = tracked_faces_and_image->add_face();
                    face_proto->mutable_header()->set_time(image.GetTimestamp());
                    face_proto->set_x(face.GetRect().GetX());
                    face_proto->set_y(face.GetRect().GetY());
                    face_proto->set_width(face.GetRect().GetWidth());
                    face_proto->set_height(face.GetRect().GetHeight());
                }

                pub_handle_->publish_string(arf_message.SerializeAsString());
            };

            Threadpool::Inst().EnqueueTask(publish_faces_task);
        };
        input->RegisterCallback(cb);
    }

  private:
    ARF::NodeHandle node_handle_;
    std::unique_ptr<ARF::PubHandle<std::string>> pub_handle_;
};

int main(int argc, char **argv)
{
    Logger::Inst().Initialize("/data/arf/face_tracker.log");
    ARF::Init(argc, argv, "vector");
    CameraNode camera_node("camera");
    ImageNode image_node("image");
    FaceTrackerNode face_tracker_node("face_tracker");
    FacePublishNode face_publish_node("face_publish");

    camera_node.Initialize();
    image_node.Initialize();
    face_tracker_node.Initialize();
    face_publish_node.Initialize();

    ARF::connect_ports(camera_node.RetrieveOutputPort<TaggedImageBuffer>(CameraNode::OUTPUT_CAMERA),
                       image_node.RetrieveInputPort<TaggedImageBuffer>(ImageNode::INPUT_BUFFER));
    ARF::connect_ports(image_node.RetrieveOutputPort<TaggedImage>(ImageNode::OUTPUT_IMAGE),
                       face_tracker_node.RetrieveInputPort<TaggedImage>(FaceTrackerNode::INPUT_IMAGE));
    ARF::connect_ports(face_tracker_node.RetrieveOutputPort<TaggedTrackedFacesAndImage>(FaceTrackerNode::OUTPUT_TRACKED_FACES_AND_IMAGE),
                       face_publish_node.RetrieveInputPort<TaggedTrackedFacesAndImage>(FacePublishNode::INPUT_TRACKED_FACES_AND_IMAGE));

    Threadpool::Inst().Initialize(5); // Num cores plus one extra potentially blocked in face tracker.
    Threadpool::Inst().StartAll();

    camera_node.Start();

    while (g_run)
    {
        usleep(100000);
    }
    Threadpool::Inst().HaltAndDeinitialize();
    Threadpool::Inst().Join();
    Logger::Inst().Shutdown();
    return 0;
}