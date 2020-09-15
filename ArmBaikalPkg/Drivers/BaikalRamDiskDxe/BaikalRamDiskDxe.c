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





/*
*==================================================
* RAMDISK block device size 128MB base 0xD0000000
*==================================================
*/

#define BAIKAL_RAMDISK_PRIVATE_DATA_SIGNATURE     SIGNATURE_32 ('B', 'R', 'D', 'K')
#define BAIKAL_RAMDISK_PRIVATE_FROM_BLKIO(a)      CR (a, BAIKAL_RAMDISK_PRIVATE_DATA, BlockIo, BAIKAL_RAMDISK_PRIVATE_DATA_SIGNATURE)
#define  BAIKAL_RAMDISK_BLOCK_SIZE 512

typedef struct {
  VENDOR_DEVICE_PATH                  Vendor;
  EFI_DEVICE_PATH_PROTOCOL            End;
} BAIKAL_RAMDISK_DEVICE_PATH;

typedef struct _BAIKAL_RAMDISK_PRIVATE_DATA {
  UINTN                           Signature;
  EFI_BLOCK_IO_PROTOCOL           BlockIo;
  EFI_BLOCK_IO_MEDIA              Media;
  BAIKAL_RAMDISK_DEVICE_PATH    DevicePath;
  UINT64                          Size;
  UINTN                           StartingAddr;
} BAIKAL_RAMDISK_PRIVATE_DATA;

BAIKAL_RAMDISK_PRIVATE_DATA mBaikalRamDisk;

STATIC EFI_HANDLE mBaikalRamDiskHandle;


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
BaikalRamDiskBlkIoReset (
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
BaikalRamDiskBlkIoReadBlocks (
  IN EFI_BLOCK_IO_PROTOCOL        *This,
  IN UINT32                       MediaId,
  IN EFI_LBA                      Lba,
  IN UINTN                        BufferSize,
  OUT VOID                        *Buffer
  )
{
  BAIKAL_RAMDISK_PRIVATE_DATA           *PrivateData;
  UINTN                           NumberOfBlocks;
  if ((BufferSize == 0) && (Buffer == NULL) &&  (Lba == 0)){
   return EFI_MEDIA_CHANGED;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  PrivateData = BAIKAL_RAMDISK_PRIVATE_FROM_BLKIO (This);

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


  CopyMem (
    Buffer,
    (VOID *)(UINTN)(PrivateData->StartingAddr + MultU64x32 (Lba, PrivateData->Media.BlockSize)),
    BufferSize
  );

  return EFI_SUCCESS;
}

/**
  Write BufferSize bytes from Lba into Buffer.

  @param[in] This            Indicates a pointer to the calling context.
  @paraha[in] MediaId         The media ID that the write request is for.
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
BaikalRamDiskBlkIoWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL        *This,
  IN UINT32                       MediaId,
  IN EFI_LBA                      Lba,
  IN UINTN                        BufferSize,
  IN VOID                         *Buffer
  )
{
  BAIKAL_RAMDISK_PRIVATE_DATA           *PrivateData;
  UINTN                           NumberOfBlocks;

  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  PrivateData = BAIKAL_RAMDISK_PRIVATE_FROM_BLKIO (This);

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

  CopyMem (
    (VOID *)(UINTN)(PrivateData->StartingAddr + MultU64x32 (Lba, PrivateData->Media.BlockSize)),
    Buffer,
    BufferSize
  );

  return EFI_SUCCESS;
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
BaikalRamDiskBlkIoFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL        *This
  )
{
  return EFI_SUCCESS;
}


//
// The EFI_BLOCK_IO_PROTOCOL instances that is installed onto the handle
//
EFI_BLOCK_IO_PROTOCOL  mBaikalRamDiskBlockIoTemplate = {
  EFI_BLOCK_IO_PROTOCOL_REVISION,
  (EFI_BLOCK_IO_MEDIA *) 0,
  BaikalRamDiskBlkIoReset,
  BaikalRamDiskBlkIoReadBlocks,
  BaikalRamDiskBlkIoWriteBlocks,
  BaikalRamDiskBlkIoFlushBlocks
};


/*
*==================================================
* INIT_PIVATE
*==================================================
*/

/**
  Initialize the BlockIO protocol of a ram disk device.
  @param[in] PrivateData     Points to disk private data.
**/
EFI_STATUS
EFIAPI
BaikalRamDiskInitPivate (
  VOID
  )
{
  BAIKAL_RAMDISK_PRIVATE_DATA    *PrivateData = &mBaikalRamDisk;
  EFI_BLOCK_IO_PROTOCOL           *BlockIo;
  EFI_BLOCK_IO_MEDIA              *Media;
  BAIKAL_RAMDISK_DEVICE_PATH    dp = {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        { (UINT8)sizeof(VENDOR_DEVICE_PATH), (UINT8)((sizeof(VENDOR_DEVICE_PATH)) >> 8) }
      },
      { 0x2, 0x2, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } }, // GUID ... NEED TO BE FILLED
    },
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
    }
  }; // DevicePath

  BlockIo  = &PrivateData->BlockIo;
  Media    = &PrivateData->Media;
  PrivateData->Size = 128*1024*1024;
  PrivateData->StartingAddr = 0xD0000000;
  PrivateData->Signature = BAIKAL_RAMDISK_PRIVATE_DATA_SIGNATURE;

  CopyMem (BlockIo, &mBaikalRamDiskBlockIoTemplate, sizeof (EFI_BLOCK_IO_PROTOCOL));

  BlockIo->Media          = Media;
  Media->RemovableMedia   = TRUE;
  Media->MediaPresent     = TRUE;
  Media->LogicalPartition = FALSE;
  Media->ReadOnly         = FALSE;
  Media->WriteCaching     = FALSE;
  Media->BlockSize        = BAIKAL_RAMDISK_BLOCK_SIZE;
  Media->LastBlock        = DivU64x32 (
                              PrivateData->Size + BAIKAL_RAMDISK_BLOCK_SIZE - 1,
                              BAIKAL_RAMDISK_BLOCK_SIZE
                              ) - 1;
  PrivateData->DevicePath =  dp;

  EFI_STATUS Status;
  Status = gBS->InstallMultipleProtocolInterfaces (
    &mBaikalRamDiskHandle,
    &gEfiDevicePathProtocolGuid,  &mBaikalRamDisk.DevicePath,
    &gEfiBlockIoProtocolGuid,     &mBaikalRamDisk.BlockIo,
    NULL
  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}


/**
-----------------
 Main
-----------------
**/

EFI_STATUS
EFIAPI
BaikalRamDiskDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS Status;

  /* RAM_DISK */
  Status = BaikalRamDiskInitPivate();

  return Status;
}
