




static const char kStateTopicName[] = "state";
static const char kPoseTopicName[] = "pose";
static const char kPoseOriginsTopicName[] = "pose_origins";
using RobotStateTopicType = Anki::Vector::RobotState;
using PoseTopicType = RobotPoseHistory;
using PoseOriginsTopicType = Anki::PoseOriginList;

void InitVectorBoard(ARF::BulletinBoard *board,
                     ARF::TopicPoster<RobotStateTopicType> *state_poster) {
  board->Declare<RobotStateTopicType>(kStateTopicName);
  board->Declare<PoseTopicType>(kPoseTopicName);
  board->Declare<PoseOriginsTopicType>(kPoseOriginsTopicName);

  ARF::TopicPoster<PoseTopicType> raw_pose_poster =
      board->MakePoster<PoseTopicType>(kPoseTopicName);
  ARF::TopicPoster<PoseOriginsTopicType> pose_origins_viewer =
      board->MakeViewer<PoseOriginsTopicType>(kPoseOriginsTopicName);

  board->RegisterCallback<RobotStateTopicType>(
      "state",
      [raw_pose_poster, pose_origins_viewer](const RobotStateTopicType &state) mutable {
          bool have_pose_origin;
          Pose3d origin;


    raw_pose_poster.InvokePost(
        [&state](PoseTopicType &history) { history.AddRawPose(state.) });

}
