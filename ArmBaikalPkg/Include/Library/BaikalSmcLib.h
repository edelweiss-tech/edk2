
#ifndef __BAIKAL_SMC_LIB_H__
#define __BAIKAL_SMC_LIB_H__

#include <stdint.h>
#include <Library/BaikalSpiLib.h>

//------------
// PROTOTYPE
//------------
	// external
	void smc_info       (void *lib, uint32_t *psize, uint32_t *pcnt);
	void smc_erase      (void *lib, uint32_t adr,             uint32_t size);
	void smc_write      (void *lib, uint32_t adr, void *data, uint32_t size);
	void smc_read       (void *lib, uint32_t adr, void *data, uint32_t size);
	void smc_push       (void *lib, void *data, uint32_t size);
	void smc_pull       (void *lib, void *data, uint32_t size);

	// internal
	void smc_position   (void *lib, uint32_t position);
	void smc_write_buf  (void *lib, uint32_t adr, uint32_t size);
	void smc_read_buf   (void *lib, uint32_t adr, uint32_t size);
	void smc_push_4word (void *lib, void *data);
	void smc_pull_4word (void *lib, void *data);
	void smc_cmd        (void *lib, uint32_t cmd, uint32_t adr, uint32_t size);


//------------
// TYPEDEF
//------------
typedef struct {
	// external
	void (*info )       (void *lib, uint32_t *psize, uint32_t *pcnt);
	void (*erase)       (void *lib, uint32_t adr,             uint32_t size);
	void (*write)       (void *lib, uint32_t adr, void *data, uint32_t size);
	void (*read )       (void *lib, uint32_t adr, void *data, uint32_t size);
	void (*push )       (void *lib, void *data, uint32_t size);
	void (*pull )       (void *lib, void *data, uint32_t size);

	// internal
	void (*position  )  (void *lib, uint32_t position);
	void (*write_buf )  (void *lib, uint32_t adr, uint32_t size);
	void (*read_buf  )  (void *lib, uint32_t adr, uint32_t size);
	void (*push_4word)  (void *lib, void *data);
	void (*pull_4word)  (void *lib, void *data);
	void (*cmd)         (void *lib, uint32_t cmd, uint32_t adr, uint32_t size);
} smc_lib_t;

typedef struct {
	smc_lib_t      smc;
	flash_info_t   info;
} smc_flash_t;

VOID
BaikalSmcSetHdmiFrequency(
  UINT32 OscFreq
  );



#endif
