/** @file
  Serial I/O Port library functions with no library constructor/destructor

  Copyright (C) 2015, Baikal Electronics. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>

#include <Drivers/DWUart.h>

//
// EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE is the only
// control bit that is not supported.
//
STATIC CONST UINT32 mInvalidControlBits = EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE;

/*

  Initialise the serial port to the specified settings.
  All unspecified settings will be set to the default values.

  @return    Always return EFI_SUCCESS or EFI_INVALID_PARAMETER.

**/
RETURN_STATUS
EFIAPI
DWUartInitializePort (
  IN OUT UINTN               UartBase,
  IN OUT UINT64              *BaudRate,
  IN OUT UINT32              *ReceiveFifoDepth,
  IN OUT EFI_PARITY_TYPE     *Parity,
  IN OUT UINT8               *DataBits,
  IN OUT EFI_STOP_BITS_TYPE  *StopBits
  )
{
  UINT32      LineControl = 0;

  /* Enable RX/TX FIFOs. */
  if ((*ReceiveFifoDepth == 0) || (*ReceiveFifoDepth >= UART_FIFO_LENGTH)) {
    WRITE_UART_REG(UART_FCR(0), (UART_FCR_FIFO_EN | UART_FCR_RXSR | UART_FCR_TXSR));
    *ReceiveFifoDepth = UART_FIFO_LENGTH;
  } else {
    ASSERT (*ReceiveFifoDepth < UART_FIFO_LENGTH);
    // Nothing else to do. 1 byte fifo is default.
    *ReceiveFifoDepth = 1;
  }

  //
  // Parity
  //
  switch (*Parity) {
  case DefaultParity:
    *Parity = NoParity;
  case NoParity:
    // Nothing to do. Parity is disabled by default.
    break;
  case EvenParity:
    LineControl |= (UART_LCR_PEN | UART_LCR_EPS);
    break;
  case OddParity:
    LineControl |= UART_LCR_PEN;
    break;
  case MarkParity:
    LineControl |= (UART_LCR_PEN | UART_LCR_STKP | UART_LCR_EPS);
    break;
  case SpaceParity:
    LineControl |= (UART_LCR_PEN | UART_LCR_STKP);
    break;
  default:
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Data Bits
  //
  switch (*DataBits) {
  case 0:
  case 8:
    *DataBits = 8;
    LineControl |= UART_LCR_WLS_8;
    break;
  case 7:
    LineControl |= UART_LCR_WLS_7;
    break;
  case 6:
    LineControl |= UART_LCR_WLS_6;
    break;
  case 5:
    LineControl |= UART_LCR_WLS_5;
    break;
  default:
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Stop Bits
  //
  switch (*StopBits) {
  case DefaultStopBits:
    *StopBits = OneStopBit;
  case OneStopBit:
    // Nothing to do. One stop bit is enabled by default.
    break;
  case TwoStopBits:
    LineControl |= UART_LCR_STB;
    break;
  case OneFiveStopBits:
    // Only 1 or 2 stops bits are supported
  default:
    return RETURN_INVALID_PARAMETER;
  }


  /* DLAB is writeable only when UART is not busy (USR[0] is equal to 0). */
  while ((READ_UART_REG(UART_USR(0)) & UART_USR_BUSY));

  WRITE_UART_REG(UART_LCR(0), 0x80);  /* DLAB -> 1 */

  /* TBD: Specify baudrate. */
  WRITE_UART_REG(UART_DLL(0), 0x4);  /* divisior 4 115200 divisor = 81, 9600, PMU doc v0.43., 12.5 MHz.*/

  WRITE_UART_REG(UART_LCR(0), LineControl);  /* DLAB -> 0, 8 data bits */

  return RETURN_SUCCESS;
}

/**

  Assert or deassert the control signals on a serial port.
  The following control signals are set according their bit settings :
  . Request to Send
  . Data Terminal Ready

  @param[in]  UartBase  UART registers base address
  @param[in]  Control   The following bits are taken into account :
                        . EFI_SERIAL_REQUEST_TO_SEND : assert/deassert the
                          "Request To Send" control signal if this bit is
                          equal to one/zero.
                        . EFI_SERIAL_DATA_TERMINAL_READY : assert/deassert
                          the "Data Terminal Ready" control signal if this
                          bit is equal to one/zero.
                        . EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE : enable/disable
                          the hardware loopback if this bit is equal to
                          one/zero.
                        . EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE : not supported.
                        . EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE : enable/
                          disable the hardware flow control based on CTS (Clear
                          To Send) and RTS (Ready To Send) control signals.

  @retval  RETURN_SUCCESS      The new control bits were set on the serial device.
  @retval  RETURN_UNSUPPORTED  The serial device does not support this operation.

**/
RETURN_STATUS
EFIAPI
DWUartSetControl (
    IN UINTN   UartBase,
    IN UINT32  Control
  )
{
  UINT32  MCR_Register;

  if (Control & (mInvalidControlBits)) {
    return RETURN_UNSUPPORTED;
  }

  MCR_Register = READ_UART_REG(UART_MCR(0));

  if (Control & EFI_SERIAL_REQUEST_TO_SEND) {
    MCR_Register |= UART_MCR_RTS;
  } else {
    MCR_Register &= ~UART_MCR_RTS;
  }

  if (Control & EFI_SERIAL_DATA_TERMINAL_READY) {
    MCR_Register |= UART_MCR_DTR;
  } else {
    MCR_Register &= ~UART_MCR_DTR;
  }

  if (Control & EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE) {
    MCR_Register |= UART_MCR_LOOP;
  } else {
    MCR_Register &= ~UART_MCR_LOOP;
  }

  if (Control & EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE) {
    MCR_Register |= UART_MCR_AFCE;
  } else {
    MCR_Register &= ~UART_MCR_AFCE;
  }

  WRITE_UART_REG(UART_MCR(0), MCR_Register);

  return RETURN_SUCCESS;
}

/**

  Retrieve the status of the control bits on a serial device.

  @param[in]   UartBase  UART registers base address
  @param[out]  Control   Status of the control bits on a serial device :

                         . EFI_SERIAL_DATA_CLEAR_TO_SEND, EFI_SERIAL_DATA_SET_READY,
                           EFI_SERIAL_RING_INDICATE, EFI_SERIAL_CARRIER_DETECT,
                           EFI_SERIAL_REQUEST_TO_SEND, EFI_SERIAL_DATA_TERMINAL_READY
                           are all related to the DTE (Data Terminal Equipment) and
                           DCE (Data Communication Equipment) modes of operation of
                           the serial device.
                         . EFI_SERIAL_INPUT_BUFFER_EMPTY : equal to one if the receive
                           buffer is empty, 0 otherwise.
                         . EFI_SERIAL_OUTPUT_BUFFER_EMPTY : equal to one if the transmit
                           buffer is empty, 0 otherwise.
                         . EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE : equal to one if the
                           hardware loopback is enabled (the ouput feeds the receive
                           buffer), 0 otherwise.
                         . EFI_SERIAL_SOFTWARE_LOOPBACK_ENABLE : equal to one if a
                           loopback is accomplished by software, 0 otherwise.
                         . EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE : equal to one if the
                           hardware flow control based on CTS (Clear To Send) and RTS
                           (Ready To Send) control signals is enabled, 0 otherwise.

  @retval RETURN_SUCCESS  The control bits were read from the serial device.

**/
RETURN_STATUS
EFIAPI
DWUartGetControl (
    IN UINTN     UartBase,
    OUT UINT32  *Control
  )
{
  UINT32      MSR_Register;
  UINT32      MCR_Register;
  UINT32      LSR_Register;

  MSR_Register = READ_UART_REG(UART_MSR(0));
  MCR_Register = READ_UART_REG(UART_MCR(0));
  LSR_Register = READ_UART_REG(UART_LSR(0));

  *Control = 0;

  if ((MSR_Register & UART_MSR_CTS) == UART_MSR_CTS) {
    *Control |= EFI_SERIAL_CLEAR_TO_SEND;
  }

  if ((MSR_Register & UART_MSR_DSR) == UART_MSR_DSR) {
    *Control |= EFI_SERIAL_DATA_SET_READY;
  }

  if ((MSR_Register & UART_MSR_RI) == UART_MSR_RI) {
    *Control |= EFI_SERIAL_RING_INDICATE;
  }

  if ((MSR_Register & UART_MSR_DCD) == UART_MSR_DCD) {
    *Control |= EFI_SERIAL_CARRIER_DETECT;
  }

  if ((MCR_Register & UART_MCR_RTS) == UART_MCR_RTS) {
    *Control |= EFI_SERIAL_REQUEST_TO_SEND;
  }

  if ((MCR_Register & UART_MCR_DTR) == UART_MCR_DTR) {
    *Control |= EFI_SERIAL_DATA_TERMINAL_READY;
  }

  if ((LSR_Register & UART_LSR_DR) == 0) {
    *Control |= EFI_SERIAL_INPUT_BUFFER_EMPTY;
  }

  if ((LSR_Register & UART_LSR_THRE) == UART_LSR_THRE) {
    *Control |= EFI_SERIAL_OUTPUT_BUFFER_EMPTY;
  }

  if ((MCR_Register & UART_MCR_AFCE) == UART_MCR_AFCE) {
    *Control |= EFI_SERIAL_HARDWARE_FLOW_CONTROL_ENABLE;
  }

  if ((MCR_Register & UART_MCR_LOOP) == UART_MCR_LOOP) {
    *Control |= EFI_SERIAL_HARDWARE_LOOPBACK_ENABLE;
  }

  return RETURN_SUCCESS;
}

/**
  Write data to serial device.

  @param  Buffer           Point of data buffer which need to be written.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Write data failed.
  @retval !0               Actual number of bytes written to serial device.

**/
UINTN
EFIAPI
DWUartWrite (
  IN  UINTN    UartBase,
  IN UINT8     *Buffer,
  IN UINTN     NumberOfBytes
  )
{
  UINT8* CONST Final = &Buffer[NumberOfBytes];

  while (Buffer < Final) {

    // Wait until UART able to accept another char
    while (!(READ_UART_REG(UART_LSR(0)) & UART_BUSY)) ;
      WRITE_UART_REG(UART_THR(0), *Buffer++);
  }

  return NumberOfBytes;
}

/**
  Read data from serial device and save the data in buffer.

  @param  Buffer           Point of data buffer which need to be written.
  @param  NumberOfBytes    Number of output bytes which are cached in Buffer.

  @retval 0                Read data failed.
  @retval !0               Actual number of bytes read from serial device.

**/
UINTN
EFIAPI
DWUartRead (
  IN  UINTN     UartBase,
  OUT UINT8     *Buffer,
  IN  UINTN     NumberOfBytes
  )
{
  UINTN   Count;

  for (Count = 0; Count < NumberOfBytes; Count++, Buffer++) {
    while ((READ_UART_REG(UART_LSR(0)) & UART_DATA_READY) == 0) ;
    *Buffer = (READ_UART_REG(UART_RBR(0)) & 0xFF);
  }

  return NumberOfBytes;
}

/**
  Check to see if any data is available to be read from the debug device.

  @retval EFI_SUCCESS       At least one byte of data is available to be read
  @retval EFI_NOT_READY     No data is available to be read
  @retval EFI_DEVICE_ERROR  The serial device is not functioning properly

**/
BOOLEAN
EFIAPI
DWUartPoll (
  IN  UINTN     UartBase
  )
{
  return ((READ_UART_REG(UART_LSR(0)) & UART_DATA_READY) != 0);
}
