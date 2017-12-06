# Using the initial Wifi Demo Kit Scripts

Here is the script to establish a wifi connection for Victor, using its onboard bluetooth for passing configs: 
`./connect_to_victor.sh <Robot ID> <SSID> <password>`

### The script will:

1. Look for Victor (based on `Robot ID`) using bluetooth 
2. Once the correct Victor is found, will then attempt to establish a bluetooth connection. 
3. Once a bluetooth connection is established, will then attempt to set up a wifi connection based on `SSID` and `password`
4. If a wifi connection is established between Victor and network, will then test to make sure properly receiving and transmitting information

