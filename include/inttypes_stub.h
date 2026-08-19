#ifndef LIMOOS_INTTYPES_STUB_H
#define LIMOOS_INTTYPES_STUB_H

/* =============================================================================
 * inttypes_stub.h — replacement for <inttypes.h>, which doesn't exist in
 * this freestanding i686-elf toolchain (built --without-headers, so only
 * GCC's own bundled headers like stdint.h are available — inttypes.h is
 * normally provided by libc, which this kernel doesn't have).
 *
 * Only the printf/scanf format macros are defined — LVGL (via
 * lv_conf.h's LV_INTTYPES_INCLUDE override) is the only consumer, and it
 * only needs these, not the full <inttypes.h> API (imaxdiv, strtoimax,
 * etc — none of that exists here either, and nothing in this kernel
 * calls it).
 *
 * Values match the ILP32 data model this i686 target uses: int is 32-bit
 * (so int32_t is `int`, not `long`), long long is 64-bit. If this
 * project ever targets x86-64 instead, these need revisiting — LP64's
 * int32_t is still `int` but long is 64-bit, which changes nothing here
 * since none of these macros use the `l` length modifier, but IS a trap
 * for anyone tempted to copy this file as-is to a 64-bit target without
 * checking.
 * ========================================================================== */

/* ---- 8-bit ------------------------------------------------------------ */
#define PRId8  "d"
#define PRIi8  "i"
#define PRIu8  "u"
#define PRIx8  "x"
#define PRIX8  "X"
#define PRIo8  "o"

/* ---- 16-bit ------------------------------------------------------------ */
#define PRId16 "d"
#define PRIi16 "i"
#define PRIu16 "u"
#define PRIx16 "x"
#define PRIX16 "X"
#define PRIo16 "o"

/* ---- 32-bit (int on ILP32 — no length modifier needed) ----------------- */
#define PRId32 "d"
#define PRIi32 "i"
#define PRIu32 "u"
#define PRIx32 "x"
#define PRIX32 "X"
#define PRIo32 "o"

/* ---- 64-bit (long long on ILP32 — needs the "ll" length modifier) ------ */
#define PRId64 "lld"
#define PRIi64 "lli"
#define PRIu64 "llu"
#define PRIx64 "llx"
#define PRIX64 "llX"
#define PRIo64 "llo"

/* ---- pointer-sized (uintptr_t is 32-bit here, same as int) ------------- */
#define PRIdPTR "d"
#define PRIiPTR "i"
#define PRIuPTR "u"
#define PRIxPTR "x"
#define PRIXPTR "X"

#endif /* LIMOOS_INTTYPES_STUB_H */
