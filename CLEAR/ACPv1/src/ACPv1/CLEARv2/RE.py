"""CLEARv2 RE: Record + Encode.

Three classes, in order:
    Recording  saves what the robot does to ACPv1/Data/<name>.clear.record
    Encode     turns that recording into ACPv1/Data/<name>.py (main_script())
    Auton      runs that script: Auton.run_script("<name>", globals())

Records everything the robot does into ACPv1/Data/<name>.clear.record so
Encode (further down) can turn it into a readable auton script.

Usage in main.py:
    rec = Recording()
    rec.start(globals(), "RightAWP_001")   # starts recording in its own thread
    ...drive the robot...
    rec.stop()                             # waits until the file is flushed
    Encode("RightAWP_001").execute()       # safe right after stop()

File format (text, one item per line):
    # CLEAR-RECORD v2                      file header
    # PERIOD 10                            frame period in ms
    # DEVICE <kind> <name> <installed>     one per device found (kind: motor,
                                           inertial, rotation, piston, var)
    # START                                end of header
    T <ms>;                                start of a frame
    <Right1> 332.7000, 217.0 / Pos [DEGREES], Vel [RPM];   every frame
    <inertial> 12.3400 / Rot [DEGREES];                    every frame
    <Xodom> 123.4000 / Pos [DEGREES];                      every frame
    <speed> 12 / Val[int];                 only when it changes
    <grabber> 1 / Val[piston];             only when it changes
"""

from vex import *
import math

try:
    from array import array
except ImportError:
    from uarray import array

brain = Brain()

from .CLA import log as cla


# ---------------------------------------------------------------------------
# Piston wrapper: VEX DigitalOut cannot be read back, so this remembers the
# last value it was set to. Recording swaps every DigitalOut in main for one.
# ---------------------------------------------------------------------------
class DigitalOutWrap():
    def __init__(self, DigitalOutObj: DigitalOut) -> None:
        self.state: bool = False
        self.obj: DigitalOut = DigitalOutObj
        DigitalOutObj.set(False)

    def set(self, state: bool) -> None:
        self.obj.set(state)
        self.state = state


def _installed(obj) -> int:
    """1 if the device reports it is plugged in, 0 if not (1 if unknown)."""
    try:
        return 1 if obj.installed() else 0
    except Exception:
        return 1


class Recording():
    PERIOD_MS = 10

    def __init__(self):
        self.StateHistory = {}          # name -> last value written (change-only events)
        self.FILEPATH = "ACPv1/Data"    # folder already exists on the SD card
        self.MainDict = {}
        self.VariablesAdded = []
        self.Motors = []                # (b"<name> ", MotorObj), resolved once
        self.Inertials = []             # (b"<name> ", InertialObj)
        self.Rotations = []             # (b"<name> ", RotationObj)
        self.Opticals = []
        self.Controllers = []
        self.Distances = []
        self.Bumpers = []
        self.Limits = []
        self.DigitalOuts = []           # names; values are DigitalOutWrap after config
        self.Potsv2s = []
        self.Analogs = []
        self.Header = b""
        self.Timer = Timer()
        self.Run = False
        self.Busy = False               # True while a record loop is still writing
        self.Gen = 0                    # bumps every start(); an old loop sees it and exits
        self.Path = ""                  # file the newest start() records into
        self.Headroom = 500

    # -----------------------------------------------------------------------
    # Config: sort every global in main into the lists above, by type.
    # -----------------------------------------------------------------------
    def _auto_config(self, MainDict: dict):
        self.MainDict = MainDict
        # Fresh lists every time, so recording twice never lists a device twice
        for lst in (self.VariablesAdded, self.Motors, self.Inertials, self.Rotations,
                    self.Opticals, self.Controllers, self.Distances, self.Bumpers,
                    self.Limits, self.DigitalOuts, self.Potsv2s, self.Analogs):
            lst.clear()
        header = [b"# CLEAR-RECORD v2\n", b"# PERIOD %d\n" % self.PERIOD_MS]

        def tag(name):
            return b"<" + name.encode() + b"> "

        def Void(name, obj):
            pass

        def set_var(name, obj):
            self.VariablesAdded.append(name)
            header.append(b"# DEVICE var " + name.encode() + b" 1\n")

        def set_motor(name, obj):
            self.Motors.append((tag(name), obj))
            header.append(b"# DEVICE motor " + name.encode() + b" %d\n" % _installed(obj))

        def set_inertial(name, obj):
            self.Inertials.append((tag(name), obj))
            header.append(b"# DEVICE inertial " + name.encode() + b" %d\n" % _installed(obj))

        def set_rotation(name, obj):
            self.Rotations.append((tag(name), obj))
            header.append(b"# DEVICE rotation " + name.encode() + b" %d\n" % _installed(obj))

        def set_controller(name, obj):
            self.Controllers.append(name)

        def set_optical(name, obj):
            self.Opticals.append(name)

        def set_distance(name, obj):
            self.Distances.append(name)

        def set_bumper(name, obj):
            self.Bumpers.append(name)

        def set_limit(name, obj):
            self.Limits.append(name)

        def set_digital_out(name, obj):
            self.DigitalOuts.append(name)
            header.append(b"# DEVICE piston " + name.encode() + b" 1\n")

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
            "<class 'triport_potv2'>": set_potv2,
            "<class 'triport_analog'>": set_analog,
        }

        # list() so we can replace entries in MainDict while looping
        for name, obj in list(MainDict.items()):
            # Skip constants ("TOTAL") and dunders ("__name__")
            if name.isupper() or (name.startswith("__") and name.endswith("__")):
                continue
            if isinstance(obj, DigitalOutWrap):   # already wrapped (recording twice)
                set_digital_out(name, obj)
                continue
            ConfigDict.get(str(type(obj)), Void)(name, obj)

        for name in self.DigitalOuts:
            obj = MainDict[name]
            if not isinstance(obj, DigitalOutWrap):
                MainDict[name] = DigitalOutWrap(obj)
            else:
                obj.set(False)  # known state at record start

        header.append(b"# START\n")
        self.Header = b"".join(header)

        # Worst-case bytes one frame can add, so the buffer never overflows
        self.Headroom = 32 + 64 * (len(self.Motors) + len(self.Inertials) + len(self.Rotations)
                                   + len(self.VariablesAdded) + len(self.DigitalOuts))

    # -----------------------------------------------------------------------
    # Record loop: one frame every PERIOD_MS.
    # -----------------------------------------------------------------------
    def _loop(self, name: str, gen: int):
        self.Busy = True
        try:
            self._record(name, gen)
        finally:
            self.Busy = False

    def _record(self, name: str, gen: int):
        MainDict = self.MainDict
        FULLFILEPATH = "%s/%s.clear.record" % (self.FILEPATH, name)

        brain.sdcard.savefile(FULLFILEPATH, bytearray(self.Header))  # create file + header

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
        Inertials = self.Inertials
        Rotations = self.Rotations

        def write(entry, Offset):
            n = len(entry)
            Buffer[Offset:Offset + n] = entry
            return Offset + n

        Start = self.Timer.time()
        NextTime = Start
        while self.Run and self.Gen == gen:
            Offset = write(b"T %d;\n" % (self.Timer.time() - Start), Offset)

            # --- continuous: written every frame ---
            for prefix, motor in Motors:
                Offset = write(prefix + b"%.4f, %.1f / Pos [DEGREES], Vel [RPM];\n"
                               % (motor.position(DEGREES), motor.velocity(RPM)), Offset)

            for prefix, imu in Inertials:
                Offset = write(prefix + b"%.4f / Rot [DEGREES];\n" % imu.rotation(DEGREES), Offset)

            for prefix, rot in Rotations:
                Offset = write(prefix + b"%.4f / Pos [DEGREES];\n" % rot.position(DEGREES), Offset)

            # --- events: written only when they change ---
            for n, prefix in Vars:
                val = MainDict.get(n)
                if val is not None and History.get(n) != val:
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

            NextTime += self.PERIOD_MS
            delay = NextTime - self.Timer.time()
            if delay > 0:
                wait(delay, MSEC)
            else:
                NextTime = self.Timer.time()  # overran: resync instead of bursting

        # Final flush, unless a newer start() already reopened this same file
        if Offset and (self.Gen == gen or self.Path != FULLFILEPATH):
            brain.sdcard.appendfile(FULLFILEPATH, Buffer[:Offset])

    def start(self, MainDict, name):
        if self.Run:
            return
        self._wait_done(1000)           # a stop() that did not wait: let it flush first
        self._auto_config(MainDict)
        self.Gen += 1
        gen = self.Gen
        self.Path = "%s/%s.clear.record" % (self.FILEPATH, name)
        self.Run = True
        self.Busy = True                # set here so stop() right after start() still waits
        Thread(lambda: self._loop(name, gen))

    def stop(self, wait_ms: int = 1000):
        """Stop recording. Waits (up to wait_ms) until the last frame is
        flushed to the SD card, so Encode can run right after."""
        self.Run = False
        return self._wait_done(wait_ms)

    def _wait_done(self, wait_ms: int) -> bool:
        waited = 0
        while self.Busy and waited < wait_ms:
            wait(5, MSEC)
            waited += 5
        return not self.Busy

# ###########################################################################
# ENCODE: turns a recording into a readable auton script
# ###########################################################################
class Encode:
    """CLEARv2 Encode: turns a recording into a readable auton script.

        Encode("RightAWP_001").execute()

    reads   ACPv1/Data/RightAWP_001.clear.record   (made by Recording in RE.py)
    writes  ACPv1/Data/RightAWP_001.py             (run it with Auton.run_script)

    It is meant to run once, after the recording, so it trades speed for a
    better script. Steps (each prints progress to the console and brain screen):

        1. Load the recording
        2. Check which sensors were really plugged in (IMU, rotation sensors)
        3. Read the robot config (wheel size, gear ratio, drive sides)
        4. Group motors that move together
        5. Build the drivetrain: distance (odom or motors) and heading (IMU or wheels)
        6. Cut the drive path into moves: straight, arc, turn_to, hold
        7. Turn mechanism groups (intake, lift...) into move_to commands
        8. Attach pistons, variables and mechanism moves to the drive moves by
           position, so they fire at the same spot on the field
        9. Measure how far the script's path is from the recording (tolerance)
       10. Write the script safely: ACPv1/TMP/<name>.CLEAR.tmp first, then
           ACPv1/Data/<name>.py
    """

    # ===========================================================================
    # SETTINGS (edit to trade precision for fewer moves)
    # ===========================================================================
    DATA_DIR = "ACPv1/Data"
    TMP_DIR = "ACPv1/TMP"
    CONFIG_FILE = "ACPv1/Config/ACPv1config.txt"

    DIST_TOL_MM = 5.0        # a move may stray this far (mm) from the recorded path
    HEAD_TOL_DEG = 1.0       # ...and this far in heading (degrees)
    MECH_TOL_DEG = 5.0       # mechanism motors: allowed error in motor degrees

    GROUP_TOL_DEG = 10.0     # two motors are "together" if they never differ more
    GROUP_TOL_FRAC = 0.03    # ...than this many degrees, or this share of their travel
    MOVING_DEG = 5.0         # a motor that moved less than this is ignored

    SENSOR_ALIVE_DEG = 0.5   # a sensor that changed less than this was not plugged in
    ODOM_MIN_CORR = 0.95     # rotation sensor counts as odom above this correlation

    STILL_MMPS = 20.0        # slower than this (mm/s)...
    STILL_DEGPS = 5.0        # ...and turning slower than this (deg/s) is "stopped"
    MIN_HOLD_MS = 150        # stops shorter than this are not written as hold()

    DEFAULT_MM_PER_TURN_DEG = 2.6   # used only if the recording never turned
    WRITE_CHUNK = 2048       # bytes per SD write while saving
    ENCODER_VERSION = "CLEARv2 Encode v1"

    # ===========================================================================
    # Small helpers
    # ===========================================================================
    @staticmethod
    def _progress(name, pct, text):
        """Show progress on the console and the brain screen."""
        msg = "Encode %s: %3d%% %s" % (name, pct, text)
        print(msg)
        try:
            brain.screen.clear_row(1)
            brain.screen.set_cursor(1, 1)
            brain.screen.print(msg)
        except Exception:
            pass


    @staticmethod
    def _zeroed(values):
        """Copy of values with the first one subtracted (start = 0)."""
        out = array("f", values)
        if len(out):
            first = out[0]
            for i in range(len(out)):
                out[i] -= first
        return out


    @staticmethod
    def _span(values):
        return (max(values) - min(values)) if len(values) else 0.0


    @staticmethod
    def _common_prefix(names):
        prefix = names[0]
        for n in names[1:]:
            while not n.startswith(prefix):
                prefix = prefix[:-1]
        return prefix.rstrip("0123456789_")


    @staticmethod
    def _fmt(x, digits=1):
        """Number as text with fixed decimals, never "-0.0"."""
        s = ("%." + str(digits) + "f") % x
        return s[1:] if s.startswith("-") and s.strip("-0.") == "" else s

    # ===========================================================================
    # The recording, loaded into memory
    # ===========================================================================
    class _Recording:
        def __init__(self):
            self.times = array("l", [])    # ms since record start, one per frame
            self.kinds = {}            # name -> "motor" / "inertial" / "rotation" / "piston" / "var"
            self.installed = {}        # name -> True/False from the header
            self.channels = {}         # name -> array of positions/rotations, one per frame
            self.events = []           # (frame, name, kind, value_text)

        def frames(self):
            return len(self.times)


    # ===========================================================================
    # A drive move (one line of the finished script)
    # ===========================================================================
    class _Move:
        def __init__(self, kind, i, j):
            self.kind = kind           # "straight", "arc", "turn", "hold"
            self.i = i                 # first frame
            self.j = j                 # last frame
            self.speed = 0.0
            self.at = []               # (progress, code text)

    def __init__(self, name: str):
        self.name = name
        self.record_path = "%s/%s.clear.record" % (self.DATA_DIR, name)
        self.script_path = "%s/%s.py" % (self.DATA_DIR, name)
        self.tmp_path = "%s/%s.CLEAR.tmp" % (self.TMP_DIR, name)
        self.notes = []            # warnings written into the script header

    # =======================================================================
    # execute(): the whole job, start to finish
    # =======================================================================
    def execute(self) -> bool:
        p = lambda pct, text: self._progress(self.name, pct, text)
        self.notes = []            # fresh notes if execute() runs twice

        p(0, "loading recording")
        rec = self._load(p)
        if rec is None or rec.frames() < 2:
            p(100, "FAILED: recording missing or empty")
            return False

        p(30, "checking sensors")
        imu, rotations = self._live_sensors(rec)

        p(33, "reading robot config")
        cfg = self._read_config()

        p(36, "grouping motors")
        left, right, mech_groups = self._find_groups(rec, cfg)

        drive = None
        if left and right:
            p(42, "building drivetrain")
            drive = self._build_drive(rec, cfg, left, right, imu, rotations)

            p(50, "cutting path into moves")
            moves = self._cut_moves(rec, drive, p)
        else:
            self.notes.append("No drivetrain found (no LeftMotors/RightMotors in config, "
                              "no motor names with left/right). Script is timed only.")
            moves = []

        p(78, "planning mechanism moves")
        events = self._mechanism_events(rec, mech_groups)
        events += self._device_events(rec)

        p(84, "attaching events to moves")
        start_lines, tail_events = self._attach_events(rec, drive, moves, events)

        p(88, "measuring tolerance")
        tol = self._tolerance(rec, drive, moves) if drive else None

        p(90, "writing script")
        text = self._script_text(rec, drive, moves, mech_groups, start_lines,
                                 tail_events, tol)
        ok = self._atomic_write(text.encode("utf-8"), p)
        p(100, "done" if ok else "FAILED while saving")
        return ok

    # =======================================================================
    # 1. Load
    # =======================================================================
    def _load(self, p):
        if not brain.sdcard.exists(self.record_path):
            return None
        rec = self._Recording()
        total = max(brain.sdcard.filesize(self.record_path), 1)
        read = 0
        next_report = 5
        frame = -1
        bad = 0                    # lines that could not be read

        with open(self.record_path, "rb") as f:
            while True:
                raw = f.readline()
                if not raw:
                    break
                read += len(raw)
                line = raw.decode("utf-8").strip()
                if not line:
                    continue

                if line[0] == "#":                         # header
                    parts = line.split()
                    if len(parts) >= 4 and parts[1] == "DEVICE":
                        kind, name = parts[2], parts[3]
                        rec.kinds[name] = kind
                        rec.installed[name] = parts[4] == "1" if len(parts) > 4 else True
                        if kind in ("motor", "inertial", "rotation"):
                            rec.channels[name] = array("f", [])

                elif line[0] == "T":                       # new frame
                    try:
                        t = int(line[2:line.index(";")])
                    except ValueError:                     # cut short by a power loss
                        bad += 1
                        continue
                    frame += 1
                    rec.times.append(t)

                elif line[0] == "<" and frame >= 0:        # a value
                    try:
                        close = line.index(">")
                        name = line[1:close]
                        body = line[close + 2:line.index(" /")]
                        kind = rec.kinds.get(name)
                        if kind == "motor":
                            rec.channels[name].append(float(body.split(",")[0]))
                        elif kind in ("inertial", "rotation"):
                            rec.channels[name].append(float(body))
                        elif kind in ("piston", "var"):
                            tag = line[line.index("[", line.index(" /")) + 1:line.rindex("]")]
                            rec.events.append((frame, name, tag, body.strip()))
                    except (ValueError, IndexError):       # half-written line
                        bad += 1

                pct = read * 100 // total
                if pct >= next_report:
                    p(pct * 30 // 100, "loading recording (%d%%)" % pct)
                    next_report = pct + 5

        # A power cut can leave the last frame half written: trim to the
        # shortest channel so every frame has every value.
        n = len(rec.times)
        for ch in rec.channels.values():
            n = min(n, len(ch))
        if n < len(rec.times):
            self.notes.append("Last %d frame(s) were cut short (power off while "
                              "recording?) and were dropped." % (len(rec.times) - n))
            rec.times = rec.times[:n]
            for name in rec.channels:
                rec.channels[name] = rec.channels[name][:n]
        rec.events = [e for e in rec.events if e[0] < n]
        if bad:
            self.notes.append("%d damaged line(s) in the recording were skipped." % bad)
        return rec

    # =======================================================================
    # 2. Which sensors were really there
    # =======================================================================
    def _live_sensors(self, rec):
        imu = None
        rotations = []
        for name, kind in rec.kinds.items():
            if kind not in ("inertial", "rotation"):
                continue
            alive = rec.installed.get(name, True) and self._span(rec.channels[name]) > self.SENSOR_ALIVE_DEG
            if not alive:
                self.notes.append('%s "%s" never changed: treated as unplugged.' % (kind, name))
                continue
            if kind == "inertial" and imu is None:
                imu = name
            elif kind == "rotation":
                rotations.append(name)
        return imu, rotations

    # =======================================================================
    # 3. Robot config written by ACPv1.start()
    # =======================================================================
    def _read_config(self):
        cfg = {"GearRatio": 1.0, "Wheelsize_MM": 101.6, "OdomWheelSize_MM": 50.8,
               "LeftMotors": [], "RightMotors": []}
        found = False
        try:
            if brain.sdcard.exists(self.CONFIG_FILE):
                text = brain.sdcard.loadfile(self.CONFIG_FILE).decode("utf-8")
                for line in text.split("\n"):
                    if ":" not in line:
                        continue
                    key, value = line.split(":", 1)
                    key, value = key.strip(), value.strip()
                    if key in ("GearRatio", "Wheelsize_MM", "OdomWheelSize_MM"):
                        cfg[key] = float(value)
                        found = True
                    elif key in ("LeftMotors", "RightMotors"):
                        names = value.strip("[]").replace("'", "").replace('"', "")
                        cfg[key] = [s.strip() for s in names.split(",") if s.strip()]
        except Exception:
            pass
        if not found:
            self.notes.append("Robot config not found: used gear 1.0, 4 in wheels. "
                              "Check mm_per_motor_deg.")
        return cfg

    # =======================================================================
    # 4. Group motors that move together
    # =======================================================================
    def _group(self, rec, names):
        """Greedy grouping. Returns [(names, flips)], flips True = mirrored."""
        zero = {n: self._zeroed(rec.channels[n]) for n in names}
        groups = []
        for n in names:
            placed = False
            for g in groups:
                ref = zero[g[0][0]]
                cur = zero[n]
                tol = max(self.GROUP_TOL_DEG, self.GROUP_TOL_FRAC * max(self._span(ref), self._span(cur)))
                same = max(abs(a - b) for a, b in zip(ref, cur))
                mirror = max(abs(a + b) for a, b in zip(ref, cur))
                if same <= tol or mirror <= tol:
                    g[0].append(n)
                    g[1].append(same > tol)
                    placed = True
                    break
            if not placed:
                groups.append(([n], [False]))
        return groups

    def _find_groups(self, rec, cfg):
        motors = [n for n, k in rec.kinds.items() if k == "motor" and rec.installed.get(n, True)]
        motors.sort()

        # Drive sides: config first, then names containing left/right
        left = [n for n in cfg["LeftMotors"] if n in motors]
        right = [n for n in cfg["RightMotors"] if n in motors]
        if not left or not right:
            left = [n for n in motors if "left" in n.lower()]
            right = [n for n in motors if "right" in n.lower()]

        others = [n for n in motors if n not in left and n not in right
                  and self._span(rec.channels[n]) > self.MOVING_DEG]

        left_side = self._side(rec, left)
        right_side = self._side(rec, right)

        mech = []
        taken = set(rec.kinds.keys()) | {"drive", "drive_left", "drive_right"}
        for names, flips in self._group(rec, others):
            gname = self._common_prefix(names) if len(names) > 1 else names[0]
            if not gname or gname in taken or not (gname[0].isalpha() or gname[0] == "_"):
                gname = (gname or "group") + "_group"
            while gname in taken:
                gname += "_"
            taken.add(gname)
            channel = self._group_channel(rec, names, flips)
            mech.append({"name": gname, "motors": names, "flips": flips, "pos": channel})
        return left_side, right_side, mech

    def _side(self, rec, names):
        if not names:
            return None
        groups = self._group(rec, names)
        if len(groups) > 1:
            self.notes.append("Drive side %s did not move as one; check for a "
                              "slipping or unplugged motor." % ", ".join(names))
        # keep every listed motor on the side; flips from the first motor
        flips = []
        ref = self._zeroed(rec.channels[names[0]])
        for n in names:
            cur = self._zeroed(rec.channels[n])
            same = max(abs(a - b) for a, b in zip(ref, cur))
            mirror = max(abs(a + b) for a, b in zip(ref, cur))
            flips.append(mirror < same)
        return {"motors": names, "flips": flips, "pos": self._group_channel(rec, names, flips)}

    def _group_channel(self, rec, names, flips):
        """Average zeroed position of a group, mirrored motors flipped."""
        n = rec.frames()
        out = array("f", [0.0] * n)
        for name, flip in zip(names, flips):
            z = self._zeroed(rec.channels[name])
            s = -1.0 if flip else 1.0
            for i in range(n):
                out[i] += s * z[i]
        k = 1.0 / len(names)
        for i in range(n):
            out[i] *= k
        return out

    # =======================================================================
    # 5. Drivetrain channels: F = distance (mm), H = heading (deg)
    # =======================================================================
    def _build_drive(self, rec, cfg, left, right, imu, rotations):
        n = rec.frames()
        mm_per_motor_deg = cfg["GearRatio"] * math.pi * cfg["Wheelsize_MM"] / 360.0
        L, R = left["pos"], right["pos"]
        f_motor = array("f", [(L[i] + R[i]) / 2 * mm_per_motor_deg for i in range(n)])
        d_motor = array("f", [(L[i] - R[i]) / 2 * mm_per_motor_deg for i in range(n)])

        drive = {"left": left, "right": right, "mm_per_motor_deg": mm_per_motor_deg,
                 "imu": imu, "odom": None, "mm_per_odom_deg": 0.0, "odom_r": 0.0}

        # --- odom: the rotation sensor that tracks the drive best ---
        best, best_r, best_cov = None, 0.0, 0.0
        for name in rotations:
            r, cov = self._correlation(self._zeroed(rec.channels[name]), f_motor)
            if abs(r) > abs(best_r):
                best, best_r, best_cov = name, r, cov
        if best is not None and abs(best_r) >= self.ODOM_MIN_CORR:
            sign = 1.0 if best_cov > 0 else -1.0
            drive["odom"] = best
            drive["odom_r"] = best_r
            drive["mm_per_odom_deg"] = sign * math.pi * cfg["OdomWheelSize_MM"] / 360.0
            odom = self._zeroed(rec.channels[best])
            F = array("f", [v * drive["mm_per_odom_deg"] for v in odom])
            ratio = self._span(F) / max(self._span(f_motor), 1e-6)
            if self._span(f_motor) > 50 and not (0.85 < ratio < 1.15):
                self.notes.append("Odom and motors disagree by %d%%: check wheel sizes "
                                  "or slip." % round((ratio - 1) * 100))
        else:
            F = f_motor
            for name in rotations:
                self.notes.append('rotation "%s" does not follow the drive: not used as odom.' % name)

        # --- heading, and wheel travel per degree of turn ---
        if imu is not None:
            H = self._zeroed(rec.channels[imu])
            num = den = 0.0
            for i in range(1, n):
                dh = H[i] - H[i - 1]
                num += (d_motor[i] - d_motor[i - 1]) * dh
                den += dh * dh
            if den > 25.0:
                drive["mm_per_turn_deg"] = num / den
            else:
                drive["mm_per_turn_deg"] = self.DEFAULT_MM_PER_TURN_DEG
                self.notes.append("Robot barely turned: mm_per_turn_deg is a guess.")
        else:
            drive["mm_per_turn_deg"] = self.DEFAULT_MM_PER_TURN_DEG
            H = array("f", [v / self.DEFAULT_MM_PER_TURN_DEG for v in d_motor])
            self.notes.append("No IMU: heading comes from the wheels (less precise).")

        drive["F"] = F
        drive["H"] = H
        return drive

    @staticmethod
    def _correlation(a, b):
        """Correlation of the frame-to-frame changes of a and b."""
        n = min(len(a), len(b))
        sa = sb = saa = sbb = sab = 0.0
        m = 0
        for i in range(1, n):
            x = a[i] - a[i - 1]
            y = b[i] - b[i - 1]
            sa += x; sb += y; saa += x * x; sbb += y * y; sab += x * y
            m += 1
        if m < 2:
            return 0.0, 0.0
        cov = sab - sa * sb / m
        va = saa - sa * sa / m
        vb = sbb - sb * sb / m
        if va <= 1e-9 or vb <= 1e-9:
            return 0.0, cov
        return cov / math.sqrt(va * vb), cov

    # =======================================================================
    # 6. Cut the drive path into moves
    # =======================================================================
    def _cut_moves(self, rec, drive, p):
        F, H, T = drive["F"], drive["H"], rec.times
        n = rec.frames()

        # --- find stops (robot still for at least self.MIN_HOLD_MS) ---
        still = bytearray(n)
        w = 3
        for k in range(n):
            a, b = max(0, k - w), min(n - 1, k + w)
            dt = max(T[b] - T[a], 1) / 1000.0
            if abs(F[b] - F[a]) / dt < self.STILL_MMPS and abs(H[b] - H[a]) / dt < self.STILL_DEGPS:
                still[k] = 1

        pieces = []                # (kind, i, j): "hold" or "path"
        k = 0
        while k < n:
            j = k
            while j < n - 1 and still[j + 1] == still[k]:
                j += 1
            if still[k] and T[j] - T[k] >= self.MIN_HOLD_MS:
                pieces.append(("hold", k, j))
            elif pieces and pieces[-1][0] == "path":
                pieces[-1] = ("path", pieces[-1][1], j)
            else:
                pieces.append(("path", k, j))
            k = j + 1
        # make pieces touch end to end
        for idx in range(1, len(pieces)):
            kind, i, j = pieces[idx]
            pieces[idx] = (kind, pieces[idx - 1][2], j)

        # sitting still before the first move or after the last is not written
        while pieces and pieces[0][0] == "hold":
            pieces.pop(0)
        while pieces and pieces[-1][0] == "hold":
            pieces.pop()

        # --- split each path piece until every move fits the tolerance ---
        moves = []
        done = 0
        for kind, i, j in pieces:
            if kind == "hold":
                moves.append(self._Move("hold", i, j))
            else:
                moves += self._split(F, H, i, j)
            done += 1
            p(50 + 20 * done // max(len(pieces), 1), "cutting path into moves (%d)" % len(moves))

        # --- merge neighbours when the merged move still fits ---
        moves = self._merge(F, H, moves)
        p(74, "cut into %d moves" % len(moves))

        # --- speeds: the fastest the recording went during each move ---
        for m in moves:
            m.speed = self._peak_speed(F if m.kind != "turn" else H, T, m.i, m.j)
            if m.kind == "turn":
                m.speed = max(m.speed, 30.0)
            elif m.kind != "hold":
                m.speed = max(m.speed, 100.0)
        return moves

    def _classify(self, F, H, i, j):
        dF = abs(F[j] - F[i])
        dH = abs(H[j] - H[i])
        if dF < self.DIST_TOL_MM and dH < self.HEAD_TOL_DEG:
            return "straight" if dF >= dH * self.DIST_TOL_MM / self.HEAD_TOL_DEG else "turn"
        if dH < self.HEAD_TOL_DEG:
            return "straight"
        if dF < self.DIST_TOL_MM:
            return "turn"
        return "arc"

    def _errors(self, kind, F, H, i, j):
        """Worst error of a move against the recording, and where it happens.
        Returns (normalized error, frame, mm error, deg error); 1.0 = at the
        tolerance limit."""
        worst, at, w_mm, w_deg = 0.0, i, 0.0, 0.0
        F0, F1, H0, H1 = F[i], F[j], H[i], H[j]
        dF = F1 - F0
        sF = 1.0 if dF >= 0 else -1.0
        sH = 1.0 if H1 >= H0 else -1.0
        run_f = F0                 # furthest point reached (to catch backing up)
        run_h = H0
        for k in range(i, j + 1):
            f, h = F[k], H[k]
            if kind == "hold":
                e_mm, e_deg = abs(f - F0), abs(h - H0)
            elif kind == "turn":
                if sH * (h - run_h) > 0:
                    run_h = h
                e_mm = abs(f - F0)
                e_deg = max(sH * (run_h - h), sH * (h - H1), 0.0)
            else:
                if sF * (f - run_f) > 0:
                    run_f = f
                back = sF * (run_f - f)                     # backed up
                over = max(sF * (f - F1), 0.0)               # went past the end
                e_mm = max(back, over)
                if kind == "straight":
                    e_deg = abs(h - H1)
                else:
                    frac = (f - F0) / dF if dF else 1.0
                    frac = 0.0 if frac < 0 else 1.0 if frac > 1 else frac
                    e_deg = abs(h - (H0 + (H1 - H0) * frac))
            e = max(e_mm / self.DIST_TOL_MM, e_deg / self.HEAD_TOL_DEG)
            if e > worst:
                worst, at, w_mm, w_deg = e, k, e_mm, e_deg
        return worst, at, w_mm, w_deg

    def _split(self, F, H, i, j):
        """Split frames i..j into moves that each fit the tolerance."""
        out = []
        stack = [(i, j)]
        while stack:
            a, b = stack.pop()
            kind = self._classify(F, H, a, b)
            err, at, _, _ = self._errors(kind, F, H, a, b)
            if err <= 1.0 or b - a < 2 or at in (a, b):
                out.append(self._Move(kind, a, b))
            else:
                stack.append((at, b))   # popped second -> keeps order
                stack.append((a, at))
        return out

    def _merge(self, F, H, moves):
        changed = True
        while changed:
            changed = False
            out = []
            for m in moves:
                if out and m.kind != "hold" and out[-1].kind != "hold":
                    prev = out[-1]
                    kind = self._classify(F, H, prev.i, m.j)
                    if self._errors(kind, F, H, prev.i, m.j)[0] <= 1.0:
                        out[-1] = self._Move(kind, prev.i, m.j)
                        changed = True
                        continue
                if out and m.kind == "hold" and out[-1].kind == "hold":
                    out[-1] = self._Move("hold", out[-1].i, m.j)
                    changed = True
                    continue
                out.append(m)
            moves = out
        return moves

    @staticmethod
    def _peak_speed(C, T, i, j):
        w = 5
        peak = 0.0
        for k in range(i, max(i + 1, j - w + 1)):
            b = min(k + w, j)
            dt = (T[b] - T[k]) / 1000.0
            if dt > 0:
                peak = max(peak, abs(C[b] - C[k]) / dt)
        return peak

    # =======================================================================
    # 7. Mechanism groups -> move_to events
    # =======================================================================
    def _mechanism_events(self, rec, groups):
        """(frame, code) for each mechanism keyframe."""
        events = []
        T = rec.times
        for g in groups:
            pos = g["pos"]
            keys = self._time_keyframes(pos, 0, len(pos) - 1, self.MECH_TOL_DEG)
            for a, b in zip(keys, keys[1:]):
                delta = pos[b] - pos[a]
                if abs(delta) < self.MECH_TOL_DEG:
                    continue
                dt = max(T[b] - T[a], 1)
                rpm = abs(delta) / dt * 1000.0 / 6.0
                events.append((a, "%s.move_to(%s, rpm=%d)" % (g["name"], self._fmt(pos[b]), max(5, round(rpm)))))
        return events

    @staticmethod
    def _time_keyframes(C, i, j, tol):
        """Ramer-Douglas-Peucker over time: frames where C changes slope."""
        keep = {i, j}
        stack = [(i, j)]
        while stack:
            a, b = stack.pop()
            worst, at = 0.0, -1
            for k in range(a + 1, b):
                line = C[a] + (C[b] - C[a]) * (k - a) / (b - a)
                e = abs(C[k] - line)
                if e > worst:
                    worst, at = e, k
            if worst > tol:
                keep.add(at)
                stack.append((a, at))
                stack.append((at, b))
        return sorted(keep)

    def _device_events(self, rec):
        """(frame, code) for pistons and variables."""
        events = []
        for frame, name, tag, value in rec.events:
            if tag in ("piston", "bool"):
                val = "True" if value not in ("0", "False") else "False"
            else:
                val = value
            if tag == "piston":
                events.append((frame, "%s.set(%s)" % (name, val)))
            else:
                events.append((frame, 'set_global("%s", %s)' % (name, val)))
        return events

    # =======================================================================
    # 8. Attach events to the move that was running, by position
    # =======================================================================
    def _attach_events(self, rec, drive, moves, events):
        events.sort(key=lambda e: e[0])
        start_lines, tail = [], []
        T = rec.times
        for frame, code in events:
            if not moves:
                tail.append((T[frame], code))     # no drive: timed list
                continue
            if frame <= moves[0].i:
                start_lines.append(code)
                continue
            if frame > moves[-1].j:
                tail.append((T[frame], code))
                continue
            for m in moves:
                if m.i <= frame <= m.j:
                    if m.kind == "hold":
                        prog = T[frame] - T[m.i]
                    elif m.kind == "turn":
                        prog = abs(drive["H"][frame] - drive["H"][m.i])
                    else:
                        prog = abs(drive["F"][frame] - drive["F"][m.i])
                    m.at.append((prog, code))
                    break
        return start_lines, tail

    # =======================================================================
    # 9. Tolerance: how far the script's path is from the recording
    # =======================================================================
    def _tolerance(self, rec, drive, moves):
        F, H = drive["F"], drive["H"]
        worst_mm = worst_deg = sum_mm = sum_deg = 0.0
        count = 0
        for m in moves:
            errs = self._frame_errors(m.kind, F, H, m.i, m.j)
            for e_mm, e_deg in errs:
                worst_mm = max(worst_mm, e_mm)
                worst_deg = max(worst_deg, e_deg)
                sum_mm += e_mm
                sum_deg += e_deg
                count += 1
        count = max(count, 1)
        return worst_mm, worst_deg, sum_mm / count, sum_deg / count

    @staticmethod
    def _frame_errors(kind, F, H, i, j):
        """Same rules as _errors, but every frame's (mm, deg) error."""
        out = []
        F0, F1, H0, H1 = F[i], F[j], H[i], H[j]
        dF = F1 - F0
        sF = 1.0 if dF >= 0 else -1.0
        sH = 1.0 if H1 >= H0 else -1.0
        run_f, run_h = F0, H0
        for k in range(i, j + 1):
            f, h = F[k], H[k]
            if kind == "hold":
                out.append((abs(f - F0), abs(h - H0)))
            elif kind == "turn":
                if sH * (h - run_h) > 0:
                    run_h = h
                out.append((abs(f - F0), max(sH * (run_h - h), sH * (h - H1), 0.0)))
            else:
                if sF * (f - run_f) > 0:
                    run_f = f
                e_mm = max(sF * (run_f - f), sF * (f - F1), 0.0)
                if kind == "straight":
                    e_deg = abs(h - H1)
                else:
                    frac = (f - F0) / dF if dF else 1.0
                    frac = 0.0 if frac < 0 else 1.0 if frac > 1 else frac
                    e_deg = abs(h - (H0 + (H1 - H0) * frac))
                out.append((e_mm, e_deg))
        return out

    # =======================================================================
    # 10a. Build the script text
    # =======================================================================
    SIZE_FIELD = "0000000000"

    def _script_text(self, rec, drive, moves, mech_groups, start_lines, tail, tol):
        T = rec.times
        L = []
        add = L.append
        bar = "# " + "=" * 74
        sub = "# " + "-" * 74

        # ----- header -----
        add(bar)
        add("#  CLEAR AUTON SCRIPT")
        add("#  Name:             %s" % self.name)
        add("#  File size:        %s bytes" % self.SIZE_FIELD)
        if tol:
            add("#  Worst tolerance:  %s mm / %s deg" % (self._fmt(tol[0]), self._fmt(tol[1], 2)))
            add("#  Avg tolerance:    %s mm / %s deg" % (self._fmt(tol[2]), self._fmt(tol[3], 2)))
        else:
            add("#  Worst tolerance:  n/a (no drivetrain)")
            add("#  Avg tolerance:    n/a (no drivetrain)")
        add("#  Recorded:         %s s (%d frames)" % (self._fmt((T[-1] - T[0]) / 1000.0, 2), len(T)))
        add("#  Moves:            %d" % len(moves))
        if drive:
            add("#  Heading from:     %s" % ('IMU "%s"' % drive["imu"] if drive["imu"] else "wheels (no IMU)"))
            add("#  Distance from:    %s" % ('odom "%s" (r = %s)' % (drive["odom"], self._fmt(abs(drive["odom_r"]), 3))
                                           if drive["odom"] else "drive motors"))
        add("#  Made by:          %s" % self.ENCODER_VERSION)
        for note in self.notes:
            add("#  NOTE: %s" % note)
        add(bar)
        add("# Run from main.py:  Auton.run_script(\"%s\", globals())" % self.name)
        add("# Each move is one line, so edit freely. Distances in mm, headings in")
        add("# degrees (clockwise +, 0 = the way the robot faced at the start),")
        add("# speeds in mm/s (straight, arc) or deg/s (turn_to).")
        add("# at=[(progress, action)] runs the action once the move has gone that far:")
        add("# mm for straight/arc, degrees for turn_to, ms for hold.")
        add("")

        # ----- robot setup -----
        add(sub)
        add("# ROBOT SETUP  (edit if the wiring changes)")
        add(sub)
        if drive:
            add(self._group_line("drive_left", drive["left"]["motors"], drive["left"]["flips"]))
            add(self._group_line("drive_right", drive["right"]["motors"], drive["right"]["flips"]))
        for g in mech_groups:
            add(self._group_line(g["name"], g["motors"], g["flips"]))
        if drive:
            add("")
            add("drive = Drive(drive_left, drive_right,")
            add("              mm_per_motor_deg=%s," % self._fmt(drive["mm_per_motor_deg"], 5))
            add("              mm_per_turn_deg=%s," % self._fmt(drive["mm_per_turn_deg"], 4))
            add("              imu=%s," % (drive["imu"] or "None"))
            if drive["odom"]:
                add("              odom=%s, mm_per_odom_deg=%s)" % (drive["odom"], self._fmt(drive["mm_per_odom_deg"], 5)))
            else:
                add("              odom=None)")
        add("")
        add("")

        # ----- main_script -----
        add("def main_script():")
        add("    # " + "-" * 70)
        add("    # START: zero everything and set the starting state")
        add("    # " + "-" * 70)
        if drive:
            add("    drive.begin()")
        for g in mech_groups:
            add("    %s.begin()" % g["name"])
        for code in start_lines:
            add("    " + code)
        add("")

        if moves:
            add("    # " + "-" * 70)
            add("    # MOVES")
            add("    # " + "-" * 70)
            for num, m in enumerate(moves, 1):
                F, H = drive["F"], drive["H"]
                add("    # Move %d of %d  (t = %s s)" % (num, len(moves), self._fmt((T[m.i] - T[0]) / 1000.0, 2)))
                if m.kind == "straight":
                    call = "drive.straight(%s, speed=%d, heading=%s" % (
                        self._fmt(F[m.j] - F[m.i]), round(m.speed), self._fmt(H[m.j], 2))
                elif m.kind == "arc":
                    call = "drive.arc(%s, to_heading=%s, speed=%d" % (
                        self._fmt(F[m.j] - F[m.i]), self._fmt(H[m.j], 2), round(m.speed))
                elif m.kind == "turn":
                    call = "drive.turn_to(%s, speed=%d" % (self._fmt(H[m.j], 2), round(m.speed))
                else:
                    call = "drive.hold(%d" % (T[m.j] - T[m.i])
                if m.at:
                    unit = "ms" if m.kind == "hold" else "deg" if m.kind == "turn" else "mm"
                    add("    " + call + ", at=[")
                    for prog, code in m.at:
                        add("        (%s, lambda: %s),   # at %s %s" % (self._fmt(prog), code, self._fmt(prog), unit))
                    add("    ])")
                else:
                    add("    " + call + ")")
                add("")

        if tail:
            add("    # " + "-" * 70)
            add("    # %s" % ("AFTER THE LAST MOVE" if moves else "TIMED ACTIONS (no drivetrain)"))
            add("    # " + "-" * 70)
            last_t = T[moves[-1].j] if moves else T[0]
            for t, code in tail:
                if t - last_t > 0:
                    add("    wait(%d, MSEC)" % (t - last_t))
                add("    " + code)
                last_t = t
            add("")

        for g in mech_groups:   # let the last mechanism moves finish
            add("    %s.wait_done()" % g["name"])
        if drive:
            add("    drive.stop()")
        elif not (mech_groups or start_lines or tail):
            add("    pass   # nothing was recorded")
        add("")
        add("")
        add("# CLEAR-END")
        add("")

        text = "\n".join(L)
        size = len(text.encode("utf-8"))
        return text.replace(self.SIZE_FIELD, "%010d" % size, 1)

    @staticmethod
    def _group_line(name, motors, flips):
        if any(flips):
            return "%s = MotorGroup([%s], [%s])" % (
                name, ", ".join(motors), ", ".join("True" if f else "False" for f in flips))
        return "%s = MotorGroup([%s])" % (name, ", ".join(motors))

    # =======================================================================
    # 10b. Save safely
    # =======================================================================
    def _atomic_write(self, data, p):
        """Write to ACPv1/TMP/<name>.CLEAR.tmp, check it, then put it in
        ACPv1/Data/<name>.py. The script ends with "# CLEAR-END", and
        run_script() falls back to the .tmp copy if the .py is cut short,
        so stopping the program at any point never leaves a broken script."""
        total = len(data)

        # 1. temp file, in chunks, with progress
        brain.sdcard.savefile(self.tmp_path, bytearray(0))
        written = 0
        while written < total:
            chunk = data[written:written + self.WRITE_CHUNK]
            brain.sdcard.appendfile(self.tmp_path, bytearray(chunk))
            written += len(chunk)
            p(90 + 7 * written // total, "writing temp file (%d/%d bytes)" % (written, total))

        if brain.sdcard.filesize(self.tmp_path) != total:
            p(97, "temp file size wrong, not replacing script")
            return False

        # 2. write the real script in one go. V5 Python has no os module
        # (no rename, no delete), so the checked .tmp stays as the backup
        # copy run_script() falls back to.
        p(98, "saving %s" % self.script_path)
        brain.sdcard.savefile(self.script_path, bytearray(data))

        ok = brain.sdcard.filesize(self.script_path) == total
        p(99, "checked %s (%d bytes)" % (self.script_path, total) if ok else "final size wrong")
        return ok

# ###########################################################################
# AUTON: runs the scripts Encode writes
# ###########################################################################
class Auton:
    """CLEARv2 Auton runtime.

    The scripts Encode writes (ACPv1/Data/<name>.py) only call what is in this
    file. Every move drives closed loop to a target, so the robot ends where the
    recording ended instead of replaying timing.

    Usage in main.py:
        from ACPv1.CLEARv2 import Auton
        def aton():
            Auton.run_script("RightAWP_001", globals())

    What a script can call:
        drive.begin()                                   zero distance + heading
        drive.straight(mm, speed, heading, at=[...])    drive straight, hold heading
        drive.arc(mm, to_heading, speed, at=[...])      drive while turning smoothly
        drive.turn_to(heading, speed, at=[...])         turn in place, hold distance
        drive.hold(ms, at=[...])                        stay put for ms
        drive.stop()                                    brake all drive motors
        group.move_to(deg, rpm)                         mechanism move (does not block)
        group.wait_done()                               wait for the last move_to
        set_global(name, value)                         change a variable in main

    at=[(progress, action), ...] runs action once the move has gone that far:
    mm for straight/arc, degrees for turn_to, ms for hold.

    Units: distance in mm, heading in degrees (clockwise is +), speed in mm/s
    for straight/arc and deg/s for turn_to.
    """

    # ===========================================================================
    # TUNING (edit these to change how hard the robot corrects)
    # ===========================================================================
    LOOP_MS = 10

    KP_DIST = 6.0          # mm/s of drive speed per mm of distance error
    KP_HEAD = 5.0          # deg/s of turn per degree of heading error (while driving)
    KP_TURN = 6.0          # deg/s of turn per degree of heading error (turn_to)
    MAX_HEAD_FIX = 120     # deg/s cap on heading correction while driving

    ACCEL_MMPS2 = 2500     # max change in drive speed per second (mm/s^2)
    TURN_ACCEL = 900       # max change in turn speed per second (deg/s^2)

    SETTLE_MM = 3.0        # straight/arc/hold is done inside this...
    SETTLE_DEG = 0.75      # ...turn_to inside this
    SETTLE_LOOPS = 3       # ...for this many loops in a row
    TIMEOUT_EXTRA_MS = 1500


    @staticmethod
    def _clamp(x, lo, hi):
        return lo if x < lo else hi if x > hi else x


    @staticmethod
    def _approach(current, target, step):
        """Move current toward target by at most step (acceleration limit)."""
        if target > current + step:
            return current + step
        if target < current - step:
            return current - step
        return target

    # ===========================================================================
    # MotorGroup: motors that always move together (found by Encode).
    # ===========================================================================
    class MotorGroup:
        def __init__(self, motors, reversed_flags=None):
            self.motors = list(motors)
            self.flips = [(-1 if r else 1) for r in (reversed_flags or [False] * len(self.motors))]
            self.zero = 0.0
            self.target = None

        def wait_done(self, tol_deg=3.0, timeout_ms=1500):
            """Wait until the last move_to() arrives (or timeout_ms passes)."""
            waited = 0
            while (self.target is not None and abs(self.position() - self.target) > tol_deg
                   and waited < timeout_ms):
                wait(Auton.LOOP_MS, MSEC)
                waited += Auton.LOOP_MS

        def position(self):
            """Average position in degrees, relative to begin()."""
            total = 0.0
            for m, f in zip(self.motors, self.flips):
                total += f * m.position(DEGREES)
            return total / len(self.motors) - self.zero

        def begin(self):
            self.zero = 0.0
            self.zero = self.position()
            self.target = None

        def spin(self, rpm):
            for m, f in zip(self.motors, self.flips):
                m.spin(FORWARD, f * rpm, RPM)

        def stop(self, mode=BRAKE):
            for m in self.motors:
                m.stop(mode)

        def move_to(self, deg, rpm=200):
            """Go to deg (relative to begin()) at rpm. Does not wait."""
            rpm = abs(rpm) if rpm else 200
            self.target = deg
            for m, f in zip(self.motors, self.flips):
                target = (deg + self.zero) * f
                m.spin_to_position(target, DEGREES, rpm, RPM, False)

    # ===========================================================================
    # Drive: left + right groups, corrected by IMU heading and odom distance.
    # ===========================================================================
    class Drive:
        def __init__(self, left, right, mm_per_motor_deg, mm_per_turn_deg,
                     imu=None, odom=None, mm_per_odom_deg=0.0):
            """mm_per_motor_deg: wheel travel per motor degree.
            mm_per_turn_deg:  wheel travel (each side) per degree of robot turn,
                              measured from the recording (includes wheel scrub).
            imu / odom:       None if the recording showed them unplugged."""
            self.left = left
            self.right = right
            self.mm_per_motor_deg = mm_per_motor_deg
            self.mm_per_turn_deg = mm_per_turn_deg
            self.imu = imu
            self.odom = odom
            self.mm_per_odom_deg = mm_per_odom_deg
            self.timer = Timer()

            # Where the robot SHOULD be. Every move adds to these, so a small miss
            # is corrected by the next move instead of piling up.
            self.want_mm = 0.0
            self.want_deg = 0.0
            self.mm0 = 0.0
            self.deg0 = 0.0

        # ----- sensors -----
        def distance(self):
            """mm driven since begin(). Odom if there is one, else motor average."""
            if self.odom is not None:
                raw = self.odom.position(DEGREES) * self.mm_per_odom_deg
            else:
                raw = (self.left.position() + self.right.position()) / 2 * self.mm_per_motor_deg
            return raw - self.mm0

        def heading(self):
            """Degrees turned since begin(), clockwise +. IMU if there is one,
            else estimated from the wheel difference."""
            if self.imu is not None:
                raw = self.imu.rotation(DEGREES)
            else:
                diff_mm = (self.left.position() - self.right.position()) / 2 * self.mm_per_motor_deg
                raw = diff_mm / self.mm_per_turn_deg
            return raw - self.deg0

        def begin(self):
            """Call once at the start of a script: current spot = zero."""
            self.left.begin()
            self.right.begin()
            self.mm0 = 0.0
            self.deg0 = 0.0
            self.mm0 = self.distance()
            self.deg0 = self.heading()
            self.want_mm = 0.0
            self.want_deg = 0.0

        def stop(self):
            self.left.stop(BRAKE)
            self.right.stop(BRAKE)

        # ----- moves -----
        def straight(self, mm, speed=600, heading=None, at=None):
            """Drive mm (negative = backwards) holding heading."""
            if heading is not None:
                self.want_deg = heading
            start = self.want_mm
            self.want_mm += mm
            h = self.want_deg
            self._run("mm", start, self.want_mm, lambda p: h, speed, at)

        def arc(self, mm, to_heading, speed=600, at=None):
            """Drive mm while the heading changes smoothly to to_heading."""
            start = self.want_mm
            h_from = self.want_deg
            self.want_mm += mm
            self.want_deg = to_heading

            def heading_at(progress):
                if mm == 0:
                    return to_heading
                frac = Auton._clamp(progress / abs(mm), 0.0, 1.0)
                return h_from + (to_heading - h_from) * frac

            # feedforward: degrees of turn per mm driven, so the heading keeps up
            ff = (to_heading - h_from) / mm if mm else 0.0
            self._run("mm", start, self.want_mm, heading_at, speed, at, ff_deg_per_mm=ff)

        def turn_to(self, heading, speed=180, at=None):
            """Turn in place to heading (degrees since begin)."""
            self.want_deg = heading
            self._run("deg", self.heading(), self.want_mm, lambda p: heading, speed, at)

        def hold(self, ms, at=None):
            """Stay on the spot for ms, still correcting position and heading."""
            self._run("ms", 0, self.want_mm, lambda p: self.want_deg, 0, at, hold_ms=ms)

        # ----- the one control loop every move uses -----
        def _run(self, kind, start, target_mm, heading_at, speed, at, hold_ms=0, ff_deg_per_mm=0.0):
            """kind "mm": progress = mm driven, ends on distance.
               kind "deg": progress = degrees turned, ends on heading.
               kind "ms":  progress = time, ends after hold_ms."""
            at = sorted(at or [], key=lambda a: a[0])
            speed = abs(speed) if speed else 600

            if kind == "mm":
                est_ms = 1000 * abs(target_mm - start) / speed
            elif kind == "deg":
                est_ms = 1000 * abs(heading_at(0) - start) / speed
            else:
                est_ms = hold_ms
            timeout = est_ms * 2 + Auton.TIMEOUT_EXTRA_MS

            t0 = self.timer.time()
            v = 0.0          # commanded drive speed, mm/s
            w = 0.0          # commanded turn speed, deg/s
            settled = 0

            while True:
                elapsed = self.timer.time() - t0
                dist = self.distance()
                head = self.heading()

                # how far this move has got, for the at=[...] actions
                if kind == "mm":
                    progress = abs(dist - start)
                elif kind == "deg":
                    progress = abs(head - start)
                else:
                    progress = elapsed
                at = self._fire(at, progress)

                # distance: P on error, capped, acceleration limited
                err_mm = target_mm - dist
                v_cap = speed if kind == "mm" else 300
                v = Auton._approach(v, Auton._clamp(Auton.KP_DIST * err_mm, -v_cap, v_cap), Auton.ACCEL_MMPS2 * Auton.LOOP_MS / 1000)

                # heading: turn_to uses its own speed; everything else just corrects
                err_deg = heading_at(progress) - head
                if kind == "deg":
                    w = Auton._approach(w, Auton._clamp(Auton.KP_TURN * err_deg, -speed, speed), Auton.TURN_ACCEL * Auton.LOOP_MS / 1000)
                else:
                    w = Auton._clamp(Auton.KP_HEAD * err_deg, -Auton.MAX_HEAD_FIX, Auton.MAX_HEAD_FIX) + ff_deg_per_mm * v

                self._spin(v, w)

                # finished?
                if kind == "ms":
                    if elapsed >= hold_ms:
                        break
                else:
                    # a turn ends on heading; a straight or arc needs distance AND heading
                    done = abs(err_deg) < Auton.SETTLE_DEG
                    if kind == "mm":
                        done = done and abs(err_mm) < Auton.SETTLE_MM
                    settled = settled + 1 if done else 0
                    if settled >= Auton.SETTLE_LOOPS or elapsed > timeout:
                        break
                wait(Auton.LOOP_MS, MSEC)

            self._fire(at, 1e18)  # anything not reached still runs
            self.stop()

        def _spin(self, v, w):
            """v: forward mm/s, w: clockwise deg/s -> motor RPM per side."""
            turn_mm = w * self.mm_per_turn_deg
            to_rpm = 1.0 / (self.mm_per_motor_deg * 6.0)   # mm/s -> motor RPM
            self.left.spin((v + turn_mm) * to_rpm)
            self.right.spin((v - turn_mm) * to_rpm)

        @staticmethod
        def _fire(at, progress):
            """Run every (progress, action) that has been reached, in order."""
            while at and progress >= at[0][0]:
                at.pop(0)[1]()
            return at

    # ===========================================================================
    # Loading a script made by Encode
    # ===========================================================================
    END_MARK = b"# CLEAR-END"
    DATA_DIR = "ACPv1/Data"
    TMP_DIR = "ACPv1/TMP"


    @staticmethod
    def _complete(data):
        # bytes(): MicroPython's bytearray has no rstrip()/endswith()
        return data is not None and bytes(data).rstrip().endswith(Auton.END_MARK)


    @staticmethod
    def _load(path):
        try:
            if brain.sdcard.exists(path):
                return brain.sdcard.loadfile(path)
        except Exception:
            pass
        return None


    @staticmethod
    def run_script(name, MainDict):
        """Load ACPv1/Data/<name>.py and run its main_script().

        If the file is missing its end marker (power cut mid-write), the finished
        copy in ACPv1/TMP/<name>.CLEAR.tmp is used instead."""
        data = Auton._load("%s/%s.py" % (Auton.DATA_DIR, name))
        if not Auton._complete(data):
            data = Auton._load("%s/%s.CLEAR.tmp" % (Auton.TMP_DIR, name))
            if not Auton._complete(data):
                print("CLEAR: script %s missing or incomplete" % name)
                return False

        env = dict(MainDict)          # the script sees main's devices by name
        env["MotorGroup"] = Auton.MotorGroup
        env["Drive"] = Auton.Drive
        env.setdefault("wait", wait)     # timed scripts need these even if
        env.setdefault("MSEC", MSEC)     # main did not import them by name

        def set_global(n, value):
            MainDict[n] = value
        env["set_global"] = set_global

        if data is None:
            return False
        try:
            exec(bytes(data).decode("utf-8"), env)
            env["main_script"]()
            return True
        except Exception as e:
            # e.g. a device renamed in main since recording: say so, never
            # leave motors spinning, and never crash the whole program
            print("CLEAR: script %s stopped: %s: %s" % (name, type(e).__name__, e))
            for v in env.values():
                if isinstance(v, (Auton.Drive, Auton.MotorGroup)):
                    try:
                        v.stop()
                    except Exception:
                        pass
            return False


# Ready-made recorder, as before: RE.recording.start(globals(), "name")
recording = Recording()
