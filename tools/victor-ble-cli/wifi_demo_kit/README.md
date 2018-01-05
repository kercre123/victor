# Using the initial Wifi Demo Kit Scripts

Here is the script to establish a wifi connection for Victor, using USB and adb for passing configs: 
`./wifi_main.sh <SSID> <SSID password>`
 
### The script will:
 
1. Have user confirm that the `SSID` and `SSID password` is correct before moving forward
1. Ask the user where the router is currently located
2. Ask user what room they are currently in
3. Create a directory for that room, which will contain four logs highlighting results of: all available nearby wifi access points, connecting Victor to provided wifi network, and performance results of said connection both local and to the cloud
3. Will first scan for all available nearby wifi access points and log results in `wifi_list.log`
4. Will then attempt a connection between Victor and wifi network, based on provided SSID and password. Results will be logged into `connect.log`
5. If a wifi connection is established between Victor and the laptop, will then test to make sure properly receiving and transmitting information. Results will be logged into `performance_local.log`
5. If a wifi connection is established between Victor and the anki cloud server, will then test to make sure properly receiving and transmitting information. Results will be logged into `performance_cloud.log`
6. User will then be prompted to input the next room they will be using for testing and repeate the steps from 3 again. Will keep prompting until asked to quit

Here is the script to establish a wifi connection for Victor, using its onboard bluetooth for passing configs: 
`./connect_to_victor.sh <Robot ID> <SSID> <password>`

### The script will:

1. Look for Victor (based on `Robot ID`) using bluetooth 
2. Once the correct Victor is found, will then attempt to establish a bluetooth connection. 
3. Once a bluetooth connection is established, will then attempt to set up a wifi connection based on `SSID` and `password`
4. If a wifi connection is established between Victor and network, will then test to make sure properly receiving and transmitting information

---
