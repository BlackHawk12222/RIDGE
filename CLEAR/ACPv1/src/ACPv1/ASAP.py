#Anti Slip Asyncronis Protocol
import math

from vex import *

from .General.LKF import LinearKalmanFilter, Matrix
from .NLADRCC import NLADRC

def _rpm_to_wheel_speed_mps(rpm: float, wheel_diameter_mm: float, gear_ratio: float = 1.0) -> float:
    return rpm * math.pi * (wheel_diameter_mm / 1000) / (60 * gear_ratio)


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

    KP = 5     # Modest tracking aggressiveness to start
    KPScale = 0.0     # Turn off error-scaling initially to establish a baseline
    B0 = 1     # Neutral input-to-output gain scaling
    Omega_o = 8    # Safe observer bandwidth for a 100Hz (dt=0.01) loop
    Alpha = 1.0       # IMPORTANT: Sets _fal() to linear mode for safe initial tuning
    MaxAccel = 5.0    # Gentler target ramping to protect your hardware initially
    Delta = 0.5
    Dt = 0.01

    # Motor controller object creation.
    LeftController=NLADRC(KP, KPScale, B0, Omega_o, Alpha, Delta, MaxAccel)
    RightController=NLADRC(KP, KPScale, B0, Omega_o, Alpha, Delta, MaxAccel)

    # Reset for clean start.
    RightController.reset(0)
    LeftController.reset(0)

    # Kalman Filter for True Speed Estimation
    TrueSpeedObservationScale = 1.0 if "Tank" in StickType or "tank" in StickType else 1 / (WheelSize_MM / 1000)
    TrueSpeedFilter=LinearKalmanFilter(A=Matrix([[1.0]]), B=Matrix([[Dt]]), H=Matrix([[TrueSpeedObservationScale]]), Q=Matrix([[0.1]]), R=Matrix([[0.8]]), x0=Matrix([[0.0]]), P0=Matrix([[9.0]]))
    LeftEncoderSpeed = LinearKalmanFilter(A=Matrix([[1.0]]), B=Matrix([[0]]), H=Matrix([[1]]), Q=Matrix([[0.5]]), R=Matrix([[0.05]]), x0=Matrix([[0.0]]), P0=Matrix([[9.0]]))
    RightEncoderSpeed = LinearKalmanFilter(A=Matrix([[1.0]]), B=Matrix([[0]]), H=Matrix([[1]]), Q=Matrix([[0.5]]), R=Matrix([[0.05]]), x0=Matrix([[0.0]]), P0=Matrix([[9.0]]))

    # Constansts and Delerations for ASAP.
    AntiFightGain=(1200/MotorRpmMax) * Dt
    Controllertolrance: int=5
    CheckedIfStraight: bool=False
    heading: float=0.0
    headingTolrance: int=2
    HeadingCorrectionGain: float=0.0
    headingCorrection: float=0.0
    # LeftMotorPower=0
    # LeftMotorCurrent=0
    # LeftMotorVoltage=0
    # RightMotorPower=0
    # RightMotorCurrent=0
    # RightMotorVoltage=0
    Stopped: bool=False
    TempHigh: bool=False
    TempVeryHigh: bool=False
    UDummy=Matrix([[0.0]])
    StickUsed=0

    # Convertis string input to int for faster processing in the loop.
    if "Tank" in StickType or "tank" in StickType:
        StickUsed=1
    elif "Arcade" in StickType or "arcade" in StickType:
        StickUsed=2
    elif "SplitArcade" in StickType or "split_arcade" in StickType:
        StickUsed=3
    else:
        raise ValueError("Invalid StickType. Must be 'Tank', 'Arcade', or 'SplitArcade'.")

    while True:

        # Uses of time to get more accurate loops.
        StartTime=timer.time()

        # Updateing all the values from the controller and motors.
        if StickUsed==1:  # Tank Control
            RightPos = Controller.axis2.position()
            LeftPos = Controller.axis3.position()

        elif StickUsed==2:  # Arcade Control
            RightPos = Controller.axis3.position() - Controller.axis4.position()
            LeftPos = Controller.axis3.position() + Controller.axis4.position()
        
        elif StickUsed==3:  # Split Arcade Control
            RightPos = Controller.axis3.position() - Controller.axis4.position()
            LeftPos = Controller.axis3.position() + Controller.axis4.position()

        else:
            raise ValueError("Invalid StickType. Must be 'Tank', 'Arcade', or 'SplitArcade'.")

        RightMotor0Velocity_RPM=RightMotorList[0].velocity(RPM)
        RightMotor1Velocity_RPM=RightMotorList[1].velocity(RPM)
        
        LeftMotor0Velocity_RPM=LeftMotorList[0].velocity(RPM)
        LeftMotor1Velocity_RPM=LeftMotorList[1].velocity(RPM)

        # LeftMotorPower=LeftMotorList[0].power(PowerUnits.WATT)
        # LeftMotorCurrent=LeftMotorList[0].current(CurrentUnits.AMP)

        # RightMotorPower=RightMotorList[0].power(PowerUnits.WATT)
        # RightMotorCurrent=RightMotorList[0].current(CurrentUnits.AMP)

        RightMotor0Temperature_PERCENT=RightMotorList[0].temperature(PERCENT)
        RightMotor1Temperature_PERCENT=RightMotorList[1].temperature(PERCENT)

        LeftMotor0Temperature_PERCENT=LeftMotorList[0].temperature(PERCENT)
        LeftMotor1Temperature_PERCENT=LeftMotorList[1].temperature(PERCENT)

        InertialHeading=Inertial.heading()

        # Check if the robot Should be going straight.
        StaySraight=bool( abs(RightPos - LeftPos) <= Controllertolrance)

        # Toggle the heading if the robot is going straight and it has not been checked yet.
        if StaySraight and not CheckedIfStraight:
            heading=Inertial.heading()
            CheckedIfStraight=True

        # Update the TrueSpeedFilter with the current acceleration and velocity.
        TrueSpeedFilter.predict(Matrix([[Inertial.acceleration(XAXIS)*9.81]]))
        TrueSpeedFilter.update(Matrix([[_rpm_to_wheel_speed_mps(XOdom.velocity(RPM), OdomWheelSize_MM)]]))

        # if  not (LeftMotorPower == 0 or  LeftMotorCurrent == 0):
        #     LeftMotorVoltage =  LeftMotorPower / LeftMotorCurrent
        # else:
        #     LeftMotorVoltage = 0

        # if not (RightMotorPower ==0 or  RightMotorCurrent == 0):
        #     RightMotorVoltage =  RightMotorPower / RightMotorCurrent
        # else:
        #     RightMotorVoltage = 0

        # Update the Kalman Filters for the left and right encoder speeds.
        LeftEncoderSpeed.predict(UDummy)
        RightEncoderSpeed.predict(UDummy)

        LeftEncoderSpeed.update(Matrix([[LeftMotor0Velocity_RPM]]))
        RightEncoderSpeed.update(Matrix([[RightMotor0Velocity_RPM]]))

        # Calculate the requested and actual RPMs for both sides of the drivetrain.
        RequestedRightRPM= RightPos*(MotorRpmMax/100)
        RequestedLeftRPM= LeftPos*(MotorRpmMax/100)
        ActualRightRPM= RightEncoderSpeed.x.data[0][0]
        ActualLeftRPM= LeftEncoderSpeed.x.data[0][0]

        # Diffrences in the velocity to check if motors need to be synced.
        VelocityDiffrenceRight=RightMotor0Velocity_RPM - RightMotor1Velocity_RPM
        VelocityDiffrenceLeft=LeftMotor0Velocity_RPM - LeftMotor1Velocity_RPM

        # Calculate the wheel speeds based on the motor velocities, gear ratio, and wheel size.
        if LeftMotor0Velocity_RPM != 0 and LeftMotor1Velocity_RPM != 0:
            LeftWheelSpeed_MPS=_rpm_to_wheel_speed_mps(LeftMotor0Velocity_RPM, WheelSize_MM, GearRatio)
        else:
            LeftWheelSpeed_MPS=0

        if RightMotor0Velocity_RPM != 0 and RightMotor1Velocity_RPM != 0:
            RightWheelSpeed_MPS=_rpm_to_wheel_speed_mps(RightMotor0Velocity_RPM, WheelSize_MM, GearRatio)
        else:
            RightWheelSpeed_MPS=0


        # Get the true speed from the TrueSpeedFilter.
        TrueSpeed= TrueSpeedFilter.x.data[0][0]

        # Check if the robot is moving and calculate the slip rates for both sides of the drivetrain.
        if (TrueSpeed > 0.2 or TrueSpeed < -0.2) and RequestedRightRPM != 0 and RequestedLeftRPM != 0:
            if RightWheelSpeed_MPS > 1 or RightWheelSpeed_MPS < -1:
                SlipRateRight_PERCENT=(TrueSpeed-RightWheelSpeed_MPS)/TrueSpeed*100
            else:
                SlipRateRight_PERCENT=0

            if LeftWheelSpeed_MPS > 1 or LeftWheelSpeed_MPS < -1:
                SlipRateLeft_PERCENT=(TrueSpeed-LeftWheelSpeed_MPS)/TrueSpeed*100
            else:
                SlipRateLeft_PERCENT=0
        else:
            SlipRateRight_PERCENT=0
            SlipRateLeft_PERCENT=0

        # Calculate the target RPMs for both sides of the drivetrain based on the requested RPMs, slip rates, and heading correction.
        if not StaySraight:
            TargetRightRPM=RequestedRightRPM
            TargetLeftRPM=RequestedLeftRPM
        else:
            SlipOffsetRight=SlipRateRight_PERCENT * (MotorRpmMax/100)
            SlipOffsetLeft=SlipRateLeft_PERCENT * (MotorRpmMax/100)

            if InertialHeading > heading + headingTolrance:
                headingCorrection=(InertialHeading - heading) * HeadingCorrectionGain
            elif InertialHeading < heading - headingTolrance:
                headingCorrection=(InertialHeading - heading) * HeadingCorrectionGain

            TargetRightRPM=max(min(RequestedRightRPM-SlipOffsetRight + headingCorrection, MotorRpmMax), -MotorRpmMax)
            TargetLeftRPM=max(min(RequestedLeftRPM-SlipOffsetLeft - headingCorrection, MotorRpmMax), -MotorRpmMax)


        LeftOutput=LeftController.compute(TargetLeftRPM, ActualLeftRPM, Dt, 12, -12)
        RightOutput=RightController.compute(TargetRightRPM, ActualRightRPM, Dt, 12, -12)

        # Check if the motors are overheating and adjust the max torque accordingly.
        if not TempVeryHigh and (RightMotor1Temperature_PERCENT >= 70 or RightMotor0Temperature_PERCENT >= 70 or LeftMotor1Temperature_PERCENT >= 70 or LeftMotor0Temperature_PERCENT >= 70):
            for motor in LeftMotorList:
                motor.set_max_torque(0.75, CurrentUnits.AMP)

            for motor in RightMotorList:
                motor.set_max_torque(0.75, CurrentUnits.AMP)

            TempVeryHigh=True
            TempHigh=False
        elif not TempHigh and (RightMotor1Temperature_PERCENT >= 50 or RightMotor0Temperature_PERCENT >= 50 or LeftMotor1Temperature_PERCENT >= 50 or LeftMotor0Temperature_PERCENT >= 50):
            for motor in LeftMotorList:
                motor.set_max_torque(1.25, CurrentUnits.AMP)

            for motor in RightMotorList:
                motor.set_max_torque(1.25, CurrentUnits.AMP)

            TempHigh=True
            TempVeryHigh=False
        else:
            TempHigh=False
            TempVeryHigh=False

        # Anti fighting code to prevent the motors from fighting each other when they are not in sync.
        if  RightMotor1Velocity_RPM >= MotorRpmMax:
            PowerDeff=RightMotor1Velocity_RPM - MotorRpmMax
            AntiFightOutputRight=[RightOutput - (PowerDeff*AntiFightGain), RightOutput]
        else:
            AntiFightOutputRight=[RightOutput, max(min(RightOutput+((VelocityDiffrenceRight/2)*AntiFightGain), 12), -12)]

        if LeftMotor1Velocity_RPM >= MotorRpmMax:
            PowerDeff=LeftMotor1Velocity_RPM - MotorRpmMax
            AntiFightOutputLeft=[LeftOutput - (PowerDeff*AntiFightGain), LeftOutput]
        else:
            AntiFightOutputLeft=[LeftOutput, max(min(LeftOutput+((VelocityDiffrenceLeft/2)*AntiFightGain), 12), -12)]

        #print(AntiFightOutputRight, AntiFightOutputLeft)

        # If the controller is not being used and the motors are not moving, stop the motors and reset the controllers.
        if RightPos == 0 and LeftPos == 0 and (ActualRightRPM < (MotorRpmMax/30) and ActualRightRPM > -(MotorRpmMax/30)) and (ActualLeftRPM < (MotorRpmMax/30) and ActualLeftRPM > -(MotorRpmMax/30)):
            for i in range(len(LeftMotorList)):
                LeftMotorList[i].stop(HOLD)
            for i in range(len(RightMotorList)):
                RightMotorList[i].stop(HOLD)
            Stopped=True
            LeftController.reset(0)
            RightController.reset(0)
        else:
            if Stopped:
                for i in range(len(LeftMotorList)):
                    LeftMotorList[i].stop(COAST)
                for i in range(len(RightMotorList)):
                    RightMotorList[i].stop(COAST)
                Stopped=False

            # Apply the outputs to the motors.
            for i in range(len(LeftMotorList)):
                LeftMotorList[i].spin(FORWARD, AntiFightOutputLeft[i], VOLT)

            for i in range(len(RightMotorList)):
                RightMotorList[i].spin(FORWARD, AntiFightOutputRight[i], VOLT)

        #print(timer.time() -StartTime)
        #print("output: %s, %s TrueSpeed: %s, targets: %s, %s RightRPM: %s LeftRPM: %s"%(AntiFightOutputLeft, AntiFightOutputRight, TrueSpeed, TargetLeftRPM, TargetRightRPM, ActualRightRPM, ActualLeftRPM))

        # Takes the time of calculation and waits the rest of the time to make sure the loop runs at 100Hz.
        wait((Dt * 1000) - (timer.time() - StartTime), MSEC)