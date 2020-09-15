/** @file

  This file contains Baikal VDU driver functions

  Copyright (C) 2019-2020 Baikal Electronics JSC

  Author: Pavel Parkhomenko <Pavel.Parkhomenko@baikalelectronics.ru>

  Parts of this file were based on sources as follows:

  Copyright (c) 2011-2018, ARM Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/ArmSmcLib.h>
#include <Library/PcdLib.h>

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/LcdHwLib.h>
#include <Library/BaikalVduPlatformLib.h>
#include <Library/TimerLib.h>

#include <Library/MemoryAllocationLib.h>

#include "BaikalHdmi.h"
#include "BaikalVdu.h"

#define BAIKAL_LCRU_LVDS_ID     0x20010000
#define BAIKAL_LCRU_HDMI_ID     0x30010000
#define BAIKAL_VDU_LVDS_BASE    0x202d0000
#define BAIKAL_VDU_HDMI_BASE    0x30260000

/* LCRU */
#define BAIKAL_SMC_LCRU		    0x82000000
#define BAIKAL_SMC_LCRU_SET     0
#define BAIKAL_SMC_LCRU_ENABLE  2
#define BAIKAL_SMC_LCRU_DISABLE 3

VOID
BaikalSetVduFrequency(
  UINT32 LcruId,
  UINT32 RefFreq,
  UINT32 OscFreq
  )
{
  ARM_SMC_ARGS Args;
  Args.Arg0 = BAIKAL_SMC_LCRU;
  Args.Arg1 = LcruId;
  Args.Arg2 = BAIKAL_SMC_LCRU_SET;
  Args.Arg3 = OscFreq;
  Args.Arg4 = RefFreq;
  ArmCallSmc(&Args);
}

/* -----------------------------------------------------------------------------
 * Synopsys PHY Handling
 */

BOOLEAN
HdmiPhyWaitI2CDone(
  IN UINT32 Timeout
  )
{
    UINT32 Val;

    while ((Val = MmioRead32(BAIKAL_HDMI_IH_I2CMPHY_STAT0) & 0x3) == 0) {
        if (Timeout-- == 0)
            return FALSE;
        MicroSecondDelay(1000);
    }
    MmioWrite32(BAIKAL_HDMI_IH_I2CMPHY_STAT0, Val);

    return TRUE;
}

VOID
HdmiPhyI2CWrite(
  IN UINT8 Addr,
  IN UINT16 Data
  )
{
  MmioWrite32(BAIKAL_HDMI_IH_I2CMPHY_STAT0, 0xFF);
  MmioWrite32(BAIKAL_HDMI_PHY_I2CM_ADDRESS_ADDR, Addr);
  MmioWrite32(BAIKAL_HDMI_PHY_I2CM_DATAO_1_ADDR, Data >> 8);
  MmioWrite32(BAIKAL_HDMI_PHY_I2CM_DATAO_0_ADDR, (Data >> 0) & 0xFF) ;
  MmioWrite32(BAIKAL_HDMI_PHY_I2CM_OPERATION_ADDR, BAIKAL_HDMI_PHY_I2CM_OPERATION_ADDR_WRITE);
  HdmiPhyWaitI2CDone(1000);
}

VOID
HdmiPhyPowerOff(
  VOID
  )
{
  UINTN i;
  UINT32 Val;
  
  Val = MmioRead32(BAIKAL_HDMI_PHY_CONF0);
  Val &= ~BAIKAL_HDMI_PHY_CONF0_GEN2_TXPWRON_MASK;
  MmioWrite32(BAIKAL_HDMI_PHY_CONF0, Val);

  /*
   * Wait for TX_PHY_LOCK to be deasserted to indicate that the PHY went
   * to low power mode.
   */ 
  for (i = 0; i < 5; ++i) {
    Val = MmioRead32(BAIKAL_HDMI_PHY_STAT0);
    if (!(Val & BAIKAL_HDMI_PHY_TX_PHY_LOCK))
      break;

    MicroSecondDelay(1000);
  }

  if (Val & BAIKAL_HDMI_PHY_TX_PHY_LOCK)
    DEBUG((DEBUG_ERROR, "HDMI PHY failed to power down\n"));
  else
    DEBUG((DEBUG_INFO, "HDMI PHY powered down in %u iterations\n", i));

  Val = MmioRead32(BAIKAL_HDMI_PHY_CONF0);
  Val |= BAIKAL_HDMI_PHY_CONF0_GEN2_PDDQ_MASK;
  MmioWrite32(BAIKAL_HDMI_PHY_CONF0, Val);

}

EFI_STATUS
HdmiPhyPowerOn(
  VOID
  )
{
  UINTN i;
  UINT32 Val;

  Val = MmioRead32(BAIKAL_HDMI_PHY_CONF0);
  Val |= BAIKAL_HDMI_PHY_CONF0_GEN2_TXPWRON_MASK;
  Val &= ~BAIKAL_HDMI_PHY_CONF0_GEN2_PDDQ_MASK;
  MmioWrite32(BAIKAL_HDMI_PHY_CONF0, Val);
  
  /* Wait for PHY PLL lock */
  for (i = 0; i < 5; ++i) {
    Val = MmioRead32(BAIKAL_HDMI_PHY_STAT0) & BAIKAL_HDMI_PHY_TX_PHY_LOCK;
    if (Val)
      break;

    MicroSecondDelay(1000);
  }

  if (!Val) {
    DEBUG((DEBUG_ERROR, "HDMI PHY PLL failed to lock\n"));
    return EFI_NOT_FOUND;
  }

  DEBUG((DEBUG_INFO, "HDMI PHY PLL locked %u iterations\n", i));
  return EFI_SUCCESS;
}

EFI_STATUS 
HdmiPhyConfigure(
  IN HDMI_PHY_SETTINGS *PhySettings
  )
{
  UINT32 Val;
	
  HdmiPhyPowerOff();

  /* Leave low power consumption mode by asserting SVSRET. */
  Val = MmioRead32(BAIKAL_HDMI_PHY_CONF0);
  Val |= BAIKAL_HDMI_PHY_CONF0_SPARECTRL_MASK;
  MmioWrite32(BAIKAL_HDMI_PHY_CONF0, Val);

  /* PHY reset. The reset signal is active high on Gen2 PHYs. */
  MmioWrite32(BAIKAL_HDMI_MC_PHYRSTZ, BAIKAL_HDMI_MC_PHYRSTZ_DEASSERT);
  MmioWrite32(BAIKAL_HDMI_MC_PHYRSTZ, 0);

  Val = MmioRead32(BAIKAL_HDMI_MC_HEACPHY_RST);
  Val |= BAIKAL_HDMI_MC_HEACPHY_RST_ASSERT;
  MmioWrite32(BAIKAL_HDMI_MC_HEACPHY_RST, Val);

  Val = MmioRead32(BAIKAL_HDMI_PHY_TST0);
  Val |= BAIKAL_HDMI_PHY_TST0_TSTCLR_MASK;
  MmioWrite32(BAIKAL_HDMI_PHY_TST0, Val);

  MmioWrite32(BAIKAL_HDMI_PHY_I2CM_SLAVE_ADDR, BAIKAL_HDMI_PHY_I2CM_SLAVE_ADDR_PHY_GEN2);

  Val = MmioRead32(BAIKAL_HDMI_PHY_TST0);
  Val &= ~BAIKAL_HDMI_PHY_TST0_TSTCLR_MASK;
  MmioWrite32(BAIKAL_HDMI_PHY_TST0, Val);

  HdmiPhyI2CWrite(BAIKAL_HDMI_PHY_OPMODE_PLLCFG, PhySettings->Opmode);
  HdmiPhyI2CWrite(BAIKAL_HDMI_PHY_PLLCURRCTRL, PhySettings->Current);
  HdmiPhyI2CWrite(BAIKAL_HDMI_PHY_PLLGMPCTRL, PhySettings->Gmp);
  HdmiPhyI2CWrite(BAIKAL_HDMI_PHY_TXTERM, PhySettings->TxTer);
  HdmiPhyI2CWrite(BAIKAL_HDMI_PHY_VLEVCTRL, PhySettings->VlevCtrl);
  HdmiPhyI2CWrite(BAIKAL_HDMI_PHY_CKSYMTXCTRL, PhySettings->CkSymTxCtrl);

  return HdmiPhyPowerOn();

}

/* HDMI Initialization Step B.1 */
VOID
HdmiInitAvComposer(
  IN SCAN_TIMINGS *Horizontal,
  IN SCAN_TIMINGS *Vertical
  )
{
  UINT16 HBlank;
  UINT8 VBlank;
  UINT8 Val;
  UINT8 Mask;
  HBlank = Horizontal->FrontPorch + Horizontal->Sync + Horizontal->BackPorch;
  VBlank = Vertical->FrontPorch + Vertical->Sync + Vertical->BackPorch;
  Val = MmioRead32(BAIKAL_HDMI_TX_INVID0);
  Val |= (1 << BAIKAL_HDMI_TX_INVID0_VIDEO_MAPPING_OFFSET);
  MmioWrite32(BAIKAL_HDMI_TX_INVID0, Val);
  Val = MmioRead32(BAIKAL_HDMI_FC_INVIDCONF);
  Mask = BAIKAL_HDMI_FC_INVIDCONF_VSYNC_IN_POLARITY_MASK |
    BAIKAL_HDMI_FC_INVIDCONF_HSYNC_IN_POLARITY_MASK |
    BAIKAL_HDMI_FC_INVIDCONF_DE_IN_POLARITY_MASK |
    BAIKAL_HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC_MASK |
    BAIKAL_HDMI_FC_INVIDCONF_IN_I_P_MASK;
  Val &= ~Mask;
  Val |= BAIKAL_HDMI_FC_INVIDCONF_VSYNC_IN_POLARITY_ACTIVE_LOW |
    BAIKAL_HDMI_FC_INVIDCONF_HSYNC_IN_POLARITY_ACTIVE_LOW |
    BAIKAL_HDMI_FC_INVIDCONF_DE_IN_POLARITY_ACTIVE_HIGH |
    BAIKAL_HDMI_FC_INVIDCONF_R_V_BLANK_IN_OSC_ACTIVE_LOW |
    BAIKAL_HDMI_FC_INVIDCONF_IN_I_P_PROGRESSIVE;

  MmioWrite32(BAIKAL_HDMI_FC_INVIDCONF, Val);
  MmioWrite32(BAIKAL_HDMI_FC_INHACTV1, Horizontal->Resolution >> 8);
  MmioWrite32(BAIKAL_HDMI_FC_INHACTV0, (Horizontal->Resolution >> 0) & 0xFF);
  MmioWrite32(BAIKAL_HDMI_FC_INVACTV1, Vertical->Resolution >> 8);
  MmioWrite32(BAIKAL_HDMI_FC_INVACTV0, (Vertical->Resolution >> 0) & 0xFF);
  MmioWrite32(BAIKAL_HDMI_FC_INHBLANK1, HBlank >> 8);
  MmioWrite32(BAIKAL_HDMI_FC_INHBLANK0, (HBlank >> 0) & 0xFF);
  MmioWrite32(BAIKAL_HDMI_FC_INVBLANK, VBlank);
  MmioWrite32(BAIKAL_HDMI_FC_HSYNCINDELAY1, Horizontal->FrontPorch >> 8);
  MmioWrite32(BAIKAL_HDMI_FC_HSYNCINDELAY0, (Horizontal->FrontPorch >> 0) & 0xFF);
  MmioWrite32(BAIKAL_HDMI_FC_VSYNCINDELAY, Vertical->FrontPorch);
  MmioWrite32(BAIKAL_HDMI_FC_HSYNCINWIDTH1, Horizontal->Sync >> 8);
  MmioWrite32(BAIKAL_HDMI_FC_HSYNCINWIDTH0, (Horizontal->Sync >> 0) & 0xFF);
  MmioWrite32(BAIKAL_HDMI_FC_VSYNCINWIDTH, Vertical->Sync);

  Val = MmioRead32(BAIKAL_HDMI_FC_INVIDCONF);
  Val &= ~BAIKAL_HDMI_FC_INVIDCONF_DVI_MODEZ_MASK;
  Val |= BAIKAL_HDMI_FC_INVIDCONF_DVI_MODEZ_DVI_MODE;
  MmioWrite32(BAIKAL_HDMI_FC_INVIDCONF, Val);
}

/* HDMI Initialization Step B.2 */
EFI_STATUS
HdmiPhyInit(
  IN HDMI_PHY_SETTINGS *PhySettings
  )
{
  UINTN i;
  UINT32 Val;
  EFI_STATUS Ret;

  /* HDMI Phy spec says to do the phy initialization sequence twice */
  for (i = 0; i < 2; i++) {
    
    Val = MmioRead32(BAIKAL_HDMI_PHY_CONF0);
    Val |= BAIKAL_HDMI_PHY_CONF0_SELDATAENPOL_MASK;
    Val &= ~BAIKAL_HDMI_PHY_CONF0_SELDIPIF_MASK;
    MmioWrite32(BAIKAL_HDMI_PHY_CONF0, Val);
 
    if ((Ret = HdmiPhyConfigure(PhySettings)) != EFI_SUCCESS)
      return Ret;
  }

  return EFI_SUCCESS;
}

/* HDMI Initialization Step B.3 */
VOID
HdmiEnableVideoPath(
  UINT32 Sync
  )
{
  UINT32 Val;

  /* control period minimum duration */
  MmioWrite32(BAIKAL_HDMI_FC_CTRLDUR, 12);
  MmioWrite32(BAIKAL_HDMI_FC_EXCTRLDUR, 32);
  MmioWrite32(BAIKAL_HDMI_FC_EXCTRLSPAC, 1);

  /* Set to fill TMDS data channels */
  MmioWrite32(BAIKAL_HDMI_FC_CH0PREAM, 0x0B);
  MmioWrite32(BAIKAL_HDMI_FC_CH1PREAM, 0x16);
  MmioWrite32(BAIKAL_HDMI_FC_CH2PREAM, 0x21);

  /* Enable pixel clock and tmds data path */
  Val = 0x7F;
  Val &= ~BAIKAL_HDMI_MC_CLKDIS_PIXELCLK_DISABLE;
  MmioWrite32(BAIKAL_HDMI_MC_CLKDIS, Val);

  Val &= ~BAIKAL_HDMI_MC_CLKDIS_TMDSCLK_DISABLE;
  MmioWrite32(BAIKAL_HDMI_MC_CLKDIS, Val);

  /* After each CLKDIS reset it is mandatory to
     set up VSYNC active edge delay (in lines) */
  MmioWrite32(BAIKAL_HDMI_FC_VSYNCINWIDTH, Sync);
}

/** Check for presence of a Baikal VDU.

  @retval EFI_SUCCESS          Returns success if platform implements a
                               Baikal VDU controller.

  @retval EFI_NOT_FOUND        Baikal VDU controller not found.
**/
EFI_STATUS
LcdIdentify (
  VOID
  )
{
  RETURN_STATUS PcdStatus;

  PcdStatus = PcdSet32S (PcdVduLcruId, BAIKAL_LCRU_HDMI_ID);
  ASSERT_RETURN_ERROR (PcdStatus);

  PcdStatus = PcdSet32S (PcdVduBase, BAIKAL_VDU_HDMI_BASE);
  ASSERT_RETURN_ERROR (PcdStatus);

  DEBUG ((EFI_D_WARN, "Probing ID registers at 0x%lx for a Baikal VDU\n",
    BAIKAL_VDU_REG_CIR));

  // Check if this is a Baikal VDU
  if (MmioRead32 (BAIKAL_VDU_REG_CIR) == BAIKAL_VDU_PERIPH_ID) {
    return EFI_SUCCESS;
  }
  return EFI_NOT_FOUND;
}

/** Initialize display.

  @param[in]  VramBaseAddress    Address of the framebuffer.

  @retval EFI_SUCCESS            Initialization of display successful.
**/
EFI_STATUS
LcdInitialize (
  IN EFI_PHYSICAL_ADDRESS   VramBaseAddress
  )
{
  // Define start of the VRAM. This never changes for any graphics mode
  MmioWrite32 (BAIKAL_VDU_REG_DBAR, (UINT32)VramBaseAddress);

  // Disable all interrupts from the VDU
  MmioWrite32 (BAIKAL_VDU_REG_IMR, 0);

  return EFI_SUCCESS;
}

EFI_STATUS
LcdSetupHdmi (
  IN UINT32  ModeNumber
  )
{
  EFI_STATUS        Status;
  SCAN_TIMINGS      *Horizontal;
  SCAN_TIMINGS      *Vertical;
  HDMI_PHY_SETTINGS *PhySettings;

  // Set the video mode timings and other relevant information
  Status = LcdPlatformGetTimings (
             ModeNumber,
             &Horizontal,
             &Vertical
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  ASSERT (Horizontal != NULL);
  ASSERT (Vertical != NULL);

  Status = LcdPlatformGetHdmiPhySettings (
             ModeNumber,
             &PhySettings
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  ASSERT (PhySettings != NULL);

  HdmiInitAvComposer(Horizontal, Vertical);
  Status = HdmiPhyInit(PhySettings);
  if (EFI_ERROR (Status)) {
    //ASSERT_EFI_ERROR (Status);
    return Status;
  }
  HdmiEnableVideoPath(Vertical->Sync);
  return EFI_SUCCESS;
}

/** Set requested mode of the display.

  @param[in] ModeNumbe           Display mode number.

  @retval EFI_SUCCESS            Display mode set successfuly.
  @retval !(EFI_SUCCESS)         Other errors.
**/
EFI_STATUS
LcdSetMode (
  IN UINT32  ModeNumber
  )
{
  EFI_STATUS        Status;
  SCAN_TIMINGS      *Horizontal;
  SCAN_TIMINGS      *Vertical;
  UINT32            BufSize;
  UINT32            LcdControl;
  LCD_BPP           LcdBpp;
  //HDMI_PHY_SETTINGS *PhySettings;

  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  ModeInfo;

  LcdSetupHdmi(ModeNumber);

  // Set the video mode timings and other relevant information
  Status = LcdPlatformGetTimings (
             ModeNumber,
             &Horizontal,
             &Vertical
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  ASSERT (Horizontal != NULL);
  ASSERT (Vertical != NULL);

  Status = LcdPlatformGetBpp (ModeNumber, &LcdBpp);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Get the pixel format information
  Status = LcdPlatformQueryMode (ModeNumber, &ModeInfo);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Set Timings
  MmioWrite32 (
    BAIKAL_VDU_REG_HTR,
    HOR_AXIS_PANEL (
      Horizontal->BackPorch,
      Horizontal->FrontPorch,
      Horizontal->Sync,
      Horizontal->Resolution
      )
    );

  if (Horizontal->Resolution > 4080 || Horizontal->Resolution % 16 != 0) {
    MmioWrite32 (
      BAIKAL_VDU_REG_HPPLOR,
      Horizontal->Resolution | BAIKAL_VDU_HPPLOR_HPOE
      );
  }

  MmioWrite32 (
    BAIKAL_VDU_REG_VTR1,
    VER_AXIS_PANEL (
      Vertical->BackPorch,
      Vertical->FrontPorch,
      Vertical->Sync
      )
    );

  MmioWrite32 (
    BAIKAL_VDU_REG_VTR2,
    Vertical->Resolution
    );

  MmioWrite32 (
    BAIKAL_VDU_REG_HVTER,
    TIMINGS_EXT (
      Horizontal->BackPorch,
      Horizontal->FrontPorch,
      Horizontal->Sync,
      Vertical->BackPorch,
      Vertical->FrontPorch,
      Vertical->Sync
      )
    );

  // Set control register
  LcdControl = BAIKAL_VDU_CR1_LCE + BAIKAL_VDU_CR1_DEP + BAIKAL_VDU_CR1_FDW_16_WORDS + BAIKAL_VDU_CR1_OPS_LCD24;
  if (ModeInfo.PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
    LcdControl |= BAIKAL_VDU_CR1_BGR;
  }

  BufSize = Horizontal->Resolution * Vertical->Resolution;

  switch (LcdBpp) {
  case LCD_BITS_PER_PIXEL_1:
    LcdControl |= BAIKAL_VDU_CR1_BPP1;
    BufSize /= 8;
    break;
  case LCD_BITS_PER_PIXEL_2:
    LcdControl |= BAIKAL_VDU_CR1_BPP2;
    BufSize /= 4;
    break;
  case LCD_BITS_PER_PIXEL_4:
    LcdControl |= BAIKAL_VDU_CR1_BPP4;
    BufSize /= 2;
    break;
  case LCD_BITS_PER_PIXEL_8:
    LcdControl |= BAIKAL_VDU_CR1_BPP8;
    break;
  case LCD_BITS_PER_PIXEL_16_555:
    LcdControl |= BAIKAL_VDU_CR1_OPS_555;
  case LCD_BITS_PER_PIXEL_16_565:
    LcdControl |= BAIKAL_VDU_CR1_BPP16;
    BufSize *= 2;
    break;
  case LCD_BITS_PER_PIXEL_24:
    LcdControl |= BAIKAL_VDU_CR1_BPP24;
    BufSize *= 4;
    break;
  default:
    return EFI_NOT_FOUND;
  }

  MmioWrite32 (BAIKAL_VDU_REG_PCTR, BAIKAL_VDU_PCTR_PCR + BAIKAL_VDU_PCTR_PCI);
  MmioWrite32 (BAIKAL_VDU_REG_MRR,
	((MmioRead32 (BAIKAL_VDU_REG_DBAR) + BufSize - 1) & BAIKAL_VDU_MRR_DEAR_MRR_MASK) | BAIKAL_VDU_MRR_OUTSTND_RQ(4)
	);
  MmioWrite32 (BAIKAL_VDU_REG_CR1, LcdControl);

  return EFI_SUCCESS;
}

/** De-initializes the display.
*/
VOID
LcdShutdown (
  VOID
  )
{
  // Disable the controller
  MmioAnd32 (BAIKAL_VDU_REG_CR1, ~BAIKAL_VDU_CR1_LCE);
}
