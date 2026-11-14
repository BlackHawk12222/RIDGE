# V5Sim benchmark. Runs unchanged on official VEXcode V5 Python, on Custom_python
# on the brain, and on the V5Sim emulator. Each line prints one result:
#   name  value  unit
# Lower us/op is better; higher Hz / runs/s is better.
from vex import *

brain = Brain()
timer = Timer()
m1 = Motor(Ports.PORT1, GearSetting.RATIO_18_1, False)
m2 = Motor(Ports.PORT2, GearSetting.RATIO_18_1, True)
imu = Inertial(Ports.PORT7)
controller = Controller(PRIMARY)
try:
    import gc
except ImportError:
    gc = None

RESULTS = []


def report(name, value, unit):
    RESULTS.append((name, value, unit))
    print("%-30s %9.2f %s" % (name, value, unit))


def per_op(name, fn, n):
    if gc:
        gc.collect()
    t0 = timer.time(MSEC)
    fn(n)
    dt = timer.time(MSEC) - t0
    report(name, dt * 1000.0 / n, "us/op")


# ---------------------------------------------------------------- pure VM
def t_empty(n):
    for _ in range(n):
        pass

def t_while(n):
    i = 0
    while i < n:
        i += 1

def t_int_math(n):
    x = 0
    for i in range(n):
        x = (x + i * 3) & 0xFFFF

def t_float_math(n):
    x = 1.5
    for _ in range(n):
        x = x * 1.0001 + 0.5 - 0.5

def f0():
    return 1

def f3(a, b, c):
    return a

def t_call0(n):
    for _ in range(n):
        f0()

def t_call3(n):
    for _ in range(n):
        f3(1, 2, 3)

class P:
    def __init__(self):
        self.x = 1.0
        self.y = 2.0

    def m(self):
        return self.x

def t_method(n):
    p = P()
    for _ in range(n):
        p.m()

def t_attr(n):
    p = P()
    for _ in range(n):
        p.x

def t_attr_store(n):
    p = P()
    for i in range(n):
        p.x = i

G = 5
def t_global(n):
    for _ in range(n):
        G

def t_list(n):
    for _ in range(n):
        a = [1, 2, 3]

def t_list_append(n):
    a = []
    for i in range(n):
        a.append(i)

def t_dict(n):
    d = {"kp": 1.0, "ki": 0.1, "kd": 0.01}
    for _ in range(n):
        d["kp"]

def t_str_format(n):
    for i in range(n):
        s = "%d:%.2f" % (i, 1.5)

def t_try(n):
    for _ in range(n):
        try:
            pass
        except Exception:
            pass

def t_obj_alloc(n):
    for _ in range(n):
        P()

def t_math_fn(n):
    import math
    for i in range(n):
        math.sin(i * 0.01)

def pid_step(state, err, dt):
    state[0] += err * dt
    d = (err - state[1]) / dt
    state[1] = err
    return 1.2 * err + 0.05 * state[0] + 0.01 * d

def t_pid(n):
    s = [0.0, 0.0]
    for i in range(n):
        pid_step(s, 10.0 - i * 0.001, 0.01)

def fib(n):
    return n if n < 2 else fib(n - 1) + fib(n - 2)

def t_fib(n):
    for _ in range(n):
        fib(15)

# ------------------------------------------------------------- device calls
def t_motor_vel(n):
    for _ in range(n):
        m1.velocity(RPM)

def t_motor_pos(n):
    for _ in range(n):
        m1.position(DEGREES)

def t_motor_spin(n):
    for _ in range(n):
        m1.spin(FORWARD, 50, PERCENT)

def t_motor_volt(n):
    for _ in range(n):
        m1.spin(FORWARD, 6, VOLT)

def t_imu(n):
    for _ in range(n):
        imu.heading()

def t_axis(n):
    for _ in range(n):
        controller.axis3.position()

def t_button(n):
    for _ in range(n):
        controller.buttonR1.pressing()

def t_timer(n):
    for _ in range(n):
        timer.time(MSEC)

def t_drive_loop(n):
    for _ in range(n):
        f = controller.axis3.position()
        t = controller.axis1.position()
        m1.spin(FORWARD, f + t, PERCENT)
        m2.spin(FORWARD, f - t, PERCENT)
        m1.velocity(RPM)
        imu.heading()

wait(300, MSEC)
print("==== V5Sim bench ====")
per_op("empty for-loop", t_empty, 20000)
per_op("while i<n: i+=1", t_while, 20000)
per_op("int math", t_int_math, 20000)
per_op("float math", t_float_math, 20000)
per_op("call f()", t_call0, 20000)
per_op("call f(a,b,c)", t_call3, 20000)
per_op("method call", t_method, 20000)
per_op("attribute read", t_attr, 20000)
per_op("attribute store", t_attr_store, 20000)
per_op("global read", t_global, 20000)
per_op("list [1,2,3]", t_list, 10000)
per_op("list.append", t_list_append, 10000)
per_op("dict lookup", t_dict, 20000)
per_op("str % format", t_str_format, 5000)
per_op("try/except (no raise)", t_try, 20000)
per_op("object alloc", t_obj_alloc, 10000)
per_op("math.sin", t_math_fn, 10000)
per_op("PID step", t_pid, 10000)
per_op("fib(15), 1973 calls", t_fib, 20)
per_op("motor.velocity()", t_motor_vel, 5000)
per_op("motor.position()", t_motor_pos, 5000)
per_op("motor.spin(PERCENT)", t_motor_spin, 5000)
per_op("motor.spin(VOLT)", t_motor_volt, 5000)
per_op("inertial.heading()", t_imu, 5000)
per_op("axis3.position()", t_axis, 5000)
per_op("buttonR1.pressing()", t_button, 5000)
per_op("timer.time()", t_timer, 5000)
per_op("drive loop (6 calls)", t_drive_loop, 2000)

# GC pause with ~1500 live objects
if gc:
    keep = [P() for _ in range(1500)]
    t0 = timer.time(MSEC)
    for _ in range(10):
        gc.collect()
    report("gc.collect (1500 objs)", (timer.time(MSEC) - t0) / 10.0, "ms")
    keep = None

# wait() accuracy
t0 = timer.time(MSEC)
for _ in range(100):
    wait(5, MSEC)
report("wait(5 ms) x100", timer.time(MSEC) - t0, "ms (ideal 500)")

# ------------------------------------------------------------------ threads
counts = [0, 0, 0]
late = [0]
hog_n = [0]
running = [True]

def make_loop(k):
    def loop():
        last = timer.time(MSEC)
        while running[0]:
            x = 0
            for i in range(30):
                x += i
            counts[k] += 1
            wait(5, MSEC)
            now = timer.time(MSEC)
            if now - last > 8:
                late[0] += 1
            last = now
    return loop

def hog():
    while running[0]:
        hog_n[0] += 1

def run_loops(with_hog):
    counts[0] = counts[1] = counts[2] = 0
    late[0] = 0
    hog_n[0] = 0
    running[0] = True
    ts = [Thread(make_loop(k)) for k in range(3)]
    if with_hog:
        ts.append(Thread(hog))
    wait(1000, MSEC)
    running[0] = False
    wait(50, MSEC)
    for t in ts:
        try:
            t.stop()
        except Exception:
            pass
    wait(20, MSEC)

run_loops(False)
report("3 x 5ms loops", sum(counts) / 3.0, "Hz each (ideal ~200)")
report("  late loops (>8 ms)", late[0], "per s")
run_loops(True)
report("3 x 5ms loops + busy thread", sum(counts) / 3.0, "Hz each (ideal ~200)")
report("  late loops (>8 ms)", late[0], "per s")
report("  busy thread iterations", hog_n[0] / 1000.0, "k per s")

worker = [0]
def worker_fn():
    while running[0]:
        worker[0] += 1
        wait(1, MSEC)

running[0] = True
th = Thread(worker_fn)
wait(20, MSEC)
worker[0] = 0
t0 = timer.time(MSEC)
while timer.time(MSEC) - t0 < 500:
    pass
report("thread while main spins", worker[0] * 2.0, "runs/s (ideal ~1000)")
running[0] = False
wait(20, MSEC)

print("==== done ====")
