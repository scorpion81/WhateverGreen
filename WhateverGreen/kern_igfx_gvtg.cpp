//
//  kern_igfx_gvtg.cpp
//  WhateverGreen
//
//  Created by scorpion81 on 11/01/2022.
//  Copyright © 2020 vit9696. All rights reserved.
//

#include "kern_igfx.hpp"

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
	uint64_t gvtg_magic = callbackIGFX->readRegister32(callbackIGFX->defaultController(), VGT_PVINFO_PAGE);
	CPUInfo::CpuGeneration generation = BaseDeviceInfo::get().cpuGeneration;
	bool valid_cpu = generation >= CPUInfo::CpuGeneration::Skylake;
	available = checkKernelArgument("-igfxgvtg") && gvtg_magic == kGVTgMagic && valid_cpu;
	if (available) {
		DBGLOG("igfx", "GVT-g is available, found GPU generation %d", generation);
	}
	else {
        DBGLOG("igfx", "GVT-g is NOT available. found GPU generation %d", generation);
	}
}

void IGFX::GVTGAwareMaker::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {

/*	auto framebuffer = Value::of(callbackIGFX->getRealFramebuffer(index));
	if (framebuffer.isOneOf(&kextIntelKBLFb)) {
		DBGLOG("igfx", "GVTG: [KBL ] Will setup the fix for KBL platform.");
		callbackIGFX->modMMIORegistersWriteSupport.replacerList.add(&dKBLPWMFreq1);
		callbackIGFX->modMMIORegistersWriteSupport.replacerList.add(&dKBLPWMCtrl1);
	} else if (framebuffer.isOneOf(&kextIntelCFLFb, &kextIntelICLLPFb)) {
		DBGLOG("igfx", "BLR: [CFL+] Will setup the fix for CFL/ICL platform.");
		callbackIGFX->modMMIORegistersWriteSupport.replacerList.add(&dCFLPWMFreq1);
		callbackIGFX->modMMIORegistersWriteSupport.replacerList.add(&dCFLPWMDuty1);
	 else {
		SYSLOG("igfx", "GVTG: [ERR!] Found an unsupported platform. Will not perform any injections.");
	}*/	
}

void IGFX::GVTGAwareMaker::processGraphicsKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	
	// Address of `__ZN16IntelAcceleratorC1EPK11OSMetaClass` ->IntelAccelerator ctor
	char* symbol = "__ZN16IntelAcceleratorC1EPK11OSMetaClass";
	//mach_vm_address_t orgIntelAcceleratorCtor = patcher.solveSymbol(index, symbol);
	
	KernelPatcher::RouteRequest routeRequest =  {
		symbol,
		orgIntelAcceleratorCtor,
		wrapIntelAcceleratorCtor
	};
	
	patcher.routeMultiple(index, &routeRequest, 1);
}

void* IGFX::GVTGAwareMaker::wrapIntelAcceleratorCtor(void* instance) {
	//callbackIGFX->readRegister32(controller, )
	//IntelAccelerator instance ?
	SYSLOG("igfx", "Routed Accelerator Constructor");
	void *accelerator = callbackIGFX->modGVTGAwareMaker.orgIntelAcceleratorCtor(instance);
	return accelerator;
}
