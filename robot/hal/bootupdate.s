                AREA    |.text|, DATA, READONLY
                ALIGN   4

                EXPORT  CurrentRobotBootloader
CurrentRobotBootloader
                INCBIN  ../binaries/robot_boot.bin

                END
