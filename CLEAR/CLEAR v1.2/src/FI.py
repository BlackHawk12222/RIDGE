from vex import *
import math, cmath

class FilteredInertial:
    def __init__(self, inertial: Inertial, XOffset_mm: float=0.0, YOffset_mm: float=0.0, ZOffset_mm: float=0.0, PollRate_Hz: int=50) -> None:
        self.Inertial: Inertial = inertial
        self.XOffset_mm=XOffset_mm
        self.YOffset_mm=YOffset_mm
        self.ZOffset_mm=ZOffset_mm
        self.PollRate_Hz=min(max(PollRate_Hz, 200), 10)
        self.XFiltered_G: float = 0.0
        self.YFiltered_G: float = 0.0
        self.ZFiltered_G: float = 0.0
        self.FilteredRoll_Rad: float = 0.0
        self.FilteredPitch_Rad: float = 0.0
        self.FilteredYaw_Rad: float = 0.0
        self.Timer=Timer()
        self.Running=True
        self.XgravityOffsetDegrees: float = 0.0
        self.YgravityOffsetDegrees: float = 0.0
        Thread(self._Run)

    def calabrate_inertial(self) -> None:
        self.Inertial.calibrate()
        try:
            RollA=math.atan2(self.Inertial.acceleration(AxisType.YAXIS), cmath.sqrt((self.Inertial.acceleration(AxisType.ZAXIS)**2) + (self.Inertial.acceleration(AxisType.XAXIS)**2)).real)
        except ZeroDivisionError:
            RollA=0
        self.XgravityOffsetDegrees=RollA
        try:
            PitchA=math.atan2(0-self.Inertial.acceleration(AxisType.XAXIS), cmath.sqrt((self.Inertial.acceleration(AxisType.YAXIS)**2) + (self.Inertial.acceleration(AxisType.ZAXIS)**2)).real)
        except ZeroDivisionError:
            PitchA=0
        self.YgravityOffsetDegrees=PitchA
    
    def _Run(self) -> None:
        AxisWeight=0.98
        TotalGravity_G=1
        self.TimeStep_Sec=1/self.PollRate_Hz
        self.calabrate_inertial()
        while self.Running:
            StartTime=self.Timer.time()
            ZAxis_G=self.Inertial.acceleration(AxisType.ZAXIS)
            XAxis_G=self.Inertial.acceleration(AxisType.XAXIS)
            YAxis_G=self.Inertial.acceleration(AxisType.YAXIS)
            
            # if ZAxis_G + XAxis_G + YAxis_G != 1:
            #     AngleOfGravity_Rad=math.acos(ZAxis_G/TotalGravity_G)
            #     XAxisForGravity_G=XAxis_G - (XAxis_G - (TotalGravity_G*math.sin(AngleOfGravity_Rad)))
            #     YAxisForGravity_G=YAxis_G - (YAxis_G - (TotalGravity_G*math.sin(AngleOfGravity_Rad)))
            #     ZaxisForGravity_G=ZAxis_G - (ZAxis_G - (TotalGravity_G*math.cos(AngleOfGravity_Rad)))
            # else:
            #     XAxisForGravity_G=self.Inertial.acceleration(AxisType.XAXIS)
            #     YAxisForGravity_G=self.Inertial.acceleration(AxisType.YAXIS)
            #     ZaxisForGravity_G=self.Inertial.acceleration(AxisType.ZAXIS)

            # XAxisForGravity_G=self.Inertial.acceleration(AxisType.XAXIS)
            # YAxisForGravity_G=self.Inertial.acceleration(AxisType.YAXIS)
            # ZAxisForGravity_G=self.Inertial.acceleration(AxisType.ZAXIS)

            # if XAxisForGravity_G==0:
            #     XAxisForGravity_G=0.0000001
            # if YAxisForGravity_G==0:
            #     YAxisForGravity_G=0.0000001
            # if ZAxisForGravity_G==0:
            #     ZAxisForGravity_G=0.0000001

            self.FilteredRoll_Rad=math.radians(self.Inertial.orientation(ROLL))
            self.FilteredPitch_Rad=math.radians(self.Inertial.orientation(PITCH))

            # try:
            #     self.FilteredRoll_Rad=AxisWeight*(self.FilteredRoll_Rad + math.radians(self.Inertial.gyro_rate(YAXIS) * self.TimeStep_Sec)) + (1-AxisWeight)*(math.atan2(YAxisForGravity_G, cmath.sqrt((ZAxisForGravity_G**2) + (XAxisForGravity_G**2)).real)-self.XgravityOffsetDegrees)
            # except ZeroDivisionError:
            #     self.FilteredRoll_Rad=AxisWeight*(self.FilteredRoll_Rad + math.radians(self.Inertial.gyro_rate(YAXIS) * self.TimeStep_Sec)) + (1-AxisWeight)*(self.XgravityOffsetDegrees)

            # try:
            #     self.FilteredPitch_Rad=AxisWeight*(self.FilteredPitch_Rad + (math.radians(self.Inertial.gyro_rate(XAXIS) * self.TimeStep_Sec))) + (1-AxisWeight)*((math.atan2(0-XAxisForGravity_G, cmath.sqrt((YAxisForGravity_G**2) + (ZAxisForGravity_G**2)).real))-self.YgravityOffsetDegrees)
            # except ZeroDivisionError:
            #     self.FilteredPitch_Rad=AxisWeight*(self.FilteredPitch_Rad + (math.radians(self.Inertial.gyro_rate(XAXIS) * self.TimeStep_Sec))) + (1-AxisWeight)*(self.YgravityOffsetDegrees)
            
            CentrificalAcceleration_G=(self.Inertial.gyro_rate(ZAXIS)**2) * math.sqrt(self.XOffset_mm**2 + self.YOffset_mm**2)
            CentificalAngle_Rad=math.atan2(self.YOffset_mm, self.XOffset_mm)
            
            self.ZFiltered_G=ZAxis_G - (TotalGravity_G*math.cos(self.FilteredRoll_Rad) * math.cos(self.FilteredPitch_Rad))
            self.XFiltered_G=XAxis_G - (TotalGravity_G*math.sin(self.FilteredPitch_Rad)) - (CentrificalAcceleration_G * math.cos(CentificalAngle_Rad))
            self.YFiltered_G=YAxis_G - (TotalGravity_G*math.sin(self.FilteredRoll_Rad)) - (CentrificalAcceleration_G * math.sin(CentificalAngle_Rad))

            self.FilteredYaw_Rad=math.radians(self.Inertial.orientation(YAW))

            print("X: ", self.XFiltered_G, "Y: ", self.YFiltered_G, "Z: ", self.ZFiltered_G, "Yaw: ", math.degrees(self.FilteredYaw_Rad), "Pitch: ", math.degrees(self.FilteredPitch_Rad), "Roll: ", math.degrees(self.FilteredRoll_Rad))
            
            wait((1000/self.PollRate_Hz) - (self.Timer.time()-StartTime), MSEC)

    def acceleration(self, Axis: AxisType) -> float:
        if Axis==AxisType.XAXIS:
            return self.XFiltered_G
        elif Axis==AxisType.YAXIS:
            return self.YFiltered_G
        elif Axis==AxisType.ZAXIS:
            return self.ZFiltered_G
        else:
            raise ValueError("Invalid axis type. Must be XAXIS, YAXIS, or ZAXIS.")
    
    def heading_Deg(self) -> float:
        return math.degrees(self.FilteredYaw_Rad)
    
    def heading_Rad(self) -> float:
        return self.FilteredYaw_Rad
    
    def rotation_Deg(self) -> float:
        return math.degrees(self.FilteredYaw_Rad)
    
    def rotation_Rad(self) -> float:
        return self.FilteredYaw_Rad
    
    def roll_Deg(self) -> float:
        return math.degrees(self.FilteredRoll_Rad)
    
    def roll_Rad(self) -> float:
        return self.FilteredRoll_Rad

    def pitch_Deg(self) -> float:
        return math.degrees(self.FilteredPitch_Rad)
    
    def pitch_Rad(self) -> float:
        return self.FilteredPitch_Rad

    def yaw_Rad(self) -> float:
        return self.FilteredYaw_Rad
    
    def yaw_Deg(self) -> float:
        return math.degrees(self.FilteredYaw_Rad)
    
    def Hz(self) -> int:
        return self.PollRate_Hz
    
    def change_poll_rate(self, PollRate_Hz: int) -> None:
        self.PollRate_Hz=min(max(PollRate_Hz, 200), 10)