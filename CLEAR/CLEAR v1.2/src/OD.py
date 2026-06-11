from vex import *
import math

class odometry:
    def __init__(self):
        self.leftEncodedMotor: Motor
        self.rightEncodedMotor: Motor
        self.XPosition: float = 0.0
        self.YPosition: float = 0.0
        self.HeadingI: float = 0.0
        self.Inertial: Inertial
        self.InertialSensorUse=False
        self.XSpeedLocalI: float = 0.0
        self.YSpeedLocalI: float = 0.0
        self.LDistanceM: float = 0.0
        self.RDistanceM: float = 0.0
        self.XDistanceI: float = 0.0
        self.YDistanceI: float = 0.0
        self.RobotWidthMM: float = 0.0
        self.WheelDiameterMM: float = 0.0
        self.DistanceGlobalM: float = 0.0
        self.Timer=Timer()
    
    @staticmethod
    def _Run() -> None:
        GlobalScope = dir()
        for item in GlobalScope:      
            try:
                item_type=str(type(eval(item)))
            except NameError:
                continue
            if item_type == "<class 'inertial'>":
                if item != "Inertial":
                    OD.InertialSensorUse=True
                    OD.Inertial = eval(item)
                    break
        
        OD.XSpeedLocalI=(OD.Inertial.acceleration(AxisType.XAXIS) * 9.81) * 0.01
        OD.YSpeedLocalI=(OD.Inertial.acceleration(AxisType.YAXIS) * 9.81) * 0.01
        OD.HeadingI=OD.Inertial.heading()

        LeftDegreesOld=OD.leftEncodedMotor.position()
        RightDegreesOld=OD.rightEncodedMotor.position()
        HeadingM: float=0.0

        while True:
            StartTime=OD.Timer.time()
            OD.XSpeedLocalI+=(OD.Inertial.acceleration(AxisType.XAXIS) * 9.81) * 0.01
            OD.YSpeedLocalI+=(OD.Inertial.acceleration(AxisType.YAXIS) * 9.81) * 0.01
            OD.XDistanceI=OD.XSpeedLocalI*0.01
            OD.YDistanceI=OD.YSpeedLocalI*0.01

            XDistanceGlobalI: float=(OD.XDistanceI*math.cos(OD.HeadingI)) - (OD.YDistanceI*math.sin(OD.HeadingI))
            YDistanceGlobalI: float=(OD.XDistanceI*math.cos(OD.HeadingI)) + (OD.YDistanceI*math.sin(OD.HeadingI))

            OD.LDistanceM=((OD.leftEncodedMotor.position()-LeftDegreesOld) / 360) * (OD.WheelDiameterMM*math.pi)
            OD.RDistanceM=((OD.rightEncodedMotor.position()-RightDegreesOld) / 360) * (OD.WheelDiameterMM*math.pi)
            OD.HeadingI=OD.Inertial.heading()
            HeadingM=(OD.LDistanceM-OD.RDistanceM)/(OD.RobotWidthMM / OD.WheelDiameterMM)

            OD.DistanceGlobalM=(OD.LDistanceM+OD.RDistanceM)/2
            XDistanceGlobalM: float=OD.DistanceGlobalM*math.cos(OD.HeadingI)
            YDistanceGlobalM: float=OD.DistanceGlobalM*math.sin(OD.HeadingI)

            OD.XPosition+=XDistanceGlobalI + XDistanceGlobalM/2
            OD.YPosition+=YDistanceGlobalI + YDistanceGlobalM/2

            LeftDegreesOld=OD.leftEncodedMotor.position()
            RightDegreesOld=OD.rightEncodedMotor.position()

            wait(10 - (OD.Timer.time()-StartTime), MSEC)

    def StartOD(self, LeftEncodedMotor: Motor, RightEncodedMotor: Motor, RobotWidthMM: float, WheelDiameterMM: float,) -> Thread:
        self.leftEncodedMotor = LeftEncodedMotor
        self.rightEncodedMotor = RightEncodedMotor
        self.RobotWidthMM = RobotWidthMM
        self.WheelDiameterMM = WheelDiameterMM
        self.ODT=Thread(OD._Run)

        return self.ODT
    
    def StopOD(self):
        self.ODT.stop()

OD=odometry()