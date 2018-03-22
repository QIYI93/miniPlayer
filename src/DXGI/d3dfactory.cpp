#include "d3dfactory.h"
#include <stdint.h>
#include "d3dadapter.h"

IDXGIFactory* D3DFactory::factory = nullptr;

void D3DFactory::CreateFactory()
{
	if (factory) return;
	if (CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory) != S_OK)
    {
		return;
	}
}

void D3DFactory::Release()
{
	factory->Release();
	factory = nullptr;
}

std::vector<D3DAdapter*> D3DFactory::GetAdapters()
{
	uint32_t index = 0;
	std::vector<D3DAdapter*> adapters;
	IDXGIAdapter* adapter = nullptr;
	while (factory->EnumAdapters(index++, &adapter) != DXGI_ERROR_NOT_FOUND)
    {
		adapters.push_back(new D3DAdapter(adapter));
	}

	if (index == 1)
    {
		printf("[D3DFactory] No adapter available");
	}

	return adapters;
}

D3DAdapter* D3DFactory::GetFirstAdapter() {

	IDXGIAdapter* adapter = nullptr;

	if (factory->EnumAdapters(0, &adapter) == DXGI_ERROR_NOT_FOUND)
    {
        printf("[D3DFactory] No adapter available");
		return nullptr;
	}

	return new D3DAdapter(adapter);
}
