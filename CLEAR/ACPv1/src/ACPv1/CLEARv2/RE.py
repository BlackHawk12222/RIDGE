from vex import *

brain = Brain()

from .CLA import log as cla


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
        self.StateHistory = {}          # name -> last value written (change-only events)
        self.FILEPATH = "/ACPv1/Data"   # folder must already exist on the SD card
        self.MainDict = {}
        self.VariablesAdded = []
        self.Motors = []                # list of (b"<name> ", MotorObj), resolved once
        self.Inertials = []
        self.Opticals = []
        self.Controllers = []
        self.Rotations = []
        self.Distances = []
        self.Bumpers = []
        self.Limits = []
        self.DigitalOuts = []           # names; values are DigitalOutWrap after config
        self.Potsv2s = []
        self.Analogs = []
        self.Timer = Timer()
        self.Run = False
        self.Headroom = 500

    def _auto_config(self, MainDict: dict):
        self.MainDict = MainDict

        def Void(name, obj):
            pass

        def set_var(name, obj):
            self.VariablesAdded.append(name)

        def set_motor(name, obj):
            self.Motors.append((b"<" + name.encode() + b"> ", obj))

        def set_controller(name, obj):
            self.Controllers.append(name)

        def set_inertial(name, obj):
            self.Inertials.append(name)

        def set_optical(name, obj):
            self.Opticals.append(name)

        def set_rotation(name, obj):
            self.Rotations.append(name)

        def set_distance(name, obj):
            self.Distances.append(name)

        def set_bumper(name, obj):
            self.Bumpers.append(name)

        def set_limit(name, obj):
            self.Limits.append(name)

        def set_digital_out(name, obj):
            self.DigitalOuts.append(name)

        def set_potv2(name, obj):
            self.Potsv2s.append(name)

        def set_analog(name, obj):
            self.Analogs.append(name)

        ConfigDict = {
            "<class 'int'>": set_var,
            "<class 'bool'>": set_var,
            "<class 'float'>": set_var,

            # Disabled for speed.
            # "<class 'str'>" : set_var,
            # "<class 'list'>" : set_var,
            # "<class 'dict'>" : set_var,
            # "<class 'tuple'>" : set_var,

            "<class 'motor'>": set_motor,
            "<class 'controller'>": set_controller,
            "<class 'inertial'>": set_inertial,
            "<class 'optical'>": set_optical,
            "<class 'rotation'>": set_rotation,
            "<class 'distance'>": set_distance,
            "<class 'triport_bumper'>": set_bumper,
            "<class 'triport_limit'>": set_limit,
            "<class 'triport_digitalout'>": set_digital_out,
            "<class 'DigitalOutWrap'>": set_digital_out,   # already wrapped (config run twice)
            "<class 'triport_potv2'>": set_potv2,
            "<class 'triport_analog'>": set_analog,
        }

        # list() so we can replace entries in MainDict while looping
        for name, obj in list(MainDict.items()):
            # Skip constants ("TOTAL") and dunders ("__name__")
            if name.isupper() or (name.startswith("__") and name.endswith("__")):
                continue
            ConfigDict.get(str(type(obj)), Void)(name, obj)

        for name in self.DigitalOuts:
            obj = MainDict[name]
            if not isinstance(obj, DigitalOutWrap):
                MainDict[name] = DigitalOutWrap(obj)
            else:
                obj.set(False)  # known state at record start

        # Worst-case bytes one frame can add, so the buffer never overflows
        self.Headroom = 32 + 64 * len(self.Motors) + 64 * (len(self.VariablesAdded) + len(self.DigitalOuts))

    # Line formats:
    #   "T <ms>;"                                      frame start
    #   "<Right1> 332.7000, 217.0 / Pos [DEGREES], Vel [RPM];"   every frame
    #   "<speed> 12 / Val[int];"                       only when it changes
    #   "<grabber> 1 / Val[piston];"                   only when it changes
    def _loop(self, name: str):
        MainDict = self.MainDict
        FULLFILEPATH = "%s/%s.clear.record" % (self.FILEPATH, name)

        brain.sdcard.savefile(FULLFILEPATH, bytearray(0))  # create/empty the file

        BufferLen = 34000
        Buffer = bytearray(BufferLen)  # 34 KB buffer
        Offset = 0
        Headroom = self.Headroom
        History = self.StateHistory
        History.clear()

        # Pre-encode names once so the loop never formats a str into bytes
        Vars = [(n, b"<" + n.encode() + b"> ") for n in self.VariablesAdded]
        Pistons = [(n, b"<" + n.encode() + b"> ", MainDict[n]) for n in self.DigitalOuts]
        Motors = self.Motors

        def write(entry, Offset):
            n = len(entry)
            Buffer[Offset:Offset + n] = entry
            return Offset + n

        NextTime = self.Timer.time()
        while self.Run:
            Offset = write(b"T %d;\n" % self.Timer.time(), Offset)

            for prefix, motor in Motors:
                Offset = write(prefix + b"%.4f, %.1f / Pos [DEGREES], Vel [RPM];\n"
                               % (motor.position(DEGREES), motor.velocity(RPM)), Offset)

            for n, prefix in Vars:
                val = MainDict[n]
                if History.get(n) != val:
                    History[n] = val
                    if type(val) is float:
                        Offset = write(prefix + b"%.5f / Val[float];\n" % val, Offset)
                    elif type(val) is bool:
                        Offset = write(prefix + b"%d / Val[bool];\n" % val, Offset)
                    else:
                        Offset = write(prefix + b"%d / Val[int];\n" % val, Offset)

            for n, prefix, wrap in Pistons:
                val = wrap.state
                if History.get(n) != val:
                    History[n] = val
                    Offset = write(prefix + b"%d / Val[piston];\n" % val, Offset)

            if Offset >= BufferLen - Headroom:
                brain.sdcard.appendfile(FULLFILEPATH, Buffer[:Offset])
                Offset = 0

            NextTime += 10
            delay = NextTime - self.Timer.time()
            if delay > 0:
                wait(delay, MSEC)
            else:
                NextTime = self.Timer.time()  # overran: resync instead of bursting

        if Offset:
            brain.sdcard.appendfile(FULLFILEPATH, Buffer[:Offset])  # final flush

    def start(self, MainDict, name):
        if self.Run:
            return
        self._auto_config(MainDict)
        self.Run = True
        Thread(lambda: self._loop(name))

    def stop(self):
        self.Run = False  # loop finishes its frame, flushes, and exits

class Encode():
    def __init__(self) -> None:
        pass

    def _loop(self, name: str) -> None:
        FULLFILEPATH = "ACPv1/Data/%s"%(name)
        FILEHEADER = b"""
        # This is a file generated using CLEAR.
        # Aton name: %s size: %03.2f.

        """

        brain.sdcard.savefile(FULLFILEPATH, bytearray(FILEHEADER))

        

    def run(self, name: str) -> None:
        Thread(lambda: self._loop(name))

recording = Recording()
encode = Encode()
