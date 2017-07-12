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

// This program is a non-exhasutive example of how to leverage clad through csharp.
//
// The following message interaction paradigms are demonstrated:
//  - sending the "SetLiftHeight" message inside a cancellable action wrapper
//  - sending the "RequestAvailableAnimations" message, and registering for its associated callbacks
//
// Some messages are meant to be wrapped in actions, others are single calls meant to set robot state,
// and some are designed to be used with callbacks.  Please refer to the python sdk for further specific examples.
//

using System.Collections.Generic;

public class SampleApp
{
  public const string kTargetIP = "127.0.0.1";
  public const int kTargetPort = 5106;

  private Anki.Cozmo.Connection _connection;
  private List<string> _supportedAnimations = new List<string>();

  private void OnAnimationAvailable(Anki.Cozmo.ExternalInterface.AnimationAvailable message)
  {
    _supportedAnimations.Add(message.animName);
  }

  private void RequestSupportedAnimations()
  {
    // clear list and register for messages
    _supportedAnimations.Clear();
    bool finished = false;
    _connection.AddCallback<Anki.Cozmo.ExternalInterface.AnimationAvailable>(this, OnAnimationAvailable);
    _connection.AddCallback<Anki.Cozmo.ExternalInterface.EndOfMessage>(this, msg => finished = true );

    Anki.Cozmo.ExternalInterface.RequestAvailableAnimations message = new Anki.Cozmo.ExternalInterface.RequestAvailableAnimations();
    _connection.SendMessage( message );

    Anki.Cozmo.BlockUntilCondition blocker = new Anki.Cozmo.BlockUntilCondition(() => finished);
    blocker.Block();

    // unregister for messages
    _connection.RemoveCallback<Anki.Cozmo.ExternalInterface.AnimationAvailable>(this);
    _connection.RemoveCallback<Anki.Cozmo.ExternalInterface.EndOfMessage>(this);
  }

  private Anki.Cozmo.Action SetLiftHeight(float percent)
  {
  	// lift: (32 mm is the minimum height, 92 mm is the maximum height)
  	float height_mm = 32.0f + percent * 60.0f;

  	Anki.Cozmo.ExternalInterface.SetLiftHeight message = new Anki.Cozmo.ExternalInterface.SetLiftHeight(height_mm: height_mm, max_speed_rad_per_sec: 10.0f, accel_rad_per_sec2: 10.0f, duration_sec: 2.0f);

  	return _connection.SendAction(message);
  }

  public void Execute()
  {
    _connection = new Anki.Cozmo.Connection(kTargetIP, kTargetPort);

    RequestSupportedAnimations();
    System.Console.WriteLine("Cozmo can perform " + _supportedAnimations.Count.ToString() + " animations");

    SetLiftHeight(1.0f).WaitForCompleted();
    SetLiftHeight(0.0f).WaitForCompleted();

    _connection.Close();
  }

	static void Main(string[] args)
	{
    SampleApp app = new SampleApp();
    app.Execute();
	}
}