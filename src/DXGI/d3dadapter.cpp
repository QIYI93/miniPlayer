#include "d3dadapter.h"
#include <cassert>
#include "d3doutput.h"

D3DAdapter::D3DAdapter(IDXGIAdapter* adapter)
{
	assert(adapter != nullptr);

	DXGI_ADAPTER_DESC desc;

	adapter->GetDesc(&desc);

	this->adapter = adapter;

	name = std::wstring(desc.Description);

	vendorId = desc.VendorId;
	deviceId = desc.DeviceId;
	subSystemId = desc.SubSysId;
	revision = desc.Revision;

	vram = desc.DedicatedVideoMemory;
	sram = desc.DedicatedSystemMemory;
	shared = desc.SharedSystemMemory;
}

D3DAdapter::~D3DAdapter()
{
	adapter->Release();
}

std::vector<D3DOutput*> D3DAdapter::GetOutputs() const
{
    assert(adapter != nullptr);
	std::vector<D3DOutput*> outputs;

	uint32_t index = 0;
	IDXGIOutput* output;

	while (adapter->EnumOutputs(index++, &output) != DXGI_ERROR_NOT_FOUND)
    {
		outputs.push_back(new D3DOutput(output));
	}
	
	return outputs;
}

D3DOutput* D3DAdapter::GetFirstOutput() const
{
    assert(adapter != nullptr);

	IDXGIOutput* output = nullptr;

	if (adapter->EnumOutputs(0, &output) == DXGI_ERROR_NOT_FOUND)
    {
        wprintf(L"[D3DAdapter] No outputs connected to \"%s\"", name.c_str());
		return nullptr;
	}

	return new D3DOutput(output);
}
