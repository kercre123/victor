#!/usr/bin/env python3
"""
Python command line interface for Robot over the network
"""

import sys, os, time, pygame, threading
if sys.version_info.major < 3:
    #sys.stderr.write("Python 2 is depricated." + os.linesep)
    from StringIO import StringIO as BytesIO
else:
    from io import BytesIO
sys.path.insert(0, os.path.join("tools"))
import robotInterface, fota, animationStreamer, minipegReceiver

class Remote:
    def receiveImage(self, img, size):
        self.image = pygame.image.load(BytesIO(img), ".jpg")

    def __init__(self):
        self.upgrader = None
        self.animStreamer = None
        self.image = None
        robotInterface.Init(False)
        pygame.init()
        self.imageReceiver = minipegReceiver.MinipegReceiver(self.receiveImage)
        self.run()

    def run(self):
        """Container for remote "game" under pygame."""

        robotInterface.Connect()

        screen_size = (640, 480)
        background = (207, 176, 123)
        screen = pygame.display.set_mode(screen_size)
        renderedGreen = False
        drive = False
        lift  = False
        head  = False
        jogging = False
        running = True
        while running:
            try:
                if robotInterface.GetState() == robotInterface.ConnectionState.disconnected:
                    running = False
                event = pygame.event.poll()
                if event.type == pygame.QUIT:
                    running = False
                elif event.type in (pygame.KEYDOWN, pygame.KEYUP):
                    key = pygame.key.get_pressed()
                    if key[pygame.K_q]: # q
                        running = False
                    elif key[pygame.K_t]: # T
                        if self.animStreamer is None:
                            self.animStreamer = animationStreamer.ToneStreamer()
                            self.animStreamer.start(800)
                        else:
                            self.animStreamer.stop()
                            time.sleep(0.1)
                            self.animStreamer = None
                    elif key[pygame.K_u]:
                        self.upgrader = fota.Upgrader()
                        threading.Thread(target=fota.UpgradeAll, args=(self.upgrader,), name="UpgradeAll").run()
                    elif key[pygame.K_j]: # J
                        robotInterface.Send(robotInterface.RI.EngineToRobot(enterTestMode=robotInterface.RI.EnterFactoryTestMode(robotInterface.RI.FactoryTestMode.FTM_None if jogging else robotInterface.RI.FactoryTestMode.FTM_motorLifeTest)))
                        jogging = not jogging
                    else:
                        left = 0.0
                        right = 0.0
                        if key[pygame.K_UP]: # up arrow
                            left  += 100.0
                            right += 100.0
                            drive = True
                        if key[pygame.K_DOWN]: # down arrow
                            left  -= 100.0
                            right -= 100.0
                            drive = True
                        if key[pygame.K_LEFT]: # left arrow
                            left  -= 100.0
                            right += 100.0
                            drive = True
                        if key[pygame.K_RIGHT]: # right arrow
                            left  += 100.0
                            right -= 100.0
                            drive = True
                        if drive:
                            robotInterface.Send(robotInterface.RI.EngineToRobot(drive=robotInterface.RI.DriveWheels(left, right)))
                            if left == 0.0 and right == 0.0: drive = False

                        if key[pygame.K_w]: # W
                            robotInterface.Send(robotInterface.RI.EngineToRobot(moveLift=robotInterface.RI.MoveLift(+1.0)))
                            lift = True
                        elif key[pygame.K_s]: # S
                            robotInterface.Send(robotInterface.RI.EngineToRobot(moveLift=robotInterface.RI.MoveLift(-1.0)))
                            lift = True
                        elif lift:
                            robotInterface.Send(robotInterface.RI.EngineToRobot(moveLift=robotInterface.RI.MoveLift(0.0)))
                            lift = False

                        if key[pygame.K_e]: # E
                            robotInterface.Send(robotInterface.RI.EngineToRobot(moveHead=robotInterface.RI.MoveHead(+1.5)))
                            head = True
                        elif key[pygame.K_d]: # D
                            robotInterface.Send(robotInterface.RI.EngineToRobot(moveHead=robotInterface.RI.MoveHead(-1.5)))
                            head = True
                        elif head:
                            robotInterface.Send(robotInterface.RI.EngineToRobot(moveHead=robotInterface.RI.MoveHead(0.0)))
                            head = False

                if self.image is None or renderedGreen is False:
                    if robotInterface.GetState() == robotInterface.ConnectionState.connected:
                        screen.fill((0, 255, 0))
                        renderedGreen = True
                    else:
                        screen.fill((255, 0, 0))
                        renderedGreen = False
                elif renderedGreen:
                    screen.blit(self.image, (0,0))
                time.sleep(0.05)
                pygame.display.flip()
            except Exception as e:
                running = False
                sys.stderr.write(str(e) + "\r\n")

        robotInterface.Send(robotInterface.RI.EngineToRobot(stop=robotInterface.RI.StopAllMotors()))
        robotInterface.Die()


if __name__ == '__main__':
    Remote()
