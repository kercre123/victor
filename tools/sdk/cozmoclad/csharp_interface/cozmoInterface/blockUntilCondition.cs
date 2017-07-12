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

using System.Threading;

namespace Anki
{
  namespace Cozmo
  {

    public class BlockUntilCondition
    {
      private System.Func<bool> _condition;
      private Thread _thread;

      public BlockUntilCondition(System.Func<bool> condition)
      {
        _condition = condition;
      }

      private void Test()
      {
        while (!_condition())
        {
          Thread.Sleep(5);
        }
      }

      public void Block()
      {
        _thread = new Thread(Test);
        _thread.Start();
        _thread.Join();
        _thread = null;
      }
    }
  } // namespace Cozmo
} // namespace Anki