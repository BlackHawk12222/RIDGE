#!/usr/bin/env python3
"""V5Sim runner: boots a Custom_python build on an emulated V5 brain (QEMU Zynq Cortex-A9).

  python3 V5Sim/run_sim.py                          # run Custom_python/bin/*.package.bin as built
  python3 V5Sim/run_sim.py --script V5Sim/tests/bench.py   # rebuild hot package with this main.py, run it
  python3 V5Sim/run_sim.py --robot V5Sim/robot.json --ms 10000

What it does
  * builds V5Sim/emu/v5emu.elf (our fake VEXos) if needed
  * optionally copies Custom_python to a temp dir, swaps py/main.py, runs `make` (hot package only)
  * packs the robot config (ports, gearsets, controller script, competition timeline, CPU speed)
  * runs qemu-system-arm -M xilinx-zynq-a9 with -icount so guest time = instructions executed
  * decodes PROS's COBS serial stream and prints the program's stdout

Needs: qemu-system-arm, arm-none-eabi-gcc, and the PROS toolchain for --script rebuilds.
"""
import argparse, json, os, shutil, struct, subprocess, sys, tempfile, time

HERE = os.path.dirname(os.path.abspath(__file__))
RIDGE = os.path.dirname(HERE)
CP = os.path.join(RIDGE, "Custom_python")
EMU = os.path.join(HERE, "emu")

TYPES = {"none": 0, "motor": 2, "rotation": 4, "imu": 6, "inertial": 6, "distance": 7,
         "adi": 12, "optical": 16, "gps": 20}
GEARS = {"36": 0, "red": 0, "100": 0, "18": 1, "green": 1, "200": 1, "6": 2, "blue": 2, "600": 2}
CTRL_INDEX = {"axis4": 0, "leftx": 0, "axis3": 1, "lefty": 1, "axis1": 2, "rightx": 2, "axis2": 3, "righty": 3,
              "l1": 6, "l2": 7, "r1": 8, "r2": 9, "up": 10, "down": 11, "left": 12, "right": 13,
              "x": 14, "b": 15, "y": 16, "a": 17}
# PROS competition status bits: 1 disabled, 2 autonomous, 4 competition switch, 8 field control
COMP = {"driver": 0, "auton": 2 | 4, "disabled": 1 | 4, "driver_comp": 4}


def build_emu():
    elf = os.path.join(EMU, "v5emu.elf")
    srcs = [os.path.join(EMU, f) for f in ("start.S", "vexos.c", "link.ld", "Makefile")]
    if not os.path.exists(elf) or any(os.path.getmtime(s) > os.path.getmtime(elf) for s in srcs):
        subprocess.check_call(["make", "-s", "-C", EMU])
    return elf


def pack_config(robot, mips, bg_us, api_us, run_ms, sd, profile_us=0):
    port_type = [0] * 32
    gear = [1] * 32
    for p, spec in robot.get("ports", {}).items():
        i = int(p) - 1
        if isinstance(spec, str):
            spec = {"type": spec}
        port_type[i] = TYPES[spec["type"].lower()]
        if "gear" in spec:
            gear[i] = GEARS[str(spec["gear"]).lower().replace(":1", "")]
    port_type[21] = 12  # brain's built-in 3-wire ports (PROS "port 22")
    flags = (1 if sd else 0) | (2 if os.environ.get("V5SIM_TRACE") else 0) | (4 if profile_us else 0)
    blob = struct.pack("<7I", 0x4D533556, mips, bg_us, run_ms, flags, int(api_us * 100), profile_us)
    blob += bytes(port_type) + bytes(gear)
    ctrl = []
    for ev in robot.get("controller", []):
        idx = ev["input"].lower()
        ctrl.append((int(ev["t"]), 1 if ev.get("partner") else 0, CTRL_INDEX[idx], int(ev["value"])))
    # Brain screen taps: {"t": ms, "x": px, "y": px, "ms": how long to hold (default 150)}
    for ev in robot.get("touch", []):
        t = int(ev["t"])
        ctrl.append((t, 2, 1, int(ev["x"])))
        ctrl.append((t, 2, 2, int(ev["y"])))
        ctrl.append((t, 2, 0, 1))
        ctrl.append((t + int(ev.get("ms", 150)), 2, 0, 0))
    ctrl.sort()
    blob += struct.pack("<I", len(ctrl))
    for t, c, i, v in ctrl[:256]:
        blob += struct.pack("<IBBHi", t, c, i, 0, v)
    blob += b"\0" * (12 * (256 - min(len(ctrl), 256)))
    comp = sorted((int(e["t"]), COMP[e["mode"]]) for e in robot.get("competition", [{"t": 0, "mode": "driver"}]))
    blob += struct.pack("<I", len(comp))
    for t, s in comp[:32]:
        blob += struct.pack("<II", t, s)
    blob += b"\0" * (8 * (32 - min(len(comp), 32)))
    return blob


def build_firmware(script, src_dir, harness=False):
    """Copy Custom_python, swap in the script as py/main.py, and rebuild (main.cpp + link only)."""
    tmp = tempfile.mkdtemp(prefix="v5sim_build_")
    dst = os.path.join(tmp, "Custom_python")
    shutil.copytree(src_dir, dst, ignore=shutil.ignore_patterns("bin", ".d"))
    for f in os.listdir(os.path.join(dst, "py")):
        if f.endswith(".py"):
            os.remove(os.path.join(dst, "py", f))
    text = open(script).read()
    if harness:
        # put the timing harness right after the program's `from vex import *`
        h = open(os.path.join(HERE, "tests", "harness.py")).read()
        key = "from vex import *"
        i = text.find(key)
        if i < 0:
            sys.exit("--harness needs a `from vex import *` line in the script")
        j = text.find("\n", i) + 1
        text = text[:j] + h + text[j:]
    with open(os.path.join(dst, "py", "main.py"), "w") as f:
        f.write(text)
    env = dict(os.environ)
    r = subprocess.run(["make", "-j4"], cwd=dst, env=env, capture_output=True, text=True)
    if r.returncode != 0:
        sys.stderr.write(r.stdout[-4000:] + r.stderr[-4000:])
        sys.exit("firmware build failed")
    return os.path.join(dst, "bin")


def cobs_decode(b):
    out, i = bytearray(), 0
    while i < len(b):
        code = b[i]
        if code == 0:
            break
        out += b[i + 1:i + code]
        i += code
        if code < 0xFF and i < len(b):
            out.append(0)
    return bytes(out)


class Decoder:
    """Splits the UART stream into simulator messages (0x01..0x02) and PROS COBS frames (..0x00)."""
    def __init__(self, out, raw=False):
        self.buf, self.sim, self.in_sim, self.out, self.raw = bytearray(), bytearray(), False, out, raw
        self.text = []

    def feed(self, data):
        for c in data:
            if self.in_sim:
                if c == 2:
                    self.emit("[sim] " + self.sim.decode(errors="replace") + "\n")
                    self.sim.clear(); self.in_sim = False
                else:
                    self.sim.append(c)
            elif c == 1 and not self.buf:
                self.in_sim = True
            elif c == 0:
                frame = cobs_decode(bytes(self.buf)); self.buf.clear()
                if frame[:4] in (b"sout", b"serr"):
                    self.emit(frame[4:].decode(errors="replace"))
                elif frame:
                    self.emit(frame.decode(errors="replace"))
            else:
                self.buf.append(c)

    def emit(self, s):
        self.text.append(s)
        self.out.write(s); self.out.flush()


def load_syms(elf):
    import bisect
    out = subprocess.run(["arm-none-eabi-nm", "-n", "-C", "--defined-only", elf], capture_output=True, text=True).stdout
    addrs, names = [], []
    for line in out.splitlines():
        parts = line.split(None, 2)
        if len(parts) == 3 and parts[1] in "tTwW":
            addrs.append(int(parts[0], 16) & ~1); names.append(parts[2])
    return addrs, names


def report_profile(work, bindir, shim_elf, top):
    import bisect
    info = os.path.join(work, "prof_info.bin")
    if not os.path.exists(info):
        print("[v5sim] no profile written"); return
    samples, other, irqmode, period = struct.unpack("<4I", open(info, "rb").read()[:16])
    regions = [("prof_cold.bin", 0x03800000, os.path.join(bindir, "cold.package.elf")),
               ("prof_hot.bin", 0x07800000, os.path.join(bindir, "hot.package.elf")),
               ("prof_shim.bin", 0x00100000, shim_elf)]
    by_fn = {}
    for fname, base, elf in regions:
        data = open(os.path.join(work, fname), "rb").read()
        counts = struct.unpack("<%dI" % (len(data) // 4), data)
        addrs, names = load_syms(elf) if os.path.exists(elf) else ([], [])
        for i, c in enumerate(counts):
            if not c:
                continue
            pc = base + 4 * i
            k = bisect.bisect_right(addrs, pc) - 1
            nm = names[k] if k >= 0 else hex(pc)
            if fname == "prof_shim.bin":
                nm = "[VEXos sim] " + nm
            by_fn[nm] = by_fn.get(nm, 0) + c
    print(f"\n==== profile: {samples} samples every {period} us of brain time "
          f"({100.0 * irqmode / max(samples, 1):.1f}% in interrupt/kernel modes) ====")
    for nm, c in sorted(by_fn.items(), key=lambda kv: -kv[1])[:top]:
        print(f"{100.0 * c / max(samples, 1):6.2f}%  {c:7d}  {nm}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--script", help="Python file to run as main.py (rebuilds the hot package)")
    ap.add_argument("--bin", help="folder with cold.package.bin/hot.package.bin (default Custom_python/bin)")
    ap.add_argument("--src", default=CP, help="Custom_python folder to build from (default: this repo's)")
    ap.add_argument("--robot", default=os.path.join(HERE, "robot.json"), help="robot config JSON")
    ap.add_argument("--ms", type=int, default=0, help="stop after this much brain time (default from robot.json or 20000)")
    ap.add_argument("--mips", type=int, default=0, help="brain speed, instructions per microsecond (default robot.json or 400)")
    ap.add_argument("--bg-us", type=int, default=-1, help="cost of each VEXos background call in us (default 20)")
    ap.add_argument("--api-us", type=float, default=-1, help="cost of each device API call in us (default 0.5)")
    ap.add_argument("--sd", help="host folder used as the SD card (/usd)")
    ap.add_argument("--harness", action="store_true", help="inject tests/harness.py to time every thread's loop (see tests/ridge_report.py)")
    ap.add_argument("--profile", action="store_true", help="sample the PC every ~97 us and print the hottest functions")
    ap.add_argument("--top", type=int, default=40, help="functions to show with --profile")
    ap.add_argument("--quiet", action="store_true")
    a = ap.parse_args()

    robot = json.load(open(a.robot)) if a.robot and os.path.exists(a.robot) else {}
    sim = robot.get("sim", {})
    mips = a.mips or sim.get("mips", 400)
    bg_us = a.bg_us if a.bg_us >= 0 else sim.get("background_us", 20)
    api_us = a.api_us if a.api_us >= 0 else sim.get("api_call_us", 0.5)
    run_ms = a.ms or sim.get("run_ms", 20000)

    elf = build_emu()
    bindir = a.bin or (build_firmware(a.script, a.src, a.harness) if a.script else os.path.join(a.src, "bin"))
    cold, hot = os.path.join(bindir, "cold.package.bin"), os.path.join(bindir, "hot.package.bin")
    for f in (cold, hot):
        if not os.path.exists(f):
            sys.exit(f"missing {f}: run `make` in Custom_python first (or pass --script)")

    work = tempfile.mkdtemp(prefix="v5sim_run_")
    sd_dir = os.path.join(work, "sd")
    if a.sd:
        shutil.copytree(a.sd, sd_dir)
    cfg = os.path.join(work, "cfg.bin")
    open(cfg, "wb").write(pack_config(robot, mips, bg_us, api_us, run_ms, bool(a.sd), 97 if a.profile else 0))

    cmd = ["qemu-system-arm", "-M", "xilinx-zynq-a9", "-m", "1G", "-nographic", "-monitor", "none",
           "-serial", "stdio", "-semihosting-config", "enable=on,target=native",
           "-icount", "shift=0,sleep=off",
           "-kernel", elf,
           "-device", f"loader,file={cold},addr=0x03800000,force-raw=on",
           "-device", f"loader,file={hot},addr=0x07800000,force-raw=on",
           "-device", f"loader,file={cfg},addr=0x00F00000,force-raw=on"]
    dec = Decoder(sys.stdout)
    t0 = time.time()
    p = subprocess.Popen(cmd, cwd=work, stdout=subprocess.PIPE, stdin=subprocess.DEVNULL)
    try:
        while True:
            chunk = p.stdout.read1(4096)
            if not chunk:
                break
            dec.feed(chunk)
    except KeyboardInterrupt:
        p.kill()
    p.wait()
    if not a.quiet:
        sys.stderr.write(f"\n[v5sim] {run_ms} ms of brain time in {time.time() - t0:.1f} s host time "
                         f"(brain speed {mips} MIPS)\n")
    if a.profile:
        report_profile(work, bindir, elf, a.top)
    shutil.rmtree(work, ignore_errors=True)
    return p.returncode


if __name__ == "__main__":
    sys.exit(main())
