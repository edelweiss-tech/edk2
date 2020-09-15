// Copyright (c) 2020 Baikal Electronics JSC
// Author: Mikhail Ivanov <michail.ivanov@baikalelectronics.ru>

#include <IndustryStandard/Pci22.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciHostBridgeLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Platform/Pcie.h>
#include <Protocol/FdtClient.h>
#include <Protocol/PciHostBridgeResourceAllocation.h>

#define BEM1000_PCIE_LCRU_GPR_BASE                   0x02050000

#define BEM1000_PCIE_LCRU_GPR_RESET_REG(PcieIdx)     (BEM1000_PCIE_LCRU_GPR_BASE + (PcieIdx) * 0x20 + 0x00)
#define BEM1000_PCIE_LCRU_GPR_RESET_PHY_RESET        BIT0
#define BEM1000_PCIE_LCRU_GPR_RESET_PIPE_RESET       BIT4
#define BEM1000_PCIE_LCRU_GPR_RESET_CORE_RST         BIT8
#define BEM1000_PCIE_LCRU_GPR_RESET_PWR_RST          BIT9
#define BEM1000_PCIE_LCRU_GPR_RESET_STICKY_RST       BIT10
#define BEM1000_PCIE_LCRU_GPR_RESET_NONSTICKY_RST    BIT11
#define BEM1000_PCIE_LCRU_GPR_RESET_HOT_RESET        BIT12
#define BEM1000_PCIE_LCRU_GPR_RESET_ADB_PWRDWN       BIT13

#define BEM1000_PCIE_LCRU_GPR_STATUS_REG(PcieIdx)    (BEM1000_PCIE_LCRU_GPR_BASE + (PcieIdx) * 0x20 + 0x04)
#define BEM1000_PCIE_LCRU_GPR_GENCTL_REG(PcieIdx)    (BEM1000_PCIE_LCRU_GPR_BASE + (PcieIdx) * 0x20 + 0x08)
#define BEM1000_PCIE_LCRU_GPR_GENCTL_LTSSM_ENABLE    BIT1
#define BEM1000_PCIE_LCRU_GPR_GENCTL_DBI2_MODE       BIT2
#define BEM1000_PCIE_LCRU_GPR_GENCTL_PHYREG_ENABLE   BIT3

#define BEM1000_PCIE_LCRU_GPR_POWERCTL_REG(PcieIdx)  (BEM1000_PCIE_LCRU_GPR_BASE + (PcieIdx) * 0x20 + 0x10)
#define BEM1000_PCIE_LCRU_GPR_MSITRANSCTL0_REG       (BEM1000_PCIE_LCRU_GPR_BASE + 0xE8)
#define BEM1000_PCIE_LCRU_GPR_MSITRANSCTL1_REG       (BEM1000_PCIE_LCRU_GPR_BASE + 0xF0)
#define BEM1000_PCIE_LCRU_GPR_MSITRANSCTL2_REG       (BEM1000_PCIE_LCRU_GPR_BASE + 0xF8)

#define BEM1000_PCIE_PF0_TYPE1_HDR_TYPE1_CLASS_CODE_REV_ID_REG                        0x008
#define BEM1000_PCIE_PF0_PCIE_CAP_LINK_CONTROL2_LINK_STATUS2_REG                      0x0a0
#define BEM1000_PCIE_PF0_PORT_LOGIC_PORT_FORCE_OFF                                    0x708
#define BEM1000_PCIE_PF0_PORT_LOGIC_PORT_FORCE_OFF_FORCE_EN                           BIT15

#define BEM1000_PCIE_PF0_PORT_LOGIC_PORT_LINK_CTRL_OFF                                0x710
#define BEM1000_PCIE_PF0_PORT_LOGIC_PORT_LINK_CTRL_OFF_LOOPBACK_ENABLE                BIT2

#define BEM1000_PCIE_PF0_PORT_LOGIC_GEN2_CTRL_OFF                                     0x80C
#define BEM1000_PCIE_PF0_PORT_LOGIC_GEN3_RELATED_OFF                                  0x890
#define BEM1000_PCIE_PF0_PORT_LOGIC_GEN3_RELATED_OFF_GEN3_EQUALIZATION_DISABLE        BIT16

#define BEM1000_PCIE_PF0_PORT_LOGIC_PIPE_LOOPBACK_CONTROL_OFF                         0x8B8
#define BEM1000_PCIE_PF0_PORT_LOGIC_PIPE_LOOPBACK_CONTROL_OFF_PIPE_LOOPBACK           BIT31

#define BEM1000_PCIE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF                                0x8BC
#define BEM1000_PCIE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_DBI_RO_WR_EN                   BIT0

#define BEM1000_PCIE_PF0_PORT_LOGIC_IATU_VIEWPORT_OFF                                 0x900
#define BEM1000_PCIE_PF0_PORT_LOGIC_IATU_VIEWPORT_OFF_REGION_DIR_INBOUND              BIT31
#define BEM1000_PCIE_PF0_PORT_LOGIC_IATU_VIEWPORT_OFF_REGION_DIR_OUTBOUND             0

#define BEM1000_PCIE_PF0_PORT_LOGIC_IATU_REGION_CTRL_1_OFF_OUTBOUND_0                 0x904
#define BEM1000_PCIE_PF0_PORT_LOGIC_IATU_REGION_CTRL_1_OFF_OUTBOUND_0_TYPE_MEM        0
#define BEM1000_PCIE_PF0_PORT_LOGIC_IATU_REGION_CTRL_1_OFF_OUTBOUND_0_TYPE_IO         2
#define BEM1000_PCIE_PF0_PORT_LOGIC_IATU_REGION_CTRL_1_OFF_OUTBOUND_0_TYPE_CFG0       4
#define BEM1000_PCIE_PF0_PORT_LOGIC_IATU_REGION_CTRL_1_OFF_OUTBOUND_0_TYPE_CFG1       5

#define BEM1000_PCIE_PF0_PORT_LOGIC_IATU_REGION_CTRL_2_OFF_OUTBOUND_0                 0x908
#define BEM1000_PCIE_PF0_PORT_LOGIC_IATU_REGION_CTRL_2_OFF_OUTBOUND_0_CFG_SHIFT_MODE  BIT28
#define BEM1000_PCIE_PF0_PORT_LOGIC_IATU_REGION_CTRL_2_OFF_OUTBOUND_0_REGION_EN       BIT31

#define BEM1000_PCIE_PF0_PORT_LOGIC_IATU_LWR_BASE_ADDR_OFF_OUTBOUND_0                 0x90C
#define BEM1000_PCIE_PF0_PORT_LOGIC_IATU_UPPER_BASE_ADDR_OFF_OUTBOUND_0               0x910
#define BEM1000_PCIE_PF0_PORT_LOGIC_IATU_LIMIT_ADDR_OFF_OUTBOUND_0                    0x914
#define BEM1000_PCIE_PF0_PORT_LOGIC_IATU_LWR_TARGET_ADDR_OFF_OUTBOUND_0               0x918
#define BEM1000_PCIE_PF0_PORT_LOGIC_IATU_UPPER_TARGET_ADDR_OFF_OUTBOUND_0             0x91C

#pragma pack(1)
typedef struct {
  ACPI_HID_DEVICE_PATH      AcpiDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  EndDevicePath;
} EFI_PCI_ROOT_BRIDGE_DEVICE_PATH;
#pragma pack ()

STATIC CONST EFI_PCI_ROOT_BRIDGE_DEVICE_PATH  mEfiPciRootBridgeDevicePaths[] = {
  {
    {
      {
        ACPI_DEVICE_PATH,
        ACPI_DP,
        {
          (UINT8)(sizeof (ACPI_HID_DEVICE_PATH)),
          (UINT8)(sizeof (ACPI_HID_DEVICE_PATH) >> 8)
        }
      },
      EISA_PNP_ID (0x0A08), // PCI Express
      0
    },
    {END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, {END_DEVICE_PATH_LENGTH, 0}}
  },
  {
    {
      {
        ACPI_DEVICE_PATH,
        ACPI_DP,
        {
          (UINT8)(sizeof (ACPI_HID_DEVICE_PATH)),
          (UINT8)(sizeof (ACPI_HID_DEVICE_PATH) >> 8)
        }
      },
      EISA_PNP_ID (0x0A08), // PCI Express
      1
    },
    {END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, {END_DEVICE_PATH_LENGTH, 0}}
  },
  {
    {
      {
        ACPI_DEVICE_PATH,
        ACPI_DP,
        {
          (UINT8)(sizeof (ACPI_HID_DEVICE_PATH)),
          (UINT8)(sizeof (ACPI_HID_DEVICE_PATH) >> 8)
        }
      },
      EISA_PNP_ID (0x0A08), // PCI Express
      2
    },
    {END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, {END_DEVICE_PATH_LENGTH, 0}}
  }
};

CONST EFI_PHYSICAL_ADDRESS         mPcieCdmBases[]    = BEM1000_PCIE_CDM_BASES;
CONST EFI_PHYSICAL_ADDRESS         mPcieCfgBases[]    = BEM1000_PCIE_CFG_BASES;
STATIC CONST UINTN                 mPcieBusMins[]     = BEM1000_PCIE_BUS_MINS;
STATIC CONST UINTN                 mPcieBusMaxs[]     = BEM1000_PCIE_BUS_MAXS;
STATIC CONST EFI_PHYSICAL_ADDRESS  mPcieMmio32Bases[] = BEM1000_PCIE_MMIO32_BASES;
STATIC CONST UINTN                 mPcieMmio32Sizes[] = BEM1000_PCIE_MMIO32_SIZES;
STATIC CONST EFI_PHYSICAL_ADDRESS  mPcieMmio64Bases[] = BEM1000_PCIE_MMIO64_BASES;
STATIC CONST UINTN                 mPcieMmio64Sizes[] = BEM1000_PCIE_MMIO64_SIZES;
STATIC CONST EFI_PHYSICAL_ADDRESS  mPciePortIoBases[] = BEM1000_PCIE_PORTIO_BASES;
STATIC CONST UINTN                 mPciePortIoSizes[] = BEM1000_PCIE_PORTIO_SIZES;
STATIC CONST UINTN                 mPciePortIoMins[]  = BEM1000_PCIE_PORTIO_MINS;
STATIC CONST UINTN                 mPciePortIoMaxs[]  = BEM1000_PCIE_PORTIO_MAXS;

STATIC PCI_ROOT_BRIDGE  *mPcieRootBridges;
STATIC UINTN             mPcieRootBridgesNum;

PCI_ROOT_BRIDGE *
EFIAPI
BaikalPciHostBridgeLibGetRootBridges (
  OUT UINTN  *Count
  )
{
  *Count = mPcieRootBridgesNum;
  return mPcieRootBridges;
}

VOID
BaikalPciHostBridgeLibCfgWindow (
  IN  EFI_PHYSICAL_ADDRESS  PcieCdmBase,
  IN  UINTN                 Idx,
  IN  UINT64                CpuBase,
  IN  UINT64                PciBase,
  IN  UINT64                Size,
  IN  UINTN                 Type,
  IN  UINTN                 EnableFlags
  )
{
  ASSERT (Idx  <= 3);
  ASSERT (Type <= 0x1F);
  ASSERT (Size >= SIZE_64KB);
  ASSERT (Size <= SIZE_4GB);

  // The addresses must be aligned to 64 KiB
  ASSERT ((CpuBase & (SIZE_64KB - 1)) == 0);
  ASSERT ((PciBase & (SIZE_64KB - 1)) == 0);
  ASSERT ((Size    & (SIZE_64KB - 1)) == 0);

  ArmDataMemoryBarrier ();
  MmioWrite32 (PcieCdmBase + BEM1000_PCIE_PF0_PORT_LOGIC_IATU_VIEWPORT_OFF,
               BEM1000_PCIE_PF0_PORT_LOGIC_IATU_VIEWPORT_OFF_REGION_DIR_OUTBOUND | Idx);

  ArmDataMemoryBarrier ();
  MmioWrite32 (PcieCdmBase + BEM1000_PCIE_PF0_PORT_LOGIC_IATU_LWR_BASE_ADDR_OFF_OUTBOUND_0,
               (UINT32)(CpuBase & 0xFFFFFFFF));

  MmioWrite32 (PcieCdmBase + BEM1000_PCIE_PF0_PORT_LOGIC_IATU_UPPER_BASE_ADDR_OFF_OUTBOUND_0,
               (UINT32)(CpuBase >> 32));

  MmioWrite32 (PcieCdmBase + BEM1000_PCIE_PF0_PORT_LOGIC_IATU_LIMIT_ADDR_OFF_OUTBOUND_0,
               (UINT32)(CpuBase + Size - 1));

  MmioWrite32 (PcieCdmBase + BEM1000_PCIE_PF0_PORT_LOGIC_IATU_LWR_TARGET_ADDR_OFF_OUTBOUND_0,
               (UINT32)(PciBase & 0xFFFFFFFF));

  MmioWrite32 (PcieCdmBase + BEM1000_PCIE_PF0_PORT_LOGIC_IATU_UPPER_TARGET_ADDR_OFF_OUTBOUND_0,
               (UINT32)(PciBase >> 32));

  MmioWrite32 (PcieCdmBase + BEM1000_PCIE_PF0_PORT_LOGIC_IATU_REGION_CTRL_1_OFF_OUTBOUND_0,
               Type);

  MmioWrite32 (PcieCdmBase + BEM1000_PCIE_PF0_PORT_LOGIC_IATU_REGION_CTRL_2_OFF_OUTBOUND_0,
               BEM1000_PCIE_PF0_PORT_LOGIC_IATU_REGION_CTRL_2_OFF_OUTBOUND_0_REGION_EN |
               EnableFlags);
}

EFI_STATUS
EFIAPI
BaikalPciHostBridgeLibCtor (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  ASSERT (ARRAY_SIZE (mPcieBusMaxs)     == ARRAY_SIZE (mEfiPciRootBridgeDevicePaths) &&
          ARRAY_SIZE (mPcieBusMins)     == ARRAY_SIZE (mEfiPciRootBridgeDevicePaths) &&
          ARRAY_SIZE (mPcieCdmBases)    == ARRAY_SIZE (mEfiPciRootBridgeDevicePaths) &&
          ARRAY_SIZE (mPcieCfgBases)    == ARRAY_SIZE (mEfiPciRootBridgeDevicePaths) &&
          ARRAY_SIZE (mPcieMmio32Bases) == ARRAY_SIZE (mEfiPciRootBridgeDevicePaths) &&
          ARRAY_SIZE (mPcieMmio32Sizes) == ARRAY_SIZE (mEfiPciRootBridgeDevicePaths) &&
          ARRAY_SIZE (mPcieMmio64Bases) == ARRAY_SIZE (mEfiPciRootBridgeDevicePaths) &&
          ARRAY_SIZE (mPcieMmio64Sizes) == ARRAY_SIZE (mEfiPciRootBridgeDevicePaths) &&
          ARRAY_SIZE (mPciePortIoBases) == ARRAY_SIZE (mEfiPciRootBridgeDevicePaths) &&
          ARRAY_SIZE (mPciePortIoSizes) == ARRAY_SIZE (mEfiPciRootBridgeDevicePaths) &&
          ARRAY_SIZE (mPciePortIoMaxs)  == ARRAY_SIZE (mEfiPciRootBridgeDevicePaths) &&
          ARRAY_SIZE (mPciePortIoMins)  == ARRAY_SIZE (mEfiPciRootBridgeDevicePaths));

  FDT_CLIENT_PROTOCOL  *FdtClient;
  INT32                 FdtNode;
  UINTN                 Iter;
  UINTN                 PcieIdxs[ARRAY_SIZE (mEfiPciRootBridgeDevicePaths)];
  EFI_STATUS            Status;
  PCI_ROOT_BRIDGE      *lPcieRootBridge;

  Status = gBS->LocateProtocol (&gFdtClientProtocolGuid, NULL, (VOID **)&FdtClient);
  ASSERT_EFI_ERROR (Status);

  for (FdtNode = 0, Iter = 0; Iter < ARRAY_SIZE (mEfiPciRootBridgeDevicePaths);) {
    CONST VOID  *Prop;
    UINT32       PropSize;

    Status = FdtClient->FindNextCompatibleNode (FdtClient, "baikal,pcie-m", FdtNode, &FdtNode);
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = FdtClient->GetNodeProperty (FdtClient, FdtNode, "status", &Prop, &PropSize);
    if (EFI_ERROR (Status) || AsciiStrCmp ((CONST CHAR8 *)Prop, "okay") != 0) {
      continue;
    }

    Status = FdtClient->GetNodeProperty (FdtClient, FdtNode, "baikal,pcie-lcru", &Prop, &PropSize);
    if (EFI_ERROR (Status) || (PropSize != 8)) {
      continue;
    }

    PcieIdxs[Iter] = (SwapBytes64 (((CONST UINT64 *)Prop)[0]) & 0xFFFFFFFF);
    if (PcieIdxs[Iter] > 2) {
      PcieIdxs[Iter] = 0;
      continue;
    }

    ++mPcieRootBridgesNum;
    ++Iter;
  }

  if (mPcieRootBridgesNum == 0) {
    return EFI_SUCCESS;
  }

  mPcieRootBridges = AllocateZeroPool (mPcieRootBridgesNum * sizeof (PCI_ROOT_BRIDGE));
  lPcieRootBridge  = &mPcieRootBridges[0];

  for (Iter = 0; Iter < mPcieRootBridgesNum; ++Iter, ++lPcieRootBridge) {
    CONST UINTN  PcieIdx = PcieIdxs[Iter];

    lPcieRootBridge->Segment    = PcieIdx;
    lPcieRootBridge->Supports   = 0;
    lPcieRootBridge->Attributes = lPcieRootBridge->Supports;
    lPcieRootBridge->DmaAbove4G = TRUE;
    lPcieRootBridge->NoExtendedConfigSpace = FALSE;
    lPcieRootBridge->ResourceAssigned      = FALSE;
    lPcieRootBridge->AllocationAttributes  = EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM |
                                             EFI_PCI_HOST_BRIDGE_MEM64_DECODE;

    lPcieRootBridge->Bus.Base          = mPcieBusMins[PcieIdx];
    lPcieRootBridge->Bus.Limit         = mPcieBusMaxs[PcieIdx];
    lPcieRootBridge->Io.Base           = mPciePortIoMins[PcieIdx];
    lPcieRootBridge->Io.Limit          = mPciePortIoMaxs[PcieIdx];
    lPcieRootBridge->Mem.Base          = mPcieMmio32Bases[PcieIdx];
    lPcieRootBridge->Mem.Limit         = mPcieMmio32Sizes[PcieIdx] < SIZE_4GB ?
                                           mPcieMmio32Bases[PcieIdx] + mPcieMmio32Sizes[PcieIdx] - 1 :
                                           mPcieMmio32Bases[PcieIdx] + SIZE_4GB - 1;

    lPcieRootBridge->MemAbove4G.Base   = mPcieMmio64Bases[PcieIdx];
    lPcieRootBridge->MemAbove4G.Limit  = mPcieMmio64Sizes[PcieIdx] < SIZE_4GB ?
                                           mPcieMmio64Bases[PcieIdx] + mPcieMmio64Sizes[PcieIdx] - 1 :
                                           mPcieMmio64Bases[PcieIdx] + SIZE_4GB - 1;

    lPcieRootBridge->PMem.Base         = MAX_UINT64;
    lPcieRootBridge->PMem.Limit        = 0;
    lPcieRootBridge->PMemAbove4G.Base  = MAX_UINT64;
    lPcieRootBridge->PMemAbove4G.Limit = 0;
    lPcieRootBridge->DevicePath        = (EFI_DEVICE_PATH_PROTOCOL *)&mEfiPciRootBridgeDevicePaths[PcieIdx];

    MmioOr32 (BEM1000_PCIE_LCRU_GPR_GENCTL_REG (PcieIdx),
              BEM1000_PCIE_LCRU_GPR_GENCTL_PHYREG_ENABLE | BEM1000_PCIE_LCRU_GPR_GENCTL_DBI2_MODE);

    MmioOr32 (BEM1000_PCIE_LCRU_GPR_RESET_REG (PcieIdx),
              BEM1000_PCIE_LCRU_GPR_RESET_PIPE_RESET    | BEM1000_PCIE_LCRU_GPR_RESET_CORE_RST   |
              BEM1000_PCIE_LCRU_GPR_RESET_PWR_RST       | BEM1000_PCIE_LCRU_GPR_RESET_STICKY_RST |
              BEM1000_PCIE_LCRU_GPR_RESET_NONSTICKY_RST | BEM1000_PCIE_LCRU_GPR_RESET_HOT_RESET  |
              BEM1000_PCIE_LCRU_GPR_RESET_ADB_PWRDWN    | BEM1000_PCIE_LCRU_GPR_RESET_PHY_RESET);

    MmioAnd32 (BEM1000_PCIE_LCRU_GPR_GENCTL_REG (PcieIdx), ~(BEM1000_PCIE_LCRU_GPR_GENCTL_LTSSM_ENABLE));

    gBS->Stall (150 * 1000);

    MmioAnd32 (BEM1000_PCIE_LCRU_GPR_RESET_REG (PcieIdx),
               ~(BEM1000_PCIE_LCRU_GPR_RESET_PIPE_RESET    | BEM1000_PCIE_LCRU_GPR_RESET_CORE_RST   |
                 BEM1000_PCIE_LCRU_GPR_RESET_PWR_RST       | BEM1000_PCIE_LCRU_GPR_RESET_STICKY_RST |
                 BEM1000_PCIE_LCRU_GPR_RESET_NONSTICKY_RST | BEM1000_PCIE_LCRU_GPR_RESET_HOT_RESET  |
                 BEM1000_PCIE_LCRU_GPR_RESET_ADB_PWRDWN    | BEM1000_PCIE_LCRU_GPR_RESET_PHY_RESET));

    MmioAnd32 (BEM1000_PCIE_LCRU_GPR_GENCTL_REG (PcieIdx), ~BEM1000_PCIE_LCRU_GPR_GENCTL_DBI2_MODE);
    MmioOr32 (mPcieCdmBases[PcieIdx] +
                BEM1000_PCIE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF,
                BEM1000_PCIE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_DBI_RO_WR_EN);

    MmioAndThenOr32 (mPcieCdmBases[PcieIdx] + BEM1000_PCIE_PF0_TYPE1_HDR_TYPE1_CLASS_CODE_REV_ID_REG,
                     0x000000FF, 0x06040100);

    MmioAnd32 (mPcieCdmBases[PcieIdx] +
                 BEM1000_PCIE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF,
                 BEM1000_PCIE_PF0_PORT_LOGIC_MISC_CONTROL_1_OFF_DBI_RO_WR_EN);

    MmioOr32 (BEM1000_PCIE_LCRU_GPR_GENCTL_REG (PcieIdx), BEM1000_PCIE_LCRU_GPR_GENCTL_DBI2_MODE);

    // Region 0: MMIO32 range
    BaikalPciHostBridgeLibCfgWindow (
      mPcieCdmBases[PcieIdx],
      0,
      lPcieRootBridge->Mem.Base,
      lPcieRootBridge->Mem.Base,
      lPcieRootBridge->Mem.Limit - lPcieRootBridge->Mem.Base + 1,
      BEM1000_PCIE_PF0_PORT_LOGIC_IATU_REGION_CTRL_1_OFF_OUTBOUND_0_TYPE_MEM,
      0
      );

    // Region 2: MMIO64 range
    BaikalPciHostBridgeLibCfgWindow (
      mPcieCdmBases[PcieIdx],
      2,
      lPcieRootBridge->MemAbove4G.Base,
      lPcieRootBridge->MemAbove4G.Base,
      lPcieRootBridge->MemAbove4G.Limit - lPcieRootBridge->MemAbove4G.Base + 1,
      BEM1000_PCIE_PF0_PORT_LOGIC_IATU_REGION_CTRL_1_OFF_OUTBOUND_0_TYPE_MEM,
      0
      );

    // Region 3: port I/O range
    BaikalPciHostBridgeLibCfgWindow (
      mPcieCdmBases[PcieIdx],
      3,
      mPciePortIoBases[PcieIdx],
      lPcieRootBridge->Io.Base,
      lPcieRootBridge->Io.Limit - lPcieRootBridge->Io.Base + 1,
      BEM1000_PCIE_PF0_PORT_LOGIC_IATU_REGION_CTRL_1_OFF_OUTBOUND_0_TYPE_IO,
      0
      );

    MmioOr32 (BEM1000_PCIE_LCRU_GPR_GENCTL_REG (PcieIdx), BEM1000_PCIE_LCRU_GPR_GENCTL_LTSSM_ENABLE);
  }

  MmioWrite32 (BEM1000_PCIE_LCRU_GPR_MSITRANSCTL0_REG, 0);
  MmioWrite32 (BEM1000_PCIE_LCRU_GPR_MSITRANSCTL2_REG, 0x220);

  return EFI_SUCCESS;
}
