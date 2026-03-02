#include <stdio.h>
#include <stdlib.h>
#include <webgpu/webgpu.h>

int main(void) {
	WGPUInstanceDescriptor desc = {0};
	WGPUInstance instance = wgpuCreateInstance(&desc);
	if (!instance) {
		printf("Failed to create WebGPU instance!\n");
		return 1;
	}
	printf("Successfully created WebGPU instance: %p\n", (void*)instance);
	wgpuInstanceRelease(instance);
	return 0;
}
