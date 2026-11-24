#Anti Slip Asyncronis Protocol
import math

from vex import *

from .Filters.CompFilter import CompFilter
from .NLADRCC import NLADRC

def _rpm_to_wheel_speed_mps(rpm: float, wheel_diameter_mm: float, gear_ratio: float = 1.0) -> float:
    return rpm * gear_ratio * math.pi * (wheel_diameter_mm / 1000) / 60


# ===========================================================================
# Robot geometry. MEASURE THESE and set them.
# ===========================================================================
TRACK_MM = 305.0         # center of left wheels to center of right wheels  (placeholder, measure)
WHEELBASE_MM = 254.0     # front axle to back axle                          (placeholder, measure)
DROP_CENTER = True       # center wheels lowered: robot rocks on center + one end
CHI_MEASURED = 0.0       # b_eff / TRACK_MM from a spin test; 0 = use the geometry estimate

XODOM_OFFSET_MM = 0.0    # forward tracking wheel: distance RIGHT of robot center (+ right, - left)
YODOM_OFFSET_MM = 0.0    # horizontal tracking wheel: distance AHEAD of robot center (+ ahead)
YODOM_SIGN = 1.0         # set -1 if the horizontal wheel reads + when the robot moves LEFT

GYRO_SIGN = 1.0          # set -1 if gyro_rate(ZAXIS) reads - when turning clockwise (right)
YACCEL_SIGN = 1.0        # set -1 if acceleration(YAXIS) reads - when sliding right

# ===========================================================================
# Anti-slip / turn tuning
# ===========================================================================
STICK_DEADBAND = 2           # percent; sticks inside this read as 0
K_YAW = 0.3                  # turn-rate feedback 0..1 (raise toward 0.6 until it twitches)
SLIP_MAX = 0.05              # wheels may outrun the ground by 15% ...
SLIP_MARGIN_MPS = 0.2        # ... plus this much. Sets launch accel: ~margin * KP = 6 m/s^2.
                             # Lower = gentler launches, less wheelspin; higher = more punch.
LAT_ACCEL_MAX = 0.7 * 9.81   # most sideways grip (m/s^2) before a fast arc slides out
LAT_SLIP_MPS = 0.15          # sideways speed that counts as sliding

# Live values for logging (CLEAR) or printing. Updated every loop.
# Turn rates rad/s (+ = clockwise), speeds m/s, slips m/s (wheel minus expected ground).
Telemetry = {"w_des": 0.0, "w_meas": 0.0, "v_true": 0.0, "v_lat": 0.0,
             "exp_left": 0.0, "exp_right": 0.0, "slip_left": 0.0, "slip_right": 0.0,
             "chi": 1.0}


def expected_chi(track_mm, wheelbase_mm, drop_center=True):
    """Skid-steer turn factor: how many times more the sides must differ than
    plain geometry says, because the end wheels scrub sideways in a turn.
    Even grip on every wheel gives chi = 1 + (L/b)^2. With a drop center the
    robot only touches across about half the wheelbase."""
    L = wheelbase_mm / 2.0 if drop_center else wheelbase_mm
    return 1.0 + (L / track_mm) ** 2


def _clamp(x, lo, hi):
    return lo if x < lo else hi if x > hi else x


def _traction_limit(target_rpm, true_rpm, mps_per_rpm):
    """Wheels may not outrun the ground by more than SLIP_MAX + margin in the
    direction they are driving. Braking and holding against a push are untouched."""
    lim = (SLIP_MAX * abs(true_rpm) * mps_per_rpm + SLIP_MARGIN_MPS) / mps_per_rpm
    hi = (true_rpm if true_rpm > 0 else 0.0) + lim
    lo = (true_rpm if true_rpm < 0 else 0.0) - lim
    return _clamp(target_rpm, lo, hi)


def _targets(req_left, req_right, v_true, w_meas, v_lat, rpm_max, mps_per_rpm, track_m, chi):
    """Stick request (motor RPM) -> target motor RPM per side.

    1. Split the sticks into forward speed and turn rate. The turn rate the
       driver asked for uses the REAL track: w_des = (L - R) / track.
    2. Never ask for more turn than side grip allows at this speed.
    3. Feedforward: the sides must differ chi times more to get that rate.
       Feedback: the gyro trims whatever the model missed (holds straight when w_des = 0).
    4. If the robot is sliding sideways, back off the turn until it grips.
    5. Traction limit each side against the ground speed that side should have.
    Returns (target_left, target_right, w_des)."""
    fwd = (req_left + req_right) / 2.0
    half_diff = (req_left - req_right) / 2.0
    w_des = 2.0 * half_diff * mps_per_rpm / track_m

    v_abs = abs(v_true)
    if v_abs > 0.1:
        w_lim = LAT_ACCEL_MAX / v_abs
        w_des = _clamp(w_des, -w_lim, w_lim)

    w_cmd = w_des + K_YAW * (w_des - w_meas)

    lat_abs = abs(v_lat)
    if lat_abs > LAT_SLIP_MPS:
        w_cmd *= LAT_SLIP_MPS / lat_abs

    half_b_eff = track_m * chi / 2.0
    turn = _clamp(w_cmd * half_b_eff / mps_per_rpm, -rpm_max, rpm_max)
    room = rpm_max - abs(turn)
    fwd = _clamp(fwd, -room, room)              # keep the turn, give up forward speed first

    true_left = (v_true + w_meas * half_b_eff) / mps_per_rpm
    true_right = (v_true - w_meas * half_b_eff) / mps_per_rpm
    target_left = _traction_limit(fwd + turn, true_left, mps_per_rpm)
    target_right = _traction_limit(fwd - turn, true_right, mps_per_rpm)
    return target_left, target_right, w_des


def Start(LeftMotorList: list[Motor], RightMotorList: list[Motor], GearRatio: float, WheelSize_MM: float, MotorRpmMax: int, Controller: Controller, XOdom: Rotation, OdomWheelSize_MM, StickType: str, Inertial: Inertial, YOdom=None):
    """YOdom: optional horizontal (sideways) tracking wheel Rotation sensor, same wheel size as XOdom."""
    RunLoop=Thread(_run, (LeftMotorList, RightMotorList, GearRatio, WheelSize_MM, MotorRpmMax, Controller, XOdom, OdomWheelSize_MM, StickType, Inertial, YOdom))
    print("ASAP started with %s and %s"%(LeftMotorList, RightMotorList))
    return RunLoop

def _run(LeftMotorList: list[Motor], RightMotorList: list[Motor], GearRatio: float, WheelSize_MM: float, MotorRpmMax: int, Controller: Controller, XOdom: Rotation, OdomWheelSize_MM, StickType: str, Inertial: Inertial, YOdom=None):

    timer=Timer()

    for motor in LeftMotorList:
        motor.set_stopping(COAST)

    for motor in RightMotorList:
        motor.set_stopping(COAST)

    KP = 15    # Controller bandwidth (rad/s): ~67 ms response, 3x open-loop speed, under sensor-delay limit
    KPSCALE = 0.0     # Turn off error-scaling initially to establish a baseline

    B0 = 229    # Plant gain (RPM/s per volt) for ~6.8 kg robot, 2x 600rpm motors/side, G=0.75, 69.85mm wheels. ~1624/mass_kg
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
    LatSpeedFilter = CompFilter(Alpha = 0.98, dt_SECONDS = DT)   # sideways speed, m/s (+ = right)

    # Geometry, worked out once.
    Chi = CHI_MEASURED if CHI_MEASURED > 0 else expected_chi(TRACK_MM, WHEELBASE_MM, DROP_CENTER)
    Telemetry["chi"] = Chi
    MpsPerRpm = _rpm_to_wheel_speed_mps(1.0, WheelSize_MM, GearRatio)   # motor RPM -> tread m/s
    TrackM = TRACK_MM / 1000.0
    HalfBEff = TrackM * Chi / 2.0
    XOffM = XODOM_OFFSET_MM / 1000.0
    YOffM = YODOM_OFFSET_MM / 1000.0

    # Constansts and Delerations for ASAP.

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
            RightPos = -(Controller.axis2.position())
            LeftPos = -(Controller.axis3.position())

        elif StickUsed == 2:  # Arcade Control
            RightPos = -(Controller.axis3.position() - Controller.axis4.position())
            LeftPos = -(Controller.axis3.position() + Controller.axis4.position())
        
        elif StickUsed == 3:  # Split Arcade Control
            RightPos = -(Controller.axis3.position() - Controller.axis4.position())
            LeftPos = -(Controller.axis3.position() + Controller.axis4.position())

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

        # Stick deadband: small stick noise reads as 0.
        if -STICK_DEADBAND < RightPos < STICK_DEADBAND:
            RightPos = 0
        if -STICK_DEADBAND < LeftPos < STICK_DEADBAND:
            LeftPos = 0

        # Turn rate from the gyro, rad/s, + = clockwise.
        WMeas = math.radians(Inertial.gyro_rate(ZAXIS, VelocityUnits.DPS)) * GYRO_SIGN

        # Forward ground speed at robot center: IMU accel (fast) + tracking wheel (absolute).
        # The tracking wheel also sees rotation if it is off center, so take that out.
        a_fwd = Inertial.acceleration(XAXIS) * 9.81 - 9.81 * math.sin(math.radians(Inertial.orientation(PITCH, DEGREES)))
        TrueSpeedFilter.update(a_fwd, _rpm_to_wheel_speed_mps(XOdom.velocity(RPM), OdomWheelSize_MM) + WMeas * XOffM)
        TrueSpeed = float(TrueSpeedFilter)

        # Sideways speed at robot center. The accelerometer also feels the turn
        # (v * w), so take that out. With no horizontal wheel the filter leaks to 0.
        a_lat = Inertial.acceleration(YAXIS) * 9.81 * YACCEL_SIGN - TrueSpeed * WMeas
        if YOdom is not None:
            LatSpeedFilter.update(a_lat, _rpm_to_wheel_speed_mps(YOdom.velocity(RPM), OdomWheelSize_MM) * YODOM_SIGN - WMeas * YOffM)
        else:
            LatSpeedFilter.update(a_lat, 0.0)
        LatSpeed = float(LatSpeedFilter)

        # Calculate the requested and actual RPMs for both sides of the drivetrain.
        RequestedRightRPM = RightPos * (MotorRpmMax / 100)
        RequestedLeftRPM = LeftPos * (MotorRpmMax / 100)

        ActualRightRPM = (RightMotor0Velocity_RPM + RightMotor1Velocity_RPM) / 2
        ActualLeftRPM = (LeftMotor0Velocity_RPM + LeftMotor1Velocity_RPM) / 2

        if RightPos == 0 and LeftPos == 0:
            # Sticks centered: stop, no turn hold (keeps the motors quiet at rest).
            TargetLeftRPM = 0.0
            TargetRightRPM = 0.0
            WDes = 0.0
        else:
            TargetLeftRPM, TargetRightRPM, WDes = _targets(
                RequestedLeftRPM, RequestedRightRPM, TrueSpeed, WMeas, LatSpeed,
                MotorRpmMax, MpsPerRpm, TrackM, Chi)

        # Expected ground speed of each side and how far the wheels are off it (slip).
        ExpLeft = TrueSpeed + WMeas * HalfBEff
        ExpRight = TrueSpeed - WMeas * HalfBEff
        Telemetry["w_des"] = WDes
        Telemetry["w_meas"] = WMeas
        Telemetry["v_true"] = TrueSpeed
        Telemetry["v_lat"] = LatSpeed
        Telemetry["exp_left"] = ExpLeft
        Telemetry["exp_right"] = ExpRight
        Telemetry["slip_left"] = ActualLeftRPM * MpsPerRpm - ExpLeft
        Telemetry["slip_right"] = ActualRightRPM * MpsPerRpm - ExpRight

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