/**
 * File: TcpSocketComms
 *
 * Author: Mark Wesley
 * Created: 05/14/16
 *
 * Description: TCP implementation for socket-based communications from e.g. Game/SDK to Engine
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Cozmo_Game_Comms_TcpSocketComms_H__
#define __Cozmo_Game_Comms_TcpSocketComms_H__


#include "engine/cozmoAPI/comms/iSocketComms.h"


class TcpServer;


namespace Anki {
namespace Cozmo {


class TcpSocketComms : public ISocketComms
{
public:
  
  explicit TcpSocketComms(bool isEnabled, uint16_t port = 0);
  virtual ~TcpSocketComms();
  
  virtual bool Init(UiConnectionType connectionType, const Json::Value& config) override;

  virtual bool AreMessagesGrouped() const override { return false; }
  
  virtual bool ConnectToDeviceByID(DeviceId deviceId) override;
  virtual bool DisconnectDeviceByID(DeviceId deviceId) override;
  virtual bool DisconnectAllDevices() override;
  
  virtual void GetAdvertisingDeviceIDs(std::vector<ISocketComms::DeviceId>& outDeviceIds) override;
  
  virtual uint32_t GetNumConnectedDevices() const override;
  
protected:
  virtual void UpdateInternal() override;

private:
  
  // ============================== Private Member Functions ==============================

  virtual bool SendMessageInternal(const Comms::MsgPacket& msgPacket) override;
  virtual bool RecvMessageInternal(std::vector<uint8_t>& outBuffer) override;

  virtual void OnEnableConnection(bool wasEnabled, bool isEnabled) override;
  
  void  HandleDisconnect();
  bool  IsConnected() const;
  
  bool  ReadFromSocket();
  bool  ExtractNextMessage(std::vector<uint8_t>& outBuffer);
  
  // ============================== Private Member Vars ==============================
  
  TcpServer*            _tcpServer;
  
  // TCP doesn't send/recv as single packets, so it's possible to receive just part of a message
  // so we hang on to the partial message bytes until we have enough to deliver
  std::vector<uint8_t>  _receivedBuffer;
  
  DeviceId              _connectedId;
  uint16_t              _port;
  bool                  _hasClient;
};

  
    
} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Game_Comms_TcpSocketComms_H__

