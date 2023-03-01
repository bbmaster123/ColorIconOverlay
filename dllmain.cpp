#include <windows.h>
#include "Resource.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <new>
#include <Initguid.h>
#pragma comment(lib, "shlwapi.lib")


// {{6C12361D-4679-4B0C-98DF-9901E2E15294}}
// Replace with your own GUID
DEFINE_GUID(CLSID_IconOverlay,
    0x6C12361D, 0x4679, 0x4B0C,
    0x98, 0xDF, 0x99, 0x01, 0xE2, 0xE1, 0x52, 0x94);

HINSTANCE g_hInst = NULL;

class CIconOverlay : public IShellIconOverlayIdentifier
{
public:
    CIconOverlay() : _cRef(1) {}

    // IUnknown methods
    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv)
    {
        static const QITAB qit[] =
        {
            QITABENT(CIconOverlay,IShellIconOverlayIdentifier),
            { 0 },
        };
        return QISearch(this, qit, riid, ppv);
    }

    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&_cRef);
    }

    IFACEMETHODIMP_(ULONG) Release()
    {
        ULONG cRef = InterlockedDecrement(&_cRef);
        if (cRef == 0)
        {
            delete this;
        }
        return cRef;
    }

    // IShellIconOverlayIdentifier methods
    IFACEMETHODIMP GetOverlayInfo(PWSTR pwszIconFile, int cchMax, int* pIndex, DWORD* pdwFlags)
    {
        HRESULT hr = StringCchCopy(pwszIconFile, cchMax, L"IconColorOverlay.dll");
        if (SUCCEEDED(hr))
        {
            *pIndex = IDI_OVERLAYICON;
            *pdwFlags = ISIOI_ICONFILE | ISIOI_ICONINDEX;
        }
        return hr;
    }
    IFACEMETHODIMP GetPriority(int* pPriority)
    {
        *pPriority = 0;
        return S_OK;
    }

    IFACEMETHODIMP IsMemberOf(PCWSTR pwszPath, DWORD dwAttrib)
    {
        // Add your own logic here to determine whether to show the overlay icon
        return S_OK;
    }

private:
    ~CIconOverlay() {}

    long _cRef;
};

HRESULT CIconOverlay_CreateInstance(REFIID riid, void** ppv)
{
    CIconOverlay* pIconOverlay = new (std::nothrow) CIconOverlay();
    HRESULT hr = pIconOverlay ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        hr = pIconOverlay->QueryInterface(riid, ppv);
        pIconOverlay->Release();
    }
    return hr;
}

HRESULT RegisterInprocServer(PCWSTR pszModule, const CLSID& clsid,
    PCWSTR pszFriendlyName, PCWSTR pszThreadModel)
{
    if (!pszModule || !pszFriendlyName || !pszThreadModel)
        return E_INVALIDARG;

    HRESULT hr;

    wchar_t szCLSID[MAX_PATH];


    wchar_t szSubkey[MAX_PATH];
    hr = StringCchPrintf(szSubkey, ARRAYSIZE(szSubkey),
        L"CLSID\\%s", szCLSID);
    if (SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegSetKeyValue(
            HKEY_CLASSES_ROOT,
            szSubkey,
            nullptr,
            REG_SZ,
            pszFriendlyName,
            (lstrlen(pszFriendlyName) + 1) * sizeof(wchar_t)));
        if (SUCCEEDED(hr))
        {
            hr = StringCchPrintf(szSubkey, ARRAYSIZE(szSubkey),
                L"CLSID\\%s\\InprocServer32", szCLSID);
            if (SUCCEEDED(hr))
            {
                hr = HRESULT_FROM_WIN32(RegSetKeyValue(
                    HKEY_CLASSES_ROOT,
                    szSubkey,
                    nullptr,
                    REG_SZ,
                    pszModule,
                    (lstrlen(pszModule) + 1) * sizeof(wchar_t)));
                if (SUCCEEDED(hr))
                {
                    hr = HRESULT_FROM_WIN32(RegSetKeyValue(
                        HKEY_CLASSES_ROOT,
                        szSubkey,
                        L"ThreadingModel",
                        REG_SZ,
                        pszThreadModel,
                        (lstrlen(pszThreadModel) + 1) * sizeof(wchar_t)));
                }
            }
        }
    }

    return hr;
}

HRESULT UnregisterInprocServer(const CLSID& clsid)
{
    HRESULT hr = S_OK;

    WCHAR szCLSID[MAX_PATH];


    WCHAR szSubkey[MAX_PATH];
    hr = StringCchPrintf(szSubkey, ARRAYSIZE(szSubkey), L"CLSID\\%s", szCLSID);
    if (SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegDeleteTree(HKEY_CLASSES_ROOT, szSubkey));
    }

    return hr;
}
// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstance);
        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
    return S_OK;
}

// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;
    if (IsEqualCLSID(CLSID_IconOverlay, rclsid))
    {
        hr = CIconOverlay_CreateInstance(riid, ppv);
    }
    return hr;
}

STDAPI DllRegisterServer()
{
    wchar_t szModule[MAX_PATH];
    if (GetModuleFileName(g_hInst, szModule, ARRAYSIZE(szModule)) == 0)
    {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    // Register the in-process component.
    HRESULT hr = RegisterInprocServer(szModule, CLSID_IconOverlay,
        L"Your Icon Overlay", L"Apartment");
    if (SUCCEEDED(hr))
    {
        // Register the icon overlay handler.
        wchar_t szSubkey[MAX_PATH];
        hr = StringCchPrintf(szSubkey, ARRAYSIZE(szSubkey),
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\Your Icon Overlay");
        
    }

    return hr;
}

STDAPI DllUnregisterServer()
{
    // Unregister the in-process component.
    HRESULT hr = UnregisterInprocServer(CLSID_IconOverlay);
    if (SUCCEEDED(hr))
    {
        // Unregister the icon overlay handler.
        wchar_t szSubkey[MAX_PATH];
        hr = StringCchPrintf(szSubkey, ARRAYSIZE(szSubkey),
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\Your Icon Overlay");
        if (SUCCEEDED(hr))
        {
            LONG lRet = RegDeleteTree(HKEY_LOCAL_MACHINE, szSubkey);
            if (lRet != ERROR_SUCCESS)
                hr = HRESULT_FROM_WIN32(lRet);
        }
    }

    return hr;
}