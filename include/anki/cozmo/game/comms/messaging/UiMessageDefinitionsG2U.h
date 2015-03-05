#include <array>
#include <cstdint>
#include <cassert>
#include <string>
#include <vector>

#include "SafeMessageBuffer.h"

namespace Anki
{
namespace Cozmo
{
using SafeMessageBuffer = Anki::Util::SafeMessageBuffer;
struct G2U_Ping
{
	uint32_t counter;

	/**** Constructors ****/
	G2U_Ping() = default;
	G2U_Ping(const G2U_Ping& other) = default;
	G2U_Ping(G2U_Ping& other) = default;
	G2U_Ping(G2U_Ping&& other) noexcept = default;
	G2U_Ping& operator=(const G2U_Ping& other) = default;
	G2U_Ping& operator=(G2U_Ping&& other) noexcept = default;

	explicit G2U_Ping(uint32_t counter)
	:counter(counter)
	{}
	explicit G2U_Ping(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit G2U_Ping(const SafeMessageBuffer& buffer)
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

	bool operator==(const G2U_Ping& other) const
	{
		if (counter != other.counter) {
			return false;
		}
		return true;
	}

	bool operator!=(const G2U_Ping& other) const
	{
		return !(operator==(other));
	}
};

struct G2U_RobotAvailable
{
	uint32_t robotID;

	/**** Constructors ****/
	G2U_RobotAvailable() = default;
	G2U_RobotAvailable(const G2U_RobotAvailable& other) = default;
	G2U_RobotAvailable(G2U_RobotAvailable& other) = default;
	G2U_RobotAvailable(G2U_RobotAvailable&& other) noexcept = default;
	G2U_RobotAvailable& operator=(const G2U_RobotAvailable& other) = default;
	G2U_RobotAvailable& operator=(G2U_RobotAvailable&& other) noexcept = default;

	explicit G2U_RobotAvailable(uint32_t robotID)
	:robotID(robotID)
	{}
	explicit G2U_RobotAvailable(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit G2U_RobotAvailable(const SafeMessageBuffer& buffer)
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
		result += 4; // = uint_32
		return result;
	}

	bool operator==(const G2U_RobotAvailable& other) const
	{
		if (robotID != other.robotID) {
			return false;
		}
		return true;
	}

	bool operator!=(const G2U_RobotAvailable& other) const
	{
		return !(operator==(other));
	}
};

struct G2U_UiDeviceAvailable
{
	uint32_t deviceID;

	/**** Constructors ****/
	G2U_UiDeviceAvailable() = default;
	G2U_UiDeviceAvailable(const G2U_UiDeviceAvailable& other) = default;
	G2U_UiDeviceAvailable(G2U_UiDeviceAvailable& other) = default;
	G2U_UiDeviceAvailable(G2U_UiDeviceAvailable&& other) noexcept = default;
	G2U_UiDeviceAvailable& operator=(const G2U_UiDeviceAvailable& other) = default;
	G2U_UiDeviceAvailable& operator=(G2U_UiDeviceAvailable&& other) noexcept = default;

	explicit G2U_UiDeviceAvailable(uint32_t deviceID)
	:deviceID(deviceID)
	{}
	explicit G2U_UiDeviceAvailable(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit G2U_UiDeviceAvailable(const SafeMessageBuffer& buffer)
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
		result += 4; // = uint_32
		return result;
	}

	bool operator==(const G2U_UiDeviceAvailable& other) const
	{
		if (deviceID != other.deviceID) {
			return false;
		}
		return true;
	}

	bool operator!=(const G2U_UiDeviceAvailable& other) const
	{
		return !(operator==(other));
	}
};

struct G2U_RobotConnected
{
	uint32_t robotID;
	uint8_t successful;

	/**** Constructors ****/
	G2U_RobotConnected() = default;
	G2U_RobotConnected(const G2U_RobotConnected& other) = default;
	G2U_RobotConnected(G2U_RobotConnected& other) = default;
	G2U_RobotConnected(G2U_RobotConnected&& other) noexcept = default;
	G2U_RobotConnected& operator=(const G2U_RobotConnected& other) = default;
	G2U_RobotConnected& operator=(G2U_RobotConnected&& other) noexcept = default;

	explicit G2U_RobotConnected(uint32_t robotID
		,uint8_t successful)
	:robotID(robotID)
	,successful(successful)
	{}
	explicit G2U_RobotConnected(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit G2U_RobotConnected(const SafeMessageBuffer& buffer)
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
		buffer.Write(this->successful);
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
		buffer.Read(this->successful);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//robotID
		result += 4; // = uint_32
		//successful
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const G2U_RobotConnected& other) const
	{
		if (robotID != other.robotID
		|| successful != other.successful) {
			return false;
		}
		return true;
	}

	bool operator!=(const G2U_RobotConnected& other) const
	{
		return !(operator==(other));
	}
};

struct G2U_RobotDisconnected
{
	uint32_t robotID;
	float timeSinceLastMsg_sec;

	/**** Constructors ****/
	G2U_RobotDisconnected() = default;
	G2U_RobotDisconnected(const G2U_RobotDisconnected& other) = default;
	G2U_RobotDisconnected(G2U_RobotDisconnected& other) = default;
	G2U_RobotDisconnected(G2U_RobotDisconnected&& other) noexcept = default;
	G2U_RobotDisconnected& operator=(const G2U_RobotDisconnected& other) = default;
	G2U_RobotDisconnected& operator=(G2U_RobotDisconnected&& other) noexcept = default;

	explicit G2U_RobotDisconnected(uint32_t robotID
		,float timeSinceLastMsg_sec)
	:robotID(robotID)
	,timeSinceLastMsg_sec(timeSinceLastMsg_sec)
	{}
	explicit G2U_RobotDisconnected(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit G2U_RobotDisconnected(const SafeMessageBuffer& buffer)
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
		buffer.Write(this->timeSinceLastMsg_sec);
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
		buffer.Read(this->timeSinceLastMsg_sec);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//robotID
		result += 4; // = uint_32
		//timeSinceLastMsg_sec
		result += 4; // = float_32
		return result;
	}

	bool operator==(const G2U_RobotDisconnected& other) const
	{
		if (robotID != other.robotID
		|| timeSinceLastMsg_sec != other.timeSinceLastMsg_sec) {
			return false;
		}
		return true;
	}

	bool operator!=(const G2U_RobotDisconnected& other) const
	{
		return !(operator==(other));
	}
};

struct G2U_UiDeviceConnected
{
	uint32_t deviceID;
	uint8_t successful;

	/**** Constructors ****/
	G2U_UiDeviceConnected() = default;
	G2U_UiDeviceConnected(const G2U_UiDeviceConnected& other) = default;
	G2U_UiDeviceConnected(G2U_UiDeviceConnected& other) = default;
	G2U_UiDeviceConnected(G2U_UiDeviceConnected&& other) noexcept = default;
	G2U_UiDeviceConnected& operator=(const G2U_UiDeviceConnected& other) = default;
	G2U_UiDeviceConnected& operator=(G2U_UiDeviceConnected&& other) noexcept = default;

	explicit G2U_UiDeviceConnected(uint32_t deviceID
		,uint8_t successful)
	:deviceID(deviceID)
	,successful(successful)
	{}
	explicit G2U_UiDeviceConnected(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit G2U_UiDeviceConnected(const SafeMessageBuffer& buffer)
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
		buffer.Write(this->successful);
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
		buffer.Read(this->successful);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//deviceID
		result += 4; // = uint_32
		//successful
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const G2U_UiDeviceConnected& other) const
	{
		if (deviceID != other.deviceID
		|| successful != other.successful) {
			return false;
		}
		return true;
	}

	bool operator!=(const G2U_UiDeviceConnected& other) const
	{
		return !(operator==(other));
	}
};

struct G2U_RobotState
{
	float pose_x;
	float pose_y;
	float pose_z;
	float poseAngle_rad;
	float leftWheelSpeed_mmps;
	float rightWheelSpeed_mmps;
	float headAngle_rad;
	float liftHeight_mm;
	float batteryVoltage;
	uint8_t status;
	uint8_t robotID;

	/**** Constructors ****/
	G2U_RobotState() = default;
	G2U_RobotState(const G2U_RobotState& other) = default;
	G2U_RobotState(G2U_RobotState& other) = default;
	G2U_RobotState(G2U_RobotState&& other) noexcept = default;
	G2U_RobotState& operator=(const G2U_RobotState& other) = default;
	G2U_RobotState& operator=(G2U_RobotState&& other) noexcept = default;

	explicit G2U_RobotState(float pose_x
		,float pose_y
		,float pose_z
		,float poseAngle_rad
		,float leftWheelSpeed_mmps
		,float rightWheelSpeed_mmps
		,float headAngle_rad
		,float liftHeight_mm
		,float batteryVoltage
		,uint8_t status
		,uint8_t robotID)
	:pose_x(pose_x)
	,pose_y(pose_y)
	,pose_z(pose_z)
	,poseAngle_rad(poseAngle_rad)
	,leftWheelSpeed_mmps(leftWheelSpeed_mmps)
	,rightWheelSpeed_mmps(rightWheelSpeed_mmps)
	,headAngle_rad(headAngle_rad)
	,liftHeight_mm(liftHeight_mm)
	,batteryVoltage(batteryVoltage)
	,status(status)
	,robotID(robotID)
	{}
	explicit G2U_RobotState(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit G2U_RobotState(const SafeMessageBuffer& buffer)
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
		buffer.Write(this->pose_x);
		buffer.Write(this->pose_y);
		buffer.Write(this->pose_z);
		buffer.Write(this->poseAngle_rad);
		buffer.Write(this->leftWheelSpeed_mmps);
		buffer.Write(this->rightWheelSpeed_mmps);
		buffer.Write(this->headAngle_rad);
		buffer.Write(this->liftHeight_mm);
		buffer.Write(this->batteryVoltage);
		buffer.Write(this->status);
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
		buffer.Read(this->pose_x);
		buffer.Read(this->pose_y);
		buffer.Read(this->pose_z);
		buffer.Read(this->poseAngle_rad);
		buffer.Read(this->leftWheelSpeed_mmps);
		buffer.Read(this->rightWheelSpeed_mmps);
		buffer.Read(this->headAngle_rad);
		buffer.Read(this->liftHeight_mm);
		buffer.Read(this->batteryVoltage);
		buffer.Read(this->status);
		buffer.Read(this->robotID);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//pose_x
		result += 4; // = float_32
		//pose_y
		result += 4; // = float_32
		//pose_z
		result += 4; // = float_32
		//poseAngle_rad
		result += 4; // = float_32
		//leftWheelSpeed_mmps
		result += 4; // = float_32
		//rightWheelSpeed_mmps
		result += 4; // = float_32
		//headAngle_rad
		result += 4; // = float_32
		//liftHeight_mm
		result += 4; // = float_32
		//batteryVoltage
		result += 4; // = float_32
		//status
		result += 1; // = uint_8
		//robotID
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const G2U_RobotState& other) const
	{
		if (pose_x != other.pose_x
		|| pose_y != other.pose_y
		|| pose_z != other.pose_z
		|| poseAngle_rad != other.poseAngle_rad
		|| leftWheelSpeed_mmps != other.leftWheelSpeed_mmps
		|| rightWheelSpeed_mmps != other.rightWheelSpeed_mmps
		|| headAngle_rad != other.headAngle_rad
		|| liftHeight_mm != other.liftHeight_mm
		|| batteryVoltage != other.batteryVoltage
		|| status != other.status
		|| robotID != other.robotID) {
			return false;
		}
		return true;
	}

	bool operator!=(const G2U_RobotState& other) const
	{
		return !(operator==(other));
	}
};

struct G2U_ImageChunk
{
	uint32_t imageId;
	uint32_t frameTimeStamp;
	uint16_t nrows;
	uint16_t ncols;
	uint16_t chunkSize;
	uint8_t imageEncoding;
	uint8_t imageChunkCount;
	uint8_t chunkId;
	std::array<uint8_t, 1024> data;

	/**** Constructors ****/
	G2U_ImageChunk() = default;
	G2U_ImageChunk(const G2U_ImageChunk& other) = default;
	G2U_ImageChunk(G2U_ImageChunk& other) = default;
	G2U_ImageChunk(G2U_ImageChunk&& other) noexcept = default;
	G2U_ImageChunk& operator=(const G2U_ImageChunk& other) = default;
	G2U_ImageChunk& operator=(G2U_ImageChunk&& other) noexcept = default;

	explicit G2U_ImageChunk(uint32_t imageId
		,uint32_t frameTimeStamp
		,uint16_t nrows
		,uint16_t ncols
		,uint16_t chunkSize
		,uint8_t imageEncoding
		,uint8_t imageChunkCount
		,uint8_t chunkId
		,const std::array<uint8_t, 1024>& data)
	:imageId(imageId)
	,frameTimeStamp(frameTimeStamp)
	,nrows(nrows)
	,ncols(ncols)
	,chunkSize(chunkSize)
	,imageEncoding(imageEncoding)
	,imageChunkCount(imageChunkCount)
	,chunkId(chunkId)
	,data(data)
	{}
	explicit G2U_ImageChunk(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit G2U_ImageChunk(const SafeMessageBuffer& buffer)
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
		buffer.Write(this->imageId);
		buffer.Write(this->frameTimeStamp);
		buffer.Write(this->nrows);
		buffer.Write(this->ncols);
		buffer.Write(this->chunkSize);
		buffer.Write(this->imageEncoding);
		buffer.Write(this->imageChunkCount);
		buffer.Write(this->chunkId);
		buffer.WriteFArray<uint8_t, 1024>(this->data);
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
		buffer.Read(this->imageId);
		buffer.Read(this->frameTimeStamp);
		buffer.Read(this->nrows);
		buffer.Read(this->ncols);
		buffer.Read(this->chunkSize);
		buffer.Read(this->imageEncoding);
		buffer.Read(this->imageChunkCount);
		buffer.Read(this->chunkId);
		buffer.ReadFArray<uint8_t, 1024>(this->data);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//imageId
		result += 4; // = uint_32
		//frameTimeStamp
		result += 4; // = uint_32
		//nrows
		result += 2; // = uint_16
		//ncols
		result += 2; // = uint_16
		//chunkSize
		result += 2; // = uint_16
		//imageEncoding
		result += 1; // = uint_8
		//imageChunkCount
		result += 1; // = uint_8
		//chunkId
		result += 1; // = uint_8
		//data
		result += 1 * 1024; // = uint_8 * 1024
		return result;
	}

	bool operator==(const G2U_ImageChunk& other) const
	{
		if (imageId != other.imageId
		|| frameTimeStamp != other.frameTimeStamp
		|| nrows != other.nrows
		|| ncols != other.ncols
		|| chunkSize != other.chunkSize
		|| imageEncoding != other.imageEncoding
		|| imageChunkCount != other.imageChunkCount
		|| chunkId != other.chunkId
		|| data != other.data) {
			return false;
		}
		return true;
	}

	bool operator!=(const G2U_ImageChunk& other) const
	{
		return !(operator==(other));
	}
};

struct G2U_RobotObservedObject
{
	uint32_t robotID;
	uint32_t objectFamily;
	uint32_t objectType;
	int32_t objectID;
	float topLeft_x;
	float topLeft_y;
	float width;
	float height;

	/**** Constructors ****/
	G2U_RobotObservedObject() = default;
	G2U_RobotObservedObject(const G2U_RobotObservedObject& other) = default;
	G2U_RobotObservedObject(G2U_RobotObservedObject& other) = default;
	G2U_RobotObservedObject(G2U_RobotObservedObject&& other) noexcept = default;
	G2U_RobotObservedObject& operator=(const G2U_RobotObservedObject& other) = default;
	G2U_RobotObservedObject& operator=(G2U_RobotObservedObject&& other) noexcept = default;

	explicit G2U_RobotObservedObject(uint32_t robotID
		,uint32_t objectFamily
		,uint32_t objectType
		,int32_t objectID
		,float topLeft_x
		,float topLeft_y
		,float width
		,float height)
	:robotID(robotID)
	,objectFamily(objectFamily)
	,objectType(objectType)
	,objectID(objectID)
	,topLeft_x(topLeft_x)
	,topLeft_y(topLeft_y)
	,width(width)
	,height(height)
	{}
	explicit G2U_RobotObservedObject(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit G2U_RobotObservedObject(const SafeMessageBuffer& buffer)
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
		buffer.Write(this->objectFamily);
		buffer.Write(this->objectType);
		buffer.Write(this->objectID);
		buffer.Write(this->topLeft_x);
		buffer.Write(this->topLeft_y);
		buffer.Write(this->width);
		buffer.Write(this->height);
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
		buffer.Read(this->objectFamily);
		buffer.Read(this->objectType);
		buffer.Read(this->objectID);
		buffer.Read(this->topLeft_x);
		buffer.Read(this->topLeft_y);
		buffer.Read(this->width);
		buffer.Read(this->height);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//robotID
		result += 4; // = uint_32
		//objectFamily
		result += 4; // = uint_32
		//objectType
		result += 4; // = uint_32
		//objectID
		result += 4; // = int_32
		//topLeft_x
		result += 4; // = float_32
		//topLeft_y
		result += 4; // = float_32
		//width
		result += 4; // = float_32
		//height
		result += 4; // = float_32
		return result;
	}

	bool operator==(const G2U_RobotObservedObject& other) const
	{
		if (robotID != other.robotID
		|| objectFamily != other.objectFamily
		|| objectType != other.objectType
		|| objectID != other.objectID
		|| topLeft_x != other.topLeft_x
		|| topLeft_y != other.topLeft_y
		|| width != other.width
		|| height != other.height) {
			return false;
		}
		return true;
	}

	bool operator!=(const G2U_RobotObservedObject& other) const
	{
		return !(operator==(other));
	}
};

struct G2U_RobotObservedNothing
{
	uint32_t robotID;

	/**** Constructors ****/
	G2U_RobotObservedNothing() = default;
	G2U_RobotObservedNothing(const G2U_RobotObservedNothing& other) = default;
	G2U_RobotObservedNothing(G2U_RobotObservedNothing& other) = default;
	G2U_RobotObservedNothing(G2U_RobotObservedNothing&& other) noexcept = default;
	G2U_RobotObservedNothing& operator=(const G2U_RobotObservedNothing& other) = default;
	G2U_RobotObservedNothing& operator=(G2U_RobotObservedNothing&& other) noexcept = default;

	explicit G2U_RobotObservedNothing(uint32_t robotID)
	:robotID(robotID)
	{}
	explicit G2U_RobotObservedNothing(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit G2U_RobotObservedNothing(const SafeMessageBuffer& buffer)
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
		result += 4; // = uint_32
		return result;
	}

	bool operator==(const G2U_RobotObservedNothing& other) const
	{
		if (robotID != other.robotID) {
			return false;
		}
		return true;
	}

	bool operator!=(const G2U_RobotObservedNothing& other) const
	{
		return !(operator==(other));
	}
};

struct G2U_DeviceDetectedVisionMarker
{
	uint32_t markerType;
	float x_upperLeft;
	float y_upperLeft;
	float x_lowerLeft;
	float y_lowerLeft;
	float x_upperRight;
	float y_upperRight;
	float x_lowerRight;
	float y_lowerRight;

	/**** Constructors ****/
	G2U_DeviceDetectedVisionMarker() = default;
	G2U_DeviceDetectedVisionMarker(const G2U_DeviceDetectedVisionMarker& other) = default;
	G2U_DeviceDetectedVisionMarker(G2U_DeviceDetectedVisionMarker& other) = default;
	G2U_DeviceDetectedVisionMarker(G2U_DeviceDetectedVisionMarker&& other) noexcept = default;
	G2U_DeviceDetectedVisionMarker& operator=(const G2U_DeviceDetectedVisionMarker& other) = default;
	G2U_DeviceDetectedVisionMarker& operator=(G2U_DeviceDetectedVisionMarker&& other) noexcept = default;

	explicit G2U_DeviceDetectedVisionMarker(uint32_t markerType
		,float x_upperLeft
		,float y_upperLeft
		,float x_lowerLeft
		,float y_lowerLeft
		,float x_upperRight
		,float y_upperRight
		,float x_lowerRight
		,float y_lowerRight)
	:markerType(markerType)
	,x_upperLeft(x_upperLeft)
	,y_upperLeft(y_upperLeft)
	,x_lowerLeft(x_lowerLeft)
	,y_lowerLeft(y_lowerLeft)
	,x_upperRight(x_upperRight)
	,y_upperRight(y_upperRight)
	,x_lowerRight(x_lowerRight)
	,y_lowerRight(y_lowerRight)
	{}
	explicit G2U_DeviceDetectedVisionMarker(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit G2U_DeviceDetectedVisionMarker(const SafeMessageBuffer& buffer)
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
		buffer.Write(this->markerType);
		buffer.Write(this->x_upperLeft);
		buffer.Write(this->y_upperLeft);
		buffer.Write(this->x_lowerLeft);
		buffer.Write(this->y_lowerLeft);
		buffer.Write(this->x_upperRight);
		buffer.Write(this->y_upperRight);
		buffer.Write(this->x_lowerRight);
		buffer.Write(this->y_lowerRight);
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
		buffer.Read(this->markerType);
		buffer.Read(this->x_upperLeft);
		buffer.Read(this->y_upperLeft);
		buffer.Read(this->x_lowerLeft);
		buffer.Read(this->y_lowerLeft);
		buffer.Read(this->x_upperRight);
		buffer.Read(this->y_upperRight);
		buffer.Read(this->x_lowerRight);
		buffer.Read(this->y_lowerRight);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//markerType
		result += 4; // = uint_32
		//x_upperLeft
		result += 4; // = float_32
		//y_upperLeft
		result += 4; // = float_32
		//x_lowerLeft
		result += 4; // = float_32
		//y_lowerLeft
		result += 4; // = float_32
		//x_upperRight
		result += 4; // = float_32
		//y_upperRight
		result += 4; // = float_32
		//x_lowerRight
		result += 4; // = float_32
		//y_lowerRight
		result += 4; // = float_32
		return result;
	}

	bool operator==(const G2U_DeviceDetectedVisionMarker& other) const
	{
		if (markerType != other.markerType
		|| x_upperLeft != other.x_upperLeft
		|| y_upperLeft != other.y_upperLeft
		|| x_lowerLeft != other.x_lowerLeft
		|| y_lowerLeft != other.y_lowerLeft
		|| x_upperRight != other.x_upperRight
		|| y_upperRight != other.y_upperRight
		|| x_lowerRight != other.x_lowerRight
		|| y_lowerRight != other.y_lowerRight) {
			return false;
		}
		return true;
	}

	bool operator!=(const G2U_DeviceDetectedVisionMarker& other) const
	{
		return !(operator==(other));
	}
};

struct G2U_RobotCompletedPickAndPlaceAction
{
	uint32_t robotID;
	uint8_t success;

	/**** Constructors ****/
	G2U_RobotCompletedPickAndPlaceAction() = default;
	G2U_RobotCompletedPickAndPlaceAction(const G2U_RobotCompletedPickAndPlaceAction& other) = default;
	G2U_RobotCompletedPickAndPlaceAction(G2U_RobotCompletedPickAndPlaceAction& other) = default;
	G2U_RobotCompletedPickAndPlaceAction(G2U_RobotCompletedPickAndPlaceAction&& other) noexcept = default;
	G2U_RobotCompletedPickAndPlaceAction& operator=(const G2U_RobotCompletedPickAndPlaceAction& other) = default;
	G2U_RobotCompletedPickAndPlaceAction& operator=(G2U_RobotCompletedPickAndPlaceAction&& other) noexcept = default;

	explicit G2U_RobotCompletedPickAndPlaceAction(uint32_t robotID
		,uint8_t success)
	:robotID(robotID)
	,success(success)
	{}
	explicit G2U_RobotCompletedPickAndPlaceAction(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit G2U_RobotCompletedPickAndPlaceAction(const SafeMessageBuffer& buffer)
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
		buffer.Write(this->success);
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
		buffer.Read(this->success);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//robotID
		result += 4; // = uint_32
		//success
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const G2U_RobotCompletedPickAndPlaceAction& other) const
	{
		if (robotID != other.robotID
		|| success != other.success) {
			return false;
		}
		return true;
	}

	bool operator!=(const G2U_RobotCompletedPickAndPlaceAction& other) const
	{
		return !(operator==(other));
	}
};

struct G2U_RobotCompletedPlaceObjectOnGroundAction
{
	uint32_t robotID;
	uint8_t success;

	/**** Constructors ****/
	G2U_RobotCompletedPlaceObjectOnGroundAction() = default;
	G2U_RobotCompletedPlaceObjectOnGroundAction(const G2U_RobotCompletedPlaceObjectOnGroundAction& other) = default;
	G2U_RobotCompletedPlaceObjectOnGroundAction(G2U_RobotCompletedPlaceObjectOnGroundAction& other) = default;
	G2U_RobotCompletedPlaceObjectOnGroundAction(G2U_RobotCompletedPlaceObjectOnGroundAction&& other) noexcept = default;
	G2U_RobotCompletedPlaceObjectOnGroundAction& operator=(const G2U_RobotCompletedPlaceObjectOnGroundAction& other) = default;
	G2U_RobotCompletedPlaceObjectOnGroundAction& operator=(G2U_RobotCompletedPlaceObjectOnGroundAction&& other) noexcept = default;

	explicit G2U_RobotCompletedPlaceObjectOnGroundAction(uint32_t robotID
		,uint8_t success)
	:robotID(robotID)
	,success(success)
	{}
	explicit G2U_RobotCompletedPlaceObjectOnGroundAction(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit G2U_RobotCompletedPlaceObjectOnGroundAction(const SafeMessageBuffer& buffer)
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
		buffer.Write(this->success);
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
		buffer.Read(this->success);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//robotID
		result += 4; // = uint_32
		//success
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const G2U_RobotCompletedPlaceObjectOnGroundAction& other) const
	{
		if (robotID != other.robotID
		|| success != other.success) {
			return false;
		}
		return true;
	}

	bool operator!=(const G2U_RobotCompletedPlaceObjectOnGroundAction& other) const
	{
		return !(operator==(other));
	}
};

struct G2U_PlaySound
{
	std::string soundFilename;
	uint8_t numLoops;
	uint8_t volume;

	/**** Constructors ****/
	G2U_PlaySound() = default;
	G2U_PlaySound(const G2U_PlaySound& other) = default;
	G2U_PlaySound(G2U_PlaySound& other) = default;
	G2U_PlaySound(G2U_PlaySound&& other) noexcept = default;
	G2U_PlaySound& operator=(const G2U_PlaySound& other) = default;
	G2U_PlaySound& operator=(G2U_PlaySound&& other) noexcept = default;

	explicit G2U_PlaySound(const std::string& soundFilename
		,uint8_t numLoops
		,uint8_t volume)
	:soundFilename(soundFilename)
	,numLoops(numLoops)
	,volume(volume)
	{}
	explicit G2U_PlaySound(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit G2U_PlaySound(const SafeMessageBuffer& buffer)
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
		buffer.WritePString<uint8_t>(this->soundFilename);
		buffer.Write(this->numLoops);
		buffer.Write(this->volume);
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
		buffer.ReadPString<uint8_t>(this->soundFilename);
		buffer.Read(this->numLoops);
		buffer.Read(this->volume);
		return buffer.GetBytesRead();
	}
	size_t Size() const 
	{
		size_t result{0};
		//soundFilename
		result += 1; // length = uint_8
		result += 1 * soundFilename.size(); //string
		//numLoops
		result += 1; // = uint_8
		//volume
		result += 1; // = uint_8
		return result;
	}

	bool operator==(const G2U_PlaySound& other) const
	{
		if (soundFilename != other.soundFilename
		|| numLoops != other.numLoops
		|| volume != other.volume) {
			return false;
		}
		return true;
	}

	bool operator!=(const G2U_PlaySound& other) const
	{
		return !(operator==(other));
	}
};

struct G2U_StopSound
{

	/**** Constructors ****/
	G2U_StopSound() = default;
	G2U_StopSound(const G2U_StopSound& other) = default;
	G2U_StopSound(G2U_StopSound& other) = default;
	G2U_StopSound(G2U_StopSound&& other) noexcept = default;
	G2U_StopSound& operator=(const G2U_StopSound& other) = default;
	G2U_StopSound& operator=(G2U_StopSound&& other) noexcept = default;

	explicit G2U_StopSound(const uint8_t* buff, size_t len)
	{
		const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
		Unpack(buffer);
	}

	explicit G2U_StopSound(const SafeMessageBuffer& buffer)
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

	bool operator==(const G2U_StopSound& other) const
	{
		return true;
	}

	bool operator!=(const G2U_StopSound& other) const
	{
		return !(operator==(other));
	}
};

class G2U_Message
{
public:
	/**** Constructors ****/
	G2U_Message() :_type(Type::INVALID) { }
	explicit G2U_Message(const SafeMessageBuffer& buff) :_type(Type::INVALID)
	{
		Unpack(buff);
	}
	explicit G2U_Message(const uint8_t* buffer, size_t length) :_type(Type::INVALID)
	{
		SafeMessageBuffer buff(const_cast<uint8_t*>(buffer), length);
		Unpack(buff);
	}

	~G2U_Message() { ClearCurrent(); }
	enum class Type : uint8_t {
		Ping,	// 0
		RobotAvailable,	// 1
		UiDeviceAvailable,	// 2
		RobotConnected,	// 3
		RobotDisconnected,	// 4
		UiDeviceConnected,	// 5
		RobotState,	// 6
		ImageChunk,	// 7
		RobotObservedObject,	// 8
		RobotObservedNothing,	// 9
		DeviceDetectedVisionMarker,	// 10
		RobotCompletedPickAndPlaceAction,	// 11
		RobotCompletedPlaceObjectOnGroundAction,	// 12
		PlaySound,	// 13
		StopSound,	// 14
		INVALID
	};
	static const char* GetTypeName(Type type) {
		switch (type) {
		case Type::Ping:
			return "Ping";
		case Type::RobotAvailable:
			return "RobotAvailable";
		case Type::UiDeviceAvailable:
			return "UiDeviceAvailable";
		case Type::RobotConnected:
			return "RobotConnected";
		case Type::RobotDisconnected:
			return "RobotDisconnected";
		case Type::UiDeviceConnected:
			return "UiDeviceConnected";
		case Type::RobotState:
			return "RobotState";
		case Type::ImageChunk:
			return "ImageChunk";
		case Type::RobotObservedObject:
			return "RobotObservedObject";
		case Type::RobotObservedNothing:
			return "RobotObservedNothing";
		case Type::DeviceDetectedVisionMarker:
			return "DeviceDetectedVisionMarker";
		case Type::RobotCompletedPickAndPlaceAction:
			return "RobotCompletedPickAndPlaceAction";
		case Type::RobotCompletedPlaceObjectOnGroundAction:
			return "RobotCompletedPlaceObjectOnGroundAction";
		case Type::PlaySound:
			return "PlaySound";
		case Type::StopSound:
			return "StopSound";
		default:
			return "INVALID";
		}
	}
	Type GetType() const { return _type; }

	/** Ping **/
	const G2U_Ping& Get_Ping() const
	{
		assert(_type == Type::Ping);
		return _Ping;
	}
	void Set_Ping(const G2U_Ping& new_Ping)
	{
		if(this->_type == Type::Ping) {
			_Ping = new_Ping;
		}
		else {
			ClearCurrent();
			new(&_Ping) G2U_Ping{new_Ping};
			_type = Type::Ping;
		}
	}
	void Set_Ping(G2U_Ping&&  new_Ping)
	{
		if(this->_type == Type::Ping) {
			_Ping = std::move(new_Ping);
		}
		else {
			ClearCurrent();
			new(&_Ping) G2U_Ping{std::move(new_Ping)};
			_type = Type::Ping;
		}
	}

	/** RobotAvailable **/
	const G2U_RobotAvailable& Get_RobotAvailable() const
	{
		assert(_type == Type::RobotAvailable);
		return _RobotAvailable;
	}
	void Set_RobotAvailable(const G2U_RobotAvailable& new_RobotAvailable)
	{
		if(this->_type == Type::RobotAvailable) {
			_RobotAvailable = new_RobotAvailable;
		}
		else {
			ClearCurrent();
			new(&_RobotAvailable) G2U_RobotAvailable{new_RobotAvailable};
			_type = Type::RobotAvailable;
		}
	}
	void Set_RobotAvailable(G2U_RobotAvailable&&  new_RobotAvailable)
	{
		if(this->_type == Type::RobotAvailable) {
			_RobotAvailable = std::move(new_RobotAvailable);
		}
		else {
			ClearCurrent();
			new(&_RobotAvailable) G2U_RobotAvailable{std::move(new_RobotAvailable)};
			_type = Type::RobotAvailable;
		}
	}

	/** UiDeviceAvailable **/
	const G2U_UiDeviceAvailable& Get_UiDeviceAvailable() const
	{
		assert(_type == Type::UiDeviceAvailable);
		return _UiDeviceAvailable;
	}
	void Set_UiDeviceAvailable(const G2U_UiDeviceAvailable& new_UiDeviceAvailable)
	{
		if(this->_type == Type::UiDeviceAvailable) {
			_UiDeviceAvailable = new_UiDeviceAvailable;
		}
		else {
			ClearCurrent();
			new(&_UiDeviceAvailable) G2U_UiDeviceAvailable{new_UiDeviceAvailable};
			_type = Type::UiDeviceAvailable;
		}
	}
	void Set_UiDeviceAvailable(G2U_UiDeviceAvailable&&  new_UiDeviceAvailable)
	{
		if(this->_type == Type::UiDeviceAvailable) {
			_UiDeviceAvailable = std::move(new_UiDeviceAvailable);
		}
		else {
			ClearCurrent();
			new(&_UiDeviceAvailable) G2U_UiDeviceAvailable{std::move(new_UiDeviceAvailable)};
			_type = Type::UiDeviceAvailable;
		}
	}

	/** RobotConnected **/
	const G2U_RobotConnected& Get_RobotConnected() const
	{
		assert(_type == Type::RobotConnected);
		return _RobotConnected;
	}
	void Set_RobotConnected(const G2U_RobotConnected& new_RobotConnected)
	{
		if(this->_type == Type::RobotConnected) {
			_RobotConnected = new_RobotConnected;
		}
		else {
			ClearCurrent();
			new(&_RobotConnected) G2U_RobotConnected{new_RobotConnected};
			_type = Type::RobotConnected;
		}
	}
	void Set_RobotConnected(G2U_RobotConnected&&  new_RobotConnected)
	{
		if(this->_type == Type::RobotConnected) {
			_RobotConnected = std::move(new_RobotConnected);
		}
		else {
			ClearCurrent();
			new(&_RobotConnected) G2U_RobotConnected{std::move(new_RobotConnected)};
			_type = Type::RobotConnected;
		}
	}

	/** RobotDisconnected **/
	const G2U_RobotDisconnected& Get_RobotDisconnected() const
	{
		assert(_type == Type::RobotDisconnected);
		return _RobotDisconnected;
	}
	void Set_RobotDisconnected(const G2U_RobotDisconnected& new_RobotDisconnected)
	{
		if(this->_type == Type::RobotDisconnected) {
			_RobotDisconnected = new_RobotDisconnected;
		}
		else {
			ClearCurrent();
			new(&_RobotDisconnected) G2U_RobotDisconnected{new_RobotDisconnected};
			_type = Type::RobotDisconnected;
		}
	}
	void Set_RobotDisconnected(G2U_RobotDisconnected&&  new_RobotDisconnected)
	{
		if(this->_type == Type::RobotDisconnected) {
			_RobotDisconnected = std::move(new_RobotDisconnected);
		}
		else {
			ClearCurrent();
			new(&_RobotDisconnected) G2U_RobotDisconnected{std::move(new_RobotDisconnected)};
			_type = Type::RobotDisconnected;
		}
	}

	/** UiDeviceConnected **/
	const G2U_UiDeviceConnected& Get_UiDeviceConnected() const
	{
		assert(_type == Type::UiDeviceConnected);
		return _UiDeviceConnected;
	}
	void Set_UiDeviceConnected(const G2U_UiDeviceConnected& new_UiDeviceConnected)
	{
		if(this->_type == Type::UiDeviceConnected) {
			_UiDeviceConnected = new_UiDeviceConnected;
		}
		else {
			ClearCurrent();
			new(&_UiDeviceConnected) G2U_UiDeviceConnected{new_UiDeviceConnected};
			_type = Type::UiDeviceConnected;
		}
	}
	void Set_UiDeviceConnected(G2U_UiDeviceConnected&&  new_UiDeviceConnected)
	{
		if(this->_type == Type::UiDeviceConnected) {
			_UiDeviceConnected = std::move(new_UiDeviceConnected);
		}
		else {
			ClearCurrent();
			new(&_UiDeviceConnected) G2U_UiDeviceConnected{std::move(new_UiDeviceConnected)};
			_type = Type::UiDeviceConnected;
		}
	}

	/** RobotState **/
	const G2U_RobotState& Get_RobotState() const
	{
		assert(_type == Type::RobotState);
		return _RobotState;
	}
	void Set_RobotState(const G2U_RobotState& new_RobotState)
	{
		if(this->_type == Type::RobotState) {
			_RobotState = new_RobotState;
		}
		else {
			ClearCurrent();
			new(&_RobotState) G2U_RobotState{new_RobotState};
			_type = Type::RobotState;
		}
	}
	void Set_RobotState(G2U_RobotState&&  new_RobotState)
	{
		if(this->_type == Type::RobotState) {
			_RobotState = std::move(new_RobotState);
		}
		else {
			ClearCurrent();
			new(&_RobotState) G2U_RobotState{std::move(new_RobotState)};
			_type = Type::RobotState;
		}
	}

	/** ImageChunk **/
	const G2U_ImageChunk& Get_ImageChunk() const
	{
		assert(_type == Type::ImageChunk);
		return _ImageChunk;
	}
	void Set_ImageChunk(const G2U_ImageChunk& new_ImageChunk)
	{
		if(this->_type == Type::ImageChunk) {
			_ImageChunk = new_ImageChunk;
		}
		else {
			ClearCurrent();
			new(&_ImageChunk) G2U_ImageChunk{new_ImageChunk};
			_type = Type::ImageChunk;
		}
	}
	void Set_ImageChunk(G2U_ImageChunk&&  new_ImageChunk)
	{
		if(this->_type == Type::ImageChunk) {
			_ImageChunk = std::move(new_ImageChunk);
		}
		else {
			ClearCurrent();
			new(&_ImageChunk) G2U_ImageChunk{std::move(new_ImageChunk)};
			_type = Type::ImageChunk;
		}
	}

	/** RobotObservedObject **/
	const G2U_RobotObservedObject& Get_RobotObservedObject() const
	{
		assert(_type == Type::RobotObservedObject);
		return _RobotObservedObject;
	}
	void Set_RobotObservedObject(const G2U_RobotObservedObject& new_RobotObservedObject)
	{
		if(this->_type == Type::RobotObservedObject) {
			_RobotObservedObject = new_RobotObservedObject;
		}
		else {
			ClearCurrent();
			new(&_RobotObservedObject) G2U_RobotObservedObject{new_RobotObservedObject};
			_type = Type::RobotObservedObject;
		}
	}
	void Set_RobotObservedObject(G2U_RobotObservedObject&&  new_RobotObservedObject)
	{
		if(this->_type == Type::RobotObservedObject) {
			_RobotObservedObject = std::move(new_RobotObservedObject);
		}
		else {
			ClearCurrent();
			new(&_RobotObservedObject) G2U_RobotObservedObject{std::move(new_RobotObservedObject)};
			_type = Type::RobotObservedObject;
		}
	}

	/** RobotObservedNothing **/
	const G2U_RobotObservedNothing& Get_RobotObservedNothing() const
	{
		assert(_type == Type::RobotObservedNothing);
		return _RobotObservedNothing;
	}
	void Set_RobotObservedNothing(const G2U_RobotObservedNothing& new_RobotObservedNothing)
	{
		if(this->_type == Type::RobotObservedNothing) {
			_RobotObservedNothing = new_RobotObservedNothing;
		}
		else {
			ClearCurrent();
			new(&_RobotObservedNothing) G2U_RobotObservedNothing{new_RobotObservedNothing};
			_type = Type::RobotObservedNothing;
		}
	}
	void Set_RobotObservedNothing(G2U_RobotObservedNothing&&  new_RobotObservedNothing)
	{
		if(this->_type == Type::RobotObservedNothing) {
			_RobotObservedNothing = std::move(new_RobotObservedNothing);
		}
		else {
			ClearCurrent();
			new(&_RobotObservedNothing) G2U_RobotObservedNothing{std::move(new_RobotObservedNothing)};
			_type = Type::RobotObservedNothing;
		}
	}

	/** DeviceDetectedVisionMarker **/
	const G2U_DeviceDetectedVisionMarker& Get_DeviceDetectedVisionMarker() const
	{
		assert(_type == Type::DeviceDetectedVisionMarker);
		return _DeviceDetectedVisionMarker;
	}
	void Set_DeviceDetectedVisionMarker(const G2U_DeviceDetectedVisionMarker& new_DeviceDetectedVisionMarker)
	{
		if(this->_type == Type::DeviceDetectedVisionMarker) {
			_DeviceDetectedVisionMarker = new_DeviceDetectedVisionMarker;
		}
		else {
			ClearCurrent();
			new(&_DeviceDetectedVisionMarker) G2U_DeviceDetectedVisionMarker{new_DeviceDetectedVisionMarker};
			_type = Type::DeviceDetectedVisionMarker;
		}
	}
	void Set_DeviceDetectedVisionMarker(G2U_DeviceDetectedVisionMarker&&  new_DeviceDetectedVisionMarker)
	{
		if(this->_type == Type::DeviceDetectedVisionMarker) {
			_DeviceDetectedVisionMarker = std::move(new_DeviceDetectedVisionMarker);
		}
		else {
			ClearCurrent();
			new(&_DeviceDetectedVisionMarker) G2U_DeviceDetectedVisionMarker{std::move(new_DeviceDetectedVisionMarker)};
			_type = Type::DeviceDetectedVisionMarker;
		}
	}

	/** RobotCompletedPickAndPlaceAction **/
	const G2U_RobotCompletedPickAndPlaceAction& Get_RobotCompletedPickAndPlaceAction() const
	{
		assert(_type == Type::RobotCompletedPickAndPlaceAction);
		return _RobotCompletedPickAndPlaceAction;
	}
	void Set_RobotCompletedPickAndPlaceAction(const G2U_RobotCompletedPickAndPlaceAction& new_RobotCompletedPickAndPlaceAction)
	{
		if(this->_type == Type::RobotCompletedPickAndPlaceAction) {
			_RobotCompletedPickAndPlaceAction = new_RobotCompletedPickAndPlaceAction;
		}
		else {
			ClearCurrent();
			new(&_RobotCompletedPickAndPlaceAction) G2U_RobotCompletedPickAndPlaceAction{new_RobotCompletedPickAndPlaceAction};
			_type = Type::RobotCompletedPickAndPlaceAction;
		}
	}
	void Set_RobotCompletedPickAndPlaceAction(G2U_RobotCompletedPickAndPlaceAction&&  new_RobotCompletedPickAndPlaceAction)
	{
		if(this->_type == Type::RobotCompletedPickAndPlaceAction) {
			_RobotCompletedPickAndPlaceAction = std::move(new_RobotCompletedPickAndPlaceAction);
		}
		else {
			ClearCurrent();
			new(&_RobotCompletedPickAndPlaceAction) G2U_RobotCompletedPickAndPlaceAction{std::move(new_RobotCompletedPickAndPlaceAction)};
			_type = Type::RobotCompletedPickAndPlaceAction;
		}
	}

	/** RobotCompletedPlaceObjectOnGroundAction **/
	const G2U_RobotCompletedPlaceObjectOnGroundAction& Get_RobotCompletedPlaceObjectOnGroundAction() const
	{
		assert(_type == Type::RobotCompletedPlaceObjectOnGroundAction);
		return _RobotCompletedPlaceObjectOnGroundAction;
	}
	void Set_RobotCompletedPlaceObjectOnGroundAction(const G2U_RobotCompletedPlaceObjectOnGroundAction& new_RobotCompletedPlaceObjectOnGroundAction)
	{
		if(this->_type == Type::RobotCompletedPlaceObjectOnGroundAction) {
			_RobotCompletedPlaceObjectOnGroundAction = new_RobotCompletedPlaceObjectOnGroundAction;
		}
		else {
			ClearCurrent();
			new(&_RobotCompletedPlaceObjectOnGroundAction) G2U_RobotCompletedPlaceObjectOnGroundAction{new_RobotCompletedPlaceObjectOnGroundAction};
			_type = Type::RobotCompletedPlaceObjectOnGroundAction;
		}
	}
	void Set_RobotCompletedPlaceObjectOnGroundAction(G2U_RobotCompletedPlaceObjectOnGroundAction&&  new_RobotCompletedPlaceObjectOnGroundAction)
	{
		if(this->_type == Type::RobotCompletedPlaceObjectOnGroundAction) {
			_RobotCompletedPlaceObjectOnGroundAction = std::move(new_RobotCompletedPlaceObjectOnGroundAction);
		}
		else {
			ClearCurrent();
			new(&_RobotCompletedPlaceObjectOnGroundAction) G2U_RobotCompletedPlaceObjectOnGroundAction{std::move(new_RobotCompletedPlaceObjectOnGroundAction)};
			_type = Type::RobotCompletedPlaceObjectOnGroundAction;
		}
	}

	/** PlaySound **/
	const G2U_PlaySound& Get_PlaySound() const
	{
		assert(_type == Type::PlaySound);
		return _PlaySound;
	}
	void Set_PlaySound(const G2U_PlaySound& new_PlaySound)
	{
		if(this->_type == Type::PlaySound) {
			_PlaySound = new_PlaySound;
		}
		else {
			ClearCurrent();
			new(&_PlaySound) G2U_PlaySound{new_PlaySound};
			_type = Type::PlaySound;
		}
	}
	void Set_PlaySound(G2U_PlaySound&&  new_PlaySound)
	{
		if(this->_type == Type::PlaySound) {
			_PlaySound = std::move(new_PlaySound);
		}
		else {
			ClearCurrent();
			new(&_PlaySound) G2U_PlaySound{std::move(new_PlaySound)};
			_type = Type::PlaySound;
		}
	}

	/** StopSound **/
	const G2U_StopSound& Get_StopSound() const
	{
		assert(_type == Type::StopSound);
		return _StopSound;
	}
	void Set_StopSound(const G2U_StopSound& new_StopSound)
	{
		if(this->_type == Type::StopSound) {
			_StopSound = new_StopSound;
		}
		else {
			ClearCurrent();
			new(&_StopSound) G2U_StopSound{new_StopSound};
			_type = Type::StopSound;
		}
	}
	void Set_StopSound(G2U_StopSound&&  new_StopSound)
	{
		if(this->_type == Type::StopSound) {
			_StopSound = std::move(new_StopSound);
		}
		else {
			ClearCurrent();
			new(&_StopSound) G2U_StopSound{std::move(new_StopSound)};
			_type = Type::StopSound;
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
				new(&(this->_Ping)) G2U_Ping(buffer);
			}
			else {
			this->_Ping.Unpack(buffer);
			}
			break;
		case Type::RobotAvailable:
			if (newType != oldType) {
				new(&(this->_RobotAvailable)) G2U_RobotAvailable(buffer);
			}
			else {
			this->_RobotAvailable.Unpack(buffer);
			}
			break;
		case Type::UiDeviceAvailable:
			if (newType != oldType) {
				new(&(this->_UiDeviceAvailable)) G2U_UiDeviceAvailable(buffer);
			}
			else {
			this->_UiDeviceAvailable.Unpack(buffer);
			}
			break;
		case Type::RobotConnected:
			if (newType != oldType) {
				new(&(this->_RobotConnected)) G2U_RobotConnected(buffer);
			}
			else {
			this->_RobotConnected.Unpack(buffer);
			}
			break;
		case Type::RobotDisconnected:
			if (newType != oldType) {
				new(&(this->_RobotDisconnected)) G2U_RobotDisconnected(buffer);
			}
			else {
			this->_RobotDisconnected.Unpack(buffer);
			}
			break;
		case Type::UiDeviceConnected:
			if (newType != oldType) {
				new(&(this->_UiDeviceConnected)) G2U_UiDeviceConnected(buffer);
			}
			else {
			this->_UiDeviceConnected.Unpack(buffer);
			}
			break;
		case Type::RobotState:
			if (newType != oldType) {
				new(&(this->_RobotState)) G2U_RobotState(buffer);
			}
			else {
			this->_RobotState.Unpack(buffer);
			}
			break;
		case Type::ImageChunk:
			if (newType != oldType) {
				new(&(this->_ImageChunk)) G2U_ImageChunk(buffer);
			}
			else {
			this->_ImageChunk.Unpack(buffer);
			}
			break;
		case Type::RobotObservedObject:
			if (newType != oldType) {
				new(&(this->_RobotObservedObject)) G2U_RobotObservedObject(buffer);
			}
			else {
			this->_RobotObservedObject.Unpack(buffer);
			}
			break;
		case Type::RobotObservedNothing:
			if (newType != oldType) {
				new(&(this->_RobotObservedNothing)) G2U_RobotObservedNothing(buffer);
			}
			else {
			this->_RobotObservedNothing.Unpack(buffer);
			}
			break;
		case Type::DeviceDetectedVisionMarker:
			if (newType != oldType) {
				new(&(this->_DeviceDetectedVisionMarker)) G2U_DeviceDetectedVisionMarker(buffer);
			}
			else {
			this->_DeviceDetectedVisionMarker.Unpack(buffer);
			}
			break;
		case Type::RobotCompletedPickAndPlaceAction:
			if (newType != oldType) {
				new(&(this->_RobotCompletedPickAndPlaceAction)) G2U_RobotCompletedPickAndPlaceAction(buffer);
			}
			else {
			this->_RobotCompletedPickAndPlaceAction.Unpack(buffer);
			}
			break;
		case Type::RobotCompletedPlaceObjectOnGroundAction:
			if (newType != oldType) {
				new(&(this->_RobotCompletedPlaceObjectOnGroundAction)) G2U_RobotCompletedPlaceObjectOnGroundAction(buffer);
			}
			else {
			this->_RobotCompletedPlaceObjectOnGroundAction.Unpack(buffer);
			}
			break;
		case Type::PlaySound:
			if (newType != oldType) {
				new(&(this->_PlaySound)) G2U_PlaySound(buffer);
			}
			else {
			this->_PlaySound.Unpack(buffer);
			}
			break;
		case Type::StopSound:
			if (newType != oldType) {
				new(&(this->_StopSound)) G2U_StopSound(buffer);
			}
			else {
			this->_StopSound.Unpack(buffer);
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
		case Type::RobotAvailable:
			this->_RobotAvailable.Pack(buffer);
			break;
		case Type::UiDeviceAvailable:
			this->_UiDeviceAvailable.Pack(buffer);
			break;
		case Type::RobotConnected:
			this->_RobotConnected.Pack(buffer);
			break;
		case Type::RobotDisconnected:
			this->_RobotDisconnected.Pack(buffer);
			break;
		case Type::UiDeviceConnected:
			this->_UiDeviceConnected.Pack(buffer);
			break;
		case Type::RobotState:
			this->_RobotState.Pack(buffer);
			break;
		case Type::ImageChunk:
			this->_ImageChunk.Pack(buffer);
			break;
		case Type::RobotObservedObject:
			this->_RobotObservedObject.Pack(buffer);
			break;
		case Type::RobotObservedNothing:
			this->_RobotObservedNothing.Pack(buffer);
			break;
		case Type::DeviceDetectedVisionMarker:
			this->_DeviceDetectedVisionMarker.Pack(buffer);
			break;
		case Type::RobotCompletedPickAndPlaceAction:
			this->_RobotCompletedPickAndPlaceAction.Pack(buffer);
			break;
		case Type::RobotCompletedPlaceObjectOnGroundAction:
			this->_RobotCompletedPlaceObjectOnGroundAction.Pack(buffer);
			break;
		case Type::PlaySound:
			this->_PlaySound.Pack(buffer);
			break;
		case Type::StopSound:
			this->_StopSound.Pack(buffer);
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
		case Type::RobotAvailable:
			result += _RobotAvailable.Size();
			break;
		case Type::UiDeviceAvailable:
			result += _UiDeviceAvailable.Size();
			break;
		case Type::RobotConnected:
			result += _RobotConnected.Size();
			break;
		case Type::RobotDisconnected:
			result += _RobotDisconnected.Size();
			break;
		case Type::UiDeviceConnected:
			result += _UiDeviceConnected.Size();
			break;
		case Type::RobotState:
			result += _RobotState.Size();
			break;
		case Type::ImageChunk:
			result += _ImageChunk.Size();
			break;
		case Type::RobotObservedObject:
			result += _RobotObservedObject.Size();
			break;
		case Type::RobotObservedNothing:
			result += _RobotObservedNothing.Size();
			break;
		case Type::DeviceDetectedVisionMarker:
			result += _DeviceDetectedVisionMarker.Size();
			break;
		case Type::RobotCompletedPickAndPlaceAction:
			result += _RobotCompletedPickAndPlaceAction.Size();
			break;
		case Type::RobotCompletedPlaceObjectOnGroundAction:
			result += _RobotCompletedPlaceObjectOnGroundAction.Size();
			break;
		case Type::PlaySound:
			result += _PlaySound.Size();
			break;
		case Type::StopSound:
			result += _StopSound.Size();
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
			_Ping.~G2U_Ping();
			break;
		case Type::RobotAvailable:
			_RobotAvailable.~G2U_RobotAvailable();
			break;
		case Type::UiDeviceAvailable:
			_UiDeviceAvailable.~G2U_UiDeviceAvailable();
			break;
		case Type::RobotConnected:
			_RobotConnected.~G2U_RobotConnected();
			break;
		case Type::RobotDisconnected:
			_RobotDisconnected.~G2U_RobotDisconnected();
			break;
		case Type::UiDeviceConnected:
			_UiDeviceConnected.~G2U_UiDeviceConnected();
			break;
		case Type::RobotState:
			_RobotState.~G2U_RobotState();
			break;
		case Type::ImageChunk:
			_ImageChunk.~G2U_ImageChunk();
			break;
		case Type::RobotObservedObject:
			_RobotObservedObject.~G2U_RobotObservedObject();
			break;
		case Type::RobotObservedNothing:
			_RobotObservedNothing.~G2U_RobotObservedNothing();
			break;
		case Type::DeviceDetectedVisionMarker:
			_DeviceDetectedVisionMarker.~G2U_DeviceDetectedVisionMarker();
			break;
		case Type::RobotCompletedPickAndPlaceAction:
			_RobotCompletedPickAndPlaceAction.~G2U_RobotCompletedPickAndPlaceAction();
			break;
		case Type::RobotCompletedPlaceObjectOnGroundAction:
			_RobotCompletedPlaceObjectOnGroundAction.~G2U_RobotCompletedPlaceObjectOnGroundAction();
			break;
		case Type::PlaySound:
			_PlaySound.~G2U_PlaySound();
			break;
		case Type::StopSound:
			_StopSound.~G2U_StopSound();
			break;
		default:
			break;
		}
		_type = Type::INVALID;
	}
	Type _type;

	union {
		G2U_Ping _Ping;
		G2U_RobotAvailable _RobotAvailable;
		G2U_UiDeviceAvailable _UiDeviceAvailable;
		G2U_RobotConnected _RobotConnected;
		G2U_RobotDisconnected _RobotDisconnected;
		G2U_UiDeviceConnected _UiDeviceConnected;
		G2U_RobotState _RobotState;
		G2U_ImageChunk _ImageChunk;
		G2U_RobotObservedObject _RobotObservedObject;
		G2U_RobotObservedNothing _RobotObservedNothing;
		G2U_DeviceDetectedVisionMarker _DeviceDetectedVisionMarker;
		G2U_RobotCompletedPickAndPlaceAction _RobotCompletedPickAndPlaceAction;
		G2U_RobotCompletedPlaceObjectOnGroundAction _RobotCompletedPlaceObjectOnGroundAction;
		G2U_PlaySound _PlaySound;
		G2U_StopSound _StopSound;
	};
};

}//namespace Anki
}//namespace Cozmo
