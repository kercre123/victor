# Using the initial Wifi Demo Kit Scripts

### If you just want to connect to Victor and don't care about logging, run:
`./connect_to_victor.sh <Robot ID> <SSID> <password>`


### To connect while logging, do one of the following. Make sure you're in the wifi\_demo\_kit directory

`./gather_wifi_info.sh <List>`: Use this if you'd like to just list your currently available wifi to the console. Will then exit.

`./gather_wifi_info.sh <Robot ID> <SSID> <password>`: Will log all output to console and in a timestamped log file that'll appear in the Wifi\_Demo\_Kit folder. Will then attempt to establish a bluetooth connection first, have Victor scan all available wifi with a count and log, and then attempt to connect. Will make 5 attempts before exiting out. 

### Known Issues:
1. The logs will have Bash escape charectors where ever the command prompt appears (if viewed in text editor) and makes it annoying to read. **Workaround** View the file in the terminal (cat, less, etc)

2. Everytime you run the gather\_wifi\_info script, you'll get a new log. Should probably be a paramter to not log information. Or just run the connect\_to\_victor script

