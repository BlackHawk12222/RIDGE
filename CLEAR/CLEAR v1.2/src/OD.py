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
        self.ODT: Thread
        self.Xodom: Rotation
        self.Yodom: Rotation
        self.OdomWheelDiameterMM: float = 0.0
        self.XOdomOffset: float = 0.0
        self.YOdomOffset: float = 0.0
    
    @staticmethod
    def _Run() -> None:
        
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
            XDistanceGlobalM: float=OD.DistanceGlobalM*math.cos(HeadingM)
            YDistanceGlobalM: float=OD.DistanceGlobalM*math.sin(HeadingM)

            XodomOffset=OD.Inertial.heading() * OD.XOdomOffset/OD.OdomWheelDiameterMM
            XDistanceO: float=(OD.Xodom.angle()-XodomOffset) / 360 * (OD.OdomWheelDiameterMM*math.pi)
            YDistanceO: float=OD.Yodom.angle() / 360 * (OD.OdomWheelDiameterMM*math.pi)

            XDistanceGlobalO: float=XDistanceO*math.cos(OD.HeadingI) - YDistanceO*math.sin(OD.HeadingI)
            YDistanceGlobalO: float=XDistanceO*math.cos(OD.HeadingI) + YDistanceO*math.sin(OD.HeadingI)

            OD.XPosition+=XDistanceGlobalI + XDistanceGlobalM + XDistanceGlobalO/3
            OD.YPosition+=YDistanceGlobalI + YDistanceGlobalM + YDistanceGlobalO/3


            LeftDegreesOld=OD.leftEncodedMotor.position()
            RightDegreesOld=OD.rightEncodedMotor.position()

            wait(10 - (OD.Timer.time()-StartTime), MSEC)

    def StartOD(self, Inertial: Inertial, LeftEncodedMotor: Motor, RightEncodedMotor: Motor, RobotWidthMM: float, WheelDiameterMM: float, Xodom: Rotation, Yodom: Rotation, OdomWheelDiameterMM: float, XOdomOffset: float, YodomOffset: float) -> Thread:
        self.Inertial = Inertial
        self.leftEncodedMotor = LeftEncodedMotor
        self.rightEncodedMotor = RightEncodedMotor
        self.RobotWidthMM = RobotWidthMM
        self.WheelDiameterMM = WheelDiameterMM
        self.Xodom = Xodom
        self.Yodom = Yodom
        self.OdomWheelDiameterMM = OdomWheelDiameterMM
        self.XOdomOffset = XOdomOffset
        self.YOdomOffset = YodomOffset
        self.ODT=Thread(OD._Run)

        return self.ODT
    
    def StopOD(self):
        self.ODT.stop()

OD=odometry()