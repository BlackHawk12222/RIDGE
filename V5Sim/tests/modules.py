# Checks that every module VEXcode V5 Python ships also imports (and basically works)
# in Custom_python. Prints one "MOD <name> PASS|FAIL <detail>" line per check.
from vex import *

def check(name, fn):
    try:
        r = fn()
        print("MOD", name, "PASS", r if r is not None else "")
    except Exception as e:
        print("MOD", name, "FAIL", type(e).__name__, e)

def t_vex():
    import vex
    return "Motor" in dir(vex)

def t_math():
    import math
    assert abs(math.sqrt(2) - 1.41421356) < 1e-6
    return math.pi

def t_cmath():
    import cmath
    return cmath.sqrt(-1)

def t_sys():
    import sys
    return sys.implementation.name

def t_gc():
    import gc
    gc.collect()
    return gc.mem_free()

def t_micropython():
    import micropython
    micropython.mem_info
    return "ok"

def t_time(modname):
    def f():
        m = __import__(modname)
        a = m.ticks_ms(); b = m.ticks_us()
        m.sleep_ms(20)
        d = m.ticks_diff(m.ticks_ms(), a)
        assert 18 <= d <= 40, d
        m.sleep(0.01); m.sleep_us(500)
        return "sleep_ms(20) took %d ms, time()=%s" % (d, m.time())
    return f

def t_random(modname):
    def f():
        m = __import__(modname)
        m.seed(1)
        v = [m.randint(1, 6) for _ in range(5)]
        assert all(1 <= x <= 6 for x in v)
        m.random(); m.uniform(1, 2); m.choice([1, 2, 3]); m.getrandbits(8); m.randrange(10)
        return v
    return f

def t_json(modname):
    def f():
        m = __import__(modname)
        s = m.dumps({"a": [1, 2.5, "x", None, True]})
        assert m.loads(s)["a"][1] == 2.5
        return s
    return f

def t_re(modname):
    def f():
        m = __import__(modname)
        g = m.match(r"(\d+)-(\w+)", "42-abc")
        assert g.group(2) == "abc"
        return m.sub(r"\d", "#", "a1b2")
    return f

def t_binascii(modname):
    def f():
        m = __import__(modname)
        h = m.hexlify(b"\x01\xab")
        assert m.unhexlify(h) == b"\x01\xab"
        assert m.a2b_base64(m.b2a_base64(b"hi")) == b"hi"
        return h, m.crc32(b"hello")
    return f

def t_struct(modname):
    def f():
        m = __import__(modname)
        return m.unpack("<hf", m.pack("<hf", -3, 1.5))
    return f

def t_array(modname):
    def f():
        m = __import__(modname)
        a = m.array("f", [1, 2, 3]); a.append(4)
        return len(a)
    return f

def t_collections(modname):
    def f():
        m = __import__(modname)
        P = m.namedtuple("P", ("x", "y"))
        d = m.OrderedDict(); d["b"] = 1; d["a"] = 2
        q = m.deque((), 4); q.append(1); q.popleft()
        return P(1, 2).y, list(d.keys())
    return f

def t_io(modname):
    def f():
        m = __import__(modname)
        s = m.StringIO(); s.write("abc")
        b = m.BytesIO(b"xy")
        return s.getvalue(), b.read()
    return f

def t_select(modname):
    def f():
        m = __import__(modname)
        return "poll" in dir(m)
    return f

def t_errno(modname):
    def f():
        m = __import__(modname)
        return m.ENOENT
    return f

def t_heapq(modname):
    def f():
        m = __import__(modname)
        h = []; m.heappush(h, 3); m.heappush(h, 1)
        return m.heappop(h)
    return f

def t_hashlib(modname):
    def f():
        m = __import__(modname)
        return m.sha256(b"a").digest()[:2]
    return f

def t_asyncio(modname):
    def f():
        m = __import__(modname)
        out = []
        async def worker(n):
            for i in range(3):
                out.append(n)
                await m.sleep_ms(10)
        async def main():
            await m.gather(worker(1), worker(2))
        t0 = Timer(); t0.reset()
        m.run(main())
        return out, "%d ms" % t0.time()
    return f

def t_thread():
    import _thread
    lock = _thread.allocate_lock()
    with lock:
        pass
    return "ok"

check("vex", t_vex)
check("math", t_math)
check("cmath", t_cmath)
check("sys", t_sys)
check("gc", t_gc)
check("micropython", t_micropython)
for n in ("time", "utime"): check(n, t_time(n))
for n in ("random", "urandom"): check(n, t_random(n))
for n in ("json", "ujson"): check(n, t_json(n))
for n in ("re", "ure"): check(n, t_re(n))
for n in ("binascii", "ubinascii"): check(n, t_binascii(n))
for n in ("struct", "ustruct"): check(n, t_struct(n))
for n in ("array", "uarray"): check(n, t_array(n))
for n in ("collections", "ucollections"): check(n, t_collections(n))
for n in ("io", "uio"): check(n, t_io(n))
for n in ("select", "uselect"): check(n, t_select(n))
for n in ("errno", "uerrno"): check(n, t_errno(n))
for n in ("heapq", "uheapq"): check(n, t_heapq(n))
for n in ("hashlib", "uhashlib"): check(n, t_hashlib(n))
for n in ("asyncio", "uasyncio"): check(n, t_asyncio(n))
check("_thread", t_thread)
print("MOD done")
