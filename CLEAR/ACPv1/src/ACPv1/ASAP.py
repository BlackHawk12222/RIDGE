#Anti Slip Asyncronis Protocol
from vex import *

import uasyncio
from .LKF import LinearKalmanFilter, Matrix
from .PDC import PD, AutoTune

def Start(LeftMotorList: list[Motor], RightMotorList: list[Motor], GearRatio: float, WheelSize_MM: float, MotorRpmMax: int, Controller: Controller, XOdom: Rotation, OdomWheelSize_MM, StickType: str, Inertial: Inertial):
    RunLoop=uasyncio.create_task(_run(LeftMotorList, RightMotorList, GearRatio, WheelSize_MM, MotorRpmMax, Controller, XOdom, OdomWheelSize_MM, StickType, Inertial))
    return RunLoop

async def _run(LeftMotorList: list[Motor], RightMotorList: list[Motor], GearRatio: float, WheelSize_MM: float, MotorRpmMax: int, Controller: Controller, XOdom: Rotation, OdomWheelSize_MM, StickType: str, Inertial: Inertial):
    timer=Timer()
    KP=0.5
    KD=0.1
    print("PD config start")
    LeftPD=PD("LeftSide", KP, KD)
    RightPD=PD("RightSide", KP, KD)
    print("PD config done")
    print("AutoTune config start")
    LeftAutoTune=AutoTune(LeftPD, 0.7, 1.0, Matrix([[0], [0], [0], [0]]))
    RightAutoTune=AutoTune(RightPD, 0.7, 1.0, Matrix([[0], [0], [0], [0]]))
    Thread(LeftAutoTune.start_tuning, (0, 0, 0, 0, 0, 0))
    Thread(RightAutoTune.start_tuning, (0, 0, 0, 0, 0, 0))
    print("AutoTune config done")
    TrueSpeedFilter=LinearKalmanFilter(A=Matrix([[1]]), B=Matrix([[0.02]]), H=Matrix([[1/(WheelSize_MM/1000)]]), Q=Matrix([[0.05]]), R=Matrix([[0.25]]), x0=Matrix([[0]]), P0=Matrix([[9]]))

    AntiFightGain=300/MotorRpmMax
    Controllertolrance=5
    CheckedIfStraight=False
    heading=0
    headingTolrance=2
    HeadingCorrectionGain=5

    while True:
        StartTime=timer.time()

        if StickType == "Tank" or StickType == "tank":
            RightPos = Controller.axis2.position()
            LeftPos = Controller.axis3.position()

            StaySraight=bool(not RightPos >= LeftPos- Controllertolrance and RightPos <= LeftPos+ Controllertolrance)

            if StaySraight and not CheckedIfStraight:
                heading=Inertial.heading()
                CheckedIfStraight=True

            RequestedRightRPM= RightPos*(MotorRpmMax/100)
            RequestedLeftRPM= LeftPos*(MotorRpmMax/100)
            AcutalRightRPM= (RightMotorList[0].velocity(RPM) + RightMotorList[1].velocity(RPM))/2
            AcutalLeftRPM= (LeftMotorList[0].velocity(RPM) + LeftMotorList[1].velocity(RPM))/2

            VelocityDiffrenceRight=RightMotorList[0].velocity(RPM) - RightMotorList[1].velocity(RPM)
            VelocityDiffrenceLeft=LeftMotorList[0].velocity(RPM) - LeftMotorList[1].velocity(RPM)

            LeftWheelSpeed=((LeftMotorList[0].velocity(RPM)*((2*3.14159)/60))/GearRatio)*(WheelSize_MM/1000)
            RightWheelSpeed=((RightMotorList[0].velocity(RPM)*((2*3.14159)/60))/GearRatio)*(WheelSize_MM/1000)

            
            TrueSpeedFilter.predict(Matrix([[Inertial.acceleration(XAXIS)*9.81]]))
            TrueSpeedFilter.update(Matrix([[XOdom.velocity(RPM)]]))

            TrueSpeed= TrueSpeedFilter.x.data[0][0]

            SlipRateRight=(TrueSpeed-RightWheelSpeed)/TrueSpeed*100
            SlipRateLeft=(TrueSpeed-LeftWheelSpeed)/TrueSpeed*100
            if not StaySraight:
                TargetRightRPM=RequestedRightRPM
                TargetLeftRPM=RequestedLeftRPM
            else:
                if Inertial.heading() > heading + headingTolrance:
                    headingCorrection=(Inertial.heading() - heading) * HeadingCorrectionGain
                elif Inertial.heading() < heading-headingTolrance:
                    headingCorrection=(Inertial.heading() - heading) * HeadingCorrectionGain

                TargetRightRPM=max(min(RequestedRightRPM-SlipRateRight + headingCorrection, -MotorRpmMax), MotorRpmMax)
                TargetLeftRPM=max(min(RequestedLeftRPM-SlipRateLeft - headingCorrection, -MotorRpmMax), MotorRpmMax)

            LeftOutput=LeftPD.compute(TargetLeftRPM, AcutalLeftRPM, 0.02, 12, -12)
            RightOutput=RightPD.compute(TargetRightRPM, AcutalRightRPM, 0.02, 12, -12)

            AntiFightOutputRight=[RightOutput-((VelocityDiffrenceRight/2)*AntiFightGain), RightOutput+((VelocityDiffrenceRight/2)*AntiFightGain)]
            AntiFightOutputLeft=[LeftOutput-((VelocityDiffrenceLeft/2)*AntiFightGain), LeftOutput+((VelocityDiffrenceLeft/2)*AntiFightGain)]

            for i in range(len(LeftMotorList)):
                LeftMotorList[i].spin(FORWARD, AntiFightOutputLeft[i], VOLT)

            for i in range(len(RightMotorList)):
                RightMotorList[i].spin(FORWARD, AntiFightOutputRight[i], VOLT)

            await uasyncio.sleep_ms(20 - StartTime)
        elif StickType == "Arcade" or StickType == "arcade":
            RightPos = Controller.axis3.position() - Controller.axis4.position()
            LeftPos = Controller.axis3.position() + Controller.axis4.position()

            StaySraight=bool(not RightPos >= LeftPos- Controllertolrance and RightPos <= LeftPos+ Controllertolrance)

            if StaySraight and not CheckedIfStraight:
                heading=Inertial.heading()
                CheckedIfStraight=True

            RequestedRightRPM= RightPos*(MotorRpmMax/100)
            RequestedLeftRPM= LeftPos*(MotorRpmMax/100)
            AcutalRightRPM= (RightMotorList[0].velocity(RPM) + RightMotorList[1].velocity(RPM))/2
            AcutalLeftRPM= (LeftMotorList[0].velocity(RPM) + LeftMotorList[1].velocity(RPM))/2

            VelocityDiffrenceRight=RightMotorList[0].velocity(RPM) - RightMotorList[1].velocity(RPM)
            VelocityDiffrenceLeft=LeftMotorList[0].velocity(RPM) - LeftMotorList[1].velocity(RPM)

            LeftWheelSpeed=((LeftMotorList[0].velocity(RPM)*((2*3.14159)/60))/GearRatio)*(WheelSize_MM/1000)
            RightWheelSpeed=((RightMotorList[0].velocity(RPM)*((2*3.14159)/60))/GearRatio)*(WheelSize_MM/1000)

            
            TrueSpeedFilter.predict(Matrix([[Inertial.acceleration(XAXIS)*9.81]]))
            TrueSpeedFilter.update(Matrix([[XOdom.velocity(RPM)]]))

            TrueSpeed= TrueSpeedFilter.x.data[0][0]

            SlipRateRight=(TrueSpeed-RightWheelSpeed)/TrueSpeed*100
            SlipRateLeft=(TrueSpeed-LeftWheelSpeed)/TrueSpeed*100
            if not StaySraight:
                TargetRightRPM=RequestedRightRPM
                TargetLeftRPM=RequestedLeftRPM
            else:
                if Inertial.heading() > heading + headingTolrance:
                    headingCorrection=(Inertial.heading() - heading) * HeadingCorrectionGain
                elif Inertial.heading() < heading-headingTolrance:
                    headingCorrection=(Inertial.heading() - heading) * HeadingCorrectionGain

                TargetRightRPM=max(min(RequestedRightRPM-SlipRateRight + headingCorrection, -MotorRpmMax), MotorRpmMax)
                TargetLeftRPM=max(min(RequestedLeftRPM-SlipRateLeft - headingCorrection, -MotorRpmMax), MotorRpmMax)

            LeftOutput=LeftPD.compute(TargetLeftRPM, AcutalLeftRPM, 0.02, 12, -12)
            RightOutput=RightPD.compute(TargetRightRPM, AcutalRightRPM, 0.02, 12, -12)

            AntiFightOutputRight=[RightOutput-((VelocityDiffrenceRight/2)*AntiFightGain), RightOutput+((VelocityDiffrenceRight/2)*AntiFightGain)]
            AntiFightOutputLeft=[LeftOutput-((VelocityDiffrenceLeft/2)*AntiFightGain), LeftOutput+((VelocityDiffrenceLeft/2)*AntiFightGain)]

            for i in range(len(LeftMotorList)):
                LeftMotorList[i].spin(FORWARD, AntiFightOutputLeft[i], VOLT)

            for i in range(len(RightMotorList)):
                RightMotorList[i].spin(FORWARD, AntiFightOutputRight[i], VOLT)

            await uasyncio.sleep_ms(20 - StartTime)


