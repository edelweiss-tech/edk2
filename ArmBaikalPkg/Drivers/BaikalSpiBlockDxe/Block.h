
#ifndef __BLOCK_H__
#define __BLOCK_H__

  #define BAIKAL_SPI_PRIVATE_DATA_SIGNATURE     SIGNATURE_32 ('B', 'S', 'P', 'I')
  #define BAIKAL_SPI_PRIVATE_FROM_BLKIO(a)      CR (a, BAIKAL_SPI_PRIVATE_DATA, BlockIo, BAIKAL_SPI_PRIVATE_DATA_SIGNATURE)
  #define SPI_DISK_BLOCK_SIZE 512
  #define UEFI_BLOCK_OFFSET  (8*1024*1024)
  #define UEFI_BLOCK_SIZE    (8*1024*1024)

  typedef struct {
      VENDOR_DEVICE_PATH                  Vendor;
      EFI_DEVICE_PATH_PROTOCOL            End;
  } BAIKAL_SPI_DEVICE_PATH;

  typedef struct _BAIKAL_SPI_PRIVATE_DATA {
      UINTN                       Signature;
      EFI_BLOCK_IO_PROTOCOL       BlockIo;
      EFI_BLOCK_IO_MEDIA          Media;
      BAIKAL_SPI_DEVICE_PATH      DevicePath;
      UINT64                      Size;
  } BAIKAL_SPI_PRIVATE_DATA;

#endif