#pragma once

#include <d3d11.h>
#include <stdint.h>
#include <dxgi.h>
#include <string>
#include <vector>

struct FD_D3DADAPTER_INFO {
	
};

class  D3DAdapter
{
private:
	friend class D3DOutput;
private:
	IDXGIAdapter* adapter;

	std::wstring name;

	uint32_t vendorId;
    uint32_t deviceId;
    uint32_t subSystemId;
    uint32_t revision;

	int vram;
    int sram;
    int shared;
public:
	D3DAdapter(IDXGIAdapter* adapter);
	~D3DAdapter();

	std::vector<D3DOutput*> GetOutputs() const;
	D3DOutput* GetFirstOutput() const;

	inline IDXGIAdapter* GetAdapter() const { return adapter; }

	inline const std::wstring& GetName() const { return name; }
	inline uint32_t GetVendorID() const { return vendorId; }
	inline uint32_t GetDeviceID() const { return deviceId; }
	inline uint32_t GetSubSystemID() const { return subSystemId; }
	inline uint32_t GetRevision() const { return revision; }

	inline uint32_t GetVideoMemory() const { return vram; }
	inline uint32_t GetSystemMemory() const { return sram; }
	inline uint32_t GetSharedSystemMemory() const { return shared; }
};
