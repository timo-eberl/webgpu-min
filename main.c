#include <stdio.h>
#include <string.h>
#include <webgpu/webgpu.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

int main_done = 0;

const char* shader_code = "\
@group(0) @binding(0) var<storage, read_write> data: array<u32>;\n\
@compute @workgroup_size(1)\n\
fn main(@builtin(global_invocation_id) id: vec3<u32>) {\n\
	data[id.x] = data[id.x] * data[id.x];\n\
}\n\
";

void on_buffer_mapped(WGPUMapAsyncStatus s, WGPUStringView m, void* u1, void* u2) {
	WGPUBuffer staging_buffer = (WGPUBuffer)u1;
	if (s == WGPUMapAsyncStatus_Success) {
		uint32_t* results = (uint32_t*)wgpuBufferGetConstMappedRange(staging_buffer, 0, 16);
		printf("Compute Result: %u, %u, %u, %u\n", results[0], results[1], results[2], results[3]);
		wgpuBufferUnmap(staging_buffer);
	}
	else { printf("Buffer mapping failed\n"); }
	main_done = 1;
}

void on_device(WGPURequestDeviceStatus s, WGPUDevice d, WGPUStringView m, void* u1, void* u2) {
	printf("Device Created: %s\n", s == WGPURequestDeviceStatus_Success ? "Success" : "Failed");
	if (s != WGPURequestDeviceStatus_Success) {
		main_done = 1;
		return;
	}

	uint32_t numbers[] = {1, 2, 3, 4};
	uint64_t size = sizeof(numbers);

	// Create Buffers
	WGPUBufferDescriptor buf_desc = {
		.usage = WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst,
		.size = size,
		.label = (WGPUStringView){"Storage Buffer", WGPU_STRLEN},
	};
	WGPUBuffer device_buffer = wgpuDeviceCreateBuffer(d, &buf_desc);

	WGPUBufferDescriptor stage_desc = {.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst,
									   .size = size,
									   .label = (WGPUStringView){"Staging Buffer", WGPU_STRLEN}};
	WGPUBuffer staging_buffer = wgpuDeviceCreateBuffer(d, &stage_desc);

	// Upload Data
	WGPUQueue queue = wgpuDeviceGetQueue(d);
	wgpuQueueWriteBuffer(queue, device_buffer, 0, numbers, size);

	// Create Pipeline
	WGPUShaderSourceWGSL wgsl_src = {.chain = {.sType = WGPUSType_ShaderSourceWGSL},
									 .code = (WGPUStringView){shader_code, strlen(shader_code)}};
	WGPUShaderModuleDescriptor sh_desc = {.nextInChain = &wgsl_src.chain};
	WGPUShaderModule shader = wgpuDeviceCreateShaderModule(d, &sh_desc);

	WGPUComputePipelineDescriptor pipe_desc = {
		.compute = {.module = shader, .entryPoint = (WGPUStringView){"main", 4}}};
	WGPUComputePipeline pipeline = wgpuDeviceCreateComputePipeline(d, &pipe_desc);

	// Create Bind Group
	WGPUBindGroupEntry entry = {.binding = 0, .buffer = device_buffer, .size = size};
	WGPUBindGroupDescriptor bg_desc = {.layout = wgpuComputePipelineGetBindGroupLayout(pipeline, 0),
									   .entryCount = 1,
									   .entries = &entry};
	WGPUBindGroup bind_group = wgpuDeviceCreateBindGroup(d, &bg_desc);

	// Encode and Submit Commands
	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(d, NULL);
	WGPUComputePassEncoder pass = wgpuCommandEncoderBeginComputePass(encoder, NULL);
	wgpuComputePassEncoderSetPipeline(pass, pipeline);
	wgpuComputePassEncoderSetBindGroup(pass, 0, bind_group, 0, NULL);
	wgpuComputePassEncoderDispatchWorkgroups(pass, 4, 1, 1);
	wgpuComputePassEncoderEnd(pass);
	wgpuCommandEncoderCopyBufferToBuffer(encoder, device_buffer, 0, staging_buffer, 0, size);

	WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, NULL);
	wgpuQueueSubmit(queue, 1, &commands);

	// Map buffer to read results
	WGPUBufferMapCallbackInfo map_cb = {.mode = WGPUCallbackMode_AllowSpontaneous,
										.callback = on_buffer_mapped,
										.userdata1 = staging_buffer};
	wgpuBufferMapAsync(staging_buffer, WGPUMapMode_Read, 0, size, map_cb);
}

void on_adapter(WGPURequestAdapterStatus s, WGPUAdapter a, WGPUStringView m, void* u1, void* u2) {
	if (s != WGPURequestAdapterStatus_Success) {
		printf("Adapter Request Failed");
		main_done = 1;
		return;
	}
	WGPURequestDeviceCallbackInfo cb = {.mode = WGPUCallbackMode_AllowSpontaneous,
										.callback = on_device};
	wgpuAdapterRequestDevice(a, NULL, cb);
}

int main(void) {
	WGPUInstance instance = wgpuCreateInstance(NULL);
	WGPURequestAdapterCallbackInfo cb = {.mode = WGPUCallbackMode_AllowSpontaneous,
										 .callback = on_adapter};
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
