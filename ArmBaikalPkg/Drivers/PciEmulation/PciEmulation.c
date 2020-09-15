#include <PiDxe.h>

#include <Library/DebugLib.h>
#include <Library/NonDiscoverableDeviceRegistrationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/EmbeddedExternalDevice.h>

STATIC
EFI_STATUS
PciEmulationInitAhci (
  )
{
  EFI_STATUS Status;
  EFI_PHYSICAL_ADDRESS    AhciBaseAddr;

  AhciBaseAddr = 0x2C600000;
  Status = RegisterNonDiscoverableMmioDevice (
                     NonDiscoverableDeviceTypeAhci,
                     NonDiscoverableDeviceDmaTypeCoherent,
                     NULL,
                     NULL,
                     1,
                     AhciBaseAddr, SIZE_4KB
                   );

  if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "PciEmulation: Cannot install Ahci device at sata0\n"));
      return Status;
  }

  AhciBaseAddr = 0x2C610000;
  Status = RegisterNonDiscoverableMmioDevice (
                     NonDiscoverableDeviceTypeAhci,
                     NonDiscoverableDeviceDmaTypeCoherent,
                     NULL,
                     NULL,
                     1,
                     AhciBaseAddr, SIZE_4KB
                   );


  if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_ERROR, "PciEmulation: Cannot install Ahci device at sata1\n"));
      return Status;
  }


  return EFI_SUCCESS;
}

#ifndef BE_QEMU
STATIC
EFI_STATUS
EFIAPI
NonDiscoverableDeviceXhciInitializer (
  IN  NON_DISCOVERABLE_DEVICE  *This
  )
{
  UINT32 * CONST  GsBusCfg0 = (UINT32 *)(This->Resources->AddrRangeMin + 0xc100);
  // Set AHB-prot/AXI-cache/OCP-ReqInfo for data/descriptor read/write
  *GsBusCfg0 = (*GsBusCfg0 & 0x0000ffff) | 0xbb770000;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
PciEmulationInitXhci (VOID)
{
  UINTN  Idx;
  CONST EFI_PHYSICAL_ADDRESS  XhciBaseAddrs[] = {0x2c400000, 0x2c500000};

  for (Idx = 0; Idx < sizeof (XhciBaseAddrs) / sizeof (XhciBaseAddrs[0]); ++Idx) {
    EFI_STATUS  Status;
    Status = RegisterNonDiscoverableMmioDevice (
               NonDiscoverableDeviceTypeXhci,
               NonDiscoverableDeviceDmaTypeCoherent,
               NonDiscoverableDeviceXhciInitializer,
               NULL,
               1,
               XhciBaseAddrs[Idx], SIZE_1MB
               );

    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "PciEmulation: unable to register NonDiscoverableMmioDeviceTypeXhci @ 0x%lx, Status: %r\n",
        XhciBaseAddrs[Idx], Status));

      return Status;
    }
  }

  return EFI_SUCCESS;
}
#endif

//
// Entry point
//
EFI_STATUS
EFIAPI
PciEmulationEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;
  Status = PciEmulationInitAhci();
  if (EFI_ERROR(Status)) {
    return Status;
  }

#ifndef BE_QEMU
  Status = PciEmulationInitXhci();
  if (EFI_ERROR (Status)) {
    return Status;
  }
#endif

  return EFI_SUCCESS;
}
