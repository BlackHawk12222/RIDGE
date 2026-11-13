#region VEXcode Generated Robot Configuration
from vex import *

# Brain should be defined by default
brain=Brain()

# Robot configuration code (same ports as the ACPv1 test)
controller_1 = Controller(PRIMARY)
Right1 = Motor(Ports.PORT17, GearSetting.RATIO_6_1, False)
Right2 = Motor(Ports.PORT18, GearSetting.RATIO_6_1, False)
left1 = Motor(Ports.PORT19, GearSetting.RATIO_6_1, True)
left2 = Motor(Ports.PORT20, GearSetting.RATIO_6_1, True)
inertial = Inertial(Ports.PORT7)
Xodom = Rotation(Ports.PORT5)
#endregion VEXcode Generated Robot Configuration

# ---------------------------------------------------------------------------
# Firmware benchmark. Times each call ASAP makes, N times in a row, so the
# millisecond timer still gives sub-microsecond resolution per call.
# Runs unchanged on official VEXcode V5 Python and on Custom_python.
# Run it on BOTH and paste both terminal outputs back to Hark.
# ---------------------------------------------------------------------------
wait(200, MSEC)
print("\033[2J")

MOTORS = [Right1, Right2, left1, left2]
N = 2000
timer = Timer()


def bench(name, fn, n=N):
    gc_collect()
    start = timer.time(MSEC)
    fn(n)
    total = timer.time(MSEC) - start
    print("%-28s %7.2f us/call  (%d ms for %d)" % (name, total * 1000.0 / n, total, n))
    return total * 1000.0 / n


def gc_collect():
    try:
        import gc
        gc.collect()
    except ImportError:
        pass


# --- each test runs the call n times -------------------------------------
def t_empty(n):
    for _ in range(n):
        pass

def t_float_math(n):
    x = 1.5
    for _ in range(n):
        x = x * 1.0001 + 0.5 - 0.5

def t_list_alloc(n):           # what the 1x1 Matrix Kalman does every loop
    for _ in range(n):
        m = [[1.0]]

def t_velocity(n):
    m = Right1
    for _ in range(n):
        m.velocity(RPM)

def t_temperature(n):
    m = Right1
    for _ in range(n):
        m.temperature(PERCENT)

def t_heading(n):
    for _ in range(n):
        inertial.heading()

def t_accel(n):
    for _ in range(n):
        inertial.acceleration(XAXIS)

def t_rot_velocity(n):
    for _ in range(n):
        Xodom.velocity(RPM)

def t_spin_volt(n):
    m = Right1
    for _ in range(n):
        m.spin(FORWARD, 0, VOLT)

def t_stop_hold(n):
    m = Right1
    for _ in range(n):
        m.stop(HOLD)

def t_set_torque(n):
    m = Right1
    for _ in range(n):
        m.set_max_torque(2.5, CurrentUnits.AMP)

def t_timer(n):
    for _ in range(n):
        timer.time()

def t_print(n):
    for _ in range(n):
        print(0)

def t_asap_io(n):              # one ASAP loop's firmware calls, no math
    for _ in range(n):
        for m in MOTORS:
            m.velocity(RPM)
        for m in MOTORS:
            m.temperature(PERCENT)
        inertial.heading()
        inertial.acceleration(XAXIS)
        Xodom.velocity(RPM)
        for m in MOTORS:
            m.spin(FORWARD, 0, VOLT)



# --- pure Python VM speed (no devices) ------------------------------------
class Box:
    def __init__(self, v):
        self.v = v
    def get(self):
        return self.v
    def __mul__(self, o):
        return Box([[self.v[0][0] * o.v[0][0]]])

def f(a):
    return a

def t_call(n):
    for _ in range(n):
        f(1)

def t_method(n):
    b = Box(1)
    for _ in range(n):
        b.get()

def t_attr(n):
    b = Box(1)
    for _ in range(n):
        b.v

def t_dict(n):
    d = {"a": 1}
    for _ in range(n):
        d["a"]

def t_matrix(n):              # like ASAP's 1x1 Matrix Kalman ops
    a = Box([[1.5]])
    c = Box([[0.5]])
    for _ in range(n):
        a * c

def t_global(n):
    for _ in range(n):
        N

print("==== 0. Pure Python VM (no devices) ====")
bench("function call", t_call, 10000)
bench("method call", t_method, 10000)
bench("attribute read", t_attr, 10000)
bench("dict lookup", t_dict, 10000)
bench("global read", t_global, 10000)
bench("1x1 matrix multiply", t_matrix, 5000)
print("")
print("==== 1. Call cost (robot idle, one thread) ====")
results = {}
results["empty"] = bench("empty loop", t_empty, 10000)
results["math"] = bench("float math", t_float_math, 10000)
results["alloc"] = bench("[[1.0]] alloc", t_list_alloc, 10000)
results["vel"] = bench("motor.velocity(RPM)", t_velocity)
results["temp"] = bench("motor.temperature(PERCENT)", t_temperature)
results["head"] = bench("inertial.heading()", t_heading)
results["acc"] = bench("inertial.acceleration()", t_accel)
results["rot"] = bench("rotation.velocity(RPM)", t_rot_velocity)
results["spin"] = bench("motor.spin(0, VOLT)", t_spin_volt)
results["stop"] = bench("motor.stop(HOLD)", t_stop_hold)
results["torque"] = bench("set_max_torque(AMP)", t_set_torque)
results["timer"] = bench("timer.time()", t_timer)
results["asap"] = bench("full ASAP I/O per loop", t_asap_io, 500)
results["print"] = bench("print(0)", t_print, 200)
for m in MOTORS:
    m.stop(COAST)

print("")
print("==== 2. wait() accuracy ====")
for ms in (1, 5, 10):
    start = timer.time(MSEC)
    for _ in range(100):
        wait(ms, MSEC)
    total = timer.time(MSEC) - start
    print("wait(%d, MSEC) x100: %d ms (ideal %d)" % (ms, total, ms * 100))

print("")
print("==== 3. GIL contention: same ASAP I/O with a second thread ====")
busy = [True]
def polite_thread():
    while busy[0]:
        wait(5, MSEC)   # like a normal sensor / screen thread
t = Thread(polite_thread)
wait(20, MSEC)
results["asap_polite"] = bench("ASAP I/O + waiting thread", t_asap_io, 500)
busy[0] = False
wait(30, MSEC)

print("")
print("==== 4. Motor temperatures and torque limit ====")
for i, m in enumerate(MOTORS):
    print("motor %d: %d C = %d%%  (ASAP limits torque at >= 50%% = 45 C)" % (i, m.temperature(CELSIUS), m.temperature(PERCENT)))

print("")
print("==== 5. Drive power (PUT THE ROBOT ON A STAND) ====")
print("Press A on the controller to run every motor at 12 V for 2 s.")
while not controller_1.buttonA.pressing():
    wait(20, MSEC)
for m in MOTORS:
    m.set_max_torque(2.5, CurrentUnits.AMP)   # full current, undo any ASAP limit
peak = [0.0, 0.0, 0.0, 0.0]
start = timer.time(MSEC)
while timer.time(MSEC) - start < 2000:
    for i, m in enumerate(MOTORS):
        m.spin(FORWARD, 12, VOLT)
        v = abs(m.velocity(RPM))
        if v > peak[i]:
            peak[i] = v
    wait(10, MSEC)
for m in MOTORS:
    m.stop(COAST)
for i in range(4):
    print("motor %d peak: %d RPM (blue free speed is about 600)" % (i, peak[i]))

print("")
print("==== DONE. Copy everything above to Hark. ====")
brain.screen.print("Benchmark done")
while True:
    wait(100, MSEC)
