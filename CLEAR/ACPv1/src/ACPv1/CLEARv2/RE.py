from vex import *

brain = Brain()

from .CLA import log as cla
import ustruct

class DigitalOutWrap():
    def __init__(self, DigitalOutObj: DigitalOut) -> None:
        self.state: bool = False
        self.obj: DigitalOut = DigitalOutObj
        DigitalOutObj.set(False)

    def set(self, state: bool) -> None:
        self.obj.set(state)
        self.state = state

class Recording():
    def __init__(self):
        self.StateHistory = {}
        self.FILEPATH = "/ACPv1/Data"
        self.MainDict = {}
        self.VariablesAdded = []
        self.ConstVars = []
        self.Motors = []
        self.Inertials = []
        self.Opticals = []
        self.Controllers: List[str]=[]
        self.Rotations = []
        self.Distances = []
        self.Bumpers = []
        self.Limits = []
        self.DigitalOuts: list[str] = []
        self.Potsv2s = []
        self.Analogs = []
        self.Timer = Timer()

    def _auto_config(self, MainDict: dict):
        
        
        scope=MainDict.values()

        def Void(obj):
            pass

        def set_var(name: str):
            if name.isupper() or (name.endswith("__") and name.startswith("__")):  # Check if variable is all caps and is a dunder variable eg. "TOTAL" or "__path__" repectively.
                self.ConstVars.append(name)
                return
            self.VariablesAdded.append(name)

        def set_motor(name: str):
            self.Motors+=[name]

        def set_controller(name: str):
            self.Controllers+=[name]

        def set_inertial(name: str):
            self.Inertials.append(name)

        def set_optical(name: str):
            self.Opticals.append(name)

        def set_rotation(name: str):
            self.Rotations.append(name)

        def set_distance(name: str):
            self.Distances.append(name)
            

        def set_bumper(name: str):
            self.Bumpers.append(name)

        def set_limit(name: str):
            self.Limits.append(name)

        def set_digital_out(name: str):
            self.DigitalOuts.append(name)

        def set_potv2(name: str):
            self.Potsv2s.append(name)

        def set_analog(name: str):
            self.Analogs.append(name)

        ConfigDict = {
            "<class 'int'>" : set_var,
            "<class 'bool'>" : set_var,
            "<class 'float'>" : set_var,
            
            # Disabled for speed.
            # "<class 'str'>" : set_var,
            # "<class 'list'>" : set_var,
            # "<class 'dict'>" : set_var,
            # "<class 'tuple'>" : set_var,

            "<class 'motor'>" : set_motor,
            "<class 'controller'>" : set_controller,
            "<class 'inertial'>" : set_inertial,
            "<class 'optical'>" : set_optical,
            "<class 'rotation'>" : set_rotation,
            "<class 'distance'>" : set_distance,
            "<class 'triport_bumper'>" : set_bumper,
            "<class 'triport_limit'>" : set_limit,
            "<class 'triport_digitalout'>" : set_digital_out,
            "<class 'triport_potv2'>" : set_potv2,
            "<class 'triport_analog'>" : set_analog,
        }

        for item in scope:
            ConfigDict.get(str(type(item)), Void) (item)

        for digital in self.DigitalOuts:
            MainDict[digital] = DigitalOutWrap(eval(digital, MainDict))

    def _loop(self, MainDict: dict, name:str):
        Run = True

        FULLFILEPATH = "%s/%s.clear.record"%(self.FILEPATH, name)

        brain.sdcard.savefile(FULLFILEPATH)

        self.BufferLen = 34000
        self.Buffer = bytearray(self.BufferLen)  # 34 KB buffer
        self.BufferOffset = 0

        # Intended format for the file per line is "<name> data( seperated by ',' ) / formatstr;" eg. "<Right> 332.7, 217 / Pos [DEGREES], Vel [RPM]"

        def Void(name, item):
            pass

        def motor_handle(name: str, item: Motor) -> None:
            entry = b"<%s> %.4f, %.1f / Pos [DEGREES], Vel [RPM]; \n"%(name, item.position(DEGREES), item.velocity(RPM))
            ustruct.pack_into("=%ds"%(len(entry)), self.Buffer, self.BufferOffset, entry)
            self.BufferOffset+=len(entry)

        def int_handle(name: str, item) -> None:
            entry = b"<%s> %d / Val[int]; \n"%(name, item)
            ustruct.pack_into("=%ds"%(len(entry)), self.Buffer, self.BufferOffset, entry)
            self.BufferOffset+=len(entry)

        def float_handle(name: str, item) -> None:
            entry = b"<%s> %.5f / Val[float]; \n"%(name, item)
            ustruct.pack_into("=%ds"%(len(entry)), self.Buffer, self.BufferOffset, entry)
            self.BufferOffset+=len(entry)

        def bool_handle(name: str, item) -> None:
            entry = b"<%s> %s / Val[bool]; \n"%(name, item)
            ustruct.pack_into("=%ds"%(len(entry)), self.Buffer, self.BufferOffset, entry)
            self.BufferOffset+=len(entry)

        TypeDict = {
            Motor : motor_handle,
            int : int_handle,
            float : float_handle,
            bool : bool_handle
        }
        
        while Run:
            StartTime = self.Timer.time()

            for key, item in MainDict.items():
                TypeDict.get(type(item), Void) (key, item)

            if self.BufferOffset >= self.BufferLen - 500:
                brain.sdcard.appendfile(FULLFILEPATH, bytearray(self.Buffer[: self.BufferOffset]))
                self.BufferOffset = 0

            TimePast = self.Timer.time() - StartTime
            wait(10 - TimePast, MSEC)

    def start(self, MainDict, name):
        Thread(lambda: self._loop(MainDict, name))
