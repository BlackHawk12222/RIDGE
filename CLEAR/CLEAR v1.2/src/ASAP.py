#Anti Slip Asyncronis Protocol
from vex import *

import uasyncio

def Start(LeftMotorList: list[Motor], RightMotorList: list[Motor], GearRatio: float, WheelSize_MM: float, MotorRpmMax: int, Controller: Controller, XOdom: Rotation, OdomWheelSize_MM, StickType: str):
    RunLoop=uasyncio.create_task(_run(LeftMotorList, RightMotorList, GearRatio, WheelSize_MM, MotorRpmMax, Controller, XOdom, OdomWheelSize_MM, StickType))
    return RunLoop

async def _run(LeftMotorList: list[Motor], RightMotorList: list[Motor], GearRatio: float, WheelSize_MM: float, MotorRpmMax: int, Controller: Controller, XOdom: Rotation, OdomWheelSize_MM, StickType: str):
    timer=Timer()

    while True:
        StartTime=timer.time()

        if StickType == "Tank" or StickType == "tank":
            RightPos = Controller.axis2.position()
            LeftPos = Controller.axis3.position()

            LeftSpeed=((LeftMotorList[0].velocity(RPM)*((2*3.14159)/60))/GearRatio)*WheelSize_MM
            RightSpeed=((RightMotorList[0].velocity(RPM)*((2*3.14159)/60))/GearRatio)*WheelSize_MM
            Truespeed=XOdom.velocity(RPM)*((2*3.14159)/60)*OdomWheelSize_MM

            sliprate=((Truespeed-((LeftSpeed+RightSpeed)/2))/Truespeed)*100

            SpeedOffsetLeft=min(sliprate-100, 0)
            SpeedOffsetRight=min(sliprate-100, 0)

            ControllerMultipyer=MotorRpmMax/100

            for motor in LeftMotorList:
                motor.spin(FORWARD, (LeftPos-SpeedOffsetLeft)*ControllerMultipyer, RPM)
            
            for motor in RightMotorList:
                motor.spin(FORWARD, (RightPos-SpeedOffsetRight)*ControllerMultipyer, RPM)
            
            await uasyncio.sleep_ms(20 - StartTime)
        elif StickType == "Arcade" or StickType == "arcade":
            RightPos = Controller.axis3.position() - Controller.axis4.position()
            LeftPos = Controller.axis3.position() + Controller.axis4.position()

            LeftSpeed=((LeftMotorList[0].velocity(RPM)*((2*3.14159)/60))/GearRatio)*WheelSize_MM
            RightSpeed=((RightMotorList[0].velocity(RPM)*((2*3.14159)/60))/GearRatio)*WheelSize_MM
            Truespeed=XOdom.velocity(RPM)*((2*3.14159)/60)*OdomWheelSize_MM

            sliprate=((Truespeed-((LeftSpeed+RightSpeed)/2))/Truespeed)*100

            SpeedOffsetLeft=min(sliprate-100, 0)
            SpeedOffsetRight=min(sliprate-100, 0)

            ControllerMultipyer=MotorRpmMax/100

            for motor in LeftMotorList:
                motor.spin(FORWARD, (LeftPos-SpeedOffsetLeft)*ControllerMultipyer, RPM)
            
            for motor in RightMotorList:
                motor.spin(FORWARD, (RightPos-SpeedOffsetRight)*ControllerMultipyer, RPM)
            
            await uasyncio.sleep_ms(20 - StartTime)

