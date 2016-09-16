#!/usr/bin/env python3
"""
Python command line interface for Robot over the network
"""

import sys, os, json, webbrowser
sys.path.insert(0, os.path.join("tools"))

from flask import Flask, make_response, send_from_directory, request

import robotInterface, minipegReceiver

import logging
log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)

def Remote():
    global LastCameraImage

    LastCameraImage = b''

    app = Flask(__name__)

    @app.route('/motors', methods=['POST'])
    def motors():
        message = json.loads(request.data.decode("utf-8"))

        robotInterface.Send(robotInterface.RI.EngineToRobot(drive=robotInterface.RI.DriveWheels(message['left'], message['right'])))
        robotInterface.Send(robotInterface.RI.EngineToRobot(moveLift=robotInterface.RI.MoveLift(message['lift'])))
        robotInterface.Send(robotInterface.RI.EngineToRobot(moveHead=robotInterface.RI.MoveHead(message['head'])))

        return "ok."

    @app.route('/lights', methods=['POST'])
    def lights():
        message = json.loads(request.data.decode("utf-8"))
        lights = robotInterface.RI.BackpackLightsMiddle()

        color = (int(message['red']   * 0x1F) << 10) + (int(message['green'] * 0x1F) << 5) + (int(message['blue']  * 0x1F) << 0)

        for light in lights.lights:
            light.onColor = light.offColor = color

        robotInterface.Send(robotInterface.RI.EngineToRobot(setBackpackLightsMiddle=lights))

        return "ok."

    @app.route('/camera')
    def camera():
        global LastCameraImage
        response = make_response(LastCameraImage)
        response.headers['Content-Type'] = 'image/jpeg'
        response.headers['Content-Disposition'] = 'attachment; filename=camera.jpg'
        return response

    @app.route('/static/<path:path>')
    def send_js(path):
        return send_from_directory('static', path)

    @app.route('/')
    def static_page():
        return send_from_directory('static', "index.html")

    def receiveImage(img, size):
        global LastCameraImage
        LastCameraImage = img

    
    robotInterface.Init(False)
    imageReceiver = minipegReceiver.MinipegReceiver(receiveImage)

    robotInterface.Connect(imageRequest=True)
    webbrowser.open("http://127.0.0.1:5000/")
    app.run()
    robotInterface.Send(robotInterface.RI.EngineToRobot(stop=robotInterface.RI.StopAllMotors()))
    robotInterface.Die()

if __name__ == '__main__':
    Remote()
