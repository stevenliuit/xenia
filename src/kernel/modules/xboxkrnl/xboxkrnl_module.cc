/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#include "kernel/modules/xboxkrnl/xboxkrnl_module.h"

#include "kernel/modules/xboxkrnl/kernel_state.h"
#include "kernel/modules/xboxkrnl/xboxkrnl_hal.h"
#include "kernel/modules/xboxkrnl/xboxkrnl_memory.h"
#include "kernel/modules/xboxkrnl/xboxkrnl_rtl.h"
#include "kernel/modules/xboxkrnl/xboxkrnl_threading.h"

#include "kernel/modules/xboxkrnl/xboxkrnl_table.h"


using namespace xe;
using namespace xe::kernel;
using namespace xe::kernel::xboxkrnl;


namespace {

  // bit 27 = something?
  typedef struct {
    uint32_t    flags;
    uint8_t     processor_count;
    uint8_t     unknown0;
    uint8_t     unknown1;
    uint8_t     unknown2;
    uint32_t    unknown4;
    uint16_t    unknown5;
    uint16_t    unknown6;
  } XboxHardwareInfo_t;
  // XboxHardwareInfo_t XboxHardwareInfo = {
  //   0x00000000, 3, 0, 0, 0, 0, 0, 0,
  // };

  // This should remain zero for now.
  // The pointer returned should have a 4b value at 0x58 that is the first
  // argument to RtlImageXexHeaderField (header base?)
  //uint32_t XexExecutableModuleHandle = 0x00000000;

}


XboxkrnlModule::XboxkrnlModule(xe_pal_ref pal, xe_memory_ref memory,
                               shared_ptr<ExportResolver> resolver) :
    KernelModule(pal, memory, resolver) {
  resolver->RegisterTable(
      "xboxkrnl.exe", xboxkrnl_export_table, XECOUNT(xboxkrnl_export_table));

  // Setup the kernel state instance.
  // This is where all kernel objects are kept while running.
  kernel_state = auto_ptr<KernelState>(new KernelState(pal, memory, resolver));

  // Register all exported functions.
  RegisterHalExports(resolver.get(), kernel_state.get());
  RegisterMemoryExports(resolver.get(), kernel_state.get());
  RegisterRtlExports(resolver.get(), kernel_state.get());
  RegisterThreadingExports(resolver.get(), kernel_state.get());

  // TODO(benvanik): alloc heap memory somewhere in user space
  // TODO(benvanik): tools for reading/writing to heap memory

  uint8_t* mem = xe_memory_addr(memory, 0);

  // HACK: register some dummy globals for now.
  // KeDebugMonitorData
  resolver->SetVariableMapping(
      "xboxkrnl.exe", 0x00000059,
      0x40001000);
  // XboxHardwareInfo
  resolver->SetVariableMapping(
      "xboxkrnl.exe", 0x00000156,
      0x40002000);

  // XexExecutableModuleHandle
  // Games try to dereference this to get a pointer to some module struct.
  // So far it seems like it's just in loader code, and only used to look up
  // the XexHeaderBase for use by RtlImageXexHeaderField.
  // We fake it so that the address passed to that looks legit.
  // 0x80100FFC <- pointer to structure
  // 0x80101000 <- our module structure
  // 0x80101058 <- pointer to xex header
  // 0x80101100 <- xex header base
  resolver->SetVariableMapping(
      "xboxkrnl.exe", 0x00000193,
      0x80100FFC);
  XESETUINT32BE(mem + 0x80100FFC, 0x80101000);
  XESETUINT32BE(mem + 0x80101058, 0x80101100);
}

XboxkrnlModule::~XboxkrnlModule() {
}
