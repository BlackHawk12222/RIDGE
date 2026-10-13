"""Auto Configuration Package. Call start() and fill inputs."""

from .ASAP import *
from .CLEARv2 import *
from vex import *

# Filler function.
def _none():
    pass

brain=Brain()
cla: Thread

# Function for the driver control.
def ACP_driver():

    print("Driver function called")

    # Load the config file and set the variables for the driver control.
    variabledata= brain.sdcard.loadfile("ACPv1/Config/ACPv1config.txt").decode("utf-8")
    variabledata=variabledata.split("\n")

    # Initializeing motor lists.
    LeftMotors: list[Motor]=[]
    RightMotors: list[Motor]=[]

    # Loop through the config file and set the variables for the driver control.
    for line in variabledata:
        if "LeftMotors" in line:
            LeftMotorsStr: list[str]=eval(line.split(": ")[1])
            for motor in LeftMotorsStr:
                LeftMotors.append(eval(motor))
        elif "RightMotors" in line:
            RightMotorsStr: list[str]=eval(line.split(": ")[1])
            for motor in RightMotorsStr:
                RightMotors.append(eval(motor))
        elif "GearRatio" in line:
            GearRatio=float(line.split(": ")[1])
        elif "Wheelsize_MM" in line:
            Wheelsize_MM=float(line.split(": ")[1])
        elif "MotorMax_RPM" in line:
            MotorMax_RPM=int(line.split(": ")[1])
        elif "OdomWheelSize_MM" in line:
            OdomWheelSize_MM=float(line.split(": ")[1])
        elif "StickType" in line:
            StickType=line.split(": ")[1]
        elif "controller" in line:
            controller: Controller=eval(line.split(": ")[1])
        elif "Xodom" in line:
            XOdom: Rotation=eval(line.split(": ")[1])
        elif "inertial" in line:
            inertial: Inertial=eval(line.split(": ")[1])

    # Start the ASAP function with the variables from the config file.
    inertial.calibrate()

    print("Variable selection suscessful, starting ASAP with vars %s, %s, %s, %s, %s, %s, %s, %s, %s, %s"%(LeftMotors, RightMotors, GearRatio, Wheelsize_MM, MotorMax_RPM, controller, XOdom, OdomWheelSize_MM, StickType, inertial))

    Start(LeftMotors, RightMotors, GearRatio, Wheelsize_MM, MotorMax_RPM, controller, XOdom, OdomWheelSize_MM, StickType, inertial)

# Primary start method for the ACPv1 package. Call this in main.py to start the package.
def start(GearRatio, Wheelsize_MM, MotorMax_RPM, OdomWheelSize_MM, StickType="Tank", AtonFunc=_none) -> Competition:
    global comp, cla

    ObjList=dir()  # Get a list of all objects in the current scope.

    # Initialize motor lists as strings.
    RightMotors: list[str]=[]
    LeftMotors: list[str]=[]

    # Auto finds the motors, controller, Inertial, and odometry sensor in the code and saves them to a config file for later use.
    for item in ObjList:
        try:
            item_type=str(type(eval(item)))
        except NameError:
            continue
        
        if item_type == "<class 'controller'>":
            controller=item
        elif item_type == "<class 'motor'>":
            if "Right" in item or "right" in item:
                RightMotors+=[item]
            elif "Left" in item or "left" in item:
                LeftMotors+=[item]
        elif item_type == "<class 'rotation'>" and ("Xodom" in item or "XOdom" in item or "xodom" in item):
            XOdom=item
        elif item_type == "<class 'inertial'>":
            inertial=item
    
    del ObjList

    brain.sdcard.savefile("ACPv1/Config/ACPv1config.txt", bytearray(b"LeftMotors: %s\n RightMotors: %s\n GearRatio: %1.5f \n Wheelsize_MM: %1.5f \n MotorMax_RPM: %d \n controller: %s \n Xodom: %s \nOdomWheelSize_MM: %1.5f \n StickType: %s \n inertial: %s"%(LeftMotors, RightMotors, GearRatio, Wheelsize_MM, MotorMax_RPM, controller, XOdom, OdomWheelSize_MM, StickType, inertial)))

    # Start the driver control and the autonomous function.
    comp=Competition(ACP_driver, AtonFunc)

    # Start the CLA thread for the CLEARv2 package.
    print(dir(CLEARv2.CLA))
    cla=CLEARv2.CLA.CLAStart()

    return comp