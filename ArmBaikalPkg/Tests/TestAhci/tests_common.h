
#ifndef __TESTS_COMMON_H
#define __TESTS_COMMON_H

#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>

typedef INT8    int8_t;
typedef UINT8   uint8_t;
typedef INT16   int16_t;
typedef UINT16  uint16_t;
typedef INT32   int32_t;
typedef UINT32  uint32_t;
typedef INT64   int64_t;
typedef UINT64  uint64_t;

#define TEST_PRINT(...) AsciiPrint("TEST: " __VA_ARGS__)
#define ERROR(...) AsciiPrint("ERROR:" __VA_ARGS__)

#define mmio_write_32(p, val) (*(uint32_t*)p) = val
#define mmio_read_32(p) (*(uint32_t*)p)

#define TEST_DEC(name, run, desc)
//        bl1_test _test_##name __section("bl1_test_desc")= { desc, 0, run }

#define flush_dcache_range(ptr, sz)

#define memset(s, c, n) SetMem( s, (UINTN)n, (UINT8)c)

#endif

