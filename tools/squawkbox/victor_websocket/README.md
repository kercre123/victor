# Victor Websocket Class, using Python 3

## Introduction:

The Python 3 Victor Websocket class connects to a Victor unit's websocket interface. Currently, when Victor turns on, and is in the Anki5Ghz network, it will automatically assign itself an IP address. Once an IP address is established, a webserver will automatically turn on, and you can access it on your phone/computers web browser, using the following URL

`http://<victor_ip>:<port_num>/webViz.html`

Where:

1.  `victor_ip`: is the IP address of your Victor unit. You can find this be pressing Victor's backpack button to navigate the different debug screens. This most likely only works on a debug build

2. `port_num`: the port that accesses the different data types, which are

	* **8888**: Behaviours, Intents, ObservedObjects, and Vision
	* **8889**: Animations and MicData

Accessing this through the browser will give you a nice visual representation of the different data, however if you want to consistently monitor data and log them, with out having to manually look, you would need a way to access Victor's websocket, which is what is pushing data as a stream.

To do this in a more convenient manner, this python 3 class will establish a websocket connection (as a background thread), request data for the data type that you want, record data when you set the instance flag variable to `True` or `False`, and then compile and return said data when asked. 

## Requirements:
1. Python 3.5 +
2. Websockets client library `pip3 install websocket-client`
3. Victor robot and its IP address
4. On the Anki5GHz network, or VPN 

As of now, the only data type currently available is MicData, which gives the most dominant direction and its confidence.

## Example Code:
Here is an example of establishing a websocket connection, recording and compiling mic data and then displaying 

```
import VictorWebsocket as vs
import time

mic_data = vs.VictorWebsocket("192.168.43.54")

while test.mic_socket_established == False:
  print("Waiting for websocket")
  time.sleep(2)
print("connection established")

mic_data.mic_stream_flag = True

#Do stuff, mic_data will be recording

mic_data.mic_stream_flag = False

results = mic_data.compile_mic_results()

print(results)

mic_data.close_mic_socket()
```

`results`: `{'final_dominant': 9, 'final_confidence': 150.22857142857143}`

## Details
1. Initialize by doing `mic_websocket = VictorWebsocket(<robot_ip)>`

2. `mic_websocket.mic_socket_established`, perform a check to make sure this flag is True. Currently the socket establishes the connection in the background and your main program may start with out the connection being established.

3. `mic_data.mic_stream_flag = True`, set this to true to start recording the dominant direction and confidence. This will be appended in the class instance variable `mic_data_raw_results`, until you set this flag back to `False`.

4. `results = mic_data.compile_mic_results()` , this will find the most common direction and its average confidence level while the mic data was being recorded. It will then empty the `mic_data_raw_results` instance variable for the next time. This output will be a dict:
`{'final_dominant': 9, 'final_confidence': 150.22857142857143}`

5. `mic_data.close_socket()`. This is a safer way to close the websocket connection. Technically, the websocket is running as a daemon thread, which means that if daemons are the last resources being run in the main application, it will still close safely. But better safe than sorry.
 


