#Anti Slip Asyncronis Protocol
from vex import *

import uasyncio, LKF, PDC

def Start(LeftMotorList: list[Motor], RightMotorList: list[Motor], GearRatio: float, WheelSize_MM: float, MotorRpmMax: int, Controller: Controller, XOdom: Rotation, OdomWheelSize_MM, StickType: str, Inertial: Inertial):
    RunLoop=uasyncio.create_task(_run(LeftMotorList, RightMotorList, GearRatio, WheelSize_MM, MotorRpmMax, Controller, XOdom, OdomWheelSize_MM, StickType, Inertial))
    return RunLoop

async def _run(LeftMotorList: list[Motor], RightMotorList: list[Motor], GearRatio: float, WheelSize_MM: float, MotorRpmMax: int, Controller: Controller, XOdom: Rotation, OdomWheelSize_MM, StickType: str, Inertial: Inertial):
    timer=Timer()

    while True:
        StartTime=timer.time()

        if StickType == "Tank" or StickType == "tank":
            RightPos = Controller.axis2.position()
            LeftPos = Controller.axis3.position()

            RequestedRightRPM= RightPos*(MotorRpmMax/100)
            RequestedLeftRPM= LeftPos*(MotorRpmMax/100)
            AcutalRightRPM= (RightMotorList[0].velocity(RPM) + RightMotorList[1].velocity(RPM))/2
            AcutalLeftRPM= (LeftMotorList[0].velocity(RPM) + LeftMotorList[1].velocity(RPM))/2

            VelocityDiffrenceRight=RightMotorList[0].velocity(RPM) - RightMotorList[1].velocity(RPM)
            VelocityDiffrenceLeft=LeftMotorList[0].velocity(RPM) - LeftMotorList[1].velocity(RPM)

            LeftWheelSpeed=((LeftMotorList[0].velocity(RPM)*((2*3.14159)/60))/GearRatio)*(WheelSize_MM/1000)
            RightWheelSpeed=((RightMotorList[0].velocity(RPM)*((2*3.14159)/60))/GearRatio)*(WheelSize_MM/1000)

            TrueSpeedFilter=LKF.LinearKalmanFilter(A=LKF.Matrix([[1]]), B=LKF.Matrix([[0.02]]), H=LKF.Matrix([[1/(WheelSize_MM/1000)]]), Q=LKF.Matrix([[0.05]]), R=LKF.Matrix([[0.25]]), x0=LKF.Matrix([[0]]), P0=LKF.Matrix([[9]]))
            TrueSpeedFilter.predict(LKF.Matrix([[Inertial.acceleration(XAXIS)*9.81]]))
            TrueSpeedFilter.update(LKF.Matrix([[XOdom.velocity(RPM)]]))

            TrueSpeed= TrueSpeedFilter.x.data[0][0]

            SlipRateRight=(TrueSpeed-RightWheelSpeed)/TrueSpeed*100
            SlipRateLeft=(TrueSpeed-LeftWheelSpeed)/TrueSpeed*100

            TargetRightRPM=RequestedRightRPM-SlipRateRight
            TargetLeftRPM=RequestedLeftRPM-SlipRateLeft
            AntiFightTargetRightRPM=[TargetRightRPM-((VelocityDiffrenceRight/2)*0.5), TargetRightRPM+((VelocityDiffrenceRight/2)*0.5)]
            AntiFightTargetLeftRPM=[TargetLeftRPM-((VelocityDiffrenceLeft/2)*0.5), TargetLeftRPM+((VelocityDiffrenceLeft/2)*0.5)]

            for i in range(len(LeftMotorList)):
                LeftMotorList[i].spin(FORWARD, PDC.PD("LeftSide", 0.5, 0.1).compute(AntiFightTargetLeftRPM[i], AcutalLeftRPM, 0.02, 12, -12), VOLT)

            for i in range(len(RightMotorList)):
                RightMotorList[i].spin(FORWARD, PDC.PD("RightSide", 0.5, 0.1).compute(AntiFightTargetRightRPM[i], AcutalRightRPM, 0.02, 12, -12), VOLT)

            await uasyncio.sleep_ms(20 - StartTime)
        elif StickType == "Arcade" or StickType == "arcade":
            RightPos = Controller.axis3.position() - Controller.axis4.position()
            LeftPos = Controller.axis3.position() + Controller.axis4.position()

            RequestedRightRPM= RightPos*(MotorRpmMax/100)
            RequestedLeftRPM= LeftPos*(MotorRpmMax/100)
            AcutalRightRPM= (RightMotorList[0].velocity(RPM) + RightMotorList[1].velocity(RPM))/2
            AcutalLeftRPM= (LeftMotorList[0].velocity(RPM) + LeftMotorList[1].velocity(RPM))/2

            VelocityDiffrenceRight=RightMotorList[0].velocity(RPM) - RightMotorList[1].velocity(RPM)
            VelocityDiffrenceLeft=LeftMotorList[0].velocity(RPM) - LeftMotorList[1].velocity(RPM)

            LeftWheelSpeed=((LeftMotorList[0].velocity(RPM)*((2*3.14159)/60))/GearRatio)*(WheelSize_MM/1000)
            RightWheelSpeed=((RightMotorList[0].velocity(RPM)*((2*3.14159)/60))/GearRatio)*(WheelSize_MM/1000)

            TrueSpeedFilter=LKF.LinearKalmanFilter(A=LKF.Matrix([[1]]), B=LKF.Matrix([[0.02]]), H=LKF.Matrix([[1/(WheelSize_MM/1000)]]), Q=LKF.Matrix([[0.05]]), R=LKF.Matrix([[0.25]]), x0=LKF.Matrix([[0]]), P0=LKF.Matrix([[9]]))
            TrueSpeedFilter.predict(LKF.Matrix([[Inertial.acceleration(XAXIS)*9.81]]))
            TrueSpeedFilter.update(LKF.Matrix([[XOdom.velocity(RPM)]]))

            TrueSpeed= TrueSpeedFilter.x.data[0][0]

            SlipRateRight=(TrueSpeed-RightWheelSpeed)/TrueSpeed*100
            SlipRateLeft=(TrueSpeed-LeftWheelSpeed)/TrueSpeed*100

            TargetRightRPM=RequestedRightRPM-SlipRateRight
            TargetLeftRPM=RequestedLeftRPM-SlipRateLeft
            AntiFightTargetRightRPM=[TargetRightRPM-((VelocityDiffrenceRight/2)*0.5), TargetRightRPM+((VelocityDiffrenceRight/2)*0.5)]
            AntiFightTargetLeftRPM=[TargetLeftRPM-((VelocityDiffrenceLeft/2)*0.5), TargetLeftRPM+((VelocityDiffrenceLeft/2)*0.5)]

            for i in range(len(LeftMotorList)):
                LeftMotorList[i].spin(FORWARD, PDC.PD("LeftSide", 0.5, 0.1).compute(AntiFightTargetLeftRPM[i], AcutalLeftRPM, 0.02, 12, -12), VOLT)

            for i in range(len(RightMotorList)):
                RightMotorList[i].spin(FORWARD, PDC.PD("RightSide", 0.5, 0.1).compute(AntiFightTargetRightRPM[i], AcutalRightRPM, 0.02, 12, -12), VOLT)

            await uasyncio.sleep_ms(20 - StartTime)

