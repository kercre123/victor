/*
 * radio.cpp
 *
 *   Implementation of HAL "radio" functionality for Cozmo V2
 *   This is actually the robot interface to the animation process
 *
 *   Author: Andrew Stein
 *
 */

#include "anki/cozmo/shared/cozmoConfig.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/logging.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "coretech/messaging/shared/LocalUdpServer.h"
#include "coretech/messaging/shared/socketConstants.h"
#include "libev/ev.h"

#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#define ARRAY_SIZE(inArray)   (sizeof(inArray) / sizeof((inArray)[0]))

namespace {
  // Temporary storage for data packets
  using Packet = std::vector<u8>;
  using PacketPtr = std::unique_ptr<Packet>;
  using PacketQueue = std::deque<PacketPtr>;
  using mutex = std::mutex;
  using lock_guard = std::lock_guard<std::mutex>;
  using thread = std::thread;

  // Incoming packets
  PacketQueue _radio_read_queue;
  mutex _radio_read_mutex;

  // Outgoing packets
  PacketQueue _radio_write_queue;
  mutex _radio_write_mutex;

  // Worker status flags
  std::atomic_bool _radio_stop(false);
  std::atomic_bool _radio_read_error(false);
  std::atomic_bool _radio_write_error(false);

  //
  // Worker event loop
  //
  // I/O processing is done outside of the main tick by using a libev event loop:
  //   https://linux.die.net/man/3/ev
  //
  // The main event loop runs on a worker thread.  It uses two io watchers
  // (_radio_read and _radio_write) to perform callbacks when the radio socket
  // is ready for read or write, plus an async watcher (_radio_break) to
  // interrupt the event loop on demand.
  //
  // The read watcher is always running so incoming data will be read ASAP.
  //
  // The write watcher is started when we have outgoing data queued for write,
  // then stopped when all outgoing data has been written.  This avoids the overhead
  // of invoking the handler when the socket is writable but we have no data queued.
  //
  // The async watcher is used to signal the worker thread when outgoing data has been
  // queued or when the thread has been signalled to stop. This breaks the event loop
  // and returns control to the top-level caller.
  //
  struct ev_loop * _radio_loop = nullptr;
  struct ev_io _radio_read;
  struct ev_io _radio_write;
  struct ev_async _radio_break;

  thread _radio_thread;

  // For communications with basestation
  LocalUdpServer server;

}

//
// Handle EV_READ event on radio socket by reading available data
// into temporary storage.  Incoming messages are enqueued for processing by
// HAL::RadioGetNextPacket() below.
//
static void RadioRead(struct ev_loop * loop, struct ev_io * w, int)
{
  static constexpr size_t RECV_BUFFER_SIZE = 1024 * 4;
  static u8 buf[RECV_BUFFER_SIZE];

  // Read incoming message
  ssize_t bytesRead = server.Recv((char *) buf, sizeof(buf));

  if (bytesRead < 0) {
    AnkiWarn("HAL.RadioRead.RecvError", "Recv error (errno %d)", errno);
    _radio_read_error = true;
    return;
  }

  if (bytesRead == 0) {
    // Nothing to read at this time - spurious wakeup?
    AnkiWarn("HAL.RadioRead.RecvZero", "Nothing to read");
    return;
  }

  // Allocate temporary packet
  PacketPtr packet = std::make_unique<Packet>(bytesRead);
  memcpy(packet->data(), buf, bytesRead);

  // Add incoming packet to incoming queue
  {
    lock_guard lock(_radio_read_mutex);
    _radio_read_queue.push_back(std::move(packet));
  }

}

//
// Handle EV_WRITE event on radio socket by writing outgoing packets
// until outgoing queue is empty.
//
static void RadioWrite(struct ev_loop * loop, struct ev_io * w, int)
{
  PacketPtr packet;
  {
    lock_guard lock(_radio_write_mutex);
    if (_radio_write_queue.empty()) {
      ev_io_stop(loop, w);
      return;
    }
    packet = std::move(_radio_write_queue.front());
    _radio_write_queue.pop_front();
  }

  // Write the packet we just popped
  const ssize_t bytesSent = server.Send((const char *) packet->data(), packet->size());
  if (bytesSent < packet->size()) {
    AnkiWarn("HAL.RadioWrite.SendError", "Failed to send packet (%zu/%zu)", bytesSent, packet->size());
    _radio_write_error = true;
  }
}

//
// Asynchronous handler used to interrupt the event loop
//
static void RadioBreak(struct ev_loop * loop, struct ev_async *, int)
{
  ev_break(loop);
}

//
// Event loop worker
//
static void RadioThread()
{
  ev_async_start(_radio_loop, &_radio_break);
  ev_io_start(_radio_loop, &_radio_read);

  while (!_radio_stop) {
    // Anything to write?
    {
      lock_guard lock(_radio_write_mutex);
      if (!_radio_write_queue.empty()) {
        ev_io_start(_radio_loop, &_radio_write);
      }
    }
    // Run until interrupted
    ev_run(_radio_loop);
  }

  ev_io_stop(_radio_loop, &_radio_read);
  ev_io_stop(_radio_loop, &_radio_write);
  ev_async_stop(_radio_loop, &_radio_break);

}

namespace Anki {
  namespace Vector {

    Result InitRadio()
    {
      const RobotID_t robotID = HAL::GetID();
      const std::string & server_path = std::string(ANIM_ROBOT_SERVER_PATH) + std::to_string(robotID);

      AnkiInfo("HAL.InitRadio.StartListening", "Start listening at %s", server_path.c_str());
      if (!server.StartListening(server_path.c_str())) {
        AnkiError("HAL.InitRadio.UDPServerFailed", "Unable to listen at %s", server_path.c_str());
        return RESULT_FAIL_IO;
      }

      // Initialize event loop and callbacks
      _radio_loop = ev_loop_new(EVBACKEND_SELECT);
      ev_io_init(&_radio_read, RadioRead, server.GetSocket(), EV_READ);
      ev_io_init(&_radio_write, RadioWrite, server.GetSocket(), EV_WRITE);
      ev_async_init(&_radio_break, RadioBreak);

      // Start the event loop
      _radio_thread = thread(RadioThread);

      return RESULT_OK;
    }

    void StopRadio()
    {

      // Stop the event loop
      if (_radio_thread.joinable()) {
        _radio_stop = true;
        ev_async_send(_radio_loop, &_radio_break);
        _radio_thread.join();
      }

      // Tear down event loop
      if (_radio_loop != nullptr) {
        ev_loop_destroy(_radio_loop);
        _radio_loop = nullptr;
      }

      // Tear down the socket
      server.StopListening();
    }

    bool HAL::RadioIsConnected(void)
    {
      return server.HasClient();
    }

    void HAL::DisconnectRadio(bool sendDisconnectMsg)
    {
      // TBD: How do we synchronize packet send with worker thread?
      if (sendDisconnectMsg && RadioIsConnected()) {
        RobotInterface::RobotServerDisconnect msg;
        RobotInterface::SendMessage(msg);
      }
      server.Disconnect();
    }

    bool HAL::RadioSendPacket(const void *buffer, const size_t length)
    {
      if (server.HasClient()) {
        // Copy buffer to temporary packet
        PacketPtr packet = std::make_unique<Packet>(length);
        memcpy(packet->data(), buffer, length);

        // Add packet to outgoing queue
        {
          lock_guard lock(_radio_write_mutex);
          _radio_write_queue.push_back(std::move(packet));
        }

        // Wake up the event loop
        ev_async_send(_radio_loop, &_radio_break);
      }
      return true;
    } // RadioSendPacket

    u32 HAL::RadioGetNextPacket(u8* buffer)
    {
      // Pop a packet from incoming queue
      PacketPtr packet;

      {
        lock_guard lock(_radio_read_mutex);
        if (_radio_read_queue.empty()) {
          // Nothing to pop
          return 0;
        }
        packet = std::move(_radio_read_queue.front());
        _radio_read_queue.pop_front();
      }

      memcpy(buffer, packet->data(), packet->size());
      return (u32) packet->size();

    } // RadioGetNextPacket()

    bool HAL::GetRadioReadError()
    {
      return _radio_read_error;
    }

    bool HAL::GetRadioWriteError()
    {
      return _radio_write_error;
    }

  } // namespace Vector
} // namespace Anki
