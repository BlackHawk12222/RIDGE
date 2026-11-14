// binascii.crc32 needs uzlib's CRC table; the rest of uzlib (deflate) isn't built.
#include "py/mpconfig.h"
#if MICROPY_PY_BINASCII && MICROPY_PY_BINASCII_CRC32 && MICROPY_PY_DEFLATE
#include "lib/uzlib/crc32.c"
#endif
