# V5Sim

Runs the real Custom_python build (the same `cold.package.bin` / `hot.package.bin` you upload) on an
emulated V5 brain, so speed and threading changes can be tested without a robot.

- **CPU:** QEMU `xilinx-zynq-a9` — the same Zynq-7000 / Cortex-A9 chip family as the V5 brain.
- **Speed cap:** QEMU runs with `-icount`, so brain time = instructions executed. `mips` in
  `robot.json` (default 400 instructions per µs) sets how fast the brain is. Results don't depend on
  how fast the host PC is.
- **VEXos:** `emu/vexos.c` is a small fake VEXos that fills the jump table at `0x037FC000`. PROS and
  Custom_python run unmodified on top of it. Any VEXos call it doesn't implement is logged once
  (`[sim] unimplemented VEXos call ...`) and returns 0.
- **Devices:** motors (rpm model with gearsets), IMU (2 s calibration), rotation, distance, optical,
  3-wire, GPS stub, controller (scripted), competition mode (scripted), SD card (a host folder).

## Setup (Ubuntu / WSL)

    sudo apt install qemu-system-arm gcc-arm-none-eabi python3

Plus the PROS toolchain if you use `--script` (it rebuilds the hot package).

## Use

    python3 V5Sim/run_sim.py                                   # run Custom_python/bin as built
    python3 V5Sim/run_sim.py --script V5Sim/tests/bench.py     # run a different main.py
    python3 V5Sim/run_sim.py --script Custom_python/py/main_ridge.py --ms 15000
    python3 V5Sim/run_sim.py --script my.py --profile          # hottest C functions

Options: `--ms` (brain time to run), `--mips`, `--bg-us` (VEXos background cost per call),
`--api-us` (cost of each device call), `--sd FOLDER`, `--profile`, `--top N`, `--robot FILE`.
Set `V5SIM_TRACE=1` for low-level boot/IRQ tracing.

`robot.json` holds the ports, gearsets, controller timeline (`t` in ms) and competition timeline.

## Benchmark

`tests/bench.py` runs unchanged on VEXcode Python, on Custom_python on the brain, and here.
It prints one line per test (µs per op, Hz, or runs/s).

## Calibrating (do this once)

1. Run `tests/bench.py` on the brain with Custom_python and note `empty for-loop` and `call f()`.
2. Run it here. Scale `mips` in `robot.json` by (sim µs / brain µs) until those two match.
3. If device calls (`motor.velocity()`) are off after that, adjust `api_call_us`.

Until calibrated, treat sim numbers as **relative** (before vs after a change), not absolute.

## Limits

- No cache, memory-bus or branch-predictor timing: every instruction costs the same. Real code
  with lots of cache misses (big gc, large lists) will be slower on the brain than here.
- Only one of the two A9 cores runs user code (same as the real brain).
- VEXos background work, device timing and the screen are approximations.
- Motor physics is a simple first-order model, not real torque/load.

## Memory map used

| Range | Use |
|---|---|
| `0x00100000` | fake VEXos code |
| `0x00F00000` | robot config block |
| `0x01000000–0x01480000` | profiler counters (only with `--profile`) |
| `0x037FC000` | VEXos jump table |
| `0x03800000` | PROS cold package |
| `0x07800000` | hot package (your Python) |
