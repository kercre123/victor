#!/usr/bin/env python3
"""
Python command line interface for Robot over the network
"""

import sys, os, time, pygame
sys.path.insert(0, os.path.join("tools"))
import robotInterface, fota, animationStreamer

class Remote:
    def __init__(self):
        self.upgrader = None
        self.animStreamer = None
        robotInterface.Init()
        pygame.init()
        self.run()
    
    def pwm(self, params):
        "Send a DriveWheels message to the robot. Args: <left wheel speed> <right wheel speed>"
        if robotInterface.GetState() == robotInterface.ConnectionState.connected:
            robotInterface.Send(robotInterface.RI.EngineToRobot(setRawPWM=robotInterface.Anki.Cozmo.RawPWM(params)))
    
    def run(self):
        """Container for remote "game" under pygame."""
        PWM_VAL = 20000
        PWM_JOG = 0x3fff
        
        robotInterface.Connect()
        
        screen_size = (640, 480)
        background = (207, 176, 123)
        screen = pygame.display.set_mode(screen_size)
        
        cmd = None
        lastCmdTime = 0
        lastJogTime = None
        running = True
        while running:
            try:
                if robotInterface.GetState() == robotInterface.ConnectionState.disconnected:
                    running = False
                event = pygame.event.poll()
                if event.type == pygame.QUIT:
                    running = False
                elif event.type == pygame.KEYDOWN:
                    key = pygame.key.get_pressed()
                    lastJogTime = None
                    cmd = None
                    if key[pygame.K_q]: # q
                        running = False
                    elif key[pygame.K_UP]: # up arrow
                        cmd = (+PWM_VAL, +PWM_VAL, 0, 0)
                    elif key[pygame.K_DOWN]: # down arrow
                        cmd =(-PWM_VAL, -PWM_VAL, 0, 0)
                    elif key[pygame.K_LEFT]: # left arrow
                        cmd =(-PWM_VAL, +PWM_VAL, 0, 0)
                    elif key[pygame.K_RIGHT]: # right arrow
                        cmd =(+PWM_VAL, -PWM_VAL, 0, 0)
                    elif key[pygame.K_w]: # W
                        cmd =(0, 0, PWM_VAL, 0)
                    elif key[pygame.K_s]: # S
                        cmd =(0, 0, -PWM_VAL, 0)
                    elif key[pygame.K_e]: # E
                        cmd =(0, 0, 0, PWM_VAL)
                    elif key[pygame.K_d]: # D
                        cmd =(0, 0, 0, -PWM_VAL)
                    elif key[pygame.K_j]: # J
                        lastJogTime = time.time()
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
                        fota.UpgradeAll(self.upgrader, wifiImage="espressif.bin", rtipImage="robot.safe", bodyImage="syscon.safe")
                    else:
                        sys.stdout.write("Key = {}\r\n".format(event.scancode))
                    lastCmdTime = 0
                elif event.type == pygame.KEYUP and cmd is not None:
                    cmd = (0, 0, 0, 0)
                    lastCmdTime = 0
                
                if self.upgrader is None:
                    now = time.time()
                    if lastJogTime is not None:
                        if (now - lastJogTime) < 1.0:
                            cmd = (PWM_JOG,)*4
                        elif (now - lastJogTime) < 2.0:
                            cmd = (-PWM_JOG,)*4
                        else:
                            lastJogTime = now
                    
                    if now - lastCmdTime > 0.100 and cmd is not None:
                        self.pwm(cmd)
                        lastCmdTime = now
            
                screen.fill((0, 255, 0) if robotInterface.GetState() == robotInterface.ConnectionState.connected else (255, 0, 0))
                pygame.display.flip()
            except Exception as e:
                running = False
                sys.stderr.write(str(e) + "\r\n")
        
        self.pwm((0, 0, 0, 0))
        robotInterface.Die()


if __name__ == '__main__':
    Remote()
