# V5Sim: vex API test. Checks (1) the API surface against VEXcode V5 Python and
# (2) behavior of each call on simulated devices (robot: tests/api_robot.json).
# Output lines: "MISS <Class>.<name>", "CHK <PASS|FAIL|ERR> <test> | got ... | want ...".
from vex import *
import vex

def chk(name, fn, want=None, tol=None, pred=None):
    try:
        got = fn()
    except Exception as e:
        print("CHK ERR %s | %s: %s" % (name, type(e).__name__, e))
        return None
    ok = True
    if pred is not None:
        ok = pred(got)
    elif tol is not None:
        ok = abs(got - want) <= tol
    elif want is not None:
        ok = got == want
    print("CHK %s %s | got %r | want %s" % ("PASS" if ok else "FAIL", name, got,
          ("%r +/- %r" % (want, tol)) if tol is not None else (repr(want) if pred is None else "condition")))
    return got

def isnum(x):
    return isinstance(x, (int, float))

# ---------------------------------------------------------------- 1. API surface
EXPECT = {
    "vex": "Brain Controller Competition Motor MotorGroup DriveTrain SmartDrive Rotation Inertial Optical "
           "Distance Gps Event Thread Timer DigitalOut DigitalIn Bumper Limit AnalogIn Pot PotV2 Encoder Led Light Line "
           "Sonar Gyro Accelerometer Pneumatics Servo Motor29 MotorVictor Electromagnet Vision AiVision MessageLink SerialLink "
           "Ports wait FORWARD REVERSE LEFT RIGHT COAST BRAKE HOLD MSEC SECONDS DEGREES TURNS RPM PERCENT DPS VOLT MM INCHES "
           "AMP WATT XAXIS YAXIS ZAXIS PITCH ROLL YAW "
           "DirectionType TurnType BrakeType TimeUnits RotationUnits VelocityUnits VoltageUnits CurrentUnits PowerUnits "
           "TorqueUnits TemperatureUnits PercentUnits DistanceUnits AnalogUnits AxisType OrientationType GearSetting "
           "FontType Color LedStateType ObjectSizeType VexlinkType",
    "Motor": "spin spin_for spin_to_position stop set_velocity set_stopping set_position reset_position set_max_torque "
             "set_timeout get_timeout set_reversed is_spinning is_done direction position velocity current power torque "
             "efficiency temperature command installed timestamp",
    "MotorGroup": "spin spin_for spin_to_position stop set_velocity set_stopping set_position reset_position set_max_torque "
                  "set_timeout get_timeout is_spinning is_done direction position velocity current power torque "
                  "efficiency temperature count",
    "DriveTrain": "drive drive_for turn turn_for stop set_drive_velocity set_turn_velocity set_turn_constant set_turn_threshold "
                  "set_stopping set_timeout get_timeout is_moving is_done velocity current power torque efficiency temperature "
                  "turn_to_heading turn_to_rotation set_heading set_rotation heading rotation",
    "Rotation": "angle position velocity set_position reset_position set_reversed changed installed timestamp",
    "Inertial": "calibrate is_calibrating set_heading set_rotation reset_heading reset_rotation heading rotation orientation "
                "gyro_rate acceleration changed collision set_turn_type get_turn_type installed timestamp",
    "Optical": "hue brightness color rgb is_near_object set_light set_light_power integration_time object_detected object_lost "
               "gesture_detected gesture_enable gesture_disable installed timestamp",
    "Distance": "object_distance object_size object_rawsize object_velocity is_object_detected changed installed timestamp",
    "Timer": "time clear reset value event system system_high_res",
    "Event": "broadcast broadcast_and_wait",
    "Thread": "stop",
    "Competition": "is_enabled is_driver_control is_autonomous is_competition_switch is_field_control",
    "DigitalOut": "set value",
}
for cls, names in EXPECT.items():
    obj = vex if cls == "vex" else getattr(vex, cls, None)
    if obj is None:
        print("MISS vex.%s (class)" % cls)
        continue
    have = dir(obj)
    for n in names.split():
        if n not in have:
            print("MISS %s.%s" % (cls, n))

# ---------------------------------------------------------------- 2. Motor
T0 = Timer()
brain = Brain()
chk("Controller() no args (VEXcode default PRIMARY)", lambda: Controller() is not None, True)
ctl = Controller(PRIMARY)
m = Motor(Ports.PORT3, GearSetting.RATIO_18_1, False)   # green, physical 18:1 in api_robot.json
chk("Motor.installed (plugged)", lambda: m.installed(), True)
chk("Motor.installed (empty port)", lambda: Motor(Ports.PORT10).installed(), False)
try:
    m.set_position(0, DEGREES)
except Exception as _e:
    print("CHK ERR line 75 %s: %s" % (type(_e).__name__, _e))
chk("Motor.set_position 0", lambda: m.position(DEGREES), 0, 1)
try:
    m.spin(FORWARD)                       # VEXcode default velocity is 50%
except Exception as _e:
    print("CHK ERR line 77 %s: %s" % (type(_e).__name__, _e))
try:
    wait(800, MSEC)
except Exception as _e:
    print("CHK ERR line 78 %s: %s" % (type(_e).__name__, _e))
chk("Motor.spin default 50%% -> rpm", lambda: m.velocity(RPM), 100, 8)
chk("Motor.velocity PERCENT", lambda: m.velocity(PERCENT), 50, 4)
chk("Motor.velocity DPS = rpm*6", lambda: m.velocity(DPS) / m.velocity(RPM), 6, 0.2)
chk("Motor.is_spinning while spin()", lambda: m.is_spinning(), True)
try:
    m.spin(FORWARD, 150, RPM); wait(800, MSEC)
except Exception as _e:
    print("CHK ERR line 83 %s: %s" % (type(_e).__name__, _e))
chk("Motor.spin 150 RPM", lambda: m.velocity(RPM), 150, 8)
try:
    m.spin(REVERSE, 25, PERCENT); wait(800, MSEC)
except Exception as _e:
    print("CHK ERR line 85 %s: %s" % (type(_e).__name__, _e))
chk("Motor.spin REVERSE 25%", lambda: m.velocity(RPM), -50, 8)
try:
    m.spin(FORWARD, 6, VOLT); wait(800, MSEC)
except Exception as _e:
    print("CHK ERR line 87 %s: %s" % (type(_e).__name__, _e))
chk("Motor.spin 6 VOLT ~half speed", lambda: m.velocity(RPM), 100, 25)
try:
    m.set_velocity(75, PERCENT); m.spin(FORWARD); wait(800, MSEC)
except Exception as _e:
    print("CHK ERR line 89 %s: %s" % (type(_e).__name__, _e))
chk("Motor.set_velocity 75% then spin()", lambda: m.velocity(RPM), 150, 8)
try:
    m.stop(); wait(500, MSEC)
except Exception as _e:
    print("CHK ERR line 91 %s: %s" % (type(_e).__name__, _e))
chk("Motor.stop -> 0 rpm", lambda: m.velocity(RPM), 0, 3)
try:
    m.set_velocity(50, PERCENT)
except Exception as _e:
    print("CHK ERR line 93 %s: %s" % (type(_e).__name__, _e))
try:
    m.reset_position()
except Exception as _e:
    print("CHK ERR line 94 %s: %s" % (type(_e).__name__, _e))
chk("Motor.reset_position", lambda: m.position(DEGREES), 0, 1)
t = Timer()
try:
    m.spin_for(FORWARD, 360, DEGREES)
except Exception as _e:
    print("CHK ERR line 97 %s: %s" % (type(_e).__name__, _e))
chk("Motor.spin_for 360 deg blocks", lambda: t.time(MSEC), 300, pred=lambda v: v > 250)
chk("Motor.spin_for 360 deg -> pos", lambda: m.position(DEGREES), 360, 10)
chk("Motor.is_done after spin_for", lambda: m.is_done(), True)
chk("Motor.position TURNS", lambda: m.position(TURNS), 1, 0.03)
try:
    m.spin_for(REVERSE, 1, TURNS)
except Exception as _e:
    print("CHK ERR line 102 %s: %s" % (type(_e).__name__, _e))
chk("Motor.spin_for REVERSE 1 TURNS", lambda: m.position(DEGREES), 0, 10)
try:
    m.spin_for(FORWARD, 90, DEGREES, 100, PERCENT)
except Exception as _e:
    print("CHK ERR line 104 %s: %s" % (type(_e).__name__, _e))
chk("Motor.spin_for with velocity", lambda: m.position(DEGREES), 90, 10)
try:
    m.spin_to_position(-180, DEGREES)
except Exception as _e:
    print("CHK ERR line 106 %s: %s" % (type(_e).__name__, _e))
chk("Motor.spin_to_position -180", lambda: m.position(DEGREES), -180, 10)
try:
    m.spin_to_position(0, DEGREES, 50, PERCENT, False)
except Exception as _e:
    print("CHK ERR line 108 %s: %s" % (type(_e).__name__, _e))
chk("Motor.spin_to_position wait=False returns", lambda: m.is_done(), False)
try:
    wait(1500, MSEC)
except Exception as _e:
    print("CHK ERR line 110 %s: %s" % (type(_e).__name__, _e))
chk("Motor.spin_to_position wait=False finishes", lambda: m.position(DEGREES), 0, 10)
chk("Motor.is_done after async move", lambda: m.is_done(), True)
chk("Motor.spin_for 500 MSEC (time units)", lambda: (t.clear(), m.spin_for(FORWARD, 500, MSEC), t.time(MSEC))[2], 500, 40)
try:
    m.stop()
except Exception as _e:
    print("CHK ERR line 114 %s: %s" % (type(_e).__name__, _e))
try:
    m.spin_to_position(720, DEGREES, 50, PERCENT, False)
except Exception as _e:
    print("CHK ERR line 115 %s: %s" % (type(_e).__name__, _e))
chk("Motor.is_done right after async start", lambda: m.is_done(), False)
try:
    m.spin_to_position(0, DEGREES)
except Exception as _e:
    print("CHK ERR line 117 %s: %s" % (type(_e).__name__, _e))
try:
    m.set_timeout(200, MSEC)
except Exception as _e:
    print("CHK ERR line 118 %s: %s" % (type(_e).__name__, _e))
chk("Motor.get_timeout", lambda: m.get_timeout(), 200)
try:
    t.clear()
except Exception as _e:
    print("CHK ERR line 120 %s: %s" % (type(_e).__name__, _e))
try:
    m.spin_for(FORWARD, 100000, DEGREES)
except Exception as _e:
    print("CHK ERR line 121 %s: %s" % (type(_e).__name__, _e))
chk("Motor.set_timeout stops spin_for", lambda: t.time(MSEC), 200, 40)
try:
    m.set_timeout(0, MSEC); m.stop()
except Exception as _e:
    print("CHK ERR line 123 %s: %s" % (type(_e).__name__, _e))
try:
    m.set_stopping(HOLD); m.spin_for(FORWARD, 90, DEGREES); p = m.position(DEGREES); wait(500, MSEC)
except Exception as _e:
    print("CHK ERR line 124 %s: %s" % (type(_e).__name__, _e))
chk("Motor.set_stopping HOLD keeps position", lambda: m.position(DEGREES), p, 3)
try:
    m.set_stopping(COAST)
except Exception as _e:
    print("CHK ERR line 126 %s: %s" % (type(_e).__name__, _e))
chk("Motor.set_max_torque PERCENT", lambda: m.set_max_torque(50, PERCENT), None)
chk("Motor.set_max_torque AMP", lambda: m.set_max_torque(1.5, CurrentUnits.AMP), None)
try:
    m.set_max_torque(100, PERCENT)
except Exception as _e:
    print("CHK ERR line 129 %s: %s" % (type(_e).__name__, _e))
try:
    m.spin(FORWARD); wait(500, MSEC)
except Exception as _e:
    print("CHK ERR line 130 %s: %s" % (type(_e).__name__, _e))
chk("Motor.current AMP", lambda: m.current(CurrentUnits.AMP), pred=lambda v: isnum(v) and 0 <= v <= 2.6)
chk("Motor.current()", lambda: m.current(), pred=isnum)
chk("Motor.power WATT", lambda: m.power(WATT), pred=lambda v: isnum(v) and v >= 0)
chk("Motor.torque NM", lambda: m.torque(TorqueUnits.NM), pred=lambda v: isnum(v) and v >= 0)
chk("Motor.torque INLB", lambda: m.torque(TorqueUnits.INLB), pred=lambda v: isnum(v) and v >= 0)
chk("Motor.efficiency PERCENT", lambda: m.efficiency(PERCENT), pred=lambda v: isnum(v) and 0 <= v <= 100)
chk("Motor.temperature CELSIUS", lambda: m.temperature(TemperatureUnits.CELSIUS), pred=lambda v: 15 <= v <= 80)
chk("Motor.temperature PERCENT", lambda: m.temperature(PERCENT), pred=lambda v: 0 <= v <= 100)
try:
    m.stop()
except Exception as _e:
    print("CHK ERR line 139 %s: %s" % (type(_e).__name__, _e))
mr = Motor(Ports.PORT4, GearSetting.RATIO_18_1, True)
try:
    mr.set_position(0, DEGREES); mr.spin_for(FORWARD, 180, DEGREES)
except Exception as _e:
    print("CHK ERR line 141 %s: %s" % (type(_e).__name__, _e))
chk("Motor reversed: spin_for FORWARD reads +180", lambda: mr.position(DEGREES), 180, 10)
try:
    mr.set_reversed(False); mr.spin(FORWARD); wait(500, MSEC)
except Exception as _e:
    print("CHK ERR line 143 %s: %s" % (type(_e).__name__, _e))
chk("Motor.set_reversed(False) velocity +", lambda: mr.velocity(RPM), 100, 10)
try:
    mr.stop()
except Exception as _e:
    print("CHK ERR line 145 %s: %s" % (type(_e).__name__, _e))
chk("Motor(port, True) reversed 2-arg form", lambda: Motor(Ports.PORT11, True) is not None, True)
chk("Motor kw args", lambda: Motor(Ports.PORT12, GearSetting.RATIO_6_1, reverse=False) is not None, True)

# ---------------------------------------------------------------- 3. MotorGroup
a = Motor(Ports.PORT1, GearSetting.RATIO_6_1, False)
b = Motor(Ports.PORT2, GearSetting.RATIO_6_1, True)
g = MotorGroup(a, b)
chk("MotorGroup.count", lambda: g.count(), 2)
try:
    g.set_position(0, DEGREES)
except Exception as _e:
    print("CHK ERR line 155 %s: %s" % (type(_e).__name__, _e))
try:
    g.spin(FORWARD, 100, PERCENT); wait(800, MSEC)
except Exception as _e:
    print("CHK ERR line 156 %s: %s" % (type(_e).__name__, _e))
chk("MotorGroup.spin 100% blue", lambda: g.velocity(RPM), 600, 25)
chk("MotorGroup members same direction", lambda: a.velocity(RPM) - b.velocity(RPM), 0, 25)
try:
    g.stop(); wait(400, MSEC)
except Exception as _e:
    print("CHK ERR line 159 %s: %s" % (type(_e).__name__, _e))
try:
    g.set_position(0, DEGREES)
except Exception as _e:
    print("CHK ERR line 160 %s: %s" % (type(_e).__name__, _e))
try:
    g.spin_for(FORWARD, 720, DEGREES)
except Exception as _e:
    print("CHK ERR line 161 %s: %s" % (type(_e).__name__, _e))
chk("MotorGroup.spin_for 720", lambda: g.position(DEGREES), 720, 15)
chk("MotorGroup member a pos", lambda: a.position(DEGREES), 720, 15)
try:
    g.spin_to_position(0, DEGREES)
except Exception as _e:
    print("CHK ERR line 164 %s: %s" % (type(_e).__name__, _e))
chk("MotorGroup.spin_to_position 0", lambda: g.position(DEGREES), 0, 15)
chk("MotorGroup.is_done", lambda: g.is_done(), True)
chk("MotorGroup.current", lambda: g.current(CurrentUnits.AMP), pred=isnum)
chk("MotorGroup.temperature", lambda: g.temperature(TemperatureUnits.CELSIUS), pred=lambda v: 15 <= v <= 80)
chk("MotorGroup([list]) form", lambda: MotorGroup([a, b]).count(), 2)

# ---------------------------------------------------------------- 4. Drivetrain
lm = Motor(Ports.PORT17, GearSetting.RATIO_6_1, False)
rm = Motor(Ports.PORT18, GearSetting.RATIO_6_1, True)
dt = DriveTrain(lm, rm, 319.19, 295, 40, MM, 1)
try:
    lm.set_position(0, DEGREES); rm.set_position(0, DEGREES)
except Exception as _e:
    print("CHK ERR line 175 %s: %s" % (type(_e).__name__, _e))
try:
    dt.drive_for(FORWARD, 200, MM)
except Exception as _e:
    print("CHK ERR line 176 %s: %s" % (type(_e).__name__, _e))
chk("Drivetrain.drive_for 200mm -> left deg", lambda: lm.position(DEGREES), 225.6, 12)
chk("Drivetrain.drive_for 200mm -> right deg", lambda: rm.position(DEGREES), 225.6, 12)
chk("Drivetrain.is_done", lambda: dt.is_done(), True)
try:
    lm.set_position(0, DEGREES); rm.set_position(0, DEGREES)
except Exception as _e:
    print("CHK ERR line 180 %s: %s" % (type(_e).__name__, _e))
try:
    dt.drive_for(REVERSE, 10, INCHES)
except Exception as _e:
    print("CHK ERR line 181 %s: %s" % (type(_e).__name__, _e))
chk("Drivetrain.drive_for REVERSE 10 in", lambda: lm.position(DEGREES), -286.5, 14)
try:
    lm.set_position(0, DEGREES); rm.set_position(0, DEGREES)
except Exception as _e:
    print("CHK ERR line 183 %s: %s" % (type(_e).__name__, _e))
try:
    dt.turn_for(RIGHT, 90, DEGREES)
except Exception as _e:
    print("CHK ERR line 184 %s: %s" % (type(_e).__name__, _e))
chk("Drivetrain.turn_for RIGHT 90 left +", lambda: lm.position(DEGREES), 261.4, 15)
chk("Drivetrain.turn_for RIGHT 90 right -", lambda: rm.position(DEGREES), -261.4, 15)
try:
    dt.set_drive_velocity(50, PERCENT); dt.drive(FORWARD); wait(800, MSEC)
except Exception as _e:
    print("CHK ERR line 187 %s: %s" % (type(_e).__name__, _e))
chk("Drivetrain.set_drive_velocity 50% + drive", lambda: lm.velocity(RPM), 300, 20)
chk("Drivetrain.is_moving", lambda: dt.is_moving(), True)
chk("Drivetrain.velocity", lambda: dt.velocity(PERCENT), 50, 5)
try:
    dt.set_turn_velocity(25, PERCENT); dt.turn(LEFT); wait(800, MSEC)
except Exception as _e:
    print("CHK ERR line 191 %s: %s" % (type(_e).__name__, _e))
chk("Drivetrain.turn LEFT 25% left -", lambda: lm.velocity(RPM), -150, 15)
try:
    dt.stop(); wait(400, MSEC)
except Exception as _e:
    print("CHK ERR line 193 %s: %s" % (type(_e).__name__, _e))
chk("Drivetrain.stop", lambda: lm.velocity(RPM), 0, 3)
try:
    dt.set_timeout(300, MSEC); t.clear(); dt.drive_for(FORWARD, 5000, MM)
except Exception as _e:
    print("CHK ERR line 195 %s: %s" % (type(_e).__name__, _e))
chk("Drivetrain.set_timeout", lambda: t.time(MSEC), 300, 50)
chk("Drivetrain.get_timeout", lambda: dt.get_timeout(), 300)
try:
    dt.set_timeout(0, MSEC); dt.stop()
except Exception as _e:
    print("CHK ERR line 198 %s: %s" % (type(_e).__name__, _e))

# ---------------------------------------------------------------- 5. Sensors
rot = Rotation(Ports.PORT5, False)
chk("Rotation.installed", lambda: rot.installed(), True)
try:
    rot.set_position(100, DEGREES)
except Exception as _e:
    print("CHK ERR line 203 %s: %s" % (type(_e).__name__, _e))
chk("Rotation.set_position 100", lambda: rot.position(DEGREES), 100, 0.5)
chk("Rotation.position TURNS", lambda: rot.position(TURNS), 100 / 360.0, 0.01)
try:
    rot.reset_position()
except Exception as _e:
    print("CHK ERR line 206 %s: %s" % (type(_e).__name__, _e))
chk("Rotation.reset_position", lambda: rot.position(DEGREES), 0, 0.5)
chk("Rotation.angle 0..360", lambda: rot.angle(DEGREES), pred=lambda v: 0 <= v < 360)
chk("Rotation.velocity RPM", lambda: rot.velocity(RPM), 0, 1)

imu = Inertial(Ports.PORT7)
for _ in range(2):
    print("IMUDIAG t=%d cal=%r head=%r" % (T0.time(MSEC), imu.is_calibrating(), imu.heading()))
    wait(100, MSEC)
chk("Inertial.installed", lambda: imu.installed(), True)
try:
    imu.calibrate()
except Exception as _e:
    print("CHK ERR line 213 %s: %s" % (type(_e).__name__, _e))
chk("Inertial.is_calibrating right after calibrate", lambda: imu.is_calibrating(), True)
for _ in range(8):
    print("IMUDIAG after calibrate t=%d cal=%r" % (T0.time(MSEC), imu.is_calibrating())); wait(400, MSEC)
try:
    imu.set_heading(90, DEGREES)
except Exception as _e:
    print("CHK ERR line 217 %s: %s" % (type(_e).__name__, _e))
chk("Inertial.set_heading 90", lambda: imu.heading(DEGREES), 90, 0.5)
try:
    imu.set_heading(450, DEGREES)
except Exception as _e:
    print("CHK ERR line 219 %s: %s" % (type(_e).__name__, _e))
chk("Inertial.set_heading 450 wraps", lambda: imu.heading(DEGREES), 90, 0.5)
try:
    imu.set_rotation(-45, DEGREES)
except Exception as _e:
    print("CHK ERR line 221 %s: %s" % (type(_e).__name__, _e))
chk("Inertial.set_rotation -45", lambda: imu.rotation(DEGREES), -45, 0.5)
try:
    imu.reset_heading()
except Exception as _e:
    print("CHK ERR line 223 %s: %s" % (type(_e).__name__, _e))
chk("Inertial.reset_heading", lambda: imu.heading(), 0, 0.5)
try:
    imu.reset_rotation()
except Exception as _e:
    print("CHK ERR line 225 %s: %s" % (type(_e).__name__, _e))
chk("Inertial.reset_rotation", lambda: imu.rotation(), 0, 0.5)
chk("Inertial.orientation ROLL", lambda: imu.orientation(ROLL, DEGREES), pred=isnum)
chk("Inertial.orientation YAW", lambda: imu.orientation(YAW), pred=isnum)
chk("Inertial.gyro_rate ZAXIS", lambda: imu.gyro_rate(ZAXIS, DPS), 0, 1)
chk("Inertial.acceleration ZAXIS ~ -1 g", lambda: imu.acceleration(ZAXIS), pred=lambda v: isnum(v) and 0.8 <= abs(v) <= 1.2)
try:
    imu.set_turn_type(LEFT)
except Exception as _e:
    print("CHK ERR line 231 %s: %s" % (type(_e).__name__, _e))
chk("Inertial.get_turn_type LEFT", lambda: imu.get_turn_type(), LEFT)
try:
    imu.set_turn_type(RIGHT)
except Exception as _e:
    print("CHK ERR line 233 %s: %s" % (type(_e).__name__, _e))

dist = Distance(Ports.PORT8)
chk("Distance.object_distance MM", lambda: dist.object_distance(MM), pred=lambda v: isnum(v) and v >= 0)
chk("Distance.object_distance INCHES ratio", lambda: dist.object_distance(MM) / max(dist.object_distance(INCHES), 1e-6), 25.4, 0.5)
chk("Distance.is_object_detected", lambda: dist.is_object_detected(), pred=lambda v: v in (True, False))
chk("Distance.object_size", lambda: dist.object_size(), pred=lambda v: v is not None)

opt = Optical(Ports.PORT9)
try:
    opt.set_light(LedStateType.ON); opt.set_light_power(50, PERCENT)
except Exception as _e:
    print("CHK ERR line 242 %s: %s" % (type(_e).__name__, _e))
chk("Optical.hue 0..360", lambda: opt.hue(), pred=lambda v: isnum(v) and 0 <= v <= 360)
chk("Optical.brightness 0..100", lambda: opt.brightness(), pred=lambda v: isnum(v) and 0 <= v <= 100)
chk("Optical.color", lambda: opt.color(), pred=lambda v: v is not None)
chk("Optical.is_near_object", lambda: opt.is_near_object(), pred=lambda v: v in (True, False))

# ---------------------------------------------------------------- 6. Brain, 3-wire, SD, battery
chk("Brain.battery.capacity", lambda: brain.battery.capacity(), pred=lambda v: 0 <= v <= 100)
chk("Brain.battery.voltage MV", lambda: brain.battery.voltage(), pred=lambda v: isnum(v) and v > 0)
chk("Brain.battery.temperature", lambda: brain.battery.temperature(), pred=isnum)
chk("Brain.timer.time", lambda: brain.timer.time(MSEC), pred=lambda v: v > 0)
chk("Brain.sdcard.is_inserted", lambda: brain.sdcard.is_inserted(), pred=lambda v: v in (True, False))
s = brain.screen
for name, f in (("clear_screen", lambda: s.clear_screen()), ("clear_screen Color", lambda: s.clear_screen(Color.RED)),
                ("set_cursor", lambda: s.set_cursor(2, 3)), ("print", lambda: s.print("x", 1, 2.5)),
                ("next_row", lambda: s.next_row()), ("set_font", lambda: s.set_font(FontType.MONO20)),
                ("set_pen_color hex", lambda: s.set_pen_color("#00FF00")), ("set_pen_color Color", lambda: s.set_pen_color(Color.BLUE)),
                ("set_fill_color", lambda: s.set_fill_color(Color.RED)), ("set_pen_width", lambda: s.set_pen_width(3)),
                ("draw_line", lambda: s.draw_line(0, 0, 100, 100)), ("draw_rectangle", lambda: s.draw_rectangle(10, 10, 50, 50)),
                ("draw_circle", lambda: s.draw_circle(100, 100, 20)), ("draw_pixel", lambda: s.draw_pixel(5, 5)),
                ("print_at", lambda: s.print_at("hi", x=50, y=50)), ("clear_row", lambda: s.clear_row(1)),
                ("set_origin", lambda: s.set_origin(0, 0)), ("set_clip_region", lambda: s.set_clip_region(0, 0, 480, 240)),
                ("render", lambda: s.render())):
    chk("Brain.screen." + name, f, pred=lambda v: True)
try:
    s.set_cursor(3, 5)
except Exception as _e:
    print("CHK ERR line 266 %s: %s" % (type(_e).__name__, _e))
chk("Brain.screen.row after set_cursor(3,5)", lambda: s.row(), 3)
chk("Brain.screen.column after set_cursor(3,5)", lambda: s.column(), 5)
chk("Brain.screen.get_string_width", lambda: s.get_string_width("hello"), pred=lambda v: isnum(v) and v > 0)
chk("Brain.screen.pressing", lambda: s.pressing(), False)
dout = DigitalOut(brain.three_wire_port.a)
try:
    dout.set(True)
except Exception as _e:
    print("CHK ERR line 272 %s: %s" % (type(_e).__name__, _e))
chk("DigitalOut.set True -> value", lambda: dout.value(), 1)
try:
    dout.set(False)
except Exception as _e:
    print("CHK ERR line 274 %s: %s" % (type(_e).__name__, _e))
chk("DigitalOut.set False -> value", lambda: dout.value(), 0)

# ---------------------------------------------------------------- 7. Timer, Event, Thread, Competition, Controller
try:
    t.clear(); wait(250, MSEC)
except Exception as _e:
    print("CHK ERR line 278 %s: %s" % (type(_e).__name__, _e))
chk("Timer.time MSEC after 250 ms", lambda: t.time(MSEC), 250, 3)
chk("Timer.time SECONDS", lambda: t.time(SECONDS), 0.25, 0.01)
chk("Timer.value (seconds)", lambda: t.value(), 0.25, 0.01)
fired = []
try:
    t.event(lambda: fired.append(t.time(MSEC)), 100)
except Exception as _e:
    print("CHK ERR line 283 %s: %s" % (type(_e).__name__, _e))
try:
    wait(300, MSEC)
except Exception as _e:
    print("CHK ERR line 284 %s: %s" % (type(_e).__name__, _e))
chk("Timer.event fires once ~100 ms", lambda: len(fired), 1)
try:
    wait(1, SECONDS)
except Exception as _e:
    print("CHK ERR line 286 %s: %s" % (type(_e).__name__, _e))
chk("wait(1, SECONDS)", lambda: t.time(MSEC), 1550, 15)

hits = []
ev = Event()
try:
    ev(lambda: hits.append(1))
except Exception as _e:
    print("CHK ERR line 291 %s: %s" % (type(_e).__name__, _e))
try:
    ev(lambda: hits.append(2))
except Exception as _e:
    print("CHK ERR line 292 %s: %s" % (type(_e).__name__, _e))
try:
    ev.broadcast(); wait(50, MSEC)
except Exception as _e:
    print("CHK ERR line 293 %s: %s" % (type(_e).__name__, _e))
chk("Event.broadcast runs all handlers", lambda: sorted(hits), [1, 2])
hits.clear()
def slow():
    wait(100, MSEC); hits.append("slow")
ev2 = Event(slow)
try:
    t.clear(); ev2.broadcast_and_wait()
except Exception as _e:
    print("CHK ERR line 299 %s: %s" % (type(_e).__name__, _e))
chk("Event.broadcast_and_wait waits", lambda: (hits, t.time(MSEC) >= 95), (["slow"], True))

out = []
def worker(x=None):
    out.append(x)
try:
    Thread(worker); wait(20, MSEC)
except Exception as _e:
    print("CHK ERR line 305 %s: %s" % (type(_e).__name__, _e))
chk("Thread(fn)", lambda: out, [None])
out.clear()
try:
    Thread(worker, (7,)); wait(20, MSEC)
except Exception as _e:
    print("CHK ERR line 308 %s: %s" % (type(_e).__name__, _e))
chk("Thread(fn, (args,))", lambda: out, [7])
cnt = [0]
def spinner():
    while True:
        cnt[0] += 1; wait(5, MSEC)
th = Thread(spinner); wait(100, MSEC); th.stop(); c = cnt[0]; wait(100, MSEC)
chk("Thread.stop", lambda: cnt[0] - c, 0, 1)

drv = []
comp = Competition(lambda: drv.append("driver"), lambda: drv.append("auto"))
try:
    wait(200, MSEC)
except Exception as _e:
    print("CHK ERR line 319 %s: %s" % (type(_e).__name__, _e))
chk("Competition driver callback (field=driver)", lambda: drv, ["driver"])
chk("Competition.is_driver_control", lambda: comp.is_driver_control(), True)
chk("Competition.is_enabled", lambda: comp.is_enabled(), True)
chk("Competition.is_autonomous", lambda: comp.is_autonomous(), False)

presses = []
try:
    ctl.buttonR1.pressed(lambda: presses.append("p"), ())
except Exception as _e:
    print("CHK ERR line 326 %s: %s" % (type(_e).__name__, _e))
try:
    ctl.buttonR1.released(lambda: presses.append("r"), ())
except Exception as _e:
    print("CHK ERR line 327 %s: %s" % (type(_e).__name__, _e))
moves = []
try:
    ctl.axis3.changed(lambda: moves.append(1), ())
except Exception as _e:
    print("CHK ERR line 329 %s: %s" % (type(_e).__name__, _e))
chk("Controller.axis3.position before input", lambda: ctl.axis3.position(), 0)
while t.time(MSEC) < 0:  # keep Python name t referenced
    pass
tw = Timer()
while T0.time(MSEC) < 30800:
    wait(20, MSEC)
chk("Controller.axis3.position scripted 100", lambda: ctl.axis3.position(), 100)
chk("Controller.axis1.position scripted 30", lambda: ctl.axis1.position(), 30)
chk("Controller.buttonR1.pressing scripted", lambda: ctl.buttonR1.pressing(), True)
chk("Controller.axis3.changed callback", lambda: len(moves) >= 1, True)
chk("Controller.buttonR1.pressed callback", lambda: presses, ["p"])
while T0.time(MSEC) < 34000:
    wait(20, MSEC)
chk("Controller.buttonR1.released callback", lambda: presses, ["p", "r"])
chk("Controller.screen.print", lambda: ctl.screen.print("hi"), None)
chk("Controller.screen.set_cursor/row", lambda: (ctl.screen.set_cursor(2, 1), ctl.screen.row())[1], 2)
chk("Controller.rumble", lambda: ctl.rumble("-."), None)
print("API TEST DONE")
