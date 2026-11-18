#Anti Slip Asyncronis Protocol
import math

from vex import *

from .Filters.CompFilter import CompFilter
from .NLADRCC import NLADRC

def _rpm_to_wheel_speed_mps(rpm: float, wheel_diameter_mm: float, gear_ratio: float = 1.0) -> float:
    return rpm * gear_ratio * math.pi * (wheel_diameter_mm / 1000) / 60


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

    KP = 15    # Controller bandwidth (rad/s): ~67 ms response, 3x open-loop speed, under sensor-delay limit
    KPSCALE = 0.0     # Turn off error-scaling initially to establish a baseline

    B0 = 239    # Plant gain (RPM/s per volt) for ~6.8 kg robot, 2x 600rpm motors/side, G=0.75, 69.85mm wheels. ~1624/mass_kg
    OMEGA_O = 45    # Observer bandwidth = 3x KP; Omega_o*dt = 0.45 (stable for Euler at 100Hz)

    ALPHA = 1.0       # IMPORTANT: Sets _fal() to linear mode for safe initial tuning

    MAXACCEL = 28    # RPM per 10ms tick: traction limit (mu 0.8 -> 2861 RPM/s), 0->600 in ~0.21 s

    DELTA = 5.0

    DT = 0.01

    # Motor controller object creation.
    LeftController = NLADRC(KP, KPSCALE, B0, OMEGA_O, ALPHA, DELTA, MAXACCEL, DT)
    RightController = NLADRC(KP, KPSCALE, B0, OMEGA_O, ALPHA, DELTA, MAXACCEL, DT)

    # Reset for clean start.
    RightController.reset(0)
    LeftController.reset(0)

    TrueSpeedFilter = CompFilter(Alpha = 0.98, dt_SECONDS = DT)

    # Constansts and Delerations for ASAP.
    CheckedIfStraight: bool = False

    heading: float = 0.0
    headingTolrance: int = 2
    HeadingCorrectionGain: float = 0.0
    headingCorrection: float = 0.0

    TempHigh: bool = False
    TempVeryHigh: bool = False

    StickUsed = 0

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
        if StickUsed == 1:  # Tank Control
            RightPos = Controller.axis2.position()
            LeftPos = Controller.axis3.position()

        elif StickUsed == 2:  # Arcade Control
            RightPos = Controller.axis3.position() - Controller.axis4.position()
            LeftPos = Controller.axis3.position() + Controller.axis4.position()
        
        elif StickUsed == 3:  # Split Arcade Control
            RightPos = Controller.axis3.position() - Controller.axis4.position()
            LeftPos = Controller.axis3.position() + Controller.axis4.position()

        else:
            raise ValueError("Invalid StickType. Must be 'Tank', 'Arcade', or 'SplitArcade'.")

        RightMotor0Velocity_RPM = RightMotorList[0].velocity(RPM)
        RightMotor1Velocity_RPM = RightMotorList[1].velocity(RPM)
        
        LeftMotor0Velocity_RPM = LeftMotorList[0].velocity(RPM)
        LeftMotor1Velocity_RPM = LeftMotorList[1].velocity(RPM)

        RightMotor0Temperature_PERCENT = RightMotorList[0].temperature(PERCENT)
        RightMotor1Temperature_PERCENT = RightMotorList[1].temperature(PERCENT)

        LeftMotor0Temperature_PERCENT = LeftMotorList[0].temperature(PERCENT)
        LeftMotor1Temperature_PERCENT = LeftMotorList[1].temperature(PERCENT)

        InertialHeading = Inertial.heading()

        # Update the TrueSpeedFilter with the current acceleration and velocity.
        a_fwd = Inertial.acceleration(XAXIS) * 9.81 - 9.81 * math.sin(math.radians(Inertial.orientation(PITCH, DEGREES)))
        TrueSpeedFilter.update(a_fwd, _rpm_to_wheel_speed_mps(XOdom.velocity(RPM), OdomWheelSize_MM))

        # Calculate the requested and actual RPMs for both sides of the drivetrain.
        RequestedRightRPM = RightPos * (MotorRpmMax / 100)
        RequestedLeftRPM = LeftPos * (MotorRpmMax / 100)

        ActualRightRPM = (RightMotor0Velocity_RPM + RightMotor1Velocity_RPM) / 2
        ActualLeftRPM = (LeftMotor0Velocity_RPM + LeftMotor1Velocity_RPM) / 2

        # Calculate the wheel speeds based on the motor velocities, gear ratio, and wheel size.
        if LeftMotor0Velocity_RPM != 0 and LeftMotor1Velocity_RPM != 0:
            LeftWheelSpeed_MPS = _rpm_to_wheel_speed_mps(LeftMotor0Velocity_RPM, WheelSize_MM, GearRatio)
        else:
            LeftWheelSpeed_MPS = 0

        if RightMotor0Velocity_RPM != 0 and RightMotor1Velocity_RPM != 0:
            RightWheelSpeed_MPS = _rpm_to_wheel_speed_mps(RightMotor0Velocity_RPM, WheelSize_MM, GearRatio)
        else:
            RightWheelSpeed_MPS = 0


        # Get the true speed from the TrueSpeedFilter.
        TrueSpeed = TrueSpeedFilter

        # Check if the robot is moving and calculate the slip rates for both sides of the drivetrain.
        if RightWheelSpeed_MPS > 1 or RightWheelSpeed_MPS < -1:
            SlipRateRight_PERCENT=(TrueSpeed-RightWheelSpeed_MPS)/TrueSpeed*100
        else:
            SlipRateRight_PERCENT=0

        if LeftWheelSpeed_MPS > 1 or LeftWheelSpeed_MPS < -1:
            SlipRateLeft_PERCENT=(TrueSpeed-LeftWheelSpeed_MPS)/TrueSpeed*100
        else:
            SlipRateLeft_PERCENT=0

        SlipOffsetRight=SlipRateRight_PERCENT * (MotorRpmMax/100)
        SlipOffsetLeft=SlipRateLeft_PERCENT * (MotorRpmMax/100)

        if InertialHeading > heading + headingTolrance:
            headingCorrection=(InertialHeading - heading) * HeadingCorrectionGain
        elif InertialHeading < heading - headingTolrance:
            headingCorrection=(InertialHeading - heading) * HeadingCorrectionGain

        TargetRightRPM=max(min(RequestedRightRPM + SlipOffsetRight + headingCorrection, MotorRpmMax), -MotorRpmMax)
        TargetLeftRPM=max(min(RequestedLeftRPM + SlipOffsetLeft - headingCorrection, MotorRpmMax), -MotorRpmMax)

        LeftOutput=LeftController.compute(TargetLeftRPM, ActualLeftRPM, DT, 12, -12)
        RightOutput=RightController.compute(TargetRightRPM, ActualRightRPM, DT, 12, -12)

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

        for i in range(len(LeftMotorList)):
            LeftMotorList[i].spin(FORWARD, LeftOutput, VOLT)

        for i in range(len(RightMotorList)):
            RightMotorList[i].spin(FORWARD, RightOutput, VOLT)

        # Takes the time of calculation and waits the rest of the time to make sure the loop runs at 100Hz.
        wait((DT * 1000) - (timer.time() - StartTime), MSEC)