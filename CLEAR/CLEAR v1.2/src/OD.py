from vex import *
import math

class odometry:
    def __init__(self):
        self.leftEncodedMotor: Motor
        self.rightEncodedMotor: Motor
        self.XPosition: float = 0.0
        self.YPosition: float = 0.0
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
        self.Weight: float = 0.98
        self.GearRatio: float = 0.0
    
    @staticmethod
    def _Run() -> None:
        
        OD.XSpeedLocalI=(round(OD.Inertial.acceleration(AxisType.XAXIS), 3)*(0.01**2))
        OD.YSpeedLocalI=(round(OD.Inertial.acceleration(AxisType.YAXIS), 3)*(0.01**2))

        LeftDegreesOld=OD.leftEncodedMotor.position()
        RightDegreesOld=OD.rightEncodedMotor.position()
        FilteredRoll: float=0.0
        FilteredPitch: float=0.0
        FilteredYaw: float=0.0

        while True:
            StartTime=OD.Timer.time()

            # Cleaining acceleration data.
            LeftSpeed=((OD.leftEncodedMotor.position(DEGREES)-LeftDegreesOld) * math.pi * OD.WheelDiameterMM) / (0.01 * 360 *OD.GearRatio)
            RightSpeed=((OD.rightEncodedMotor.position(DEGREES)-RightDegreesOld) * math.pi * OD.WheelDiameterMM) / (0.01 * 360 *OD.GearRatio)
            HeadingRateM=(RightSpeed-LeftSpeed)/(OD.RobotWidthMM/1000)


            AxisWeight=0.9
            ZAxis=OD.Inertial.acceleration(AxisType.ZAXIS)
            XAxis=OD.Inertial.acceleration(AxisType.XAXIS)
            YAxis=OD.Inertial.acceleration(AxisType.YAXIS)

            FilteredRoll=AxisWeight*(FilteredRoll + OD.Inertial.gyro_rate(XAXIS)) + (1-AxisWeight)*math.degrees(math.atan2(YAxis, XAxis))
            FilteredPitch=AxisWeight*(FilteredPitch + OD.Inertial.gyro_rate(YAXIS)) + (1-AxisWeight)*math.degrees(math.atan2(0-XAxis, math.sqrt(ZAxis**2 + YAxis**2)))
            FilteredYaw=AxisWeight*(FilteredYaw + OD.Inertial.gyro_rate(ZAXIS)) + (1-AxisWeight)*HeadingRateM

            # Inertial calulations.
            OD.XSpeedLocalI+=(round(OD.Inertial.acceleration(AxisType.XAXIS), 3)*(0.01**2))
            OD.YSpeedLocalI+=(round(OD.Inertial.acceleration(AxisType.YAXIS), 3)*(0.01**2))
            OD.XDistanceI=OD.XSpeedLocalI*0.01
            OD.YDistanceI=OD.YSpeedLocalI*0.01

            XDistanceGlobalI: float=(OD.XDistanceI*math.degrees(math.cos(FilteredYaw))) - (OD.YDistanceI*math.degrees(math.sin(FilteredYaw)))
            YDistanceGlobalI: float=(OD.XDistanceI*math.degrees(math.cos(FilteredYaw))) + (OD.YDistanceI*math.degrees(math.sin(FilteredYaw)))

            # Motor calulations.
            OD.LDistanceM=((OD.leftEncodedMotor.position()-LeftDegreesOld) / 360) * (OD.WheelDiameterMM*math.pi)
            OD.RDistanceM=((OD.rightEncodedMotor.position()-RightDegreesOld) / 360) * (OD.WheelDiameterMM*math.pi)

            OD.DistanceGlobalM=(OD.LDistanceM+OD.RDistanceM)/2
            XDistanceGlobalM: float=OD.DistanceGlobalM*math.degrees(math.cos(FilteredYaw))
            YDistanceGlobalM: float=OD.DistanceGlobalM*math.degrees(math.sin(FilteredYaw))

            # Slip ratio calculations for comp. filter weight.
            try:
                SlipRatio=OD.Yodom.velocity()/(OD.leftEncodedMotor.velocity()+OD.rightEncodedMotor.velocity()/2)
            except ZeroDivisionError:
                SlipRatio=1

            Mtrust=min(max(SlipRatio-0.5, 0.0), 1.0)

            # Odometer calculations.
            XodomOffset=OD.Inertial.heading() * OD.XOdomOffset/OD.OdomWheelDiameterMM
            XDistanceO: float=(OD.Xodom.position()-XodomOffset) / 360 * (OD.OdomWheelDiameterMM*math.pi)
            YDistanceO: float=OD.Yodom.position() / 360 * (OD.OdomWheelDiameterMM*math.pi)

            XDistanceGlobalO: float=XDistanceO*math.degrees(math.cos(FilteredYaw)) - YDistanceO*math.degrees(math.sin(FilteredYaw))
            YDistanceGlobalO: float=XDistanceO*math.degrees(math.cos(FilteredYaw)) + YDistanceO*math.degrees(math.sin(FilteredYaw))

            # Final filtered distance calculations using powered motors and dead wheel odometry.
            XDistanceGlobalF: float=Mtrust*XDistanceGlobalM + (1-Mtrust)*XDistanceGlobalO
            YDistanceGlobalF: float=Mtrust*YDistanceGlobalM + (1-Mtrust)*YDistanceGlobalO

            OD.XPosition+=OD.Weight*XDistanceGlobalF + (1-OD.Weight)*XDistanceGlobalI
            OD.YPosition+=OD.Weight*YDistanceGlobalF + (1-OD.Weight)*YDistanceGlobalI

            # Update the old positions for the next iteration.
            LeftDegreesOld=OD.leftEncodedMotor.position()
            RightDegreesOld=OD.rightEncodedMotor.position()

            OD.Xodom.set_position(0)
            OD.Yodom.set_position(0)

            wait(10 - (OD.Timer.time()-StartTime), MSEC)

    def StartOD(self, Inertial: Inertial, LeftEncodedMotor: Motor, RightEncodedMotor: Motor, RobotWidthMM: float, WheelDiameterMM: float, GearRatio: float, Xodom: Rotation, Yodom: Rotation, OdomWheelDiameterMM: float, XOdomOffset: float, YodomOffset: float) -> Thread:
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
        self.GearRatio = GearRatio
        self.ODT=Thread(OD._Run)

        return self.ODT
    
    def StopOD(self):
        self.ODT.stop()

OD=odometry()