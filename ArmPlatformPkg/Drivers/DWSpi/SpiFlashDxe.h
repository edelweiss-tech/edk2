/** @file  SpiFlashDxe.h

  Copyright (C) 2015, Baikal Electronics. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __SPI_FLASH_DXE_H__
#define __SPI_FLASH_DXE_H__

#include <stdint.h>
#include <Base.h>
#include <PiDxe.h>

#include <Guid/EventGroup.h>

#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>
#include <Protocol/FirmwareVolumeBlock.h>

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/SpiFlashPlatformLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeLib.h>
#include "Spi.h"

#define SPI0_BASE	0x00157a0000
#define SPI1_BASE	0x00157b0000
#define SPI_BASE	SPI0_BASE


#define SPI_FLASH_ERASE_RETRY                     10

// Device access macros
// These are necessary because we use 2 x 16bit parts to make up 32bit data

#define HIGH_16_BITS                              0xFFFF0000
#define LOW_16_BITS                               0x0000FFFF
#define LOW_8_BITS                                0x000000FF

#define FOLD_32BIT_INTO_16BIT(value)              ( ( value >> 16 ) | ( value & LOW_16_BITS ) )

#define GET_LOW_BYTE(value)                       ( value & LOW_8_BITS )
#define GET_HIGH_BYTE(value)                      ( GET_LOW_BYTE( value >> 16 ) )

// Each command must be sent simultaneously to both chips,
// i.e. at the lower 16 bits AND at the higher 16 bits
#define CREATE_SPI_ADDRESS(BaseAddr,OffsetAddr)   ((BaseAddr) + ((OffsetAddr) << 2))
#define CREATE_DUAL_CMD(Cmd)                      ( ( Cmd << 16) | ( Cmd & LOW_16_BITS) )
#define SEND_SPI_COMMAND(BaseAddr,Offset,Cmd) MmioWrite32 (CREATE_SPI_ADDRESS(BaseAddr,Offset), CREATE_DUAL_CMD(Cmd))
#define GET_SPI_BLOCK_ADDRESS(BaseAddr,Lba,LbaSize)( BaseAddr + (UINTN)((Lba) * LbaSize) )


#define SPI_FLASH_SIGNATURE                       SIGNATURE_32('s', 'p', 'i', '0')
#define INSTANCE_FROM_FVB_THIS(a)                 CR(a, SPI_FLASH_INSTANCE, FvbProtocol, SPI_FLASH_SIGNATURE)
#define INSTANCE_FROM_BLKIO_THIS(a)               CR(a, SPI_FLASH_INSTANCE, BlockIoProtocol, SPI_FLASH_SIGNATURE)
#define INSTANCE_FROM_DISKIO_THIS(a)              CR(a, SPI_FLASH_INSTANCE, DiskIoProtocol, SPI_FLASH_SIGNATURE)

typedef struct _SPI_FLASH_INSTANCE                SPI_FLASH_INSTANCE;

typedef EFI_STATUS (*SPI_FLASH_INITIALIZE)        (SPI_FLASH_INSTANCE* Instance);

typedef struct {
  VENDOR_DEVICE_PATH                  Vendor;
  EFI_DEVICE_PATH_PROTOCOL            End;
} SPI_FLASH_DEVICE_PATH;

struct _SPI_FLASH_INSTANCE {
  UINT32                              Signature;
  EFI_HANDLE                          Handle;

  BOOLEAN                             Initialized;
  SPI_FLASH_INITIALIZE                Initialize;

  UINTN                               DeviceBaseAddress;
  UINTN                               RegionBaseAddress;
  UINTN                               Size;
  EFI_LBA                             StartLba;

  EFI_BLOCK_IO_PROTOCOL               BlockIoProtocol;
  EFI_BLOCK_IO_MEDIA                  Media;
  EFI_DISK_IO_PROTOCOL                DiskIoProtocol;
  EFI_SPI_PROTOCOL                    SpiProtocol;

  BOOLEAN                             SupportFvb;
  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL FvbProtocol;
  VOID*                               ShadowBuffer;

  SPI_FLASH_DEVICE_PATH               DevicePath;
};


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
  );

EFI_STATUS
SpiLock (
  IN EFI_SPI_PROTOCOL     * This
  );

EFI_STATUS
SpiInit (
  IN EFI_SPI_PROTOCOL     * This,
  IN SPI_INIT_TABLE       * InitTable
  );

EFI_STATUS
SpiFlashReadCfiData (
  IN  UINTN                   DeviceBaseAddress,
  IN  UINTN                   CFI_Offset,
  IN  UINT32                  NumberOfBytes,
  OUT UINT32                  *Data
  );

EFI_STATUS
SpiFlashWriteBuffer (
  IN SPI_FLASH_INSTANCE     *Instance,
  IN UINTN                  TargetAddress,
  IN UINTN                  BufferSizeInBytes,
  IN UINT32                 *Buffer
  );

//
// BlockIO Protocol function EFI_BLOCK_IO_PROTOCOL.Reset
//
EFI_STATUS
EFIAPI
SpiFlashBlockIoReset (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN BOOLEAN                  ExtendedVerification
  );

//
// BlockIO Protocol function EFI_BLOCK_IO_PROTOCOL.ReadBlocks
//
EFI_STATUS
EFIAPI
SpiFlashBlockIoReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 Lba,
  IN  UINTN                   BufferSizeInBytes,
  OUT VOID                    *Buffer
);

//
// BlockIO Protocol function EFI_BLOCK_IO_PROTOCOL.WriteBlocks
//
EFI_STATUS
EFIAPI
SpiFlashBlockIoWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 Lba,
  IN  UINTN                   BufferSizeInBytes,
  IN  VOID                    *Buffer
);

//
// BlockIO Protocol function EFI_BLOCK_IO_PROTOCOL.FlushBlocks
//
EFI_STATUS
EFIAPI
SpiFlashBlockIoFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL    *This
);

//
// DiskIO Protocol function EFI_DISK_IO_PROTOCOL.ReadDisk
//
EFI_STATUS
EFIAPI
SpiFlashDiskIoReadDisk (
  IN EFI_DISK_IO_PROTOCOL         *This,
  IN UINT32                       MediaId,
  IN UINT64                       Offset,
  IN UINTN                        BufferSize,
  OUT VOID                        *Buffer
  );

//
// DiskIO Protocol function EFI_DISK_IO_PROTOCOL.WriteDisk
//
EFI_STATUS
EFIAPI
SpiFlashDiskIoWriteDisk (
  IN EFI_DISK_IO_PROTOCOL         *This,
  IN UINT32                       MediaId,
  IN UINT64                       Offset,
  IN UINTN                        BufferSize,
  IN VOID                         *Buffer
  );

//
// SpiFlashFvbDxe.c
//

EFI_STATUS
EFIAPI
SpiFlashFvbInitialize (
  IN SPI_FLASH_INSTANCE*                            Instance
  );

EFI_STATUS
EFIAPI
FvbGetAttributes(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL     *This,
  OUT       EFI_FVB_ATTRIBUTES_2                    *Attributes
  );

EFI_STATUS
EFIAPI
FvbSetAttributes(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL     *This,
  IN OUT    EFI_FVB_ATTRIBUTES_2                    *Attributes
  );

EFI_STATUS
EFIAPI
FvbGetPhysicalAddress(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL     *This,
  OUT       EFI_PHYSICAL_ADDRESS                    *Address
  );

EFI_STATUS
EFIAPI
FvbGetBlockSize(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL     *This,
  IN        EFI_LBA                                 Lba,
  OUT       UINTN                                   *BlockSize,
  OUT       UINTN                                   *NumberOfBlocks
  );

EFI_STATUS
EFIAPI
FvbRead(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL     *This,
  IN        EFI_LBA                                 Lba,
  IN        UINTN                                   Offset,
  IN OUT    UINTN                                   *NumBytes,
  IN OUT    UINT8                                   *Buffer
  );

EFI_STATUS
EFIAPI
FvbWrite(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL     *This,
  IN        EFI_LBA                                 Lba,
  IN        UINTN                                   Offset,
  IN OUT    UINTN                                   *NumBytes,
  IN        UINT8                                   *Buffer
  );

EFI_STATUS
EFIAPI
FvbEraseBlocks(
  IN CONST  EFI_FIRMWARE_VOLUME_BLOCK2_PROTOCOL     *This,
  ...
  );

//
// SpiFlashDxe.c
//

EFI_STATUS
SpiFlashUnlockAndEraseSingleBlock (
  IN SPI_FLASH_INSTANCE     *Instance,
  IN UINTN                  BlockAddress
  );

EFI_STATUS
SpiFlashWriteSingleBlock (
  IN        SPI_FLASH_INSTANCE   *Instance,
  IN        EFI_LBA               Lba,
  IN        UINTN                 Offset,
  IN OUT    UINTN                *NumBytes,
  IN        UINT8                *Buffer
  );

EFI_STATUS
SpiFlashWriteBlocks (
  IN  SPI_FLASH_INSTANCE *Instance,
  IN  EFI_LBA           Lba,
  IN  UINTN             BufferSizeInBytes,
  IN  VOID              *Buffer
  );

EFI_STATUS
SpiFlashReadBlocks (
  IN SPI_FLASH_INSTANCE   *Instance,
  IN EFI_LBA              Lba,
  IN UINTN                BufferSizeInBytes,
  OUT VOID                *Buffer
  );

EFI_STATUS
SpiFlashRead (
  IN SPI_FLASH_INSTANCE   *Instance,
  IN EFI_LBA              Lba,
  IN UINTN                Offset,
  IN UINTN                BufferSizeInBytes,
  OUT VOID                *Buffer
  );

EFI_STATUS
SpiFlashWrite (
  IN        SPI_FLASH_INSTANCE   *Instance,
  IN        EFI_LBA               Lba,
  IN        UINTN                 Offset,
  IN OUT    UINTN                *NumBytes,
  IN        UINT8                *Buffer
  );

EFI_STATUS
SpiFlashReset (
  IN  SPI_FLASH_INSTANCE *Instance
  );


typedef enum {
  SPI_READ_ID,        // Opcode 0: READ ID, Read cycle with address.
  SPI_READ,           // Opcode 1: READ, Read cycle with address.
  SPI_RDSR,           // Opcode 2: Read Status Register, No address.
  SPI_WRDI_SFDP,      // Opcode 3: Write Disable or Discovery Parameters, No address.
  SPI_SERASE,         // Opcode 4: Sector Erase (4KB), Write cycle with address.
  SPI_BERASE,         // Opcode 5: Block Erase (32KB), Write cycle with address.
  SPI_PROG,           // Opcode 6: Byte Program, Write cycle with address.
  SPI_WRSR,           // Opcode 7: Write Status Register, No address.
} SPI_OPCODE_INDEX; // TODO TBD


#define	OK	0
#define ERROR	-1

#define SPI_OFFSET		0x10000
#define SPI_PORT(p)		(SPI_BASE + (p * SPI_OFFSET))

#define SPI_CTRLR0(p)	(SPI_PORT(p) + 0x0) /* Control Register 0  */
#define SPI_CTRLR1(p)	(SPI_PORT(p) + 0x4) /* Control Register 1  */
#define SPI_SSIENR(p)	(SPI_PORT(p) + 0x8) /* SSI Enable Register */
#define SPI_MWCR(p)	(SPI_PORT(p) + 0xc) /* Microwire Control Register.  */
#define SPI_SER(p)	(SPI_PORT(p) + 0x10) /* Slave Enable Register.  */
#define SPI_BAUDR(p)	(SPI_PORT(p) + 0x14) /* Baud Rate Select.  */
#define SPI_TXFTLR(p)	(SPI_PORT(p) + 0x18) /* Transmit FIFO Threshold Level.  */
#define SPI_RXFTLR(p)	(SPI_PORT(p) + 0x1c) /* Receive FIFO Threshold level.  */
#define SPI_TXFLR(p)	(SPI_PORT(p) + 0x20) /* Transmit FIFO Level Register */
#define SPI_RXFLR(p)	(SPI_PORT(p) + 0x24) /* Receive FIFO Level Register */
#define SPI_SR(p)	(SPI_PORT(p) + 0x28) /* Status Register */
#define SPI_IMR(p)	(SPI_PORT(p) + 0x2c) /* Interrupt Mask Register */
#define SPI_IS(p)	(SPI_PORT(p) + 0x30) /* Interrupt Status Register */
#define SPI_RISR(p)	(SPI_PORT(p) + 0x34) /* Raw Interrupt StatusRegister */
#define SPI_TXOICR(p)	(SPI_PORT(p) + 0x38) /* Transmit FIFO Overflow Interrupt Clear Register */
#define SPI_RXOICR(p)	(SPI_PORT(p) + 0x3c) /* Receive FIFO Overflow Interrupt Clear Register */
#define SPI_RXUICR(p)	(SPI_PORT(p) + 0x40) /* Receive FIFO Underflow Interrupt Clear Register */
#define SPI_MSTICR(p)	(SPI_PORT(p) + 0x44) /* Multi-Master Interrupt Clear Register */
#define SPI_ICR(p)	(SPI_PORT(p) + 0x48) /* Interrupt Clear Register */
#define SPI_DMACR(p)	(SPI_PORT(p) + 0x4c) /* DMA Control Register.  */
#define SPI_DMATDLR(p)	(SPI_PORT(p) + 0x50) /* DMA Transmit Data Level.  */
#define SPI_DMARDLR(p)	(SPI_PORT(p) + 0x54) /* DMA Receive Data Level.  */
#define SPI_IDR(p)	(SPI_PORT(p) + 0x58) /* Identification Register.  */
#define SPI_SSI_VERSION_ID(p) (SPI_PORT(p) + 0x5c) /* coreKit Version ID Register */
#define SPI_DR0(p)	(SPI_PORT(p) + 0x60) /* A 16-bit read/write buffer for the transmit/receive FIFOs. */
#define SPI_DR35(p)	(SPI_PORT(p) + 0xec) /* A 16-bit read/write buffer for the transmit/receive FIFOs. */
#define SPI_RX_SAMPLE_DLY(p) (SPI_PORT(p) + 0xf0) /* RX Sample Delay. */
#define SPI_RSVD_0(p)	(SPI_PORT(p) + 0xf4) /* RSVD_0 - Reserved address location */
#define SPI_RSVD_1(p)	(SPI_PORT(p) + 0xf8) /* RSVD_1 - Reserved address location */
#define SPI_RSVD_2(p)	(SPI_PORT(p) + 0xfc) /* RSVD_2 - Reserved address location */

#define SPI_IDR_VAL	0x0000abf0
#define SPI_SSI_VERSION_ID_VAL	0x3332322a

/* Enable bit. */
#define SSIENR_SSI_EN (1 << 0)

/* Transfer mode bits */
#define SPI_TMOD_OFFSET			8
#define SPI_TMOD_MASK			(0x3 << SPI_TMOD_OFFSET)
#define SPI_TMOD_TR			0x0 /* transmit and receive */
#define SPI_TMOD_TO			0x1 /* transmit only */
#define SPI_TMOD_RO			0x2 /* receive only */
#define SPI_TMOD_EPROMREAD		0x3 /* EEPROM read */

/* Slave output enable. */
#define SPI_SLV_OE_OFFSET		10
#define SPI_SLV_OE			1

/* Serial Clock Polarity. */
#define SPI_SCPOL_OFFSET		7
#define SPI_SCPOL_LOW			0
#define SPI_SCPOL_HIGH			1	

/* Serial Clock Phase. */
#define SPI_SCPH_OFFSET			6
#define SPI_SCPH_MIDDLE			0
#define SPI_SCPH_START			1

/* Frame format. */
#define SPI_FRF_OFFSET			4
#define SPI_FRF_SPI			0x0
#define SPI_FRF_SSP			0x1
#define SPI_FRF_MICROWIRE		0x2
#define SPI_FRF_RESV			0x3

/* Data Frame Size */
#define SPI_DFS(x)			(x - 1)

/* Status register bits. */
#define SPI_SR_DCOL	(1 << 6)	/* Data Collision Error */
#define SPI_SR_TXE	(1 << 5)	/* Transmition Error. */
#define SPI_SR_RFF	(1 << 4)	/* Receive FIFO Full */
#define SPI_SR_RFNE	(1 << 3)	/* Receive FIFO Not Empty */
#define SPI_SR_TFE	(1 << 2)	/* Transmit FIFO Empty */
#define SPI_SR_TFNF	(1 << 1)	/* Transmit FIFO Not Full */
#define SPI_SR_BUSY	(1 << 0)	/* SSI Busy Flag. */

#define SPI_DUMMY 0

/* Define SPI flash commands. */
#define	SPI_FLASH_RDID		0x9F	/* Read identification. */
#define	SPI_FLASH_READ		0x03	/* Read Data Bytes */
#define	SPI_FLASH_WREN		0x06	/* Write Enable */
#define	SPI_FLASH_WRDI		0x04	/* Write Disable */
#define	SPI_FLASH_PP		0x02	/* Page Program */
#define SPI_FLASH_SSE		0x20	/* SubSector Erase */
#define	SPI_FLASH_SE		0xD8	/* Sector Erase */
#define SPI_FLASH_RDSR		0x05	/* Read Status Register */
#define SPI_FLASH_WRSR		0x01	/* Write Status Register */
#define SPI_FLASH_RDLR		0xE8	/* Read Lock Register */
#define SPI_FLASH_WRLR		0xE5	/* Write Lock Register */
#define SPI_FLASH_RSTEN		0x66	/* Reset Enable */
#define SPI_FLASH_RST		0x99	/* Reset Memory */

#define JEDEC_DATA	20

#define MAX_SPI_TRIES	100000 //10000

#define SLAVE_LINE(line)	(1 << (line))
#define SLAVE_MAX_LINE		3

typedef struct {
	unsigned int mid;
	unsigned int dev_type;
	unsigned int dev_cap;
	char	name[10];
} llenv32_spi_flashes_t;

/* Define the default baud rate and thresholds. */
#define SPI_BAUDR_SCKDV			4
/* 
 * The threshold should not be greater or equal than TX/RX FIFOs
 * According to the tmp_DW_apb_ssi specification the RX/TX FIFO
 * depths are equal 0x8.
 */
#define SPI_TH_TX			0x4
#define SPI_TH_RX			0x4

#define READ_SPI_REG(r)       (*((volatile uint32_t *) (uint64_t)(r)))
#define WRITE_SPI_REG(r, v)   (*((volatile uint32_t *) (uint64_t)(r)) = v)

#define READ_SPI_DR(r)		READ_SPI_REG(r)
#define WRITE_SPI_DR(r, v)	WRITE_SPI_REG(r, v)


#define SPI_PORT0	0
#define SPI_PORT1	1

#define SPI_PORT_START	SPI_PORT0
#define SPI_PORT_END	SPI_PORT1



#endif /* __SPI_FLASH_DXE_H__ */
