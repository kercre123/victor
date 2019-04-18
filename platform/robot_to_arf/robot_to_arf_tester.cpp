#include <mutex>
#include <thread>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>

#include "robot_to_arf_converter.h"

static struct termios old_term, new_term;

/* Initialize new terminal i/o settings */
void initTermios() 
{
  tcgetattr(0, &old_term); /* grab old terminal i/o settings */
  new_term = old_term; /* make new settings same as old settings */
  new_term.c_lflag &= ~ICANON; /* disable buffered i/o */
  new_term.c_lflag &= ~ECHO; /* set no echo mode */
  tcsetattr(0, TCSANOW, &new_term); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void) 
{
  tcsetattr(0, TCSANOW, &old_term);
}

char getch() 
{
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
  converter.SendSync();

  std::mutex state_mutex;
  bool got_recent_state = false;
  Anki::Vector::RobotState state;

  std::thread keyboard_input_thread([&converter, &state_mutex, &got_recent_state, &state]() {
    while (g_run) {
      uint8_t action_id = 0;
      char g = getch();
      if (!got_recent_state)
        continue;
      switch (g) {
      case 'y': {
        std::lock_guard<std::mutex> lock(state_mutex);
        Anki::Vector::RobotInterface::SetHeadAngle head_command;
        head_command.actionID = action_id;
        ++action_id;
        head_command.angle_rad = state.headAngle + .05;
        std::cout << "Sending command up " << head_command.angle_rad << std::endl;
        head_command.duration_sec = kKeyboardCheckUsec * 1e-6;
        converter.SendHeadCommand(head_command);
        break;
      }
      case 'h': {
        std::lock_guard<std::mutex> lock(state_mutex);
        Anki::Vector::RobotInterface::SetHeadAngle head_command;
        head_command.actionID = action_id;
        ++action_id;
        head_command.angle_rad = state.headAngle - .05;
        std::cout << "Sending command down " << head_command.angle_rad << std::endl;
        head_command.duration_sec = kKeyboardCheckUsec * 1e-6;
        converter.SendHeadCommand(head_command);
        break;
      }
      }
      std::cout << "Current head angle: " << state.headAngle << std::endl;
      usleep(kKeyboardCheckUsec);
    }
  });

  while (g_run) {
    {
      std::lock_guard<std::mutex> lock(state_mutex);
      got_recent_state = converter.ReadMessages(&state);
    }
    usleep(kReadMessagesUsec);
  }

  keyboard_input_thread.join();
  converter.Disconnect();
  resetTermios();
}
