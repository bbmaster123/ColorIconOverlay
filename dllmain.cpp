#include <windows.h>
#include "Resource.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <new>
#include <Initguid.h>
#pragma comment(lib, "shlwapi.lib")
#include <vector>
#include <string>


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
    // Check if the path is a file
    if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)
    {
        // It's a directory (folder)
        return S_FALSE; // Return S_FALSE for folders
    }
    else
    {
        // It's a file
        return S_OK; // Return S_OK for files
    }
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

    wchar_t L"{6C12361D-4679-4B0C-98DF-9901E2E15294}"[MAX_PATH];


    wchar_t szSubkey[MAX_PATH];
    hr = StringCchPrintf(szSubkey, ARRAYSIZE(szSubkey),
        L"CLSID\\%s", L"{6C12361D-4679-4B0C-98DF-9901E2E15294}");
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
                L"CLSID\\%s\\InprocServer32", L"{6C12361D-4679-4B0C-98DF-9901E2E15294}");
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

    WCHAR L"{6C12361D-4679-4B0C-98DF-9901E2E15294}"[MAX_PATH];


    WCHAR szSubkey[MAX_PATH];
    hr = StringCchPrintf(szSubkey, ARRAYSIZE(szSubkey), L"CLSID\\%s", L"{6C12361D-4679-4B0C-98DF-9901E2E15294}");
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



class IconColorContextMenuHandler : public IShellExtInit, public IContextMenu
{
public:
    // IUnknown methods
    IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);
    IFACEMETHODIMP_(ULONG) AddRef();
    IFACEMETHODIMP_(ULONG) Release();

    // IShellExtInit methods
    IFACEMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID);

    // IContextMenu methods
    IFACEMETHODIMP QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    IFACEMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici);
    IFACEMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pReserved, LPSTR pszName, UINT cchMax);

    IconColorContextMenuHandler() : m_refCount(1) {}

private:
    ~IconColorContextMenuHandler() {}
    long m_refCount;
    std::vector<std::wstring> m_colors{ L"Red", L"Green", L"Blue", L"Yellow", L"Purple", L"Orange", L"None" };

    // Helper function to add color options to the context menu
    void AddColorOptions(HMENU hSubMenu);
};

// Implementation of IShellExtInit::Initialize
// ...

// Implementation of IContextMenu::QueryContextMenu
IFACEMETHODIMP IconColorContextMenuHandler::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    // Only display the context menu when right-clicking
    if (uFlags & CMF_DEFAULTONLY)
        return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);

    HMENU hSubMenu = CreatePopupMenu();
    if (!hSubMenu)
        return E_OUTOFMEMORY;

    AddColorOptions(hSubMenu);

    InsertMenu(hMenu, indexMenu, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hSubMenu, L"Icon Color");
    return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, m_colors.size());
}

IFACEMETHODIMP IconColorContextMenuHandler::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
    // Ensure this is not an offset but the actual command ID
    if (!HIWORD(pici->lpVerb))
    {
        UINT idCmd = LOWORD(pici->lpVerb);

        // Execute the command corresponding to the menu item selected by the user
        switch (idCmd)
        {
            case ID_RED:
                // Code to change the icon color to red
                break;
            case ID_GREEN:
                // Code to change the icon color to green
                break;
            // ... other cases for different colors
            default:
                return E_FAIL; // Command ID not recognized
        }
        return S_OK;
    }
    return E_FAIL; // If the command ID is not recognized
}

// Implementation of IContextMenu::GetCommandString
// ...

// Implementation of IconColorContextMenuHandler::AddColorOptions
void IconColorContextMenuHandler::AddColorOptions(HMENU hSubMenu)
{
    for (size_t i = 0; i < m_colors.size(); ++i)
    {
        InsertMenu(hSubMenu, i, MF_BYPOSITION, i, m_colors[i].c_str());
    }
}

