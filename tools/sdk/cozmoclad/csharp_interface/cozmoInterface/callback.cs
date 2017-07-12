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

using System;

namespace Anki
{
	namespace Cozmo
	{
    public interface iCallback
    {
      void Execute( object message );
    }

    public class CallbackImpl<T> : iCallback
    {
      Action<T> _action;

      public CallbackImpl(Action<T> action)
      {
        _action = action;
      }

      public void Execute(object message)
      {
        if (!(message is T)) {
          throw new System.Exception("CallbackImpl<" + typeof(T).FullName + ">.Execute - can't execute with param of type " + message.GetType().FullName);
        }
        if (_action != null) {
          _action((T)message);
        }
      }
    }
	} // namespace Cozmo
} // namespace Anki