/** @file

  This file contains Baikal VDU driver functions

  Copyright (C) 2019-2020 Baikal Electronics JSC

  Author: Pavel Parkhomenko <Pavel.Parkhomenko@baikalelectronics.ru>

  Parts of this file were based on sources as follows:

  Copyright (c) 2011-2018, ARM Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/BaikalVduPlatformLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/Cpu.h>
#include <Protocol/EdidDiscovered.h>
#include <Protocol/EdidActive.h>

#include <ArmPlatform.h>
#include "BaikalHdmi.h"
#include "BaikalVdu.h"

typedef struct {
  UINT32                     Mode;
  LCD_BPP                    Bpp;
  UINT32                     OscFreq;

  SCAN_TIMINGS               Horizontal;
  SCAN_TIMINGS               Vertical;
  HDMI_PHY_SETTINGS          PhySettings;
} DISPLAY_MODE;

/** The display modes supported by the platform.
**/
STATIC DISPLAY_MODE mDisplayModes[] = {
  { // Mode 0 : VGA : 640 x 480 x 24 bpp
    VGA, LCD_BITS_PER_PIXEL_24,
    VGA_OSC_FREQUENCY,
    {VGA_H_RES_PIXELS, VGA_H_SYNC, VGA_H_BACK_PORCH, VGA_H_FRONT_PORCH},
    {VGA_V_RES_PIXELS, VGA_V_SYNC, VGA_V_BACK_PORCH, VGA_V_FRONT_PORCH},
    {VGA_OPMODE, VGA_CURRENT, VGA_GMP, VGA_TXTER, VGA_VLEVCTRL, VGA_CKSYMTXCTRL}
  },
  { // Mode 1 : SVGA : 800 x 600 x 24 bpp
    SVGA, LCD_BITS_PER_PIXEL_24,
    SVGA_OSC_FREQUENCY,
    {SVGA_H_RES_PIXELS, SVGA_H_SYNC, SVGA_H_BACK_PORCH, SVGA_H_FRONT_PORCH},
    {SVGA_V_RES_PIXELS, SVGA_V_SYNC, SVGA_V_BACK_PORCH, SVGA_V_FRONT_PORCH},
    {SVGA_OPMODE, SVGA_CURRENT, SVGA_GMP, SVGA_TXTER, SVGA_VLEVCTRL, SVGA_CKSYMTXCTRL}
  },
  { // Mode 2 : XGA : 1024 x 768 x 24 bpp
    XGA, LCD_BITS_PER_PIXEL_24,
    XGA_OSC_FREQUENCY,
    {XGA_H_RES_PIXELS, XGA_H_SYNC, XGA_H_BACK_PORCH, XGA_H_FRONT_PORCH},
    {XGA_V_RES_PIXELS, XGA_V_SYNC, XGA_V_BACK_PORCH, XGA_V_FRONT_PORCH},
    {XGA_OPMODE, XGA_CURRENT, XGA_GMP, XGA_TXTER, XGA_VLEVCTRL, XGA_CKSYMTXCTRL}
  },
  { // Mode 3 : HD720 : 1280 x 720 x 24 bpp
    HD720, LCD_BITS_PER_PIXEL_24,
    HD720_OSC_FREQUENCY,
    {HD720_H_RES_PIXELS, HD720_H_SYNC, HD720_H_BACK_PORCH, HD720_H_FRONT_PORCH},
    {HD720_V_RES_PIXELS, HD720_V_SYNC, HD720_V_BACK_PORCH, HD720_V_FRONT_PORCH},
    {HD720_OPMODE, HD720_CURRENT, HD720_GMP, HD720_TXTER, HD720_VLEVCTRL, HD720_CKSYMTXCTRL}
  },
  { // Mode 4 : WXGA : 1280 x 800 x 24 bpp
    WXGA, LCD_BITS_PER_PIXEL_24,
    WXGA_OSC_FREQUENCY,
    {WXGA_H_RES_PIXELS, WXGA_H_SYNC, WXGA_H_BACK_PORCH, WXGA_H_FRONT_PORCH},
    {WXGA_V_RES_PIXELS, WXGA_V_SYNC, WXGA_V_BACK_PORCH, WXGA_V_FRONT_PORCH},
    {WXGA_OPMODE, WXGA_CURRENT, WXGA_GMP, WXGA_TXTER, SXGA_VLEVCTRL, SXGA_CKSYMTXCTRL}
  },
  { // Mode 5 : SXGA : 1280 x 1024 x 24 bpp
    SXGA, LCD_BITS_PER_PIXEL_24,
    SXGA_OSC_FREQUENCY,
    {SXGA_H_RES_PIXELS, SXGA_H_SYNC, SXGA_H_BACK_PORCH, SXGA_H_FRONT_PORCH},
    {SXGA_V_RES_PIXELS, SXGA_V_SYNC, SXGA_V_BACK_PORCH, SXGA_V_FRONT_PORCH},
    {SXGA_OPMODE, SXGA_CURRENT, SXGA_GMP, SXGA_TXTER, SXGA_VLEVCTRL, SXGA_CKSYMTXCTRL}
  },
  { // Mode 6 : WSXGA : 1680 x 1050 x 24 bpp
    WSXGA, LCD_BITS_PER_PIXEL_24,
    WSXGA_OSC_FREQUENCY,
    {WSXGA_H_RES_PIXELS, WSXGA_H_SYNC, WSXGA_H_BACK_PORCH, WSXGA_H_FRONT_PORCH},
    {WSXGA_V_RES_PIXELS, WSXGA_V_SYNC, WSXGA_V_BACK_PORCH, WSXGA_V_FRONT_PORCH},
    {WSXGA_OPMODE, WSXGA_CURRENT, WSXGA_GMP, WSXGA_TXTER, WSXGA_VLEVCTRL, WSXGA_CKSYMTXCTRL}
  },
  { // Mode 7 : UXGA : 1600 x 1200 x 24 bpp
    UXGA, LCD_BITS_PER_PIXEL_24,
    UXGA_OSC_FREQUENCY,
    {UXGA_H_RES_PIXELS, UXGA_H_SYNC, UXGA_H_BACK_PORCH, UXGA_H_FRONT_PORCH},
    {UXGA_V_RES_PIXELS, UXGA_V_SYNC, UXGA_V_BACK_PORCH, UXGA_V_FRONT_PORCH},
    {UXGA_OPMODE, UXGA_CURRENT, UXGA_GMP, UXGA_TXTER, UXGA_VLEVCTRL, UXGA_CKSYMTXCTRL}
  },
  { // Mode 8 : FullHD : 1920 x 1080 x 24 bpp
    HD, LCD_BITS_PER_PIXEL_24,
    HD_OSC_FREQUENCY,
    {HD_H_RES_PIXELS, HD_H_SYNC, HD_H_BACK_PORCH, HD_H_FRONT_PORCH},
    {HD_V_RES_PIXELS, HD_V_SYNC, HD_V_BACK_PORCH, HD_V_FRONT_PORCH},
    {HD_OPMODE, HD_CURRENT, HD_GMP, HD_TXTER, HD_VLEVCTRL, HD_CKSYMTXCTRL}
  },
};

EFI_EDID_DISCOVERED_PROTOCOL  mEdidDiscovered = {
  0,
  NULL
};

EFI_EDID_ACTIVE_PROTOCOL      mEdidActive = {
  0,
  NULL
};

/** Baikal VDU Platform specific initialization function.

  @param[in] Handle              Handle to the LCD device instance.

  @retval EFI_SUCCESS            Plaform library initialized successfully.
  @retval EFI_UNSUPPORTED        PcdGopPixelFormat must be
                                 PixelRedGreenBlueReserved8BitPerColor OR
                                 PixelBlueGreenRedReserved8BitPerColor
                                 any other format is not supported
  @retval !(EFI_SUCCESS)         Other errors.
**/
EFI_STATUS
LcdPlatformInitializeDisplay (
  IN EFI_HANDLE   Handle
  )
{
  EFI_STATUS  Status;
  EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;

  // PixelBitMask and PixelBltOnly pixel formats are not supported
  PixelFormat = FixedPcdGet32 (PcdGopPixelFormat);
  if (PixelFormat != PixelRedGreenBlueReserved8BitPerColor &&
      PixelFormat != PixelBlueGreenRedReserved8BitPerColor) {

    ASSERT (PixelFormat == PixelRedGreenBlueReserved8BitPerColor ||
      PixelFormat == PixelBlueGreenRedReserved8BitPerColor);
    return EFI_UNSUPPORTED;
  }

  // Install the EDID Protocols
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiEdidDiscoveredProtocolGuid,
                  &mEdidDiscovered,
                  &gEfiEdidActiveProtocolGuid,
                  &mEdidActive,
                  NULL
                  );

  return Status;
}

/** Allocate VRAM memory in DRAM for the framebuffer
  (unless it is reserved already).

  The allocated address can be used to set the framebuffer.

  @param[out] VramBaseAddress     A pointer to the framebuffer address.
  @param[out] VramSize            A pointer to the size of the framebuffer
                                  in bytes

  @retval EFI_SUCCESS             Framebuffer memory allocated successfully.
  @retval !(EFI_SUCCESS)          Other errors.
**/
EFI_STATUS
LcdPlatformGetVram (
  OUT EFI_PHYSICAL_ADDRESS*  VramBaseAddress,
  OUT UINTN*                 VramSize
  )
{
  EFI_STATUS              Status;
  EFI_CPU_ARCH_PROTOCOL   *Cpu;

  //ASSERT (VramBaseAddress != NULL);
  //ASSERT (VramSize != NULL);

  *VramBaseAddress = (EFI_PHYSICAL_ADDRESS) (SIZE_4GB - 1);
  *VramSize = LCD_VRAM_SIZE;

  // Allocate the VRAM from the DRAM so that nobody else uses it.
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiBootServicesData,
                  EFI_SIZE_TO_PAGES (((UINTN)LCD_VRAM_SIZE)),
                  VramBaseAddress
                  );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Ensure the Cpu architectural protocol is already installed
  Status = gBS->LocateProtocol (
                  &gEfiCpuArchProtocolGuid,
                  NULL,
                  (VOID **)&Cpu
                  );
  if (!EFI_ERROR (Status)) {
    // The VRAM is inside the DRAM, which is cacheable.
    // Mark the VRAM as write-combining (uncached) and non-executable.
    Status = Cpu->SetMemoryAttributes (
                    Cpu,
                    *VramBaseAddress,
                    *VramSize,
                    EFI_MEMORY_WC | EFI_MEMORY_XP
                    );
  }
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    gBS->FreePages (*VramBaseAddress, EFI_SIZE_TO_PAGES (*VramSize));
  }

  return Status;
}

/** Return total number of modes supported.

  Note: Valid mode numbers are 0 to MaxMode - 1
  See Section 12.9 of the UEFI Specification 2.7

  @retval UINT32             Mode Number.
**/
UINT32
LcdPlatformGetMaxMode (VOID)
{
  // The following line would correctly report the total number
  // of graphics modes supported by the Baikal VDU.
  // return (sizeof(mResolutions) / sizeof(DISPLAY_MODE)) - 1;

  // However, on some platforms it is desirable to ignore some graphics modes.

  // Set the maximum mode allowed
  return (FixedPcdGet32(PcdVduMaxMode));
}

EFI_STATUS
LcdPlatformSetMode (
  IN UINT32                         ModeNumber
  )
{
  EFI_STATUS            Status;

  Status = EFI_SUCCESS;

  if (ModeNumber >= LcdPlatformGetMaxMode ()) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  // Set the video mode pixel frequency
  BaikalSetVduFrequency(PcdGet32 (PcdVduLcruId),
                        FixedPcdGet32 (PcdHdmiRefFrequency),
                        mDisplayModes[ModeNumber].OscFreq
                        );

  return Status;
}

/** Return information for the requested mode number.

  @param[in]  ModeNumber         Mode Number.
  @param[out] Info               Pointer for returned mode information
                                 (on success).

  @retval EFI_SUCCESS             Mode information for the requested mode
                                  returned successfully.
  @retval EFI_INVALID_PARAMETER   Requested mode not found.
**/
EFI_STATUS
LcdPlatformQueryMode (
  IN  UINT32                                ModeNumber,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info
  )
{
  ASSERT (Info != NULL);

  if (ModeNumber >= LcdPlatformGetMaxMode ()) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Info->Version = 0;
  Info->HorizontalResolution = mDisplayModes[ModeNumber].Horizontal.Resolution;
  Info->VerticalResolution = mDisplayModes[ModeNumber].Vertical.Resolution;
  Info->PixelsPerScanLine = mDisplayModes[ModeNumber].Horizontal.Resolution;

  Info->PixelFormat = FixedPcdGet32 (PcdGopPixelFormat);

  return EFI_SUCCESS;
}

/** Return display timing information for the requested mode number.

  @param[in]  ModeNumber          Mode Number.

  @param[out] Horizontal          Pointer to horizontal timing parameters.
                                  (Resolution, Sync, Back porch, Front porch)
  @param[out] Vertical            Pointer to vertical timing parameters.
                                  (Resolution, Sync, Back porch, Front porch)

  @retval EFI_SUCCESS             Display timing information for the requested
                                  mode returned successfully.
  @retval EFI_INVALID_PARAMETER   Requested mode not found.
**/
EFI_STATUS
LcdPlatformGetTimings (
  IN  UINT32                  ModeNumber,
  OUT SCAN_TIMINGS         ** Horizontal,
  OUT SCAN_TIMINGS         ** Vertical
  )
{
  // One of the pointers is NULL
  ASSERT (Horizontal != NULL);
  ASSERT (Vertical != NULL);

  if (ModeNumber >= LcdPlatformGetMaxMode ()) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  *Horizontal = &mDisplayModes[ModeNumber].Horizontal;
  *Vertical   = &mDisplayModes[ModeNumber].Vertical;

  return EFI_SUCCESS;
}

/** Return HDMI PHY settings for the requested mode number.

  @param[in]  ModeNumber          Mode Number.

  @param[out] PhySettings         Pointer to HDMI PHY settings.

  @retval EFI_SUCCESS             HDMI PHY settings for the requested
                                  mode returned successfully.
  @retval EFI_INVALID_PARAMETER   Requested mode not found.
**/
EFI_STATUS
LcdPlatformGetHdmiPhySettings (
  IN  UINT32                              ModeNumber,
  OUT HDMI_PHY_SETTINGS                ** PhySettings
  )
{

  ASSERT (PhySettings != NULL);

  if (ModeNumber >= LcdPlatformGetMaxMode ()) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  *PhySettings = &mDisplayModes[ModeNumber].PhySettings;

  return EFI_SUCCESS;
}

/** Return bits per pixel information for a mode number.

  @param[in]  ModeNumber          Mode Number.

  @param[out] Bpp                 Pointer to bits per pixel information.

  @retval EFI_SUCCESS             Bits per pixel information for the requested
                                  mode returned successfully.
  @retval EFI_INVALID_PARAMETER   Requested mode not found.
**/
EFI_STATUS
LcdPlatformGetBpp (
  IN  UINT32                              ModeNumber,
  OUT LCD_BPP  *                          Bpp
  )
{
  ASSERT (Bpp != NULL);

  if (ModeNumber >= LcdPlatformGetMaxMode ()) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  *Bpp = mDisplayModes[ModeNumber].Bpp;

  return EFI_SUCCESS;
}
