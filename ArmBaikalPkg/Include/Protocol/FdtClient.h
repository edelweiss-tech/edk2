/** @file

  DISCLAIMER: the FDT_CLIENT_PROTOCOL introduced here is a work in progress,
  and should not be used outside of the EDK II tree.

  Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __FDT_CLIENT_H__
#define __FDT_CLIENT_H__

#define FDT_CLIENT_PROTOCOL_GUID { \
  0xE11FACA0, 0x4710, 0x4C8E, {0xA7, 0xA2, 0x01, 0xBA, 0xA2, 0x59, 0x1B, 0x4C} \
  }

//
// Protocol interface structure
//
typedef struct _FDT_CLIENT_PROTOCOL FDT_CLIENT_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *FDT_CLIENT_GET_NODE_PROPERTY) (
  IN  FDT_CLIENT_PROTOCOL     *This,
  IN  INT32                   Node,
  IN  CONST CHAR8             *PropertyName,
  OUT CONST VOID              **Prop,
  OUT UINT32                  *PropSize OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *FDT_CLIENT_SET_NODE_PROPERTY) (
  IN  FDT_CLIENT_PROTOCOL     *This,
  IN  INT32                   Node,
  IN  CONST CHAR8             *PropertyName,
  IN  CONST VOID              *Prop,
  IN  UINT32                  PropSize
  );

typedef
EFI_STATUS
(EFIAPI *FDT_CLIENT_FIND_COMPATIBLE_NODE) (
  IN  FDT_CLIENT_PROTOCOL     *This,
  IN  CONST CHAR8             *CompatibleString,
  OUT INT32                   *Node
  );

typedef
EFI_STATUS
(EFIAPI *FDT_CLIENT_FIND_NEXT_COMPATIBLE_NODE) (
  IN  FDT_CLIENT_PROTOCOL     *This,
  IN  CONST CHAR8             *CompatibleString,
  IN  INT32                   PrevNode,
  OUT INT32                   *Node
  );

typedef
EFI_STATUS
(EFIAPI *FDT_CLIENT_FIND_NEXT_SUBNODE) (
  IN  FDT_CLIENT_PROTOCOL     *This,
  IN  CONST CHAR8             *SubnodeString,
  IN  INT32                   PrevSubnode,
  OUT INT32                   *Subnode
  );

typedef
EFI_STATUS
(EFIAPI *FDT_CLIENT_FIND_COMPATIBLE_NODE_PROPERTY) (
  IN  FDT_CLIENT_PROTOCOL     *This,
  IN  CONST CHAR8             *CompatibleString,
  IN  CONST CHAR8             *PropertyName,
  OUT CONST VOID              **Prop,
  OUT UINT32                  *PropSize OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *FDT_CLIENT_FIND_COMPATIBLE_NODE_REG) (
  IN  FDT_CLIENT_PROTOCOL     *This,
  IN  CONST CHAR8             *CompatibleString,
  OUT CONST VOID              **Reg,
  OUT UINTN                   *AddressCells,
  OUT UINTN                   *SizeCells,
  OUT UINT32                  *RegSize
  );

typedef
EFI_STATUS
(EFIAPI *FDT_CLIENT_FIND_NEXT_MEMORY_NODE_REG) (
  IN  FDT_CLIENT_PROTOCOL     *This,
  IN  INT32                   PrevNode,
  OUT INT32                   *Node,
  OUT CONST VOID              **Reg,
  OUT UINTN                   *AddressCells,
  OUT UINTN                   *SizeCells,
  OUT UINT32                  *RegSize
  );

typedef
EFI_STATUS
(EFIAPI *FDT_CLIENT_FIND_MEMORY_NODE_REG) (
  IN  FDT_CLIENT_PROTOCOL     *This,
  OUT INT32                   *Node,
  OUT CONST VOID              **Reg,
  OUT UINTN                   *AddressCells,
  OUT UINTN                   *SizeCells,
  OUT UINT32                  *RegSize
  );

typedef
EFI_STATUS
(EFIAPI *FDT_CLIENT_GET_OR_INSERT_CHOSEN_NODE) (
  IN  FDT_CLIENT_PROTOCOL     *This,
  OUT INT32                   *Node
  );

struct _FDT_CLIENT_PROTOCOL {
  FDT_CLIENT_GET_NODE_PROPERTY             GetNodeProperty;
  FDT_CLIENT_SET_NODE_PROPERTY             SetNodeProperty;

  FDT_CLIENT_FIND_COMPATIBLE_NODE          FindCompatibleNode;
  FDT_CLIENT_FIND_NEXT_COMPATIBLE_NODE     FindNextCompatibleNode;
  FDT_CLIENT_FIND_NEXT_SUBNODE             FindNextSubnode;
  FDT_CLIENT_FIND_COMPATIBLE_NODE_PROPERTY FindCompatibleNodeProperty;
  FDT_CLIENT_FIND_COMPATIBLE_NODE_REG      FindCompatibleNodeReg;

  FDT_CLIENT_FIND_MEMORY_NODE_REG          FindMemoryNodeReg;
  FDT_CLIENT_FIND_NEXT_MEMORY_NODE_REG     FindNextMemoryNodeReg;

  FDT_CLIENT_GET_OR_INSERT_CHOSEN_NODE     GetOrInsertChosenNode;
};

extern EFI_GUID gFdtClientProtocolGuid;

#endif
