#include <stdio.h>
#include <webgpu/webgpu.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

int main_done = 0;

void on_device(WGPURequestDeviceStatus s, WGPUDevice d, WGPUStringView m, void* u1, void* u2) {
	printf("Device Created: %s\n", s == WGPURequestDeviceStatus_Success ? "Success" : "Failed");
	main_done = 1;
}

void on_adapter(WGPURequestAdapterStatus s, WGPUAdapter a, WGPUStringView m, void* u1, void* u2) {
	if (s != WGPURequestAdapterStatus_Success) return;
	WGPURequestDeviceCallbackInfo cb = {.mode = WGPUCallbackMode_AllowSpontaneous,
										.callback = on_device};
	wgpuAdapterRequestDevice(a, NULL, cb);
}

int main(void) {
	WGPUInstance instance = wgpuCreateInstance(NULL);
	WGPURequestAdapterCallbackInfo cb = {.mode = WGPUCallbackMode_AllowSpontaneous,
										 .callback = on_adapter};

	printf("Initializing WebGPU...\n");
	WGPURequestAdapterOptions options = {.powerPreference = WGPUPowerPreference_HighPerformance};
	wgpuInstanceRequestAdapter(instance, &options, cb);

#ifdef __EMSCRIPTEN__
	emscripten_exit_with_live_runtime(); // Return control to browser
#else
	while (!main_done)
		// Poll until done (wgpuInstanceWaitAny isn't implemented)
		wgpuInstanceProcessEvents(instance);
#endif
	return 0;
}
