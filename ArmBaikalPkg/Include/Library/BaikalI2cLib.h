// Copyright (c) 2020 Baikal Electronics JSC
// Author: Mikhail Ivanov <michail.ivanov@baikalelectronics.ru>

#ifndef BAIKAL_I2C_LIB_H_
#define BAIKAL_I2C_LIB_H_

EFI_STATUS
EFIAPI
I2cTxRx (
  IN   UINTN    Bus,
  IN   UINT16   TargetAddr,
  IN   UINT8   *TxBuf,
  IN   UINTN    TxSize,
  OUT  UINT8   *RxBuf,
  IN   UINTN    RxSize
  );

#endif // BAIKAL_I2C_LIB_H_
