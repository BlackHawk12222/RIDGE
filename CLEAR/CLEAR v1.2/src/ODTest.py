import math, cmath, time, threading, random

class Motor:
    def __init__(self):
        self._position_deg: float = 0.0
        self._velocity_rpm: float = 0.0

    def position(self) -> int:
        return int(self._position_deg)
    
    def velocity(self, unit=None) -> float:
        return self._velocity_rpm

    def set_position(self, position: int):
        self._position_deg = float(position)

    def set_velocity(self, rpm: float):
        self._velocity_rpm = float(rpm)

    def set_velosity(self, rpm: float):
        self.set_velocity(rpm)

    def update(self, time_sec: float):
        self._position_deg += self._velocity_rpm * 360.0 / 60.0 * time_sec

FORWARD = 1
REVERSE = -1
VOLT = "VOLT"
RPM = "RPM"
YAXIS = 0
XAXIS = 1
ZAXIS = 2
MSEC = "MSEC"
SECONDS = "SECONDS"

def wait(Number: int, Unit: str):
    if Number < 0:
        Number = 0
    if Unit == MSEC:
        time.sleep(Number / 1000)
    elif Unit == SECONDS:
        time.sleep(Number)

class AxisType:
    XAXIS = 0
    YAXIS = 1
    ZAXIS = 2

class Inertial:
    def __init__(self):
        self._accel_g = [0.0, 0.0, 1.0]
        self._gyro_dps = [0.0, 0.0, 0.0]
        self._heading = 0.0
        self._last_forward_mps = 0.0
        self._heading_bias_deg = 0.0
        self._speed_mps = 0.0
        self._speed_from_accel_mps = 0.0
        self._calibrated = False

    def calibrate(self):
        self._calibrated = True
        self._heading = 0.0
        self._heading_bias_deg = 0.0
        self._last_forward_mps = 0.0
        self._speed_mps = 0.0
        self._speed_from_accel_mps = 0.0
        self._accel_g = [0.0, 0.0, 1.0]
        self._gyro_dps = [0.0, 0.0, 0.0]
    
    def acceleration(self, axis) -> float:
        return self._accel_g[axis]
    
    def gyro_rate(self, axis) -> float:
        return self._gyro_dps[axis]
    
    def heading(self) -> float:
        return self._heading

    def speed_mps(self) -> float:
        return self._speed_mps

    def speed_from_accel_mps(self) -> float:
        return self._speed_from_accel_mps

    def update(self, forward_mps: float, yaw_dps: float, time_sec: float):
        acceleration_mps2 = 0.0
        if time_sec > 0:
            acceleration_mps2 = (forward_mps - self._last_forward_mps) / time_sec

        self._accel_g[0] = acceleration_mps2 / 9.81
        self._accel_g[1] = 0.0
        self._accel_g[2] = 1.0
        self._gyro_dps[0] = 0.0
        self._gyro_dps[1] = 0.0
        self._gyro_dps[2] = yaw_dps
        self._heading = (self._heading + yaw_dps * time_sec) % 360.0
        self._speed_mps = forward_mps
        self._speed_from_accel_mps += acceleration_mps2 * time_sec
        self._last_forward_mps = forward_mps
        print(self._accel_g)

class Timer:
    
    def time(self):
        return int(time.time()*1000)

class Thread():
    def __init__(self, function, *args, **kwargs):
        self.function = function
        self.stop_event = threading.Event()
        self.thread = threading.Thread(target=self._run_with_stop, name="ODThread", args=args, kwargs=kwargs)

    def start(self):
        self.thread.start()

    def _run_with_stop(self, *args, **kwargs):
        self.function(*args, **kwargs)

    def stop(self):
        pass
        

class Rotation:
    def __init__(self):
        self.PositionOfRotation: float = 0.0
        self._velocity_rpm: float = 0.0

    def position(self) -> int:
        return int(self.PositionOfRotation)
    
    def velocity(self, unit=None) -> float:
        return self._velocity_rpm
    
    def set_position(self, position: int):
        self.PositionOfRotation = float(position)

    def update(self, rpm: float, time_sec: float):
        self._velocity_rpm = rpm
        self.PositionOfRotation += rpm * 360.0 / 60.0 * time_sec

    def linear_speed_mps(self, wheel_diameter_mm: float) -> float:
        wheel_radius_m = (wheel_diameter_mm / 1000.0) / 2.0
        wheel_circumference_m = 2 * math.pi * wheel_radius_m
        return (self._velocity_rpm * wheel_circumference_m) / 60.0

    def update_from_linear(self, forward_mps: float, wheel_diameter_mm: float, time_sec: float):
        if wheel_diameter_mm <= 0 or time_sec <= 0:
            return
        wheel_circumference_m = math.pi * (wheel_diameter_mm / 1000.0)
        rpm = (forward_mps * 60.0) / wheel_circumference_m
        self.update(rpm, time_sec)

class odometry:
    def __init__(self):
        self.leftEncodedMotor: Motor
        self.rightEncodedMotor: Motor
        self.XPosition: float = 0.0
        self.YPosition: float = 0.0
        self.Inertial: Inertial
        self.InertialSensorUse=False
        self.XSpeedLocalI_MPS: float = 0.0
        self.YSpeedLocalI_MPS: float = 0.0
        self.LDistanceM: float = 0.0
        self.RDistanceM: float = 0.0
        self.XDistanceI: float = 0.0
        self.YDistanceI: float = 0.0
        self.RobotWidthMM: float = 0.0
        self.WheelDiameterMM: float = 0.0
        self.XDistanceLocalM: float = 0.0
        self.Timer=Timer()
        self.ODT: Thread
        self.Xodom: Rotation
        self.Yodom: Rotation
        self.OdomWheelDiameter_MM: float = 0.0
        self.XOdomOffset: float = 0.0
        self.YOdomOffset: float = 0.0
        self.Weight: float = 0.98
        self.GearRatio: float = 0.0
        self.InertialXOffset: float = 0.0
        self.InertialYOffset: float = 0.0
        self.InertialZOffset: float = 0.0
        self.XgravityOffsetDegrees: float = 0.0
        self.YgravityOffsetDegrees: float = 0.0
        self.ZgravityOffsetDegrees: float = 0.0
        self.TimeStep_Sec: float = 0.01
        self.PureXaxis_G: float = 0.0
        self.PureYaxis_G: float = 0.0
        self.PureZaxis_G: float = 0.0
        self.FilteredRoll_Rad: float = 0.0
        self.FilteredPitch_Rad: float = 0.0
        self.FilteredYaw_Rad: float = 0.0
        self.Running=True

    @staticmethod
    def calaibrate_inertial():
        OD.Inertial.calibrate()
        RollA=math.degrees(math.atan(OD.Inertial.acceleration(AxisType.YAXIS)/OD.Inertial.acceleration(AxisType.ZAXIS)))
        OD.XgravityOffsetDegrees=RollA
        PitchA=math.degrees(math.asin(OD.Inertial.acceleration(AxisType.XAXIS)/9.81))
        OD.YgravityOffsetDegrees=PitchA

    @staticmethod
    def _Run() -> None:
        
        OD.XSpeedLocalI_MPS = 0.0
        OD.YSpeedLocalI_MPS = 0.0

        LeftDegOld=OD.leftEncodedMotor.position()
        RightDegOld=OD.rightEncodedMotor.position()
        OldHeading: float = OD.Inertial.heading()
        AxisWeight: float=0.9
        DistancePerDegree_Mpd: float=(math.pi*(OD.WheelDiameterMM/1000))/360
        OldXOdomVelocity_RPM: float = 0.0
        OldYOdomVelocity_RPM: float = 0.0
        TotalGravity=9.81
        OldXAxis: float = 0.0
        OldYAxis: float = 0.0
        OldZAxis: float = 0.0
        BaseI=1
        BaseM=1
        BaseO=1
        InertialWeight: float = 0.9

        while OD.Running:
            StartTime=OD.Timer.time()

            # Cleaining acceleration data.
            LeftSpeed_Mps=((OD.leftEncodedMotor.velocity(RPM)/OD.GearRatio) * DistancePerDegree_Mpd) / 60
            RightSpeed_Mps=((OD.rightEncodedMotor.velocity(RPM)/OD.GearRatio) * DistancePerDegree_Mpd) / 60
            HeadingRate_Mps=math.degrees(((RightSpeed_Mps-LeftSpeed_Mps))/(OD.RobotWidthMM/1000))
            
            ZAxis_G=OD.Inertial.acceleration(AxisType.ZAXIS)
            XAxis_G=OD.Inertial.acceleration(AxisType.XAXIS)
            YAxis_G=OD.Inertial.acceleration(AxisType.YAXIS)
            XAxisForGravity_G=OD.Inertial.acceleration(AxisType.XAXIS) - ((OD.Xodom.velocity(RPM) - OldXOdomVelocity_RPM) * OD.OdomWheelDiameter_MM * math.pi / 60)/OD.TimeStep_Sec
            YAxisForGravity_G=OD.Inertial.acceleration(AxisType.YAXIS) - ((OD.Yodom.velocity(RPM) - OldYOdomVelocity_RPM) * OD.OdomWheelDiameter_MM * math.pi / 60)/OD.TimeStep_Sec

            OldXOdomVelocity_RPM= OD.Xodom.velocity(RPM)
            OldYOdomVelocity_RPM= OD.Yodom.velocity(RPM)

            OD.FilteredRoll_Rad=AxisWeight*(OD.FilteredRoll_Rad + math.radians(OD.Inertial.gyro_rate(YAXIS) * OD.TimeStep_Sec)) + (1-AxisWeight)*((cmath.atan(YAxisForGravity_G/ZAxis_G).real)-OD.XgravityOffsetDegrees)
            OD.FilteredPitch_Rad=AxisWeight*(OD.FilteredPitch_Rad + math.radians(OD.Inertial.gyro_rate(XAXIS) * OD.TimeStep_Sec)) + (1-AxisWeight)*((cmath.asin(XAxisForGravity_G/TotalGravity).real)-OD.YgravityOffsetDegrees)
            OD.FilteredYaw_Rad=AxisWeight*(OD.FilteredYaw_Rad + math.radians(OD.Inertial.gyro_rate(ZAXIS) * OD.TimeStep_Sec)) + (1-AxisWeight)*(HeadingRate_Mps*OD.TimeStep_Sec)

            CentrificalAcceleration_G=(OD.Inertial.gyro_rate(ZAXIS)**2) * math.sqrt(OD.InertialXOffset**2 + OD.InertialYOffset**2)
            CentificalAngle_Rad=math.atan2(OD.InertialYOffset, OD.InertialXOffset)
            OD.PureXaxis_G=(((((XAxis_G * math.sin(OD.FilteredPitch_Rad)) + (ZAxis_G * math.cos(OD.FilteredPitch_Rad))) - (TotalGravity*math.sin(OD.FilteredPitch_Rad))) - (CentrificalAcceleration_G * math.cos(CentificalAngle_Rad))) + OldXAxis) / 2
            OD.PureYaxis_G=((((YAxis_G * math.sin(OD.FilteredRoll_Rad) + (ZAxis_G * math.cos(OD.FilteredRoll_Rad))) - (TotalGravity*math.sin(OD.FilteredRoll_Rad)*math.cos(OD.FilteredPitch_Rad))) - (CentrificalAcceleration_G * math.sin(CentificalAngle_Rad))) + OldYAxis) / 2
            OD.PureZaxis_G=((((ZAxis_G * math.tan(OD.FilteredPitch_Rad/OD.FilteredRoll_Rad)) - (TotalGravity*math.cos(OD.FilteredRoll_Rad)*math.cos(OD.FilteredPitch_Rad))) - (CentrificalAcceleration_G * math.tan(CentificalAngle_Rad))) + OldZAxis) / 2
            
            PureXaxis_MPS=OD.PureXaxis_G*TotalGravity
            PureYaxis_MPS=OD.PureYaxis_G*TotalGravity

            # Inertial calulations.
            OD.XDistanceI=OD.XSpeedLocalI_MPS*OD.TimeStep_Sec + 1/2*PureXaxis_MPS*(OD.TimeStep_Sec**2)
            OD.YDistanceI=OD.YSpeedLocalI_MPS*OD.TimeStep_Sec + 1/2*PureYaxis_MPS*(OD.TimeStep_Sec**2)
            OD.XSpeedLocalI_MPS+=(InertialWeight * PureXaxis_MPS + (1-InertialWeight) * ((OD.Xodom.velocity(RPM)* ((OD.OdomWheelDiameter_MM/1000)*math.pi))/60))*OD.TimeStep_Sec
            OD.YSpeedLocalI_MPS+=(InertialWeight * PureYaxis_MPS + (1-InertialWeight) * ((OD.Yodom.velocity(RPM)* ((OD.OdomWheelDiameter_MM/1000)*math.pi))/60))*OD.TimeStep_Sec

            # Motor calulations.
            OD.LDistanceM=((OD.leftEncodedMotor.position()-LeftDegOld) / 360) * DistancePerDegree_Mpd * OD.GearRatio
            OD.RDistanceM=((OD.rightEncodedMotor.position()-RightDegOld) / 360) * DistancePerDegree_Mpd * OD.GearRatio

            OD.XDistanceLocalM=(OD.LDistanceM+OD.RDistanceM)/2

            # Odometer calculations.
            XodomOffset=(OD.Inertial.heading() - OldHeading) * OD.XOdomOffset/OD.OdomWheelDiameter_MM
            XDistanceO: float=(OD.Xodom.position()-XodomOffset) / 360 * (OD.OdomWheelDiameter_MM*math.pi)
            YDistanceO: float=OD.Yodom.position() / 360 * (OD.OdomWheelDiameter_MM*math.pi)
            # Final filtered distance calculations using powered motors and dead wheel odometry.

            XBaseline=(OD.XDistanceI + OD.XDistanceLocalM + XDistanceO)/3
            XResidualI=abs(OD.XDistanceI - XBaseline)
            XResidualM=abs(OD.XDistanceLocalM - XBaseline)
            XResidualO=abs(XDistanceO - XBaseline)

            XInvVarI=1/(XResidualI**2 + BaseI**2)
            XInvVarM=1/(XResidualM**2 + BaseM**2)
            XInvVarO=1/(XResidualO**2 + BaseO**2)

            XWeightTotal=XInvVarI + XInvVarM + XInvVarO
            XIWeight=XInvVarI/XWeightTotal
            XMWeight=XInvVarM/XWeightTotal
            XOWeight=XInvVarO/XWeightTotal

            YBaseLine=(OD.YDistanceI + YDistanceO)/2
            YResidualI=abs(OD.YDistanceI - YBaseLine)
            YResidualO=abs(YDistanceO - YBaseLine)

            YInvVarI=1/(YResidualI**2 + BaseI**2)
            YInvVarO=1/(YResidualO**2 + BaseO**2)

            YWeightTotal=YInvVarI  + YInvVarO
            YIWeight=YInvVarI/YWeightTotal
            YOWeight=YInvVarO/YWeightTotal

            XLocalDistanceTotal=((XIWeight * OD.XDistanceI) + (XMWeight * OD.XDistanceLocalM) + (XOWeight * XDistanceO))
            YLocalDistanceTotal=(YIWeight * OD.YDistanceI) + (YOWeight * YDistanceO)

            OD.XPosition+=math.sin(OD.FilteredYaw_Rad) * XLocalDistanceTotal + math.cos(OD.FilteredYaw_Rad) * YLocalDistanceTotal
            OD.YPosition+=math.cos(OD.FilteredYaw_Rad) * XLocalDistanceTotal - math.sin(OD.FilteredYaw_Rad) * YLocalDistanceTotal

            # Update the old positions for the next iteration.
            LeftDegOld=OD.leftEncodedMotor.position()
            RightDegOld=OD.rightEncodedMotor.position()

            OD.Xodom.set_position(0)
            OD.Yodom.set_position(0)
            OldHeading=OD.Inertial.heading()
            OldYAxis=OD.PureYaxis_G
            OldXAxis=OD.PureXaxis_G
            OldZAxis=OD.PureZaxis_G

            wait(10 - (OD.Timer.time()-StartTime), MSEC)

    def StartOD(self, Inertial: Inertial, InertialXOffset: float, InertialYOffset: float, InertialZOffset: float, LeftEncodedMotor: Motor, RightEncodedMotor: Motor, RobotWidthMM: float, WheelDiameterMM: float, GearRatio: float, Xodom: Rotation, Yodom: Rotation, OdomWheelDiameterMM: float, XOdomOffset: float, YodomOffset: float) -> Thread:
        self.Inertial = Inertial
        self.InertialXOffset = InertialXOffset
        self.InertialYOffset = InertialYOffset
        self.InertialZOffset = InertialZOffset
        self.leftEncodedMotor = LeftEncodedMotor
        self.rightEncodedMotor = RightEncodedMotor
        self.RobotWidthMM = RobotWidthMM
        self.WheelDiameterMM = WheelDiameterMM
        self.Xodom = Xodom
        self.Yodom = Yodom
        self.OdomWheelDiameter_MM = OdomWheelDiameterMM
        self.XOdomOffset = XOdomOffset
        self.YOdomOffset = YodomOffset
        self.GearRatio = GearRatio
        odometry.calaibrate_inertial()
        self.ODT = Thread(odometry._Run)
        self.ODT.start()

        return self.ODT
    
    def StopOD(self):
        self.Running=False

    def simulate_impulse(self, left_rpm: float, right_rpm: float, duration_s: float, update_interval_s: float = 0.02):
        if not hasattr(self, 'leftEncodedMotor') or not hasattr(self, 'rightEncodedMotor'):
            return []

        self.leftEncodedMotor.set_velocity(left_rpm)
        self.rightEncodedMotor.set_velocity(right_rpm)
        elapsed = 0.0
        summary = []

        while elapsed < duration_s and not getattr(self.ODT, 'stop_event', threading.Event()).is_set():
            step = min(update_interval_s, duration_s - elapsed)
            left_rpm_command = self.leftEncodedMotor.velocity(RPM)
            right_rpm_command = self.rightEncodedMotor.velocity(RPM)

            left_mps = _rpm_to_mps(left_rpm_command, self.WheelDiameterMM, self.GearRatio)
            right_mps = _rpm_to_mps(right_rpm_command, self.WheelDiameterMM, self.GearRatio)
            forward_mps = (left_mps + right_mps) / 2.0
            if abs(self.RobotWidthMM) < 1e-6:
                yaw_dps = 0.0
            else:
                yaw_dps = math.degrees((right_mps - left_mps) / (self.RobotWidthMM / 1000.0))

            if hasattr(self.leftEncodedMotor, 'update'):
                self.leftEncodedMotor.update(step)
            if hasattr(self.rightEncodedMotor, 'update'):
                self.rightEncodedMotor.update(step)
            if hasattr(self.Inertial, 'update'):
                self.Inertial.update(forward_mps, yaw_dps, step)

            if hasattr(self.Xodom, 'update'):
                self.Xodom.update((left_rpm_command + right_rpm_command) / 2.0, step)
            if hasattr(self.Yodom, 'update'):
                self.Yodom.update(0.0, step)

            summary.append({
                'time_s': elapsed + step,
                'left_deg': self.leftEncodedMotor.position(),
                'right_deg': self.rightEncodedMotor.position(),
                'heading': self.Inertial.heading(),
                'motor_forward_mps': forward_mps,
                'odom_forward_mps': self.Xodom.linear_speed_mps(self.OdomWheelDiameter_MM) if hasattr(self.Xodom, 'linear_speed_mps') else None,
                'inertial_forward_mps': self.Inertial.speed_mps() if hasattr(self.Inertial, 'speed_mps') else None,
                'inertial_speed_from_accel_mps': self.Inertial.speed_from_accel_mps() if hasattr(self.Inertial, 'speed_from_accel_mps') else None,
                'x': self.XPosition,
                'y': self.YPosition,
            })

            elapsed += step
            wait(int(step * 1000), MSEC)

        self.leftEncodedMotor.set_velocity(0.0)
        self.rightEncodedMotor.set_velocity(0.0)
        return summary

OD=odometry()


def _rpm_to_mps(rpm: float, wheel_diameter_mm: float, gear_ratio: float) -> float:
    wheel_radius_m = (wheel_diameter_mm / 1000.0) / 2.0
    wheel_circumference_m = 2 * math.pi * wheel_radius_m
    return (rpm / gear_ratio) * wheel_circumference_m / 60.0


def degrees_to_meters(degrees: float, wheel_diameter_mm: float) -> float:
    return (degrees / 360.0) * math.pi * (wheel_diameter_mm / 1000.0)


class SimMotor(Motor):
    def __init__(self, name="SimMotor", wheel_diameter_mm: float = 68.0, gear_ratio: float = 1.0, encoder_drift_deg_per_s: float = 0.05, encoder_noise_std_deg: float = 0.2, velocity_noise_std_rpm: float = 0.5):
        self.name = name
        self._true_position_deg: float = 0.0
        self._position_bias_deg: float = 0.0
        self._velocity_rpm: float = 0.0
        self._velocity_bias_rpm: float = 0.0
        self.wheel_diameter_mm = wheel_diameter_mm
        self.gear_ratio = gear_ratio
        self.encoder_drift_deg_per_s = encoder_drift_deg_per_s
        self.encoder_noise_std_deg = encoder_noise_std_deg
        self.velocity_noise_std_rpm = velocity_noise_std_rpm

    def position(self) -> int:
        noise = random.gauss(0.0, self.encoder_noise_std_deg) if self.encoder_noise_std_deg > 0 else 0.0
        reported = self._true_position_deg + self._position_bias_deg + noise
        return int(reported)

    def velocity(self, unit=None) -> float:
        noise = random.gauss(0.0, self.velocity_noise_std_rpm) if self.velocity_noise_std_rpm > 0 else 0.0
        return self._velocity_rpm + self._velocity_bias_rpm + noise

    def set_position(self, position: int):
        self._true_position_deg = float(position)
        self._position_bias_deg = 0.0

    def set_velocity(self, rpm: float):
        self._velocity_rpm = float(rpm)

    def spin(self, direction, speed, unit=RPM):
        speed = float(speed)
        self._velocity_rpm = speed if direction == FORWARD else -speed

    def stop(self):
        self._velocity_rpm = 0.0

    def update(self, time_sec: float):
        self._true_position_deg += self._velocity_rpm * 360.0 / 60.0 * time_sec
        self._position_bias_deg += self.encoder_drift_deg_per_s * time_sec

    def true_position(self) -> float:
        return self._true_position_deg


class SimInertial(Inertial):
    def __init__(self, accel_bias_g: float = 0.01, gyro_bias_dps: float = 0.1, heading_drift_dps: float = 0.05, accel_noise_std_g: float = 0.02, gyro_noise_std_dps: float = 0.2, heading_noise_std_deg: float = 0.3):
        super().__init__()
        self.accel_bias_g = accel_bias_g
        self.gyro_bias_dps = gyro_bias_dps
        self.heading_drift_dps = heading_drift_dps
        self.accel_noise_std_g = accel_noise_std_g
        self.gyro_noise_std_dps = gyro_noise_std_dps
        self.heading_noise_std_deg = heading_noise_std_deg

    def calibrate(self):
        super().calibrate()

    def acceleration(self, axis) -> float:
        noise = random.gauss(0.0, self.accel_noise_std_g) if self.accel_noise_std_g > 0 else 0.0
        bias = self.accel_bias_g if axis == XAXIS else 0.0
        return self._accel_g[axis] + bias + noise

    def gyro_rate(self, axis) -> float:
        noise = random.gauss(0.0, self.gyro_noise_std_dps) if self.gyro_noise_std_dps > 0 else 0.0
        bias = self.gyro_bias_dps if axis == ZAXIS else 0.0
        return self._gyro_dps[axis] + bias + noise

    def heading(self) -> float:
        noise = random.gauss(0.0, self.heading_noise_std_deg) if self.heading_noise_std_deg > 0 else 0.0
        return (self._heading + self._heading_bias_deg + noise) % 360.0

    def true_heading(self) -> float:
        return self._heading

    def update(self, forward_mps: float, yaw_dps: float, time_sec: float):
        acceleration_mps2 = 0.0
        if time_sec > 0:
            acceleration_mps2 = (forward_mps - self._last_forward_mps) / time_sec
        self._accel_g[0] = acceleration_mps2 / 9.81
        self._accel_g[1] = 0.0
        self._accel_g[2] = 1.0
        self._gyro_dps[0] = 0.0
        self._gyro_dps[1] = 0.0
        self._gyro_dps[2] = yaw_dps
        self._heading = (self._heading + yaw_dps * time_sec) % 360.0
        self._heading_bias_deg += self.heading_drift_dps * time_sec
        self._last_forward_mps = forward_mps


class SimDriveTrain:
    def __init__(self, left_motor: SimMotor, right_motor: SimMotor, inertial: SimInertial, xodom: Rotation, yodom: Rotation, robot_width_mm: float = 250.0, wheel_diameter_mm: float = 68.0, gear_ratio: float = 1.0):
        self.left_motor = left_motor
        self.right_motor = right_motor
        self.inertial = inertial
        self.xodom = xodom
        self.yodom = yodom
        self.robot_width_mm = robot_width_mm
        self.wheel_diameter_mm = wheel_diameter_mm
        self.gear_ratio = gear_ratio
        self.drift_error_m = 0.0
        self.true_x_m = 0.0
        self.true_y_m = 0.0

    def step(self, time_sec: float):
        left_rpm = self.left_motor.velocity(RPM)
        right_rpm = self.right_motor.velocity(RPM)
        left_mps = _rpm_to_mps(left_rpm, self.wheel_diameter_mm, self.gear_ratio)
        right_mps = _rpm_to_mps(right_rpm, self.wheel_diameter_mm, self.gear_ratio)
        forward_mps = (left_mps + right_mps) / 2.0
        if abs(self.robot_width_mm) < 1e-3:
            yaw_dps = 0.0
        else:
            yaw_dps = math.degrees((right_mps - left_mps) / (self.robot_width_mm / 1000.0))

        self.left_motor.update(time_sec)
        self.right_motor.update(time_sec)
        self.xodom.update_from_linear(forward_mps, self.wheel_diameter_mm, time_sec)
        self.yodom.update(0.0, time_sec)
        self.inertial.update(forward_mps, yaw_dps, time_sec)

        distance_moved = forward_mps * time_sec
        heading_rad = math.radians(self.inertial._heading)
        self.true_x_m += distance_moved * math.cos(heading_rad)
        self.true_y_m += distance_moved * math.sin(heading_rad)
        self.drift_error_m += abs(distance_moved) * 0.001 * time_sec

        return {
            "forward_mps": forward_mps,
            "yaw_dps": yaw_dps,
            "left_deg": self.left_motor.position(),
            "right_deg": self.right_motor.position(),
            "heading": self.inertial.heading(),
            "true_x_m": self.true_x_m,
            "true_y_m": self.true_y_m,
            "drift_error_m": self.drift_error_m,
        }

    def drive_rpm(self, left_rpm: float, right_rpm: float):
        self.left_motor.set_velocity(left_rpm)
        self.right_motor.set_velocity(right_rpm)

    def stop(self):
        self.left_motor.stop()
        self.right_motor.stop()

    def sensor_summary(self):
        return {
            "left_position_deg": self.left_motor.position(),
            "right_position_deg": self.right_motor.position(),
            "x_odom_deg": self.xodom.position(),
            "y_odom_deg": self.yodom.position(),
            "heading": self.inertial.heading(),
            "motor_forward_mps": (_rpm_to_mps(self.left_motor.velocity(RPM), self.wheel_diameter_mm, self.gear_ratio) + _rpm_to_mps(self.right_motor.velocity(RPM), self.wheel_diameter_mm, self.gear_ratio)) / 2.0,
            "xodom_forward_mps": self.xodom.linear_speed_mps(self.wheel_diameter_mm),
            "inertial_forward_mps": self.inertial.speed_mps(),
            "inertial_speed_from_accel_mps": self.inertial.speed_from_accel_mps(),
            "accel_x_g": self.inertial.acceleration(XAXIS),
            "accel_y_g": self.inertial.acceleration(YAXIS),
            "accel_z_g": self.inertial.acceleration(ZAXIS),
        }

    def true_summary(self):
        return {
            "true_left_deg": self.left_motor.true_position(),
            "true_right_deg": self.right_motor.true_position(),
            "true_x_m": self.true_x_m,
            "true_y_m": self.true_y_m,
            "true_heading": self.inertial.true_heading(),
            "drift_error_m": self.drift_error_m,
        }

    def speed_consistency(self):
        left_mps = _rpm_to_mps(self.left_motor.velocity(RPM), self.wheel_diameter_mm, self.gear_ratio)
        right_mps = _rpm_to_mps(self.right_motor.velocity(RPM), self.wheel_diameter_mm, self.gear_ratio)
        motor_forward_mps = (left_mps + right_mps) / 2.0
        xodom_forward_mps = self.xodom.linear_speed_mps(self.wheel_diameter_mm)
        inertial_forward_mps = self.inertial.speed_mps()
        inertial_accel_forward_mps = self.inertial.speed_from_accel_mps()
        return {
            "motor_forward_mps": motor_forward_mps,
            "xodom_forward_mps": xodom_forward_mps,
            "inertial_forward_mps": inertial_forward_mps,
            "inertial_accel_forward_mps": inertial_accel_forward_mps,
            "motor_to_xodom_diff_mps": motor_forward_mps - xodom_forward_mps,
            "motor_to_inertial_diff_mps": motor_forward_mps - inertial_forward_mps,
            "inertial_accel_diff_mps": inertial_forward_mps - inertial_accel_forward_mps,
        }


def impulse_speed(left_rpm: float, right_rpm: float, duration_s: float, drivetrain: SimDriveTrain, update_interval_s: float = 0.02):
    drivetrain.drive_rpm(left_rpm, right_rpm)
    elapsed = 0.0
    summary = []
    while elapsed < duration_s:
        step = min(update_interval_s, duration_s - elapsed)
        state = drivetrain.step(step)
        summary.append(state)
        elapsed += step
        wait(int(step * 1000), MSEC)
    drivetrain.stop()
    return summary


def run_drift_accuracy_test():
    left_motor = SimMotor("LeftMotor", wheel_diameter_mm=68.0, gear_ratio=1.0, encoder_drift_deg_per_s=0.1, encoder_noise_std_deg=0.5, velocity_noise_std_rpm=0.8)
    right_motor = SimMotor("RightMotor", wheel_diameter_mm=68.0, gear_ratio=1.0, encoder_drift_deg_per_s=0.08, encoder_noise_std_deg=0.5, velocity_noise_std_rpm=0.8)
    inertial = SimInertial(accel_bias_g=0.02, gyro_bias_dps=0.15, heading_drift_dps=0.08, accel_noise_std_g=0.03, gyro_noise_std_dps=0.25, heading_noise_std_deg=0.4)
    xodom = Rotation()
    yodom = Rotation()

    OD.StartOD(inertial, 0, 0, 0, left_motor, right_motor, 250.0, 68.0, 1.0, xodom, yodom, 76.2, 0, 0)

    print("--- Drift and Accuracy Simulation (using odometry) ---")
    print("Impulse 1: straight forward, both motors 100 RPM for 20.0 seconds")
    OD.simulate_impulse(100.0, 100.0, 20.0, update_interval_s=0.05)

    estimated_distance = math.hypot(OD.XPosition, OD.YPosition)
    measured_left_deg = left_motor.position()
    measured_right_deg = right_motor.position()
    true_left_deg = left_motor.true_position()
    true_right_deg = right_motor.true_position()
    true_distance = degrees_to_meters((true_left_deg + true_right_deg) / 2.0, 68.0)
    measured_heading = inertial.heading()
    true_heading = inertial.true_heading()
    heading_error = abs((measured_heading - true_heading + 180) % 360 - 180)

    print("After straight impulse:")
    print(f"  Estimated pose: x={OD.XPosition:.3f} m, y={OD.YPosition:.3f} m")
    print(f"  True distance: {true_distance:.3f} m")
    print(f"  Estimated distance from odometry: {estimated_distance:.3f} m")
    print(f"  Estimated heading: {measured_heading:.2f} deg, true heading: {true_heading:.2f} deg")
    print(f"  Heading error: {heading_error:.2f} deg")
    print(f"  Encoder left measured: {measured_left_deg} deg, true: {true_left_deg:.1f} deg")
    print(f"  Encoder right measured: {measured_right_deg} deg, true: {true_right_deg:.1f} deg")
    print(f"  X odom rotation: {OD.Xodom.position()} deg")
    print(f"  Y odom rotation: {OD.Yodom.position()} deg")
    print(f"  X odom forward speed: {OD.Xodom.linear_speed_mps(OD.OdomWheelDiameter_MM):.3f} m/s")
    print(f"  Inertial forward speed: {inertial.speed_mps():.3f} m/s")
    print(f"  Inertial accel-derived speed: {inertial.speed_from_accel_mps():.3f} m/s")

    print("\nImpulse 2: rotation, left 80 RPM, right -80 RPM for 1.0 seconds")
    OD.simulate_impulse(80.0, -80.0, 1.0, update_interval_s=0.05)
    measured_heading = inertial.heading()
    true_heading = inertial.true_heading()
    heading_error = abs((measured_heading - true_heading + 180) % 360 - 180)
    estimated_distance = math.hypot(OD.XPosition, OD.YPosition)

    print("After rotation impulse:")
    print(f"  Estimated pose: x={OD.XPosition:.3f} m, y={OD.YPosition:.3f} m")
    print(f"  Estimated heading: {measured_heading:.2f} deg")
    print(f"  True heading: {true_heading:.2f} deg")
    print(f"  Heading error: {heading_error:.2f} deg")
    print(f"  X odom rotation: {OD.Xodom.position()} deg")
    print(f"  Y odom rotation: {OD.Yodom.position()} deg")
    print(f"  X odom forward speed: {OD.Xodom.linear_speed_mps(OD.OdomWheelDiameter_MM):.3f} m/s")
    print(f"  Inertial forward speed: {inertial.speed_mps():.3f} m/s")
    print(f"  Inertial accel-derived speed: {inertial.speed_from_accel_mps():.3f} m/s")

    OD.StopOD()


if __name__ == "__main__":
    run_drift_accuracy_test()
    OD.StopOD()


# def run_simulation_test():
#     left_motor = SimMotor("LeftMotor", wheel_diameter_mm=68.0, gear_ratio=1.0)
#     right_motor = SimMotor("RightMotor", wheel_diameter_mm=68.0, gear_ratio=1.0)
#     inertial = SimInertial()
#     inertial.calibrate()
#     xodom = Rotation()
#     yodom = Rotation()

#     drivetrain = SimDriveTrain(
#         left_motor=left_motor,
#         right_motor=right_motor,
#         inertial=inertial,
#         xodom=xodom,
#         yodom=yodom,
#         robot_width_mm=250.0,
#         wheel_diameter_mm=68.0,
#         gear_ratio=1.0,
#     )

#     print("--- VEX V5 Sensor Simulation Test ---")
#     print("Starting impulse: both motors 100 RPM for 1.0 seconds")
#     results = impulse_speed(100.0, 100.0, 5.0, drivetrain, update_interval_s=0.05)

#     final = drivetrain.sensor_summary()
#     print("Final sensor state:")
#     print(f"  Left encoder: {final['left_position_deg']} deg")
#     print(f"  Right encoder: {final['right_position_deg']} deg")
#     print(f"  X odom: {final['x_odom_deg']} deg")
#     print(f"  Heading: {final['heading']:.2f} deg")
#     print(f"  Acceleration X: {final['accel_x_g']:.3f} g")
#     print(f"  Acceleration Z: {final['accel_z_g']:.3f} g")
#     print(f"  Distance traveled: {degrees_to_meters((final['left_position_deg'] + final['right_position_deg']) / 2.0, 68.0):.3f} m")

#     print("\nSecond impulse: left motor 80 RPM, right motor -80 RPM for 0.8 seconds")
#     results = impulse_speed(80.0, -80.0, 0.8, drivetrain, update_interval_s=0.05)
#     final = drivetrain.sensor_summary()
#     print("Final sensor state after turn:")
#     print(f"  Left encoder: {final['left_position_deg']} deg")
#     print(f"  Right encoder: {final['right_position_deg']} deg")
#     print(f"  Heading: {final['heading']:.2f} deg")
#     print(f"  Acceleration X: {final['accel_x_g']:.3f} g")
#     print(f"  Acceleration Z: {final['accel_z_g']:.3f} g")
#     print(f"  X odom: {final['x_odom_deg']} deg")


# if __name__ == "__main__":
#     run_simulation_test()