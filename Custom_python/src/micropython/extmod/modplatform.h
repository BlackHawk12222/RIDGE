#ifndef MICROPY_INCLUDED_EXTMOD_MODPLATFORM_H
#define MICROPY_INCLUDED_EXTMOD_MODPLATFORM_H

/* Identify the VEX V5 System & Architecture Details */
#define MICROPY_PLATFORM_SYSTEM        "VEX-V5"
#define MICROPY_PLATFORM_ARCH          "ARM-Cortex-A9"
#define MICROPY_PLATFORM_VERSION       "PROS-Kernel"

/* Provide Mock C-Library Identifiers for the PROS toolchain */
#define MICROPY_PLATFORM_LIBC_LIB      "newlib"
#define MICROPY_PLATFORM_LIBC_VER      "GCC"

#define MICROPY_PLATFORM_COMPILER      "arm-none-eabi-gcc"

#endif // MICROPY_INCLUDED_EXTMOD_MODPLATFORM_H
