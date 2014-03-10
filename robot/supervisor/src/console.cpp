#include "console.h"
#include "anki/cozmo/robot/hal.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace Console
    {
      static const u32 BUFFER_SIZE = 128;
      static char m_buffer[BUFFER_SIZE];
      static s32 m_index = 0;
      static u32 m_argumentCount = 0;

      typedef int (*ConsoleFunction)();
      static void ParseCommand();

      static char* GetArgument(u8 index)
      {
        if (index >= m_argumentCount)
        {
          return NULL;
        }

        u8 currentIndex = 0;
        char* c = m_buffer;
        while (currentIndex != index)
        {
          if (*c == 0)
          {
            currentIndex++;
          }
          c++;
        }

        return c;
      }

      static int ParseInt(const char* str)
      {
        int mul = 1;
        int val = 0;
        while (*str)
        {
          if (*str == '-')
          {
            mul = -1;
          } else if (*str >= '0' && *str <= '9') {
            val = (val * 10) + (*str - '0');
          }

          str++;
        }

        return val * mul;
      }

      void Init()
      {
      }

      void Update()
      {
        int value = HAL::USBGetChar();
        if (value < 0)  // No work to do if there's no data
          return;

        u8 c = value;  // Using u8 should eliminate sign-extension

        // Echo legal ASCII back to the console
        if (c >= 32 && c <= 126)
        {
          printf("%c", c);
          // Prevent overflow
          if (m_index + 1 >= BUFFER_SIZE)
          {
            m_index = 0;
          }

          m_buffer[m_index++] = c;
        } else if (c == 27) {  // Check for escape
          m_index = 0;
        } else if (c == 0x7f || c == 8) {  // Check for delete
          printf("%c", c);
          
          m_index--;
          if (m_index < 0)
          {
            m_index = 0;
          }
          m_buffer[m_index] = 0;
        } else if ('\r' || c == '\n') {
          printf("\n");
          m_buffer[m_index] = 0;
          m_index = 0;
          ParseCommand();
        }
      }

      static int GetFrontImage()
      {
        return 0;
      }

      static int GetMatImage()
      {
        return 0;
      }

      static int ShakeHead()
      {
        u32 up, down, pause, count;
        float power;
        char* arg = GetArgument(1);
        if (!arg)
          return 1;

        up = ParseInt(arg);

        arg = GetArgument(2);
        if (!arg)
          return 1;

        down = ParseInt(arg);

        arg = GetArgument(3);
        if (!arg)
          return 1;

        pause = ParseInt(arg);

        arg = GetArgument(4);
        if (!arg)
          return 1;

        count = ParseInt(arg);

        arg = GetArgument(5);
        if (!arg)
          return 1;

        power = ParseInt(arg) / 100.0f;

        for (int i = 0; i < count; i++)
        {
          HAL::MotorSetPower(HAL::MOTOR_HEAD, power);
          HAL::MicroWait(up * 1000);
          HAL::MotorSetPower(HAL::MOTOR_HEAD, 0);
          HAL::MicroWait(pause * 1000);
          HAL::MotorSetPower(HAL::MOTOR_HEAD, -power);
          HAL::MicroWait(down * 1000);
          HAL::MotorSetPower(HAL::MOTOR_HEAD, 0);
          HAL::MicroWait(pause * 1000);
        }

        return 0;
      }

      static int SetMotors()
      {
        float value;
        char* arg = GetArgument(1);
        if (!arg)
          return 1;

        value = ParseInt(arg) / 100.0f;

        HAL::MotorSetPower(HAL::MOTOR_LEFT_WHEEL, value);

        arg = GetArgument(2);
        if (!arg)
          return 1;

        value = ParseInt(arg) / 100.0f;
        HAL::MotorSetPower(HAL::MOTOR_RIGHT_WHEEL, value);

        arg = GetArgument(3);
        if (!arg)
          return 1;

        value = ParseInt(arg) / 100.0f;
        HAL::MotorSetPower(HAL::MOTOR_LIFT, value);

        arg = GetArgument(4);
        if (!arg)
          return 1;

        value = ParseInt(arg) / 100.0f;
        HAL::MotorSetPower(HAL::MOTOR_HEAD, value);

        // Get the number of milliseconds to wait
        arg = GetArgument(6);
        if (!arg)
          return 1;

        HAL::MicroWait(ParseInt(arg) * 1000);

        HAL::MotorSetPower(HAL::MOTOR_LEFT_WHEEL, 0);
        HAL::MotorSetPower(HAL::MOTOR_RIGHT_WHEEL, 0);
        HAL::MotorSetPower(HAL::MOTOR_LIFT, 0);
        HAL::MotorSetPower(HAL::MOTOR_HEAD, 0);

        return 0;
      }

      struct ConsoleCommand
      {
        const char* command;
        ConsoleFunction function;
      };

      static const ConsoleCommand m_commands[] =
      {
        {"GetFrontImage", GetFrontImage},
        {"GetMatImage", GetMatImage},
        {"SetMotors", SetMotors},
        {"ShakeHead", ShakeHead},
      };

      static void ParseCommand()
      {
        const int functionCount = sizeof(m_commands) / sizeof(ConsoleCommand);
        char* buffer = m_buffer;
        if (!strcmp(buffer, "?"))
        {
          printf("\nCommands:\n");
          for (int i = 0; i < functionCount; i++)
          {
            printf("%s\n", m_commands[i].command);
          }
          printf("\n");
        } else {
          // Tokenize by spaces
          m_argumentCount = 1;
          char* b = buffer;
          while (*b)
          {
            if (*b == ' ')
            {
              *b = 0;
              m_argumentCount++;
            }
            b++;
          }

          bool foundCommand = false;
          for (int i = 0; i < functionCount; i++)
          {
            const ConsoleCommand* cmd = &m_commands[i];
            if (!strcasecmp(cmd->command, buffer) && cmd->function)
            {
              foundCommand = true;

              int ret = cmd->function();
              printf("status,%i\n", ret);
              break;
            }
          }

          if (!foundCommand && strcmp(buffer, ""))
          {
            printf("Unknown command: %s\n", buffer);
          }
        }
      }
    }
  }
}

