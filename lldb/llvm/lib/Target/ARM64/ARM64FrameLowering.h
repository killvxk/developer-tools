//===-- ARM64FrameLowering.h - TargetFrameLowering for ARM64 ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//
//
//===----------------------------------------------------------------------===//

#ifndef ARM64_FRAMELOWERING_H
#define ARM64_FRAMELOWERING_H

#include "llvm/Target/TargetFrameLowering.h"

namespace llvm {
  class ARM64Subtarget;

class ARM64FrameLowering : public TargetFrameLowering {
protected:
  const ARM64Subtarget &STI;

public:
  explicit ARM64FrameLowering(const ARM64Subtarget &sti)
    : TargetFrameLowering(StackGrowsDown, 16, -16, 16,
                          false/*StackRealignable*/), STI(sti) {
  }

  void emitCalleeSavedFrameMoves(MachineFunction &MF, MCSymbol *Label,
                                 unsigned FramePtr) const;

  /// emitProlog/emitEpilog - These methods insert prolog and epilog code into
  /// the function.
  void emitPrologue(MachineFunction &MF) const;
  void emitEpilogue(MachineFunction &MF, MachineBasicBlock &MBB) const;

  int getFrameIndexOffset(const MachineFunction &MF, int FI) const;
  int getFrameIndexReference(const MachineFunction &MF, int FI,
                             unsigned &FrameReg) const;
  int resolveFrameIndexReference(const MachineFunction &MF, int FI,
                                 unsigned &FrameReg) const;
  bool spillCalleeSavedRegisters(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MI,
                                 const std::vector<CalleeSavedInfo> &CSI,
                                 const TargetRegisterInfo *TRI) const;

  bool restoreCalleeSavedRegisters(MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator MI,
                                   const std::vector<CalleeSavedInfo> &CSI,
                                   const TargetRegisterInfo *TRI) const;

  /// \brief Can this function use the red zone for local allocations.
  bool canUseRedZone(const MachineFunction &MF) const;

  bool hasFP(const MachineFunction &MF) const;
  bool hasReservedCallFrame(const MachineFunction &MF) const;

  void processFunctionBeforeCalleeSavedScan(MachineFunction &MF,
                                            RegScavenger *RS) const;

  uint32_t getCompactUnwindEncoding(MachineFunction &MF) const;
};

} // End llvm namespace

#endif
