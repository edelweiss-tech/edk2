// Copyright (c) 2019-2020 Baikal Electronics JSC
// Author: Mikhail Ivanov <michail.ivanov@baikalelectronics.ru>

#ifndef BAIKAL_ETH_GMAC_DESCS_H_
#define BAIKAL_ETH_GMAC_DESCS_H_

typedef struct {
  UINT32  Rdes0;
  UINT32  Rdes1;
  UINT32  Rdes2;
  UINT32  Rdes3;
  UINT32  Rdes4;
  UINT32  Rdes5;
  UINT32  Rdes6;
  UINT32  Rdes7;
} BAIKAL_ETH_GMAC_RDESC;

typedef struct {
  UINT32  Tdes0;
  UINT32  Tdes1;
  UINT32  Tdes2;
  UINT32  Tdes3;
  UINT32  Tdes4;
  UINT32  Tdes5;
  UINT32  Tdes6;
  UINT32  Tdes7;
} BAIKAL_ETH_GMAC_TDESC;

#define RDES0_OWN       (1 << 31) // Own bit
#define RDES0_AFM       (1 << 30) // Destination address filter fail
#define RDES0_FL_POS    16        // Frame length bits position
#define RDES0_FL_MSK    0x3fff    // Frame length bitmask
#define RDES0_ES        (1 << 15) // Error summary
#define RDES0_DESCERR   (1 << 14) // Descriptor error
#define RDES0_SAF       (1 << 13) // Source address filter fail
#define RDES0_LE        (1 << 12) // Length error
#define RDES0_OE        (1 << 11) // Overflow error
#define RDES0_VLAN      (1 << 10) // VLAN tag
#define RDES0_FS        (1 <<  9) // First descriptor
#define RDES0_LS        (1 <<  8) // Last descriptor
#define RDES0_TSCRCERR  (1 <<  7) // Timestamp available, IP checksum error of giant frame
#define RDES0_LC        (1 <<  6) // Late collision
#define RDES0_FT        (1 <<  5) // Frame type
#define RDES0_RWT       (1 <<  4) // Receive watchdog timeout
#define RDES0_RE        (1 <<  3) // Receive error
#define RDES0_DRIBERR   (1 <<  2) // Dribble bit error
#define RDES0_CE        (1 <<  1) // CRC error
#define RDES0_EXTSTS    (1 <<  0) // Extended status available / Rx MAC address

#define RDES1_DIC       (1 << 31) // Disable interrupt on completion
#define RDES1_RBS2_POS  16        // Receive buffer 2 size bits position
#define RDES1_RER       (1 << 15) // Receive end of ring
#define RDES1_RCH       (1 << 14) // Second address chained
#define RDES1_RBS1_POS  0         // Receive buffer 1 size bits position

#define TDES0_OWN       (1 << 31) // Own bit
#define TDES0_IC        (1 << 30) // Interrupt on completion
#define TDES0_LS        (1 << 29) // Last segment
#define TDES0_FS        (1 << 28) // First segment
#define TDES0_DC        (1 << 27) // Disable CRC
#define TDES0_DP        (1 << 26) // Disable pad
#define TDES0_TTSE      (1 << 25) // Transmit timestamp enable
#define TDES0_CRCR      (1 << 24) // CRC replacement control
#define TDES0_CIC_POS   22        // Checksum insertion control bits position
#define TDES0_TER       (1 << 21) // Transmit end of ring
#define TDES0_TCH       (1 << 20) // Second address chained
#define TDES0_VLIC_POS  18        // VLAN insertion control bits position
#define TDES0_TTSS      (1 << 17) // Transmit timestamp status
#define TDES0_IHE       (1 << 16) // IP header error
#define TDES0_ES        (1 << 15) // Error summary
#define TDES0_JT        (1 << 14) // Jabber timeout
#define TDES0_FF        (1 << 13) // Frame flushed
#define TDES0_IPE       (1 << 12) // IP payload error
#define TDES0_LOC       (1 << 11) // Loss of carrier
#define TDES0_NC        (1 << 10) // No carrier
#define TDES0_LC        (1 <<  9) // Late collision
#define TDES0_EC        (1 <<  8) // Excessive collision
#define TDES0_VF        (1 <<  7) // VLAN frame
#define TDES0_CC_POS    3         // Collision count (status field) bits position
#define TDES0_ED        (1 <<  1) // Underflow error
#define TDES0_DB        (1 <<  0) // Deferred bit

#define TDES1_SAIC_POS  29        // SA insertion control bits position
#define TDES1_TBS2_POS  16        // Transmit buffer 2 size bits position
#define TDES1_TBS1_POS  0         // Transmit buffer 1 size bits position

#endif // BAIKAL_ETH_GMAC_DESCS_H_
