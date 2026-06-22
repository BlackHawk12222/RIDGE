import math, time

class Motor:
    def position(self) -> int:
        return 0
    
    def velocity(self, rpm: float) -> float:
        return 0.0
    
    def set_position(self, position: int):
        pass

RPM=0
YAXIS=0
XAXIS=0
ZAXIS=0
MSEC="MSEC"

def wait(Number: int, Unit: str):
    if Unit == "MSEC":
        time.sleep(Number / 1000)
    elif Unit == "SECONDS":
        time.sleep(Number)

class AxisType:
    XAXIS = 0
    YAXIS = 1
    ZAXIS = 2

class Inertial:
    def calibrate(self):
        pass
    
    def acceleration(self, axis) -> float:
        return 1.0
    
    def gyro_rate(self, axis) -> float:
        return 0.0
    
    def heading(self) -> float:
        return 0.0

class Timer:
    
    def time(self):
        return int(time.time()*1000)

class Thread():
    def __init__(self, function):
        pass

    def stop(self):
        pass

class Rotation:
    def position(self) -> int:
        return 0
    
    def velocity(self, rpm: float) -> float:
        return 0.0
    
    def set_position(self, position: int):
        pass

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

    @staticmethod
    def calabrate_inertial():
        OD.Inertial.calibrate()
        RollA=math.degrees(math.atan(OD.Inertial.acceleration(AxisType.YAXIS)/OD.Inertial.acceleration(AxisType.ZAXIS)))
        OD.XgravityOffsetDegrees=RollA
        PitchA=math.degrees(math.asin(OD.Inertial.acceleration(AxisType.XAXIS)/9.81))
        OD.YgravityOffsetDegrees=PitchA

    @staticmethod
    def _Run() -> None:
        
        OD.XSpeedLocalI_MPS=(round(OD.Inertial.acceleration(AxisType.XAXIS), 3)*(0.01**2))
        OD.YSpeedLocalI_MPS=(round(OD.Inertial.acceleration(AxisType.YAXIS), 3)*(0.01**2))

        LeftDegOld=OD.leftEncodedMotor.position()
        RightDegOld=OD.rightEncodedMotor.position()
        FilteredRoll_Rad: float
        FilteredPitch_Rad: float
        FilteredYaw_Rad: float
        AxisWeight: float=0.9
        DistancePerDegree_Mpd: float=(math.pi*(OD.WheelDiameterMM/1000))/360
        TotalGravity=9.81
        OldXAxis: float = 0.0
        OldYAxis: float = 0.0
        OldZAxis: float = 0.0

        while True:
            StartTime=OD.Timer.time()

            # Cleaining acceleration data.
            LeftSpeed_Mps=((OD.leftEncodedMotor.velocity(RPM)/OD.GearRatio) * DistancePerDegree_Mpd) / 60
            RightSpeed_Mps=((OD.rightEncodedMotor.velocity(RPM)/OD.GearRatio) * DistancePerDegree_Mpd) / 60
            HeadingRate_Mps=math.degrees(((RightSpeed_Mps-LeftSpeed_Mps))/(OD.RobotWidthMM/1000))
            
            ZAxis_G=OD.Inertial.acceleration(AxisType.ZAXIS)
            XAxis_G=OD.Inertial.acceleration(AxisType.XAXIS)
            YAxis_G=OD.Inertial.acceleration(AxisType.YAXIS)
            XAxisForGravity_G=OD.Inertial.acceleration(AxisType.XAXIS) - (OD.Xodom.velocity(RPM) * OD.OdomWheelDiameter_MM * math.pi / 60)/OD.TimeStep_Sec
            YAxisForGravity_G=OD.Inertial.acceleration(AxisType.YAXIS) - (OD.Yodom.velocity(RPM) * OD.OdomWheelDiameter_MM * math.pi / 60)/OD.TimeStep_Sec

            FilteredRoll_Rad=AxisWeight*(FilteredRoll_Rad + math.radians(OD.Inertial.gyro_rate(YAXIS)* OD.TimeStep_Sec)) + (1-AxisWeight)*((math.atan(YAxisForGravity_G/ZAxis_G))-OD.XgravityOffsetDegrees)
            FilteredPitch_Rad=AxisWeight*(FilteredPitch_Rad + math.radians(OD.Inertial.gyro_rate(XAXIS)* OD.TimeStep_Sec)) + (1-AxisWeight)*((math.asin(XAxisForGravity_G/TotalGravity))-OD.YgravityOffsetDegrees)
            FilteredYaw_Rad=AxisWeight*(FilteredYaw_Rad + math.radians(OD.Inertial.gyro_rate(ZAXIS)* OD.TimeStep_Sec)) + (1-AxisWeight)* (FilteredYaw_Rad + HeadingRate_Mps*OD.TimeStep_Sec)

            CentrificalAcceleration_G=(OD.Inertial.gyro_rate(ZAXIS)**2) * math.sqrt(OD.InertialXOffset**2 + OD.InertialYOffset**2)
            CentificalAngle_Rad=math.atan(OD.InertialYOffset/OD.InertialXOffset)
            PureXaxis=((((XAxis_G * math.sin(FilteredPitch_Rad)) + (ZAxis_G * math.cos(FilteredPitch_Rad))) - (TotalGravity*math.sin(FilteredPitch_Rad))) - (CentrificalAcceleration_G * math.cos(CentificalAngle_Rad)) + OldXAxis) / 2
            PureYaxis=(((YAxis_G * math.sin(FilteredRoll_Rad) + (ZAxis_G * math.cos(FilteredRoll_Rad))) - (TotalGravity*math.sin(FilteredRoll_Rad)*math.cos(FilteredPitch_Rad))) - (CentrificalAcceleration_G * math.sin(CentificalAngle_Rad)) + OldYAxis) / 2
            PureZaxis=(((ZAxis_G * math.tan(FilteredPitch_Rad/FilteredRoll_Rad)) - (TotalGravity*math.cos(FilteredRoll_Rad)*math.cos(FilteredPitch_Rad))) - (CentrificalAcceleration_G * math.tan(CentificalAngle_Rad)) + OldZAxis) / 2

            # Inertial calulations.
            OD.XSpeedLocalI_MPS+=((PureXaxis*math.sin(FilteredPitch_Rad) + (PureZaxis*math.cos(FilteredRoll_Rad))))*(OD.TimeStep_Sec**2)
            OD.YSpeedLocalI_MPS+=((PureYaxis*math.cos(FilteredRoll_Rad) - (PureZaxis*math.sin(FilteredPitch_Rad))))*(OD.TimeStep_Sec**2)
            OD.XDistanceI=OD.XSpeedLocalI_MPS*OD.TimeStep_Sec
            OD.YDistanceI=OD.YSpeedLocalI_MPS*OD.TimeStep_Sec

            YawWeight=((OD.Xodom.velocity(RPM)* (OD.OdomWheelDiameter_MM/1000) * math.pi)/60) / OD.XSpeedLocalI_MPS

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

            BaseI=1
            BaseM=1
            BaseO=1

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

            OD.XPosition+=math.sin(FilteredYaw_Rad) * XLocalDistanceTotal + math.cos(FilteredYaw_Rad) * YLocalDistanceTotal
            OD.YPosition+=math.cos(FilteredYaw_Rad) * XLocalDistanceTotal - math.sin(FilteredYaw_Rad) * YLocalDistanceTotal

            # Update the old positions for the next iteration.
            LeftDegOld=OD.leftEncodedMotor.position()
            RightDegOld=OD.rightEncodedMotor.position()

            OD.Xodom.set_position(0)
            OD.Yodom.set_position(0)
            OldHeading=OD.Inertial.heading()
            OldYAxis=PureYaxis
            OldXAxis=PureXaxis
            OldZAxis=PureZaxis

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
        odometry.calabrate_inertial()
        self.ODT=Thread(odometry._Run)

        return self.ODT
    
    def StopOD(self):
        self.ODT.stop()

OD=odometry()