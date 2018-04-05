Microphone Capture Information

Note these instructions are based on using the DVT2 robots.

To clear mic data from the robot if desired:
 - Connect to the robot when on the same network: adb connect 192.168.X.X
 - Give the command to delete all the captured mic data: ./project/victor/scripts/clear_micdata.sh

To create recordings saved to the robot:
 - To save to the "triggeredCapture" folder, say "Hey Cozmo" + whatever you want to record (the "Hey Cozmo" is not captured)
 - To save to the "debugCapture" folder, go to the mic data debug screen (using Victor's button), and move the lift to the top. This will start a 15 second recording, and will keep starting recordings until the lift is moved down. The face displays the time remaining on the current recording.
   - Note each "debugCapture" recording will also contain the raw unprocessed microphone capture, suffixed with "_raw"

To pull the recordings from the robot:
 - Connect to the robot when on the same network: adb connect 192.168.X.X
 - Use the script that pulls all the stored mic captures from the robot: ./project/victor/scripts/pull_micdata.sh
 - This script creates a new folder in Downloads named with the date and time, and copies the microphone captures to it
