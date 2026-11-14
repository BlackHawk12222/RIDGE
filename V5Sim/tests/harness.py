# ---- V5Sim test harness (injected by run_sim.py --harness, right after `from vex import *`) ----
# Wraps wait() and Thread() to measure every thread's loop: how often it wakes, how long it
# works between waits, and how late its waits return. Every second it prints one "[H]" line per
# thread and one "[S]" line with memory, so run_sim.py can build the results table.
import time as _h_time
import gc as _h_gc
import _thread as _h_thread
import sys as _h_sys

_h_wait = wait
_h_Thread = Thread
_h_stats = {}      # thread ident -> [name, waits, work_sum_us, work_max_us, late_sum_us, late_max_us, late_cnt, last_exit_us, errors]
_h_errors = []
_h_mem_min = [1 << 30]


def _h_rec():
    i = _h_thread.get_ident()
    s = _h_stats.get(i)
    if s is None:
        s = ["main" if not _h_stats else "thread%d" % len(_h_stats), 0, 0, 0, 0, 0, 0, 0, 0]
        _h_stats[i] = s
    return s


def wait(t, units=SECONDS):
    s = _h_rec()
    now = _h_time.ticks_us()
    if s[7]:
        work = _h_time.ticks_diff(now, s[7])
        s[2] += work
        if work > s[3]:
            s[3] = work
    ms = t * 1000 if units == SECONDS else t
    _h_wait(t, units)
    after = _h_time.ticks_us()
    late = _h_time.ticks_diff(after, now) - int(ms * 1000)
    s[1] += 1
    if late > 0:
        s[4] += late
        if late > s[5]:
            s[5] = late
        if late > 2000:
            s[6] += 1
    s[7] = after


def sleep(t, units=MSEC):
    wait(t, units)


def Thread(fn, *args):
    name = getattr(fn, "__name__", "?")
    if name == "<lambda>":
        name = "lambda"

    def _h_run():
        s = _h_rec()
        s[0] = name
        try:
            fn(*args)
        except Exception as e:
            s[8] += 1
            _h_errors.append("%s: %s %s" % (name, type(e).__name__, e))
            print("[E]", name, type(e).__name__, e)
            _h_sys.print_exception(e)
    return _h_Thread(_h_run)


def _h_report():
    _h_rec()[0] = "harness"
    sec = 0
    while True:
        # report every 5 s and skip gc.collect(), so the report itself barely disturbs timing
        _h_wait(5000, MSEC)
        sec += 5
        free = _h_gc.mem_free()
        if free < _h_mem_min[0]:
            _h_mem_min[0] = free
        print("[S] t=%d free=%d alloc=%d threads=%d errors=%d" % (sec, free, _h_gc.mem_alloc(), len(_h_stats), len(_h_errors)))
        for s in list(_h_stats.values()):
            if s[0] == "harness":
                continue
            n = s[1] or 1
            print("[H] t=%d %s waits=%d work_avg=%d work_max=%d late_avg=%d late_max=%d late2ms=%d err=%d" % (
                sec, s[0], s[1], s[2] // n, s[3], s[4] // n, s[5], s[6], s[8]))
            s[1] = s[2] = s[3] = s[4] = s[5] = s[6] = 0
        _h_state(sec)


def _h_state(sec=0):
    g = globals()
    parts = []
    for k in ("setUp", "RunAuto", "Loading", "MyColor", "Auto_Side", "PosX", "PosY", "Heading"):
        if k in g:
            v = g[k]
            parts.append("%s=%s" % (k, ("%.1f" % v) if isinstance(v, float) else v))
    for k in ("left_motor_a", "right_motor_a", "liftOne", "Intake", "Claw", "Bar_Switcher"):
        m = g.get(k)
        if m is not None:
            try:
                parts.append("%s=%.0frpm/%.0fdeg" % (k, m.velocity(RPM), m.position(DEGREES)))
            except Exception as e:
                parts.append("%s=?" % k)
    for k in ("grabber", "intakeLift"):
        d = g.get(k)
        if d is not None:
            try:
                parts.append("%s=%d" % (k, d.value()))
            except Exception as e:
                parts.append("%s=err:%s" % (k, e))
    print("[R] t=%d" % sec, " ".join(parts))


_h_Thread(_h_report)
# ---- end harness ----
