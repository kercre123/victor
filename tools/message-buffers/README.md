#CLAD
##The C-Like Abstract Data Language

###Abstract:
CLAD is a light-weight definition language for defining binary-serializable messages.  The language uniquely determines the layout, and provides some out of bound type information.

###Defense:
Message-passing is the backbone of Anki's software architecture, and has proven to be a huge benefit in terms of modularity, flexibility and safety.
However, writing the code that serializes and deserializes the messages is error-prone and time consuming.  Additionally, the packing and unpacking code is often duplicated, making sweeping optimizations a challenge.

"Protocol Buffers" is a difficult to use and cumbersome utility that supports much more than basic data serialization and the code they generate is not very intelligible.  Adding new environments or custom targets is not easy.

So Anki now has CLAD, the C-Like Abstract Data Language, to generate light-weight, flexible and human-readable data object wrappers in the programming language of your choice. A small amount of message definition code is needed to uniquely define the messages you wish to pack and unpack, and the emitters use that information to generate the code for you.  This frees you up to think about the what and not the how.

The resulting output will be in the language you need, and will be suitable to check into source control, added to your project and indexed directly by our favorite IDE (to allow for code-completion and straight-forward debugging).

Any new emitter can easily be unit-tested against the other emitters, meaning that the code confidence grows with the square of the number of emitters.

###A System of Two Parts:
1. A LALR parser for the CLAD language, written in Python using PLY (Python Lex/Yacc), which produces an abstract syntax tree full of rich type information.
2. A set of emitters for various languages / environments that walk the abstract syntax tree and print well formatted code, full of useful comments and other helpful utilities, optimized as is appropriate for the target of the emitter.

###Details:
* A clad file consists of: 
	* enum declarations (like C enums)
	* message declarations (like C structs)
	* union declarations (like C unions + a tag field for type information)

* Enum types:

		 enum *primitive_type* *name* {
		     memberOne [= *number-constant*] 
		     [, memberTwo [= *number-constant*] 
		 }
		  
	* members may have initializers to lock values
	* guaranteed on-the-wire storage size
	* emitter generates type-safe enum suited for your language

* Message types: (composite types, product types)

		message *name* {  
		  *type* memberOne,
		  *type* memberTwo[*int_type*],
		  ...
		}
		
	* members can be of primitive or v-array type
	* members are serialized in the order in which they are declared
	* members are packed as tightly as possible, with no space in between members
	* the v-array type is a variable length array, with a length field *k* at the beginning followed by *k* serialized objects of type *type*
	* emitted code contains pack and unpack methods or functions as appropriate for your language or environment.

* Union types: (sum types / 'or' types)

		union *name* {
		  *type* memberOne,
		  *type* memberTwo,
		  *type* memberThree
		}
	
	* members can be of composite type only (currently)
	* object will be serialized as a *1-byte tag*, follwed immediately by the serialization of the underlying type
	* used to 'wrap' a set of messages into a protocol
	* emitted code contains pack and unpack functions, just like the message objects
	* emitted code contains a tag and a GetTag method as is appropriate for your language or environment.

* Primitive types:
    * 'bool' (bool)
    * 'int_8' (char)
    * 'int_16' (short)
    * 'int_32' (int)
    * 'int_64' (long)
    * 'uint_8' (unsigned char)
    * 'uint_16' (unsigned short)
    * 'uint_32' (unsigned int)
    * 'uint_64' (unsigned long)
    * 'float_32' (single-precision float)
    * 'float_64' (double precision float)

* Array types:
	* variable length arrays:

			type memberName[int_type]

	* length field size is defined by the type given between square brackets.
	* currently only primitive types are allowed for variable-length arrays.
	* static arrays are currently not implemented (but should be easily possible)

* String types:

		string strName
	or
	
		string[int_type] strName

	* implemented as a variable-length array
		* length field followed by a UTF-8 string
	* defaults to uint_8 length
	* emitted code will treat this as a native string as is appropriate for your language or environment.

* Void type:
	* looks like an empty message.
	* currently not implemented.

***Example:***

	// PlayerMessage message definition file
	//  for the C-Like Abstract Data language
	// Author: Mark Pauley
	// Copyright: Anki, Inc (c) 2015
	
	// Void type is not yet implemented, this is a place-holder for that type.
	message Void {}
	
	// Wrapped buffer
	//  Unions currently don't support array members directly, so we wrap them in messages.
	//  This also has the side effect of reduced code-bloat in the packer / unpacker code.
	message Buffer16
	{
	  uint_8 buffer[uint_16]
	}
	
	// PlayerId
	message PlayerId
	{
	  uint_8 playerId
	}
	
	// Player and Profile pair
	message PlayerAndProfile
	{
	  uint_8 playerId,
	  uint_8 profileBuffer[uint_8]
	}
	
	// Player And Vehicle pair
	message PlayerAndVehicle
	{
	  uint_8 playerId,
	  uint_64 vehicleUUID
	}
	
	// Lobby Service
	enum uint_8 ClientLobbyState {
	  None = 0,
	  PreLobby,
	  InLobby,
	  GameReady,
	  InGame
	}
	
	message LobbyState
	{
	  ClientLobbyState lobbyState
	}
	
	// Emotion Service
	enum uint_8 EmotionRequestType {
	  None = 0,
	  Start,
	  Stop
	}
	
	message PlayerEmotion
	{
	  string soundToPlay,
	  EmotionRequestType requestType
	}
	
	union PlayerMessage
	{
	    Buffer16         UiMessageData, // wrapped UIMessage
	    Void             RequestLobbyData,
	    Buffer16         ResponseLobbyData,
	    PlayerId         RequestPlayerData,  //data for playerId
	    PlayerAndProfile ResponsePlayerData, //wrapped profile
	    PlayerAndVehicle PlayerRequestVehicleSelect,
	    Void             HostReadyForGameStart,
	    Void             HostBackToLobby,
	    Void             ClientReadForGameStart,
	    LobbyState       ClientStateUpdate,
	    PlayerEmotion    EmotionMessage,
	    Buffer16         VehicleControllerMessage
	}


Which the parser Turns into a tree with type information.
> For in-depth info the parser contains its own ast debug info emitter.
> $ parser/parser.py PlayerMessage.clad

This in turn results in the following CPP code when the CPP_emitter walks the AST:

	./CPP_emitter.py -n Anki -n DriveEngine -n NetworkService -g __PlayerService_Connection_Messages_PlayerMessage_H__ -I "util/dataPacking" PlayerMessage.clad > PlayerMessage.out.hpp

results in

	#ifndef __PlayerService_Connection_Messages_PlayerMessage_H__
	#define __PlayerService_Connection_Messages_PlayerMessage_H__
	#include <cstdint>
	#include <cassert>
	#include <string>
	#include <vector>
	
	#include "util/dataPacking/SafeMessageBuffer.h"
	
	namespace Anki
	{
	namespace DriveEngine
	{
	namespace NetworkService
	{
	using SafeMessageBuffer = Anki::Util::SafeMessageBuffer;
	// Generated from PlayerMessage.clad
	
	
	enum class ClientLobbyState : uint8_t {
		None = 0,	// 0
		PreLobby,	// 1
		InLobby,	// 2
		GameReady,	// 3
		InGame	// 4
	};
	
	
	enum class EmotionRequestType : uint8_t {
		None = 0,	// 0
		Start,	// 1
		Stop	// 2
	};
	
	struct Void
	{
	
		/**** Constructors ****/
		Void() = default;
	
		explicit Void(const uint8_t* buff, size_t len)
		{
			const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
			Unpack(buffer);
		}
	
		explicit Void(const SafeMessageBuffer& buffer)
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
	
		bool operator==(const Void& other) const
		{
			return true;
		}
	
		bool operator!=(const Void& other) const
		{
			return !(operator==(other));
		}
	};
	
	struct Buffer16
	{
		std::vector<uint8_t> buffer;
	
		/**** Constructors ****/
		Buffer16() = default;
	
		explicit Buffer16(const std::vector<uint8_t>& buffer)
		:buffer(buffer)
		{}
		explicit Buffer16(const uint8_t* buff, size_t len)
		{
			const SafeMessageBuffer buffer(const_cast<uint8_t*>(buff), len, false);
			Unpack(buffer);
		}
	
		explicit Buffer16(const SafeMessageBuffer& buffer)
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
			buffer.WriteVArray<uint8_t, uint16_t>(this->buffer);
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
			buffer.ReadVArray<uint8_t, uint16_t>(this->buffer);
			return buffer.GetBytesRead();
		}
		size_t Size() const 
		{
			size_t result{0};
			//buffer
			result += 2; //length = uint_16
			result += 1 * buffer.size(); //member = uint_8
			return result;
		}
	
		bool operator==(const Buffer16& other) const
		{
			if (buffer != other.buffer) {
				return false;
			}
			return true;
		}
	
		bool operator!=(const Buffer16& other) const
		{
			return !(operator==(other));
		}
	};

...

	class PlayerMessage
	{
	public:
		/**** Constructors ****/
		PlayerMessage() :_type(Type::INVALID) { }
		explicit PlayerMessage(const SafeMessageBuffer& buff) :_type(Type::INVALID)
		{
			Unpack(buff);
		}
		explicit PlayerMessage(const uint8_t* buffer, size_t length)
		{
			SafeMessageBuffer buff(const_cast<uint8_t*>(buffer), length);
			Unpack(buff);
		}
	
		~PlayerMessage() { ClearCurrent(); }
		enum class Type : uint8_t {
			UiMessageData,	// 0
			RequestLobbyData,	// 1
			ResponseLobbyData,	// 2
			RequestPlayerData,	// 3
			ResponsePlayerData,	// 4
			PlayerRequestVehicleSelect,	// 5
			HostReadyForGameStart,	// 6
			HostBackToLobby,	// 7
			ClientReadForGameStart,	// 8
			ClientStateUpdate,	// 9
			EmotionMessage,	// 10
			VehicleControllerMessage,	// 11
			INVALID
		};
		Type GetType() const { return _type; }
	
		const Buffer16& Get_UiMessageData() const
		{
			assert(_type == Type::UiMessageData);
			return _UiMessageData;
		}
		void Set_UiMessageData(const Buffer16& new_UiMessageData)
		{
			if(this->_type == Type::UiMessageData) {
				_UiMessageData = new_UiMessageData;
			}
			else {
				ClearCurrent();
				new(&_UiMessageData) Buffer16{new_UiMessageData};
				_type = Type::UiMessageData;
			}
		}
	
		const Void& Get_RequestLobbyData() const
		{
			assert(_type == Type::RequestLobbyData);
			return _RequestLobbyData;
		}
		void Set_RequestLobbyData(const Void& new_RequestLobbyData)
		{
			if(this->_type == Type::RequestLobbyData) {
				_RequestLobbyData = new_RequestLobbyData;
			}
			else {
				ClearCurrent();
				new(&_RequestLobbyData) Void{new_RequestLobbyData};
				_type = Type::RequestLobbyData;
			}
		}
	
		const Buffer16& Get_ResponseLobbyData() const
		{
			assert(_type == Type::ResponseLobbyData);
			return _ResponseLobbyData;
		}
		void Set_ResponseLobbyData(const Buffer16& new_ResponseLobbyData)
		{
			if(this->_type == Type::ResponseLobbyData) {
				_ResponseLobbyData = new_ResponseLobbyData;
			}
			else {
				ClearCurrent();
				new(&_ResponseLobbyData) Buffer16{new_ResponseLobbyData};
				_type = Type::ResponseLobbyData;
			}
		}
	
		const PlayerId& Get_RequestPlayerData() const
		{
			assert(_type == Type::RequestPlayerData);
			return _RequestPlayerData;
		}
		void Set_RequestPlayerData(const PlayerId& new_RequestPlayerData)
		{
			if(this->_type == Type::RequestPlayerData) {
				_RequestPlayerData = new_RequestPlayerData;
			}
			else {
				ClearCurrent();
				new(&_RequestPlayerData) PlayerId{new_RequestPlayerData};
				_type = Type::RequestPlayerData;
			}
		}
	
		const PlayerAndProfile& Get_ResponsePlayerData() const
		{
			assert(_type == Type::ResponsePlayerData);
			return _ResponsePlayerData;
		}
		void Set_ResponsePlayerData(const PlayerAndProfile& new_ResponsePlayerData)
		{
			if(this->_type == Type::ResponsePlayerData) {
				_ResponsePlayerData = new_ResponsePlayerData;
			}
			else {
				ClearCurrent();
				new(&_ResponsePlayerData) PlayerAndProfile{new_ResponsePlayerData};
				_type = Type::ResponsePlayerData;
			}
		}
	
		const PlayerAndVehicle& Get_PlayerRequestVehicleSelect() const
		{
			assert(_type == Type::PlayerRequestVehicleSelect);
			return _PlayerRequestVehicleSelect;
		}
		void Set_PlayerRequestVehicleSelect(const PlayerAndVehicle& new_PlayerRequestVehicleSelect)
		{
			if(this->_type == Type::PlayerRequestVehicleSelect) {
				_PlayerRequestVehicleSelect = new_PlayerRequestVehicleSelect;
			}
			else {
				ClearCurrent();
				new(&_PlayerRequestVehicleSelect) PlayerAndVehicle{new_PlayerRequestVehicleSelect};
				_type = Type::PlayerRequestVehicleSelect;
			}
		}
	
		const Void& Get_HostReadyForGameStart() const
		{
			assert(_type == Type::HostReadyForGameStart);
			return _HostReadyForGameStart;
		}
		void Set_HostReadyForGameStart(const Void& new_HostReadyForGameStart)
		{
			if(this->_type == Type::HostReadyForGameStart) {
				_HostReadyForGameStart = new_HostReadyForGameStart;
			}
			else {
				ClearCurrent();
				new(&_HostReadyForGameStart) Void{new_HostReadyForGameStart};
				_type = Type::HostReadyForGameStart;
			}
		}
	
		const Void& Get_HostBackToLobby() const
		{
			assert(_type == Type::HostBackToLobby);
			return _HostBackToLobby;
		}
		void Set_HostBackToLobby(const Void& new_HostBackToLobby)
		{
			if(this->_type == Type::HostBackToLobby) {
				_HostBackToLobby = new_HostBackToLobby;
			}
			else {
				ClearCurrent();
				new(&_HostBackToLobby) Void{new_HostBackToLobby};
				_type = Type::HostBackToLobby;
			}
		}
	
		const Void& Get_ClientReadForGameStart() const
		{
			assert(_type == Type::ClientReadForGameStart);
			return _ClientReadForGameStart;
		}
		void Set_ClientReadForGameStart(const Void& new_ClientReadForGameStart)
		{
			if(this->_type == Type::ClientReadForGameStart) {
				_ClientReadForGameStart = new_ClientReadForGameStart;
			}
			else {
				ClearCurrent();
				new(&_ClientReadForGameStart) Void{new_ClientReadForGameStart};
				_type = Type::ClientReadForGameStart;
			}
		}
	
		const LobbyState& Get_ClientStateUpdate() const
		{
			assert(_type == Type::ClientStateUpdate);
			return _ClientStateUpdate;
		}
		void Set_ClientStateUpdate(const LobbyState& new_ClientStateUpdate)
		{
			if(this->_type == Type::ClientStateUpdate) {
				_ClientStateUpdate = new_ClientStateUpdate;
			}
			else {
				ClearCurrent();
				new(&_ClientStateUpdate) LobbyState{new_ClientStateUpdate};
				_type = Type::ClientStateUpdate;
			}
		}
	
		const PlayerEmotion& Get_EmotionMessage() const
		{
			assert(_type == Type::EmotionMessage);
			return _EmotionMessage;
		}
		void Set_EmotionMessage(const PlayerEmotion& new_EmotionMessage)
		{
			if(this->_type == Type::EmotionMessage) {
				_EmotionMessage = new_EmotionMessage;
			}
			else {
				ClearCurrent();
				new(&_EmotionMessage) PlayerEmotion{new_EmotionMessage};
				_type = Type::EmotionMessage;
			}
		}
	
		const Buffer16& Get_VehicleControllerMessage() const
		{
			assert(_type == Type::VehicleControllerMessage);
			return _VehicleControllerMessage;
		}
		void Set_VehicleControllerMessage(const Buffer16& new_VehicleControllerMessage)
		{
			if(this->_type == Type::VehicleControllerMessage) {
				_VehicleControllerMessage = new_VehicleControllerMessage;
			}
			else {
				ClearCurrent();
				new(&_VehicleControllerMessage) Buffer16{new_VehicleControllerMessage};
				_type = Type::VehicleControllerMessage;
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
			case Type::UiMessageData:
				if (newType != oldType) {
					new(&(this->_UiMessageData)) Buffer16(buffer);
				}
				else {
				this->_UiMessageData.Unpack(buffer);
				}
				break;
			case Type::RequestLobbyData:
				if (newType != oldType) {
					new(&(this->_RequestLobbyData)) Void(buffer);
				}
				else {
				this->_RequestLobbyData.Unpack(buffer);
				}
				break;
			case Type::ResponseLobbyData:
				if (newType != oldType) {
					new(&(this->_ResponseLobbyData)) Buffer16(buffer);
				}
				else {
				this->_ResponseLobbyData.Unpack(buffer);
				}
				break;
			case Type::RequestPlayerData:
				if (newType != oldType) {
					new(&(this->_RequestPlayerData)) PlayerId(buffer);
				}
				else {
				this->_RequestPlayerData.Unpack(buffer);
				}
				break;
			case Type::ResponsePlayerData:
				if (newType != oldType) {
					new(&(this->_ResponsePlayerData)) PlayerAndProfile(buffer);
				}
				else {
				this->_ResponsePlayerData.Unpack(buffer);
				}
				break;
			case Type::PlayerRequestVehicleSelect:
				if (newType != oldType) {
					new(&(this->_PlayerRequestVehicleSelect)) PlayerAndVehicle(buffer);
				}
				else {
				this->_PlayerRequestVehicleSelect.Unpack(buffer);
				}
				break;
			case Type::HostReadyForGameStart:
				if (newType != oldType) {
					new(&(this->_HostReadyForGameStart)) Void(buffer);
				}
				else {
				this->_HostReadyForGameStart.Unpack(buffer);
				}
				break;
			case Type::HostBackToLobby:
				if (newType != oldType) {
					new(&(this->_HostBackToLobby)) Void(buffer);
				}
				else {
				this->_HostBackToLobby.Unpack(buffer);
				}
				break;
			case Type::ClientReadForGameStart:
				if (newType != oldType) {
					new(&(this->_ClientReadForGameStart)) Void(buffer);
				}
				else {
				this->_ClientReadForGameStart.Unpack(buffer);
				}
				break;
			case Type::ClientStateUpdate:
				if (newType != oldType) {
					new(&(this->_ClientStateUpdate)) LobbyState(buffer);
				}
				else {
				this->_ClientStateUpdate.Unpack(buffer);
				}
				break;
			case Type::EmotionMessage:
				if (newType != oldType) {
					new(&(this->_EmotionMessage)) PlayerEmotion(buffer);
				}
				else {
				this->_EmotionMessage.Unpack(buffer);
				}
				break;
			case Type::VehicleControllerMessage:
				if (newType != oldType) {
					new(&(this->_VehicleControllerMessage)) Buffer16(buffer);
				}
				else {
				this->_VehicleControllerMessage.Unpack(buffer);
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
			case Type::UiMessageData:
				this->_UiMessageData.Pack(buffer);
				break;
			case Type::RequestLobbyData:
				this->_RequestLobbyData.Pack(buffer);
				break;
			case Type::ResponseLobbyData:
				this->_ResponseLobbyData.Pack(buffer);
				break;
			case Type::RequestPlayerData:
				this->_RequestPlayerData.Pack(buffer);
				break;
			case Type::ResponsePlayerData:
				this->_ResponsePlayerData.Pack(buffer);
				break;
			case Type::PlayerRequestVehicleSelect:
				this->_PlayerRequestVehicleSelect.Pack(buffer);
				break;
			case Type::HostReadyForGameStart:
				this->_HostReadyForGameStart.Pack(buffer);
				break;
			case Type::HostBackToLobby:
				this->_HostBackToLobby.Pack(buffer);
				break;
			case Type::ClientReadForGameStart:
				this->_ClientReadForGameStart.Pack(buffer);
				break;
			case Type::ClientStateUpdate:
				this->_ClientStateUpdate.Pack(buffer);
				break;
			case Type::EmotionMessage:
				this->_EmotionMessage.Pack(buffer);
				break;
			case Type::VehicleControllerMessage:
				this->_VehicleControllerMessage.Pack(buffer);
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
			case Type::UiMessageData:
				result += _UiMessageData.Size();
				break;
			case Type::RequestLobbyData:
				result += _RequestLobbyData.Size();
				break;
			case Type::ResponseLobbyData:
				result += _ResponseLobbyData.Size();
				break;
			case Type::RequestPlayerData:
				result += _RequestPlayerData.Size();
				break;
			case Type::ResponsePlayerData:
				result += _ResponsePlayerData.Size();
				break;
			case Type::PlayerRequestVehicleSelect:
				result += _PlayerRequestVehicleSelect.Size();
				break;
			case Type::HostReadyForGameStart:
				result += _HostReadyForGameStart.Size();
				break;
			case Type::HostBackToLobby:
				result += _HostBackToLobby.Size();
				break;
			case Type::ClientReadForGameStart:
				result += _ClientReadForGameStart.Size();
				break;
			case Type::ClientStateUpdate:
				result += _ClientStateUpdate.Size();
				break;
			case Type::EmotionMessage:
				result += _EmotionMessage.Size();
				break;
			case Type::VehicleControllerMessage:
				result += _VehicleControllerMessage.Size();
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
			case Type::UiMessageData:
				_UiMessageData.~Buffer16();
				break;
			case Type::RequestLobbyData:
				_RequestLobbyData.~Void();
				break;
			case Type::ResponseLobbyData:
				_ResponseLobbyData.~Buffer16();
				break;
			case Type::RequestPlayerData:
				_RequestPlayerData.~PlayerId();
				break;
			case Type::ResponsePlayerData:
				_ResponsePlayerData.~PlayerAndProfile();
				break;
			case Type::PlayerRequestVehicleSelect:
				_PlayerRequestVehicleSelect.~PlayerAndVehicle();
				break;
			case Type::HostReadyForGameStart:
				_HostReadyForGameStart.~Void();
				break;
			case Type::HostBackToLobby:
				_HostBackToLobby.~Void();
				break;
			case Type::ClientReadForGameStart:
				_ClientReadForGameStart.~Void();
				break;
			case Type::ClientStateUpdate:
				_ClientStateUpdate.~LobbyState();
				break;
			case Type::EmotionMessage:
				_EmotionMessage.~PlayerEmotion();
				break;
			case Type::VehicleControllerMessage:
				_VehicleControllerMessage.~Buffer16();
				break;
			default:
				break;
			}
			_type = Type::INVALID;
		}
		Type _type;
	
		union {
			Buffer16 _UiMessageData;
			Void _RequestLobbyData;
			Buffer16 _ResponseLobbyData;
			PlayerId _RequestPlayerData;
			PlayerAndProfile _ResponsePlayerData;
			PlayerAndVehicle _PlayerRequestVehicleSelect;
			Void _HostReadyForGameStart;
			Void _HostBackToLobby;
			Void _ClientReadForGameStart;
			LobbyState _ClientStateUpdate;
			PlayerEmotion _EmotionMessage;
			Buffer16 _VehicleControllerMessage;
		};
	};
	
	}//namespace Anki
	}//namespace DriveEngine
	}//namespace NetworkService
	#endif // __PlayerService_Connection_Messages_PlayerMessage_H__