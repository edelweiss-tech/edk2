
#ifndef __BAIKAL_SPI_LIB_H__
#define __BAIKAL_SPI_LIB_H__

#include <stdint.h>

/* ---------- */
/* flash_map */
/* ---------- */
#define FLASH_MAP_FDT      0x40000
#define FLASH_MAP_EFI_VARS 0x50000
#define FLASH_MAP_FIP      0x110000
#define UEFI_VAR_OFFSET    FLASH_MAP_EFI_VARS
// see: armtf/plat/baikal/include/platform_def.h



/* ---------- */
/* macros     */
/* ---------- */
#define COUNTOF(x)  (sizeof(x)/sizeof(x[0]))
#define DIVIDE(x,n) (((x) + (n) - 1) / (n))   /* round up */



/* ---------- */
/* flash_info */
/* ---------- */
#define SECTOR_SIZE        (64*1024)
#define SECTOR_CNT          512
#define SPI_NOR_MAX_ID_LEN  3

/*
 * id - This array stores the ID bytes.
 *      The first three bytes are the JEDIC ID.
 *      JEDEC ID zero means "no ID" (mostly older chips).
 *
 * sector_size - The size listed here is what works
 *      with SPINOR_OP_SE, which isn't necessarily
 *      called a "sector" by the vendor.
 */
typedef struct {
    char *name;
    uint8_t id [SPI_NOR_MAX_ID_LEN];
    uint32_t sector_size;
    uint32_t n_sectors;
} flash_info_t;



/* ----------- */
/* PROTOTYPE'S */
/* ----------- */
const flash_info_t * llenv32_spi_init (int port, int line);
int  llenv32_spi_erase   (int port, int line, uint32_t adr, uint32_t size, uint32_t sectore_size);
int  llenv32_spi_read    (int port, int line, uint32_t adr, void *data, uint32_t size);
int  llenv32_spi_write   (int port, int line, uint32_t adr, void *data, uint32_t size);
int  llenv32_spi_ping    (int port, int line);
int  llenv32_enter_4byte (int port, int line);
int  llenv32_exit_4byte  (int port, int line);



#endif
