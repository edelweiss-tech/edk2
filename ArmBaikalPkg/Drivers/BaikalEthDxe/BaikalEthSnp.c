// Copyright (c) 2019-2020 Baikal Electronics JSC
// Author: Mikhail Ivanov <michail.ivanov@baikalelectronics.ru>

#include <Library/DebugLib.h>
#include <Library/NetLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/SimpleNetwork.h>
#include "BaikalEthGmacDescs.h"
#include "BaikalEthGmacRegs.h"
#include "BaikalEthSnp.h"

#define RX_BUF_SIZE  2048
#define RX_DESC_NUM  64
#define TX_DESC_NUM  64

typedef struct {
  EFI_HANDLE                      Handle;
  EFI_SIMPLE_NETWORK_PROTOCOL     Snp;
  EFI_SIMPLE_NETWORK_MODE         SnpMode;
  volatile BAIKAL_ETH_GMAC_REGS  *GmacRegs;
  volatile UINT8                  RxBufs[RX_DESC_NUM][RX_BUF_SIZE];
  volatile VOID                  *RxDescBaseAddr;
  volatile UINT8                  RxDescArea[(RX_DESC_NUM + 1) * sizeof (BAIKAL_ETH_GMAC_RDESC)];
  volatile VOID                  *TxDescBaseAddr;
  volatile UINT8                  TxDescArea[(TX_DESC_NUM + 1) * sizeof (BAIKAL_ETH_GMAC_TDESC)];
  UINTN                           RxDescReadIdx;
  UINTN                           TxDescWriteIdx;
  UINTN                           TxDescReleaseIdx;
} BAIKAL_ETH_INSTANCE;

STATIC
UINT32
EFIAPI
BaikalEthSnpGetReceiveFilterSetting (
  IN     EFI_SIMPLE_NETWORK_PROTOCOL  *Snp
  );

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpGetStatus (
  IN     EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  OUT    UINT32                       *InterruptStatus,  OPTIONAL
  OUT    VOID                        **TxBuf            OPTIONAL
  );

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpInitialize (
  IN     EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN     UINTN                         ExtraRxBufSize,  OPTIONAL
  IN     UINTN                         ExtraTxBufSize   OPTIONAL
  );

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpMCastIpToMac (
  IN     EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN     BOOLEAN                       Ipv6,
  IN     EFI_IP_ADDRESS               *Ip,
  OUT    EFI_MAC_ADDRESS              *McastMacAddr
  );

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpNvData (
  IN     EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN     BOOLEAN                       ReadWrite,
  IN     UINTN                         Offset,
  IN     UINTN                         BufSize,
  IN OUT VOID                         *Buf
  );

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpReset (
  IN     EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN     BOOLEAN                       ExtendedVerification
  );

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpReceiveFilters (
  IN     EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN     UINT32                        Enable,
  IN     UINT32                        Disable,
  IN     BOOLEAN                       ResetMCastFilter,
  IN     UINTN                         MCastFilterCnt,   OPTIONAL
  IN     EFI_MAC_ADDRESS              *MCastFilter       OPTIONAL
  );

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpShutdown (
  IN     EFI_SIMPLE_NETWORK_PROTOCOL  *Snp
  );

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpStart (
  IN     EFI_SIMPLE_NETWORK_PROTOCOL  *Snp
  );

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpStationAddress (
  IN     EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN     BOOLEAN                       Reset,
  IN     EFI_MAC_ADDRESS              *NewMacAddr  OPTIONAL
  );

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpStatistics (
  IN     EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN     BOOLEAN                       Reset,
  IN OUT UINTN                        *StatisticsSize,  OPTIONAL
  OUT    EFI_NETWORK_STATISTICS       *StatisticsTable  OPTIONAL
  );

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpStop (
  IN     EFI_SIMPLE_NETWORK_PROTOCOL  *Snp
  );

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpReceive (
  IN     EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  OUT    UINTN                        *HdrSize,  OPTIONAL
  IN OUT UINTN                        *BufSize,
  OUT    VOID                         *Buf,
  OUT    EFI_MAC_ADDRESS              *SrcAddr,  OPTIONAL
  OUT    EFI_MAC_ADDRESS              *DstAddr,  OPTIONAL
  OUT    UINT16                       *Protocol  OPTIONAL
  );

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpTransmit (
  IN     EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN     UINTN                         HdrSize,
  IN     UINTN                         BufSize,
  IN     VOID                         *Buf,
  IN     EFI_MAC_ADDRESS              *SrcAddr,  OPTIONAL
  IN     EFI_MAC_ADDRESS              *DstAddr,  OPTIONAL
  IN     UINT16                       *Protocol  OPTIONAL
  );

STATIC
UINT32
BaikalEthSnpGetReceiveFilterSetting (
  IN  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp
  )
{
  CONST BAIKAL_ETH_INSTANCE *CONST  EthInst = BASE_CR (Snp, BAIKAL_ETH_INSTANCE, Snp);
  UINT32  ReceiveFilterSetting = EFI_SIMPLE_NETWORK_RECEIVE_UNICAST;

  if (!(EthInst->GmacRegs->MacFrameFilter & MAC_FRAMEFILTER_DBF)) {
    ReceiveFilterSetting |= EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST;
  }

  if (EthInst->GmacRegs->MacFrameFilter & MAC_FRAMEFILTER_PR) {
    ReceiveFilterSetting |= EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS;
  }

  if (EthInst->GmacRegs->MacFrameFilter & MAC_FRAMEFILTER_PM) {
    ReceiveFilterSetting |= EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST;
  }

  DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpGetReceiveFilterSetting: FilterSet = %c%c%c%c%c\n",
    EthInst->GmacRegs,
    ReceiveFilterSetting & EFI_SIMPLE_NETWORK_RECEIVE_UNICAST               ? 'U' : 'u',
    ReceiveFilterSetting & EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST             ? 'B' : 'b',
    ReceiveFilterSetting & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST             ? 'M' : 'm',
    ReceiveFilterSetting & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS           ? 'P' : 'p',
    ReceiveFilterSetting & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST ? 'W' : 'w'
    ));

  return ReceiveFilterSetting;
}

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpGetStatus (
  IN   EFI_SIMPLE_NETWORK_PROTOCOL   *Snp,
  OUT  UINT32                        *InterruptStatus,  OPTIONAL
  OUT  VOID                         **TxBuf             OPTIONAL
  )
{
  BAIKAL_ETH_INSTANCE *CONST  EthInst = BASE_CR (Snp, BAIKAL_ETH_INSTANCE, Snp);
  BOOLEAN  MediaPresent;
  EFI_TPL  SavedTpl;

  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SavedTpl = gBS->RaiseTPL (TPL_CALLBACK);

  switch (Snp->Mode->State) {
  case EfiSimpleNetworkInitialized:
    break;
  case EfiSimpleNetworkStarted:
    DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpGetStatus: SNP not initialized\n", EthInst->GmacRegs));
    gBS->RestoreTPL (SavedTpl);
    return EFI_DEVICE_ERROR;
  case EfiSimpleNetworkStopped:
    DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpGetStatus: SNP not started\n", EthInst->GmacRegs));
    gBS->RestoreTPL (SavedTpl);
    return EFI_NOT_STARTED;
  default:
    DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpGetStatus: SNP invalid state = %u\n", EthInst->GmacRegs, Snp->Mode->State));
    gBS->RestoreTPL (SavedTpl);
    return EFI_DEVICE_ERROR;
  }

  MediaPresent = EthInst->GmacRegs->MacMiiStatus & MAC_MIISTATUS_LNKSTS;

  if (Snp->Mode->MediaPresent != MediaPresent) {
    Snp->Mode->MediaPresent    = MediaPresent;
    DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpGetStatus: link %s\n", EthInst->GmacRegs, MediaPresent ? L"up" : L"down"));
  }

  if (TxBuf != NULL && EthInst->TxDescReleaseIdx != EthInst->TxDescWriteIdx) {
    CONST BAIKAL_ETH_GMAC_TDESC *TxDescs = (BAIKAL_ETH_GMAC_TDESC *) EthInst->TxDescBaseAddr;
    *((UINT32 *) TxBuf) = TxDescs[EthInst->TxDescReleaseIdx].Tdes2;
    EthInst->TxDescReleaseIdx = (EthInst->TxDescReleaseIdx + 1) % TX_DESC_NUM;
  }

  if (InterruptStatus) {
    *InterruptStatus =
      (EthInst->GmacRegs->DmaStatus & DMA_STATUS_RI ? EFI_SIMPLE_NETWORK_RECEIVE_INTERRUPT  : 0) |
      (EthInst->GmacRegs->DmaStatus & DMA_STATUS_TI ? EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT : 0);

    EthInst->GmacRegs->DmaStatus =
      (*InterruptStatus & EFI_SIMPLE_NETWORK_RECEIVE_INTERRUPT  ? DMA_STATUS_RI : 0) |
      (*InterruptStatus & EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT ? DMA_STATUS_TI : 0);
  }

  gBS->RestoreTPL (SavedTpl);
  return EFI_SUCCESS;
}

EFI_STATUS
BaikalEthSnpInstanceCtor (
  IN   VOID              *GmacRegs,
  IN   EFI_MAC_ADDRESS   *MacAddr,
  OUT  VOID             **Snp,
  OUT  EFI_HANDLE       **Handle
  )
{
  BAIKAL_ETH_INSTANCE   *EthInst;
  EFI_PHYSICAL_ADDRESS   PhysicalAddr;
  EFI_STATUS             Status;

  PhysicalAddr = (EFI_PHYSICAL_ADDRESS) (SIZE_4GB - 1);
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiBootServicesData,
                  EFI_SIZE_TO_PAGES (sizeof (BAIKAL_ETH_INSTANCE)),
                  &PhysicalAddr
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "BaikalEth(%p)SnpInstaceCtor: unable to allocate EthInst, Status = %r\n", GmacRegs, Status));
    return Status;
  }

  EthInst = (VOID *) PhysicalAddr;
  EthInst->Handle   = NULL;
  EthInst->GmacRegs = GmacRegs;

  EthInst->RxDescReadIdx    = 0;
  EthInst->TxDescWriteIdx   = 0;
  EthInst->TxDescReleaseIdx = 0;

  EthInst->Snp.Revision       = EFI_SIMPLE_NETWORK_PROTOCOL_REVISION;
  EthInst->Snp.Start          = BaikalEthSnpStart;
  EthInst->Snp.Stop           = BaikalEthSnpStop;
  EthInst->Snp.Initialize     = BaikalEthSnpInitialize;
  EthInst->Snp.Reset          = BaikalEthSnpReset;
  EthInst->Snp.Shutdown       = BaikalEthSnpShutdown;
  EthInst->Snp.ReceiveFilters = BaikalEthSnpReceiveFilters;
  EthInst->Snp.StationAddress = BaikalEthSnpStationAddress;
  EthInst->Snp.Statistics     = BaikalEthSnpStatistics;
  EthInst->Snp.MCastIpToMac   = BaikalEthSnpMCastIpToMac;
  EthInst->Snp.NvData         = BaikalEthSnpNvData;
  EthInst->Snp.GetStatus      = BaikalEthSnpGetStatus;
  EthInst->Snp.Transmit       = BaikalEthSnpTransmit;
  EthInst->Snp.Receive        = BaikalEthSnpReceive;
  EthInst->Snp.WaitForPacket  = NULL;
  EthInst->Snp.Mode           = &EthInst->SnpMode;

  EthInst->SnpMode.State                 = EfiSimpleNetworkStopped;
  EthInst->SnpMode.HwAddressSize         = NET_ETHER_ADDR_LEN;
  EthInst->SnpMode.MediaHeaderSize       = sizeof (ETHER_HEAD);
  EthInst->SnpMode.MaxPacketSize         = 1500;
  EthInst->SnpMode.NvRamSize             = 0;
  EthInst->SnpMode.NvRamAccessSize       = 0;
  EthInst->SnpMode.ReceiveFilterMask     = EFI_SIMPLE_NETWORK_RECEIVE_UNICAST     |
                                           EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST   |
                                           EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS |
                                           EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST;

  EthInst->SnpMode.ReceiveFilterSetting  = BaikalEthSnpGetReceiveFilterSetting (&EthInst->Snp);
  EthInst->SnpMode.MaxMCastFilterCount   = MAX_MCAST_FILTER_CNT;
  EthInst->SnpMode.MCastFilterCount      = 0;
  EthInst->SnpMode.IfType                = NET_IFTYPE_ETHERNET;
  EthInst->SnpMode.MacAddressChangeable  = TRUE;
  EthInst->SnpMode.MultipleTxSupported   = FALSE;
  EthInst->SnpMode.MediaPresentSupported = TRUE;
  EthInst->SnpMode.MediaPresent          = FALSE;

  gBS->CopyMem (&EthInst->SnpMode.CurrentAddress,   MacAddr, sizeof (EFI_MAC_ADDRESS));
  gBS->CopyMem (&EthInst->SnpMode.PermanentAddress, MacAddr, sizeof (EFI_MAC_ADDRESS));
  gBS->SetMem (&EthInst->SnpMode.MCastFilter, MAX_MCAST_FILTER_CNT * sizeof (EFI_MAC_ADDRESS), 0);
  gBS->SetMem (&EthInst->SnpMode.BroadcastAddress, sizeof (EFI_MAC_ADDRESS), 0xff);

  *Handle = &EthInst->Handle;
  *Snp    = &EthInst->Snp;

  /* TODO: create event to reset (DmaBusMode.SWR = 1) the EthInst when exit boot service
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  BaikalEthSnpExitBootService,
                  EthInst,
                  &gEfiEventExitBootServicesGuid,
                  &EthInst->ExitBootServiceEvent
                  );
  */

  return EFI_SUCCESS;
}

EFI_STATUS
BaikalEthSnpInstanceDtor (
  IN  VOID  *Snp
  )
{
  BAIKAL_ETH_INSTANCE *CONST  EthInst = BASE_CR (Snp, BAIKAL_ETH_INSTANCE, Snp);
  EFI_STATUS                  Status;

  Status = gBS->FreePages ((EFI_PHYSICAL_ADDRESS) EthInst, EFI_SIZE_TO_PAGES (sizeof (BAIKAL_ETH_INSTANCE)));
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpInitialize (
  IN  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN  UINTN                         ExtraRxBufSize,  OPTIONAL
  IN  UINTN                         ExtraTxBufSize   OPTIONAL
  )
{
  BAIKAL_ETH_INSTANCE *CONST       EthInst = BASE_CR (Snp, BAIKAL_ETH_INSTANCE, Snp);
  UINTN                            DescIdx;
  UINTN                            Limit;
  volatile BAIKAL_ETH_GMAC_RDESC  *RxDescs;
  EFI_TPL                          SavedTpl;
  volatile BAIKAL_ETH_GMAC_TDESC  *TxDescs;

  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (ExtraRxBufSize != 0 || ExtraTxBufSize != 0) {
    return EFI_UNSUPPORTED;
  }

  DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpInitialize()\n", EthInst->GmacRegs));
  SavedTpl = gBS->RaiseTPL (TPL_CALLBACK);

  switch (Snp->Mode->State) {
  case EfiSimpleNetworkStarted:
    break;
  case EfiSimpleNetworkInitialized:
    DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpInitialize: SNP already initialized\n", EthInst->GmacRegs));
    gBS->RestoreTPL (SavedTpl);
    return EFI_SUCCESS;
  case EfiSimpleNetworkStopped:
    DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpInitialize: SNP not started\n", EthInst->GmacRegs));
    gBS->RestoreTPL (SavedTpl);
    return EFI_NOT_STARTED;
  default:
    DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpInitialize: SNP invalid state = %u\n", EthInst->GmacRegs, Snp->Mode->State));
    gBS->RestoreTPL (SavedTpl);
    return EFI_DEVICE_ERROR;
  }

  EthInst->GmacRegs->DmaBusMode = DMA_BUSMODE_SWR;
  EthInst->GmacRegs->MacGpio   |= MAC_GPIO_GPO;

  for (Limit = 3000; Limit; --Limit) {
    if (!(EthInst->GmacRegs->DmaBusMode & DMA_BUSMODE_SWR)) {
      break;
    }

    gBS->Stall (1000);
  }

  if (!Limit) {
    DEBUG ((EFI_D_ERROR, "BaikalEth(%p)SnpInitialize: GMAC reset not completed\n", EthInst->GmacRegs));
    return EFI_DEVICE_ERROR;
  }

  for (Limit = 3000; Limit; --Limit) {
    if (!(EthInst->GmacRegs->DmaAxiStatus & (DMA_AXISTATUS_AXIRDSTS | DMA_AXISTATUS_AXIWHSTS))) {
      break;
    }

    gBS->Stall (1000);
  }

  EthInst->GmacRegs->MacAddr0Hi =
    Snp->Mode->CurrentAddress.Addr[4] |
    Snp->Mode->CurrentAddress.Addr[5] << 8;

  EthInst->GmacRegs->MacAddr0Lo =
    Snp->Mode->CurrentAddress.Addr[0]       |
    Snp->Mode->CurrentAddress.Addr[1] << 8  |
    Snp->Mode->CurrentAddress.Addr[2] << 16 |
    Snp->Mode->CurrentAddress.Addr[3] << 24;

  // Descriptor list alignment
  EthInst->RxDescBaseAddr = EthInst->RxDescArea +
    (sizeof (BAIKAL_ETH_GMAC_RDESC) - (UINTN) EthInst->RxDescArea % sizeof (BAIKAL_ETH_GMAC_RDESC));

  EthInst->TxDescBaseAddr = EthInst->TxDescArea +
    (sizeof (BAIKAL_ETH_GMAC_TDESC) - (UINTN) EthInst->TxDescArea % sizeof (BAIKAL_ETH_GMAC_TDESC));

  EthInst->GmacRegs->DmaBusMode       |= DMA_BUSMODE_ATDS;
  EthInst->GmacRegs->DmaRxDescBaseAddr = (UINTN) EthInst->RxDescBaseAddr;
  EthInst->GmacRegs->DmaTxDescBaseAddr = (UINTN) EthInst->TxDescBaseAddr;

  RxDescs = (BAIKAL_ETH_GMAC_RDESC *) EthInst->RxDescBaseAddr;
  TxDescs = (BAIKAL_ETH_GMAC_TDESC *) EthInst->TxDescBaseAddr;

  for (DescIdx = 0; DescIdx < RX_DESC_NUM; ++DescIdx) {
    RxDescs[DescIdx].Rdes0 = RDES0_OWN;
    RxDescs[DescIdx].Rdes1 = RDES1_RCH | (RX_BUF_SIZE << RDES1_RBS1_POS);
    RxDescs[DescIdx].Rdes2 = (UINTN) EthInst->RxBufs[DescIdx];
    RxDescs[DescIdx].Rdes3 = (UINTN) &RxDescs[(DescIdx + 1) % RX_DESC_NUM];
  }

  for (DescIdx = 0; DescIdx < TX_DESC_NUM; ++DescIdx) {
    TxDescs[DescIdx].Tdes0 = 0;
    TxDescs[DescIdx].Tdes3 = (UINTN) &TxDescs[(DescIdx + 1) % TX_DESC_NUM];
  }

  EthInst->GmacRegs->DmaStatus = 0xffffffff;
  EthInst->GmacRegs->MacConfig = MAC_CONFIG_BE   |
                                 MAC_CONFIG_DCRS |
                                 MAC_CONFIG_DO   |
                                 MAC_CONFIG_DM   |
                                 MAC_CONFIG_IPC  |
                                 MAC_CONFIG_ACS;

  EthInst->GmacRegs->DmaOperationMode = DMA_OPERATIONMODE_TSF |
                                        DMA_OPERATIONMODE_RSF |
                                        DMA_OPERATIONMODE_ST  |
                                        DMA_OPERATIONMODE_SR;

  EthInst->GmacRegs->MacConfig       |= MAC_CONFIG_TE | MAC_CONFIG_RE;
  EthInst->GmacRegs->DmaRxPollDemand  = 0;

  Snp->Mode->State = EfiSimpleNetworkInitialized;
  gBS->RestoreTPL (SavedTpl);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpMCastIpToMac (
  IN   EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN   BOOLEAN                       Ipv6,
  IN   EFI_IP_ADDRESS               *Ip,
  OUT  EFI_MAC_ADDRESS              *McastMacAddr
  )
{
  EFI_TPL  SavedTpl;

  if (Snp == NULL || Ip == NULL || McastMacAddr == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SavedTpl = gBS->RaiseTPL (TPL_CALLBACK);

  switch (Snp->Mode->State) {
  case EfiSimpleNetworkInitialized:
    break;
  case EfiSimpleNetworkStopped:
    gBS->RestoreTPL (SavedTpl);
    return EFI_NOT_STARTED;
  default:
    gBS->RestoreTPL (SavedTpl);
    return EFI_DEVICE_ERROR;
  }

  gBS->RestoreTPL (SavedTpl);
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpNvData (
  IN      EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN      BOOLEAN                       ReadWrite,
  IN      UINTN                         Offset,
  IN      UINTN                         BufSize,
  IN OUT  VOID                         *Buf
  )
{
  EFI_TPL  SavedTpl;

  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SavedTpl = gBS->RaiseTPL (TPL_CALLBACK);

  switch (Snp->Mode->State) {
  case EfiSimpleNetworkInitialized:
    break;
  case EfiSimpleNetworkStopped:
    gBS->RestoreTPL (SavedTpl);
    return EFI_NOT_STARTED;
  default:
    gBS->RestoreTPL (SavedTpl);
    return EFI_DEVICE_ERROR;
  }

  gBS->RestoreTPL (SavedTpl);
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpReset (
  IN  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN  BOOLEAN                       ExtendedVerification
  )
{
  EFI_TPL  SavedTpl;

  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SavedTpl = gBS->RaiseTPL (TPL_CALLBACK);

  switch (Snp->Mode->State) {
  case EfiSimpleNetworkInitialized:
    break;
  case EfiSimpleNetworkStopped:
    gBS->RestoreTPL (SavedTpl);
    return EFI_NOT_STARTED;
  default:
    gBS->RestoreTPL (SavedTpl);
    return EFI_DEVICE_ERROR;
  }

  gBS->RestoreTPL (SavedTpl);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpReceiveFilters (
  IN  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN  UINT32                        Enable,
  IN  UINT32                        Disable,
  IN  BOOLEAN                       ResetMCastFilter,
  IN  UINTN                         MCastFilterCnt,   OPTIONAL
  IN  EFI_MAC_ADDRESS              *MCastFilter       OPTIONAL
  )
{
  BAIKAL_ETH_INSTANCE *CONST  EthInst = BASE_CR (Snp, BAIKAL_ETH_INSTANCE, Snp);
  CONST UINT32  ResultingMsk = Enable & ~Disable;
  EFI_TPL       SavedTpl;

  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SavedTpl = gBS->RaiseTPL (TPL_CALLBACK);

  switch (Snp->Mode->State) {
  case EfiSimpleNetworkInitialized:
    break;
  case EfiSimpleNetworkStarted:
    DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpReceiveFilters: SNP not initialized\n", EthInst->GmacRegs));
    gBS->RestoreTPL (SavedTpl);
    return EFI_DEVICE_ERROR;
  case EfiSimpleNetworkStopped:
    DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpReceiveFilters: SNP not started\n", EthInst->GmacRegs));
    gBS->RestoreTPL (SavedTpl);
    return EFI_NOT_STARTED;
  default:
    DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpReceiveFilters: SNP invalid state = %u\n", EthInst->GmacRegs, Snp->Mode->State));
    gBS->RestoreTPL (SavedTpl);
    return EFI_DEVICE_ERROR;
  }

  DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpReceiveFilters: FilterReq = %c%c%c%c%c\n",
    EthInst->GmacRegs,
    ResultingMsk & EFI_SIMPLE_NETWORK_RECEIVE_UNICAST               ? 'U' : 'u',
    ResultingMsk & EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST             ? 'B' : 'b',
    ResultingMsk & EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST             ? 'M' : 'm',
    ResultingMsk & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS           ? 'P' : 'p',
    ResultingMsk & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST ? 'W' : 'w'
    ));

  if (ResultingMsk & EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST) {
    EthInst->GmacRegs->MacFrameFilter &= ~MAC_FRAMEFILTER_DBF;
  } else {
    EthInst->GmacRegs->MacFrameFilter |=  MAC_FRAMEFILTER_DBF;
  }

  if (ResultingMsk & (EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST |
                      EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST)) {
    EthInst->GmacRegs->MacFrameFilter |=  MAC_FRAMEFILTER_PM;
  } else {
    EthInst->GmacRegs->MacFrameFilter &= ~MAC_FRAMEFILTER_PM;
  }

  if (ResultingMsk & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS) {
    EthInst->GmacRegs->MacFrameFilter |=  MAC_FRAMEFILTER_PR;
  } else {
    EthInst->GmacRegs->MacFrameFilter &= ~MAC_FRAMEFILTER_PR;
  }

  Snp->Mode->ReceiveFilterSetting = BaikalEthSnpGetReceiveFilterSetting (Snp);
  gBS->RestoreTPL (SavedTpl);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpShutdown (
  IN  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp
  )
{
  EFI_TPL  SavedTpl;

  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SavedTpl = gBS->RaiseTPL (TPL_CALLBACK);

  switch (Snp->Mode->State) {
  case EfiSimpleNetworkInitialized:
    break;
  case EfiSimpleNetworkStopped:
    gBS->RestoreTPL (SavedTpl);
    return EFI_NOT_STARTED;
  default:
    gBS->RestoreTPL (SavedTpl);
    return EFI_DEVICE_ERROR;
  }

  gBS->RestoreTPL (SavedTpl);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpStart (
  IN  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp
  )
{
  EFI_TPL  SavedTpl;

  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SavedTpl = gBS->RaiseTPL (TPL_CALLBACK);

  switch (Snp->Mode->State) {
  case EfiSimpleNetworkStopped:
    break;
  case EfiSimpleNetworkStarted:
  case EfiSimpleNetworkInitialized:
    gBS->RestoreTPL (SavedTpl);
    return EFI_ALREADY_STARTED;
  default:
    gBS->RestoreTPL (SavedTpl);
    return EFI_DEVICE_ERROR;
  }

  Snp->Mode->State = EfiSimpleNetworkStarted;
  gBS->RestoreTPL (SavedTpl);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpStationAddress (
  IN  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN  BOOLEAN                       Reset,
  IN  EFI_MAC_ADDRESS              *NewMacAddr  OPTIONAL
  )
{
  BAIKAL_ETH_INSTANCE *CONST  EthInst = BASE_CR (Snp, BAIKAL_ETH_INSTANCE, Snp);
  EFI_TPL  SavedTpl;

  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpStationAddress()\n", EthInst->GmacRegs));
  SavedTpl = gBS->RaiseTPL (TPL_CALLBACK);

  switch (Snp->Mode->State) {
  case EfiSimpleNetworkInitialized:
    break;
  case EfiSimpleNetworkStarted:
    DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpStationAddress: SNP not initialized\n", EthInst->GmacRegs));
    gBS->RestoreTPL (SavedTpl);
    return EFI_DEVICE_ERROR;
  case EfiSimpleNetworkStopped:
    DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpStationAddress: SNP not started\n", EthInst->GmacRegs));
    gBS->RestoreTPL (SavedTpl);
    return EFI_NOT_STARTED;
  default:
    DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpStationAddress: SNP invalid state = %u\n", EthInst->GmacRegs, Snp->Mode->State));
    gBS->RestoreTPL (SavedTpl);
    return EFI_DEVICE_ERROR;
  }

  if (Reset) {
    DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpStationAddress: reset MAC address\n", EthInst->GmacRegs));
    Snp->Mode->CurrentAddress = Snp->Mode->PermanentAddress;
  } else {
    if (NewMacAddr == NULL) {
      gBS->RestoreTPL (SavedTpl);
      return EFI_INVALID_PARAMETER;
    }

    gBS->CopyMem (&Snp->Mode->CurrentAddress, NewMacAddr, sizeof (EFI_MAC_ADDRESS));
  }

  EthInst->GmacRegs->MacAddr0Hi =
    Snp->Mode->CurrentAddress.Addr[4] |
    Snp->Mode->CurrentAddress.Addr[5] << 8;

  EthInst->GmacRegs->MacAddr0Lo =
    Snp->Mode->CurrentAddress.Addr[0]       |
    Snp->Mode->CurrentAddress.Addr[1] << 8  |
    Snp->Mode->CurrentAddress.Addr[2] << 16 |
    Snp->Mode->CurrentAddress.Addr[3] << 24;

  DEBUG ((
    EFI_D_NET,
    "BaikalEth(%p)SnpStationAddress: current MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
    EthInst->GmacRegs,
    Snp->Mode->CurrentAddress.Addr[0],
    Snp->Mode->CurrentAddress.Addr[1],
    Snp->Mode->CurrentAddress.Addr[2],
    Snp->Mode->CurrentAddress.Addr[3],
    Snp->Mode->CurrentAddress.Addr[4],
    Snp->Mode->CurrentAddress.Addr[5]
    ));

  gBS->RestoreTPL (SavedTpl);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpStatistics (
  IN   EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN   BOOLEAN                       Reset,
  IN   OUT UINTN                    *StatisticsSize,  OPTIONAL
  OUT  EFI_NETWORK_STATISTICS       *StatisticsTable  OPTIONAL
  )
{
  EFI_TPL  SavedTpl;

  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SavedTpl = gBS->RaiseTPL (TPL_CALLBACK);

  switch (Snp->Mode->State) {
  case EfiSimpleNetworkInitialized:
    break;
  case EfiSimpleNetworkStopped:
    gBS->RestoreTPL (SavedTpl);
    return EFI_NOT_STARTED;
  default:
    gBS->RestoreTPL (SavedTpl);
    return EFI_DEVICE_ERROR;
  }

  gBS->RestoreTPL (SavedTpl);
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpStop (
  IN  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp
  )
{
  EFI_TPL  SavedTpl;

  if (Snp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SavedTpl = gBS->RaiseTPL (TPL_CALLBACK);

  switch (Snp->Mode->State) {
  case EfiSimpleNetworkStarted:
  case EfiSimpleNetworkInitialized:
    break;
  case EfiSimpleNetworkStopped:
    gBS->RestoreTPL (SavedTpl);
    return EFI_NOT_STARTED;
  default:
    gBS->RestoreTPL (SavedTpl);
    return EFI_DEVICE_ERROR;
  }

  Snp->Mode->State = EfiSimpleNetworkStopped;
  gBS->RestoreTPL (SavedTpl);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpReceive ( // Receive a packet from a network interface
  IN      EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  OUT     UINTN                        *HdrSize,  OPTIONAL
  IN OUT  UINTN                        *BufSize,
  OUT     VOID                         *Buf,
  OUT     EFI_MAC_ADDRESS              *SrcAddr,  OPTIONAL
  OUT     EFI_MAC_ADDRESS              *DstAddr,  OPTIONAL
  OUT     UINT16                       *Protocol  OPTIONAL
  )
{
  BAIKAL_ETH_INSTANCE *CONST       EthInst = BASE_CR (Snp, BAIKAL_ETH_INSTANCE, Snp);
  volatile BAIKAL_ETH_GMAC_RDESC  *RxDescs;
  EFI_TPL                          SavedTpl;

  if (Snp == NULL || BufSize == NULL || Buf == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SavedTpl = gBS->RaiseTPL (TPL_CALLBACK);

  switch (Snp->Mode->State) {
  case EfiSimpleNetworkInitialized:
    break;
  case EfiSimpleNetworkStarted:
    DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpReceive: SNP not initialized\n", EthInst->GmacRegs));
    gBS->RestoreTPL (SavedTpl);
    return EFI_DEVICE_ERROR;
  case EfiSimpleNetworkStopped:
    DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpReceive: SNP not started\n", EthInst->GmacRegs));
    gBS->RestoreTPL (SavedTpl);
    return EFI_NOT_STARTED;
  default:
    DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpReceive: SNP invalid state = %u\n", EthInst->GmacRegs, Snp->Mode->State));
    gBS->RestoreTPL (SavedTpl);
    return EFI_DEVICE_ERROR;
  }

  EFI_STATUS Status = EFI_NOT_READY;
  RxDescs = (BAIKAL_ETH_GMAC_RDESC *) EthInst->RxDescBaseAddr;

  while (!(RxDescs[EthInst->RxDescReadIdx].Rdes0 & RDES0_OWN) && (Status == EFI_NOT_READY)) {
    if (((RDES0_FS | RDES0_LS) & RxDescs[EthInst->RxDescReadIdx].Rdes0) ==
         (RDES0_FS | RDES0_LS)) {
      CONST UINTN FrameLen = (RxDescs[EthInst->RxDescReadIdx].Rdes0 >> RDES0_FL_POS) & RDES0_FL_MSK;

      if (*BufSize < FrameLen) {
        DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpReceive: receive BufSize(%u) < FrameLen(%u)\n", EthInst->GmacRegs, *BufSize, FrameLen));
        Status = EFI_BUFFER_TOO_SMALL;
      }

      *BufSize = FrameLen;

      if (Status != EFI_BUFFER_TOO_SMALL) {
        gBS->CopyMem (Buf, (VOID *) EthInst->RxBufs[EthInst->RxDescReadIdx], FrameLen);

        if (HdrSize != NULL) {
          *HdrSize = Snp->Mode->MediaHeaderSize;
        }

        if (DstAddr != NULL) {
          gBS->CopyMem (DstAddr, Buf, NET_ETHER_ADDR_LEN);
        }

        if (SrcAddr != NULL) {
          gBS->CopyMem (SrcAddr, (UINT8*) Buf + 6, NET_ETHER_ADDR_LEN);
        }

        if (Protocol != NULL) {
          *Protocol = NTOHS (*(UINT16*)((UINT8*) Buf + 12));
        }

        Status = EFI_SUCCESS;
      }
    }

    RxDescs[EthInst->RxDescReadIdx].Rdes0 = RDES0_OWN;
    EthInst->RxDescReadIdx = (EthInst->RxDescReadIdx + 1) % RX_DESC_NUM;

    if (EthInst->GmacRegs->DmaStatus & DMA_STATUS_RU) {
      EthInst->GmacRegs->DmaStatus   = DMA_STATUS_RU;
      EthInst->GmacRegs->DmaRxPollDemand = 0;
    }
  }

  gBS->RestoreTPL (SavedTpl);
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
BaikalEthSnpTransmit (
  IN  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp,
  IN  UINTN                         HdrSize,
  IN  UINTN                         BufSize,
  IN  VOID                         *Buf,
  IN  EFI_MAC_ADDRESS              *SrcAddr,  OPTIONAL
  IN  EFI_MAC_ADDRESS              *DstAddr,  OPTIONAL
  IN  UINT16                       *Protocol  OPTIONAL
  )
{
  BAIKAL_ETH_INSTANCE *CONST       EthInst = BASE_CR (Snp, BAIKAL_ETH_INSTANCE, Snp);
  EFI_TPL                          SavedTpl;
  EFI_STATUS                       Status;
  volatile BAIKAL_ETH_GMAC_TDESC  *TxDescs;

  if (Snp == NULL || Buf == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SavedTpl = gBS->RaiseTPL (TPL_CALLBACK);

  switch (Snp->Mode->State) {
  case EfiSimpleNetworkInitialized:
    break;
  case EfiSimpleNetworkStarted:
    DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpTransmit: SNP not initialized\n", EthInst->GmacRegs));
    gBS->RestoreTPL (SavedTpl);
    return EFI_DEVICE_ERROR;
  case EfiSimpleNetworkStopped:
    DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpTransmit: SNP not started\n", EthInst->GmacRegs));
    gBS->RestoreTPL (SavedTpl);
    return EFI_NOT_STARTED;
  default:
    DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpTransmit: SNP invalid state = %u\n", EthInst->GmacRegs, Snp->Mode->State));
    gBS->RestoreTPL (SavedTpl);
    return EFI_DEVICE_ERROR;
  }

  if (HdrSize != 0) {
    if (HdrSize != Snp->Mode->MediaHeaderSize) {
      DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpTransmit: HdrSize(%u) != Snp->Mode->MediaHeaderSize(%u)\n", EthInst->GmacRegs, HdrSize, Snp->Mode->MediaHeaderSize));
      gBS->RestoreTPL (SavedTpl);
      return EFI_INVALID_PARAMETER;
    }

    if (DstAddr == NULL || Protocol == NULL) {
      DEBUG ((EFI_D_NET, "BaikalEth(%p)SnpTransmit: Hdr DstAddr(%p) or Protocol(%p) is NULL\n", EthInst->GmacRegs, DstAddr, Protocol));
      gBS->RestoreTPL (SavedTpl);
      return EFI_INVALID_PARAMETER;
    }
  }

  if (!Snp->Mode->MediaPresent) {
    gBS->RestoreTPL (SavedTpl);
    return EFI_NOT_READY;
  }

  if (HdrSize != 0) {
    UINT16 EtherType = HTONS (*Protocol);

    gBS->CopyMem (Buf, DstAddr, NET_ETHER_ADDR_LEN);

    if (SrcAddr != NULL) {
      gBS->CopyMem ((UINT8 *) Buf + NET_ETHER_ADDR_LEN, SrcAddr, NET_ETHER_ADDR_LEN);
    } else {
      gBS->CopyMem ((UINT8 *) Buf + NET_ETHER_ADDR_LEN, &Snp->Mode->CurrentAddress, NET_ETHER_ADDR_LEN);
    }

    gBS->CopyMem ((UINT8 *) Buf + NET_ETHER_ADDR_LEN * 2, &EtherType, 2);
  }

  TxDescs = (BAIKAL_ETH_GMAC_TDESC *) EthInst->TxDescBaseAddr;

  if (!(TxDescs[EthInst->TxDescWriteIdx].Tdes0 & TDES0_OWN) &&
      ((EthInst->TxDescWriteIdx + 1) % TX_DESC_NUM) != EthInst->TxDescWriteIdx) {
    TxDescs[EthInst->TxDescWriteIdx].Tdes2 = (UINTN) Buf;
    TxDescs[EthInst->TxDescWriteIdx].Tdes1 = BufSize << TDES1_TBS1_POS;
    TxDescs[EthInst->TxDescWriteIdx].Tdes0 = TDES0_OWN | TDES0_IC | TDES0_LS | TDES0_FS | TDES0_TCH;
    EthInst->TxDescWriteIdx = (EthInst->TxDescWriteIdx + 1) % TX_DESC_NUM;
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_NOT_READY;
  }

  EthInst->GmacRegs->DmaOperationMode |= DMA_OPERATIONMODE_ST;
  EthInst->GmacRegs->MacConfig        |= MAC_CONFIG_TE;
  EthInst->GmacRegs->DmaTxPollDemand   = 0;
  gBS->RestoreTPL (SavedTpl);
  return Status;
}
