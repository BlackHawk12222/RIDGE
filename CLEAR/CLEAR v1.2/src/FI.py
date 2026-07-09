from vex import *
import math

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
            RollA=math.degrees(math.atan(self.Inertial.acceleration(AxisType.YAXIS)/self.Inertial.acceleration(AxisType.ZAXIS)))
        except ZeroDivisionError:
            RollA=0
        self.XgravityOffsetDegrees=RollA
        try:
            PitchA=math.degrees(math.asin(self.Inertial.acceleration(AxisType.XAXIS)/9.81))
        except ZeroDivisionError:
            PitchA=0
        self.YgravityOffsetDegrees=PitchA
    
    def _Run(self) -> None:
        AxisWeight=0.98
        TotalGravity=9.81
        self.TimeStep_Sec=1/self.PollRate_Hz
        self.calabrate_inertial()
        while self.Running:
            StartTime=self.Timer.time()
            ZAxis_G=self.Inertial.acceleration(AxisType.ZAXIS)
            XAxis_G=self.Inertial.acceleration(AxisType.XAXIS)
            YAxis_G=self.Inertial.acceleration(AxisType.YAXIS)
            
            if ZAxis_G + XAxis_G + YAxis_G != 1:
                AngleOfGravity_Rad=math.acos(ZAxis_G/TotalGravity)
                XAxisForGravity_G=XAxis_G - (XAxis_G - (TotalGravity*math.sin(AngleOfGravity_Rad)))
                YAxisForGravity_G=YAxis_G - (YAxis_G - (TotalGravity*math.sin(AngleOfGravity_Rad)))
                ZaxisForGravity_G=ZAxis_G - (ZAxis_G - (TotalGravity*math.cos(AngleOfGravity_Rad)))
            else:
                XAxisForGravity_G=self.Inertial.acceleration(AxisType.XAXIS)
                YAxisForGravity_G=self.Inertial.acceleration(AxisType.YAXIS)
                ZaxisForGravity_G=self.Inertial.acceleration(AxisType.ZAXIS)


            try:
                self.FilteredRoll_Rad=AxisWeight*(self.FilteredRoll_Rad + math.radians(self.Inertial.gyro_rate(YAXIS) * self.TimeStep_Sec)) + (1-AxisWeight)*((math.atan(YAxisForGravity_G/ZaxisForGravity_G))-self.XgravityOffsetDegrees)
            except ZeroDivisionError:
                self.FilteredRoll_Rad=AxisWeight*(self.FilteredRoll_Rad + math.radians(self.Inertial.gyro_rate(YAXIS) * self.TimeStep_Sec)) + (1-AxisWeight)*((math.atan(0))-self.XgravityOffsetDegrees)
            
            try:
                self.FilteredPitch_Rad=AxisWeight*(self.FilteredPitch_Rad + math.radians(self.Inertial.gyro_rate(XAXIS) * self.TimeStep_Sec)) + (1-AxisWeight)*((math.asin(XAxisForGravity_G/TotalGravity))-self.YgravityOffsetDegrees)
            except ZeroDivisionError:
                self.FilteredPitch_Rad=AxisWeight*(self.FilteredPitch_Rad + math.radians(self.Inertial.gyro_rate(XAXIS) * self.TimeStep_Sec)) + (1-AxisWeight)*((math.asin(0))-self.YgravityOffsetDegrees)
            
            CentrificalAcceleration_G=(self.Inertial.gyro_rate(ZAXIS)**2) * math.sqrt(self.XOffset_mm**2 + self.YOffset_mm**2)
            CentificalAngle_Rad=math.atan2(self.YOffset_mm, self.XOffset_mm)
            
            self.ZFiltered_G=(ZAxis_G * math.cos(self.FilteredRoll_Rad) * math.cos(self.FilteredPitch_Rad)) - (TotalGravity*math.cos(self.FilteredRoll_Rad) * math.cos(self.FilteredPitch_Rad))
            self.XFiltered_G=(XAxis_G * math.sin(self.FilteredPitch_Rad)  + self.ZFiltered_G * math.cos(self.FilteredPitch_Rad)) - (TotalGravity*math.sin(self.FilteredPitch_Rad)) - (CentrificalAcceleration_G * math.cos(CentificalAngle_Rad))
            self.YFiltered_G=(YAxis_G * math.cos(self.FilteredRoll_Rad) - self.ZFiltered_G * math.sin(self.FilteredRoll_Rad)) - (TotalGravity*math.sin(self.FilteredRoll_Rad)) - (CentrificalAcceleration_G * math.sin(CentificalAngle_Rad))

            self.FilteredYaw_Rad=math.radians(self.Inertial.orientation(YAW))

            print("X: ", self.XFiltered_G, "Y: ", self.YFiltered_G, "Z: ", self.ZFiltered_G, "Yaw: ", self.FilteredYaw_Rad, "Pitch: ", self.FilteredPitch_Rad, "Roll: ", self.FilteredRoll_Rad)
            
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