#include <stdio.h>
#include <termios.h>
#include <thread>
#include <unistd.h>

#include "arf/BulletinBoard.h"
#include "robot_to_arf_converter.h"

static struct termios old_term, new_term;

/* Initialize new terminal i/o settings */
void initTermios() {
  tcgetattr(0, &old_term);     /* grab old terminal i/o settings */
  new_term = old_term;         /* make new settings same as old settings */
  new_term.c_lflag &= ~ICANON; /* disable buffered i/o */
  new_term.c_lflag &= ~ECHO;   /* set no echo mode */
  tcsetattr(0, TCSANOW,
            &new_term); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void) { tcsetattr(0, TCSANOW, &old_term); }

char getch() {
  char ch;
  ch = getchar();
  return ch;
}

std::atomic<bool> g_run;

// Read messages at 20 Hz.
static constexpr int kReadMessagesUsec = 50000;
// Fastest consecutive keyboard press.
static constexpr int kKeyboardCheckUsec = 100000;

void intHandler(int /*dummy*/) { g_run = false; }

void InitVectorBoard(ARF::BulletinBoard *board,
                     ARF::TopicPoster<Anki::Vector::RobotState> *state_poster) {
  board->Declare<Anki::Vector::RobotState>("state");
  board->Declare<float>("head_angle");

  *state_poster = board->MakePoster<Anki::Vector::RobotState>("state");
  ARF::TopicPoster<float> head_angle_poster =
      board->MakePoster<float>("head_angle");

  board->RegisterCallback<Anki::Vector::RobotState>(
      "state",
      [head_angle_poster](const Anki::Vector::RobotState &state) mutable {
        std::cout << "Pose frame id " << state.pose_frame_id << " origin "
                  << state.pose_origin_id << " pose " << state.pose.x << " "
                  << state.pose.y << " " << state.pose.z << std::endl;
        head_angle_poster.Post(state.headAngle);
      });
}

int main(int argc, char **argv) {
  g_run = true;
  initTermios();
  ARF::RobotToArfConverter converter;

  bool connected = converter.Connect();

  if (!connected) {
    std::cout << "Did not connect." << std::endl;
    return -1;
  }
  std::cout << "Successfully connected." << std::endl;

  ARF::BulletinBoard board;
  ARF::TopicPoster<Anki::Vector::RobotState> state_poster;
  InitVectorBoard(&board, &state_poster);

  converter.SendSync();

  std::thread keyboard_input_thread([&converter, &board]() mutable {
    ARF::TopicViewer<float> head_viewer = board.MakeViewer<float>("head_angle");
    while (g_run) {
      bool commanding_robot_speed = false;
      uint8_t action_id = 0;
      char g = getch();
      fflush(stdin);
      if (!head_viewer.IsInitialized())
        continue;
      switch (g) {
      case 'y': {
        Anki::Vector::RobotInterface::SetHeadAngle head_command;
        head_command.actionID = action_id;
        ++action_id;
        head_viewer.Retrieve(head_command.angle_rad);
        head_command.angle_rad += .05;
        std::cout << "Sending command up " << head_command.angle_rad
                  << std::endl;
        head_command.duration_sec = kKeyboardCheckUsec * 1e-6;
        converter.SendHeadCommand(head_command);
        break;
      }
      case 'h': {
        Anki::Vector::RobotInterface::SetHeadAngle head_command;
        head_command.actionID = action_id;
        ++action_id;
        head_viewer.Retrieve(head_command.angle_rad);
        head_command.angle_rad -= .05;
        std::cout << "Sending command down " << head_command.angle_rad
                  << std::endl;
        head_command.duration_sec = kKeyboardCheckUsec * 1e-6;
        converter.SendHeadCommand(head_command);
        break;
      }
      case 'i': {
        commanding_robot_speed = true;
        Anki::Vector::RobotInterface::DriveWheelsCurvature curvature_command;
        curvature_command.speed = 25.0;
        curvature_command.accel = 0;
        curvature_command.curvatureRadius_mm =
            std::numeric_limits<int16_t>::max();
        converter.SendMessage(
            Anki::Vector::RobotInterface::EngineToRobot(curvature_command));
        break;
      }
      case 'k': {
        commanding_robot_speed = true;
        Anki::Vector::RobotInterface::DriveWheelsCurvature curvature_command;
        curvature_command.speed = -25.0;
        curvature_command.accel = 0;
        curvature_command.curvatureRadius_mm =
            std::numeric_limits<int16_t>::max();
        converter.SendMessage(
            Anki::Vector::RobotInterface::EngineToRobot(curvature_command));
        break;
      }
      case 'o': {
        Anki::Vector::RobotInterface::StopAllMotors stop_command;
        converter.SendMessage(
            Anki::Vector::RobotInterface::EngineToRobot(stop_command));
      }
      }
      usleep(kKeyboardCheckUsec);
    }
  });

  Anki::Vector::RobotState state;
  while (g_run) {
    {
      if (converter.ReadMessages(&state)) {
        state_poster.Post(state);
      };
    }
    usleep(kReadMessagesUsec);
  }

  keyboard_input_thread.join();
  converter.Disconnect();
  resetTermios();
}
