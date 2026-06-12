#region VEXcode Generated Robot Configuration
from vex import *
import RE

# Brain should be defined by default
brain=Brain()

def none():
    pass

# Robot configuration code
controller_1 = Controller(PRIMARY)
Right1 = Motor(Ports.PORT18, GearSetting.RATIO_6_1, True)
Right2 = Motor(Ports.PORT19, GearSetting.RATIO_6_1, False)
Right3 = Motor(Ports.PORT20, GearSetting.RATIO_6_1, True)
left1 = Motor(Ports.PORT11, GearSetting.RATIO_6_1, False)
left2 = Motor(Ports.PORT12, GearSetting.RATIO_6_1, True)
left3 = Motor(Ports.PORT13, GearSetting.RATIO_6_1, False)
inertial = Inertial(Ports.PORT3)
Xodom = Rotation(Ports.PORT1)
Yodom = Rotation(Ports.PORT2)

# wait for rotation sensor to fully initialize
wait(30, MSEC)

# add a small delay to make sure we don't print in the middle of the REPL header
wait(200, MSEC)
# clear the console to make sure we don't have the REPL in the console
print("\033[2J")

#endregion VEXcode Generated Robot Configuration
screen_precision = 0
console_precision = 0
pusher_state = False
loader_state = False
record = 0
recording_state=0
pushing=False

# ---------------------------------------------------------------------------- #
#                                                                              #
# 	Module:       CLEAR.py                                                     #
# 	Author:       Micah Bow                                                    #
# 	Created:      1/27/2026, 12:42 PM                                          #
#   Last Edited:  2/23/2026, 10:00 PM                                          #
# 	Description:  Capture, logging, Encoding, Archiving, Recording.            #
#                                                                              #
# ---------------------------------------------------------------------------- #

Right1.set_stopping(HOLD)
Right2.set_stopping(HOLD)
left1.set_stopping(HOLD)
left2.set_stopping(HOLD)
inertial.calibrate()

# Driver Control Functions

def rightside():
    rightspeed = controller_1.axis2.position() / 8.33
    Right1.spin(FORWARD, rightspeed, VOLT)
    Right2.spin(FORWARD, rightspeed, VOLT)

def leftside():
    leftspeed = controller_1.axis3.position() / 8.33
    left1.spin(FORWARD, leftspeed, VOLT)
    left2.spin(FORWARD, leftspeed, VOLT)

# Aton Functions

def leftmove(leftspeed):
    print("left: ", leftspeed)
    if leftspeed != 0:
        leftspeed=leftspeed/8.33
    left1.spin(FORWARD, leftspeed, VOLT)
    left2.spin(FORWARD, leftspeed, VOLT)

def rightmove(rightspeed):
    print("Right: ", rightspeed)
    if rightspeed != 0:
        rightspeed=rightspeed/8.33
    Right1.spin(FORWARD, rightspeed, VOLT)
    Right2.spin(FORWARD, rightspeed, VOLT)

def aton():
    RE.encode.run("TestRecording")

comp=Competition(none, aton)

def state_test():
    global pushing
    pushing=True
    print("Pushing: ", pushing)

if brain.sdcard.is_inserted() and brain.sdcard.exists("CLA.py"):

    rightmotorlist=["Right1", "Right2", "Right3"]
    leftmotorlist=["left1", "left2", "left3"]
    import CLA
    CLA.log.start()
    
    def toggle_recording():
        global recording_state
        if recording_state == 0:
            recording_state = 1
            Thread(lambda: RE.recording.start(controller_1, Right1, left1))
            controller_1.rumble("-")
            print("Recording Started")
        else:
            recording_state = 0
            RE.recording.stop("TestRecording", rightmotorlist, leftmotorlist)
            controller_1.rumble("--")
            print("Recording Stopped")
    controller_1.buttonA.pressed(toggle_recording)
    
    import OD
    ODMainLoop=OD.OD.StartOD(inertial, left1, Right1, 1, 69.85, Xodom, Yodom, 76.2, 81, 1)

    def print_location_loop():
        while True:
            print("X: ", OD.OD.XPosition, " Y: ", OD.OD.YPosition, " Heading: ", OD.OD.HeadingI)
            wait(100, MSEC)
            
    ODLoop=Thread(print_location_loop)

# Event setup
controller_1.axis2.changed(rightside)
controller_1.axis3.changed(leftside)
controller_1.buttonL1.pressed(state_test)