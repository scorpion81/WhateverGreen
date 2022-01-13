//
//  kern_igfx_gvtg.cpp
//  WhateverGreen
//
//  Created by scorpion81 on 11/01/2022.
//  Copyright © 2020 vit9696. All rights reserved.
//

#include <IOKit/IOService.h>
#include <Headers/kern_patcher.hpp>
#include <Headers/kern_devinfo.hpp>
#include <Headers/kern_cpu.hpp>
#include <Headers/kern_disasm.hpp>
#include <IOKit/IOLib.h>
#include <IOKit/IOMessage.h>
#include <mach/clock.h>

#include "kern_igfx.hpp"

namespace {
constexpr const char* log = "igfx_gvtg";

// MARK: - GVT-g Awareness Setup

void IGFX::GVTGAwareMaker::init() {
	// We need to patch both drivers
	requiresPatchingGraphics = true;
	requiresPatchingFramebuffer = true;
	
	// Requires access to global framebuffer controllers
	requiresGlobalFramebufferControllersAccess = true;
	
	// Requires read and write access to MMIO registers
	requiresMMIORegistersReadAccess = true;
	// requiresMMIORegistersWriteAccess = true;
}

void IGFX::GVTGAwareMaker::processKernel(KernelPatcher &patcher, DeviceInfo *info) {
	uint64_t gvtg = 0;
    uint32_t generation = 0;
	bool enabled = false;
	if (PE_parse_boot_argn("igfxgvtg", &gvtg, sizeof(gvtg)) ||
		generation = BaseDeviceInfo::get().cpuGeneration;
		gvtg = callbackIGFX->readRegister32(callbackIGFX->defaultController(), VGT_PVINFO_PAGE);
		enabled = (gvtg == kGVTgMagic) && (generation >= CPUInfo::CPUGeneration::Skylake);
		DBGLOG("igfx", "GVT-g enabled (%u), found GPU generation %d", enabled, generation);
	}
}

void IGFX::GVTGAwareMaker::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {

/*	auto framebuffer = Value::of(callbackIGFX->getRealFramebuffer(index));
	if (framebuffer.isOneOf(&kextIntelKBLFb)) {
		DBGLOG("igfx", "GVTG: [KBL ] Will setup the fix for KBL platform.");
		callbackIGFX->modMMIORegistersWriteSupport.replacerList.add(&dKBLPWMFreq1);
		callbackIGFX->modMMIORegistersWriteSupport.replacerList.add(&dKBLPWMCtrl1);
	/*} else if (framebuffer.isOneOf(&kextIntelCFLFb, &kextIntelICLLPFb)) {
		DBGLOG("igfx", "BLR: [CFL+] Will setup the fix for CFL/ICL platform.");
		callbackIGFX->modMMIORegistersWriteSupport.replacerList.add(&dCFLPWMFreq1);
		callbackIGFX->modMMIORegistersWriteSupport.replacerList.add(&dCFLPWMDuty1);
	}* else {
		SYSLOG("igfx", "GVTG: [ERR!] Found an unsupported platform. Will not perform any injections.");
	}*/	
}

void IGFX::GVTGAwareMaker::processGraphicsKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	
	// Address of `__ZN16IntelAcceleratorC1EPK11OSMetaClass` ->IntelAccelerator ctor
	mach_vm_address_t orgIntelAcceleratorCtor = patcher.solveSymbol(index, '__ZN16IntelAcceleratorC1EPK11OSMetaClass')
	if (addrIntelAcceleratorCtor > 0) {
		patcher.routeFunction(orgIntelAcceleratorCtor, wrapIntelAcceleratorCtor);
	}
}

void* IGFX::GVTGAwareMaker::wrapIntelAcceleratorCtor(void* instance) {
	//callbackIGFX->readRegister32(controller, )
	//IntelAccelerator instance ?
	SYSLOG("igfx", "Routed Accelerator Constructor")
	void *accelerator = callbackIGFX->modGVTGAwareMaker.orgIntelAcceleratorCtor(instance);
}
