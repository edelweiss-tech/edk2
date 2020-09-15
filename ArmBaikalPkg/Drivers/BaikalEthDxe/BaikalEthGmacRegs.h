// Copyright (c) 2019-2020 Baikal Electronics JSC
// Author: Mikhail Ivanov <michail.ivanov@baikalelectronics.ru>

#ifndef BAIKAL_ETH_GMAC_REGS_H_
#define BAIKAL_ETH_GMAC_REGS_H_

typedef struct {
  UINT32  MacConfig;
  UINT32  MacFrameFilter;
  UINT32  MacHashHi;
  UINT32  MacHashLo;
  UINT32  MacMiiAddr;
  UINT32  MacMiiData;
  UINT32  MacFlowCtrl;
  UINT32  MacVlanTag;
  UINT32  MacVersion;
  UINT32  MacDebug;
  UINT32  MacWakeupFilter;
  UINT32  MacPmtCtrlStatus;
  UINT32  MacLpiCtrlStatus;
  UINT32  MacLpiTimersCtrl;
  UINT32  MacInt;
  UINT32  MacIntMsk;
  UINT32  MacAddr0Hi;
  UINT32  MacAddr0Lo;
  UINT32  MacAddr1Hi;
  UINT32  MacAddr1Lo;
  UINT32  MacAddr2Hi;
  UINT32  MacAddr2Lo;
  UINT32  MacAddr3Hi;
  UINT32  MacAddr3Lo;
  UINT32  MacAddr4Hi;
  UINT32  MacAddr4Lo;
  UINT32  MacAddr5Hi;
  UINT32  MacAddr5Lo;
  UINT32  MacAddr6Hi;
  UINT32  MacAddr6Lo;
  UINT32  MacAddr7Hi;
  UINT32  MacAddr7Lo;
  UINT32  MacAddr8Hi;
  UINT32  MacAddr8Lo;
  UINT32  MacReserved[20];
  UINT32  MacMiiStatus;
  UINT32  MacWdtTimeout;
  UINT32  MacGpio;

  UINT32  Reserved[967];

  UINT32  DmaBusMode;
  UINT32  DmaTxPollDemand;
  UINT32  DmaRxPollDemand;
  UINT32  DmaRxDescBaseAddr;
  UINT32  DmaTxDescBaseAddr;
  UINT32  DmaStatus;
  UINT32  DmaOperationMode;
  UINT32  DmaIntEn;
  UINT32  DmaMissedFrameCntr;
  UINT32  DmaRxWatchdog;
  UINT32  DmaAxiBusMode;
  UINT32  DmaAxiStatus;
  UINT32  DmaReserved[6];
  UINT32  DmaCurTxDescAddr;
  UINT32  DmaCurRxDescAddr;
  UINT32  DmaCurTxBufAddr;
  UINT32  DmaCurRxBufAddr;
  UINT32  DmaHwFeature;
} BAIKAL_ETH_GMAC_REGS;

#define MAC_CONFIG_SARC_POS         28        // Source address insertion or replacement control
#define MAC_CONFIG_TWOKPE           (1 << 27) // IEEE 802.3as support for 2K packets
#define MAC_CONFIG_SFTERR           (1 << 26) // SMII force transmit error
#define MAC_CONFIG_CST              (1 << 25) // CRC stripping for type frames
#define MAC_CONFIG_TC               (1 << 24) // Transmit configuration in RGMII
#define MAC_CONFIG_WD               (1 << 23) // Watchdog disable
#define MAC_CONFIG_JD               (1 << 22) // Jabber disable
#define MAC_CONFIG_BE               (1 << 21) // Frame burst enable
#define MAC_CONFIG_JE               (1 << 20) // Jumbo frame enable
#define MAC_CONFIG_IFG_POS          17        // Inter-frame gap
#define MAC_CONFIG_DCRS             (1 << 16) // Disable carrier sense during transmission
#define MAC_CONFIG_PS               (1 << 15) // Port selecct
#define MAC_CONFIG_FES              (1 << 14) // Speed
#define MAC_CONFIG_DO               (1 << 13) // Disable receive own
#define MAC_CONFIG_LM               (1 << 12) // Loopback mode
#define MAC_CONFIG_DM               (1 << 11) // Duplex mode
#define MAC_CONFIG_IPC              (1 << 10) // Checksum offload
#define MAC_CONFIG_DR               (1 <<  9) // Disable retry
#define MAC_CONFIG_LUD              (1 <<  8) // Link up or down
#define MAC_CONFIG_ACS              (1 <<  7) // Automatic pad or CRC stripping
#define MAC_CONFIG_BL_POS            5        // Back-off limit bits position
#define MAC_CONFIG_DC               (1 <<  4) // Deferral check
#define MAC_CONFIG_TE               (1 <<  3) // Transmitter enable
#define MAC_CONFIG_RE               (1 <<  2) // Receiver enable
#define MAC_CONFIG_PRELEN_MSK       0x3       // Preamble length for transmit frames

#define MAC_FRAMEFILTER_RA          (1 << 31) // Receive all
#define MAC_FRAMEFILTER_DNTU        (1 << 21) // Drop non-TCP/UDP over IP frames
#define MAC_FRAMEFILTER_IPFE        (1 << 20) // Layer 3 and layer 4 filter enable
#define MAC_FRAMEFILTER_VTFE        (1 << 16) // VLAN tag filter enable
#define MAC_FRAMEFILTER_HPF         (1 << 10) // Hash or perfect filter
#define MAC_FRAMEFILTER_SAF         (1 <<  9) // Source address filter enable
#define MAC_FRAMEFILTER_SAIF        (1 <<  8) // SA inverse filtering
#define MAC_FRAMEFILTER_PCF_POS      6        // Pass control frames bits position
#define MAC_FRAMEFILTER_DBF         (1 <<  5) // Disable broadcast frames
#define MAC_FRAMEFILTER_PM          (1 <<  4) // Pass all multicast
#define MAC_FRAMEFILTER_DAIF        (1 <<  3) // DA inverse filtering
#define MAC_FRAMEFILTER_HMC         (1 <<  2) // Hash multicast
#define MAC_FRAMEFILTER_HUC         (1 <<  1) // Hash unicast
#define MAC_FRAMEFILTER_PR          (1 <<  0) // Promiscuous mode

#define MAC_MIIADDR_PA_POS          11        // PHY address bits position
#define MAC_MIIADDR_GR_POS           6        // GMII register bits position
#define MAC_MIIADDR_CR_POS           2        // CSR clock range bits position
#define MAC_MIIADDR_GW              (1 <<  1) // GMII write
#define MAC_MIIADDR_GB              (1 <<  0) // GMII busy

#define MAC_MIISTATUS_SMIIDRXS      (1 << 16) // Delay SMII RX data sampling
#define MAC_MIISTATUS_FALSCARDET    (1 <<  5) // False carrier detected
#define MAC_MIISTATUS_JABTO         (1 <<  4) // Jabber timeout
#define MAC_MIISTATUS_LNKSTS        (1 <<  3) // Link status
#define MAC_MIISTATUS_LNKSPEED_POS   2        // Link speed bits position
#define MAC_MIISTATUS_LNKMOD        (1 <<  0) // Link mode

#define MAC_GPIO_GPIT               (1 << 24) // GPI type
#define MAC_GPIO_GPIE               (1 << 16) // GPI interrupt enable
#define MAC_GPIO_GPO                (1 <<  8) // General purpose output
#define MAC_GPIO_GPIS               (1 <<  0) // General purpose input status

#define DMA_BUSMODE_RIB             (1 << 31) // Rebuild INCRx burst
#define DMA_BUSMODE_PRWG_POS        28        // Channel priority weights bits position
#define DMA_BUSMODE_TXPR            (1 << 27) // Transmit priority
#define DMA_BUSMODE_MB              (1 << 26) // Mixed burst
#define DMA_BUSMODE_AAL             (1 << 25) // Address aligned beats
#define DMA_BUSMODE_PBLX8           (1 << 24) // PBLx8 mode
#define DMA_BUSMODE_USP             (1 << 23) // Use separate PBL
#define DMA_BUSMODE_RPBL_POS        17        // Rx DMA PBL bits position
#define DMA_BUSMODE_FB              (1 << 16) // Fixed burst
#define DMA_BUSMODE_PR_POS          14        // Priority ratio bits position
#define DMA_BUSMODE_PBL_POS          8        // Programmable burst length bits position
#define DMA_BUSMODE_ATDS            (1 <<  7) // Alternate descriptor size
#define DMA_BUSMODE_DSL_POS          2        // Descriptor skip length bits position
#define DMA_BUSMODE_DA              (1 <<  1) // DMA arbitration scheme
#define DMA_BUSMODE_SWR             (1 <<  0) // Software Reset

#define DMA_STATUS_GLPII            (1 << 30) // GMAC LPI interrupt
#define DMA_STATUS_TTI              (1 << 29) // Timestamp trigger interrupt
#define DMA_STATUS_GPI              (1 << 28) // GMAC PMT interrupt
#define DMA_STATUS_GMI              (1 << 27) // GMAC MMC interrupt
#define DMA_STATUS_GLI              (1 << 26) // GMAC line interface interrupt
#define DMA_STATUS_EB_POS           23        // Error bits position
#define DMA_STATUS_TS_POS           20        // Transmit process state bits position
#define DMA_STATUS_RS_POS           17        // Received process state bits position
#define DMA_STATUS_NIS              (1 << 16) // Normal interrupt summary
#define DMA_STATUS_AIS              (1 << 15) // Abnormal interrupt summary
#define DMA_STATUS_ERI              (1 << 14) // Early receive interrupt
#define DMA_STATUS_FBI              (1 << 13) // Fatal bus error interrupt
#define DMA_STATUS_ETI              (1 << 10) // Early transmit interrupt
#define DMA_STATUS_RWT              (1 <<  9) // Receive watchdog timeout
#define DMA_STATUS_RPS              (1 <<  8) // Receive process stopped
#define DMA_STATUS_RU               (1 <<  7) // Receive buffer unavailable
#define DMA_STATUS_RI               (1 <<  6) // Receive interrupt
#define DMA_STATUS_UNF              (1 <<  5) // Transmit underflow
#define DMA_STATUS_OVF              (1 <<  4) // Receive overflow
#define DMA_STATUS_TJT              (1 <<  3) // Transmit jabber timeout
#define DMA_STATUS_TU               (1 <<  2) // Transmit buffer unavailable
#define DMA_STATUS_TPS              (1 <<  1) // Transmit process stopped
#define DMA_STATUS_TI               (1 <<  0) // Transmit interrupt

#define DMA_OPERATIONMODE_DT        (1 << 26) // Disable dropping of TCP/IP checksum error frames
#define DMA_OPERATIONMODE_RSF       (1 << 25) // Receive store and forward
#define DMA_OPERATIONMODE_DFF       (1 << 24) // Disable flushing of received frames
#define DMA_OPERATIONMODE_RFA2      (1 << 23) // MSB of threshold for activating flow control
#define DMA_OPERATIONMODE_RFD2      (1 << 22) // MSB of threshold for deactivating flow control
#define DMA_OPERATIONMODE_TSF       (1 << 21) // Transmit store and forward
#define DMA_OPERATIONMODE_FTF       (1 << 20) // Flush transmit FIFO
#define DMA_OPERATIONMODE_TTC_POS   14        // Transmit threshold control bits position
#define DMA_OPERATIONMODE_ST        (1 << 13) // Start or stop transmission command
#define DMA_OPERATIONMODE_RFD_POS   11        // Threshold for deactivating flow control bits position
#define DMA_OPERATIONMODE_RFA_POS    9        // Threshold for activating flow control bits position
#define DMA_OPERATIONMODE_EFC       (1 <<  8) // Enable HW flow control
#define DMA_OPERATIONMODE_FEF       (1 <<  7) // Forward error frames
#define DMA_OPERATIONMODE_FUF       (1 <<  6) // Forward undersized good frames
#define DMA_OPERATIONMODE_DGF       (1 <<  5) // Drop giant frames
#define DMA_OPERATIONMODE_RTC_POS    3        // Receive threshold control bits position
#define DMA_OPERATIONMODE_OSF       (1 <<  2) // Operate on second frame
#define DMA_OPERATIONMODE_SR        (1 <<  1) // Start of stop receive

#define DMA_AXISTATUS_AXIRDSTS      (1 <<  1) // AXI master read channel status
#define DMA_AXISTATUS_AXIWHSTS      (1 <<  0) // AXI master write channel status

#endif // BAIKAL_ETH_GMAC_REGS_H_
