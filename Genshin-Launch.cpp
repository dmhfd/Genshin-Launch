#define INITGUID
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <AclApi.h>
#include <WtsApi32.h>
#include <WinUser.h>
#include <winternl.h>
#include <d2d1.h>
#include <wincodec.h>
#include <PropIdl.h>
#include <math.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audioclient.h>
#include <sddl.h>
#include <tlhelp32.h>
#include "resource.h"
#include "Genshin-Launch.h"
using namespace D2D1;
HWND hwnd1;
ID2D1Factory *pFactory;
int SetVolum(double level)
{
	CoInitialize(NULL);
	HRESULT hr;
	IMMDeviceEnumerator *deviceEnumerator = NULL;
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
	if (!SUCCEEDED(hr)) {
		CoUninitialize();
		return 0;
	}
	IMMDevice *defaultDevice = NULL;
	hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
	if (!SUCCEEDED(hr)) {
		CoUninitialize();
		return 0;
	}
	deviceEnumerator->Release();
	deviceEnumerator = NULL;
	IAudioEndpointVolume *endpointVolume = NULL;
	hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume);
	if (!SUCCEEDED(hr)) {
		CoUninitialize();
		return 0;
	}
	defaultDevice->Release();
	defaultDevice = NULL;
	double newVolume = level;
	float currentVolume = 0;
	endpointVolume->GetMasterVolumeLevel(&currentVolume);
	hr = endpointVolume->GetMasterVolumeLevelScalar(&currentVolume);
	if (!SUCCEEDED(hr)) {
		CoUninitialize();
		return 0;
	}
	// hr = endpointVolume->SetMasterVolumeLevel((float)newVolume, NULL);
	hr = endpointVolume->SetMasterVolumeLevelScalar((float)newVolume, NULL);
	endpointVolume->SetMute(FALSE, NULL);
	endpointVolume->Release();
	CoUninitialize();
	return 1;
}

BOOL RegisterGIF(HWND hwnd, GIF *gif, unsigned int Delay, unsigned int timeID, unsigned int ResourceID, Group *group)
{
	HRESULT hr;
	IWICBitmapFrameDecode *pSource;
	IWICFormatConverter *pConverter;
	UINT xtmp, ytmp;
	int random = GetTickCount64() % 65536;
	PropVariantInit(&gif->propValue);
	GetClientRect(hwnd, &gif->rc);
	gif->size = SizeU(gif->rc.right, gif->rc.bottom);
	hr = pFactory->CreateHwndRenderTarget(RenderTargetProperties(), HwndRenderTargetProperties(hwnd, gif->size), &gif->pRenderTarget);
	gif->pRenderTarget->SetDpi(96.f, 96.f);
	if (!SUCCEEDED(hr))
		return 0;
	hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (!SUCCEEDED(hr))
		return 0;
	hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&gif->factory));
	if (!SUCCEEDED(hr))
		return 0;
	gif->imageResHandle = FindResourceA(NULL, MAKEINTRESOURCEA(ResourceID), "GIF");
	gif->imageFileSize = SizeofResource(NULL, gif->imageResHandle);
	gif->imageResDataHandle = LoadResource(NULL, gif->imageResHandle);
	gif->pImageFile = LockResource(gif->imageResDataHandle);
	hr = gif->factory->CreateStream(&gif->pStream);
	if (!SUCCEEDED(hr))
		return 0;
	hr = gif->pStream->InitializeFromMemory(reinterpret_cast<BYTE *>(gif->pImageFile), gif->imageFileSize);
	if (!SUCCEEDED(hr))
		return 0;
	hr = gif->factory->CreateDecoderFromStream(gif->pStream, NULL, WICDecodeMetadataCacheOnLoad, &gif->pDecoder);
	if (!SUCCEEDED(hr))
		return 0;
	hr = gif->pDecoder->GetFrameCount(&gif->m_cFrames);
	if (!SUCCEEDED(hr))
		return 0;
	for (UINT i = 0; i < gif->m_cFrames; i++)
	{
		hr = gif->pDecoder->GetFrame(gif->counter, &pSource);
		if (!SUCCEEDED(hr))
			return 0;
		hr = pSource->GetSize(&xtmp, &ytmp);
		if (!SUCCEEDED(hr))
			return 0;
		gif->length = MAX(gif->length, xtmp);
		gif->height = MAX(gif->height, ytmp);
		pSource->Release();
	}
	hr = gif->pDecoder->GetFrame(gif->counter, &pSource);
	if (!SUCCEEDED(hr))
		return 0;
	hr = gif->factory->CreateFormatConverter(&pConverter);
	if (!SUCCEEDED(hr))
		return 0;
	hr = pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeCustom);
	if (!SUCCEEDED(hr))
		return 0;
	hr = gif->pRenderTarget->CreateBitmapFromWicBitmap(pConverter, &gif->pBitmap);
	if (!SUCCEEDED(hr))
		return 0;
	int wide = GetSystemMetrics(SM_CXSCREEN), high = GetSystemMetrics(SM_CYSCREEN);
	if (timeID == 1)
	{
		group->one.width = gif->height;
		group->one.length = gif->length;
		SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
	}
	SetTimer(hwnd, timeID, Delay, 0);
	return 1;
}
BOOL ReSize(HWND hwnd, GIF *gif)
{
	HRESULT hr;
	GetClientRect(hwnd, &gif->rc);
	gif->size = SizeU(gif->rc.right, gif->rc.bottom);
	hr = gif->pRenderTarget->Resize(&gif->size);
	if (!SUCCEEDED(hr))
		return 0;
	return 1;
}
BOOL ResetBITMAP(HWND hwnd, GIF *gif)
{
	HRESULT hr;
	IWICBitmapFrameDecode *pSource;
	IWICMetadataQueryReader *pFrameMetadataQueryReader=0;
	IWICFormatConverter *pConverter=0;
	gif->pBitmap->Release();
	hr = gif->pDecoder->GetFrame(gif->counter, &pSource);
	if (!SUCCEEDED(hr))
	{
		goto err;
	}
	hr = gif->factory->CreateFormatConverter(&pConverter);
	if (!SUCCEEDED(hr))
	{
		goto err;
	}
	if (gif->Transform)
	{
		IWICBitmapFlipRotator *pIFlipRotator = 0;
		gif->factory->CreateBitmapFlipRotator(&pIFlipRotator);
		pIFlipRotator->Initialize(pSource, WICBitmapTransformFlipHorizontal);
		pConverter->Initialize(pIFlipRotator, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeCustom);
		pIFlipRotator->Release();
	}
	else
		pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 0.f, WICBitmapPaletteTypeCustom);
	hr = pSource->GetMetadataQueryReader(&pFrameMetadataQueryReader);
	if (!SUCCEEDED(hr))
	{
		goto err;
	}
	hr = pFrameMetadataQueryReader->GetMetadataByName(L"/imgdesc/Left", &gif->propValue);
	if (!SUCCEEDED(hr))
	{
		goto err;
	}
	gif->currentx = gif->propValue.uiVal;
	PropVariantInit(&gif->propValue);
	hr = pFrameMetadataQueryReader->GetMetadataByName(L"/imgdesc/Top", &gif->propValue);
	if (!SUCCEEDED(hr))
	{
		goto err;
	}
	gif->currenty = gif->propValue.uiVal;
	PropVariantInit(&gif->propValue);
	hr = gif->pRenderTarget->CreateBitmapFromWicBitmap(pConverter, &gif->pBitmap);
	if (!SUCCEEDED(hr))
	{
		goto err;
	}
	gif->size2 = gif->pBitmap->GetSize();
	InvalidateRect(hwnd, 0, 0);
	gif->counter = (gif->counter + 1) % gif->m_cFrames;
	if (gif->counter == 0){
		ShowWindow(hwnd1, SW_HIDE);
		PlaySoundW(0, NULL, 0);
	}
err:
	pSource->Release();
	pConverter->Release();
	pFrameMetadataQueryReader->Release();
	return 1;
}
void PaintBitmap(HWND hwnd, GIF *gif, unsigned Param)
{
	HRESULT hr = 0;
	PAINTSTRUCT ps;
	BeginPaint(hwnd, &ps);
	int wide = GetSystemMetrics(SM_CXVIRTUALSCREEN), high = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	gif->pRenderTarget->BeginDraw();
	gif->pRenderTarget->Clear(0);
	gif->pRenderTarget->DrawBitmap(gif->pBitmap, RectF(gif->currentx, gif->currenty, wide, high));
	gif->pRenderTarget->EndDraw();
	EndPaint(hwnd, &ps);
}
LRESULT CALLBACK WindowProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		return 0;
	case WM_CREATE:
	{
		MakePtr *Ptr = (MakePtr *)LPCREATESTRUCTW(lParam)->lpCreateParams;
		Group *group = (Group *)Ptr->b;
		createparam *temppara = (createparam *)Ptr->a;
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)Ptr);
		if (!RegisterGIF(hwnd, temppara->gif, temppara->delay, temppara->timeID, temppara->ResID, group))
			return -1;
		return 0;
	}
	case WM_TIMER:
	{
		MakePtr *Ptr = (MakePtr *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
		Group *group = (Group *)Ptr->b;
		createparam *temppara = (createparam *)Ptr->a;
		if (!ResetBITMAP(hwnd, temppara->gif))
			return -1;
		POINT cursor;
		GetCursorPos(&cursor);
		int random = GetTickCount() % 65536;
		int wide = GetSystemMetrics(SM_CXVIRTUALSCREEN), high = GetSystemMetrics(SM_CYVIRTUALSCREEN);
		if (wParam == 1)
		{
			SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, wide, high, 0);
		}
		return 0;
	}
	case WM_SIZE:
	{
		MakePtr *Ptr = (MakePtr *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
		createparam *temppara = (createparam *)Ptr->a;
		ReSize(hwnd, temppara->gif);
		return 0;
	}
	case WM_PAINT:
	{
		MakePtr *Ptr = (MakePtr *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
		createparam *temppara = (createparam *)Ptr->a;
		PaintBitmap(hwnd, temppara->gif, temppara->timeID);
		return 0;
	}
	case WM_DESTROY:
		// PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}
}
BOOL MyRegisterClass(void *WndProcCallback, WNDCLASSEXW *wc, const wchar_t *ClassName)
{
	wc->lpfnWndProc = (WNDPROC)WndProcCallback;
	wc->hInstance = GetModuleHandleW(0);
	wc->cbSize = sizeof(WNDCLASSEXW);
	wc->lpszClassName = ClassName;
	return RegisterClassExW(wc);
}

DWORD GetProcessIdByName(const wchar_t* name)
{
	HANDLE snapshot;
	DWORD pid = 0;
	if ((snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)) != INVALID_HANDLE_VALUE)
	{
		PROCESSENTRY32W pe;
		pe.dwSize = sizeof(PROCESSENTRY32W);
		if (Process32FirstW(snapshot, &pe))
			do
				if (_wcsicmp(pe.szExeFile, name) == 0)
				{
					pid = pe.th32ProcessID;
					break;
				}
		while (Process32NextW(snapshot, &pe));
		CloseHandle(snapshot);
	}
	return pid;
}

int WINAPI scanScreen(LPVOID a)
{
	while (true)
	{
		Sleep(300);
		if (GetProcessIdByName(L"yuanshen.exe")) continue;
		HDC hDesktopDC = GetDC(0);
		int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
		int nScreenHeight = GetSystemMetrics(SM_CYSCREEN);
		int origSize = nScreenWidth * nScreenHeight;
		double scaleFactor = sqrt((double)520000 / (double)origSize);
		int scaledWidth = (nScreenWidth * (scaleFactor > 1 ? 1 : scaleFactor));
		int scaledHeight = (nScreenHeight * (scaleFactor > 1 ? 1 : scaleFactor));
		HDC hBitmapDC = CreateCompatibleDC(hDesktopDC);
		BITMAPINFO bmpinfo = {{sizeof(BITMAPINFOHEADER), scaledWidth, -scaledHeight, 1, 32, BI_RGB}};
		unsigned char *pixels;
		HBITMAP hBitmap = CreateDIBSection(0, &bmpinfo, 0, (void **)&pixels, 0, 0);
		HGDIOBJ hOldBitmap = SelectObject(hBitmapDC, hBitmap);
		StretchBlt(hBitmapDC, 0, 0, scaledWidth, scaledHeight, hDesktopDC, 0, 0, nScreenWidth, nScreenHeight, SRCCOPY);
		int count = 0;
		for (int i = 0; i < scaledWidth; i++)
		{
			for (int o = 0; o < scaledHeight; o++)
			{
				int R = pixels[i * 3 * scaledHeight + o * 3];
				int G = pixels[i * 3 * scaledHeight + o * 3 + 1];
				int B = pixels[i * 3 * scaledHeight + o * 3 + 2];
				int gray = int(0.299 * (double)R + 0.587 * (double)G + 0.114 * (double)B);
				if (gray >= 240)
					count++;
			}
		}
		double per = (double)count / (double)(scaledWidth * scaledHeight);
		DeleteObject(hBitmap);
		DeleteDC(hBitmapDC);
		ReleaseDC(0,hDesktopDC);
		if (per > 0.95)
		{
			SetVolum(0.7f);
			PlaySoundW((LPWSTR)2001, GetModuleHandleW(0), SND_RESOURCE | SND_ASYNC);
			((GIF*)a)->counter = 0;
			if(hwnd1) ShowWindow(hwnd1, SW_SHOW);
			Sleep(60000);
		}
	}
}
void PrintLastError()
{
	DWORD errorCode = GetLastError();
	LPSTR errorMessage = NULL;
	DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
	DWORD languageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);

	if (FormatMessageA(flags, NULL, errorCode, languageId, (LPSTR)&errorMessage, 0, NULL) != 0)
	{
		printf("Error: %s\n", errorMessage);
		LocalFree(errorMessage);
	}
	else
	{
		printf("Failed to retrieve error message. Error code: %lu\n", errorCode);
	}
}
int WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nShowCmd)
{
	HANDLE hMutex = CreateMutexW(NULL, TRUE, L"dmhfdGenshinWhiteScreen");
	SetProcessDPIAware();
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		CloseHandle(hMutex);
		return 0;
	}
	MSG msg;
	WNDCLASSEXW wc = { 0 };
	GIF gif1;
	Group group;
	createparam para1 = {IDB_GIF1, 1, 128, &gif1};
	MakePtr a;
	a = {(LONG_PTR)&para1, (LONG_PTR)&group};
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)scanScreen, &gif1, 0, 0);
	MyRegisterClass((void *)WindowProc, &wc, L"dmhfd");
	D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory);
	hwnd1 = CreateWindowExW(WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW, L"dmhfd", 0, WS_POPUP, 0, 0, 0, 0, 0, 0, 0, &a);
	SetWindowDisplayAffinity(hwnd1, 1);
	while (GetMessageW(&msg, 0, 0, 0))
		DispatchMessageW(&msg);
	CloseHandle(hMutex);
	return 0;
	ExitProcess(0);
}
