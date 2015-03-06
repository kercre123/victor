#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include "SafeMessageBuffer.h"

namespace Anki
{
namespace Cozmo
{
using SafeMessageBuffer = Anki::Util::SafeMessageBuffer;
struct U2G_Ping
{
	uint32_t counter;

	/**** Constructors ****/
	U2G_Ping() = default;
	U2G_Ping(const U2G_Ping& other) = default;
	U2G_Ping(U2G_Ping& other) = default;
	U2G_Ping(U2G_Ping&& other) noexcept = default;
	U2G_Ping& operator=(const U2G_Ping& other) = default;
	U2G_Ping& operator=(U2G_Ping&& other) noexcept = default;

	explicit U2G_Ping(uint32_t counter)
	:counter(counter)
	{}
	explicit U2G_Ping(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_Ping(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->counter);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->counter);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//counter
		result += 4; // = uint_32
		return result;
	}

	bool operator==(const U2G_Ping& other) const
	{
		if (counter != other.counter) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_Ping& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_ConnectToRobot
{
	uint8_t robotID;

	/**** Constructors ****/
	U2G_ConnectToRobot() = default;
	U2G_ConnectToRobot(const U2G_ConnectToRobot& other) = default;
	U2G_ConnectToRobot(U2G_ConnectToRobot& other) = default;
	U2G_ConnectToRobot(U2G_ConnectToRobot&& other) noexcept = default;
	U2G_ConnectToRobot& operator=(const U2G_ConnectToRobot& other) = default;
	U2G_ConnectToRobot& operator=(U2G_ConnectToRobot&& other) noexcept = default;

	explicit U2G_ConnectToRobot(uint8_t robotID)
	:robotID(robotID)
	{}
	explicit U2G_ConnectToRobot(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_ConnectToRobot(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->robotID);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->robotID);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//robotID
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const U2G_ConnectToRobot& other) const
	{
		if (robotID != other.robotID) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_ConnectToRobot& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_ConnectToUiDevice
{
	uint8_t deviceID;

	/**** Constructors ****/
	U2G_ConnectToUiDevice() = default;
	U2G_ConnectToUiDevice(const U2G_ConnectToUiDevice& other) = default;
	U2G_ConnectToUiDevice(U2G_ConnectToUiDevice& other) = default;
	U2G_ConnectToUiDevice(U2G_ConnectToUiDevice&& other) noexcept = default;
	U2G_ConnectToUiDevice& operator=(const U2G_ConnectToUiDevice& other) = default;
	U2G_ConnectToUiDevice& operator=(U2G_ConnectToUiDevice&& other) noexcept = default;

	explicit U2G_ConnectToUiDevice(uint8_t deviceID)
	:deviceID(deviceID)
	{}
	explicit U2G_ConnectToUiDevice(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_ConnectToUiDevice(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->deviceID);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->deviceID);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//deviceID
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const U2G_ConnectToUiDevice& other) const
	{
		if (deviceID != other.deviceID) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_ConnectToUiDevice& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_DisconnectFromUiDevice
{
	uint8_t deviceID;

	/**** Constructors ****/
	U2G_DisconnectFromUiDevice() = default;
	U2G_DisconnectFromUiDevice(const U2G_DisconnectFromUiDevice& other) = default;
	U2G_DisconnectFromUiDevice(U2G_DisconnectFromUiDevice& other) = default;
	U2G_DisconnectFromUiDevice(U2G_DisconnectFromUiDevice&& other) noexcept = default;
	U2G_DisconnectFromUiDevice& operator=(const U2G_DisconnectFromUiDevice& other) = default;
	U2G_DisconnectFromUiDevice& operator=(U2G_DisconnectFromUiDevice&& other) noexcept = default;

	explicit U2G_DisconnectFromUiDevice(uint8_t deviceID)
	:deviceID(deviceID)
	{}
	explicit U2G_DisconnectFromUiDevice(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_DisconnectFromUiDevice(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->deviceID);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->deviceID);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//deviceID
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const U2G_DisconnectFromUiDevice& other) const
	{
		if (deviceID != other.deviceID) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_DisconnectFromUiDevice& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_ForceAddRobot
{
	std::array<uint8_t, 16> ipAddress;
	uint8_t robotID;
	uint8_t isSimulated;

	/**** Constructors ****/
	U2G_ForceAddRobot() = default;
	U2G_ForceAddRobot(const U2G_ForceAddRobot& other) = default;
	U2G_ForceAddRobot(U2G_ForceAddRobot& other) = default;
	U2G_ForceAddRobot(U2G_ForceAddRobot&& other) noexcept = default;
	U2G_ForceAddRobot& operator=(const U2G_ForceAddRobot& other) = default;
	U2G_ForceAddRobot& operator=(U2G_ForceAddRobot&& other) noexcept = default;

	explicit U2G_ForceAddRobot(const std::array<uint8_t, 16>& ipAddress
		,uint8_t robotID
		,uint8_t isSimulated)
	:ipAddress(ipAddress)
	,robotID(robotID)
	,isSimulated(isSimulated)
	{}
	explicit U2G_ForceAddRobot(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_ForceAddRobot(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.WriteFArray<uint8_t, 16>(this->ipAddress);
		buffer.Write(this->robotID);
		buffer.Write(this->isSimulated);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.ReadFArray<uint8_t, 16>(this->ipAddress);
		buffer.Read(this->robotID);
		buffer.Read(this->isSimulated);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//ipAddress
		result += 1 * 16; // = uint_8 * 16
		//robotID
		result += 1; // = uint_8
		//isSimulated
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const U2G_ForceAddRobot& other) const
	{
		if (ipAddress != other.ipAddress
		|| robotID != other.robotID
		|| isSimulated != other.isSimulated) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_ForceAddRobot& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_StartEngine
{
	uint8_t asHost;
	std::array<uint8_t, 16> vizHostIP;

	/**** Constructors ****/
	U2G_StartEngine() = default;
	U2G_StartEngine(const U2G_StartEngine& other) = default;
	U2G_StartEngine(U2G_StartEngine& other) = default;
	U2G_StartEngine(U2G_StartEngine&& other) noexcept = default;
	U2G_StartEngine& operator=(const U2G_StartEngine& other) = default;
	U2G_StartEngine& operator=(U2G_StartEngine&& other) noexcept = default;

	explicit U2G_StartEngine(uint8_t asHost
		,const std::array<uint8_t, 16>& vizHostIP)
	:asHost(asHost)
	,vizHostIP(vizHostIP)
	{}
	explicit U2G_StartEngine(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_StartEngine(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->asHost);
		buffer.WriteFArray<uint8_t, 16>(this->vizHostIP);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->asHost);
		buffer.ReadFArray<uint8_t, 16>(this->vizHostIP);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//asHost
		result += 1; // = uint_8
		//vizHostIP
		result += 1 * 16; // = uint_8 * 16
		return result;
	}

	bool operator==(const U2G_StartEngine& other) const
	{
		if (asHost != other.asHost
		|| vizHostIP != other.vizHostIP) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_StartEngine& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_DriveWheels
{
	float lwheel_speed_mmps;
	float rwheel_speed_mmps;

	/**** Constructors ****/
	U2G_DriveWheels() = default;
	U2G_DriveWheels(const U2G_DriveWheels& other) = default;
	U2G_DriveWheels(U2G_DriveWheels& other) = default;
	U2G_DriveWheels(U2G_DriveWheels&& other) noexcept = default;
	U2G_DriveWheels& operator=(const U2G_DriveWheels& other) = default;
	U2G_DriveWheels& operator=(U2G_DriveWheels&& other) noexcept = default;

	explicit U2G_DriveWheels(float lwheel_speed_mmps
		,float rwheel_speed_mmps)
	:lwheel_speed_mmps(lwheel_speed_mmps)
	,rwheel_speed_mmps(rwheel_speed_mmps)
	{}
	explicit U2G_DriveWheels(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_DriveWheels(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->lwheel_speed_mmps);
		buffer.Write(this->rwheel_speed_mmps);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->lwheel_speed_mmps);
		buffer.Read(this->rwheel_speed_mmps);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//lwheel_speed_mmps
		result += 4; // = float_32
		//rwheel_speed_mmps
		result += 4; // = float_32
		return result;
	}

	bool operator==(const U2G_DriveWheels& other) const
	{
		if (lwheel_speed_mmps != other.lwheel_speed_mmps
		|| rwheel_speed_mmps != other.rwheel_speed_mmps) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_DriveWheels& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_TurnInPlace
{
	float angle_rad;
	uint8_t robotID;

	/**** Constructors ****/
	U2G_TurnInPlace() = default;
	U2G_TurnInPlace(const U2G_TurnInPlace& other) = default;
	U2G_TurnInPlace(U2G_TurnInPlace& other) = default;
	U2G_TurnInPlace(U2G_TurnInPlace&& other) noexcept = default;
	U2G_TurnInPlace& operator=(const U2G_TurnInPlace& other) = default;
	U2G_TurnInPlace& operator=(U2G_TurnInPlace&& other) noexcept = default;

	explicit U2G_TurnInPlace(float angle_rad
		,uint8_t robotID)
	:angle_rad(angle_rad)
	,robotID(robotID)
	{}
	explicit U2G_TurnInPlace(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_TurnInPlace(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->angle_rad);
		buffer.Write(this->robotID);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->angle_rad);
		buffer.Read(this->robotID);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//angle_rad
		result += 4; // = float_32
		//robotID
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const U2G_TurnInPlace& other) const
	{
		if (angle_rad != other.angle_rad
		|| robotID != other.robotID) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_TurnInPlace& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_MoveHead
{
	float speed_rad_per_sec;

	/**** Constructors ****/
	U2G_MoveHead() = default;
	U2G_MoveHead(const U2G_MoveHead& other) = default;
	U2G_MoveHead(U2G_MoveHead& other) = default;
	U2G_MoveHead(U2G_MoveHead&& other) noexcept = default;
	U2G_MoveHead& operator=(const U2G_MoveHead& other) = default;
	U2G_MoveHead& operator=(U2G_MoveHead&& other) noexcept = default;

	explicit U2G_MoveHead(float speed_rad_per_sec)
	:speed_rad_per_sec(speed_rad_per_sec)
	{}
	explicit U2G_MoveHead(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_MoveHead(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->speed_rad_per_sec);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->speed_rad_per_sec);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//speed_rad_per_sec
		result += 4; // = float_32
		return result;
	}

	bool operator==(const U2G_MoveHead& other) const
	{
		if (speed_rad_per_sec != other.speed_rad_per_sec) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_MoveHead& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_MoveLift
{
	float speed_rad_per_sec;

	/**** Constructors ****/
	U2G_MoveLift() = default;
	U2G_MoveLift(const U2G_MoveLift& other) = default;
	U2G_MoveLift(U2G_MoveLift& other) = default;
	U2G_MoveLift(U2G_MoveLift&& other) noexcept = default;
	U2G_MoveLift& operator=(const U2G_MoveLift& other) = default;
	U2G_MoveLift& operator=(U2G_MoveLift&& other) noexcept = default;

	explicit U2G_MoveLift(float speed_rad_per_sec)
	:speed_rad_per_sec(speed_rad_per_sec)
	{}
	explicit U2G_MoveLift(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_MoveLift(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->speed_rad_per_sec);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->speed_rad_per_sec);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//speed_rad_per_sec
		result += 4; // = float_32
		return result;
	}

	bool operator==(const U2G_MoveLift& other) const
	{
		if (speed_rad_per_sec != other.speed_rad_per_sec) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_MoveLift& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_SetLiftHeight
{
	float height_mm;
	float max_speed_rad_per_sec;
	float accel_rad_per_sec2;

	/**** Constructors ****/
	U2G_SetLiftHeight() = default;
	U2G_SetLiftHeight(const U2G_SetLiftHeight& other) = default;
	U2G_SetLiftHeight(U2G_SetLiftHeight& other) = default;
	U2G_SetLiftHeight(U2G_SetLiftHeight&& other) noexcept = default;
	U2G_SetLiftHeight& operator=(const U2G_SetLiftHeight& other) = default;
	U2G_SetLiftHeight& operator=(U2G_SetLiftHeight&& other) noexcept = default;

	explicit U2G_SetLiftHeight(float height_mm
		,float max_speed_rad_per_sec
		,float accel_rad_per_sec2)
	:height_mm(height_mm)
	,max_speed_rad_per_sec(max_speed_rad_per_sec)
	,accel_rad_per_sec2(accel_rad_per_sec2)
	{}
	explicit U2G_SetLiftHeight(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_SetLiftHeight(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->height_mm);
		buffer.Write(this->max_speed_rad_per_sec);
		buffer.Write(this->accel_rad_per_sec2);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->height_mm);
		buffer.Read(this->max_speed_rad_per_sec);
		buffer.Read(this->accel_rad_per_sec2);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//height_mm
		result += 4; // = float_32
		//max_speed_rad_per_sec
		result += 4; // = float_32
		//accel_rad_per_sec2
		result += 4; // = float_32
		return result;
	}

	bool operator==(const U2G_SetLiftHeight& other) const
	{
		if (height_mm != other.height_mm
		|| max_speed_rad_per_sec != other.max_speed_rad_per_sec
		|| accel_rad_per_sec2 != other.accel_rad_per_sec2) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_SetLiftHeight& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_SetHeadAngle
{
	float angle_rad;
	float max_speed_rad_per_sec;
	float accel_rad_per_sec2;

	/**** Constructors ****/
	U2G_SetHeadAngle() = default;
	U2G_SetHeadAngle(const U2G_SetHeadAngle& other) = default;
	U2G_SetHeadAngle(U2G_SetHeadAngle& other) = default;
	U2G_SetHeadAngle(U2G_SetHeadAngle&& other) noexcept = default;
	U2G_SetHeadAngle& operator=(const U2G_SetHeadAngle& other) = default;
	U2G_SetHeadAngle& operator=(U2G_SetHeadAngle&& other) noexcept = default;

	explicit U2G_SetHeadAngle(float angle_rad
		,float max_speed_rad_per_sec
		,float accel_rad_per_sec2)
	:angle_rad(angle_rad)
	,max_speed_rad_per_sec(max_speed_rad_per_sec)
	,accel_rad_per_sec2(accel_rad_per_sec2)
	{}
	explicit U2G_SetHeadAngle(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_SetHeadAngle(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->angle_rad);
		buffer.Write(this->max_speed_rad_per_sec);
		buffer.Write(this->accel_rad_per_sec2);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->angle_rad);
		buffer.Read(this->max_speed_rad_per_sec);
		buffer.Read(this->accel_rad_per_sec2);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//angle_rad
		result += 4; // = float_32
		//max_speed_rad_per_sec
		result += 4; // = float_32
		//accel_rad_per_sec2
		result += 4; // = float_32
		return result;
	}

	bool operator==(const U2G_SetHeadAngle& other) const
	{
		if (angle_rad != other.angle_rad
		|| max_speed_rad_per_sec != other.max_speed_rad_per_sec
		|| accel_rad_per_sec2 != other.accel_rad_per_sec2) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_SetHeadAngle& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_StopAllMotors
{

	/**** Constructors ****/
	U2G_StopAllMotors() = default;
	U2G_StopAllMotors(const U2G_StopAllMotors& other) = default;
	U2G_StopAllMotors(U2G_StopAllMotors& other) = default;
	U2G_StopAllMotors(U2G_StopAllMotors&& other) noexcept = default;
	U2G_StopAllMotors& operator=(const U2G_StopAllMotors& other) = default;
	U2G_StopAllMotors& operator=(U2G_StopAllMotors&& other) noexcept = default;

	explicit U2G_StopAllMotors(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_StopAllMotors(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		return result;
	}

	bool operator==(const U2G_StopAllMotors& other) const
	{
		return true;
	}

	bool operator!=(const U2G_StopAllMotors& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_ImageRequest
{
	uint8_t robotID;
	uint8_t mode;

	/**** Constructors ****/
	U2G_ImageRequest() = default;
	U2G_ImageRequest(const U2G_ImageRequest& other) = default;
	U2G_ImageRequest(U2G_ImageRequest& other) = default;
	U2G_ImageRequest(U2G_ImageRequest&& other) noexcept = default;
	U2G_ImageRequest& operator=(const U2G_ImageRequest& other) = default;
	U2G_ImageRequest& operator=(U2G_ImageRequest&& other) noexcept = default;

	explicit U2G_ImageRequest(uint8_t robotID
		,uint8_t mode)
	:robotID(robotID)
	,mode(mode)
	{}
	explicit U2G_ImageRequest(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_ImageRequest(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->robotID);
		buffer.Write(this->mode);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->robotID);
		buffer.Read(this->mode);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//robotID
		result += 1; // = uint_8
		//mode
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const U2G_ImageRequest& other) const
	{
		if (robotID != other.robotID
		|| mode != other.mode) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_ImageRequest& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_SetRobotImageSendMode
{
	uint8_t mode;
	uint8_t resolution;

	/**** Constructors ****/
	U2G_SetRobotImageSendMode() = default;
	U2G_SetRobotImageSendMode(const U2G_SetRobotImageSendMode& other) = default;
	U2G_SetRobotImageSendMode(U2G_SetRobotImageSendMode& other) = default;
	U2G_SetRobotImageSendMode(U2G_SetRobotImageSendMode&& other) noexcept = default;
	U2G_SetRobotImageSendMode& operator=(const U2G_SetRobotImageSendMode& other) = default;
	U2G_SetRobotImageSendMode& operator=(U2G_SetRobotImageSendMode&& other) noexcept = default;

	explicit U2G_SetRobotImageSendMode(uint8_t mode
		,uint8_t resolution)
	:mode(mode)
	,resolution(resolution)
	{}
	explicit U2G_SetRobotImageSendMode(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_SetRobotImageSendMode(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->mode);
		buffer.Write(this->resolution);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->mode);
		buffer.Read(this->resolution);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//mode
		result += 1; // = uint_8
		//resolution
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const U2G_SetRobotImageSendMode& other) const
	{
		if (mode != other.mode
		|| resolution != other.resolution) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_SetRobotImageSendMode& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_SaveImages
{
	uint8_t mode;

	/**** Constructors ****/
	U2G_SaveImages() = default;
	U2G_SaveImages(const U2G_SaveImages& other) = default;
	U2G_SaveImages(U2G_SaveImages& other) = default;
	U2G_SaveImages(U2G_SaveImages&& other) noexcept = default;
	U2G_SaveImages& operator=(const U2G_SaveImages& other) = default;
	U2G_SaveImages& operator=(U2G_SaveImages&& other) noexcept = default;

	explicit U2G_SaveImages(uint8_t mode)
	:mode(mode)
	{}
	explicit U2G_SaveImages(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_SaveImages(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->mode);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->mode);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//mode
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const U2G_SaveImages& other) const
	{
		if (mode != other.mode) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_SaveImages& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_EnableDisplay
{
	uint8_t enable;

	/**** Constructors ****/
	U2G_EnableDisplay() = default;
	U2G_EnableDisplay(const U2G_EnableDisplay& other) = default;
	U2G_EnableDisplay(U2G_EnableDisplay& other) = default;
	U2G_EnableDisplay(U2G_EnableDisplay&& other) noexcept = default;
	U2G_EnableDisplay& operator=(const U2G_EnableDisplay& other) = default;
	U2G_EnableDisplay& operator=(U2G_EnableDisplay&& other) noexcept = default;

	explicit U2G_EnableDisplay(uint8_t enable)
	:enable(enable)
	{}
	explicit U2G_EnableDisplay(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_EnableDisplay(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->enable);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->enable);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//enable
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const U2G_EnableDisplay& other) const
	{
		if (enable != other.enable) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_EnableDisplay& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_SetHeadlights
{
	uint8_t intensity;

	/**** Constructors ****/
	U2G_SetHeadlights() = default;
	U2G_SetHeadlights(const U2G_SetHeadlights& other) = default;
	U2G_SetHeadlights(U2G_SetHeadlights& other) = default;
	U2G_SetHeadlights(U2G_SetHeadlights&& other) noexcept = default;
	U2G_SetHeadlights& operator=(const U2G_SetHeadlights& other) = default;
	U2G_SetHeadlights& operator=(U2G_SetHeadlights&& other) noexcept = default;

	explicit U2G_SetHeadlights(uint8_t intensity)
	:intensity(intensity)
	{}
	explicit U2G_SetHeadlights(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_SetHeadlights(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->intensity);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->intensity);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//intensity
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const U2G_SetHeadlights& other) const
	{
		if (intensity != other.intensity) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_SetHeadlights& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_GotoPose
{
	float x_mm;
	float y_mm;
	float rad;
	uint8_t level;
	uint8_t useManualSpeed;

	/**** Constructors ****/
	U2G_GotoPose() = default;
	U2G_GotoPose(const U2G_GotoPose& other) = default;
	U2G_GotoPose(U2G_GotoPose& other) = default;
	U2G_GotoPose(U2G_GotoPose&& other) noexcept = default;
	U2G_GotoPose& operator=(const U2G_GotoPose& other) = default;
	U2G_GotoPose& operator=(U2G_GotoPose&& other) noexcept = default;

	explicit U2G_GotoPose(float x_mm
		,float y_mm
		,float rad
		,uint8_t level
		,uint8_t useManualSpeed)
	:x_mm(x_mm)
	,y_mm(y_mm)
	,rad(rad)
	,level(level)
	,useManualSpeed(useManualSpeed)
	{}
	explicit U2G_GotoPose(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_GotoPose(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->x_mm);
		buffer.Write(this->y_mm);
		buffer.Write(this->rad);
		buffer.Write(this->level);
		buffer.Write(this->useManualSpeed);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->x_mm);
		buffer.Read(this->y_mm);
		buffer.Read(this->rad);
		buffer.Read(this->level);
		buffer.Read(this->useManualSpeed);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//x_mm
		result += 4; // = float_32
		//y_mm
		result += 4; // = float_32
		//rad
		result += 4; // = float_32
		//level
		result += 1; // = uint_8
		//useManualSpeed
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const U2G_GotoPose& other) const
	{
		if (x_mm != other.x_mm
		|| y_mm != other.y_mm
		|| rad != other.rad
		|| level != other.level
		|| useManualSpeed != other.useManualSpeed) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_GotoPose& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_PlaceObjectOnGround
{
	float x_mm;
	float y_mm;
	float rad;
	uint8_t level;
	uint8_t useManualSpeed;

	/**** Constructors ****/
	U2G_PlaceObjectOnGround() = default;
	U2G_PlaceObjectOnGround(const U2G_PlaceObjectOnGround& other) = default;
	U2G_PlaceObjectOnGround(U2G_PlaceObjectOnGround& other) = default;
	U2G_PlaceObjectOnGround(U2G_PlaceObjectOnGround&& other) noexcept = default;
	U2G_PlaceObjectOnGround& operator=(const U2G_PlaceObjectOnGround& other) = default;
	U2G_PlaceObjectOnGround& operator=(U2G_PlaceObjectOnGround&& other) noexcept = default;

	explicit U2G_PlaceObjectOnGround(float x_mm
		,float y_mm
		,float rad
		,uint8_t level
		,uint8_t useManualSpeed)
	:x_mm(x_mm)
	,y_mm(y_mm)
	,rad(rad)
	,level(level)
	,useManualSpeed(useManualSpeed)
	{}
	explicit U2G_PlaceObjectOnGround(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_PlaceObjectOnGround(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->x_mm);
		buffer.Write(this->y_mm);
		buffer.Write(this->rad);
		buffer.Write(this->level);
		buffer.Write(this->useManualSpeed);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->x_mm);
		buffer.Read(this->y_mm);
		buffer.Read(this->rad);
		buffer.Read(this->level);
		buffer.Read(this->useManualSpeed);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//x_mm
		result += 4; // = float_32
		//y_mm
		result += 4; // = float_32
		//rad
		result += 4; // = float_32
		//level
		result += 1; // = uint_8
		//useManualSpeed
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const U2G_PlaceObjectOnGround& other) const
	{
		if (x_mm != other.x_mm
		|| y_mm != other.y_mm
		|| rad != other.rad
		|| level != other.level
		|| useManualSpeed != other.useManualSpeed) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_PlaceObjectOnGround& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_PlaceObjectOnGroundHere
{

	/**** Constructors ****/
	U2G_PlaceObjectOnGroundHere() = default;
	U2G_PlaceObjectOnGroundHere(const U2G_PlaceObjectOnGroundHere& other) = default;
	U2G_PlaceObjectOnGroundHere(U2G_PlaceObjectOnGroundHere& other) = default;
	U2G_PlaceObjectOnGroundHere(U2G_PlaceObjectOnGroundHere&& other) noexcept = default;
	U2G_PlaceObjectOnGroundHere& operator=(const U2G_PlaceObjectOnGroundHere& other) = default;
	U2G_PlaceObjectOnGroundHere& operator=(U2G_PlaceObjectOnGroundHere&& other) noexcept = default;

	explicit U2G_PlaceObjectOnGroundHere(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_PlaceObjectOnGroundHere(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		return result;
	}

	bool operator==(const U2G_PlaceObjectOnGroundHere& other) const
	{
		return true;
	}

	bool operator!=(const U2G_PlaceObjectOnGroundHere& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_ExecuteTestPlan
{

	/**** Constructors ****/
	U2G_ExecuteTestPlan() = default;
	U2G_ExecuteTestPlan(const U2G_ExecuteTestPlan& other) = default;
	U2G_ExecuteTestPlan(U2G_ExecuteTestPlan& other) = default;
	U2G_ExecuteTestPlan(U2G_ExecuteTestPlan&& other) noexcept = default;
	U2G_ExecuteTestPlan& operator=(const U2G_ExecuteTestPlan& other) = default;
	U2G_ExecuteTestPlan& operator=(U2G_ExecuteTestPlan&& other) noexcept = default;

	explicit U2G_ExecuteTestPlan(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_ExecuteTestPlan(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		return result;
	}

	bool operator==(const U2G_ExecuteTestPlan& other) const
	{
		return true;
	}

	bool operator!=(const U2G_ExecuteTestPlan& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_SelectNextObject
{

	/**** Constructors ****/
	U2G_SelectNextObject() = default;
	U2G_SelectNextObject(const U2G_SelectNextObject& other) = default;
	U2G_SelectNextObject(U2G_SelectNextObject& other) = default;
	U2G_SelectNextObject(U2G_SelectNextObject&& other) noexcept = default;
	U2G_SelectNextObject& operator=(const U2G_SelectNextObject& other) = default;
	U2G_SelectNextObject& operator=(U2G_SelectNextObject&& other) noexcept = default;

	explicit U2G_SelectNextObject(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_SelectNextObject(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		return result;
	}

	bool operator==(const U2G_SelectNextObject& other) const
	{
		return true;
	}

	bool operator!=(const U2G_SelectNextObject& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_PickAndPlaceObject
{
	int32_t objectID;
	uint8_t usePreDockPose;
	uint8_t useManualSpeed;

	/**** Constructors ****/
	U2G_PickAndPlaceObject() = default;
	U2G_PickAndPlaceObject(const U2G_PickAndPlaceObject& other) = default;
	U2G_PickAndPlaceObject(U2G_PickAndPlaceObject& other) = default;
	U2G_PickAndPlaceObject(U2G_PickAndPlaceObject&& other) noexcept = default;
	U2G_PickAndPlaceObject& operator=(const U2G_PickAndPlaceObject& other) = default;
	U2G_PickAndPlaceObject& operator=(U2G_PickAndPlaceObject&& other) noexcept = default;

	explicit U2G_PickAndPlaceObject(int32_t objectID
		,uint8_t usePreDockPose
		,uint8_t useManualSpeed)
	:objectID(objectID)
	,usePreDockPose(usePreDockPose)
	,useManualSpeed(useManualSpeed)
	{}
	explicit U2G_PickAndPlaceObject(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_PickAndPlaceObject(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->objectID);
		buffer.Write(this->usePreDockPose);
		buffer.Write(this->useManualSpeed);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->objectID);
		buffer.Read(this->usePreDockPose);
		buffer.Read(this->useManualSpeed);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//objectID
		result += 4; // = int_32
		//usePreDockPose
		result += 1; // = uint_8
		//useManualSpeed
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const U2G_PickAndPlaceObject& other) const
	{
		if (objectID != other.objectID
		|| usePreDockPose != other.usePreDockPose
		|| useManualSpeed != other.useManualSpeed) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_PickAndPlaceObject& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_TraverseObject
{
	uint8_t usePreDockPose;
	uint8_t useManualSpeed;

	/**** Constructors ****/
	U2G_TraverseObject() = default;
	U2G_TraverseObject(const U2G_TraverseObject& other) = default;
	U2G_TraverseObject(U2G_TraverseObject& other) = default;
	U2G_TraverseObject(U2G_TraverseObject&& other) noexcept = default;
	U2G_TraverseObject& operator=(const U2G_TraverseObject& other) = default;
	U2G_TraverseObject& operator=(U2G_TraverseObject&& other) noexcept = default;

	explicit U2G_TraverseObject(uint8_t usePreDockPose
		,uint8_t useManualSpeed)
	:usePreDockPose(usePreDockPose)
	,useManualSpeed(useManualSpeed)
	{}
	explicit U2G_TraverseObject(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_TraverseObject(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->usePreDockPose);
		buffer.Write(this->useManualSpeed);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->usePreDockPose);
		buffer.Read(this->useManualSpeed);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//usePreDockPose
		result += 1; // = uint_8
		//useManualSpeed
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const U2G_TraverseObject& other) const
	{
		if (usePreDockPose != other.usePreDockPose
		|| useManualSpeed != other.useManualSpeed) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_TraverseObject& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_ClearAllBlocks
{

	/**** Constructors ****/
	U2G_ClearAllBlocks() = default;
	U2G_ClearAllBlocks(const U2G_ClearAllBlocks& other) = default;
	U2G_ClearAllBlocks(U2G_ClearAllBlocks& other) = default;
	U2G_ClearAllBlocks(U2G_ClearAllBlocks&& other) noexcept = default;
	U2G_ClearAllBlocks& operator=(const U2G_ClearAllBlocks& other) = default;
	U2G_ClearAllBlocks& operator=(U2G_ClearAllBlocks&& other) noexcept = default;

	explicit U2G_ClearAllBlocks(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_ClearAllBlocks(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		return result;
	}

	bool operator==(const U2G_ClearAllBlocks& other) const
	{
		return true;
	}

	bool operator!=(const U2G_ClearAllBlocks& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_ExecuteBehavior
{
	uint8_t behaviorMode;

	/**** Constructors ****/
	U2G_ExecuteBehavior() = default;
	U2G_ExecuteBehavior(const U2G_ExecuteBehavior& other) = default;
	U2G_ExecuteBehavior(U2G_ExecuteBehavior& other) = default;
	U2G_ExecuteBehavior(U2G_ExecuteBehavior&& other) noexcept = default;
	U2G_ExecuteBehavior& operator=(const U2G_ExecuteBehavior& other) = default;
	U2G_ExecuteBehavior& operator=(U2G_ExecuteBehavior&& other) noexcept = default;

	explicit U2G_ExecuteBehavior(uint8_t behaviorMode)
	:behaviorMode(behaviorMode)
	{}
	explicit U2G_ExecuteBehavior(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_ExecuteBehavior(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->behaviorMode);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->behaviorMode);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//behaviorMode
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const U2G_ExecuteBehavior& other) const
	{
		if (behaviorMode != other.behaviorMode) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_ExecuteBehavior& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_SetBehaviorState
{
	uint8_t behaviorState;

	/**** Constructors ****/
	U2G_SetBehaviorState() = default;
	U2G_SetBehaviorState(const U2G_SetBehaviorState& other) = default;
	U2G_SetBehaviorState(U2G_SetBehaviorState& other) = default;
	U2G_SetBehaviorState(U2G_SetBehaviorState&& other) noexcept = default;
	U2G_SetBehaviorState& operator=(const U2G_SetBehaviorState& other) = default;
	U2G_SetBehaviorState& operator=(U2G_SetBehaviorState&& other) noexcept = default;

	explicit U2G_SetBehaviorState(uint8_t behaviorState)
	:behaviorState(behaviorState)
	{}
	explicit U2G_SetBehaviorState(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_SetBehaviorState(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->behaviorState);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->behaviorState);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//behaviorState
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const U2G_SetBehaviorState& other) const
	{
		if (behaviorState != other.behaviorState) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_SetBehaviorState& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_AbortPath
{

	/**** Constructors ****/
	U2G_AbortPath() = default;
	U2G_AbortPath(const U2G_AbortPath& other) = default;
	U2G_AbortPath(U2G_AbortPath& other) = default;
	U2G_AbortPath(U2G_AbortPath&& other) noexcept = default;
	U2G_AbortPath& operator=(const U2G_AbortPath& other) = default;
	U2G_AbortPath& operator=(U2G_AbortPath&& other) noexcept = default;

	explicit U2G_AbortPath(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_AbortPath(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		return result;
	}

	bool operator==(const U2G_AbortPath& other) const
	{
		return true;
	}

	bool operator!=(const U2G_AbortPath& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_AbortAll
{

	/**** Constructors ****/
	U2G_AbortAll() = default;
	U2G_AbortAll(const U2G_AbortAll& other) = default;
	U2G_AbortAll(U2G_AbortAll& other) = default;
	U2G_AbortAll(U2G_AbortAll&& other) noexcept = default;
	U2G_AbortAll& operator=(const U2G_AbortAll& other) = default;
	U2G_AbortAll& operator=(U2G_AbortAll&& other) noexcept = default;

	explicit U2G_AbortAll(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_AbortAll(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		return result;
	}

	bool operator==(const U2G_AbortAll& other) const
	{
		return true;
	}

	bool operator!=(const U2G_AbortAll& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_DrawPoseMarker
{
	float x_mm;
	float y_mm;
	float rad;
	uint8_t level;

	/**** Constructors ****/
	U2G_DrawPoseMarker() = default;
	U2G_DrawPoseMarker(const U2G_DrawPoseMarker& other) = default;
	U2G_DrawPoseMarker(U2G_DrawPoseMarker& other) = default;
	U2G_DrawPoseMarker(U2G_DrawPoseMarker&& other) noexcept = default;
	U2G_DrawPoseMarker& operator=(const U2G_DrawPoseMarker& other) = default;
	U2G_DrawPoseMarker& operator=(U2G_DrawPoseMarker&& other) noexcept = default;

	explicit U2G_DrawPoseMarker(float x_mm
		,float y_mm
		,float rad
		,uint8_t level)
	:x_mm(x_mm)
	,y_mm(y_mm)
	,rad(rad)
	,level(level)
	{}
	explicit U2G_DrawPoseMarker(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_DrawPoseMarker(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->x_mm);
		buffer.Write(this->y_mm);
		buffer.Write(this->rad);
		buffer.Write(this->level);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->x_mm);
		buffer.Read(this->y_mm);
		buffer.Read(this->rad);
		buffer.Read(this->level);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//x_mm
		result += 4; // = float_32
		//y_mm
		result += 4; // = float_32
		//rad
		result += 4; // = float_32
		//level
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const U2G_DrawPoseMarker& other) const
	{
		if (x_mm != other.x_mm
		|| y_mm != other.y_mm
		|| rad != other.rad
		|| level != other.level) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_DrawPoseMarker& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_ErasePoseMarker
{

	/**** Constructors ****/
	U2G_ErasePoseMarker() = default;
	U2G_ErasePoseMarker(const U2G_ErasePoseMarker& other) = default;
	U2G_ErasePoseMarker(U2G_ErasePoseMarker& other) = default;
	U2G_ErasePoseMarker(U2G_ErasePoseMarker&& other) noexcept = default;
	U2G_ErasePoseMarker& operator=(const U2G_ErasePoseMarker& other) = default;
	U2G_ErasePoseMarker& operator=(U2G_ErasePoseMarker&& other) noexcept = default;

	explicit U2G_ErasePoseMarker(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_ErasePoseMarker(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		return result;
	}

	bool operator==(const U2G_ErasePoseMarker& other) const
	{
		return true;
	}

	bool operator!=(const U2G_ErasePoseMarker& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_SetHeadControllerGains
{
	float kp;
	float ki;
	float maxIntegralError;

	/**** Constructors ****/
	U2G_SetHeadControllerGains() = default;
	U2G_SetHeadControllerGains(const U2G_SetHeadControllerGains& other) = default;
	U2G_SetHeadControllerGains(U2G_SetHeadControllerGains& other) = default;
	U2G_SetHeadControllerGains(U2G_SetHeadControllerGains&& other) noexcept = default;
	U2G_SetHeadControllerGains& operator=(const U2G_SetHeadControllerGains& other) = default;
	U2G_SetHeadControllerGains& operator=(U2G_SetHeadControllerGains&& other) noexcept = default;

	explicit U2G_SetHeadControllerGains(float kp
		,float ki
		,float maxIntegralError)
	:kp(kp)
	,ki(ki)
	,maxIntegralError(maxIntegralError)
	{}
	explicit U2G_SetHeadControllerGains(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_SetHeadControllerGains(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->kp);
		buffer.Write(this->ki);
		buffer.Write(this->maxIntegralError);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->kp);
		buffer.Read(this->ki);
		buffer.Read(this->maxIntegralError);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//kp
		result += 4; // = float_32
		//ki
		result += 4; // = float_32
		//maxIntegralError
		result += 4; // = float_32
		return result;
	}

	bool operator==(const U2G_SetHeadControllerGains& other) const
	{
		if (kp != other.kp
		|| ki != other.ki
		|| maxIntegralError != other.maxIntegralError) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_SetHeadControllerGains& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_SetLiftControllerGains
{
	float kp;
	float ki;
	float maxIntegralError;

	/**** Constructors ****/
	U2G_SetLiftControllerGains() = default;
	U2G_SetLiftControllerGains(const U2G_SetLiftControllerGains& other) = default;
	U2G_SetLiftControllerGains(U2G_SetLiftControllerGains& other) = default;
	U2G_SetLiftControllerGains(U2G_SetLiftControllerGains&& other) noexcept = default;
	U2G_SetLiftControllerGains& operator=(const U2G_SetLiftControllerGains& other) = default;
	U2G_SetLiftControllerGains& operator=(U2G_SetLiftControllerGains&& other) noexcept = default;

	explicit U2G_SetLiftControllerGains(float kp
		,float ki
		,float maxIntegralError)
	:kp(kp)
	,ki(ki)
	,maxIntegralError(maxIntegralError)
	{}
	explicit U2G_SetLiftControllerGains(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_SetLiftControllerGains(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->kp);
		buffer.Write(this->ki);
		buffer.Write(this->maxIntegralError);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->kp);
		buffer.Read(this->ki);
		buffer.Read(this->maxIntegralError);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//kp
		result += 4; // = float_32
		//ki
		result += 4; // = float_32
		//maxIntegralError
		result += 4; // = float_32
		return result;
	}

	bool operator==(const U2G_SetLiftControllerGains& other) const
	{
		if (kp != other.kp
		|| ki != other.ki
		|| maxIntegralError != other.maxIntegralError) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_SetLiftControllerGains& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_SelectNextSoundScheme
{

	/**** Constructors ****/
	U2G_SelectNextSoundScheme() = default;
	U2G_SelectNextSoundScheme(const U2G_SelectNextSoundScheme& other) = default;
	U2G_SelectNextSoundScheme(U2G_SelectNextSoundScheme& other) = default;
	U2G_SelectNextSoundScheme(U2G_SelectNextSoundScheme&& other) noexcept = default;
	U2G_SelectNextSoundScheme& operator=(const U2G_SelectNextSoundScheme& other) = default;
	U2G_SelectNextSoundScheme& operator=(U2G_SelectNextSoundScheme&& other) noexcept = default;

	explicit U2G_SelectNextSoundScheme(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_SelectNextSoundScheme(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		return result;
	}

	bool operator==(const U2G_SelectNextSoundScheme& other) const
	{
		return true;
	}

	bool operator!=(const U2G_SelectNextSoundScheme& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_StartTestMode
{
	int32_t p1;
	int32_t p2;
	int32_t p3;
	uint8_t mode;

	/**** Constructors ****/
	U2G_StartTestMode() = default;
	U2G_StartTestMode(const U2G_StartTestMode& other) = default;
	U2G_StartTestMode(U2G_StartTestMode& other) = default;
	U2G_StartTestMode(U2G_StartTestMode&& other) noexcept = default;
	U2G_StartTestMode& operator=(const U2G_StartTestMode& other) = default;
	U2G_StartTestMode& operator=(U2G_StartTestMode&& other) noexcept = default;

	explicit U2G_StartTestMode(int32_t p1
		,int32_t p2
		,int32_t p3
		,uint8_t mode)
	:p1(p1)
	,p2(p2)
	,p3(p3)
	,mode(mode)
	{}
	explicit U2G_StartTestMode(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_StartTestMode(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->p1);
		buffer.Write(this->p2);
		buffer.Write(this->p3);
		buffer.Write(this->mode);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->p1);
		buffer.Read(this->p2);
		buffer.Read(this->p3);
		buffer.Read(this->mode);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//p1
		result += 4; // = int_32
		//p2
		result += 4; // = int_32
		//p3
		result += 4; // = int_32
		//mode
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const U2G_StartTestMode& other) const
	{
		if (p1 != other.p1
		|| p2 != other.p2
		|| p3 != other.p3
		|| mode != other.mode) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_StartTestMode& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_IMURequest
{
	uint32_t length_ms;

	/**** Constructors ****/
	U2G_IMURequest() = default;
	U2G_IMURequest(const U2G_IMURequest& other) = default;
	U2G_IMURequest(U2G_IMURequest& other) = default;
	U2G_IMURequest(U2G_IMURequest&& other) noexcept = default;
	U2G_IMURequest& operator=(const U2G_IMURequest& other) = default;
	U2G_IMURequest& operator=(U2G_IMURequest&& other) noexcept = default;

	explicit U2G_IMURequest(uint32_t length_ms)
	:length_ms(length_ms)
	{}
	explicit U2G_IMURequest(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_IMURequest(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->length_ms);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->length_ms);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//length_ms
		result += 4; // = uint_32
		return result;
	}

	bool operator==(const U2G_IMURequest& other) const
	{
		if (length_ms != other.length_ms) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_IMURequest& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_PlayAnimation
{
	uint32_t numLoops;
	std::string animationName;

	/**** Constructors ****/
	U2G_PlayAnimation() = default;
	U2G_PlayAnimation(const U2G_PlayAnimation& other) = default;
	U2G_PlayAnimation(U2G_PlayAnimation& other) = default;
	U2G_PlayAnimation(U2G_PlayAnimation&& other) noexcept = default;
	U2G_PlayAnimation& operator=(const U2G_PlayAnimation& other) = default;
	U2G_PlayAnimation& operator=(U2G_PlayAnimation&& other) noexcept = default;

	explicit U2G_PlayAnimation(uint32_t numLoops
		,const std::string& animationName)
	:numLoops(numLoops)
	,animationName(animationName)
	{}
	explicit U2G_PlayAnimation(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_PlayAnimation(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->numLoops);
		buffer.WritePString<uint8_t>(this->animationName);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->numLoops);
		buffer.ReadPString<uint8_t>(this->animationName);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//numLoops
		result += 4; // = uint_32
		//animationName
		result += 1; // length = uint_8
		result += 1 * animationName.size(); //string
		return result;
	}

	bool operator==(const U2G_PlayAnimation& other) const
	{
		if (numLoops != other.numLoops
		|| animationName != other.animationName) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_PlayAnimation& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_ReadAnimationFile
{

	/**** Constructors ****/
	U2G_ReadAnimationFile() = default;
	U2G_ReadAnimationFile(const U2G_ReadAnimationFile& other) = default;
	U2G_ReadAnimationFile(U2G_ReadAnimationFile& other) = default;
	U2G_ReadAnimationFile(U2G_ReadAnimationFile&& other) noexcept = default;
	U2G_ReadAnimationFile& operator=(const U2G_ReadAnimationFile& other) = default;
	U2G_ReadAnimationFile& operator=(U2G_ReadAnimationFile&& other) noexcept = default;

	explicit U2G_ReadAnimationFile(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_ReadAnimationFile(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		return result;
	}

	bool operator==(const U2G_ReadAnimationFile& other) const
	{
		return true;
	}

	bool operator!=(const U2G_ReadAnimationFile& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_StartFaceTracking
{
	uint8_t timeout_sec;

	/**** Constructors ****/
	U2G_StartFaceTracking() = default;
	U2G_StartFaceTracking(const U2G_StartFaceTracking& other) = default;
	U2G_StartFaceTracking(U2G_StartFaceTracking& other) = default;
	U2G_StartFaceTracking(U2G_StartFaceTracking&& other) noexcept = default;
	U2G_StartFaceTracking& operator=(const U2G_StartFaceTracking& other) = default;
	U2G_StartFaceTracking& operator=(U2G_StartFaceTracking&& other) noexcept = default;

	explicit U2G_StartFaceTracking(uint8_t timeout_sec)
	:timeout_sec(timeout_sec)
	{}
	explicit U2G_StartFaceTracking(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_StartFaceTracking(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->timeout_sec);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->timeout_sec);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//timeout_sec
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const U2G_StartFaceTracking& other) const
	{
		if (timeout_sec != other.timeout_sec) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_StartFaceTracking& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_StopFaceTracking
{

	/**** Constructors ****/
	U2G_StopFaceTracking() = default;
	U2G_StopFaceTracking(const U2G_StopFaceTracking& other) = default;
	U2G_StopFaceTracking(U2G_StopFaceTracking& other) = default;
	U2G_StopFaceTracking(U2G_StopFaceTracking&& other) noexcept = default;
	U2G_StopFaceTracking& operator=(const U2G_StopFaceTracking& other) = default;
	U2G_StopFaceTracking& operator=(U2G_StopFaceTracking&& other) noexcept = default;

	explicit U2G_StopFaceTracking(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_StopFaceTracking(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		return result;
	}

	bool operator==(const U2G_StopFaceTracking& other) const
	{
		return true;
	}

	bool operator!=(const U2G_StopFaceTracking& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_StartLookingForMarkers
{

	/**** Constructors ****/
	U2G_StartLookingForMarkers() = default;
	U2G_StartLookingForMarkers(const U2G_StartLookingForMarkers& other) = default;
	U2G_StartLookingForMarkers(U2G_StartLookingForMarkers& other) = default;
	U2G_StartLookingForMarkers(U2G_StartLookingForMarkers&& other) noexcept = default;
	U2G_StartLookingForMarkers& operator=(const U2G_StartLookingForMarkers& other) = default;
	U2G_StartLookingForMarkers& operator=(U2G_StartLookingForMarkers&& other) noexcept = default;

	explicit U2G_StartLookingForMarkers(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_StartLookingForMarkers(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		return result;
	}

	bool operator==(const U2G_StartLookingForMarkers& other) const
	{
		return true;
	}

	bool operator!=(const U2G_StartLookingForMarkers& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_StopLookingForMarkers
{

	/**** Constructors ****/
	U2G_StopLookingForMarkers() = default;
	U2G_StopLookingForMarkers(const U2G_StopLookingForMarkers& other) = default;
	U2G_StopLookingForMarkers(U2G_StopLookingForMarkers& other) = default;
	U2G_StopLookingForMarkers(U2G_StopLookingForMarkers&& other) noexcept = default;
	U2G_StopLookingForMarkers& operator=(const U2G_StopLookingForMarkers& other) = default;
	U2G_StopLookingForMarkers& operator=(U2G_StopLookingForMarkers&& other) noexcept = default;

	explicit U2G_StopLookingForMarkers(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_StopLookingForMarkers(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		return result;
	}

	bool operator==(const U2G_StopLookingForMarkers& other) const
	{
		return true;
	}

	bool operator!=(const U2G_StopLookingForMarkers& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_SetVisionSystemParams
{
	int32_t autoexposureOn;
	float exposureTime;
	int32_t integerCountsIncrement;
	float minExposureTime;
	float maxExposureTime;
	float percentileToMakeHigh;
	int32_t limitFramerate;
	uint8_t highValue;

	/**** Constructors ****/
	U2G_SetVisionSystemParams() = default;
	U2G_SetVisionSystemParams(const U2G_SetVisionSystemParams& other) = default;
	U2G_SetVisionSystemParams(U2G_SetVisionSystemParams& other) = default;
	U2G_SetVisionSystemParams(U2G_SetVisionSystemParams&& other) noexcept = default;
	U2G_SetVisionSystemParams& operator=(const U2G_SetVisionSystemParams& other) = default;
	U2G_SetVisionSystemParams& operator=(U2G_SetVisionSystemParams&& other) noexcept = default;

	explicit U2G_SetVisionSystemParams(int32_t autoexposureOn
		,float exposureTime
		,int32_t integerCountsIncrement
		,float minExposureTime
		,float maxExposureTime
		,float percentileToMakeHigh
		,int32_t limitFramerate
		,uint8_t highValue)
	:autoexposureOn(autoexposureOn)
	,exposureTime(exposureTime)
	,integerCountsIncrement(integerCountsIncrement)
	,minExposureTime(minExposureTime)
	,maxExposureTime(maxExposureTime)
	,percentileToMakeHigh(percentileToMakeHigh)
	,limitFramerate(limitFramerate)
	,highValue(highValue)
	{}
	explicit U2G_SetVisionSystemParams(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_SetVisionSystemParams(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->autoexposureOn);
		buffer.Write(this->exposureTime);
		buffer.Write(this->integerCountsIncrement);
		buffer.Write(this->minExposureTime);
		buffer.Write(this->maxExposureTime);
		buffer.Write(this->percentileToMakeHigh);
		buffer.Write(this->limitFramerate);
		buffer.Write(this->highValue);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->autoexposureOn);
		buffer.Read(this->exposureTime);
		buffer.Read(this->integerCountsIncrement);
		buffer.Read(this->minExposureTime);
		buffer.Read(this->maxExposureTime);
		buffer.Read(this->percentileToMakeHigh);
		buffer.Read(this->limitFramerate);
		buffer.Read(this->highValue);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//autoexposureOn
		result += 4; // = int_32
		//exposureTime
		result += 4; // = float_32
		//integerCountsIncrement
		result += 4; // = int_32
		//minExposureTime
		result += 4; // = float_32
		//maxExposureTime
		result += 4; // = float_32
		//percentileToMakeHigh
		result += 4; // = float_32
		//limitFramerate
		result += 4; // = int_32
		//highValue
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const U2G_SetVisionSystemParams& other) const
	{
		if (autoexposureOn != other.autoexposureOn
		|| exposureTime != other.exposureTime
		|| integerCountsIncrement != other.integerCountsIncrement
		|| minExposureTime != other.minExposureTime
		|| maxExposureTime != other.maxExposureTime
		|| percentileToMakeHigh != other.percentileToMakeHigh
		|| limitFramerate != other.limitFramerate
		|| highValue != other.highValue) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_SetVisionSystemParams& other) const
	{
		return !(operator==(other));
	}
};

struct U2G_SetFaceDetectParams
{
	float scaleFactor;
	int32_t minNeighbors;
	int32_t minObjectHeight;
	int32_t minObjectWidth;
	int32_t maxObjectHeight;
	int32_t maxObjectWidth;

	/**** Constructors ****/
	U2G_SetFaceDetectParams() = default;
	U2G_SetFaceDetectParams(const U2G_SetFaceDetectParams& other) = default;
	U2G_SetFaceDetectParams(U2G_SetFaceDetectParams& other) = default;
	U2G_SetFaceDetectParams(U2G_SetFaceDetectParams&& other) noexcept = default;
	U2G_SetFaceDetectParams& operator=(const U2G_SetFaceDetectParams& other) = default;
	U2G_SetFaceDetectParams& operator=(U2G_SetFaceDetectParams&& other) noexcept = default;

	explicit U2G_SetFaceDetectParams(float scaleFactor
		,int32_t minNeighbors
		,int32_t minObjectHeight
		,int32_t minObjectWidth
		,int32_t maxObjectHeight
		,int32_t maxObjectWidth)
	:scaleFactor(scaleFactor)
	,minNeighbors(minNeighbors)
	,minObjectHeight(minObjectHeight)
	,minObjectWidth(minObjectWidth)
	,maxObjectHeight(maxObjectHeight)
	,maxObjectWidth(maxObjectWidth)
	{}
	explicit U2G_SetFaceDetectParams(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit U2G_SetFaceDetectParams(const SafeMessageBuffer& buffer)
	{
		Unpack(buffer);
	}

	/**** Pack ****/
	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(this->scaleFactor);
		buffer.Write(this->minNeighbors);
		buffer.Write(this->minObjectHeight);
		buffer.Write(this->minObjectWidth);
		buffer.Write(this->maxObjectHeight);
		buffer.Write(this->maxObjectWidth);
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}

	/**** Unpack ****/
	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}

	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		buffer.Read(this->scaleFactor);
		buffer.Read(this->minNeighbors);
		buffer.Read(this->minObjectHeight);
		buffer.Read(this->minObjectWidth);
		buffer.Read(this->maxObjectHeight);
		buffer.Read(this->maxObjectWidth);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//scaleFactor
		result += 4; // = float_32
		//minNeighbors
		result += 4; // = int_32
		//minObjectHeight
		result += 4; // = int_32
		//minObjectWidth
		result += 4; // = int_32
		//maxObjectHeight
		result += 4; // = int_32
		//maxObjectWidth
		result += 4; // = int_32
		return result;
	}

	bool operator==(const U2G_SetFaceDetectParams& other) const
	{
		if (scaleFactor != other.scaleFactor
		|| minNeighbors != other.minNeighbors
		|| minObjectHeight != other.minObjectHeight
		|| minObjectWidth != other.minObjectWidth
		|| maxObjectHeight != other.maxObjectHeight
		|| maxObjectWidth != other.maxObjectWidth) {
			return false;
		}
		return true;
	}

	bool operator!=(const U2G_SetFaceDetectParams& other) const
	{
		return !(operator==(other));
	}
};

class U2G_Message
{
public:
	/**** Constructors ****/
	U2G_Message() :_type(Type::INVALID) { }
	explicit U2G_Message(const SafeMessageBuffer& buff) :_type(Type::INVALID)
	{
		Unpack(buff);
	}
	explicit U2G_Message(const uint8_t* buffer, size_t length) :_type(Type::INVALID)
	{
		SafeMessageBuffer buff(const_cast<uint8_t*>(buffer), length);
		Unpack(buff);
	}

	~U2G_Message() { ClearCurrent(); }
	enum class Type : uint8_t {
		Ping,	// 0
		ConnectToRobot,	// 1
		ConnectToUiDevice,	// 2
		DisconnectFromUiDevice,	// 3
		ForceAddRobot,	// 4
		StartEngine,	// 5
		DriveWheels,	// 6
		TurnInPlace,	// 7
		MoveHead,	// 8
		MoveLift,	// 9
		SetLiftHeight,	// 10
		SetHeadAngle,	// 11
		StopAllMotors,	// 12
		ImageRequest,	// 13
		SetRobotImageSendMode,	// 14
		SaveImages,	// 15
		EnableDisplay,	// 16
		SetHeadlights,	// 17
		GotoPose,	// 18
		PlaceObjectOnGround,	// 19
		PlaceObjectOnGroundHere,	// 20
		ExecuteTestPlan,	// 21
		SelectNextObject,	// 22
		PickAndPlaceObject,	// 23
		TraverseObject,	// 24
		ClearAllBlocks,	// 25
		ExecuteBehavior,	// 26
		SetBehaviorState,	// 27
		AbortPath,	// 28
		AbortAll,	// 29
		DrawPoseMarker,	// 30
		ErasePoseMarker,	// 31
		SetHeadControllerGains,	// 32
		SetLiftControllerGains,	// 33
		SelectNextSoundScheme,	// 34
		StartTestMode,	// 35
		IMURequest,	// 36
		PlayAnimation,	// 37
		ReadAnimationFile,	// 38
		StartFaceTracking,	// 39
		StopFaceTracking,	// 40
		StartLookingForMarkers,	// 41
		StopLookingForMarkers,	// 42
		SetVisionSystemParams,	// 43
		SetFaceDetectParams,	// 44
		INVALID
	};
	static const char* GetTypeName(Type type) {
		switch (type) {
		case Type::Ping:
			return "Ping";
		case Type::ConnectToRobot:
			return "ConnectToRobot";
		case Type::ConnectToUiDevice:
			return "ConnectToUiDevice";
		case Type::DisconnectFromUiDevice:
			return "DisconnectFromUiDevice";
		case Type::ForceAddRobot:
			return "ForceAddRobot";
		case Type::StartEngine:
			return "StartEngine";
		case Type::DriveWheels:
			return "DriveWheels";
		case Type::TurnInPlace:
			return "TurnInPlace";
		case Type::MoveHead:
			return "MoveHead";
		case Type::MoveLift:
			return "MoveLift";
		case Type::SetLiftHeight:
			return "SetLiftHeight";
		case Type::SetHeadAngle:
			return "SetHeadAngle";
		case Type::StopAllMotors:
			return "StopAllMotors";
		case Type::ImageRequest:
			return "ImageRequest";
		case Type::SetRobotImageSendMode:
			return "SetRobotImageSendMode";
		case Type::SaveImages:
			return "SaveImages";
		case Type::EnableDisplay:
			return "EnableDisplay";
		case Type::SetHeadlights:
			return "SetHeadlights";
		case Type::GotoPose:
			return "GotoPose";
		case Type::PlaceObjectOnGround:
			return "PlaceObjectOnGround";
		case Type::PlaceObjectOnGroundHere:
			return "PlaceObjectOnGroundHere";
		case Type::ExecuteTestPlan:
			return "ExecuteTestPlan";
		case Type::SelectNextObject:
			return "SelectNextObject";
		case Type::PickAndPlaceObject:
			return "PickAndPlaceObject";
		case Type::TraverseObject:
			return "TraverseObject";
		case Type::ClearAllBlocks:
			return "ClearAllBlocks";
		case Type::ExecuteBehavior:
			return "ExecuteBehavior";
		case Type::SetBehaviorState:
			return "SetBehaviorState";
		case Type::AbortPath:
			return "AbortPath";
		case Type::AbortAll:
			return "AbortAll";
		case Type::DrawPoseMarker:
			return "DrawPoseMarker";
		case Type::ErasePoseMarker:
			return "ErasePoseMarker";
		case Type::SetHeadControllerGains:
			return "SetHeadControllerGains";
		case Type::SetLiftControllerGains:
			return "SetLiftControllerGains";
		case Type::SelectNextSoundScheme:
			return "SelectNextSoundScheme";
		case Type::StartTestMode:
			return "StartTestMode";
		case Type::IMURequest:
			return "IMURequest";
		case Type::PlayAnimation:
			return "PlayAnimation";
		case Type::ReadAnimationFile:
			return "ReadAnimationFile";
		case Type::StartFaceTracking:
			return "StartFaceTracking";
		case Type::StopFaceTracking:
			return "StopFaceTracking";
		case Type::StartLookingForMarkers:
			return "StartLookingForMarkers";
		case Type::StopLookingForMarkers:
			return "StopLookingForMarkers";
		case Type::SetVisionSystemParams:
			return "SetVisionSystemParams";
		case Type::SetFaceDetectParams:
			return "SetFaceDetectParams";
		default:
			return "INVALID";
		}
	}
	Type GetType() const { return _type; }

	/** Ping **/
	const U2G_Ping& Get_Ping() const
	{
		assert(_type == Type::Ping);
		return _Ping;
	}
	void Set_Ping(const U2G_Ping& new_Ping)
	{
		if(this->_type == Type::Ping) {
			_Ping = new_Ping;
		}
		else {
			ClearCurrent();
			new(&_Ping) U2G_Ping{new_Ping};
			_type = Type::Ping;
		}
	}
	void Set_Ping(U2G_Ping&&  new_Ping)
	{
		if(this->_type == Type::Ping) {
			_Ping = std::move(new_Ping);
		}
		else {
			ClearCurrent();
			new(&_Ping) U2G_Ping{std::move(new_Ping)};
			_type = Type::Ping;
		}
	}

	/** ConnectToRobot **/
	const U2G_ConnectToRobot& Get_ConnectToRobot() const
	{
		assert(_type == Type::ConnectToRobot);
		return _ConnectToRobot;
	}
	void Set_ConnectToRobot(const U2G_ConnectToRobot& new_ConnectToRobot)
	{
		if(this->_type == Type::ConnectToRobot) {
			_ConnectToRobot = new_ConnectToRobot;
		}
		else {
			ClearCurrent();
			new(&_ConnectToRobot) U2G_ConnectToRobot{new_ConnectToRobot};
			_type = Type::ConnectToRobot;
		}
	}
	void Set_ConnectToRobot(U2G_ConnectToRobot&&  new_ConnectToRobot)
	{
		if(this->_type == Type::ConnectToRobot) {
			_ConnectToRobot = std::move(new_ConnectToRobot);
		}
		else {
			ClearCurrent();
			new(&_ConnectToRobot) U2G_ConnectToRobot{std::move(new_ConnectToRobot)};
			_type = Type::ConnectToRobot;
		}
	}

	/** ConnectToUiDevice **/
	const U2G_ConnectToUiDevice& Get_ConnectToUiDevice() const
	{
		assert(_type == Type::ConnectToUiDevice);
		return _ConnectToUiDevice;
	}
	void Set_ConnectToUiDevice(const U2G_ConnectToUiDevice& new_ConnectToUiDevice)
	{
		if(this->_type == Type::ConnectToUiDevice) {
			_ConnectToUiDevice = new_ConnectToUiDevice;
		}
		else {
			ClearCurrent();
			new(&_ConnectToUiDevice) U2G_ConnectToUiDevice{new_ConnectToUiDevice};
			_type = Type::ConnectToUiDevice;
		}
	}
	void Set_ConnectToUiDevice(U2G_ConnectToUiDevice&&  new_ConnectToUiDevice)
	{
		if(this->_type == Type::ConnectToUiDevice) {
			_ConnectToUiDevice = std::move(new_ConnectToUiDevice);
		}
		else {
			ClearCurrent();
			new(&_ConnectToUiDevice) U2G_ConnectToUiDevice{std::move(new_ConnectToUiDevice)};
			_type = Type::ConnectToUiDevice;
		}
	}

	/** DisconnectFromUiDevice **/
	const U2G_DisconnectFromUiDevice& Get_DisconnectFromUiDevice() const
	{
		assert(_type == Type::DisconnectFromUiDevice);
		return _DisconnectFromUiDevice;
	}
	void Set_DisconnectFromUiDevice(const U2G_DisconnectFromUiDevice& new_DisconnectFromUiDevice)
	{
		if(this->_type == Type::DisconnectFromUiDevice) {
			_DisconnectFromUiDevice = new_DisconnectFromUiDevice;
		}
		else {
			ClearCurrent();
			new(&_DisconnectFromUiDevice) U2G_DisconnectFromUiDevice{new_DisconnectFromUiDevice};
			_type = Type::DisconnectFromUiDevice;
		}
	}
	void Set_DisconnectFromUiDevice(U2G_DisconnectFromUiDevice&&  new_DisconnectFromUiDevice)
	{
		if(this->_type == Type::DisconnectFromUiDevice) {
			_DisconnectFromUiDevice = std::move(new_DisconnectFromUiDevice);
		}
		else {
			ClearCurrent();
			new(&_DisconnectFromUiDevice) U2G_DisconnectFromUiDevice{std::move(new_DisconnectFromUiDevice)};
			_type = Type::DisconnectFromUiDevice;
		}
	}

	/** ForceAddRobot **/
	const U2G_ForceAddRobot& Get_ForceAddRobot() const
	{
		assert(_type == Type::ForceAddRobot);
		return _ForceAddRobot;
	}
	void Set_ForceAddRobot(const U2G_ForceAddRobot& new_ForceAddRobot)
	{
		if(this->_type == Type::ForceAddRobot) {
			_ForceAddRobot = new_ForceAddRobot;
		}
		else {
			ClearCurrent();
			new(&_ForceAddRobot) U2G_ForceAddRobot{new_ForceAddRobot};
			_type = Type::ForceAddRobot;
		}
	}
	void Set_ForceAddRobot(U2G_ForceAddRobot&&  new_ForceAddRobot)
	{
		if(this->_type == Type::ForceAddRobot) {
			_ForceAddRobot = std::move(new_ForceAddRobot);
		}
		else {
			ClearCurrent();
			new(&_ForceAddRobot) U2G_ForceAddRobot{std::move(new_ForceAddRobot)};
			_type = Type::ForceAddRobot;
		}
	}

	/** StartEngine **/
	const U2G_StartEngine& Get_StartEngine() const
	{
		assert(_type == Type::StartEngine);
		return _StartEngine;
	}
	void Set_StartEngine(const U2G_StartEngine& new_StartEngine)
	{
		if(this->_type == Type::StartEngine) {
			_StartEngine = new_StartEngine;
		}
		else {
			ClearCurrent();
			new(&_StartEngine) U2G_StartEngine{new_StartEngine};
			_type = Type::StartEngine;
		}
	}
	void Set_StartEngine(U2G_StartEngine&&  new_StartEngine)
	{
		if(this->_type == Type::StartEngine) {
			_StartEngine = std::move(new_StartEngine);
		}
		else {
			ClearCurrent();
			new(&_StartEngine) U2G_StartEngine{std::move(new_StartEngine)};
			_type = Type::StartEngine;
		}
	}

	/** DriveWheels **/
	const U2G_DriveWheels& Get_DriveWheels() const
	{
		assert(_type == Type::DriveWheels);
		return _DriveWheels;
	}
	void Set_DriveWheels(const U2G_DriveWheels& new_DriveWheels)
	{
		if(this->_type == Type::DriveWheels) {
			_DriveWheels = new_DriveWheels;
		}
		else {
			ClearCurrent();
			new(&_DriveWheels) U2G_DriveWheels{new_DriveWheels};
			_type = Type::DriveWheels;
		}
	}
	void Set_DriveWheels(U2G_DriveWheels&&  new_DriveWheels)
	{
		if(this->_type == Type::DriveWheels) {
			_DriveWheels = std::move(new_DriveWheels);
		}
		else {
			ClearCurrent();
			new(&_DriveWheels) U2G_DriveWheels{std::move(new_DriveWheels)};
			_type = Type::DriveWheels;
		}
	}

	/** TurnInPlace **/
	const U2G_TurnInPlace& Get_TurnInPlace() const
	{
		assert(_type == Type::TurnInPlace);
		return _TurnInPlace;
	}
	void Set_TurnInPlace(const U2G_TurnInPlace& new_TurnInPlace)
	{
		if(this->_type == Type::TurnInPlace) {
			_TurnInPlace = new_TurnInPlace;
		}
		else {
			ClearCurrent();
			new(&_TurnInPlace) U2G_TurnInPlace{new_TurnInPlace};
			_type = Type::TurnInPlace;
		}
	}
	void Set_TurnInPlace(U2G_TurnInPlace&&  new_TurnInPlace)
	{
		if(this->_type == Type::TurnInPlace) {
			_TurnInPlace = std::move(new_TurnInPlace);
		}
		else {
			ClearCurrent();
			new(&_TurnInPlace) U2G_TurnInPlace{std::move(new_TurnInPlace)};
			_type = Type::TurnInPlace;
		}
	}

	/** MoveHead **/
	const U2G_MoveHead& Get_MoveHead() const
	{
		assert(_type == Type::MoveHead);
		return _MoveHead;
	}
	void Set_MoveHead(const U2G_MoveHead& new_MoveHead)
	{
		if(this->_type == Type::MoveHead) {
			_MoveHead = new_MoveHead;
		}
		else {
			ClearCurrent();
			new(&_MoveHead) U2G_MoveHead{new_MoveHead};
			_type = Type::MoveHead;
		}
	}
	void Set_MoveHead(U2G_MoveHead&&  new_MoveHead)
	{
		if(this->_type == Type::MoveHead) {
			_MoveHead = std::move(new_MoveHead);
		}
		else {
			ClearCurrent();
			new(&_MoveHead) U2G_MoveHead{std::move(new_MoveHead)};
			_type = Type::MoveHead;
		}
	}

	/** MoveLift **/
	const U2G_MoveLift& Get_MoveLift() const
	{
		assert(_type == Type::MoveLift);
		return _MoveLift;
	}
	void Set_MoveLift(const U2G_MoveLift& new_MoveLift)
	{
		if(this->_type == Type::MoveLift) {
			_MoveLift = new_MoveLift;
		}
		else {
			ClearCurrent();
			new(&_MoveLift) U2G_MoveLift{new_MoveLift};
			_type = Type::MoveLift;
		}
	}
	void Set_MoveLift(U2G_MoveLift&&  new_MoveLift)
	{
		if(this->_type == Type::MoveLift) {
			_MoveLift = std::move(new_MoveLift);
		}
		else {
			ClearCurrent();
			new(&_MoveLift) U2G_MoveLift{std::move(new_MoveLift)};
			_type = Type::MoveLift;
		}
	}

	/** SetLiftHeight **/
	const U2G_SetLiftHeight& Get_SetLiftHeight() const
	{
		assert(_type == Type::SetLiftHeight);
		return _SetLiftHeight;
	}
	void Set_SetLiftHeight(const U2G_SetLiftHeight& new_SetLiftHeight)
	{
		if(this->_type == Type::SetLiftHeight) {
			_SetLiftHeight = new_SetLiftHeight;
		}
		else {
			ClearCurrent();
			new(&_SetLiftHeight) U2G_SetLiftHeight{new_SetLiftHeight};
			_type = Type::SetLiftHeight;
		}
	}
	void Set_SetLiftHeight(U2G_SetLiftHeight&&  new_SetLiftHeight)
	{
		if(this->_type == Type::SetLiftHeight) {
			_SetLiftHeight = std::move(new_SetLiftHeight);
		}
		else {
			ClearCurrent();
			new(&_SetLiftHeight) U2G_SetLiftHeight{std::move(new_SetLiftHeight)};
			_type = Type::SetLiftHeight;
		}
	}

	/** SetHeadAngle **/
	const U2G_SetHeadAngle& Get_SetHeadAngle() const
	{
		assert(_type == Type::SetHeadAngle);
		return _SetHeadAngle;
	}
	void Set_SetHeadAngle(const U2G_SetHeadAngle& new_SetHeadAngle)
	{
		if(this->_type == Type::SetHeadAngle) {
			_SetHeadAngle = new_SetHeadAngle;
		}
		else {
			ClearCurrent();
			new(&_SetHeadAngle) U2G_SetHeadAngle{new_SetHeadAngle};
			_type = Type::SetHeadAngle;
		}
	}
	void Set_SetHeadAngle(U2G_SetHeadAngle&&  new_SetHeadAngle)
	{
		if(this->_type == Type::SetHeadAngle) {
			_SetHeadAngle = std::move(new_SetHeadAngle);
		}
		else {
			ClearCurrent();
			new(&_SetHeadAngle) U2G_SetHeadAngle{std::move(new_SetHeadAngle)};
			_type = Type::SetHeadAngle;
		}
	}

	/** StopAllMotors **/
	const U2G_StopAllMotors& Get_StopAllMotors() const
	{
		assert(_type == Type::StopAllMotors);
		return _StopAllMotors;
	}
	void Set_StopAllMotors(const U2G_StopAllMotors& new_StopAllMotors)
	{
		if(this->_type == Type::StopAllMotors) {
			_StopAllMotors = new_StopAllMotors;
		}
		else {
			ClearCurrent();
			new(&_StopAllMotors) U2G_StopAllMotors{new_StopAllMotors};
			_type = Type::StopAllMotors;
		}
	}
	void Set_StopAllMotors(U2G_StopAllMotors&&  new_StopAllMotors)
	{
		if(this->_type == Type::StopAllMotors) {
			_StopAllMotors = std::move(new_StopAllMotors);
		}
		else {
			ClearCurrent();
			new(&_StopAllMotors) U2G_StopAllMotors{std::move(new_StopAllMotors)};
			_type = Type::StopAllMotors;
		}
	}

	/** ImageRequest **/
	const U2G_ImageRequest& Get_ImageRequest() const
	{
		assert(_type == Type::ImageRequest);
		return _ImageRequest;
	}
	void Set_ImageRequest(const U2G_ImageRequest& new_ImageRequest)
	{
		if(this->_type == Type::ImageRequest) {
			_ImageRequest = new_ImageRequest;
		}
		else {
			ClearCurrent();
			new(&_ImageRequest) U2G_ImageRequest{new_ImageRequest};
			_type = Type::ImageRequest;
		}
	}
	void Set_ImageRequest(U2G_ImageRequest&&  new_ImageRequest)
	{
		if(this->_type == Type::ImageRequest) {
			_ImageRequest = std::move(new_ImageRequest);
		}
		else {
			ClearCurrent();
			new(&_ImageRequest) U2G_ImageRequest{std::move(new_ImageRequest)};
			_type = Type::ImageRequest;
		}
	}

	/** SetRobotImageSendMode **/
	const U2G_SetRobotImageSendMode& Get_SetRobotImageSendMode() const
	{
		assert(_type == Type::SetRobotImageSendMode);
		return _SetRobotImageSendMode;
	}
	void Set_SetRobotImageSendMode(const U2G_SetRobotImageSendMode& new_SetRobotImageSendMode)
	{
		if(this->_type == Type::SetRobotImageSendMode) {
			_SetRobotImageSendMode = new_SetRobotImageSendMode;
		}
		else {
			ClearCurrent();
			new(&_SetRobotImageSendMode) U2G_SetRobotImageSendMode{new_SetRobotImageSendMode};
			_type = Type::SetRobotImageSendMode;
		}
	}
	void Set_SetRobotImageSendMode(U2G_SetRobotImageSendMode&&  new_SetRobotImageSendMode)
	{
		if(this->_type == Type::SetRobotImageSendMode) {
			_SetRobotImageSendMode = std::move(new_SetRobotImageSendMode);
		}
		else {
			ClearCurrent();
			new(&_SetRobotImageSendMode) U2G_SetRobotImageSendMode{std::move(new_SetRobotImageSendMode)};
			_type = Type::SetRobotImageSendMode;
		}
	}

	/** SaveImages **/
	const U2G_SaveImages& Get_SaveImages() const
	{
		assert(_type == Type::SaveImages);
		return _SaveImages;
	}
	void Set_SaveImages(const U2G_SaveImages& new_SaveImages)
	{
		if(this->_type == Type::SaveImages) {
			_SaveImages = new_SaveImages;
		}
		else {
			ClearCurrent();
			new(&_SaveImages) U2G_SaveImages{new_SaveImages};
			_type = Type::SaveImages;
		}
	}
	void Set_SaveImages(U2G_SaveImages&&  new_SaveImages)
	{
		if(this->_type == Type::SaveImages) {
			_SaveImages = std::move(new_SaveImages);
		}
		else {
			ClearCurrent();
			new(&_SaveImages) U2G_SaveImages{std::move(new_SaveImages)};
			_type = Type::SaveImages;
		}
	}

	/** EnableDisplay **/
	const U2G_EnableDisplay& Get_EnableDisplay() const
	{
		assert(_type == Type::EnableDisplay);
		return _EnableDisplay;
	}
	void Set_EnableDisplay(const U2G_EnableDisplay& new_EnableDisplay)
	{
		if(this->_type == Type::EnableDisplay) {
			_EnableDisplay = new_EnableDisplay;
		}
		else {
			ClearCurrent();
			new(&_EnableDisplay) U2G_EnableDisplay{new_EnableDisplay};
			_type = Type::EnableDisplay;
		}
	}
	void Set_EnableDisplay(U2G_EnableDisplay&&  new_EnableDisplay)
	{
		if(this->_type == Type::EnableDisplay) {
			_EnableDisplay = std::move(new_EnableDisplay);
		}
		else {
			ClearCurrent();
			new(&_EnableDisplay) U2G_EnableDisplay{std::move(new_EnableDisplay)};
			_type = Type::EnableDisplay;
		}
	}

	/** SetHeadlights **/
	const U2G_SetHeadlights& Get_SetHeadlights() const
	{
		assert(_type == Type::SetHeadlights);
		return _SetHeadlights;
	}
	void Set_SetHeadlights(const U2G_SetHeadlights& new_SetHeadlights)
	{
		if(this->_type == Type::SetHeadlights) {
			_SetHeadlights = new_SetHeadlights;
		}
		else {
			ClearCurrent();
			new(&_SetHeadlights) U2G_SetHeadlights{new_SetHeadlights};
			_type = Type::SetHeadlights;
		}
	}
	void Set_SetHeadlights(U2G_SetHeadlights&&  new_SetHeadlights)
	{
		if(this->_type == Type::SetHeadlights) {
			_SetHeadlights = std::move(new_SetHeadlights);
		}
		else {
			ClearCurrent();
			new(&_SetHeadlights) U2G_SetHeadlights{std::move(new_SetHeadlights)};
			_type = Type::SetHeadlights;
		}
	}

	/** GotoPose **/
	const U2G_GotoPose& Get_GotoPose() const
	{
		assert(_type == Type::GotoPose);
		return _GotoPose;
	}
	void Set_GotoPose(const U2G_GotoPose& new_GotoPose)
	{
		if(this->_type == Type::GotoPose) {
			_GotoPose = new_GotoPose;
		}
		else {
			ClearCurrent();
			new(&_GotoPose) U2G_GotoPose{new_GotoPose};
			_type = Type::GotoPose;
		}
	}
	void Set_GotoPose(U2G_GotoPose&&  new_GotoPose)
	{
		if(this->_type == Type::GotoPose) {
			_GotoPose = std::move(new_GotoPose);
		}
		else {
			ClearCurrent();
			new(&_GotoPose) U2G_GotoPose{std::move(new_GotoPose)};
			_type = Type::GotoPose;
		}
	}

	/** PlaceObjectOnGround **/
	const U2G_PlaceObjectOnGround& Get_PlaceObjectOnGround() const
	{
		assert(_type == Type::PlaceObjectOnGround);
		return _PlaceObjectOnGround;
	}
	void Set_PlaceObjectOnGround(const U2G_PlaceObjectOnGround& new_PlaceObjectOnGround)
	{
		if(this->_type == Type::PlaceObjectOnGround) {
			_PlaceObjectOnGround = new_PlaceObjectOnGround;
		}
		else {
			ClearCurrent();
			new(&_PlaceObjectOnGround) U2G_PlaceObjectOnGround{new_PlaceObjectOnGround};
			_type = Type::PlaceObjectOnGround;
		}
	}
	void Set_PlaceObjectOnGround(U2G_PlaceObjectOnGround&&  new_PlaceObjectOnGround)
	{
		if(this->_type == Type::PlaceObjectOnGround) {
			_PlaceObjectOnGround = std::move(new_PlaceObjectOnGround);
		}
		else {
			ClearCurrent();
			new(&_PlaceObjectOnGround) U2G_PlaceObjectOnGround{std::move(new_PlaceObjectOnGround)};
			_type = Type::PlaceObjectOnGround;
		}
	}

	/** PlaceObjectOnGroundHere **/
	const U2G_PlaceObjectOnGroundHere& Get_PlaceObjectOnGroundHere() const
	{
		assert(_type == Type::PlaceObjectOnGroundHere);
		return _PlaceObjectOnGroundHere;
	}
	void Set_PlaceObjectOnGroundHere(const U2G_PlaceObjectOnGroundHere& new_PlaceObjectOnGroundHere)
	{
		if(this->_type == Type::PlaceObjectOnGroundHere) {
			_PlaceObjectOnGroundHere = new_PlaceObjectOnGroundHere;
		}
		else {
			ClearCurrent();
			new(&_PlaceObjectOnGroundHere) U2G_PlaceObjectOnGroundHere{new_PlaceObjectOnGroundHere};
			_type = Type::PlaceObjectOnGroundHere;
		}
	}
	void Set_PlaceObjectOnGroundHere(U2G_PlaceObjectOnGroundHere&&  new_PlaceObjectOnGroundHere)
	{
		if(this->_type == Type::PlaceObjectOnGroundHere) {
			_PlaceObjectOnGroundHere = std::move(new_PlaceObjectOnGroundHere);
		}
		else {
			ClearCurrent();
			new(&_PlaceObjectOnGroundHere) U2G_PlaceObjectOnGroundHere{std::move(new_PlaceObjectOnGroundHere)};
			_type = Type::PlaceObjectOnGroundHere;
		}
	}

	/** ExecuteTestPlan **/
	const U2G_ExecuteTestPlan& Get_ExecuteTestPlan() const
	{
		assert(_type == Type::ExecuteTestPlan);
		return _ExecuteTestPlan;
	}
	void Set_ExecuteTestPlan(const U2G_ExecuteTestPlan& new_ExecuteTestPlan)
	{
		if(this->_type == Type::ExecuteTestPlan) {
			_ExecuteTestPlan = new_ExecuteTestPlan;
		}
		else {
			ClearCurrent();
			new(&_ExecuteTestPlan) U2G_ExecuteTestPlan{new_ExecuteTestPlan};
			_type = Type::ExecuteTestPlan;
		}
	}
	void Set_ExecuteTestPlan(U2G_ExecuteTestPlan&&  new_ExecuteTestPlan)
	{
		if(this->_type == Type::ExecuteTestPlan) {
			_ExecuteTestPlan = std::move(new_ExecuteTestPlan);
		}
		else {
			ClearCurrent();
			new(&_ExecuteTestPlan) U2G_ExecuteTestPlan{std::move(new_ExecuteTestPlan)};
			_type = Type::ExecuteTestPlan;
		}
	}

	/** SelectNextObject **/
	const U2G_SelectNextObject& Get_SelectNextObject() const
	{
		assert(_type == Type::SelectNextObject);
		return _SelectNextObject;
	}
	void Set_SelectNextObject(const U2G_SelectNextObject& new_SelectNextObject)
	{
		if(this->_type == Type::SelectNextObject) {
			_SelectNextObject = new_SelectNextObject;
		}
		else {
			ClearCurrent();
			new(&_SelectNextObject) U2G_SelectNextObject{new_SelectNextObject};
			_type = Type::SelectNextObject;
		}
	}
	void Set_SelectNextObject(U2G_SelectNextObject&&  new_SelectNextObject)
	{
		if(this->_type == Type::SelectNextObject) {
			_SelectNextObject = std::move(new_SelectNextObject);
		}
		else {
			ClearCurrent();
			new(&_SelectNextObject) U2G_SelectNextObject{std::move(new_SelectNextObject)};
			_type = Type::SelectNextObject;
		}
	}

	/** PickAndPlaceObject **/
	const U2G_PickAndPlaceObject& Get_PickAndPlaceObject() const
	{
		assert(_type == Type::PickAndPlaceObject);
		return _PickAndPlaceObject;
	}
	void Set_PickAndPlaceObject(const U2G_PickAndPlaceObject& new_PickAndPlaceObject)
	{
		if(this->_type == Type::PickAndPlaceObject) {
			_PickAndPlaceObject = new_PickAndPlaceObject;
		}
		else {
			ClearCurrent();
			new(&_PickAndPlaceObject) U2G_PickAndPlaceObject{new_PickAndPlaceObject};
			_type = Type::PickAndPlaceObject;
		}
	}
	void Set_PickAndPlaceObject(U2G_PickAndPlaceObject&&  new_PickAndPlaceObject)
	{
		if(this->_type == Type::PickAndPlaceObject) {
			_PickAndPlaceObject = std::move(new_PickAndPlaceObject);
		}
		else {
			ClearCurrent();
			new(&_PickAndPlaceObject) U2G_PickAndPlaceObject{std::move(new_PickAndPlaceObject)};
			_type = Type::PickAndPlaceObject;
		}
	}

	/** TraverseObject **/
	const U2G_TraverseObject& Get_TraverseObject() const
	{
		assert(_type == Type::TraverseObject);
		return _TraverseObject;
	}
	void Set_TraverseObject(const U2G_TraverseObject& new_TraverseObject)
	{
		if(this->_type == Type::TraverseObject) {
			_TraverseObject = new_TraverseObject;
		}
		else {
			ClearCurrent();
			new(&_TraverseObject) U2G_TraverseObject{new_TraverseObject};
			_type = Type::TraverseObject;
		}
	}
	void Set_TraverseObject(U2G_TraverseObject&&  new_TraverseObject)
	{
		if(this->_type == Type::TraverseObject) {
			_TraverseObject = std::move(new_TraverseObject);
		}
		else {
			ClearCurrent();
			new(&_TraverseObject) U2G_TraverseObject{std::move(new_TraverseObject)};
			_type = Type::TraverseObject;
		}
	}

	/** ClearAllBlocks **/
	const U2G_ClearAllBlocks& Get_ClearAllBlocks() const
	{
		assert(_type == Type::ClearAllBlocks);
		return _ClearAllBlocks;
	}
	void Set_ClearAllBlocks(const U2G_ClearAllBlocks& new_ClearAllBlocks)
	{
		if(this->_type == Type::ClearAllBlocks) {
			_ClearAllBlocks = new_ClearAllBlocks;
		}
		else {
			ClearCurrent();
			new(&_ClearAllBlocks) U2G_ClearAllBlocks{new_ClearAllBlocks};
			_type = Type::ClearAllBlocks;
		}
	}
	void Set_ClearAllBlocks(U2G_ClearAllBlocks&&  new_ClearAllBlocks)
	{
		if(this->_type == Type::ClearAllBlocks) {
			_ClearAllBlocks = std::move(new_ClearAllBlocks);
		}
		else {
			ClearCurrent();
			new(&_ClearAllBlocks) U2G_ClearAllBlocks{std::move(new_ClearAllBlocks)};
			_type = Type::ClearAllBlocks;
		}
	}

	/** ExecuteBehavior **/
	const U2G_ExecuteBehavior& Get_ExecuteBehavior() const
	{
		assert(_type == Type::ExecuteBehavior);
		return _ExecuteBehavior;
	}
	void Set_ExecuteBehavior(const U2G_ExecuteBehavior& new_ExecuteBehavior)
	{
		if(this->_type == Type::ExecuteBehavior) {
			_ExecuteBehavior = new_ExecuteBehavior;
		}
		else {
			ClearCurrent();
			new(&_ExecuteBehavior) U2G_ExecuteBehavior{new_ExecuteBehavior};
			_type = Type::ExecuteBehavior;
		}
	}
	void Set_ExecuteBehavior(U2G_ExecuteBehavior&&  new_ExecuteBehavior)
	{
		if(this->_type == Type::ExecuteBehavior) {
			_ExecuteBehavior = std::move(new_ExecuteBehavior);
		}
		else {
			ClearCurrent();
			new(&_ExecuteBehavior) U2G_ExecuteBehavior{std::move(new_ExecuteBehavior)};
			_type = Type::ExecuteBehavior;
		}
	}

	/** SetBehaviorState **/
	const U2G_SetBehaviorState& Get_SetBehaviorState() const
	{
		assert(_type == Type::SetBehaviorState);
		return _SetBehaviorState;
	}
	void Set_SetBehaviorState(const U2G_SetBehaviorState& new_SetBehaviorState)
	{
		if(this->_type == Type::SetBehaviorState) {
			_SetBehaviorState = new_SetBehaviorState;
		}
		else {
			ClearCurrent();
			new(&_SetBehaviorState) U2G_SetBehaviorState{new_SetBehaviorState};
			_type = Type::SetBehaviorState;
		}
	}
	void Set_SetBehaviorState(U2G_SetBehaviorState&&  new_SetBehaviorState)
	{
		if(this->_type == Type::SetBehaviorState) {
			_SetBehaviorState = std::move(new_SetBehaviorState);
		}
		else {
			ClearCurrent();
			new(&_SetBehaviorState) U2G_SetBehaviorState{std::move(new_SetBehaviorState)};
			_type = Type::SetBehaviorState;
		}
	}

	/** AbortPath **/
	const U2G_AbortPath& Get_AbortPath() const
	{
		assert(_type == Type::AbortPath);
		return _AbortPath;
	}
	void Set_AbortPath(const U2G_AbortPath& new_AbortPath)
	{
		if(this->_type == Type::AbortPath) {
			_AbortPath = new_AbortPath;
		}
		else {
			ClearCurrent();
			new(&_AbortPath) U2G_AbortPath{new_AbortPath};
			_type = Type::AbortPath;
		}
	}
	void Set_AbortPath(U2G_AbortPath&&  new_AbortPath)
	{
		if(this->_type == Type::AbortPath) {
			_AbortPath = std::move(new_AbortPath);
		}
		else {
			ClearCurrent();
			new(&_AbortPath) U2G_AbortPath{std::move(new_AbortPath)};
			_type = Type::AbortPath;
		}
	}

	/** AbortAll **/
	const U2G_AbortAll& Get_AbortAll() const
	{
		assert(_type == Type::AbortAll);
		return _AbortAll;
	}
	void Set_AbortAll(const U2G_AbortAll& new_AbortAll)
	{
		if(this->_type == Type::AbortAll) {
			_AbortAll = new_AbortAll;
		}
		else {
			ClearCurrent();
			new(&_AbortAll) U2G_AbortAll{new_AbortAll};
			_type = Type::AbortAll;
		}
	}
	void Set_AbortAll(U2G_AbortAll&&  new_AbortAll)
	{
		if(this->_type == Type::AbortAll) {
			_AbortAll = std::move(new_AbortAll);
		}
		else {
			ClearCurrent();
			new(&_AbortAll) U2G_AbortAll{std::move(new_AbortAll)};
			_type = Type::AbortAll;
		}
	}

	/** DrawPoseMarker **/
	const U2G_DrawPoseMarker& Get_DrawPoseMarker() const
	{
		assert(_type == Type::DrawPoseMarker);
		return _DrawPoseMarker;
	}
	void Set_DrawPoseMarker(const U2G_DrawPoseMarker& new_DrawPoseMarker)
	{
		if(this->_type == Type::DrawPoseMarker) {
			_DrawPoseMarker = new_DrawPoseMarker;
		}
		else {
			ClearCurrent();
			new(&_DrawPoseMarker) U2G_DrawPoseMarker{new_DrawPoseMarker};
			_type = Type::DrawPoseMarker;
		}
	}
	void Set_DrawPoseMarker(U2G_DrawPoseMarker&&  new_DrawPoseMarker)
	{
		if(this->_type == Type::DrawPoseMarker) {
			_DrawPoseMarker = std::move(new_DrawPoseMarker);
		}
		else {
			ClearCurrent();
			new(&_DrawPoseMarker) U2G_DrawPoseMarker{std::move(new_DrawPoseMarker)};
			_type = Type::DrawPoseMarker;
		}
	}

	/** ErasePoseMarker **/
	const U2G_ErasePoseMarker& Get_ErasePoseMarker() const
	{
		assert(_type == Type::ErasePoseMarker);
		return _ErasePoseMarker;
	}
	void Set_ErasePoseMarker(const U2G_ErasePoseMarker& new_ErasePoseMarker)
	{
		if(this->_type == Type::ErasePoseMarker) {
			_ErasePoseMarker = new_ErasePoseMarker;
		}
		else {
			ClearCurrent();
			new(&_ErasePoseMarker) U2G_ErasePoseMarker{new_ErasePoseMarker};
			_type = Type::ErasePoseMarker;
		}
	}
	void Set_ErasePoseMarker(U2G_ErasePoseMarker&&  new_ErasePoseMarker)
	{
		if(this->_type == Type::ErasePoseMarker) {
			_ErasePoseMarker = std::move(new_ErasePoseMarker);
		}
		else {
			ClearCurrent();
			new(&_ErasePoseMarker) U2G_ErasePoseMarker{std::move(new_ErasePoseMarker)};
			_type = Type::ErasePoseMarker;
		}
	}

	/** SetHeadControllerGains **/
	const U2G_SetHeadControllerGains& Get_SetHeadControllerGains() const
	{
		assert(_type == Type::SetHeadControllerGains);
		return _SetHeadControllerGains;
	}
	void Set_SetHeadControllerGains(const U2G_SetHeadControllerGains& new_SetHeadControllerGains)
	{
		if(this->_type == Type::SetHeadControllerGains) {
			_SetHeadControllerGains = new_SetHeadControllerGains;
		}
		else {
			ClearCurrent();
			new(&_SetHeadControllerGains) U2G_SetHeadControllerGains{new_SetHeadControllerGains};
			_type = Type::SetHeadControllerGains;
		}
	}
	void Set_SetHeadControllerGains(U2G_SetHeadControllerGains&&  new_SetHeadControllerGains)
	{
		if(this->_type == Type::SetHeadControllerGains) {
			_SetHeadControllerGains = std::move(new_SetHeadControllerGains);
		}
		else {
			ClearCurrent();
			new(&_SetHeadControllerGains) U2G_SetHeadControllerGains{std::move(new_SetHeadControllerGains)};
			_type = Type::SetHeadControllerGains;
		}
	}

	/** SetLiftControllerGains **/
	const U2G_SetLiftControllerGains& Get_SetLiftControllerGains() const
	{
		assert(_type == Type::SetLiftControllerGains);
		return _SetLiftControllerGains;
	}
	void Set_SetLiftControllerGains(const U2G_SetLiftControllerGains& new_SetLiftControllerGains)
	{
		if(this->_type == Type::SetLiftControllerGains) {
			_SetLiftControllerGains = new_SetLiftControllerGains;
		}
		else {
			ClearCurrent();
			new(&_SetLiftControllerGains) U2G_SetLiftControllerGains{new_SetLiftControllerGains};
			_type = Type::SetLiftControllerGains;
		}
	}
	void Set_SetLiftControllerGains(U2G_SetLiftControllerGains&&  new_SetLiftControllerGains)
	{
		if(this->_type == Type::SetLiftControllerGains) {
			_SetLiftControllerGains = std::move(new_SetLiftControllerGains);
		}
		else {
			ClearCurrent();
			new(&_SetLiftControllerGains) U2G_SetLiftControllerGains{std::move(new_SetLiftControllerGains)};
			_type = Type::SetLiftControllerGains;
		}
	}

	/** SelectNextSoundScheme **/
	const U2G_SelectNextSoundScheme& Get_SelectNextSoundScheme() const
	{
		assert(_type == Type::SelectNextSoundScheme);
		return _SelectNextSoundScheme;
	}
	void Set_SelectNextSoundScheme(const U2G_SelectNextSoundScheme& new_SelectNextSoundScheme)
	{
		if(this->_type == Type::SelectNextSoundScheme) {
			_SelectNextSoundScheme = new_SelectNextSoundScheme;
		}
		else {
			ClearCurrent();
			new(&_SelectNextSoundScheme) U2G_SelectNextSoundScheme{new_SelectNextSoundScheme};
			_type = Type::SelectNextSoundScheme;
		}
	}
	void Set_SelectNextSoundScheme(U2G_SelectNextSoundScheme&&  new_SelectNextSoundScheme)
	{
		if(this->_type == Type::SelectNextSoundScheme) {
			_SelectNextSoundScheme = std::move(new_SelectNextSoundScheme);
		}
		else {
			ClearCurrent();
			new(&_SelectNextSoundScheme) U2G_SelectNextSoundScheme{std::move(new_SelectNextSoundScheme)};
			_type = Type::SelectNextSoundScheme;
		}
	}

	/** StartTestMode **/
	const U2G_StartTestMode& Get_StartTestMode() const
	{
		assert(_type == Type::StartTestMode);
		return _StartTestMode;
	}
	void Set_StartTestMode(const U2G_StartTestMode& new_StartTestMode)
	{
		if(this->_type == Type::StartTestMode) {
			_StartTestMode = new_StartTestMode;
		}
		else {
			ClearCurrent();
			new(&_StartTestMode) U2G_StartTestMode{new_StartTestMode};
			_type = Type::StartTestMode;
		}
	}
	void Set_StartTestMode(U2G_StartTestMode&&  new_StartTestMode)
	{
		if(this->_type == Type::StartTestMode) {
			_StartTestMode = std::move(new_StartTestMode);
		}
		else {
			ClearCurrent();
			new(&_StartTestMode) U2G_StartTestMode{std::move(new_StartTestMode)};
			_type = Type::StartTestMode;
		}
	}

	/** IMURequest **/
	const U2G_IMURequest& Get_IMURequest() const
	{
		assert(_type == Type::IMURequest);
		return _IMURequest;
	}
	void Set_IMURequest(const U2G_IMURequest& new_IMURequest)
	{
		if(this->_type == Type::IMURequest) {
			_IMURequest = new_IMURequest;
		}
		else {
			ClearCurrent();
			new(&_IMURequest) U2G_IMURequest{new_IMURequest};
			_type = Type::IMURequest;
		}
	}
	void Set_IMURequest(U2G_IMURequest&&  new_IMURequest)
	{
		if(this->_type == Type::IMURequest) {
			_IMURequest = std::move(new_IMURequest);
		}
		else {
			ClearCurrent();
			new(&_IMURequest) U2G_IMURequest{std::move(new_IMURequest)};
			_type = Type::IMURequest;
		}
	}

	/** PlayAnimation **/
	const U2G_PlayAnimation& Get_PlayAnimation() const
	{
		assert(_type == Type::PlayAnimation);
		return _PlayAnimation;
	}
	void Set_PlayAnimation(const U2G_PlayAnimation& new_PlayAnimation)
	{
		if(this->_type == Type::PlayAnimation) {
			_PlayAnimation = new_PlayAnimation;
		}
		else {
			ClearCurrent();
			new(&_PlayAnimation) U2G_PlayAnimation{new_PlayAnimation};
			_type = Type::PlayAnimation;
		}
	}
	void Set_PlayAnimation(U2G_PlayAnimation&&  new_PlayAnimation)
	{
		if(this->_type == Type::PlayAnimation) {
			_PlayAnimation = std::move(new_PlayAnimation);
		}
		else {
			ClearCurrent();
			new(&_PlayAnimation) U2G_PlayAnimation{std::move(new_PlayAnimation)};
			_type = Type::PlayAnimation;
		}
	}

	/** ReadAnimationFile **/
	const U2G_ReadAnimationFile& Get_ReadAnimationFile() const
	{
		assert(_type == Type::ReadAnimationFile);
		return _ReadAnimationFile;
	}
	void Set_ReadAnimationFile(const U2G_ReadAnimationFile& new_ReadAnimationFile)
	{
		if(this->_type == Type::ReadAnimationFile) {
			_ReadAnimationFile = new_ReadAnimationFile;
		}
		else {
			ClearCurrent();
			new(&_ReadAnimationFile) U2G_ReadAnimationFile{new_ReadAnimationFile};
			_type = Type::ReadAnimationFile;
		}
	}
	void Set_ReadAnimationFile(U2G_ReadAnimationFile&&  new_ReadAnimationFile)
	{
		if(this->_type == Type::ReadAnimationFile) {
			_ReadAnimationFile = std::move(new_ReadAnimationFile);
		}
		else {
			ClearCurrent();
			new(&_ReadAnimationFile) U2G_ReadAnimationFile{std::move(new_ReadAnimationFile)};
			_type = Type::ReadAnimationFile;
		}
	}

	/** StartFaceTracking **/
	const U2G_StartFaceTracking& Get_StartFaceTracking() const
	{
		assert(_type == Type::StartFaceTracking);
		return _StartFaceTracking;
	}
	void Set_StartFaceTracking(const U2G_StartFaceTracking& new_StartFaceTracking)
	{
		if(this->_type == Type::StartFaceTracking) {
			_StartFaceTracking = new_StartFaceTracking;
		}
		else {
			ClearCurrent();
			new(&_StartFaceTracking) U2G_StartFaceTracking{new_StartFaceTracking};
			_type = Type::StartFaceTracking;
		}
	}
	void Set_StartFaceTracking(U2G_StartFaceTracking&&  new_StartFaceTracking)
	{
		if(this->_type == Type::StartFaceTracking) {
			_StartFaceTracking = std::move(new_StartFaceTracking);
		}
		else {
			ClearCurrent();
			new(&_StartFaceTracking) U2G_StartFaceTracking{std::move(new_StartFaceTracking)};
			_type = Type::StartFaceTracking;
		}
	}

	/** StopFaceTracking **/
	const U2G_StopFaceTracking& Get_StopFaceTracking() const
	{
		assert(_type == Type::StopFaceTracking);
		return _StopFaceTracking;
	}
	void Set_StopFaceTracking(const U2G_StopFaceTracking& new_StopFaceTracking)
	{
		if(this->_type == Type::StopFaceTracking) {
			_StopFaceTracking = new_StopFaceTracking;
		}
		else {
			ClearCurrent();
			new(&_StopFaceTracking) U2G_StopFaceTracking{new_StopFaceTracking};
			_type = Type::StopFaceTracking;
		}
	}
	void Set_StopFaceTracking(U2G_StopFaceTracking&&  new_StopFaceTracking)
	{
		if(this->_type == Type::StopFaceTracking) {
			_StopFaceTracking = std::move(new_StopFaceTracking);
		}
		else {
			ClearCurrent();
			new(&_StopFaceTracking) U2G_StopFaceTracking{std::move(new_StopFaceTracking)};
			_type = Type::StopFaceTracking;
		}
	}

	/** StartLookingForMarkers **/
	const U2G_StartLookingForMarkers& Get_StartLookingForMarkers() const
	{
		assert(_type == Type::StartLookingForMarkers);
		return _StartLookingForMarkers;
	}
	void Set_StartLookingForMarkers(const U2G_StartLookingForMarkers& new_StartLookingForMarkers)
	{
		if(this->_type == Type::StartLookingForMarkers) {
			_StartLookingForMarkers = new_StartLookingForMarkers;
		}
		else {
			ClearCurrent();
			new(&_StartLookingForMarkers) U2G_StartLookingForMarkers{new_StartLookingForMarkers};
			_type = Type::StartLookingForMarkers;
		}
	}
	void Set_StartLookingForMarkers(U2G_StartLookingForMarkers&&  new_StartLookingForMarkers)
	{
		if(this->_type == Type::StartLookingForMarkers) {
			_StartLookingForMarkers = std::move(new_StartLookingForMarkers);
		}
		else {
			ClearCurrent();
			new(&_StartLookingForMarkers) U2G_StartLookingForMarkers{std::move(new_StartLookingForMarkers)};
			_type = Type::StartLookingForMarkers;
		}
	}

	/** StopLookingForMarkers **/
	const U2G_StopLookingForMarkers& Get_StopLookingForMarkers() const
	{
		assert(_type == Type::StopLookingForMarkers);
		return _StopLookingForMarkers;
	}
	void Set_StopLookingForMarkers(const U2G_StopLookingForMarkers& new_StopLookingForMarkers)
	{
		if(this->_type == Type::StopLookingForMarkers) {
			_StopLookingForMarkers = new_StopLookingForMarkers;
		}
		else {
			ClearCurrent();
			new(&_StopLookingForMarkers) U2G_StopLookingForMarkers{new_StopLookingForMarkers};
			_type = Type::StopLookingForMarkers;
		}
	}
	void Set_StopLookingForMarkers(U2G_StopLookingForMarkers&&  new_StopLookingForMarkers)
	{
		if(this->_type == Type::StopLookingForMarkers) {
			_StopLookingForMarkers = std::move(new_StopLookingForMarkers);
		}
		else {
			ClearCurrent();
			new(&_StopLookingForMarkers) U2G_StopLookingForMarkers{std::move(new_StopLookingForMarkers)};
			_type = Type::StopLookingForMarkers;
		}
	}

	/** SetVisionSystemParams **/
	const U2G_SetVisionSystemParams& Get_SetVisionSystemParams() const
	{
		assert(_type == Type::SetVisionSystemParams);
		return _SetVisionSystemParams;
	}
	void Set_SetVisionSystemParams(const U2G_SetVisionSystemParams& new_SetVisionSystemParams)
	{
		if(this->_type == Type::SetVisionSystemParams) {
			_SetVisionSystemParams = new_SetVisionSystemParams;
		}
		else {
			ClearCurrent();
			new(&_SetVisionSystemParams) U2G_SetVisionSystemParams{new_SetVisionSystemParams};
			_type = Type::SetVisionSystemParams;
		}
	}
	void Set_SetVisionSystemParams(U2G_SetVisionSystemParams&&  new_SetVisionSystemParams)
	{
		if(this->_type == Type::SetVisionSystemParams) {
			_SetVisionSystemParams = std::move(new_SetVisionSystemParams);
		}
		else {
			ClearCurrent();
			new(&_SetVisionSystemParams) U2G_SetVisionSystemParams{std::move(new_SetVisionSystemParams)};
			_type = Type::SetVisionSystemParams;
		}
	}

	/** SetFaceDetectParams **/
	const U2G_SetFaceDetectParams& Get_SetFaceDetectParams() const
	{
		assert(_type == Type::SetFaceDetectParams);
		return _SetFaceDetectParams;
	}
	void Set_SetFaceDetectParams(const U2G_SetFaceDetectParams& new_SetFaceDetectParams)
	{
		if(this->_type == Type::SetFaceDetectParams) {
			_SetFaceDetectParams = new_SetFaceDetectParams;
		}
		else {
			ClearCurrent();
			new(&_SetFaceDetectParams) U2G_SetFaceDetectParams{new_SetFaceDetectParams};
			_type = Type::SetFaceDetectParams;
		}
	}
	void Set_SetFaceDetectParams(U2G_SetFaceDetectParams&&  new_SetFaceDetectParams)
	{
		if(this->_type == Type::SetFaceDetectParams) {
			_SetFaceDetectParams = std::move(new_SetFaceDetectParams);
		}
		else {
			ClearCurrent();
			new(&_SetFaceDetectParams) U2G_SetFaceDetectParams{std::move(new_SetFaceDetectParams)};
			_type = Type::SetFaceDetectParams;
		}
	}


	size_t Unpack(const uint8_t* buff, const size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		return Unpack(buffer);
	}
	size_t Unpack(const SafeMessageBuffer& buffer)
	{
		Type newType {Type::INVALID};
		const Type oldType {GetType()};
		buffer.Read(newType);
		if (newType != oldType) {
			ClearCurrent();
		}
		switch(newType) {
		case Type::Ping:
			if (newType != oldType) {
				new(&(this->_Ping)) U2G_Ping(buffer);
			}
			else {
			this->_Ping.Unpack(buffer);
			}
			break;
		case Type::ConnectToRobot:
			if (newType != oldType) {
				new(&(this->_ConnectToRobot)) U2G_ConnectToRobot(buffer);
			}
			else {
			this->_ConnectToRobot.Unpack(buffer);
			}
			break;
		case Type::ConnectToUiDevice:
			if (newType != oldType) {
				new(&(this->_ConnectToUiDevice)) U2G_ConnectToUiDevice(buffer);
			}
			else {
			this->_ConnectToUiDevice.Unpack(buffer);
			}
			break;
		case Type::DisconnectFromUiDevice:
			if (newType != oldType) {
				new(&(this->_DisconnectFromUiDevice)) U2G_DisconnectFromUiDevice(buffer);
			}
			else {
			this->_DisconnectFromUiDevice.Unpack(buffer);
			}
			break;
		case Type::ForceAddRobot:
			if (newType != oldType) {
				new(&(this->_ForceAddRobot)) U2G_ForceAddRobot(buffer);
			}
			else {
			this->_ForceAddRobot.Unpack(buffer);
			}
			break;
		case Type::StartEngine:
			if (newType != oldType) {
				new(&(this->_StartEngine)) U2G_StartEngine(buffer);
			}
			else {
			this->_StartEngine.Unpack(buffer);
			}
			break;
		case Type::DriveWheels:
			if (newType != oldType) {
				new(&(this->_DriveWheels)) U2G_DriveWheels(buffer);
			}
			else {
			this->_DriveWheels.Unpack(buffer);
			}
			break;
		case Type::TurnInPlace:
			if (newType != oldType) {
				new(&(this->_TurnInPlace)) U2G_TurnInPlace(buffer);
			}
			else {
			this->_TurnInPlace.Unpack(buffer);
			}
			break;
		case Type::MoveHead:
			if (newType != oldType) {
				new(&(this->_MoveHead)) U2G_MoveHead(buffer);
			}
			else {
			this->_MoveHead.Unpack(buffer);
			}
			break;
		case Type::MoveLift:
			if (newType != oldType) {
				new(&(this->_MoveLift)) U2G_MoveLift(buffer);
			}
			else {
			this->_MoveLift.Unpack(buffer);
			}
			break;
		case Type::SetLiftHeight:
			if (newType != oldType) {
				new(&(this->_SetLiftHeight)) U2G_SetLiftHeight(buffer);
			}
			else {
			this->_SetLiftHeight.Unpack(buffer);
			}
			break;
		case Type::SetHeadAngle:
			if (newType != oldType) {
				new(&(this->_SetHeadAngle)) U2G_SetHeadAngle(buffer);
			}
			else {
			this->_SetHeadAngle.Unpack(buffer);
			}
			break;
		case Type::StopAllMotors:
			if (newType != oldType) {
				new(&(this->_StopAllMotors)) U2G_StopAllMotors(buffer);
			}
			else {
			this->_StopAllMotors.Unpack(buffer);
			}
			break;
		case Type::ImageRequest:
			if (newType != oldType) {
				new(&(this->_ImageRequest)) U2G_ImageRequest(buffer);
			}
			else {
			this->_ImageRequest.Unpack(buffer);
			}
			break;
		case Type::SetRobotImageSendMode:
			if (newType != oldType) {
				new(&(this->_SetRobotImageSendMode)) U2G_SetRobotImageSendMode(buffer);
			}
			else {
			this->_SetRobotImageSendMode.Unpack(buffer);
			}
			break;
		case Type::SaveImages:
			if (newType != oldType) {
				new(&(this->_SaveImages)) U2G_SaveImages(buffer);
			}
			else {
			this->_SaveImages.Unpack(buffer);
			}
			break;
		case Type::EnableDisplay:
			if (newType != oldType) {
				new(&(this->_EnableDisplay)) U2G_EnableDisplay(buffer);
			}
			else {
			this->_EnableDisplay.Unpack(buffer);
			}
			break;
		case Type::SetHeadlights:
			if (newType != oldType) {
				new(&(this->_SetHeadlights)) U2G_SetHeadlights(buffer);
			}
			else {
			this->_SetHeadlights.Unpack(buffer);
			}
			break;
		case Type::GotoPose:
			if (newType != oldType) {
				new(&(this->_GotoPose)) U2G_GotoPose(buffer);
			}
			else {
			this->_GotoPose.Unpack(buffer);
			}
			break;
		case Type::PlaceObjectOnGround:
			if (newType != oldType) {
				new(&(this->_PlaceObjectOnGround)) U2G_PlaceObjectOnGround(buffer);
			}
			else {
			this->_PlaceObjectOnGround.Unpack(buffer);
			}
			break;
		case Type::PlaceObjectOnGroundHere:
			if (newType != oldType) {
				new(&(this->_PlaceObjectOnGroundHere)) U2G_PlaceObjectOnGroundHere(buffer);
			}
			else {
			this->_PlaceObjectOnGroundHere.Unpack(buffer);
			}
			break;
		case Type::ExecuteTestPlan:
			if (newType != oldType) {
				new(&(this->_ExecuteTestPlan)) U2G_ExecuteTestPlan(buffer);
			}
			else {
			this->_ExecuteTestPlan.Unpack(buffer);
			}
			break;
		case Type::SelectNextObject:
			if (newType != oldType) {
				new(&(this->_SelectNextObject)) U2G_SelectNextObject(buffer);
			}
			else {
			this->_SelectNextObject.Unpack(buffer);
			}
			break;
		case Type::PickAndPlaceObject:
			if (newType != oldType) {
				new(&(this->_PickAndPlaceObject)) U2G_PickAndPlaceObject(buffer);
			}
			else {
			this->_PickAndPlaceObject.Unpack(buffer);
			}
			break;
		case Type::TraverseObject:
			if (newType != oldType) {
				new(&(this->_TraverseObject)) U2G_TraverseObject(buffer);
			}
			else {
			this->_TraverseObject.Unpack(buffer);
			}
			break;
		case Type::ClearAllBlocks:
			if (newType != oldType) {
				new(&(this->_ClearAllBlocks)) U2G_ClearAllBlocks(buffer);
			}
			else {
			this->_ClearAllBlocks.Unpack(buffer);
			}
			break;
		case Type::ExecuteBehavior:
			if (newType != oldType) {
				new(&(this->_ExecuteBehavior)) U2G_ExecuteBehavior(buffer);
			}
			else {
			this->_ExecuteBehavior.Unpack(buffer);
			}
			break;
		case Type::SetBehaviorState:
			if (newType != oldType) {
				new(&(this->_SetBehaviorState)) U2G_SetBehaviorState(buffer);
			}
			else {
			this->_SetBehaviorState.Unpack(buffer);
			}
			break;
		case Type::AbortPath:
			if (newType != oldType) {
				new(&(this->_AbortPath)) U2G_AbortPath(buffer);
			}
			else {
			this->_AbortPath.Unpack(buffer);
			}
			break;
		case Type::AbortAll:
			if (newType != oldType) {
				new(&(this->_AbortAll)) U2G_AbortAll(buffer);
			}
			else {
			this->_AbortAll.Unpack(buffer);
			}
			break;
		case Type::DrawPoseMarker:
			if (newType != oldType) {
				new(&(this->_DrawPoseMarker)) U2G_DrawPoseMarker(buffer);
			}
			else {
			this->_DrawPoseMarker.Unpack(buffer);
			}
			break;
		case Type::ErasePoseMarker:
			if (newType != oldType) {
				new(&(this->_ErasePoseMarker)) U2G_ErasePoseMarker(buffer);
			}
			else {
			this->_ErasePoseMarker.Unpack(buffer);
			}
			break;
		case Type::SetHeadControllerGains:
			if (newType != oldType) {
				new(&(this->_SetHeadControllerGains)) U2G_SetHeadControllerGains(buffer);
			}
			else {
			this->_SetHeadControllerGains.Unpack(buffer);
			}
			break;
		case Type::SetLiftControllerGains:
			if (newType != oldType) {
				new(&(this->_SetLiftControllerGains)) U2G_SetLiftControllerGains(buffer);
			}
			else {
			this->_SetLiftControllerGains.Unpack(buffer);
			}
			break;
		case Type::SelectNextSoundScheme:
			if (newType != oldType) {
				new(&(this->_SelectNextSoundScheme)) U2G_SelectNextSoundScheme(buffer);
			}
			else {
			this->_SelectNextSoundScheme.Unpack(buffer);
			}
			break;
		case Type::StartTestMode:
			if (newType != oldType) {
				new(&(this->_StartTestMode)) U2G_StartTestMode(buffer);
			}
			else {
			this->_StartTestMode.Unpack(buffer);
			}
			break;
		case Type::IMURequest:
			if (newType != oldType) {
				new(&(this->_IMURequest)) U2G_IMURequest(buffer);
			}
			else {
			this->_IMURequest.Unpack(buffer);
			}
			break;
		case Type::PlayAnimation:
			if (newType != oldType) {
				new(&(this->_PlayAnimation)) U2G_PlayAnimation(buffer);
			}
			else {
			this->_PlayAnimation.Unpack(buffer);
			}
			break;
		case Type::ReadAnimationFile:
			if (newType != oldType) {
				new(&(this->_ReadAnimationFile)) U2G_ReadAnimationFile(buffer);
			}
			else {
			this->_ReadAnimationFile.Unpack(buffer);
			}
			break;
		case Type::StartFaceTracking:
			if (newType != oldType) {
				new(&(this->_StartFaceTracking)) U2G_StartFaceTracking(buffer);
			}
			else {
			this->_StartFaceTracking.Unpack(buffer);
			}
			break;
		case Type::StopFaceTracking:
			if (newType != oldType) {
				new(&(this->_StopFaceTracking)) U2G_StopFaceTracking(buffer);
			}
			else {
			this->_StopFaceTracking.Unpack(buffer);
			}
			break;
		case Type::StartLookingForMarkers:
			if (newType != oldType) {
				new(&(this->_StartLookingForMarkers)) U2G_StartLookingForMarkers(buffer);
			}
			else {
			this->_StartLookingForMarkers.Unpack(buffer);
			}
			break;
		case Type::StopLookingForMarkers:
			if (newType != oldType) {
				new(&(this->_StopLookingForMarkers)) U2G_StopLookingForMarkers(buffer);
			}
			else {
			this->_StopLookingForMarkers.Unpack(buffer);
			}
			break;
		case Type::SetVisionSystemParams:
			if (newType != oldType) {
				new(&(this->_SetVisionSystemParams)) U2G_SetVisionSystemParams(buffer);
			}
			else {
			this->_SetVisionSystemParams.Unpack(buffer);
			}
			break;
		case Type::SetFaceDetectParams:
			if (newType != oldType) {
				new(&(this->_SetFaceDetectParams)) U2G_SetFaceDetectParams(buffer);
			}
			else {
			this->_SetFaceDetectParams.Unpack(buffer);
			}
			break;
		default:
			break;
		}
		_type = newType;
		return buffer.GetBytesRead();
	}


	size_t Pack(uint8_t* buff, size_t len) const
	{
		SafeMessageBuffer buffer(buff, len, false);
		return Pack(buffer);
	}

	size_t Pack(SafeMessageBuffer& buffer) const
	{
		buffer.Write(_type);
		switch(GetType()) {
		case Type::Ping:
			this->_Ping.Pack(buffer);
			break;
		case Type::ConnectToRobot:
			this->_ConnectToRobot.Pack(buffer);
			break;
		case Type::ConnectToUiDevice:
			this->_ConnectToUiDevice.Pack(buffer);
			break;
		case Type::DisconnectFromUiDevice:
			this->_DisconnectFromUiDevice.Pack(buffer);
			break;
		case Type::ForceAddRobot:
			this->_ForceAddRobot.Pack(buffer);
			break;
		case Type::StartEngine:
			this->_StartEngine.Pack(buffer);
			break;
		case Type::DriveWheels:
			this->_DriveWheels.Pack(buffer);
			break;
		case Type::TurnInPlace:
			this->_TurnInPlace.Pack(buffer);
			break;
		case Type::MoveHead:
			this->_MoveHead.Pack(buffer);
			break;
		case Type::MoveLift:
			this->_MoveLift.Pack(buffer);
			break;
		case Type::SetLiftHeight:
			this->_SetLiftHeight.Pack(buffer);
			break;
		case Type::SetHeadAngle:
			this->_SetHeadAngle.Pack(buffer);
			break;
		case Type::StopAllMotors:
			this->_StopAllMotors.Pack(buffer);
			break;
		case Type::ImageRequest:
			this->_ImageRequest.Pack(buffer);
			break;
		case Type::SetRobotImageSendMode:
			this->_SetRobotImageSendMode.Pack(buffer);
			break;
		case Type::SaveImages:
			this->_SaveImages.Pack(buffer);
			break;
		case Type::EnableDisplay:
			this->_EnableDisplay.Pack(buffer);
			break;
		case Type::SetHeadlights:
			this->_SetHeadlights.Pack(buffer);
			break;
		case Type::GotoPose:
			this->_GotoPose.Pack(buffer);
			break;
		case Type::PlaceObjectOnGround:
			this->_PlaceObjectOnGround.Pack(buffer);
			break;
		case Type::PlaceObjectOnGroundHere:
			this->_PlaceObjectOnGroundHere.Pack(buffer);
			break;
		case Type::ExecuteTestPlan:
			this->_ExecuteTestPlan.Pack(buffer);
			break;
		case Type::SelectNextObject:
			this->_SelectNextObject.Pack(buffer);
			break;
		case Type::PickAndPlaceObject:
			this->_PickAndPlaceObject.Pack(buffer);
			break;
		case Type::TraverseObject:
			this->_TraverseObject.Pack(buffer);
			break;
		case Type::ClearAllBlocks:
			this->_ClearAllBlocks.Pack(buffer);
			break;
		case Type::ExecuteBehavior:
			this->_ExecuteBehavior.Pack(buffer);
			break;
		case Type::SetBehaviorState:
			this->_SetBehaviorState.Pack(buffer);
			break;
		case Type::AbortPath:
			this->_AbortPath.Pack(buffer);
			break;
		case Type::AbortAll:
			this->_AbortAll.Pack(buffer);
			break;
		case Type::DrawPoseMarker:
			this->_DrawPoseMarker.Pack(buffer);
			break;
		case Type::ErasePoseMarker:
			this->_ErasePoseMarker.Pack(buffer);
			break;
		case Type::SetHeadControllerGains:
			this->_SetHeadControllerGains.Pack(buffer);
			break;
		case Type::SetLiftControllerGains:
			this->_SetLiftControllerGains.Pack(buffer);
			break;
		case Type::SelectNextSoundScheme:
			this->_SelectNextSoundScheme.Pack(buffer);
			break;
		case Type::StartTestMode:
			this->_StartTestMode.Pack(buffer);
			break;
		case Type::IMURequest:
			this->_IMURequest.Pack(buffer);
			break;
		case Type::PlayAnimation:
			this->_PlayAnimation.Pack(buffer);
			break;
		case Type::ReadAnimationFile:
			this->_ReadAnimationFile.Pack(buffer);
			break;
		case Type::StartFaceTracking:
			this->_StartFaceTracking.Pack(buffer);
			break;
		case Type::StopFaceTracking:
			this->_StopFaceTracking.Pack(buffer);
			break;
		case Type::StartLookingForMarkers:
			this->_StartLookingForMarkers.Pack(buffer);
			break;
		case Type::StopLookingForMarkers:
			this->_StopLookingForMarkers.Pack(buffer);
			break;
		case Type::SetVisionSystemParams:
			this->_SetVisionSystemParams.Pack(buffer);
			break;
		case Type::SetFaceDetectParams:
			this->_SetFaceDetectParams.Pack(buffer);
			break;
		default:
			break;
		}
		const size_t bytesWritten {buffer.GetBytesWritten()};
		return bytesWritten;
	}


	size_t Size() const
	{
		size_t result {1}; // tag = uint_8
		switch(GetType()) {
		case Type::Ping:
			result += _Ping.Size();
			break;
		case Type::ConnectToRobot:
			result += _ConnectToRobot.Size();
			break;
		case Type::ConnectToUiDevice:
			result += _ConnectToUiDevice.Size();
			break;
		case Type::DisconnectFromUiDevice:
			result += _DisconnectFromUiDevice.Size();
			break;
		case Type::ForceAddRobot:
			result += _ForceAddRobot.Size();
			break;
		case Type::StartEngine:
			result += _StartEngine.Size();
			break;
		case Type::DriveWheels:
			result += _DriveWheels.Size();
			break;
		case Type::TurnInPlace:
			result += _TurnInPlace.Size();
			break;
		case Type::MoveHead:
			result += _MoveHead.Size();
			break;
		case Type::MoveLift:
			result += _MoveLift.Size();
			break;
		case Type::SetLiftHeight:
			result += _SetLiftHeight.Size();
			break;
		case Type::SetHeadAngle:
			result += _SetHeadAngle.Size();
			break;
		case Type::StopAllMotors:
			result += _StopAllMotors.Size();
			break;
		case Type::ImageRequest:
			result += _ImageRequest.Size();
			break;
		case Type::SetRobotImageSendMode:
			result += _SetRobotImageSendMode.Size();
			break;
		case Type::SaveImages:
			result += _SaveImages.Size();
			break;
		case Type::EnableDisplay:
			result += _EnableDisplay.Size();
			break;
		case Type::SetHeadlights:
			result += _SetHeadlights.Size();
			break;
		case Type::GotoPose:
			result += _GotoPose.Size();
			break;
		case Type::PlaceObjectOnGround:
			result += _PlaceObjectOnGround.Size();
			break;
		case Type::PlaceObjectOnGroundHere:
			result += _PlaceObjectOnGroundHere.Size();
			break;
		case Type::ExecuteTestPlan:
			result += _ExecuteTestPlan.Size();
			break;
		case Type::SelectNextObject:
			result += _SelectNextObject.Size();
			break;
		case Type::PickAndPlaceObject:
			result += _PickAndPlaceObject.Size();
			break;
		case Type::TraverseObject:
			result += _TraverseObject.Size();
			break;
		case Type::ClearAllBlocks:
			result += _ClearAllBlocks.Size();
			break;
		case Type::ExecuteBehavior:
			result += _ExecuteBehavior.Size();
			break;
		case Type::SetBehaviorState:
			result += _SetBehaviorState.Size();
			break;
		case Type::AbortPath:
			result += _AbortPath.Size();
			break;
		case Type::AbortAll:
			result += _AbortAll.Size();
			break;
		case Type::DrawPoseMarker:
			result += _DrawPoseMarker.Size();
			break;
		case Type::ErasePoseMarker:
			result += _ErasePoseMarker.Size();
			break;
		case Type::SetHeadControllerGains:
			result += _SetHeadControllerGains.Size();
			break;
		case Type::SetLiftControllerGains:
			result += _SetLiftControllerGains.Size();
			break;
		case Type::SelectNextSoundScheme:
			result += _SelectNextSoundScheme.Size();
			break;
		case Type::StartTestMode:
			result += _StartTestMode.Size();
			break;
		case Type::IMURequest:
			result += _IMURequest.Size();
			break;
		case Type::PlayAnimation:
			result += _PlayAnimation.Size();
			break;
		case Type::ReadAnimationFile:
			result += _ReadAnimationFile.Size();
			break;
		case Type::StartFaceTracking:
			result += _StartFaceTracking.Size();
			break;
		case Type::StopFaceTracking:
			result += _StopFaceTracking.Size();
			break;
		case Type::StartLookingForMarkers:
			result += _StartLookingForMarkers.Size();
			break;
		case Type::StopLookingForMarkers:
			result += _StopLookingForMarkers.Size();
			break;
		case Type::SetVisionSystemParams:
			result += _SetVisionSystemParams.Size();
			break;
		case Type::SetFaceDetectParams:
			result += _SetFaceDetectParams.Size();
			break;
		default:
			return 0;
		}
		return result;
	}
private:
	void ClearCurrent()
	{
		switch(GetType()) {
		case Type::Ping:
			_Ping.~U2G_Ping();
			break;
		case Type::ConnectToRobot:
			_ConnectToRobot.~U2G_ConnectToRobot();
			break;
		case Type::ConnectToUiDevice:
			_ConnectToUiDevice.~U2G_ConnectToUiDevice();
			break;
		case Type::DisconnectFromUiDevice:
			_DisconnectFromUiDevice.~U2G_DisconnectFromUiDevice();
			break;
		case Type::ForceAddRobot:
			_ForceAddRobot.~U2G_ForceAddRobot();
			break;
		case Type::StartEngine:
			_StartEngine.~U2G_StartEngine();
			break;
		case Type::DriveWheels:
			_DriveWheels.~U2G_DriveWheels();
			break;
		case Type::TurnInPlace:
			_TurnInPlace.~U2G_TurnInPlace();
			break;
		case Type::MoveHead:
			_MoveHead.~U2G_MoveHead();
			break;
		case Type::MoveLift:
			_MoveLift.~U2G_MoveLift();
			break;
		case Type::SetLiftHeight:
			_SetLiftHeight.~U2G_SetLiftHeight();
			break;
		case Type::SetHeadAngle:
			_SetHeadAngle.~U2G_SetHeadAngle();
			break;
		case Type::StopAllMotors:
			_StopAllMotors.~U2G_StopAllMotors();
			break;
		case Type::ImageRequest:
			_ImageRequest.~U2G_ImageRequest();
			break;
		case Type::SetRobotImageSendMode:
			_SetRobotImageSendMode.~U2G_SetRobotImageSendMode();
			break;
		case Type::SaveImages:
			_SaveImages.~U2G_SaveImages();
			break;
		case Type::EnableDisplay:
			_EnableDisplay.~U2G_EnableDisplay();
			break;
		case Type::SetHeadlights:
			_SetHeadlights.~U2G_SetHeadlights();
			break;
		case Type::GotoPose:
			_GotoPose.~U2G_GotoPose();
			break;
		case Type::PlaceObjectOnGround:
			_PlaceObjectOnGround.~U2G_PlaceObjectOnGround();
			break;
		case Type::PlaceObjectOnGroundHere:
			_PlaceObjectOnGroundHere.~U2G_PlaceObjectOnGroundHere();
			break;
		case Type::ExecuteTestPlan:
			_ExecuteTestPlan.~U2G_ExecuteTestPlan();
			break;
		case Type::SelectNextObject:
			_SelectNextObject.~U2G_SelectNextObject();
			break;
		case Type::PickAndPlaceObject:
			_PickAndPlaceObject.~U2G_PickAndPlaceObject();
			break;
		case Type::TraverseObject:
			_TraverseObject.~U2G_TraverseObject();
			break;
		case Type::ClearAllBlocks:
			_ClearAllBlocks.~U2G_ClearAllBlocks();
			break;
		case Type::ExecuteBehavior:
			_ExecuteBehavior.~U2G_ExecuteBehavior();
			break;
		case Type::SetBehaviorState:
			_SetBehaviorState.~U2G_SetBehaviorState();
			break;
		case Type::AbortPath:
			_AbortPath.~U2G_AbortPath();
			break;
		case Type::AbortAll:
			_AbortAll.~U2G_AbortAll();
			break;
		case Type::DrawPoseMarker:
			_DrawPoseMarker.~U2G_DrawPoseMarker();
			break;
		case Type::ErasePoseMarker:
			_ErasePoseMarker.~U2G_ErasePoseMarker();
			break;
		case Type::SetHeadControllerGains:
			_SetHeadControllerGains.~U2G_SetHeadControllerGains();
			break;
		case Type::SetLiftControllerGains:
			_SetLiftControllerGains.~U2G_SetLiftControllerGains();
			break;
		case Type::SelectNextSoundScheme:
			_SelectNextSoundScheme.~U2G_SelectNextSoundScheme();
			break;
		case Type::StartTestMode:
			_StartTestMode.~U2G_StartTestMode();
			break;
		case Type::IMURequest:
			_IMURequest.~U2G_IMURequest();
			break;
		case Type::PlayAnimation:
			_PlayAnimation.~U2G_PlayAnimation();
			break;
		case Type::ReadAnimationFile:
			_ReadAnimationFile.~U2G_ReadAnimationFile();
			break;
		case Type::StartFaceTracking:
			_StartFaceTracking.~U2G_StartFaceTracking();
			break;
		case Type::StopFaceTracking:
			_StopFaceTracking.~U2G_StopFaceTracking();
			break;
		case Type::StartLookingForMarkers:
			_StartLookingForMarkers.~U2G_StartLookingForMarkers();
			break;
		case Type::StopLookingForMarkers:
			_StopLookingForMarkers.~U2G_StopLookingForMarkers();
			break;
		case Type::SetVisionSystemParams:
			_SetVisionSystemParams.~U2G_SetVisionSystemParams();
			break;
		case Type::SetFaceDetectParams:
			_SetFaceDetectParams.~U2G_SetFaceDetectParams();
			break;
		default:
			break;
		}
		_type = Type::INVALID;
	}
	Type _type;

	union {
		U2G_Ping _Ping;
		U2G_ConnectToRobot _ConnectToRobot;
		U2G_ConnectToUiDevice _ConnectToUiDevice;
		U2G_DisconnectFromUiDevice _DisconnectFromUiDevice;
		U2G_ForceAddRobot _ForceAddRobot;
		U2G_StartEngine _StartEngine;
		U2G_DriveWheels _DriveWheels;
		U2G_TurnInPlace _TurnInPlace;
		U2G_MoveHead _MoveHead;
		U2G_MoveLift _MoveLift;
		U2G_SetLiftHeight _SetLiftHeight;
		U2G_SetHeadAngle _SetHeadAngle;
		U2G_StopAllMotors _StopAllMotors;
		U2G_ImageRequest _ImageRequest;
		U2G_SetRobotImageSendMode _SetRobotImageSendMode;
		U2G_SaveImages _SaveImages;
		U2G_EnableDisplay _EnableDisplay;
		U2G_SetHeadlights _SetHeadlights;
		U2G_GotoPose _GotoPose;
		U2G_PlaceObjectOnGround _PlaceObjectOnGround;
		U2G_PlaceObjectOnGroundHere _PlaceObjectOnGroundHere;
		U2G_ExecuteTestPlan _ExecuteTestPlan;
		U2G_SelectNextObject _SelectNextObject;
		U2G_PickAndPlaceObject _PickAndPlaceObject;
		U2G_TraverseObject _TraverseObject;
		U2G_ClearAllBlocks _ClearAllBlocks;
		U2G_ExecuteBehavior _ExecuteBehavior;
		U2G_SetBehaviorState _SetBehaviorState;
		U2G_AbortPath _AbortPath;
		U2G_AbortAll _AbortAll;
		U2G_DrawPoseMarker _DrawPoseMarker;
		U2G_ErasePoseMarker _ErasePoseMarker;
		U2G_SetHeadControllerGains _SetHeadControllerGains;
		U2G_SetLiftControllerGains _SetLiftControllerGains;
		U2G_SelectNextSoundScheme _SelectNextSoundScheme;
		U2G_StartTestMode _StartTestMode;
		U2G_IMURequest _IMURequest;
		U2G_PlayAnimation _PlayAnimation;
		U2G_ReadAnimationFile _ReadAnimationFile;
		U2G_StartFaceTracking _StartFaceTracking;
		U2G_StopFaceTracking _StopFaceTracking;
		U2G_StartLookingForMarkers _StartLookingForMarkers;
		U2G_StopLookingForMarkers _StopLookingForMarkers;
		U2G_SetVisionSystemParams _SetVisionSystemParams;
		U2G_SetFaceDetectParams _SetFaceDetectParams;
	};
};

}//namespace Anki
}//namespace Cozmo
