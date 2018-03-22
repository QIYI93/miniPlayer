#include "d3doutput.h"
#include <cassert>

D3DOutput::D3DOutput(IDXGIOutput* output)
{
	assert(output != nullptr);

	DXGI_OUTPUT_DESC desc;

	output->GetDesc(&desc);

	this->output = output;
	
	name = std::wstring(desc.DeviceName);

	desktopCoordinates = desc.DesktopCoordinates;
	attachedToDesktop = desc.AttachedToDesktop == 1 ? true : false;
	rotation = (FD_MONITOR_ROTATION)desc.Rotation;
	monitorHandle = desc.Monitor;

	uint32_t numModes = 0;

	output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numModes, nullptr);

	if (!numModes) return;

	DXGI_MODE_DESC* descs = new DXGI_MODE_DESC[numModes];

	output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &numModes, descs);

	for (int i = 0; i < numModes; i++)
    {
		modes.push_back(descs[i]);
	}
}

D3DOutput::~D3DOutput()
{
	output->Release();
	output = nullptr;
}
