// Copyright (c) 2016-2017 Anki, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License in the file LICENSE.txt or at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

using Anki.Cozmo.ExternalInterface;

namespace Anki
{
  namespace Cozmo
  {

    public class SDKMessageOut
    {
      public readonly MessageGameToEngine Message = new MessageGameToEngine();

      #region IMessageWrapper implementation

      public void Unpack(System.IO.Stream stream)
      {
        Message.Unpack(stream);
      }

      public void Unpack(System.IO.BinaryReader reader)
      {
        Message.Unpack(reader);
      }

      public void Pack(System.IO.Stream stream)
      {
        Message.Pack(stream);
      }

      public void Pack(System.IO.BinaryWriter writer)
      {
        Message.Pack(writer);
      }

      public string GetTag()
      {
        return Message.GetTag().ToString();
      }

      public int Size
      {
        get
        {
          return Message.Size;
        }
      }

      public bool IsValid
      {
        get
        {
          return Message.GetTag() != MessageGameToEngine.Tag.INVALID;
        }
      }

      #endregion
    }

    public class SDKMessageIn
    {
      public readonly MessageEngineToGame Message = new MessageEngineToGame();

      #region IMessageWrapper implementation

      public void Unpack(System.IO.Stream stream)
      {
        Message.Unpack(stream);
      }

      public void Unpack(System.IO.BinaryReader reader)
      {
        Message.Unpack(reader);
      }

      public void Pack(System.IO.Stream stream)
      {
        Message.Pack(stream);
      }

      public void Pack(System.IO.BinaryWriter writer)
      {
        Message.Pack(writer);
      }

      public string GetTag()
      {
        return Message.GetTag().ToString();
      }

      public int Size
      {
        get
        {
          return Message.Size;
        }
      }

      public bool IsValid
      {
        get
        {
          return Message.GetTag() != MessageEngineToGame.Tag.INVALID;
        }
      }

      #endregion
    }

  } // namespace Cozmo
} // namespace Anki