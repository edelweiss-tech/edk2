/** @file  BaikalVdu.h

 Copyright (C) 2019-2020 Baikal Electronics JSC

 Author: Pavel Parkhomenko <Pavel.Parkhomenko@baikalelectronics.ru>

 Parts of this file were based on sources as follows:

 Copyright (c) 2011, ARM Ltd. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#ifndef _BAIKAL_VDU_H__
#define _BAIKAL_VDU_H__

// Controller Register Offsets
#define BAIKAL_VDU_REG_CR1             ((UINTN)PcdGet32 (PcdVduBase) + 0x000)
#define BAIKAL_VDU_REG_HTR             ((UINTN)PcdGet32 (PcdVduBase) + 0x008)
#define BAIKAL_VDU_REG_VTR1            ((UINTN)PcdGet32 (PcdVduBase) + 0x00C)
#define BAIKAL_VDU_REG_VTR2            ((UINTN)PcdGet32 (PcdVduBase) + 0x010)
#define BAIKAL_VDU_REG_PCTR            ((UINTN)PcdGet32 (PcdVduBase) + 0x014)
#define BAIKAL_VDU_REG_IMR             ((UINTN)PcdGet32 (PcdVduBase) + 0x01C)
#define BAIKAL_VDU_REG_DBAR            ((UINTN)PcdGet32 (PcdVduBase) + 0x028)
#define BAIKAL_VDU_REG_DEAR            ((UINTN)PcdGet32 (PcdVduBase) + 0x030)
#define BAIKAL_VDU_REG_HVTER           ((UINTN)PcdGet32 (PcdVduBase) + 0x044)
#define BAIKAL_VDU_REG_HPPLOR          ((UINTN)PcdGet32 (PcdVduBase) + 0x048)
#define BAIKAL_VDU_REG_CIR             ((UINTN)PcdGet32 (PcdVduBase) + 0x1FC)
#define BAIKAL_VDU_REG_MRR             ((UINTN)PcdGet32 (PcdVduBase) + 0xFFC)

/**********************************************************************/

// Register components (register bits)

// This should make life easier to program specific settings in the different registers
// by simplifying the setting up of the individual bits of each register
// and then assembling the final register value.

/**********************************************************************/

// Register: HTR
#define HOR_AXIS_PANEL(hbp,hfp,hsw,hor_res) (UINT32) ((((UINT32)(hsw) - 1) << 24) | (UINT32)((hbp) << 16) | ((UINT32)((hor_res) / 16) << 8) | ((UINT32)(hfp) << 0))

// Register: VTR1
#define VER_AXIS_PANEL(vbp,vfp,vsw) (UINT32)(((UINT32)(vbp) << 16) | ((UINT32)(vfp) << 8) | ((UINT32)(vsw) << 0))

// Register: HVTER
#define TIMINGS_EXT(hbp,hfp,hsw,vbp,vfp,vsw) (UINT32)(((UINT32)(vsw / 256) << 24) | ((UINT32)(hsw / 256) << 16) | ((UINT32)(vbp / 256) << 12) | ((UINT32)(vfp / 256) << 8) | ((UINT32)(hbp / 256) << 4) | ((UINT32)(hfp / 256) << 4))

#define BAIKAL_VDU_CR1_FDW_4_WORDS        (0 << 16)
#define BAIKAL_VDU_CR1_FDW_8_WORDS        (1 << 16)
#define BAIKAL_VDU_CR1_FDW_16_WORDS       (2 << 16)
#define BAIKAL_VDU_CR1_OPS_LCD24          (1 << 13)
#define BAIKAL_VDU_CR1_OPS_555            (1 << 12)
#define BAIKAL_VDU_CR1_VSP                (1 << 11)
#define BAIKAL_VDU_CR1_HSP                (1 << 10)
#define BAIKAL_VDU_CR1_DEP                (1 << 8)
#define BAIKAL_VDU_CR1_BGR                (1 << 5)
#define BAIKAL_VDU_CR1_BPP1               (0 << 2)
#define BAIKAL_VDU_CR1_BPP2               (1 << 2)
#define BAIKAL_VDU_CR1_BPP4               (2 << 2)
#define BAIKAL_VDU_CR1_BPP8               (3 << 2)
#define BAIKAL_VDU_CR1_BPP16              (4 << 2)
#define BAIKAL_VDU_CR1_BPP18              (5 << 2)
#define BAIKAL_VDU_CR1_BPP24              (6 << 2)
#define BAIKAL_VDU_CR1_LCE                (1 << 0)

#define BAIKAL_VDU_HPPLOR_HPOE            (1 << 31)

#define BAIKAL_VDU_PCTR_PCR               (1 << 10)
#define BAIKAL_VDU_PCTR_PCI               (1 << 9)

#define BAIKAL_VDU_MRR_DEAR_MRR_MASK      (0xfffffff8)
#define BAIKAL_VDU_MRR_OUTSTND_RQ(x)      ((x >> 1) << 0)

#define BAIKAL_VDU_PERIPH_ID              0x0090550F

VOID
BaikalSetVduFrequency(
  UINT32 LcruId,
  UINT32 RefFreq,
  UINT32 OscFreq
  );

#endif /* _BAIKAL_VDU_H__ */
