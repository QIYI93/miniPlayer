#pragma once

#include <D3D11.h>
#include <vector>
#include <dxgi.h>

#pragma comment(lib,"DXGI.lib")

class  D3DFactory
{
private:
	friend class D3DAdapter;
private:
	static IDXGIFactory* factory;

public:

	static void CreateFactory();
	static void Release();
	
	static std::vector<D3DAdapter*> GetAdapters();
	static D3DAdapter* GetFirstAdapter();

	inline static IDXGIFactory* GetFactory() { return factory; }
};

class test
{
public:
    static void haha() {};
    test() = delete;
    ~test() = delete;
};
