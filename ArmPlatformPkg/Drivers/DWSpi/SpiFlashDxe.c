/** @file  SpiFlashDxe.c

  Copyright (C) 2015, Baikal Electronics. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>

#include "SpiFlashDxe.h"

STATIC EFI_EVENT mSpiFlashVirtualAddrChangeEvent;

//
// Global variable declarations
//
SPI_FLASH_INSTANCE **mSpiFlashInstances;
UINT32               mSpiFlashDeviceCount;

SPI_FLASH_INSTANCE  mSpiFlashInstanceTemplate = {
  SPI_FLASH_SIGNATURE, // Signature
  NULL, // Handle ... NEED TO BE FILLED

  FALSE, // Initialized
  NULL, // Initialize

  0, // DeviceBaseAddress ... NEED TO BE FILLED
  0, // RegionBaseAddress ... NEED TO BE FILLED
  0, // Size ... NEED TO BE FILLED
  0, // StartLba

  {
    EFI_BLOCK_IO_PROTOCOL_REVISION2, // Revision
    NULL, // Media ... NEED TO BE FILLED
    0, //SpiFlashBlockIoReset, // Reset;
    0, //SpiFlashBlockIoReadBlocks,          // ReadBlocks
    0, //SpiFlashBlockIoWriteBlocks,         // WriteBlocks
    0 //SpiFlashBlockIoFlushBlocks          // FlushBlocks
  }, // BlockIoProtocol

  {
    0, // MediaId ... NEED TO BE FILLED
    FALSE, // RemovableMedia
    TRUE, // MediaPresent
    FALSE, // LogicalPartition
    FALSE, // ReadOnly
    FALSE, // WriteCaching;
    0, // BlockSize ... NEED TO BE FILLED
    4, //  IoAlign
    0, // LastBlock ... NEED TO BE FILLED
    0, // LowestAlignedLba
    1, // LogicalBlocksPerPhysicalBlock
  }, //Media;

  {
    0, //EFI_DISK_IO_PROTOCOL_REVISION, // Revision
    0, //SpiFlashDiskIoReadDisk,        // ReadDisk
    0 //SpiFlashDiskIoWriteDisk        // WriteDisk
  }, //DiskIoProtocol

  {
    SpiInit,
    SpiLock,
    SpiExecute,
    0,		// spi_port
    0		// spi_line
  }, //SpiProtocol

  FALSE, // SupportFvb ... NEED TO BE FILLED
  {
    0, //FvbGetAttributes, // GetAttributes
    0, //FvbSetAttributes, // SetAttributes
    0, //FvbGetPhysicalAddress,  // GetPhysicalAddress
    0, //FvbGetBlockSize,  // GetBlockSize
    0, //FvbRead,  // Read
    0, //FvbWrite, // Write
    0, //FvbEraseBlocks, // EraseBlocks
    NULL, //ParentHandle
  }, //  FvbProtoccol;
  NULL, // ShadowBuffer
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        { (UINT8)sizeof(VENDOR_DEVICE_PATH), (UINT8)((sizeof(VENDOR_DEVICE_PATH)) >> 8) }
      },
      { 0x0, 0x0, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } }, // GUID ... NEED TO BE FILLED
    },
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
    }
    } // DevicePath
};

EFI_STATUS
SpiFlashCreateInstance (
  IN UINTN                  SpiFlashDeviceBase,
  IN UINTN                  SpiFlashRegionBase,
  IN UINTN                  SpiFlashSize,
  IN UINT32                 MediaId,
  IN UINT32                 BlockSize,
  IN BOOLEAN                SupportFvb,
  IN CONST GUID             *SpiFlashGuid,
  OUT SPI_FLASH_INSTANCE**  SpiFlashInstance
  )
{
  EFI_STATUS Status;
  SPI_FLASH_INSTANCE* Instance;

  ASSERT(SpiFlashInstance != NULL);

  Instance = AllocateRuntimeCopyPool (sizeof(SPI_FLASH_INSTANCE),&mSpiFlashInstanceTemplate);
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Instance->DeviceBaseAddress = SpiFlashDeviceBase;
  Instance->RegionBaseAddress = SpiFlashRegionBase;
  Instance->Size = SpiFlashSize;

  Instance->BlockIoProtocol.Media = &Instance->Media;
  Instance->Media.MediaId = MediaId;
  Instance->Media.BlockSize = BlockSize;
  Instance->Media.LastBlock = (SpiFlashSize / BlockSize)-1;

  CopyGuid (&Instance->DevicePath.Vendor.Guid, SpiFlashGuid);

  Instance->ShadowBuffer = AllocateRuntimePool (BlockSize);;
  if (Instance->ShadowBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (SupportFvb) {
    Instance->SupportFvb = TRUE;
 // TODO alm   Instance->Initialize = SpiFlashFvbInitialize;

    Status = gBS->InstallMultipleProtocolInterfaces (
                  &Instance->Handle,
                  &gEfiSpiProtocolGuid, &Instance->SpiProtocol,
// TODO                 &gEfiDevicePathProtocolGuid, &Instance->DevicePath,
//                  &gEfiBlockIoProtocolGuid,  &Instance->BlockIoProtocol,
//                  &gEfiFirmwareVolumeBlockProtocolGuid, &Instance->FvbProtocol,
                  NULL
                  );
    if (EFI_ERROR(Status)) {
      FreePool (Instance);
      return Status;
    }
  } else {
    Instance->Initialized = TRUE;

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Instance->Handle,
                    &gEfiSpiProtocolGuid, &Instance->SpiProtocol,
//   TODO                 &gEfiDevicePathProtocolGuid, &Instance->DevicePath,
//                    &gEfiBlockIoProtocolGuid,  &Instance->BlockIoProtocol,
//                    &gEfiDiskIoProtocolGuid, &Instance->DiskIoProtocol,
                    NULL
                    );
    if (EFI_ERROR(Status)) {
      FreePool (Instance);
      return Status;
    }
  }

  *SpiFlashInstance = Instance;
  return Status;
}


void llenv32_spi_master_init(uint32_t spi_port, uint32_t spi_mode)
{
	uint32_t reg;

	DEBUG((EFI_D_VERBOSE, "%s: set SPI%d mode=%d\n", __FUNCTION__, spi_port, spi_mode));

	/* Disable device. */
	WRITE_SPI_REG(SPI_SSIENR(spi_port), 0);

	/* Mask all interrupts. */
	WRITE_SPI_REG(SPI_IMR(spi_port), 0);

	/* Set CTRLR0 */
	reg = (spi_mode << SPI_TMOD_OFFSET);
	reg |= (SPI_SCPOL_LOW << SPI_SCPOL_OFFSET);
	reg |= (SPI_SCPH_MIDDLE << SPI_SCPH_OFFSET);
	reg |= (SPI_FRF_SPI << SPI_FRF_OFFSET);
	reg |= SPI_DFS(8);
	WRITE_SPI_REG(SPI_CTRLR0(spi_port), reg);

	/* Specify the baud rate */
	WRITE_SPI_REG(SPI_BAUDR(spi_port), SPI_BAUDR_SCKDV);

	/* Set thresholds. */
	WRITE_SPI_REG(SPI_TXFTLR(spi_port), 48);
	WRITE_SPI_REG(SPI_RXFTLR(spi_port), 48);

	/* Enable device. */
	WRITE_SPI_REG(SPI_SSIENR(spi_port), SSIENR_SSI_EN);
}

void llenv32_set_cs(int spi_port, int line)
{
	uint32_t reg = READ_SPI_REG(SPI_SER(spi_port));

	WRITE_SPI_REG(SPI_SER(spi_port), (reg | line));
}

void llenv32_clear_cs(int spi_port, int line)
{
	uint32_t reg = READ_SPI_REG(SPI_SER(spi_port));

	WRITE_SPI_REG(SPI_SER(spi_port), (reg & (~line)));
}

int llenv32_spi_transfer_cs(int spi_port, int spi_line, uint8_t *in, int in_len, uint8_t *out, int out_len)
{
	int i;
	uint32_t reg;
	int spi_mode;

	/* Get SPI mode */
	reg = READ_SPI_REG(SPI_CTRLR0(spi_port));
	spi_mode = ((reg & SPI_TMOD_MASK) >> SPI_TMOD_OFFSET);

	if (((spi_mode == SPI_TMOD_EPROMREAD) || (spi_mode == SPI_TMOD_RO)) && (out_len)) {
		/* Disable device. CTRLR1 should be set when spi_port is disabled. */
		WRITE_SPI_REG(SPI_SSIENR(spi_port), 0);

		/* Set Number of Data Frames that should be out_len-1 */
		reg = WRITE_SPI_REG(SPI_CTRLR1(spi_port), (out_len - 1));

		/* Enable device. */
		WRITE_SPI_REG(SPI_SSIENR(spi_port), SSIENR_SSI_EN);
	}

	/* Write incomming buffer to TX FIFO. */
	for (i = 0; i < in_len; i++) {
		WRITE_SPI_DR(SPI_DR0(spi_port), in[i]);
	}

	if (spi_mode == SPI_TMOD_TR) {
		/* Write the dummy data to shift outcomming data. */
		for (i = 0; i < out_len; i++) {
			WRITE_SPI_DR(SPI_DR0(spi_port), SPI_DUMMY);
		}
	}

	/*
	 * Chip select. Enable slave right after filling TX FIFO to avoid breaking
	 * a SPI transfer.
	 */
	llenv32_set_cs(spi_port, spi_line);

	for (i = 0; i < MAX_SPI_TRIES; i++) {
		/* Get status register */
		reg = READ_SPI_REG(SPI_SR(spi_port));

		if ((reg & SPI_SR_TFE) && ((reg & SPI_SR_BUSY) == 0)) {
			break;
		}
	}

	if (i == MAX_SPI_TRIES) {
		DEBUG((EFI_D_ERROR, "SPI: transer is not completed SR=0x%x\n", reg));
		return ERROR;
	}

	if (spi_mode != SPI_TMOD_EPROMREAD) {
		/* Read dummy data. */
		for (i = 0; i < in_len; i++) {
			reg = READ_SPI_DR(SPI_DR0(spi_port));
		}
	}

	DEBUG((EFI_D_ERROR, "Spi Data: "));

	/* Read the actual data.*/
	for (i = 0; i < out_len; i++) {
		out[i] = READ_SPI_DR(SPI_DR0(spi_port));
		DEBUG((EFI_D_ERROR, "%x ", out[i]));
	}

	DEBUG((EFI_D_ERROR, "\n"));

	/* Clear chip select. */
	llenv32_clear_cs(spi_port, spi_line);

	return OK;
}

#define SPI_FIFO_SIZE	32
#define SET_ADDRESS(a, b)	do {		\
	*((UINT8*)b) = ((a >> 16) & 0xFF); 	\
	*((UINT8*)b + 1) = ((a >> 8) & 0xFF);	\
	*((UINT8*)b + 2) = (a & 0xFF);		\
} while (0)

int llenv32_spi_read_cs(int spi_port, int spi_line, uint32_t addr, uint8_t *out, int out_len)
{
	uint8_t cmd[4];

	cmd[0] = SPI_FLASH_READ;
	SET_ADDRESS(addr, &cmd[1]);

	if (llenv32_spi_transfer_cs(spi_port, spi_line, cmd, 4, out, out_len) != OK) {
		return ERROR;
	}

	return OK;
}



EFI_STATUS
SpiExecute (
  IN     EFI_SPI_PROTOCOL   * This,
  IN     UINT8              OpcodeIndex,
  IN     UINT8              PrefixOpcodeIndex,
  IN     BOOLEAN            DataCycle,
  IN     BOOLEAN            Atomic,
  IN     BOOLEAN            ShiftOut,
  IN     UINTN              Address,
  IN     UINT32             DataByteCount,
  IN OUT UINT8              *Buffer,
  IN     SPI_REGION_TYPE    SpiRegionType
  )
{
	int spi_port = 0;
	int spi_line = 1;
	int i;
	UINT8 cmd[SPI_FIFO_SIZE];

	DEBUG((EFI_D_ERROR, "SPI Exc: port=%d line=%d\n", spi_port, spi_line));
	DEBUG((EFI_D_ERROR, "SPI Exc: op=%d pref_op=%d\n", OpcodeIndex, PrefixOpcodeIndex));

	switch (OpcodeIndex) {
		case SPI_READ_ID:
			{
				cmd[0] = SPI_FLASH_RDID;
				if (llenv32_spi_transfer_cs(spi_port, spi_line, cmd, 1, Buffer, DataByteCount) != 0) {
					return EFI_DEVICE_ERROR;
				}

				break;
			}

		case SPI_READ:
			{
				if (llenv32_spi_read_cs(spi_port, spi_line, Address, Buffer, DataByteCount) != 0) {
					return EFI_DEVICE_ERROR;
				}
				break;
			}
		case SPI_RDSR:
			{
				cmd[0] = SPI_FLASH_RDSR;
				if (llenv32_spi_transfer_cs(spi_port, spi_line, cmd, 1, Buffer, DataByteCount) != 0) {
					return EFI_DEVICE_ERROR;
				}
				break;
			}

		case SPI_WRSR:
			{
				cmd[0] = SPI_FLASH_WRSR;
				cmd[1] = Buffer[0];
				if (llenv32_spi_transfer_cs(spi_port, spi_line, cmd, 2, NULL, 0) != 0) {
					return EFI_DEVICE_ERROR;
				}
				break;
			}

		case SPI_SERASE:
			{
				cmd[0] = SPI_FLASH_SSE;
				SET_ADDRESS(Address, &cmd[1]);

				if (llenv32_spi_transfer_cs(spi_port, spi_line, cmd, 4, NULL, 0) != OK) {
					return EFI_DEVICE_ERROR;
				}
				break;
			}
		case SPI_BERASE:
			{
				cmd[0] = SPI_FLASH_SE;
				SET_ADDRESS(Address, &cmd[1]);

				if (llenv32_spi_transfer_cs(spi_port, spi_line, cmd, 4, NULL, 0) != OK) {
					return EFI_DEVICE_ERROR;
				}				
				break;
			}
		case SPI_PROG:
			{

				cmd[0] = SPI_FLASH_WREN;
				if (llenv32_spi_transfer_cs(spi_port, spi_line, cmd, 1, NULL, 0) != 0) {
					return EFI_DEVICE_ERROR;
				}

				DEBUG((EFI_D_ERROR, "SPI Exc: PP address = 0x%x data size = %d\n", Address, DataByteCount));
				cmd[0] = SPI_FLASH_PP;
				SET_ADDRESS(Address, &cmd[1]);

				DEBUG((EFI_D_ERROR, "SPI Exc: PP data: "));
				for (i = 0; ((i < (SPI_FIFO_SIZE - 4)) && (i < DataByteCount)); i++) {
					cmd[i + 4] = Buffer[i];
					DEBUG((EFI_D_ERROR, "%x ", cmd[i + 4]));
				}
				DEBUG((EFI_D_ERROR, "\n i =%d\n", i));

				if (llenv32_spi_transfer_cs(spi_port, spi_line, cmd, (i + 4), NULL, 0) != OK) {
					DEBUG((EFI_D_ERROR, "SPI Exc: PP error.\n "));
					return EFI_DEVICE_ERROR;
				}
				DEBUG((EFI_D_ERROR, "SPI Exc: PP Ok.\n "));
				break;
			}
		default:
			return EFI_UNSUPPORTED;
	}
	
	return EFI_SUCCESS;
}

EFI_STATUS
SpiLock (
  IN EFI_SPI_PROTOCOL     * This
  )
{
	DEBUG((EFI_D_ERROR, "SPI Lock\n"));
	return 0;
}

EFI_STATUS
SpiInit (
  IN EFI_SPI_PROTOCOL     * This,
  IN SPI_INIT_TABLE       * InitTable
  )
{
	llenv32_spi_master_init(0, SPI_TMOD_TR);
	DEBUG((EFI_D_ERROR, "SPI Init\n"));
	return EFI_SUCCESS;
}


UINT32
SpiFlashReadStatusRegister (
  IN SPI_FLASH_INSTANCE     *Instance,
  IN UINTN                  SR_Address
  )
{
  // Prepare to read the status register
  
  return 0;
}

#if 0
STATIC
BOOLEAN
SpiFlashBlockIsLocked (
  IN SPI_FLASH_INSTANCE     *Instance,
  IN UINTN                  BlockAddress
  )
{
 
  DEBUG((EFI_D_ERROR, "SpiFlashBlockIsLocked: WARNING: Block LOCKED DOWN\n"));

  return (0);
}

STATIC
EFI_STATUS
SpiFlashUnlockSingleBlock (
  IN SPI_FLASH_INSTANCE     *Instance,
  IN UINTN                  BlockAddress
  )
{
 
  DEBUG((DEBUG_BLKIO, "UnlockSingleBlock: BlockAddress=0x%08x\n", BlockAddress));

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SpiFlashUnlockSingleBlockIfNecessary (
  IN SPI_FLASH_INSTANCE     *Instance,
  IN UINTN                  BlockAddress
  )
{
  EFI_STATUS Status;

  Status = EFI_SUCCESS;

  return Status;
}


/**
 * The following function presumes that the block has already been unlocked.
 **/
STATIC
EFI_STATUS
SpiFlashEraseSingleBlock (
  IN SPI_FLASH_INSTANCE     *Instance,
  IN UINTN                  BlockAddress
  )
{
  EFI_STATUS            Status;

  Status = EFI_SUCCESS;

  return Status;
}

/**
 * This function unlock and erase an entire SPI Flash block.
 **/
EFI_STATUS
SpiFlashUnlockAndEraseSingleBlock (
  IN SPI_FLASH_INSTANCE     *Instance,
  IN UINTN                  BlockAddress
  )
{
  EFI_STATUS      Status = EFI_SUCCESS;

  return Status;
}



STATIC
EFI_STATUS
SpiFlashWriteSingleWord (
  IN SPI_FLASH_INSTANCE     *Instance,
  IN UINTN                  WordAddress,
  IN UINT32                 WriteData
  )
{
  EFI_STATUS            Status;

  Status = EFI_SUCCESS;


  return Status;
}

#endif /* 0/1 */

/*
 * Writes data to the SPI Flash using the Buffered Programming method.
 *
 * The maximum size of the on-chip buffer is 32-words, because of hardware restrictions.
 * Therefore this function will only handle buffers up to 32 words or 128 bytes.
 * To deal with larger buffers, call this function again.
 *
 * This function presumes that both the TargetAddress and the TargetAddress+BufferSize
 * exist entirely within the SPI Flash. Therefore these conditions will not be checked here.
 *
 * In buffered programming, if the target address not at the beginning of a 32-bit word boundary,
 * then programming time is doubled and power consumption is increased.
 * Therefore, it is a requirement to align buffer writes to 32-bit word boundaries.
 * i.e. the last 4 bits of the target start address must be zero: 0x......00
 */
EFI_STATUS
SpiFlashWriteBuffer (
  IN SPI_FLASH_INSTANCE     *Instance,
  IN UINTN                  TargetAddress,
  IN UINTN                  BufferSizeInBytes,
  IN UINT32                 *Buffer
  )
{
  EFI_STATUS            Status = 0;


  return Status;
}

#if 0

STATIC
EFI_STATUS
SpiFlashWriteFullBlock (
  IN SPI_FLASH_INSTANCE     *Instance,
  IN EFI_LBA                Lba,
  IN UINT32                 *DataBuffer,
  IN UINT32                 BlockSizeInWords
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;

  return Status;
}
#endif

EFI_STATUS
SpiFlashWriteBlocks (
  IN SPI_FLASH_INSTANCE     *Instance,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSizeInBytes,
  IN VOID                   *Buffer
  )
{
  EFI_STATUS      Status = EFI_SUCCESS;

  DEBUG((DEBUG_BLKIO, "SpiFlashWriteBlocks: Exit Status = \"%r\".\n", Status));
  return Status;
}

EFI_STATUS
SpiFlashReadBlocks (
  IN SPI_FLASH_INSTANCE   *Instance,
  IN EFI_LBA              Lba,
  IN UINTN                BufferSizeInBytes,
  OUT VOID                *Buffer
  )
{

  return EFI_SUCCESS;
}

EFI_STATUS
SpiFlashRead (
  IN SPI_FLASH_INSTANCE   *Instance,
  IN EFI_LBA              Lba,
  IN UINTN                Offset,
  IN UINTN                BufferSizeInBytes,
  OUT VOID                *Buffer
  )
{

  return EFI_SUCCESS;
}

/*
  Write a full or portion of a block. It must not span block boundaries; that is,
  Offset + *NumBytes <= Instance->Media.BlockSize.
*/
EFI_STATUS
SpiFlashWriteSingleBlock (
  IN        SPI_FLASH_INSTANCE   *Instance,
  IN        EFI_LBA               Lba,
  IN        UINTN                 Offset,
  IN OUT    UINTN                *NumBytes,
  IN        UINT8                *Buffer
  )
{

  return EFI_SUCCESS;
}

/*
  Although DiskIoDxe will automatically install the DiskIO protocol whenever
  we install the BlockIO protocol, its implementation is sub-optimal as it reads
  and writes entire blocks using the BlockIO protocol. In fact we can access
  SPI flash with a finer granularity than that, so we can improve performance
  by directly producing the DiskIO protocol.
*/

/**
  Read BufferSize bytes from Offset into Buffer.

  @param  This                  Protocol instance pointer.
  @param  MediaId               Id of the media, changes every time the media is replaced.
  @param  Offset                The starting byte offset to read from
  @param  BufferSize            Size of Buffer
  @param  Buffer                Buffer containing read data

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_INVALID_PARAMETER The read request contains device addresses that are not
                                valid for the device.

**/
EFI_STATUS
EFIAPI
SpiFlashDiskIoReadDisk (
  IN EFI_DISK_IO_PROTOCOL         *This,
  IN UINT32                       MediaId,
  IN UINT64                       DiskOffset,
  IN UINTN                        BufferSize,
  OUT VOID                        *Buffer
  )
{

  SPI_FLASH_INSTANCE *Instance;
  UINT32              BlockSize;
  UINT32              BlockOffset;
  EFI_LBA             Lba;

  Instance = INSTANCE_FROM_DISKIO_THIS(This);

  if (MediaId != Instance->Media.MediaId) {
    return EFI_MEDIA_CHANGED;
  }

  BlockSize = Instance->Media.BlockSize;
  Lba = (EFI_LBA) DivU64x32Remainder (DiskOffset, BlockSize, &BlockOffset);

  return SpiFlashRead (Instance, Lba, BlockOffset, BufferSize, Buffer);
}

/**
  Writes a specified number of bytes to a device.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    ID of the medium to be written.
  @param  Offset     The starting byte offset on the logical block I/O device to write.
  @param  BufferSize The size in bytes of Buffer. The number of bytes to write to the device.
  @param  Buffer     A pointer to the buffer containing the data to be written.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_INVALID_PARAMETER The write request contains device addresses that are not
                                 valid for the device.

**/
EFI_STATUS
EFIAPI
SpiFlashDiskIoWriteDisk (
  IN EFI_DISK_IO_PROTOCOL         *This,
  IN UINT32                       MediaId,
  IN UINT64                       DiskOffset,
  IN UINTN                        BufferSize,
  IN VOID                         *Buffer
  )
{
  return 0;
}

EFI_STATUS
SpiFlashReset (
  IN  SPI_FLASH_INSTANCE *Instance
  )
{
  return EFI_SUCCESS;
}

/**
  Fixup internal data so that EFI can be call in virtual mode.
  Call the passed in Child Notify event and convert any pointers in
  lib to virtual mode.

  @param[in]    Event   The Event that is being processed
  @param[in]    Context Event Context
**/
VOID
EFIAPI
SpiFlashVirtualNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
#if 0
  UINTN Index;

  for (Index = 0; Index < mSpiFlashDeviceCount; Index++) {
    EfiConvertPointer (0x0, (VOID**)&mSpiFlashInstances[Index]->DeviceBaseAddress);
    EfiConvertPointer (0x0, (VOID**)&mSpiFlashInstances[Index]->RegionBaseAddress);

    // Convert BlockIo protocol
    EfiConvertPointer (0x0, (VOID**)&mSpiFlashInstances[Index]->BlockIoProtocol.FlushBlocks);
    EfiConvertPointer (0x0, (VOID**)&mSpiFlashInstances[Index]->BlockIoProtocol.ReadBlocks);
    EfiConvertPointer (0x0, (VOID**)&mSpiFlashInstances[Index]->BlockIoProtocol.Reset);
    EfiConvertPointer (0x0, (VOID**)&mSpiFlashInstances[Index]->BlockIoProtocol.WriteBlocks);

    // Convert Fvb
    EfiConvertPointer (0x0, (VOID**)&mSpiFlashInstances[Index]->FvbProtocol.EraseBlocks);
    EfiConvertPointer (0x0, (VOID**)&mSpiFlashInstances[Index]->FvbProtocol.GetAttributes);
    EfiConvertPointer (0x0, (VOID**)&mSpiFlashInstances[Index]->FvbProtocol.GetBlockSize);
    EfiConvertPointer (0x0, (VOID**)&mSpiFlashInstances[Index]->FvbProtocol.GetPhysicalAddress);
    EfiConvertPointer (0x0, (VOID**)&mSpiFlashInstances[Index]->FvbProtocol.Read);
    EfiConvertPointer (0x0, (VOID**)&mSpiFlashInstances[Index]->FvbProtocol.SetAttributes);
    EfiConvertPointer (0x0, (VOID**)&mSpiFlashInstances[Index]->FvbProtocol.Write);

    if (mSpiFlashInstances[Index]->ShadowBuffer != NULL) {
      EfiConvertPointer (0x0, (VOID**)&mSpiFlashInstances[Index]->ShadowBuffer);
    }
  }
#endif
  return;
}

EFI_STATUS
EFIAPI
SpiFlashInitialise (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS              Status;
  UINT32                  Index;
  SPI_FLASH_DESCRIPTION*  SpiFlashDevices;
  BOOLEAN                 ContainVariableStorage;

  Status = SpiFlashPlatformInitialization ();
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR,"SpiFlashInitialise: Fail to initialize Spi Flash devices\n"));
    return Status;
  }

  Status = SpiFlashPlatformGetDevices (&SpiFlashDevices, &mSpiFlashDeviceCount);
  if (EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR,"SpiFlashInitialise: Fail to get Spi Flash devices\n"));
    return Status;
  }

  mSpiFlashInstances = AllocateRuntimePool (sizeof(SPI_FLASH_INSTANCE*) * mSpiFlashDeviceCount);

  for (Index = 0; Index < mSpiFlashDeviceCount; Index++) {
    // Check if this SPI Flash device contain the variable storage region
    ContainVariableStorage =
        (SpiFlashDevices[Index].RegionBaseAddress <= PcdGet32 (PcdFlashNvStorageVariableBase)) &&
        (PcdGet32 (PcdFlashNvStorageVariableBase) + PcdGet32 (PcdFlashNvStorageVariableSize) <= SpiFlashDevices[Index].RegionBaseAddress + SpiFlashDevices[Index].Size);

    Status = SpiFlashCreateInstance (
      SpiFlashDevices[Index].DeviceBaseAddress,
      SpiFlashDevices[Index].RegionBaseAddress,
      SpiFlashDevices[Index].Size,
      Index,
      SpiFlashDevices[Index].BlockSize,
      ContainVariableStorage,
      &SpiFlashDevices[Index].Guid,
      &mSpiFlashInstances[Index]
    );
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR,"SpiFlashInitialise: Fail to create instance for SpiFlash[%d]\n",Index));
    }
  }

  //
  // Register for the virtual address change event
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  SpiFlashVirtualNotifyEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mSpiFlashVirtualAddrChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
