from vex import *

class odometry:
    def __init__(self):
        self.XOdomSensor: Rotation
        self.YOdomSensor: Rotation
        self.XPosition: float = 0
        self.YPosition: float = 0
        self.Heading: float = 0
        self.Inertial: Inertial
        self.InertialSensorUse=False
        self.XSpeed: float = 0
        self.YSpeed: float = 0
    
    @staticmethod
    def _Run() -> None:
        GlobalScope = dir()
        for item in GlobalScope:      
            try:
                item_type=str(type(eval(item)))
            except NameError:
                continue
            if item_type == "<class 'inertial'>":
                OD.InertialSensorUse=True
                OD.Inertial = eval(item)
                break
        
        OD.XSpeed=(OD.Inertial.acceleration(AxisType.XAXIS) * 9.81) * 0.01
        OD.YSpeed=(OD.Inertial.acceleration(AxisType.YAXIS) * 9.81) * 0.01

        while True:
            OD.XSpeed+=(OD.Inertial.acceleration(AxisType.XAXIS) * 9.81) * 0.01
            OD.YSpeed+=(OD.Inertial.acceleration(AxisType.YAXIS) * 9.81) * 0.01
            OD.Heading=OD.Inertial.heading()
            
            wait(10, MSEC)

    def StartOD(self, XOdomSensor: Rotation, YOdomSensor: Rotation) -> Thread:
        self.XOdomSensor = XOdomSensor
        self.YOdomSensor = YOdomSensor
        self.ODT=Thread(OD._Run)

        return self.ODT
    
    def StopOD(self):
        self.ODT.stop()

OD=odometry()