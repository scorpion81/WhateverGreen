//
//  kern_igfx_gvtg.cpp
//  WhateverGreen
//
//  Created by scorpion81 on 11/01/2022.
//  Copyright © 2020 vit9696. All rights reserved.
//

#include "kern_igfx.hpp"

constexpr const char* log = "igfx_gvtg";
typedef void*(*fn_ptr)(void*);

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
	enabled = checkKernelArgument("-igfxgvtg");
	if (enabled) {
		SYSLOG("igfx", "GVT-g helper enabled, trying its best to make GVT-g working ;-)");
	}	
}

void IGFX::GVTGAwareMaker::processFramebufferKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {

	uint64_t gvtg_magic = callbackIGFX->readRegister32(callbackIGFX->defaultController(), VGT_PVINFO_PAGE);
	CPUInfo::CpuGeneration generation = BaseDeviceInfo::get().cpuGeneration;
	bool valid_cpu = generation >= CPUInfo::CpuGeneration::Skylake;
	available = gvtg_magic == kGVTgMagic && valid_cpu;
	if (available) {
		SYSLOG("igfx", "GVT-g is available, found GPU generation %d", generation);
	}
	else {
        SYSLOG("igfx", "GVT-g is NOT available. found GPU generation %d", generation);
	}

	//const OSSymbol* oss = OSSymbol::withCString("AppleIntelFramebufferController");
	//const OSMetaClass *mc = OSMetaClass::getMetaClassWithName(oss);

	//CFL only for now
	// char* symbol = '__ZN31AppleIntelFramebufferController5startEP9IOService';
	char* ctor = "__ZN31AppleIntelFramebufferControllerC1EPK11OSMetaClass";
	char* hasAccelerator = "__ZN21AppleIntelFramebuffer14HasAcceleratorEv";

	orgIntelFramebufferControllerCtor = (fn_ptr)patcher.solveSymbol(index, ctor);
	orgHasAccelerator = (fn_ptr)patcher.solveSymbol(index, hasAccelerator);

	//try it....
/*	SYSLOG("igfx", "Danger close.");
	void* instance = orgIntelFramebufferControllerCtor((void*)mc);
	void* value = orgHasAccelerator(instance);
	SYSLOG("igfx", "Framebuffer has Accelerator ? %d", value); */

	// route request must be set up before that function is called (i think), else it "misses" its
	// call point in time.
/*	KernelPatcher::RouteRequest routeRequest =  {
		ctor,
		wrapIntelFramebufferControllerCtor,
		orgIntelFramebufferControllerCtor,
	};

	//attach wrapper ? (is index just 1 ?!) 
	patcher.routeMultiple(index, &routeRequest, 1);*/


	// lets test if we can call here ourself that function....
	/*char* hasAccelerator = "__ZN21AppleIntelFramebuffer14HasAcceleratorEv";
	KernelPatcher::RouteRequest routeRequest =  {
		hasAccelerator,
		wrapHasAccelerator,
		orgHasAccelerator
	};

	patcher.routeMultiple(index, &routeRequest, 1);*/
	//orgHasAccelerator = (fn_ptr)patcher.solveSymbol(index, hasAccelerator);
}

void IGFX::GVTGAwareMaker::processGraphicsKext(KernelPatcher &patcher, size_t index, mach_vm_address_t address, size_t size) {
	
	// Address of `__ZN16IntelAcceleratorC1EPK11OSMetaClass` ->IntelAccelerator ctor
	char* symbol = "__ZN16IntelAcceleratorC1EPK11OSMetaClass";
	//mach_vm_address_t orgIntelAcceleratorCtor = patcher.solveSymbol(index, symbol);
	
	KernelPatcher::RouteRequest routeRequest2 =  {
		symbol,
		wrapIntelAcceleratorCtor,
		orgIntelAcceleratorCtor,
	};
	
	// i have a bad feeling about this :-D 
	patcher.routeMultiple(index, &routeRequest2, 1);
}

void* IGFX::GVTGAwareMaker::wrapIntelAcceleratorCtor(void* instance) {
	//callbackIGFX->readRegister32(controller, )
	//IntelAccelerator instance ?
	SYSLOG("igfx", "Routed Accelerator Constructor");
	//void *accelerator = callbackIGFX->modGVTGAwareMaker.orgIntelAcceleratorCtor(instance);
	return instance;
}

void* IGFX::GVTGAwareMaker::wrapIntelFramebufferControllerCtor(void* instance) {
	SYSLOG("igfx", "Routed FramebufferController Constructor");
	return instance;
}

void* IGFX::GVTGAwareMaker::wrapHasAccelerator(void* instance) {
	fn_ptr fn = callbackIGFX->modGVTGAwareMaker.orgHasAccelerator;
	//void *controller = callbackIGFX->modGVTGAwareMaker.orgIntelFramebufferControllerCtor(instance);
	uint64_t value = 2;
	if (fn) {
		value = (uint64_t)fn(instance);
		SYSLOG("igfx", "Framebuffer has Accelerator %d", value);
	}
	else {
		SYSLOG("igfx", "Address of hasAccelerator could not be resolved!");
	}

	return (void*)value;
}
