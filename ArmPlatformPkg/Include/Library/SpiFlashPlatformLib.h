/** @file

 Copyright (C) 2015, Baikal Electronics. All rights reserved.

 This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/

#ifndef _SPIFLASHPLATFORMLIB_H_
#define _SPIFLASHPLATFORMLIB_H_

typedef struct {
  UINTN       DeviceBaseAddress;    // Start address of the Device Base Address (DBA)
  UINTN       RegionBaseAddress;    // Start address of one single region
  UINTN       Size;
  UINTN       BlockSize;
  EFI_GUID    Guid;
} SPI_FLASH_DESCRIPTION;

EFI_STATUS
SpiFlashPlatformInitialization (
  VOID
  );

EFI_STATUS
SpiFlashPlatformGetDevices (
  OUT SPI_FLASH_DESCRIPTION   **SpiFlashDescriptions,
  OUT UINT32                  *Count
  );

#endif /* _SPIFLASHPLATFORMLIB_H_ */
