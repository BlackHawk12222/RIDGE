/*
 * V5Sim fake VEXos.
 *
 * Runs the real PROS program (cold.package.bin + hot.package.bin, built by `make`)
 * on QEMU's Zynq-7000 Cortex-A9 machine. Every VEXos call PROS makes goes through
 * the jump table at 0x037FC000, so we fill that table with our own functions:
 * timers + interrupts, serial (to the QEMU UART), SD card (to a host folder via
 * semihosting), battery, competition, a scripted controller, and simple physics
 * models of motors, the IMU, rotation, distance and optical sensors.
 *
 * Speed: QEMU runs with -icount, so guest time = instructions executed. The runner
 * tells us the brain's effective speed (MIPS = instructions per microsecond) and we
 * scale the 1 ms FreeRTOS tick and every clock read to it. That pins Python/PROS
 * code to Cortex-A9 speed regardless of how fast the host is.
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "jt_names.h"

typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int32_t i32; typedef uint64_t u64;

/* ---------------------------------------------------------------- hardware */
#define REG(a) (*(volatile u32 *)(a))
#define UART0      0xE0000000u
#define GT_LO      0xF8F00200u
#define GT_HI      0xF8F00204u
#define GT_CTRL    0xF8F00208u
#define PT_LOAD    0xF8F00600u
#define PT_COUNT   0xF8F00604u
#define PT_CTRL    0xF8F00608u
#define PT_ISR     0xF8F0060Cu
#define GICC       0xF8F00100u
#define GICD       0xF8F01000u
#define IRQ_PTIMER 29

static void uart_init(void) {
    REG(UART0 + 0x00) = (1u << 4) | (1u << 2) | 3; /* TXEN | RXEN | reset fifos */
    REG(UART0 + 0x04) = 0x20;                       /* 8N1 */
}
static void uart_putc(char c) {
    while (REG(UART0 + 0x2C) & (1u << 4)) { }       /* TX FIFO full */
    REG(UART0 + 0x30) = (u8)c;
}
static void uart_puts(const char *s) { while (*s) uart_putc(*s++); }
static void uart_hex(u32 v) { const char *h = "0123456789abcdef"; uart_puts("0x"); for (int i = 28; i >= 0; i -= 4) uart_putc(h[(v >> i) & 15]); }
static void uart_dec(u32 v) { char b[12]; int n = 0; do { b[n++] = '0' + v % 10; v /= 10; } while (v); while (n) uart_putc(b[--n]); }
/* Simulator messages are framed so the runner can tell them from PROS's COBS stream. */
static void simlog(const char *s) { uart_putc(0x01); uart_puts(s); uart_putc(0x02); }

/* ------------------------------------------------------------- semihosting */
static inline int semihost(int op, void *arg) {
    register int r0 __asm__("r0") = op;
    register void *r1 __asm__("r1") = arg;
    __asm__ volatile(".inst 0xe10f0070" : "+r"(r0) : "r"(r1) : "memory"); /* HLT #0xF000 */
    return r0;
}
struct sh_open { const char *name; u32 mode; u32 len; };
static void prof_dump(void);
static void sim_exit(int code) {
    prof_dump();
    u32 a[2] = { 0x20026, (u32)code };
    semihost(0x20, a); /* SYS_EXIT_EXTENDED */
    semihost(0x18, (void *)0x20026);
    for (;;) { }
}

/* --------------------------------------------------------------- MMU (flat) */
__attribute__((section(".bss.ttb"), aligned(16384))) static u32 ttb[4096];
void mmu_init(void) {
    for (u32 i = 0; i < 4096; i++) {
        u32 base = i << 20;
        if (i < 0x400) /* DDR 0..1 GB: normal, write-back, shareable, full access */
            ttb[i] = base | (1u << 16) | (3u << 10) | (1u << 12) | (1u << 3) | (1u << 2) | 2;
        else           /* peripherals: device memory, execute-never */
            ttb[i] = base | (3u << 10) | (1u << 4) | (1u << 2) | 2;
    }
    __asm__ volatile(
        "mcr p15, 0, %0, c2, c0, 0\n"   /* TTBR0 */
        "mov r1, #0\n mcr p15, 0, r1, c2, c0, 2\n" /* TTBCR = 0 */
        "mvn r1, #0\n mcr p15, 0, r1, c3, c0, 0\n" /* DACR: manager */
        "mcr p15, 0, r1, c8, c7, 0\n"   /* TLB invalidate */
        "dsb\n isb\n"
        "mrc p15, 0, r1, c1, c0, 0\n"
        "orr r1, r1, #(1<<0)\n"         /* M  */
        "orr r1, r1, #(1<<2)\n"         /* C  */
        "orr r1, r1, #(1<<12)\n"        /* I  */
        "orr r1, r1, #(1<<11)\n"        /* Z  */
        "bic r1, r1, #(1<<1)\n"         /* A off: unaligned allowed, like VEXos */
        "mcr p15, 0, r1, c1, c0, 0\n"
        "dsb\n isb\n" :: "r"(ttb) : "r1", "memory");
}

/* ------------------------------------------------------------------ config */
#define CFG_ADDR 0x00F00000u
#define CFG_MAGIC 0x4D533556u /* "V5SM" */
#define MAX_PORTS 32
struct ctrl_ev { u32 t_ms; u8 ctrl; u8 index; u16 pad; i32 value; };
struct comp_ev { u32 t_ms; u32 status; };
struct simcfg {
    u32 magic, mips, bg_us, run_ms, flags, api_us_x100, prof_us;
    u8 port_type[MAX_PORTS];
    u8 motor_gear[MAX_PORTS];
    u32 n_ctrl; struct ctrl_ev ctrl[256];
    u32 n_comp; struct comp_ev comp[32];
};
#define CFG_SD      (1u << 0)
#define CFG_TRACE   (1u << 1)
#define CFG_PROFILE (1u << 2)
static struct simcfg *cfg = (struct simcfg *)CFG_ADDR;

/* ------------------------------------------------------------------- clock */
static u32 mips = 400;             /* brain instructions per microsecond */
static double gt_per_insn, pt_per_insn;
static double gt_per_us;           /* global-timer ticks per brain microsecond */
static u64 gt_start;

static inline u64 gt_read(void) {
    u32 hi, lo, hi2;
    do { hi = REG(GT_HI); lo = REG(GT_LO); hi2 = REG(GT_HI); } while (hi != hi2);
    return ((u64)hi << 32) | lo;
}
static void spin(u32 iters) { /* 2 instructions per iteration */
    __asm__ volatile("1: subs %0, %0, #1\n bne 1b" : "+r"(iters) :: "cc");
}
static void calibrate(void) {
    REG(GT_CTRL) = 1;
    REG(PT_CTRL) = 0; REG(PT_LOAD) = 0xFFFFFFFF; REG(PT_CTRL) = 1;
    const u32 n = 2000000;
    u64 g0 = gt_read(); u32 p0 = REG(PT_COUNT);
    spin(n);
    u64 g1 = gt_read(); u32 p1 = REG(PT_COUNT);
    REG(PT_CTRL) = 0;
    gt_per_insn = (double)(g1 - g0) / (2.0 * n);
    pt_per_insn = (double)(p0 - p1) / (2.0 * n);
    gt_per_us = gt_per_insn * mips;
}
static inline u64 now_us(void) { return (u64)((double)(gt_read() - gt_start) / gt_per_us); }
static inline u32 now_ms(void) { return (u32)(now_us() / 1000); }
static void burn_us(double us) { u32 it = (u32)(us * mips / 2.0); if (it) spin(it); }
/* Small cost of crossing into VEXos for a device call (shared-memory read on the brain). */
static inline void api_cost(void) { if (cfg->api_us_x100) burn_us(cfg->api_us_x100 / 100.0); }

/* --------------------------------------------------------------- interrupts */
static void (*tick_handler)(void *);
static void prof_start(void);
static u32 tick_load;

static void gic_init(void) {
    REG(GICD + 0x000) = 0;            /* disable distributor while configuring */
    for (int i = 0; i < 8; i++) REG(GICD + 0x180 + 4 * i) = 0xFFFFFFFF; /* disable all */
    for (int i = 0; i < 64; i++) REG(GICD + 0x400 + 4 * i) = 0xA0A0A0A0;
    REG(GICD + 0x000) = 1;
    REG(GICC + 0x004) = 0xFF;         /* PMR: allow all */
    REG(GICC + 0x008) = 0;            /* BPR */
    REG(GICC + 0x000) = 1;
}
static void vexSystemTimerStop(void) { REG(PT_CTRL) = 0; REG(PT_ISR) = 1; }
static void vexSystemTimerClearInterrupt(void) { REG(PT_ISR) = 1; }
static i32 vexSystemTimerReinitForRtos(u32 priority, void (*handler)(void *)) {
    tick_handler = handler;
    volatile u8 *pri = (volatile u8 *)(GICD + 0x400);
    pri[IRQ_PTIMER] = (u8)priority;
    REG(GICD + 0x100) = 1u << IRQ_PTIMER;            /* enable PPI 29 */
    tick_load = (u32)(pt_per_insn * mips * 1000.0 + 0.5) - 1;  /* 1 brain-ms */
    REG(PT_CTRL) = 0; REG(PT_ISR) = 1;
    REG(PT_LOAD) = tick_load;
    REG(PT_CTRL) = 1 | 2 | 4;                        /* enable, auto-reload, IRQ */
    static bool prof_on; if (!prof_on) { prof_on = true; prof_start(); } /* PROS's vectors are live now */
    if (cfg->flags & CFG_TRACE) { uart_putc(1); uart_puts("tick timer load "); uart_dec(tick_load); uart_puts(" prio "); uart_hex(priority); uart_putc(2); }
    return 0;
}
static void check_run_limit(void);

/* ---- sampling profiler: global-timer comparator (PPI 27) at a non-harmonic rate.
   FreeRTOS_IRQ_Handler pushes the interrupted PC first on the IRQ stack, so the word
   just below __irq_stack_top is the PC we interrupted. Histograms are dumped at exit. */
extern u32 __irq_stack_top[];
#define IRQ_GTIMER 27
#define PROF_COLD ((u32 *)0x01000000)  /* 0x03800000..0x03C00000, one counter per word */
#define PROF_HOT  ((u32 *)0x01400000)  /* 0x07800000..0x07840000 */
#define PROF_SHIM ((u32 *)0x01440000)  /* 0x00100000..0x00140000 (fake VEXos itself) */
static u32 prof_samples, prof_other, prof_irqmode;
static u32 prof_inc;
static void prof_start(void) {
    if (!(cfg->flags & CFG_PROFILE)) return;
    memset(PROF_COLD, 0, 0x400000); memset(PROF_HOT, 0, 0x40000); memset(PROF_SHIM, 0, 0x40000);
    u32 us = cfg->prof_us ? cfg->prof_us : 97;
    prof_inc = (u32)(gt_per_us * us);
    volatile u8 *pri = (volatile u8 *)(GICD + 0x400);
    pri[IRQ_GTIMER] = 0x00;                       /* above FreeRTOS's mask: samples critical sections too */
    REG(GICD + 0x100) = 1u << IRQ_GTIMER;
    u64 c = gt_read() + prof_inc;
    REG(GT_CTRL) = 1;
    REG(0xF8F00210) = (u32)c; REG(0xF8F00214) = (u32)(c >> 32);
    REG(0xF8F00218) = prof_inc;
    REG(GT_CTRL) = 1 | 2 | 4 | 8;                 /* timer, comparator, IRQ, auto-increment */
}
static void prof_sample(void) {
    REG(0xF8F0020C) = 1;                          /* clear event */
    u32 pc = __irq_stack_top[-1];
    u32 spsr = __irq_stack_top[-2];
    prof_samples++;
    if ((spsr & 0x1f) != 0x1f) prof_irqmode++;
    if (pc >= 0x03800000 && pc < 0x03C00000) PROF_COLD[(pc - 0x03800000) >> 2]++;
    else if (pc >= 0x07800000 && pc < 0x07840000) PROF_HOT[(pc - 0x07800000) >> 2]++;
    else if (pc >= 0x00100000 && pc < 0x00140000) PROF_SHIM[(pc - 0x00100000) >> 2]++;
    else prof_other++;
}
static void sh_dump(const char *name, const void *p, u32 len) {
    struct sh_open a = { name, 5, (u32)strlen(name) };
    int fd = semihost(0x01, &a); if (fd < 0) return;
    u32 w[3] = { (u32)fd, (u32)p, len }; semihost(0x05, w);
    u32 f = (u32)fd; semihost(0x02, &f);
}
static void prof_dump(void) {
    if (!(cfg->flags & CFG_PROFILE)) return;
    REG(GT_CTRL) = 1;
    sh_dump("prof_cold.bin", PROF_COLD, 0x400000);
    sh_dump("prof_hot.bin", PROF_HOT, 0x40000);
    sh_dump("prof_shim.bin", PROF_SHIM, 0x40000);
    u32 hdr[4] = { prof_samples, prof_other, prof_irqmode, cfg->prof_us ? cfg->prof_us : 97 };
    sh_dump("prof_info.bin", hdr, sizeof hdr);
}
static u32 irq_count;
static void vexSystemApplicationIRQHandler(u32 iar) {
    if ((cfg->flags & CFG_TRACE) && (irq_count++ % 1000) == 0) { uart_putc(1); uart_puts("irq "); uart_dec(iar); uart_puts(" n="); uart_dec(irq_count); uart_putc(2); }
    if ((iar & 0x3FF) == IRQ_GTIMER) { prof_sample(); return; }
    if ((iar & 0x3FF) == IRQ_PTIMER) {
        check_run_limit();
        if (tick_handler) tick_handler(0);
    }
}
static i32 vexSystemWatchdogReinitRtos(void) { return 0; }
static u32 vexSystemWatchdogGet(void) { return 0; }
static void vexSystemBoot(void) { }
static void vexSystemFIQInterrupt(void) { }

/* ------------------------------------------------------------------ system */
static u32 vexSystemTimeGet(void) { return now_ms(); }
static u64 vexSystemHighResTimeGet(void) { return now_us(); }
static u64 vexSystemPowerupTimeGet(void) { return now_us() + 3000000; }
static u32 vexSystemLinkAddrGet(void) { return 0x03800000; }
static u32 vexSystemVersion(void) { return 0x01010F00; }
static u32 vexSystemUsbStatus(void) { return 1; }
static void vexSystemExitRequest(void) { simlog("program exited (vexSystemExitRequest)"); sim_exit(0); }
static void vexPrivateApiDisable(u32 x) { (void)x; }
static u32 bg_calls;
static void vexBackgroundProcessing(void) {
    bg_calls++;
    if (cfg->bg_us) burn_us((double)cfg->bg_us);
}
static void check_run_limit(void) {
    if (cfg->run_ms && now_ms() >= cfg->run_ms) {
        char m[64]; (void)m;
        uart_putc(0x01); uart_puts("run limit reached at "); uart_dec(now_ms()); uart_puts(" ms"); uart_putc(0x02);
        sim_exit(0);
    }
}
static i32 vex_vprintf(const char *fmt, void *ap) { (void)ap; simlog(fmt); return 0; }

/* ------------------------------------------------------------------ serial */
static i32 vexSerialWriteBuffer(u32 ch, const u8 *data, u32 len) {
    (void)ch;
    for (u32 i = 0; i < len; i++) uart_putc((char)data[i]);
    return (i32)len;
}
static i32 vexSerialWriteChar(u32 ch, u8 c) { (void)ch; uart_putc((char)c); return 1; }
static i32 vexSerialReadChar(u32 ch) { (void)ch; return -1; }
static i32 vexSerialPeekChar(u32 ch) { (void)ch; return -1; }
static i32 vexSerialWriteFree(u32 ch) { (void)ch; return 2048; }

/* ---------------------------------------------------------------- SD card */
/* FIL* handles are (host fd + 1). Paths map to ./sd/<name> on the host. */
static u32 fil_pos[64];
static void *sd_open(const char *name, u32 mode) {
    if (!(cfg->flags & CFG_SD) || !name) return 0;
    static char path[300];
    const char *n = name;
    if (!strncmp(n, "/usd/", 5)) n += 5;
    while (*n == '/') n++;
    strcpy(path, "sd/"); strncat(path, n, sizeof(path) - 4);
    struct sh_open a = { path, mode, (u32)strlen(path) };
    int fd = semihost(0x01, &a);
    if (fd < 0 || fd >= 63) return 0;
    fil_pos[fd] = 0;
    if (mode == 9) { /* append */
        u32 l = (u32)fd; int sz = semihost(0x0C, &l); if (sz > 0) fil_pos[fd] = (u32)sz;
    }
    return (void *)(uintptr_t)(fd + 1);
}
static int fd_of(void *f) { return (int)(uintptr_t)f - 1; }
static i32 vexFileMountSD(void) { return (cfg->flags & CFG_SD) ? 0 : 3; }
static bool vexFileDriveStatus(u32 d) { (void)d; return (cfg->flags & CFG_SD) != 0; }
static void *vexFileOpen(const char *name, const char *mode) { (void)mode; return sd_open(name, 1); }
static void *vexFileOpenWrite(const char *name) { return sd_open(name, 5); }
static void *vexFileOpenCreate(const char *name) { return sd_open(name, 9); }
static void vexFileClose(void *f) { if (f) { u32 fd = (u32)fd_of(f); semihost(0x02, &fd); } }
static i32 vexFileRead(char *buf, u32 size, u32 n, void *f) {
    if (!f) return 0;
    u32 a[3] = { (u32)fd_of(f), (u32)buf, size * n };
    int left = semihost(0x06, a);
    u32 got = size * n - (u32)left;
    fil_pos[fd_of(f)] += got;
    return (i32)(size ? got / size : 0);
}
static i32 vexFileWrite(char *buf, u32 size, u32 n, void *f) {
    if (!f) return 0;
    u32 a[3] = { (u32)fd_of(f), (u32)buf, size * n };
    int left = semihost(0x05, a);
    u32 put = size * n - (u32)left;
    fil_pos[fd_of(f)] += put;
    return (i32)(size ? put / size : 0);
}
static i32 vexFileSize(void *f) { if (!f) return -1; u32 fd = (u32)fd_of(f); return semihost(0x0C, &fd); }
static i32 vexFileSeek(void *f, u32 off, i32 whence) {
    if (!f) return 1;
    int fd = fd_of(f); u32 pos = off;
    if (whence == 1) pos = fil_pos[fd] + off;
    else if (whence == 2) pos = (u32)vexFileSize(f) + off;
    u32 a[2] = { (u32)fd, pos };
    if (semihost(0x0A, a) != 0) return 1;
    fil_pos[fd] = pos; return 0;
}
static i32 vexFileTell(void *f) { return f ? (i32)fil_pos[fd_of(f)] : -1; }
static void vexFileSync(void *f) { (void)f; }
static u32 vexFileStatus(const char *name) { void *f = sd_open(name, 1); if (!f) return 0; vexFileClose(f); return 1; }

/* -------------------------------------------------------------- battery etc */
static i32 vexBatteryVoltageGet(void) { return 12600; }
static i32 vexBatteryCurrentGet(void) { return 1500; }
static double vexBatteryTemperatureGet(void) { return 32.0; }
static double vexBatteryCapacityGet(void) { return 90.0; }

static u32 comp_status(void) {
    u32 t = now_ms(), s = 0;
    for (u32 i = 0; i < cfg->n_comp && i < 32; i++) if (cfg->comp[i].t_ms <= t) s = cfg->comp[i].status;
    return s;
}
static u32 vexCompetitionStatus(void) { api_cost(); return comp_status(); }

static i32 vexControllerGet(u32 id, u32 index) {
    api_cost();
    u32 t = now_ms(); i32 v = 0;
    if (index == 19) return 100;   /* battery level */
    if (index == 22) return 100;
    if (index == 21) return 0;
    for (u32 i = 0; i < cfg->n_ctrl && i < 256; i++) {
        const struct ctrl_ev *e = &cfg->ctrl[i];
        if (e->t_ms <= t && e->ctrl == (u8)id && e->index == (u8)index) v = e->value;
    }
    return v;
}
static u32 vexControllerConnectionStatusGet(u32 id) { return id == 0 ? 1 : 0; }
static void vexTouchDataGet(void *st) { memset(st, 0, 12); }

/* ----------------------------------------------------------------- devices */
enum { T_NONE = 0, T_MOTOR = 2, T_ROT = 4, T_IMU = 6, T_DIST = 7, T_ADI = 12, T_OPT = 16, T_GPS = 20 };

struct motor {
    u8 gear, brake, units, reversed;
    u8 mode;            /* 0 velocity, 1 voltage, 2 position */
    i32 vel_cmd;        /* rpm */
    i32 volt_cmd;       /* mV */
    double target;      /* degrees */
    i32 pos_vel;        /* rpm cap for position moves */
    i32 cur_limit, volt_limit;
    double pos_deg;     /* output shaft degrees */
    double rpm;
    double temp;
    u64 last_us;
};
struct device {
    u32 port; u8 type;
    struct motor m;
    /* IMU / rotation */
    double heading, rotation_deg; u64 cal_until_us; i32 rot_pos; bool rot_rev;
    u32 adi_cfg[8]; i32 adi_val[8];
    i32 led_pwm;
};
static struct device devs[MAX_PORTS];

static double max_rpm(u8 g) { return g == 0 ? 100.0 : g == 2 ? 600.0 : 200.0; }
static double ticks_per_rev(u8 g) { return g == 0 ? 1800.0 : g == 2 ? 300.0 : 900.0; }

/* V5 motors report new data every 10 ms; step a first-order model at that rate. */
static void motor_update(struct device *d) {
    struct motor *m = &d->m;
    u64 t = now_us();
    u64 step = 10000;
    if (m->last_us == 0) m->last_us = t - (t % step);
    while (m->last_us + step <= t) {
        m->last_us += step;
        double mx = max_rpm(m->gear), want;
        if (m->mode == 1) want = mx * (double)m->volt_cmd / 12000.0;
        else if (m->mode == 2) {
            double err = m->target - m->pos_deg;
            double cap = m->pos_vel > 0 ? (double)m->pos_vel : mx;
            want = err * 2.0; /* deg -> rpm, P-loop */
            if (want > cap) want = cap; if (want < -cap) want = -cap;
            if (fabs(err) < 1.0) want = 0;
        } else want = (double)m->vel_cmd;
        if (want > mx) want = mx; if (want < -mx) want = -mx;
        if (m->mode == 0 && m->vel_cmd == 0 && m->brake == 0) want = m->rpm * 0.7; /* coast */
        m->rpm += (want - m->rpm) * 0.16;                         /* ~60 ms time constant */
        m->pos_deg += m->rpm * 6.0 * 0.01;                         /* rpm -> deg per 10 ms */
        m->temp += (fabs(m->rpm) / mx * 0.002) - (m->temp - 25.0) * 0.0005;
    }
}
static struct device *dev(void *h) { return (struct device *)h; }
static void *vexDeviceGetByIndex(u32 i) { api_cost(); return i < MAX_PORTS ? &devs[i] : 0; }
static i32 vexDeviceGetStatus(u8 *types) { for (int i = 0; i < MAX_PORTS; i++) types[i] = devs[i].type; return MAX_PORTS; }
static u32 vexDevicesGetNumber(void) { return MAX_PORTS; }
static u32 vexDeviceGetTimestamp(void *h) { (void)h; return now_ms(); }

static double units_from_deg(struct motor *m, double deg) {
    if (m->units == 1) return deg / 360.0;
    if (m->units == 2) return deg / 360.0 * ticks_per_rev(m->gear);
    return deg;
}
static double units_to_deg(struct motor *m, double v) {
    if (m->units == 1) return v * 360.0;
    if (m->units == 2) return v * 360.0 / ticks_per_rev(m->gear);
    return v;
}
#define MOT(h) struct device *d = dev(h); struct motor *m = &d->m; (void)m; api_cost(); motor_update(d)
static void vexDeviceMotorVelocitySet(void *h, i32 v) { MOT(h); m->mode = 0; m->vel_cmd = v; }
static void vexDeviceMotorVelocityUpdate(void *h, i32 v) { MOT(h); if (m->mode == 2) m->pos_vel = v; else m->vel_cmd = v; }
static i32 vexDeviceMotorVelocityGet(void *h) { MOT(h); return m->vel_cmd; }
static double vexDeviceMotorActualVelocityGet(void *h) { MOT(h); return m->rpm; }
static i32 vexDeviceMotorDirectionGet(void *h) { MOT(h); return m->rpm < 0 ? -1 : 1; }
static void vexDeviceMotorCurrentLimitSet(void *h, i32 v) { MOT(h); m->cur_limit = v; }
static i32 vexDeviceMotorCurrentLimitGet(void *h) { MOT(h); return m->cur_limit; }
static i32 vexDeviceMotorCurrentGet(void *h) { MOT(h); return (i32)(fabs(m->rpm) / max_rpm(m->gear) * 900 + 50); }
static double vexDeviceMotorPowerGet(void *h) { MOT(h); return fabs(m->rpm) / max_rpm(m->gear) * 6.0; }
static double vexDeviceMotorTorqueGet(void *h) { MOT(h); return 0.05; }
static double vexDeviceMotorEfficiencyGet(void *h) { MOT(h); return fabs(m->rpm) > 1 ? 45.0 : 0.0; }
static double vexDeviceMotorTemperatureGet(void *h) { MOT(h); return m->temp; }
static bool vexDeviceMotorOverTempFlagGet(void *h) { MOT(h); return m->temp > 55.0; }
static bool vexDeviceMotorCurrentLimitFlagGet(void *h) { MOT(h); return false; }
static bool vexDeviceMotorZeroVelocityFlagGet(void *h) { MOT(h); return fabs(m->rpm) < 1; }
static void vexDeviceMotorReverseFlagSet(void *h, bool r) { MOT(h); m->reversed = r; }
static bool vexDeviceMotorReverseFlagGet(void *h) { MOT(h); return m->reversed; }
static void vexDeviceMotorEncoderUnitsSet(void *h, u32 u) { MOT(h); m->units = (u8)u; }
static u32 vexDeviceMotorEncoderUnitsGet(void *h) { MOT(h); return m->units; }
static void vexDeviceMotorBrakeModeSet(void *h, u32 b) { MOT(h); m->brake = (u8)b; }
static u32 vexDeviceMotorBrakeModeGet(void *h) { MOT(h); return m->brake; }
static void vexDeviceMotorPositionSet(void *h, double p) { MOT(h); double nd = units_to_deg(m, p); m->target += nd - m->pos_deg; m->pos_deg = nd; }
static double vexDeviceMotorPositionGet(void *h) { MOT(h); return units_from_deg(m, m->pos_deg); }
static i32 vexDeviceMotorPositionRawGet(void *h, u32 *ts) { MOT(h); if (ts) *ts = now_ms(); return (i32)(m->pos_deg / 360.0 * ticks_per_rev(m->gear)); }
static void vexDeviceMotorPositionReset(void *h) { MOT(h); m->target -= m->pos_deg; m->pos_deg = 0; }
static double vexDeviceMotorTargetGet(void *h) { MOT(h); return units_from_deg(m, m->target); }
static void vexDeviceMotorAbsoluteTargetSet(void *h, double p, i32 v) { MOT(h); m->mode = 2; m->target = units_to_deg(m, p); m->pos_vel = v; }
static void vexDeviceMotorRelativeTargetSet(void *h, double p, i32 v) { MOT(h); m->mode = 2; m->target = m->pos_deg + units_to_deg(m, p); m->pos_vel = v; }
static u32 vexDeviceMotorFaultsGet(void *h) { MOT(h); return 0; }
static u32 vexDeviceMotorFlagsGet(void *h) { MOT(h); return 0; }
static void vexDeviceMotorVoltageSet(void *h, i32 v) { MOT(h); m->mode = 1; m->volt_cmd = v; }
static i32 vexDeviceMotorVoltageGet(void *h) { MOT(h); return m->mode == 1 ? m->volt_cmd : (i32)(m->rpm / max_rpm(m->gear) * 12000); }
static void vexDeviceMotorGearingSet(void *h, u32 g) { MOT(h); m->gear = (u8)g; }
static u32 vexDeviceMotorGearingGet(void *h) { MOT(h); return m->gear; }
static void vexDeviceMotorVoltageLimitSet(void *h, i32 v) { MOT(h); m->volt_limit = v; }
static i32 vexDeviceMotorVoltageLimitGet(void *h) { MOT(h); return m->volt_limit; }
static u32 vexDeviceMotorTypeGet(void *h) { MOT(h); return 0; }
static void vexDeviceMotorModeSet(void *h, u32 mode) { MOT(h); (void)mode; }
static u32 vexDeviceMotorModeGet(void *h) { MOT(h); return 5; }

/* IMU: calibrates for ~2 s after reset or program start, like the real sensor. */
struct imu_raw { double x, y, z, w; } __attribute__((packed));
struct imu_att { double pitch, roll, yaw; } __attribute__((packed));
static void vexDeviceImuReset(void *h) { api_cost(); dev(h)->cal_until_us = now_us() + 2000000; dev(h)->heading = 0; dev(h)->rotation_deg = 0; }
static double vexDeviceImuHeadingGet(void *h) { api_cost(); return dev(h)->heading; }
static double vexDeviceImuDegreesGet(void *h) { api_cost(); return dev(h)->rotation_deg; }
static void vexDeviceImuAttitudeGet(void *h, struct imu_att *a) { api_cost(); a->pitch = 0; a->roll = 0; a->yaw = dev(h)->heading; }
static void vexDeviceImuRawGyroGet(void *h, struct imu_raw *r) { api_cost(); (void)h; memset(r, 0, sizeof *r); }
static void vexDeviceImuRawAccelGet(void *h, struct imu_raw *r) { api_cost(); (void)h; memset(r, 0, sizeof *r); r->z = 1.0; }
static u32 vexDeviceImuStatusGet(void *h) { api_cost(); return now_us() < dev(h)->cal_until_us ? 1 : 0; }
static void vexDeviceImuDataRateSet(void *h, u32 r) { (void)h; (void)r; api_cost(); }
static void vexDeviceImuModeSet(void *h, u32 r) { (void)h; (void)r; }
static u32 vexDeviceImuModeGet(void *h) { (void)h; return 0; }

/* Rotation sensor (centidegrees) */
static void vexDeviceAbsEncReset(void *h) { api_cost(); dev(h)->rot_pos = 0; }
static void vexDeviceAbsEncPositionSet(void *h, i32 p) { api_cost(); dev(h)->rot_pos = p; }
static i32 vexDeviceAbsEncPositionGet(void *h) { api_cost(); return dev(h)->rot_pos; }
static i32 vexDeviceAbsEncVelocityGet(void *h) { api_cost(); (void)h; return 0; }
static i32 vexDeviceAbsEncAngleGet(void *h) { api_cost(); i32 a = dev(h)->rot_pos % 36000; return a < 0 ? a + 36000 : a; }
static void vexDeviceAbsEncReverseFlagSet(void *h, bool r) { api_cost(); dev(h)->rot_rev = r; }
static bool vexDeviceAbsEncReverseFlagGet(void *h) { api_cost(); return dev(h)->rot_rev; }
static u32 vexDeviceAbsEncStatusGet(void *h) { (void)h; return 0; }
static void vexDeviceAbsEncDataRateSet(void *h, u32 r) { (void)h; (void)r; api_cost(); }

/* Distance */
static u32 vexDeviceDistanceDistanceGet(void *h) { (void)h; api_cost(); return 500; }
static u32 vexDeviceDistanceConfidenceGet(void *h) { (void)h; api_cost(); return 63; }
static u32 vexDeviceDistanceStatusGet(void *h) { (void)h; api_cost(); return 0x82; }
static i32 vexDeviceDistanceObjectSizeGet(void *h) { (void)h; api_cost(); return 100; }
static double vexDeviceDistanceObjectVelocityGet(void *h) { (void)h; api_cost(); return 0.0; }

/* Optical */
static double vexDeviceOpticalHueGet(void *h) { (void)h; api_cost(); return 120.0; }
static double vexDeviceOpticalSatGet(void *h) { (void)h; api_cost(); return 0.5; }
static double vexDeviceOpticalBrightnessGet(void *h) { (void)h; api_cost(); return 0.3; }
static i32 vexDeviceOpticalProximityGet(void *h) { (void)h; api_cost(); return 50; }
static void vexDeviceOpticalRgbGet(void *h, double *d) { (void)h; api_cost(); d[0] = 0.1; d[1] = 0.6; d[2] = 0.1; d[3] = 0.3; }
static void vexDeviceOpticalLedPwmSet(void *h, i32 v) { api_cost(); dev(h)->led_pwm = v; }
static i32 vexDeviceOpticalLedPwmGet(void *h) { api_cost(); return dev(h)->led_pwm; }
static void vexDeviceOpticalRawGet(void *h, u16 *d) { (void)h; api_cost(); d[0] = 100; d[1] = 600; d[2] = 100; d[3] = 300; }
static u32 vexDeviceOpticalGestureGet(void *h, void *g) { (void)h; api_cost(); memset(g, 0, 16); return 0; }
static void vexDeviceOpticalGestureEnable(void *h) { (void)h; }
static void vexDeviceOpticalGestureDisable(void *h) { (void)h; }
static u32 vexDeviceOpticalStatusGet(void *h) { (void)h; return 0; }

/* ADI (3-wire) on the brain's internal port */
static void vexDeviceAdiPortConfigSet(void *h, u32 p, u32 c) { api_cost(); if (p < 8) dev(h)->adi_cfg[p] = c; }
static u32 vexDeviceAdiPortConfigGet(void *h, u32 p) { api_cost(); return p < 8 ? dev(h)->adi_cfg[p] : 0; }
static void vexDeviceAdiValueSet(void *h, u32 p, i32 v) { api_cost(); if (p < 8) dev(h)->adi_val[p] = v; }
static i32 vexDeviceAdiValueGet(void *h, u32 p) { api_cost(); return p < 8 ? dev(h)->adi_val[p] : 0; }
static void vexDeviceAdiAddrLedSet(void *h, u32 p, u32 *d, u32 o, u32 l, u32 opt) { (void)h; (void)p; (void)d; (void)o; (void)l; (void)opt; api_cost(); }

/* Display: drawing is accepted and discarded (LVGL still renders in PROS). */
static void vexDisplayNop(void) { }

/* --------------------------------------------------------- jump table fill */
#define JT_ADDR 0x037FC000u
#define JT_N 1024
static void **jt = (void **)JT_ADDR;
static u8 warned[JT_N];

/* Every slot we don't implement points at a unique trampoline that logs its name once
   and returns 0. Generated with a macro so the slot index is known. */
static void unknown_api(u32 idx) {
    if (idx < JT_N && !warned[idx]) {
        warned[idx] = 1;
        const char *nm = 0;
        for (u32 i = 0; i < sizeof(jt_names) / sizeof(jt_names[0]); i++) if (jt_names[i].off == idx * 4) nm = jt_names[i].name;
        uart_putc(0x01); uart_puts("unimplemented VEXos call "); uart_puts(nm ? nm : "?"); uart_puts(" @"); uart_hex(idx * 4); uart_putc(0x02);
    }
}
#define TR(n) static u32 tr_##n(void) { unknown_api(n); return 0; }
#define TR8(n) TR(n##0) TR(n##1) TR(n##2) TR(n##3) TR(n##4) TR(n##5) TR(n##6) TR(n##7)
/* trampolines are generated in trampolines.h (1024 entries) */
#include "trampolines.h"

#define SET(off, fn) jt[(off) / 4] = (void *)(fn)
static void jt_fill(void) {
    for (int i = 0; i < JT_N; i++) jt[i] = tramp_table[i];
    SET(0x020, vexPrivateApiDisable);
    SET(0x05c, vexBackgroundProcessing);
    SET(0xf74, vexBackgroundProcessing);
    SET(0x0f0, vex_vprintf);
    SET(0x118, vexSystemTimeGet);
    SET(0x130, vexSystemExitRequest);
    SET(0x134, vexSystemHighResTimeGet);
    SET(0x138, vexSystemPowerupTimeGet);
    SET(0x13c, vexSystemLinkAddrGet);
    SET(0x174, vexSystemUsbStatus);
    SET(0x190, vexDevicesGetNumber);
    SET(0x19c, vexDeviceGetByIndex);
    SET(0x1a0, vexDeviceGetStatus);
    SET(0x1a4, vexControllerGet);
    SET(0x1a8, vexControllerConnectionStatusGet);
    SET(0x1b0, vexDeviceGetTimestamp);
    SET(0x208, vexDeviceAdiPortConfigSet);
    SET(0x20c, vexDeviceAdiPortConfigGet);
    SET(0x210, vexDeviceAdiValueSet);
    SET(0x214, vexDeviceAdiValueGet);
    SET(0x21c, vexDeviceAdiAddrLedSet);
    SET(0x2d0, vexDeviceMotorVelocitySet);
    SET(0x2d4, vexDeviceMotorVelocityGet);
    SET(0x2d8, vexDeviceMotorActualVelocityGet);
    SET(0x2dc, vexDeviceMotorDirectionGet);
    SET(0x2e0, vexDeviceMotorModeSet);
    SET(0x2e4, vexDeviceMotorModeGet);
    SET(0x2f0, vexDeviceMotorCurrentLimitSet);
    SET(0x2f4, vexDeviceMotorCurrentLimitGet);
    SET(0x2f8, vexDeviceMotorCurrentGet);
    SET(0x2fc, vexDeviceMotorPowerGet);
    SET(0x300, vexDeviceMotorTorqueGet);
    SET(0x304, vexDeviceMotorEfficiencyGet);
    SET(0x308, vexDeviceMotorTemperatureGet);
    SET(0x30c, vexDeviceMotorOverTempFlagGet);
    SET(0x310, vexDeviceMotorCurrentLimitFlagGet);
    SET(0x314, vexDeviceMotorZeroVelocityFlagGet);
    SET(0x31c, vexDeviceMotorReverseFlagSet);
    SET(0x320, vexDeviceMotorReverseFlagGet);
    SET(0x324, vexDeviceMotorEncoderUnitsSet);
    SET(0x328, vexDeviceMotorEncoderUnitsGet);
    SET(0x32c, vexDeviceMotorBrakeModeSet);
    SET(0x330, vexDeviceMotorBrakeModeGet);
    SET(0x334, vexDeviceMotorPositionSet);
    SET(0x338, vexDeviceMotorPositionGet);
    SET(0x33c, vexDeviceMotorPositionRawGet);
    SET(0x340, vexDeviceMotorPositionReset);
    SET(0x344, vexDeviceMotorTargetGet);
    SET(0x34c, vexDeviceMotorAbsoluteTargetSet);
    SET(0x350, vexDeviceMotorRelativeTargetSet);
    SET(0x354, vexDeviceMotorFaultsGet);
    SET(0x358, vexDeviceMotorFlagsGet);
    SET(0x35c, vexDeviceMotorVoltageSet);
    SET(0x360, vexDeviceMotorVoltageGet);
    SET(0x364, vexDeviceMotorGearingSet);
    SET(0x368, vexDeviceMotorGearingGet);
    SET(0x36c, vexDeviceMotorVoltageLimitSet);
    SET(0x370, vexDeviceMotorVoltageLimitGet);
    SET(0x374, vexDeviceMotorVelocityUpdate);
    SET(0x384, vexDeviceMotorTypeGet);
    SET(0x410, vexDeviceImuReset);
    SET(0x414, vexDeviceImuHeadingGet);
    SET(0x418, vexDeviceImuDegreesGet);
    SET(0x420, vexDeviceImuAttitudeGet);
    SET(0x424, vexDeviceImuRawGyroGet);
    SET(0x428, vexDeviceImuRawAccelGet);
    SET(0x42c, vexDeviceImuStatusGet);
    SET(0x438, vexDeviceImuModeSet);
    SET(0x43c, vexDeviceImuModeGet);
    SET(0x444, vexDeviceImuDataRateSet);
    SET(0x488, vexDeviceAbsEncReset);
    SET(0x48c, vexDeviceAbsEncPositionSet);
    SET(0x490, vexDeviceAbsEncPositionGet);
    SET(0x494, vexDeviceAbsEncVelocityGet);
    SET(0x498, vexDeviceAbsEncAngleGet);
    SET(0x49c, vexDeviceAbsEncReverseFlagSet);
    SET(0x4a0, vexDeviceAbsEncReverseFlagGet);
    SET(0x4a4, vexDeviceAbsEncStatusGet);
    SET(0x4c0, vexDeviceAbsEncDataRateSet);
    SET(0x500, vexDeviceDistanceDistanceGet);
    SET(0x504, vexDeviceDistanceConfidenceGet);
    SET(0x508, vexDeviceDistanceStatusGet);
    SET(0x518, vexDeviceDistanceObjectSizeGet);
    SET(0x51c, vexDeviceDistanceObjectVelocityGet);
    SET(0x528, vexDeviceOpticalHueGet);
    SET(0x52c, vexDeviceOpticalSatGet);
    SET(0x530, vexDeviceOpticalBrightnessGet);
    SET(0x534, vexDeviceOpticalProximityGet);
    SET(0x538, vexDeviceOpticalRgbGet);
    SET(0x53c, vexDeviceOpticalLedPwmSet);
    SET(0x540, vexDeviceOpticalLedPwmGet);
    SET(0x544, vexDeviceOpticalStatusGet);
    SET(0x548, vexDeviceOpticalRawGet);
    SET(0x558, vexDeviceOpticalGestureGet);
    SET(0x55c, vexDeviceOpticalGestureEnable);
    SET(0x560, vexDeviceOpticalGestureDisable);
    /* display: every slot 0x640..0x6ff accepted and ignored */
    for (u32 o = 0x640; o < 0x700; o += 4) SET(o, vexDisplayNop);
    SET(0x7d0, vexFileMountSD);
    SET(0x7d8, vexFileOpen);
    SET(0x7dc, vexFileOpenWrite);
    SET(0x7e0, vexFileOpenCreate);
    SET(0x7e4, vexFileClose);
    SET(0x7ec, vexFileWrite);
    SET(0x7f0, vexFileSize);
    SET(0x7f4, vexFileSeek);
    SET(0x7f8, vexFileRead);
    SET(0x7fc, vexFileDriveStatus);
    SET(0x800, vexFileTell);
    SET(0x804, vexFileSync);
    SET(0x808, vexFileStatus);
    SET(0x898, vexSerialWriteChar);
    SET(0x89c, vexSerialWriteBuffer);
    SET(0x8a0, vexSerialReadChar);
    SET(0x8a4, vexSerialPeekChar);
    SET(0x8ac, vexSerialWriteFree);
    SET(0x8c0, vexSystemTimerStop);
    SET(0x8c4, vexSystemTimerClearInterrupt);
    SET(0x8c8, vexSystemTimerReinitForRtos);
    SET(0x8cc, vexSystemApplicationIRQHandler);
    SET(0x8d0, vexSystemWatchdogReinitRtos);
    SET(0x8d4, vexSystemWatchdogGet);
    SET(0x910, vexSystemBoot);
    SET(0x918, vexSystemFIQInterrupt);
    SET(0x964, vexTouchDataGet);
    SET(0x9d8, vexCompetitionStatus);
    SET(0xa00, vexBatteryVoltageGet);
    SET(0xa04, vexBatteryCurrentGet);
    SET(0xa08, vexBatteryTemperatureGet);
    SET(0xa0c, vexBatteryCapacityGet);
    (void)vexSystemVersion;
    /* vexSystemVersion() is not a call: libv5rts reads this word. PROS needs > 1.0.3 for hot/cold. */
    *(volatile u32 *)0x037FD000 = 0x01010500; /* VEXos 1.1.5 */
}

/* ------------------------------------------------------------------- main */
extern void vexos_jump(u32 entry);
void vexos_main(void) {
    uart_init();
    if (cfg->magic != CFG_MAGIC) { simlog("bad config block"); sim_exit(2); }
    mips = cfg->mips ? cfg->mips : 400;
    calibrate();
    for (int i = 0; i < MAX_PORTS; i++) {
        devs[i].port = i + 1;
        devs[i].type = cfg->port_type[i];
        devs[i].m.gear = cfg->motor_gear[i];
        devs[i].m.temp = 25.0;
        devs[i].m.cur_limit = 2500; devs[i].m.volt_limit = 12000;
        devs[i].cal_until_us = 2000000; /* sensors calibrate at power-up */
    }
    jt_fill();
    gic_init();
    __asm__ volatile("dsb\n isb");
    uart_putc(0x01); uart_puts("V5Sim fake VEXos: Zynq Cortex-A9 @ "); uart_dec(mips);
    uart_puts(" MIPS, bg "); uart_dec(cfg->bg_us); uart_puts(" us"); uart_putc(0x02);
    gt_start = gt_read();
    vexos_jump(0x03800020); /* vexStartup, right after the 32-byte code signature */
    simlog("program returned");
    sim_exit(1);
}
