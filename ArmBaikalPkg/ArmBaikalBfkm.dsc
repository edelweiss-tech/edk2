#
#  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
#  Copyright (c) 2014, Linaro Limited. All rights reserved.
#  Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#

################################################################################
#
# Defines Section - statements that will be processed to create a Makefile.
#
################################################################################
[Defines]
  PLATFORM_NAME                  = ArmBaikalBfkm
  PLATFORM_GUID                  = 37d7e986-f7e9-45c2-8067-e371421a626c
  PLATFORM_VERSION               = 0.1
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/ArmBaikalBfkm-$(ARCH)
  SUPPORTED_ARCHITECTURES        = AARCH64|ARM
  BUILD_TARGETS                  = DEBUG|RELEASE
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = ArmBaikalPkg/ArmBaikalBfkm.fdf

  # extra configs
  WITH_LINUX                     = FALSE
  WITH_SEMIHOST                  = FALSE
  FD_6MB                         = FALSE
  FD_4MB                         = FALSE
  FD_2MB                         = TRUE
  FD_512K                        = FALSE
  #
  # Defines for default states.  These can be changed on the command line.
  # -D FLAG=VALUE
  #
  DEFINE SECURE_BOOT_ENABLE      = FALSE
  DEFINE HTTP_BOOT_ENABLE        = FALSE

!include ArmBaikalPkg/ArmBaikal.dsc.inc

[LibraryClasses.common]
  ArmSmcLib|ArmPkg/Library/ArmSmcLib/ArmSmcLib.inf
  ArmLib|ArmPkg/Library/ArmLib/ArmBaseLib.inf
  ArmMmuLib|ArmPkg/Library/ArmMmuLib/ArmMmuBaseLib.inf

  # Virtio Support
  VirtioLib|OvmfPkg/Library/VirtioLib/VirtioLib.inf
  VirtioMmioDeviceLib|OvmfPkg/Library/VirtioMmioDeviceLib/VirtioMmioDeviceLib.inf
  # QemuFwCfgLib|ArmBaikalPkg/Library/QemuFwCfgLib/QemuFwCfgLib.inf
  # QemuFwCfgS3Lib|OvmfPkg/Library/QemuFwCfgS3Lib/BaseQemuFwCfgS3LibNull.inf

  #ArmPlatformLib|ArmBaikalPkg/Library/ArmQemuRelocatablePlatformLib/ArmQemuRelocatablePlatformLib.inf
  ArmPlatformLib|ArmBaikalPkg/Library/ArmBaikalFdtPlatformLib/ArmBaikalFdtPlatformLib.inf
  ArmPlatformSysConfigLib|ArmPlatformPkg/Library/ArmPlatformSysConfigLibNull/ArmPlatformSysConfigLibNull.inf

  TimerLib|ArmPkg/Library/ArmArchTimerLib/ArmArchTimerLib.inf
  # NorFlashPlatformLib|ArmBaikalPkg/Library/NorFlashQemuLib/NorFlashQemuLib.inf

  UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
  PlatformBootManagerLib|ArmPkg/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf
  BootLogoLib|MdeModulePkg/Library/BootLogoLib/BootLogoLib.inf
  CustomizedDisplayLib|MdeModulePkg/Library/CustomizedDisplayLib/CustomizedDisplayLib.inf
  CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf

  # PlatformBootManagerLib|ArmBaikalPkg/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf
  # CustomizedDisplayLib|MdeModulePkg/Library/CustomizedDisplayLib/CustomizedDisplayLib.inf
  # QemuBootOrderLib|OvmfPkg/Library/QemuBootOrderLib/QemuBootOrderLib.inf

  FileExplorerLib|MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf
  PciHostBridgeLib|ArmBaikalPkg/Library/BaikalPciHostBridgeLib/PciHostBridgeLib.inf
  PciSegmentLib|ArmBaikalPkg/Library/BaikalPciSegmentLib/PciSegmentLib.inf

  LcdPlatformLib|ArmBaikalPkg/Library/ArmBaikalVduLib/BaikalVduLib.inf
  LcdHwLib|ArmBaikalPkg/Library/ArmBaikalVduLib/BaikalVduHwLib.inf

!if $(HTTP_BOOT_ENABLE) == TRUE
  HttpLib|MdeModulePkg/Library/DxeHttpLib/DxeHttpLib.inf
!endif

[LibraryClasses.common.UEFI_DRIVER]
  UefiScsiLib|MdePkg/Library/UefiScsiLib/UefiScsiLib.inf

[BuildOptions]
  RVCT:*_*_ARM_PLATFORM_FLAGS == --cpu Cortex-A15 -I$(WORKSPACE)/ArmBaikalPkg/Include
  GCC:*_*_ARM_PLATFORM_FLAGS == -mcpu=cortex-a15 -I$(WORKSPACE)/ArmBaikalPkg/Include

!if $(BE_MITX) == TRUE
  *_*_AARCH64_PLATFORM_FLAGS == -I$(WORKSPACE)/ArmBaikalPkg/Include -DBE_MITX
!elseif $(BE_BFKM) == TRUE
  *_*_AARCH64_PLATFORM_FLAGS == -I$(WORKSPACE)/ArmBaikalPkg/Include -DBE_BFKM
!elseif $(BE_QEMU) == TRUE
  *_*_AARCH64_PLATFORM_FLAGS == -I$(WORKSPACE)/ArmBaikalPkg/Include -DBE_QEMU
!else
  *_*_AARCH64_PLATFORM_FLAGS == -I$(WORKSPACE)/ArmBaikalPkg/Include
!endif

[BuildOptions.ARM.EDKII.SEC, BuildOptions.ARM.EDKII.BASE]
  # Avoid MOVT/MOVW instruction pairs in code that may end up in the PIE
  # executable we build for the relocatable PrePi. They are not runtime
  # relocatable in ELF.
  *_CLANG35_*_CC_FLAGS = -mno-movt

################################################################################
#
# Pcd Section - list of all EDK II PCD Entries defined by this Platform
#
################################################################################

[PcdsFeatureFlag.common]
  gUefiOvmfPkgTokenSpaceGuid.PcdQemuBootOrderPciTranslation|TRUE
  gUefiOvmfPkgTokenSpaceGuid.PcdQemuBootOrderMmioTranslation|TRUE

  ## If TRUE, Graphics Output Protocol will be installed on virtual handle created by ConsplitterDxe.
  #  It could be set FALSE to save size.
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutGopSupport|TRUE
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutUgaSupport|FALSE

[PcdsFixedAtBuild.common]
  gArmPlatformTokenSpaceGuid.PcdCoreCount|1
!if $(ARCH) == AARCH64
  gArmTokenSpaceGuid.PcdVFPEnabled|1
!endif

  gArmPlatformTokenSpaceGuid.PcdCPUCoresStackBase|0x4007c000
  gArmPlatformTokenSpaceGuid.PcdCPUCorePrimaryStackSize|0x4000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x2000
  gEfiMdeModulePkgTokenSpaceGuid.PcdMaxAuthVariableSize|0x2800

  # Size of the region used by UEFI in permanent memory (Reserved 64MB)
  gArmPlatformTokenSpaceGuid.PcdSystemMemoryUefiRegionSize|0x03000000

  ## Trustzone enable (to make the transition from EL3 to EL2 in ArmPlatformPkg/Sec)
  gArmTokenSpaceGuid.PcdTrustzoneSupport|FALSE

  gEfiMdeModulePkgTokenSpaceGuid.PcdPciDisableBusEnumeration|FALSE

  #
  # ARM PrimeCell
  #

  ## DW - Serial Terminal
  gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase|0x20230000
  gEfiMdePkgTokenSpaceGuid.PcdUartDefaultBaudRate|115200

  ## Default Terminal Type
  ## 0-PCANSI, 1-VT100, 2-VT00+, 3-UTF8, 4-TTYTERM
  gEfiMdePkgTokenSpaceGuid.PcdDefaultTerminalType|4

  #
  # ARM Virtual Architectural Timer -- fetch frequency from QEMU (TCG) or KVM
  #
  gArmTokenSpaceGuid.PcdArmArchTimerFreqInHz|0

!if $(HTTP_BOOT_ENABLE) == TRUE
  gEfiNetworkPkgTokenSpaceGuid.PcdAllowHttpConnections|TRUE
!endif

[PcdsPatchableInModule.common]
  #
  # This will be overridden in the code
  #
  gArmTokenSpaceGuid.PcdSystemMemoryBase|0x0
  gArmTokenSpaceGuid.PcdSystemMemorySize|0x0

  #
  # Define a default initial address for the device tree.
  # Ignored if x0 != 0 at entry.
  #
  gArmBaikalTokenSpaceGuid.PcdDeviceTreeInitialBaseAddress|0x80000000

  gArmTokenSpaceGuid.PcdFdBaseAddress|0x0
  gArmTokenSpaceGuid.PcdFvBaseAddress|0x0

[PcdsFixedAtBuild.AARCH64]

  gEfiMdeModulePkgTokenSpaceGuid.PcdResetOnMemoryTypeInformationChange|FALSE
  gEfiMdeModulePkgTokenSpaceGuid.PcdBootManagerMenuFile|{ 0x21, 0xaa, 0x2c, 0x46, 0x14, 0x76, 0x03, 0x45, 0x83, 0x6e, 0x8a, 0xb6, 0xf4, 0x66, 0x23, 0x31 }

  #
  # The maximum physical I/O addressability of the processor, set with
  # BuildCpuHob().
  #
  gEmbeddedTokenSpaceGuid.PcdPrePiCpuIoSize|0x00000016

  # KVM limits it IPA space to 40 bits (1 TB), so there is no need to
  # support anything bigger, even if the host hardware does
  gEmbeddedTokenSpaceGuid.PcdPrePiCpuMemorySize|40

[PcdsDynamicDefault.common]
  gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut|3

  gArmTokenSpaceGuid.PcdArmArchTimerSecIntrNum|0x0
  gArmTokenSpaceGuid.PcdArmArchTimerIntrNum|0x0
  gArmTokenSpaceGuid.PcdArmArchTimerVirtIntrNum|0x0
  gArmTokenSpaceGuid.PcdArmArchTimerHypIntrNum|0x0

  #
  # ARM General Interrupt Controller
  #
  gArmTokenSpaceGuid.PcdGicDistributorBase|0x0
  gArmTokenSpaceGuid.PcdGicRedistributorsBase|0x0
  gArmTokenSpaceGuid.PcdGicInterruptInterfaceBase|0x0

  ## PL031 RealTimeClock
  gArmPlatformTokenSpaceGuid.PcdPL031RtcBase|0x0

  #
  # Set video resolution for boot options and for text setup.
  # PlatformDxe can set the former at runtime.
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoHorizontalResolution|1920
  gEfiMdeModulePkgTokenSpaceGuid.PcdVideoVerticalResolution|1080
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoHorizontalResolution|1920
  gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoVerticalResolution|1080

  ## This PCD defines the Console output column and the default value is 25 according to UEFI spec.
  #  This PCD could be set to 0 then console output could be at max column and max row.
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutRow|25
  ## This PCD defines the Console output row and the default value is 80 according to UEFI spec.
  #  This PCD could be set to 0 then console output could be at max column and max row.
  gEfiMdeModulePkgTokenSpaceGuid.PcdConOutColumn|80

  #
  # SMBIOS entry point version
  #
  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosVersion|0x0300
  gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosDocRev|0x0
  gUefiOvmfPkgTokenSpaceGuid.PcdQemuSmbiosValidated|FALSE

################################################################################
#
# Components Section - list of all EDK II Modules needed by this Platform
#
################################################################################
[Components.common]
  #
  # PEI Phase modules
  #
  ArmBaikalPkg/PrePi/ArmBaikalPrePiUniCoreRelocatable.inf {
    <LibraryClasses>
      ExtractGuidedSectionLib|EmbeddedPkg/Library/PrePiExtractGuidedSectionLib/PrePiExtractGuidedSectionLib.inf
      LzmaDecompressLib|MdeModulePkg/Library/LzmaCustomDecompressLib/LzmaCustomDecompressLib.inf
      PrePiLib|EmbeddedPkg/Library/PrePiLib/PrePiLib.inf
      HobLib|EmbeddedPkg/Library/PrePiHobLib/PrePiHobLib.inf
      PrePiHobListPointerLib|ArmPlatformPkg/Library/PrePiHobListPointerLib/PrePiHobListPointerLib.inf
      MemoryAllocationLib|EmbeddedPkg/Library/PrePiMemoryAllocationLib/PrePiMemoryAllocationLib.inf
  }

  #
  # DXE
  #
  MdeModulePkg/Core/Dxe/DxeMain.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
  }
  MdeModulePkg/Universal/PCD/Dxe/Pcd.inf {
    <LibraryClasses>
      PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  }

  #
  # Architectural Protocols
  #
  ArmPkg/Drivers/CpuDxe/CpuDxe.inf
  MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
  }
!if $(SECURE_BOOT_ENABLE) == TRUE
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf {
    <LibraryClasses>
      NULL|SecurityPkg/Library/DxeImageVerificationLib/DxeImageVerificationLib.inf
  }
  SecurityPkg/VariableAuthenticated/SecureBootConfigDxe/SecureBootConfigDxe.inf
!else
  MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
!endif
  MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  MdeModulePkg/Universal/FaultTolerantWriteDxe/FaultTolerantWriteDxe.inf
  MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
  MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
  EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf
#{
#    <LibraryClasses>
#      NULL|ArmBaikalPkg/Library/ArmBaikalPL031FdtClientLib/ArmBaikalPL031FdtClientLib.inf
#  }
  EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf

  MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
  MdeModulePkg/Universal/Console/TerminalDxe/TerminalDxe.inf
  MdeModulePkg/Universal/SerialDxe/SerialDxe.inf

  ArmBaikalPkg/Drivers/LcdGraphicsOutputDxe/LcdGraphicsOutputDxe.inf {
!if $(TARGET) == DEBUG
    <PcdsFixedAtBuild>
      gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x8FFFFFBF
!endif
  }

  MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf

  ArmPkg/Drivers/ArmGic/ArmGicDxe.inf
  ArmPkg/Drivers/TimerDxe/TimerDxe.inf {
    <LibraryClasses>
      NULL|ArmBaikalPkg/Library/ArmBaikalTimerFdtClientLib/ArmBaikalTimerFdtClientLib.inf
  }
  ArmBaikalPkg/Drivers/BaikalSpiFvDxe/BaikalSpiFvDxe.inf
  ArmBaikalPkg/Drivers/BaikalSpiBlockDxe/BaikalSpiBlockDxe.inf

  # ArmBaikalPkg/Drivers/BaikalRamDiskDxe/BaikalRamDiskDxe.inf
  MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
!if $(BE_MITX) == TRUE
  ArmBaikalPkg/Drivers/BaikalI2cDxe/BaikalI2cDxe.inf
  ArmBaikalPkg/Drivers/I2cDevicesDxe/I2cDevicesDxe.inf
!else
  ArmBaikalPkg/Drivers/BaikalRtcDxe/BaikalRtcDxe.inf
!endif
  ArmBaikalPkg/Drivers/BaikalEthDxe/BaikalEthDxe.inf

  #
  # Platform Driver
  #
  # ArmBaikalPkg/VirtioFdtDxe/VirtioFdtDxe.inf
  ArmBaikalPkg/Drivers/FdtClientDxe/FdtClientDxe.inf
  ArmBaikalPkg/Drivers/HighMemDxe/HighMemDxe.inf
  #OvmfPkg/VirtioBlkDxe/VirtioBlk.inf
  #OvmfPkg/VirtioScsiDxe/VirtioScsi.inf
  #OvmfPkg/VirtioNetDxe/VirtioNet.inf
  #OvmfPkg/VirtioRngDxe/VirtioRng.inf

!if $(BE_MITX) == TRUE
  #
  # I2C Support
  #
  MdeModulePkg/Bus/I2c/I2cDxe/I2cDxe.inf
!endif

  #
  # FAT filesystem + GPT/MBR partitioning + UDF filesystem
  #
  MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf
  FatPkg/EnhancedFatDxe/Fat.inf
  MdeModulePkg/Universal/Disk/UdfDxe/UdfDxe.inf

  #
  # Bds
  #
  MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  MdeModulePkg/Universal/DisplayEngineDxe/DisplayEngineDxe.inf
  MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
  MdeModulePkg/Universal/DriverHealthManagerDxe/DriverHealthManagerDxe.inf
  MdeModulePkg/Universal/BdsDxe/BdsDxe.inf
  MdeModulePkg/Logo/LogoDxe.inf
  MdeModulePkg/Application/UiApp/UiApp.inf {
    <LibraryClasses>
      NULL|MdeModulePkg/Library/DeviceManagerUiLib/DeviceManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootManagerUiLib/BootManagerUiLib.inf
      NULL|MdeModulePkg/Library/BootMaintenanceManagerUiLib/BootMaintenanceManagerUiLib.inf
  }

  #
  # Networking stack
  #
  MdeModulePkg/Universal/Network/DpcDxe/DpcDxe.inf
  MdeModulePkg/Universal/Network/ArpDxe/ArpDxe.inf
  MdeModulePkg/Universal/Network/Dhcp4Dxe/Dhcp4Dxe.inf
  MdeModulePkg/Universal/Network/Ip4Dxe/Ip4Dxe.inf
  MdeModulePkg/Universal/Network/MnpDxe/MnpDxe.inf
  # MdeModulePkg/Universal/Network/VlanConfigDxe/VlanConfigDxe.inf
  MdeModulePkg/Universal/Network/Mtftp4Dxe/Mtftp4Dxe.inf
  # MdeModulePkg/Universal/Network/Tcp4Dxe/Tcp4Dxe.inf
  MdeModulePkg/Universal/Network/Udp4Dxe/Udp4Dxe.inf
  # MdeModulePkg/Universal/Network/UefiPxeBcDxe/UefiPxeBcDxe.inf
  # MdeModulePkg/Universal/Network/IScsiDxe/IScsiDxe.inf
!if $(HTTP_BOOT_ENABLE) == TRUE
  NetworkPkg/DnsDxe/DnsDxe.inf
  NetworkPkg/HttpUtilitiesDxe/HttpUtilitiesDxe.inf
  NetworkPkg/HttpDxe/HttpDxe.inf
  NetworkPkg/HttpBootDxe/HttpBootDxe.inf
!endif

  #
  # SCSI Bus and Disk Driver
  #

  #
  #  AHCI Support
  #
  ArmBaikalPkg/Drivers/PciEmulation/PciEmulation.inf
  MdeModulePkg/Bus/Ata/AtaAtapiPassThru/AtaAtapiPassThru.inf
  MdeModulePkg/Bus/Ata/AtaBusDxe/AtaBusDxe.inf
  MdeModulePkg/Bus/Pci/NonDiscoverablePciDeviceDxe/NonDiscoverablePciDeviceDxe.inf
  MdeModulePkg/Bus/Pci/NvmExpressDxe/NvmExpressDxe.inf
  MdeModulePkg/Bus/Pci/SataControllerDxe/SataControllerDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiBusDxe/ScsiBusDxe.inf
  MdeModulePkg/Bus/Scsi/ScsiDiskDxe/ScsiDiskDxe.inf

  #
  # SMBIOS Support
  #
  # MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf {
  #  <LibraryClasses>
  #    NULL|OvmfPkg/Library/SmbiosVersionLib/DetectSmbiosVersionLib.inf
  #}
  #OvmfPkg/SmbiosPlatformDxe/SmbiosPlatformDxe.inf

  #
  # PCI support
  #
  ArmBaikalPkg/Drivers/BaikalPciCpuIo2Dxe/BaikalPciCpuIo2Dxe.inf
  MdeModulePkg/Bus/Pci/PciBusDxe/PciBusDxe.inf
  MdeModulePkg/Bus/Pci/PciHostBridgeDxe/PciHostBridgeDxe.inf

  #
  # Video support
  #
  OvmfPkg/VirtioGpuDxe/VirtioGpu.inf
  OvmfPkg/PlatformDxe/Platform.inf

  #
  # USB Support
  #
  MdeModulePkg/Bus/Pci/XhciDxe/XhciDxe.inf
  MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
  MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf
  MdeModulePkg/Bus/Usb/UsbMouseDxe/UsbMouseDxe.inf

  #
  # ACPI Support
  #
  # ArmBaikalPkg/PlatformHasAcpiDtDxe/PlatformHasAcpiDtDxe.inf

  #
  # ?
  #
  ArmPkg/Library/ArmSmcLib/ArmSmcLib.inf


[Components.AARCH64]
  # MdeModulePkg/Universal/Acpi/BootGraphicsResourceTableDxe/BootGraphicsResourceTableDxe.inf
  # OvmfPkg/AcpiPlatformDxe/QemuFwCfgAcpiPlatformDxe.inf {
  #  <LibraryClasses>
  #    NULL|ArmBaikalPkg/Library/FdtPciPcdProducerLib/FdtPciPcdProducerLib.inf
  # }

  #
  # Semihost FS
  #

!if $(WITH_SEMIHOST) == TRUE
  ArmPkg/Filesystem/SemihostFs/SemihostFs.inf {
            <LibraryClasses>
      NULL|ArmPkg/Library/SemihostLib/SemihostLib.inf
  }
!endif

  #
  # FV FS
  #
  MdeModulePkg/Universal/FvSimpleFileSystemDxe/FvSimpleFileSystemDxe.inf

  #
  # Linux
  #
!if $(WITH_LINUX) == TRUE
  ArmBaikalPkg/Tests/KernelBin/Linux.inf
  ArmBaikalPkg/Tests/KernelBin/Initrd.inf
!endif

  ArmBaikalPkg/Tests/TestAhci/TestAhci.inf
