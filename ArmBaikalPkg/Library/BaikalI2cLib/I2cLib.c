// Copyright (c) 2020 Baikal Electronics JSC
// Author: Mikhail Ivanov <michail.ivanov@baikalelectronics.ru>

#include <Uefi.h>
#include <Library/BaikalI2cLib.h>

#define BAIKAL_I2C_BASE            0x20250000
#define BAIKAL_I2C_OFFSET          0x10000
#define BAIKAL_I2C_REGS(bus)       (volatile I2C_CONTROLLER_REGS * CONST)((EFI_PHYSICAL_ADDRESS)BAIKAL_I2C_BASE + BAIKAL_I2C_OFFSET * (bus))

#define IC_SPEED_MODE_STANDARD     1
#define IC_SPEED_MODE_FAST         2
#define IC_SPEED_MODE_MAX          3

#define IC_CON_MASTER_MODE         (1 << 0)
#define IC_CON_SPEED               (IC_SPEED_MODE_FAST << 1)
#define IC_CON_IC_SLAVE_DISABLE    (1 << 6)

#define IC_DATA_CMD_CMD            (1 << 8)

#define IC_RAW_INTR_STAT_STOP_DET  (1 << 9)

#define IC_STATUS_MST_ACTIVITY     (1 << 5)
#define IC_STATUS_RFF              (1 << 4)
#define IC_STATUS_RFNE             (1 << 3)
#define IC_STATUS_TFE              (1 << 2)
#define IC_STATUS_TFNF             (1 << 1)

#define I2C_TIMEOUT                1000000
#define IC_CLK                         166
#define CONFIG_SYS_HZ                 1200
#define NANO_TO_MICRO                 1000
#define MIN_FS_SCL_HIGHTIME            600
#define MIN_FS_SCL_LOWTIME            1300

typedef struct {
  UINT32  IcCon;
  UINT32  IcTar;
  UINT32  IcSar;
  UINT32  IcHsMaddr;
  UINT32  IcDataCmd;
  UINT32  IcSsSclHcnt;
  UINT32  IcSsSclLcnt;
  UINT32  IcFsSclHcnt;
  UINT32  IcFsSclLcnt;
  UINT32  IcHsSclHcnt;
  UINT32  IcHsSclLcnt;
  UINT32  IcIntrStat;
  UINT32  IcIntrMask;
  UINT32  IcRawIntrStat;
  UINT32  IcRxTl;
  UINT32  IcTxTl;
  UINT32  IcClrIntr;
  UINT32  IcClrRxUnder;
  UINT32  IcClrRxOver;
  UINT32  IcClrTxOver;
  UINT32  IcClrRdReq;
  UINT32  IcClrTxAbrt;
  UINT32  IcClrRxDone;
  UINT32  IcClrActivity;
  UINT32  IcClrStopDet;
  UINT32  IcClrStartDet;
  UINT32  IcClrGenCall;

  struct {
    UINT32  Enable:1;
    UINT32  Abort:1;
    UINT32  Reserved:30;
  } IcEnable;

  UINT32  IcStatus;
  UINT32  IcTxFlr;
  UINT32  IcRxFlr;
  UINT32  IcSdaHold;
  UINT32  IcTxAbrtSource;
  UINT32  Reserved0;
  UINT32  IcDmaCr;
  UINT32  IcDmaTdlr;
  UINT32  IcDmaRdlr;
  UINT32  IcSdaSetup;
  UINT32  IcAckGeneralCall;
  UINT32  IcEnableStatus;
  UINT32  IcFsSpkLen;
  UINT32  IcHsSpkLen;
  UINT32  Reserved1[19];
  UINT32  IcCompParam1;
  UINT32  IcCompVersion;
  UINT32  IcCompType;
} I2C_CONTROLLER_REGS;

STATIC
VOID
EFIAPI
I2cFlushRxFifo (
  IN  UINTN  Bus
  );

STATIC
EFI_STATUS
EFIAPI
I2cXferFinish (
  IN  UINTN  Bus
  );

STATIC
EFI_STATUS
EFIAPI
I2cWaitBusBusy (
  IN  UINTN  Bus
  );

STATIC
VOID
EFIAPI
I2cFlushRxFifo (
  IN  UINTN  Bus
  )
{
  volatile I2C_CONTROLLER_REGS *CONST  I2cRegs = BAIKAL_I2C_REGS (Bus);

  while (I2cRegs->IcStatus & IC_STATUS_RFNE) {
    I2cRegs->IcDataCmd;
  }
}

STATIC
EFI_STATUS
EFIAPI
I2cXferFinish (
  IN  UINTN  Bus
  )
{
  volatile I2C_CONTROLLER_REGS *CONST  I2cRegs = BAIKAL_I2C_REGS (Bus);
  UINTN Timeout = I2C_TIMEOUT;

  for (;;) {
    if (I2cRegs->IcRawIntrStat & IC_RAW_INTR_STAT_STOP_DET) {
      I2cRegs->IcClrStopDet;
      break;
    }

    if (!Timeout--) {
      return EFI_TIMEOUT;
    }
  }

  if (I2cWaitBusBusy (Bus)) {
    return EFI_TIMEOUT;
  }

  I2cFlushRxFifo (Bus);
  I2cRegs->IcEnable.Enable = 0;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
I2cWaitBusBusy (
  IN  UINTN  Bus
  )
{
  volatile I2C_CONTROLLER_REGS *CONST  I2cRegs = BAIKAL_I2C_REGS (Bus);
  UINTN Timeout = I2C_TIMEOUT;

  while (Timeout--) {
    UINTN  IcStatus = I2cRegs->IcStatus;

    if (!(IcStatus & IC_STATUS_MST_ACTIVITY) &&
         (IcStatus & IC_STATUS_TFE)) {
      return EFI_SUCCESS;
    }
  }

  return EFI_TIMEOUT;
}

EFI_STATUS
EFIAPI
I2cTxRx (
  IN   UINTN    Bus,
  IN   UINT16   TargetAddr,
  IN   UINT8   *TxBuf,
  IN   UINTN    TxSize,
  OUT  UINT8   *RxBuf,
  IN   UINTN    RxSize
  )
{
  volatile I2C_CONTROLLER_REGS *CONST  I2cRegs = BAIKAL_I2C_REGS (Bus);
  UINTN  Timeout;

  I2cRegs->IcEnable.Enable = 0;
  I2cRegs->IcCon           = IC_CON_IC_SLAVE_DISABLE | IC_CON_SPEED | IC_CON_MASTER_MODE;
  I2cRegs->IcTar           = TargetAddr;
  I2cRegs->IcRxTl          = 0;
  I2cRegs->IcTxTl          = 0;
  I2cRegs->IcIntrMask      = 0;
  I2cRegs->IcFsSclHcnt     = (IC_CLK * MIN_FS_SCL_HIGHTIME) / NANO_TO_MICRO;
  I2cRegs->IcFsSclLcnt     = (IC_CLK * MIN_FS_SCL_LOWTIME)  / NANO_TO_MICRO;
  I2cRegs->IcEnable.Enable = 1;

  Timeout = I2C_TIMEOUT;
  while ((I2cRegs->IcStatus & IC_STATUS_MST_ACTIVITY) ||
        !(I2cRegs->IcStatus & IC_STATUS_TFE)) {
    if (!Timeout--) {
      return EFI_TIMEOUT;
    }
  }

  Timeout = I2C_TIMEOUT;
  while (TxSize) {
    if (I2cRegs->IcStatus & IC_STATUS_TFNF) {
      I2cRegs->IcDataCmd = *TxBuf++;
      --TxSize;
    }

    if (!Timeout--) {
      I2cXferFinish (Bus);
      return EFI_TIMEOUT;
    }
  }

  Timeout = I2C_TIMEOUT;
  while (RxSize) {
    I2cRegs->IcDataCmd = IC_DATA_CMD_CMD;

    if (I2cRegs->IcStatus & IC_STATUS_RFNE) {
      *RxBuf++ = I2cRegs->IcDataCmd;
      --RxSize;
    }

    if (!Timeout--) {
      I2cXferFinish (Bus);
      return EFI_TIMEOUT;
    }
  }

  return I2cXferFinish (Bus);
}
