#!/usr/bin/env python3
"""
Python command line interface for Robot over the network
"""

import sys, os, time, json
sys.path.insert(0, os.path.join("tools"))
import robotInterface, fota, animationStreamer, minipegReceiver
import threading
from io import BytesIO
from flask import Flask
import asyncio
import websockets

client = """<html>
    <head>
        <title>FARTS</title>
    </head>
    <body>
        <script>
      var ws = new WebSocket("ws://127.0.0.1:8765/");
      var img = document.createElement("img");
      var motors = { left: 0, right: 0 }
      document.body.appendChild(img);

      function calcSpeed(motors) {
        var left = 0, right = 0;
        if (motors.up) {
          left  += 100.0;
          right += 100.0;
          }
          if (motors.down) {
          left  -= 100.0;
          right -= 100.0;
        }
          if (motors.left) {
          left  -= 100.0;
          right += 100.0;
        }
          if (motors.right) {
          left  += 100.0;
          right -= 100.0;
        }

        return { left, right }
      }

      document.addEventListener("keydown", function (e) {
        switch (e.keyCode) {
            case 38: // up
                motors.up = true;
                break ;
            case 40: // down
                motors.down = true;
                break ;
            case 37: // left
                motors.left = true;
                break ;
            case 39: // right
                motors.right = true;
                break ;
        }
        
        ws.send(JSON.stringify(calcSpeed(motors)));
      });

      document.addEventListener("keyup", function (e) {
        switch (e.keyCode) {
            case 38: // up
                motors.up = false;
                break ;
            case 40: // down
                motors.down = false;
                break ;
            case 37: // left
                motors.left = false;
                break ;
            case 39: // right
                motors.right = false;
                break ;
        }

        ws.send(JSON.stringify(calcSpeed(motors)));
      });

      ws.onmessage = function (event) {
                var reader  = new FileReader();

                reader.addEventListener("load", function () {
                    img.src = reader.result;
                }, false);

                reader.readAsDataURL(event.data);
      };
        </script>
    </body>
</html>
"""

class WebServer(threading.Thread):
    def run(self):
        app = Flask(__name__)

        @app.route('/')
        def static_page():
            return client

        app.run()

class Remote:
    def __init__(self):
        self.upgrader = None
        self.animStreamer = None
        self.images = []

        robotInterface.Init(False)

        self.imageReceiver = minipegReceiver.MinipegReceiver(self.receiveImage)
        self.run()

    def receiveImage(self, img, size):
        self.images = self.images + [img]

    async def producer(self):
        while len(self.images) < 1:
            asyncio.sleep(0.01)
        image, self.images = self.images[0], self.images[1:]

        return image

    async def consumer(self, message):
        message = json.loads(message)
        robotInterface.Send(robotInterface.RI.EngineToRobot(drive=robotInterface.RI.DriveWheels(message['left'], message['right'])))

    async def socket_loop(self, websocket, path):
        while True:
            listener_task = asyncio.ensure_future(websocket.recv())
            producer_task = asyncio.ensure_future(self.producer())
            
            done, pending = await asyncio.wait(
                [listener_task, producer_task],
                return_when=asyncio.FIRST_COMPLETED)

            if listener_task in done:
                message = listener_task.result()
                await self.consumer(message)
            else:
                listener_task.cancel()

            if producer_task in done:
                message = producer_task.result()
                await websocket.send(message)
            else:
                producer_task.cancel()

    def flask(self):
        thread = threading.Thread()        

    def run(self):
        robotInterface.Connect()
        webserver = WebServer()
        webserver.start()

        start_server = websockets.serve(self.socket_loop, 'localhost', 8765)

        asyncio.get_event_loop().run_until_complete(start_server)
        asyncio.get_event_loop().run_forever()

        robotInterface.Send(robotInterface.RI.EngineToRobot(stop=robotInterface.RI.StopAllMotors()))
        robotInterface.Die()

if __name__ == '__main__':
    Remote()
