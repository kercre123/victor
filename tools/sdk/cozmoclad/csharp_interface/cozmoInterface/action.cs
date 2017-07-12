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

namespace Anki
{
  namespace Cozmo
  {
    public class Action
    {
      private Connection _connection = null;
      private ExternalInterface.QueueSingleAction _message = null;
      private byte _robotId = 0;
      private uint _id = 0;
      private bool _completed = false;

      private static uint _nextActionId = (uint)ActionConstants.FIRST_SDK_TAG;

      public uint ID { get { return _id; } }
      public ExternalInterface.QueueSingleAction Message { get { return _message; } }

      public Action(Connection connection, byte robotId)
      {
        _connection = connection;
        _robotId = robotId;

        _id = _nextActionId;
        _nextActionId++;
        if (_nextActionId >= (uint)ActionConstants.LAST_SDK_TAG)
        {
          _nextActionId = (uint)ActionConstants.FIRST_SDK_TAG;
        }
      }

      public void Abort()
      {
        _connection.SendMessage(new ExternalInterface.CancelActionByIdTag(_id, _robotId));
      }

      public void MarkAsComplete()
      {
        _completed = true;
      }

      public void WaitForCompleted()
      {
        BlockUntilCondition block = new BlockUntilCondition(() => _completed==true);
        block.Block();
      }

      public void Initialize<T>(T state, int numRetries, bool inParallel)
      {
        Cozmo.QueueActionPosition position = inParallel ? Cozmo.QueueActionPosition.IN_PARALLEL : Cozmo.QueueActionPosition.NOW;
        ExternalInterface.RobotActionUnion action = new ExternalInterface.RobotActionUnion();
        action.Initialize(state);
        _message = new ExternalInterface.QueueSingleAction(robotID: _robotId, idTag: _id, numRetries: (byte)numRetries, position: position, action: action);
      }
    }
  } // namespace Cozmo
} // namespace Anki