/** @file
*
*  Copyright (C) 2015, Baikal Electronics. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __DW_UART_H__
#define __DW_UART_H__

#include <Uefi.h>
#include <Protocol/SerialIo.h>

/*
 * These are the definitions for the FIFO Control Register
 */
#define UART_FCR_FIFO_EN	0x01 /* Fifo enable */
#define UART_FCR_CLEAR_RCVR	0x02 /* Clear the RCVR FIFO */
#define UART_FCR_CLEAR_XMIT	0x04 /* Clear the XMIT FIFO */
#define UART_FCR_DMA_SELECT	0x08 /* For DMA applications */
#define UART_FCR_TRIGGER_MASK	0xC0 /* Mask for the FIFO trigger range */
#define UART_FCR_TRIGGER_1	0x00 /* Mask for trigger set at 1 */
#define UART_FCR_TRIGGER_4	0x40 /* Mask for trigger set at 4 */
#define UART_FCR_TRIGGER_8	0x80 /* Mask for trigger set at 8 */
#define UART_FCR_TRIGGER_14	0xC0 /* Mask for trigger set at 14 */

#define UART_FCR_RXSR		0x02 /* Receiver soft reset */
#define UART_FCR_TXSR		0x04 /* Transmitter soft reset */
#define UART_FIFO_LENGTH	16 /* FIFOs length */

/*
 * These are the definitions for the Modem Control Register
 */
#define UART_MCR_DTR	0x01		/* DTR   */
#define UART_MCR_RTS	0x02		/* RTS   */
#define UART_MCR_OUT1	0x04		/* Out 1 */
#define UART_MCR_OUT2	0x08		/* Out 2 */
#define UART_MCR_LOOP	0x10		/* Enable loopback test mode */
#define UART_MCR_AFCE   0x20		/* Auto Flow Control Enable */
#define UART_MCR_DMA_EN	0x04
#define UART_MCR_TX_DFR	0x08

/*
 * These are the definitions for the Line Control Register
 *
 * Note: if the word length is 5 bits (UART_LCR_WLEN5), then setting
 * UART_LCR_STOP will select 1.5 stop bits, not 2 stop bits.
 */
#define UART_LCR_WLS_MSK 0x03		/* character length select mask */
#define UART_LCR_WLS_5	0x00		/* 5 bit character length */
#define UART_LCR_WLS_6	0x01		/* 6 bit character length */
#define UART_LCR_WLS_7	0x02		/* 7 bit character length */
#define UART_LCR_WLS_8	0x03		/* 8 bit character length */
#define UART_LCR_STB	0x04		/* # stop Bits, off=1, on=1.5 or 2) */
#define UART_LCR_PEN	0x08		/* Parity eneble */
#define UART_LCR_EPS	0x10		/* Even Parity Select */
#define UART_LCR_STKP	0x20		/* Stick Parity */
#define UART_LCR_SBRK	0x40		/* Set Break */
#define UART_LCR_BKSE	0x80		/* Bank select enable */
#define UART_LCR_DLAB	0x80		/* Divisor latch access bit */

/*
 * These are the definitions for the Line Status Register
 */
#define UART_LSR_DR	0x01		/* Data ready */
#define UART_LSR_OE	0x02		/* Overrun */
#define UART_LSR_PE	0x04		/* Parity error */
#define UART_LSR_FE	0x08		/* Framing error */
#define UART_LSR_BI	0x10		/* Break */
#define UART_LSR_THRE	0x20		/* Xmit holding register empty */
#define UART_LSR_TEMT	0x40		/* Xmitter empty */
#define UART_LSR_ERR	0x80		/* Error */

#define UART_MSR_DCD	0x80		/* Data Carrier Detect */
#define UART_MSR_RI	0x40		/* Ring Indicator */
#define UART_MSR_DSR	0x20		/* Data Set Ready */
#define UART_MSR_CTS	0x10		/* Clear to Send */
#define UART_MSR_DDCD	0x08		/* Delta DCD */
#define UART_MSR_TERI	0x04		/* Trailing edge ring indicator */
#define UART_MSR_DDSR	0x02		/* Delta DSR */
#define UART_MSR_DCTS	0x01		/* Delta CTS */

#define UART_BUSY		0x40
#define UART_DATA_READY		0x01
#define UART_USR_BUSY		0x01

/*
 * These are the definitions for the Interrupt Identification Register
 */
#define UART_IIR_NO_INT	0x01	/* No interrupts pending */
#define UART_IIR_ID	0x0f	/* Mask for the interrupt ID */

#define UART_IIR_MSI	0x00	/* Modem status interrupt */
#define UART_IIR_THRI	0x02	/* Transmitter holding register empty */
#define UART_IIR_RDI	0x04	/* Receiver data interrupt */
#define UART_IIR_RLSI	0x06	/* Receiver line status interrupt */

/*
 * These are the definitions for the Interrupt Enable Register
 */
#define UART_IER_MSI	0x08	/* Enable Modem status interrupt */
#define UART_IER_RLSI	0x04	/* Enable receiver line status interrupt */
#define UART_IER_THRI	0x02	/* Enable Transmitter holding register int. */
#define UART_IER_RDI	0x01	/* Enable receiver data interrupt */

#define READ_UART_REG(r)       (*((volatile unsigned int *) (r)))
#define WRITE_UART_REG(r, v)   (*((volatile unsigned int *) (r)) = v)


/* 
 * TEST_UART_REG_ID macro returns 0 if the reg register is to equal to
 * its reg_val default value.
 */
#define TEST_UART_REG_ID TEST_MEM_REG_ID
#define TEST_UART_REG_VAL TEST_MEM_REG_ID

/* 
 * TEST_UART_READ macro returns 0 if the reg register can be read.
 */
#define TEST_UART_READ	TEST_READ_MEM_REG

/* Set the UART base address. */
#if 1
/* Pass the base address as the function argument. */
#define UART_BASE	UartBase
#else
#define UART_BASE	0x15770000
#endif /* 0/1 */

#define UPORT_OFF	0x00010000

#define UART_PORT(p)		(UART_BASE + (p)*UPORT_OFF)
#define UART_PORT_ADDR(p) 	((volatile llenv32_uart_t *) UART_PORT(p))

#define UART_RBR(p)	(UART_PORT(p) + 0x0) /* Receive Buffer Register */
#define UART_DLL(p)	(UART_PORT(p) + 0x0) /* Receive Buffer Register */
#define UART_THR(p)	(UART_PORT(p) + 0x0) /* Receive Buffer Register */
#define UART_DLH(p)	(UART_PORT(p) + 0x4) /* Divisor Latch High (DLH) Register. */
#define UART_IER(p)	(UART_PORT(p) + 0x4) /* Interrupt Enable Register */
#define UART_IIR(p)	(UART_PORT(p) + 0x8) /* Interrupt Identification Register */
#define UART_FCR(p)	(UART_PORT(p) + 0x8) /* FIFO Control Register. */
#define UART_LCR(p)	(UART_PORT(p) + 0xc) /* Line Control Register */
#define UART_MCR(p)	(UART_PORT(p) + 0x10) /* Modem Control Register */
#define UART_LSR(p)	(UART_PORT(p) + 0x14) /* Line Status Register */
#define UART_MSR(p)	(UART_PORT(p) + 0x18) /* Modem Status Register */
#define UART_SCR(p)	(UART_PORT(p) + 0x1c) /* Scratchpad Register */
#define UART_STHR0(p)	(UART_PORT(p) + 0x30) /* Shadow Transmit Holding Register */
#define UART_SRBR0(p)	(UART_PORT(p) + 0x30) /* Shadow Receive Buffer Register */
#define UART_SRBR1(p)	(UART_PORT(p) + 0x34) /* Shadow Receive Buffer Register 1 */
#define UART_STHR1(p)	(UART_PORT(p) + 0x34) /* Shadow Transmit Holding Register 1 */
#define UART_SRBR2(p)	(UART_PORT(p) + 0x38) /* Shadow Receive Buffer Register 2 */
#define UART_STHR2(p)	(UART_PORT(p) + 0x38) /* Shadow Transmit Holding Register 2 */
#define UART_STHR3(p)	(UART_PORT(p) + 0x3c) /* Shadow Transmit Holding Register 3 */
#define UART_SRBR3(p)	(UART_PORT(p) + 0x3c) /* Shadow Receive Buffer Register 3 */
#define UART_STHR4(p)	(UART_PORT(p) + 0x40) /* Shadow Transmit Holding Register 4 */
#define UART_SRBR4(p)	(UART_PORT(p) + 0x40) /* Shadow Receive Buffer Register 4 */
#define UART_SRBR5(p)	(UART_PORT(p) + 0x44) /* Shadow Receive Buffer Register 5 */
#define UART_STHR5(p)	(UART_PORT(p) + 0x44) /* Shadow Transmit Holding Register 5 */
#define UART_STHR6(p)	(UART_PORT(p) + 0x48) /* Shadow Transmit Holding Register 6 */
#define UART_SRBR6(p)	(UART_PORT(p) + 0x48) /* Shadow Receive Buffer Register 6 */
#define UART_SRBR7(p)	(UART_PORT(p) + 0x4c) /* Shadow Receive Buffer Register 7 */
#define UART_STHR7(p)	(UART_PORT(p) + 0x4c) /* Shadow Transmit Holding Register 7 */
#define UART_SRBR8(p)	(UART_PORT(p) + 0x50) /* Shadow Receive Buffer Register 8 */
#define UART_STHR8(p)	(UART_PORT(p) + 0x50) /* Shadow Transmit Holding Register 8 */
#define UART_STHR9(p)	(UART_PORT(p) + 0x54) /* Shadow Transmit Holding Register 9 */
#define UART_SRBR9(p)	(UART_PORT(p) + 0x54) /* Shadow Receive Buffer Register 9 */
#define UART_STHR10(p)	(UART_PORT(p) + 0x58) /* Shadow Transmit Holding Register 10 */
#define UART_SRBR10(p)	(UART_PORT(p) + 0x58) /* Shadow Receive Buffer Register 10 */
#define UART_STHR11(p)	(UART_PORT(p) + 0x5c) /* Shadow Transmit Holding Register 11 */
#define UART_SRBR11(p)	(UART_PORT(p) + 0x5c) /* Shadow Receive Buffer Register 11 */
#define UART_STHR12(p)	(UART_PORT(p) + 0x60) /* Shadow Transmit Holding Register 12 */
#define UART_SRBR12(p)	(UART_PORT(p) + 0x60) /* Shadow Receive Buffer Register 12 */
#define UART_SRBR13(p)	(UART_PORT(p) + 0x64) /* Shadow Receive Buffer Register 13 */
#define UART_STHR13(p)	(UART_PORT(p) + 0x64) /* Shadow Transmit Holding Register 13 */
#define UART_SRBR14(p)	(UART_PORT(p) + 0x68) /* Shadow Receive Buffer Register 14 */
#define UART_STHR14(p)	(UART_PORT(p) + 0x68) /* Shadow Transmit Holding Register 14 */
#define UART_SRBR15(p)	(UART_PORT(p) + 0x6c) /* Shadow Receive Buffer Register 15 */
#define UART_STHR15(p)	(UART_PORT(p) + 0x6c) /* Shadow Transmit Holding Register 15 */
#define UART_FAR(p)	(UART_PORT(p) + 0x70) /* FIFO Access Register */
#define UART_TFR(p)	(UART_PORT(p) + 0x74) /* Transmit FIFO Read */
#define UART_RFW(p)	(UART_PORT(p) + 0x78) /* Receive FIFO Write */
#define UART_USR(p)	(UART_PORT(p) + 0x7c) /* UART Status register. */
#define UART_TFL(p)	(UART_PORT(p) + 0x80) /* Transmit FIFO Level. */
#define UART_RFL(p)	(UART_PORT(p) + 0x84) /* Receive FIFO Level. */
#define UART_SRR(p)	(UART_PORT(p) + 0x88) /* Software Reset Register. */
#define UART_SRTS(p)	(UART_PORT(p) + 0x8c) /* Shadow Request to Send. */
#define UART_SBCR(p)	(UART_PORT(p) + 0x90) /* Shadow Break Control Register. */
#define UART_SDMAM(p)	(UART_PORT(p) + 0x94) /* Shadow DMA Mode. */
#define UART_SFE(p)	(UART_PORT(p) + 0x98) /* Shadow FIFO Enable */
#define UART_SRT(p)	(UART_PORT(p) + 0x9c) /* Shadow RCVR Trigger */
#define UART_STET(p)	(UART_PORT(p) + 0xa0) /* Shadow TX Empty Trigger */
#define UART_HTX(p)	(UART_PORT(p) + 0xa4) /* Halt TX */
#define UART_DMASA(p)	(UART_PORT(p) + 0xa8) /* DMA Software Acknowledge */
#define UART_CPR(p)	(UART_PORT(p) + 0xf4) /* Component Parameter Register */
#define UART_CPR_VAL	0x13f22
#define UART_UCV(p)	(UART_PORT(p) + 0xf8) /* Component Version */
#define UART_UCV_VAL	0x3331342a
#define UART_CTR(p)	(UART_PORT(p) + 0xfc) /* Component Type Register */
#define UART_CTR_VAL	0x44570110

/*

  Programmed hardware of Serial port.

  @return    Always return EFI_UNSUPPORTED.

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
  );

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
  );

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
  );

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
  IN  UINTN       UartBase,
  IN  UINT8       *Buffer,
  IN  UINTN       NumberOfBytes
  );

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
  IN  UINTN       UartBase,
  OUT UINT8       *Buffer,
  IN  UINTN       NumberOfBytes
  );

/**
  Check to see if any data is available to be read from the debug device.

  @retval EFI_SUCCESS       At least one byte of data is available to be read
  @retval EFI_NOT_READY     No data is available to be read
  @retval EFI_DEVICE_ERROR  The serial device is not functioning properly

**/
BOOLEAN
EFIAPI
DWUartPoll (
  IN  UINTN       UartBase
  );

#endif /* __DW_UART_H__ */
