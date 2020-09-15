/** @file

  FV block I/O protocol driver for SPI flash

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Protocol/FirmwareVolumeBlock.h>
#include <Protocol/BlockIo.h>
#include <Guid/VariableFormat.h>
#include <Guid/SystemNvDataGuid.h>
#include <Library/BaikalSpiLib.h>
#include <Library/BaikalSmcLib.h>
#include <Library/BaikalDebug.h>
#include "Block.h"


// static flash_info_t info;
static smc_flash_t flash;

EFI_HANDLE mBaikalSpiBlockHandle;
BAIKAL_SPI_PRIVATE_DATA mBaikalSpi;
BAIKAL_SPI_DEVICE_PATH dp0 = {
  //Vendor
  {
    //Header
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof(VENDOR_DEVICE_PATH)),
        (UINT8) (sizeof(VENDOR_DEVICE_PATH) >> 8)
      }
    },
    //Guid
    {
      0xA, 0xA, 0xA, { 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA, 0xA }
    },
  },
  //End
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      sizeof(EFI_DEVICE_PATH_PROTOCOL),
      0
    }
  }
};




/*
*==================================================
* EFI_BLOCK_IO_PROTOCOL
*==================================================
*/

  /**
    Reset the Block Device.

    @param  This                 Indicates a pointer to the calling context.
    @param  ExtendedVerification Driver may perform diagnostics on reset.

    @retval EFI_SUCCESS          The device was reset.
    @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                                 not be reset.
  **/
  EFI_STATUS
  EFIAPI
  BaikalSpiBlkIoReset (
    IN EFI_BLOCK_IO_PROTOCOL        *This,
    IN BOOLEAN                      ExtendedVerification
    )
  {
    return EFI_SUCCESS;
  }

  /**
    Read BufferSize bytes from Lba into Buffer.

    @param[in]  This           Indicates a pointer to the calling context.
    @param[in]  MediaId        Id of the media, changes every time the media is
                               replaced.
    @param[in]  Lba            The starting Logical Block Address to read from.
    @param[in]  BufferSize     Size of Buffer, must be a multiple of device block
                               size.
    @param[out] Buffer         A pointer to the destination buffer for the data.
                               The caller is responsible for either having
                               implicit or explicit ownership of the buffer.

    @retval EFI_SUCCESS             The data was read correctly from the device.
    @retval EFI_DEVICE_ERROR        The device reported an error while performing
                                    the read.
    @retval EFI_NO_MEDIA            There is no media in the device.
    @retval EFI_MEDIA_CHANGED       The MediaId does not matched the current
                                    device.
    @retval EFI_BAD_BUFFER_SIZE     The Buffer was not a multiple of the block
                                    size of the device.
    @retval EFI_INVALID_PARAMETER   The read request contains LBAs that are not
                                    valid, or the buffer is not on proper alignment.
  **/
  EFI_STATUS
  EFIAPI
  BaikalSpiBlkIoReadBlocks (
    IN EFI_BLOCK_IO_PROTOCOL        *This,
    IN UINT32                       MediaId,
    IN EFI_LBA                      Lba,
    IN UINTN                        BufferSize,
    OUT VOID                        *Buffer
    )
  {
    BAIKAL_SPI_PRIVATE_DATA        *PrivateData;
    UINTN                           NumberOfBlocks;
    UINTN                           SpiOffset;

    if (Buffer == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    if (BufferSize == 0) {
      return EFI_SUCCESS;
    }

    PrivateData = BAIKAL_SPI_PRIVATE_FROM_BLKIO (This);

    if (MediaId != PrivateData->Media.MediaId) {
      return EFI_MEDIA_CHANGED;
    }

    if ((BufferSize % PrivateData->Media.BlockSize) != 0) {
      return EFI_BAD_BUFFER_SIZE;
    }

    if (Lba > PrivateData->Media.LastBlock) {
      return EFI_INVALID_PARAMETER;
    }

    NumberOfBlocks = BufferSize / PrivateData->Media.BlockSize;
    if ((Lba + NumberOfBlocks - 1) > PrivateData->Media.LastBlock) {
      return EFI_INVALID_PARAMETER;
    }
    SpiOffset = Lba * SPI_DISK_BLOCK_SIZE + UEFI_BLOCK_OFFSET; //1024*1024*8;

    smc_read(&flash.smc, SpiOffset,Buffer,BufferSize);

    return EFI_SUCCESS;
  }

  /**
    Write BufferSize bytes from Buffer into Lba.

    @param[in] This            Indicates a pointer to the calling context.
    @param[in] MediaId         The media ID that the write request is for.
    @param[in] Lba             The starting logical block address to be written.
                               The caller is responsible for writing to only
                               legitimate locations.
    @param[in] BufferSize      Size of Buffer, must be a multiple of device block
                               size.
    @param[in] Buffer          A pointer to the source buffer for the data.

    @retval EFI_SUCCESS             The data was written correctly to the device.
    @retval EFI_WRITE_PROTECTED     The device can not be written to.
    @retval EFI_DEVICE_ERROR        The device reported an error while performing
                                    the write.
    @retval EFI_NO_MEDIA            There is no media in the device.
    @retval EFI_MEDIA_CHNAGED       The MediaId does not matched the current
                                    device.
    @retval EFI_BAD_BUFFER_SIZE     The Buffer was not a multiple of the block
                                    size of the device.
    @retval EFI_INVALID_PARAMETER   The write request contains LBAs that are not
                                    valid, or the buffer is not on proper alignment.
  **/
  EFI_STATUS
  EFIAPI
  BaikalSpiBlkIoWriteBlocks (
    IN EFI_BLOCK_IO_PROTOCOL        *This,
    IN UINT32                       MediaId,
    IN EFI_LBA                      Lba,
    IN UINTN                        BufferSize,
    IN VOID                         *Buffer
    )
  {
    BAIKAL_SPI_PRIVATE_DATA         *PrivateData;
    UINTN                           NumberOfBlocks;
    EFI_STATUS                      ref = EFI_DEVICE_ERROR;

    if (Buffer == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    if (BufferSize == 0) {
      return EFI_SUCCESS;
    }

    PrivateData = BAIKAL_SPI_PRIVATE_FROM_BLKIO (This);

    if (MediaId != PrivateData->Media.MediaId) {
      return EFI_MEDIA_CHANGED;
    }

    if (TRUE == PrivateData->Media.ReadOnly) {
      return EFI_WRITE_PROTECTED;
    }

    if ((BufferSize % PrivateData->Media.BlockSize) != 0) {
      return EFI_BAD_BUFFER_SIZE;
    }

    if (Lba > PrivateData->Media.LastBlock) {
      return EFI_INVALID_PARAMETER;
    }

    NumberOfBlocks = BufferSize / PrivateData->Media.BlockSize;
    if ((Lba + NumberOfBlocks - 1) > PrivateData->Media.LastBlock) {
      return EFI_INVALID_PARAMETER;
    }

    UINTN SpiOffset  = Lba * SPI_DISK_BLOCK_SIZE  + UEFI_BLOCK_OFFSET;
    UINTN adr = (SpiOffset / flash.info.sector_size) * flash.info.sector_size;
    UINTN offset = SpiOffset - adr;
    UINTN total_size = offset + BufferSize;
    UINTN cnt = DIVIDE(total_size, flash.info.sector_size);
    UINTN size = cnt * flash.info.sector_size;

    if(total_size > size){
      return EFI_OUT_OF_RESOURCES;
    }

    VOID *buf = AllocatePool(size);
    if(!buf){
      return EFI_OUT_OF_RESOURCES;
    }

    smc_read(&flash.smc, adr,buf,size);

    if(offset + BufferSize > size){
      goto out;
    }

    CopyMem (buf+offset,Buffer,BufferSize);

    smc_erase(&flash.smc, adr,size);
    smc_write(&flash.smc, adr,buf,size);
    ref = EFI_SUCCESS;

  out:
    if(buf)
      FreePool(buf);
    return ref;
  }

  /**
    Flush the Block Device.

    @param[in] This            Indicates a pointer to the calling context.

    @retval EFI_SUCCESS             All outstanding data was written to the device.
    @retval EFI_DEVICE_ERROR        The device reported an error while writting
                                    back the data
    @retval EFI_NO_MEDIA            There is no media in the device.
  **/
  EFI_STATUS
  EFIAPI
  BaikalSpiBlkIoFlushBlocks (
    IN EFI_BLOCK_IO_PROTOCOL        *This
    )
  {
    return EFI_SUCCESS;
  }

  //
  // The EFI_BLOCK_IO_PROTOCOL instances that is installed onto the handle
  //
  EFI_BLOCK_IO_PROTOCOL
  mBaikalSpiBlockIoProtocol = {
    EFI_BLOCK_IO_PROTOCOL_REVISION,
    (EFI_BLOCK_IO_MEDIA *) 0,
    BaikalSpiBlkIoReset,
    BaikalSpiBlkIoReadBlocks,
    BaikalSpiBlkIoWriteBlocks,
    BaikalSpiBlkIoFlushBlocks
  };




/*
*==================================================
* INSTALL
*==================================================
*/
  EFI_STATUS EFIAPI InstallBlock (VOID)
  {
    EFI_STATUS Status;

    /* BLOCK */
    BAIKAL_SPI_PRIVATE_DATA   *PrivateData = &mBaikalSpi;
    EFI_BLOCK_IO_PROTOCOL     *BlockIo;
    EFI_BLOCK_IO_MEDIA        *Media;

    BlockIo  = &PrivateData->BlockIo;
    Media    = &PrivateData->Media;

    PrivateData->Size = flash.info.sector_size * flash.info.n_sectors - UEFI_BLOCK_OFFSET;
    PrivateData->Signature = BAIKAL_SPI_PRIVATE_DATA_SIGNATURE;

    CopyMem (BlockIo, &mBaikalSpiBlockIoProtocol, sizeof (EFI_BLOCK_IO_PROTOCOL));

    BlockIo->Media          = Media;
    Media->RemovableMedia   = FALSE;
    Media->MediaPresent     = TRUE;
    Media->LogicalPartition = FALSE;
    Media->ReadOnly         = FALSE;
    Media->WriteCaching     = FALSE;
    Media->BlockSize        = SPI_DISK_BLOCK_SIZE;
    Media->LastBlock        = DivU64x32 (PrivateData->Size + SPI_DISK_BLOCK_SIZE - 1, SPI_DISK_BLOCK_SIZE) - 1;
    PrivateData->DevicePath = dp0;

    /* INSTALL */
    Status = gBS->InstallMultipleProtocolInterfaces (
      &mBaikalSpiBlockHandle,
      &gEfiDevicePathProtocolGuid, &mBaikalSpi.DevicePath,
      &gEfiBlockIoProtocolGuid,    &mBaikalSpi.BlockIo,
      NULL
    );
    ASSERT_EFI_ERROR (Status);

    return Status;
  }


/*
*==================================================
* INITIALIZE
*==================================================
*/

EFI_STATUS EFIAPI BaikalSpiBlockDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS Status;

  /* LIB */
  // external
  flash.smc.info       = smc_info;
  flash.smc.erase      = smc_erase;
  flash.smc.write      = smc_write;
  flash.smc.read       = smc_read;
  flash.smc.push       = smc_push;
  flash.smc.pull       = smc_pull;

  // internal
  flash.smc.position   = smc_position;
  flash.smc.write_buf  = smc_write_buf;
  flash.smc.read_buf   = smc_read_buf;
  flash.smc.push_4word = smc_push_4word;
  flash.smc.pull_4word = smc_pull_4word;
  flash.smc.cmd        = smc_cmd;

  /* INFO */
  smc_info(&flash.smc, &(flash.info.sector_size), &(flash.info.n_sectors));

  /* INSTALL */
  Status = InstallBlock();
  if(Status != EFI_SUCCESS){
    return Status;
  }

  return EFI_SUCCESS;
}
