#region VEXcode Generated Robot Configuration
from vex import *
import ACP

# Brain should be defined by default
brain=Brain()

# Robot configuration code
controller_1 = Controller(PRIMARY)
Right1 = Motor(Ports.PORT13, GearSetting.RATIO_6_1, True)
Right2 = Motor(Ports.PORT14, GearSetting.RATIO_6_1, False)
left1 = Motor(Ports.PORT11, GearSetting.RATIO_6_1, False)
left2 = Motor(Ports.PORT12, GearSetting.RATIO_6_1, True)
OtherMotor=Motor(Ports.PORT2)
inertial = Inertial(Ports.PORT4)
Xodom = Rotation(Ports.PORT1)

# add a small delay to make sure we don't print in the middle of the REPL header
wait(200, MSEC)
# clear the console to make sure we don't have the REPL in the console
print("\033[2J")

# ---------------------------------------------------------------------------- #
#                                                                              #
# 	Module:       CLEAR.py                                                     #
# 	Author:       Micah Bow                                                    #
# 	Created:      1/27/2026, 12:42 PM                                          #
#   Last Edited:  2/23/2026, 10:00 PM                                          #
# 	Description:  Capture, logging, Encoding, Archiving, Recording.            #
#                                                                              #
# ---------------------------------------------------------------------------- #

test=1

inertial.calibrate()

#Future aton test
def aton():
    pass

print(dir(test))

# Aton Functions
comp=ACP.start(GearRatio=0.75, Wheelsize_MM=69.85, MotorMax_RPM=600, OdomWheelSize_MM=50.8, StickType="Arcade", AtonFunc=aton)

test=2