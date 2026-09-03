#Anti Slip Asyncronis Protocol
from vex import *

from .LKF import LinearKalmanFilter, Matrix
from .PIDC import PID, AutoTune

def Start(LeftMotorList: list[Motor], RightMotorList: list[Motor], GearRatio: float, WheelSize_MM: float, MotorRpmMax: int, Controller: Controller, XOdom: Rotation, OdomWheelSize_MM, StickType: str, Inertial: Inertial):
    RunLoop=Thread(_run, (LeftMotorList, RightMotorList, GearRatio, WheelSize_MM, MotorRpmMax, Controller, XOdom, OdomWheelSize_MM, StickType, Inertial))
    print("ASAP started with %s and %s"%(LeftMotorList, RightMotorList))
    return RunLoop

def _run(LeftMotorList: list[Motor], RightMotorList: list[Motor], GearRatio: float, WheelSize_MM: float, MotorRpmMax: int, Controller: Controller, XOdom: Rotation, OdomWheelSize_MM, StickType: str, Inertial: Inertial):
    timer=Timer()
    for motor in LeftMotorList:
        motor.set_stopping(COAST)

    for motor in RightMotorList:
        motor.set_stopping(COAST)

    KP=0.002
    KI=0.0
    KD=0.00001
    print("PID config start")
    LeftPID=PID("LeftSide", KP, KI, KD)
    RightPID=PID("RightSide", KP, KI, KD)
    print("PID config done")
    print("AutoTune config start")
    #LeftAutoTune=AutoTune(LeftPID, 0.7, 1.0, Matrix([[0.0], [0.0], [0.0], [0.0]]))
    #RightAutoTune=AutoTune(RightPID, 0.7, 1.0, Matrix([[0.0], [0.0], [0.0], [0.0]]))
    #Thread(LeftAutoTune.start_tuning, (0, 0, 3, 3, 2, 2))
    #Thread(RightAutoTune.start_tuning, (0, 0, 3, 3, 2, 2))
    print("AutoTune config done")
    TrueSpeedFilter=LinearKalmanFilter(A=Matrix([[1.0]]), B=Matrix([[0.02]]), H=Matrix([[1/(WheelSize_MM/1000)]]), Q=Matrix([[0.1]]), R=Matrix([[0.8]]), x0=Matrix([[0.0]]), P0=Matrix([[9.0]]))

    AntiFightGain=(1200/MotorRpmMax) * 0.02
    Controllertolrance=5
    CheckedIfStraight=False
    heading=0
    headingTolrance=2
    HeadingCorrectionGain=0
    headingCorrection=0

    if "Tank" in StickType or "tank" in StickType:

        while True:

            StartTime=timer.time()

            RightPos = Controller.axis2.position()
            LeftPos = Controller.axis3.position()

            StaySraight=bool(not RightPos >= LeftPos - Controllertolrance and not RightPos <= LeftPos+ Controllertolrance)

            if StaySraight and not CheckedIfStraight:
                heading=Inertial.heading()
                CheckedIfStraight=True

            RequestedRightRPM= RightPos*(MotorRpmMax/100)
            RequestedLeftRPM= LeftPos*(MotorRpmMax/100)
            AcutalRightRPM= (RightMotorList[0].velocity(RPM) + RightMotorList[1].velocity(RPM))/2
            AcutalLeftRPM= (LeftMotorList[0].velocity(RPM) + LeftMotorList[1].velocity(RPM))/2

            VelocityDiffrenceRight=RightMotorList[0].velocity(RPM) - RightMotorList[1].velocity(RPM)
            VelocityDiffrenceLeft=LeftMotorList[0].velocity(RPM) - LeftMotorList[1].velocity(RPM)

            if LeftMotorList[0].velocity(RPM) != 0 and LeftMotorList[1].velocity(RPM) != 0:
                LeftWheelSpeed=((LeftMotorList[0].velocity(RPM)*((2*3.14159)/60))/GearRatio)*(WheelSize_MM/1000)
            else:
                LeftWheelSpeed=0

            if RightMotorList[0].velocity(RPM) != 0 and RightMotorList[1].velocity(RPM) != 0:
                RightWheelSpeed=((RightMotorList[0].velocity(RPM)*((2*3.14159)/60))/GearRatio)*(WheelSize_MM/1000)
            else:
                RightWheelSpeed=0

            
            TrueSpeedFilter.predict(Matrix([[Inertial.acceleration(XAXIS)*9.81]]))
            TrueSpeedFilter.update(Matrix([[XOdom.velocity(RPM)]]))

            TrueSpeed= TrueSpeedFilter.x.data[0][0]

            if (TrueSpeed > 0.2 or TrueSpeed < -0.2) and RequestedRightRPM != 0 and RequestedLeftRPM != 0:
                if RightWheelSpeed > 1 or RightWheelSpeed < -1:
                    SlipRateRight=(TrueSpeed-RightWheelSpeed)/TrueSpeed*100
                else:
                    SlipRateRight=0

                if LeftWheelSpeed > 1 or LeftWheelSpeed < -1:
                    SlipRateLeft=(TrueSpeed-LeftWheelSpeed)/TrueSpeed*100
                else:
                    SlipRateLeft=0
            else:
                SlipRateRight=0
                SlipRateLeft=0

            if not StaySraight:
                TargetRightRPM=RequestedRightRPM
                TargetLeftRPM=RequestedLeftRPM
            else:
                SlipOffsetRight=SlipRateRight * (MotorRpmMax/100)
                SlipOffsetLeft=SlipRateLeft * (MotorRpmMax/100)

                if Inertial.heading() > heading + headingTolrance:
                    headingCorrection=(Inertial.heading() - heading) * HeadingCorrectionGain
                elif Inertial.heading() < heading - headingTolrance:
                    headingCorrection=(Inertial.heading() + heading) * HeadingCorrectionGain

                TargetRightRPM=max(min(RequestedRightRPM-SlipOffsetRight + headingCorrection, -MotorRpmMax), MotorRpmMax)
                TargetLeftRPM=max(min(RequestedLeftRPM-SlipOffsetLeft - headingCorrection, -MotorRpmMax), MotorRpmMax)

            LeftOutput=LeftPID.compute(TargetLeftRPM, AcutalLeftRPM, 0.02, 12, -12)
            RightOutput=RightPID.compute(TargetRightRPM, AcutalRightRPM, 0.02, 12, -12)

            AntiFightOutputRight=[RightOutput, RightOutput+((VelocityDiffrenceRight/2)*AntiFightGain)]
            AntiFightOutputLeft=[LeftOutput, LeftOutput+((VelocityDiffrenceLeft/2)*AntiFightGain)]

            #print(AntiFightOutputRight, AntiFightOutputLeft)
            
            for i in range(len(LeftMotorList)):
                LeftMotorList[i].spin(FORWARD, AntiFightOutputLeft[i], VOLT)

            for i in range(len(RightMotorList)):
                RightMotorList[i].spin(FORWARD, AntiFightOutputRight[i], VOLT)

            #print(timer.time() -StartTime)
            print("output: %s, %s TrueSpeed: %s, targets: %s, %s RightRPM: %s LeftRPM: %s"%(AntiFightOutputLeft, AntiFightOutputRight, TrueSpeed, TargetLeftRPM, TargetRightRPM, AcutalRightRPM, AcutalLeftRPM))

            wait(20 - (timer.time() - StartTime), MSEC)
    elif "Arcade" in StickType or "arcade" in StickType:
        while True:

            StartTime=timer.time()

            RightPos = Controller.axis3.position() - Controller.axis4.position()
            LeftPos = Controller.axis3.position() + Controller.axis4.position()

            StaySraight=bool(not RightPos >= LeftPos- Controllertolrance and not RightPos <= LeftPos+ Controllertolrance)

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
                elif Inertial.heading() < heading - headingTolrance:
                    headingCorrection=(Inertial.heading() + heading) * HeadingCorrectionGain

                TargetRightRPM=max(min(RequestedRightRPM-SlipRateRight + headingCorrection, -MotorRpmMax), MotorRpmMax)
                TargetLeftRPM=max(min(RequestedLeftRPM-SlipRateLeft - headingCorrection, -MotorRpmMax), MotorRpmMax)

            LeftOutput=LeftPID.compute(TargetLeftRPM, AcutalLeftRPM, 0.02, 12, -12)
            RightOutput=RightPID.compute(TargetRightRPM, AcutalRightRPM, 0.02, 12, -12)

            AntiFightOutputRight=[RightOutput-((VelocityDiffrenceRight/2)*AntiFightGain), RightOutput+((VelocityDiffrenceRight/2)*AntiFightGain)]
            AntiFightOutputLeft=[LeftOutput-((VelocityDiffrenceLeft/2)*AntiFightGain), LeftOutput+((VelocityDiffrenceLeft/2)*AntiFightGain)]

            for i in range(len(LeftMotorList)):
                LeftMotorList[i].spin(FORWARD, AntiFightOutputLeft[i], VOLT)

            for i in range(len(RightMotorList)):
                RightMotorList[i].spin(FORWARD, AntiFightOutputRight[i], VOLT)

            print(timer.time() -StartTime)
            print("output: %s, %s TrueSpeed: %s, targets: %s, %s"%(AntiFightOutputLeft, AntiFightOutputRight, TrueSpeed, TargetLeftRPM, TargetRightRPM))

            wait(20 - StartTime, MSEC)
    else:
        raise ValueError("Invalid StickType. Must be 'Tank' or 'Arcade'.")


