
#include <stdint.h>
#include <Library/BaseLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/BaikalDebug.h>
#include <Library/BaikalSmcLib.h>


#define BAIKAL_SMC_FLASH_DATA_SIZE    1024

/* PVT */
#define BAIKAL_SMC_PVT_ID         0x82000001

/* FLASH */
#define BAIKAL_SMC_FLASH          0x82000002
#define BAIKAL_SMC_FLASH_WRITE    (BAIKAL_SMC_FLASH +0)
#define BAIKAL_SMC_FLASH_READ     (BAIKAL_SMC_FLASH +1)
#define BAIKAL_SMC_FLASH_ERASE    (BAIKAL_SMC_FLASH +2)
#define BAIKAL_SMC_FLASH_PUSH     (BAIKAL_SMC_FLASH +3)
#define BAIKAL_SMC_FLASH_PULL     (BAIKAL_SMC_FLASH +4)
#define BAIKAL_SMC_FLASH_POSITION (BAIKAL_SMC_FLASH +5)
#define BAIKAL_SMC_FLASH_INFO     (BAIKAL_SMC_FLASH +6)



//---------------
// DEFAULT LIB
//---------------
const smc_lib_t default_lib = {
  .info       = smc_info,
  .erase      = smc_erase,
  .write      = smc_write,
  .read       = smc_read,
  .push       = smc_push,
  .pull       = smc_pull,

  .position   = smc_position,
  .write_buf  = smc_write_buf,
  .read_buf   = smc_read_buf,
  .push_4word = smc_push_4word,
  .pull_4word = smc_pull_4word,
  .cmd        = smc_cmd,
};

#define SMC_LIB          \
    smc_lib_t *lib = l;  \
    if (!l)              \
      lib = (void*)&default_lib



//---------------
// INTERNAL
//---------------
void smc_cmd (void *l, uint32_t cmd, uint32_t arg1, uint32_t arg2)
{
  // SMC_LIB;
  ARM_SMC_ARGS args;
  args.Arg0 = cmd;
  args.Arg1 = (uint64_t) arg1;
  args.Arg2 = (uint64_t) arg2;
  ArmCallSmc (&args);
}

void smc_position (void *l, uint32_t position)
{
  SMC_LIB;
  if(position >= BAIKAL_SMC_FLASH_DATA_SIZE)
    return;
  lib->cmd (lib,BAIKAL_SMC_FLASH_POSITION,position,0);
}

void smc_write_buf (void *l, uint32_t adr, uint32_t size)
{
  SMC_LIB;
  if(!size)
    return;
  lib->cmd (lib,BAIKAL_SMC_FLASH_WRITE,adr,size);
}

void smc_read_buf (void *l, uint32_t adr, uint32_t size)
{
  SMC_LIB;
  if(!size)
    return;
  lib->cmd (lib,BAIKAL_SMC_FLASH_READ,adr,size);
}

/* push 4 uint64_t word to arm-tf */
void smc_push_4word (void *l, void *data)
{
  // SMC_LIB;
  if(!data)
    return;
  uint64_t *d = data;

  ARM_SMC_ARGS args;
  args.Arg0 = BAIKAL_SMC_FLASH_PUSH;
  args.Arg1 = *d++;
  args.Arg2 = *d++;
  args.Arg3 = *d++;
  args.Arg4 = *d++;

  ArmCallSmc (&args);
}

/* pull 4 uint64_t word from arm-tf */
void smc_pull_4word (void *l, void *data)
{
  // SMC_LIB;
  if(!data)
    return;
  uint64_t *d = data;

  ARM_SMC_ARGS args;
  args.Arg0 = BAIKAL_SMC_FLASH_PULL;

  ArmCallSmc (&args);

  *d++ = args.Arg0;
  *d++ = args.Arg1;
  *d++ = args.Arg2;
  *d++ = args.Arg3;
}


//---------------
// EXTERNAL
//---------------

/* push buffer to arm-tf */
void smc_push (void *l, void *data, uint32_t size)
{
  SMC_LIB;
  if(!data || !size || (size > BAIKAL_SMC_FLASH_DATA_SIZE))
    return;
  uint8_t *a = data;
  uint8_t *pb;
  uint8_t  part;
  lib->position(lib,0);

  while (size) {
    uint64_t b[4] = {0,0,0,0};
    pb = (void*)b;
    part = size > sizeof(b)? sizeof(b) : size;
    size -= part;

    while (part--)
      *pb++ = *a++;

    lib->push_4word(lib,b);
  }
}

/* get buffer from arm-tf */
void smc_pull (void *l, void *data, uint32_t size)
{
  SMC_LIB;
  if(!data || !size || (size > BAIKAL_SMC_FLASH_DATA_SIZE))
    return;
  uint8_t *a = data;
  uint8_t *pb;
  uint8_t  part;
  lib->position(lib,0);

  while (size) {
    uint64_t b[4] = {0,0,0,0};
    lib->pull_4word(lib,b);

    pb = (void*)b;
    part = size > sizeof(b)? sizeof(b) : size;
    size -= part;
    while (part--)
      *a++ = *pb++;
  }
}

void smc_erase (void *l, uint32_t adr, uint32_t size)
{
  SMC_LIB;
  lib->cmd (lib,BAIKAL_SMC_FLASH_ERASE, adr, size);
}

void smc_write (void *l, uint32_t adr, void *data, uint32_t size)
{
    SMC_LIB;
    if(!data || !size)
      return;
    uint32_t part;
    uint8_t *pdata = data;
    while (size) {

        part = (size > BAIKAL_SMC_FLASH_DATA_SIZE)? BAIKAL_SMC_FLASH_DATA_SIZE : size;

        lib->push(lib,pdata,part);
        lib->write_buf(lib,adr,part);

        adr   += part;
        pdata += part;
        size  -= part;
    }
}

void smc_read (void *l, uint32_t adr, void* data, uint32_t size)
{
    SMC_LIB;
    if(!data || !size)
      return;
    uint32_t part;
    uint8_t *pdata = data;
    while (size) {

        part = (size > BAIKAL_SMC_FLASH_DATA_SIZE)? BAIKAL_SMC_FLASH_DATA_SIZE : size;

        lib->read_buf(lib,adr,part);
        lib->pull(lib,pdata,part);

        adr   += part;
        pdata += part;
        size  -= part;
    }
}

void smc_info (void *l, uint32_t *sector_size, uint32_t* sector_cnt)
{
  // SMC_LIB;
  ARM_SMC_ARGS args;
  args.Arg0 = BAIKAL_SMC_FLASH_INFO;
  ArmCallSmc (&args);
  if(sector_size)
    *sector_size = args.Arg0;
  if(sector_cnt)
    *sector_cnt = args.Arg1;
}
