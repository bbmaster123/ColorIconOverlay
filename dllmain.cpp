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
#include <propsys.h>
#include <propkey.h>
#include <propvarutil.h>
#include <STDDEF.h>



// {{6C12361D-4679-4B0C-98DF-9901E2E15294}}
// Replace with your own GUID
DEFINE_GUID(CLSID_IconOverlay,
    0x6C12361D, 0x4679, 0x4B0C,
    0x98, 0xDF, 0x99, 0x01, 0xE2, 0xE1, 0x52, 0x94);
DEFINE_GUID(CLSID_IconColorContextMenuHandler, 0xE00A6ADA, 0x8A09, 0x47D1, 
    0x9C, 0xDA, 0x24, 0x6C, 0x0F, 0xDA, 0xD9, 0x21);


HRESULT GetFilePropertyStore(PCWSTR pwszPath, IPropertyStore** ppPropStore)
{
    HRESULT hr = S_OK;

    // Get the file's property store
    IShellItem* pShellItem = NULL;
    hr = SHCreateItemFromParsingName(pwszPath, NULL, IID_PPV_ARGS(&pShellItem));
    if (SUCCEEDED(hr))
    {
        IPropertyStore* pPropStore;
        hr = pShellItem->QueryInterface(IID_PPV_ARGS(&pPropStore));
        *ppPropStore = pPropStore;
        pShellItem->Release();
    }

    return hr;
}

// Forward declaration of WriteColorToComment
HRESULT WriteColorToComment(PCWSTR pwszPath, const std::wstring& color)
{
    HRESULT hr = S_OK;

    // Get the file's property store
    IPropertyStore* pPropStore = NULL;
    hr = GetFilePropertyStore(pwszPath, &pPropStore);
    if (SUCCEEDED(hr))
    {
        // Write the color information to the file's comment field
        PROPVARIANT propVar;
        propVar.vt = VT_LPWSTR;
        propVar.pwszVal = const_cast<LPWSTR>(color.c_str());
        hr = pPropStore->SetValue(PKEY_Comment, propVar);
        PropVariantClear(&propVar);

        // Save the changes
        hr = pPropStore->Commit();
        pPropStore->Release();
    }

    return hr;
}

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
        HRESULT hr = StringCchCopy(pwszIconFile, cchMax, L"ColorIconOverlay.dll");
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
    CIconOverlay* pIconOverlay = new CIconOverlay();
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

    wchar_t message[] = L"{6C12361D-4679-4B0C-98DF-9901E2E15294}";


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

    WCHAR message[] = L"{6C12361D-4679-4B0C-98DF-9901E2E15294}";


    WCHAR szSubkey[MAX_PATH];
    hr = StringCchPrintf(szSubkey, ARRAYSIZE(szSubkey), L"CLSID\\%s", L"{6C12361D-4679-4B0C-98DF-9901E2E15294}");
    if (SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegDeleteTree(HKEY_CLASSES_ROOT, szSubkey));
    }

    return hr;
}
// DLL Entry Point
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
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
    HKEY hKey;
    LONG lResult = RegCreateKeyEx(HKEY_CLASSES_ROOT, TEXT(".png\\shell\\Icon Color\\DefaultIcon"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
    if (lResult != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(lResult);
    }
    lResult = RegSetValueEx(hKey, L"{6C12361D-4679-4B0C-98DF-9901E2E15294}", 0, REG_SZ, (LPBYTE)L"C:\\users\\bbmaster123\\desktop\\ColorIconOverlay-main\\x64\\Debug\\ColorIconOverlay.dll", wcslen(L"Icon Color Overlay"));
    RegCloseKey(hKey);
    if (lResult != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(lResult);
    }

    // Register the in-process component.
    wchar_t szModule[MAX_PATH];
    HRESULT hr = RegisterInprocServer(szModule, CLSID_IconOverlay,
        L"Icon Color Overlay", L"Apartment");
    hr = RegisterInprocServer(L"Directory", CLSID_IconColorContextMenuHandler, L"Icon Color Overlay", L"Apartment");
    if (SUCCEEDED(hr))
    {
        // Register the context menu handler.
        wchar_t szSubkey[MAX_PATH];
        hr = StringCchPrintf(szSubkey, ARRAYSIZE(szSubkey), L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\Icon Color Overlay");
        if (SUCCEEDED(hr))
        {
            HKEY hKey;
            LONG lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szSubkey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL);
            if (lRet == ERROR_SUCCESS)
            {
                lRet = RegSetValueEx(hKey, NULL, 0, REG_SZ, (LPBYTE)L"{6C12361D-4679-4B0C-98DF-9901E2E15294}", sizeof(L"{6C12361D-4679-4B0C-98DF-9901E2E15294}"));
                RegCloseKey(hKey);
            }
            hr = HRESULT_FROM_WIN32(lRet);
        }
        if (SUCCEEDED(hr))
        {
            // Register the context menu handler.
            wchar_t szSubkey[MAX_PATH];
            hr = StringCchPrintf(szSubkey, ARRAYSIZE(szSubkey),
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved");

            if (SUCCEEDED(hr))
            {
                HKEY hKey;
                LONG lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szSubkey, 0, KEY_WRITE, &hKey);
                if (lResult == ERROR_SUCCESS)
                {
                    lResult = RegSetValueEx(hKey, L"{6C12361D-4679-4B0C-98DF-9901E2E15294}", 0, REG_SZ, (LPBYTE)L"Icon Color Overlay", sizeof(L"Icon Color Overlay"));
                    RegCloseKey(hKey);
                }
                hr = HRESULT_FROM_WIN32(lResult);
            }
        }
        // register COM object for shortcut menu handler
            hr = RegisterInprocServer(szModule, CLSID_IconOverlay, L"AppCustomExtensions", L"Apartment");
            hr = RegisterInprocServer(L"*", CLSID_IconOverlay, L"ContextMenuExt", L"Apartment");
        if (SUCCEEDED(hr))
        {
            // register shortcut menu handler
            
        }
        // register COM object for overlay icon handler
        hr = RegisterInprocServer(szModule, CLSID_IconOverlay, L"Icon Color Overlay", L"Apartment");
        
        hr = RegisterInprocServer(szModule, CLSID_IconColorContextMenuHandler, L"Icon Color Overlay", L"Apartment");

    }
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

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
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\Icon Color Overlay");
        if (SUCCEEDED(hr))
        {
            LONG lRet = RegDeleteTree(HKEY_LOCAL_MACHINE, szSubkey);
            if (lRet != ERROR_SUCCESS)
                hr = HRESULT_FROM_WIN32(lRet);
        }
    }

    // Unregister the context menu handler
    hr = UnregisterInprocServer(CLSID_IconColorContextMenuHandler);

    return hr;
}



class IconColorContextMenuHandler : public IShellExtInit, public IContextMenu
{
public:
    // IUnknown methods
    IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv);

    IFACEMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_refCount); }
    IFACEMETHODIMP_(ULONG) Release() { return InterlockedDecrement(&m_refCount); }


    // IShellExtInit methods
    IFACEMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID);


    // IContextMenu methods
    IFACEMETHODIMP QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    IFACEMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici);
    IFACEMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT* pReserved, LPSTR pszName, UINT cchMax);

    IconColorContextMenuHandler() : m_refCount(1) {}

private:
    LPDATAOBJECT pDataObj; // Add this member variable
    ~IconColorContextMenuHandler() {}
    long m_refCount;
    std::vector<std::wstring> m_colors{ L"Red", L"Green", L"Blue", L"Yellow", L"Purple", L"Orange", L"None" };
    std::vector<std::wstring> m_files;
    // Helper function to add color options to the context menu
    void AddColorOptions(HMENU hSubMenu);
};
IFACEMETHODIMP IconColorContextMenuHandler::GetCommandString(UINT_PTR idCmd, UINT uType, UINT* pReserved, LPSTR pszName, UINT cchMax)
{
    // Check if the idCmd is valid
    if (idCmd < m_colors.size())
    {
        switch (uType)
        {
        case GCS_HELPTEXTW:
            // Return the help string for the menu item
            StringCchCopyW(reinterpret_cast<LPWSTR>(pszName), cchMax, m_colors[idCmd].c_str());
            break;
        case GCS_VALIDATEA:
        case GCS_VALIDATEW:
            return S_OK;
        }
    }
    return S_OK;
}



  // IShellExtInit methods
IFACEMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID);
// Implementation of IShellExtInit::Initialize
std::vector<std::wstring> m_files;
IFACEMETHODIMP IconColorContextMenuHandler::Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID)
{
    if (pDataObj == NULL)
    {
        return E_INVALIDARG;
    }

    FORMATETC format = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM medium;

    // pDataObj contains the objects being acted upon. In this case, it is the list of files that were selected.
    HRESULT hr = pDataObj->GetData(&format, &medium);
    if (SUCCEEDED(hr))
    {
        // Get the list of files from the data object.
        HDROP hDrop = static_cast<HDROP>(GlobalLock(medium.hGlobal));
        if (hDrop != NULL)
        {
            UINT numFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
            for (UINT i = 0; i < numFiles; i++)
            {
                wchar_t filePath[MAX_PATH];
                if (DragQueryFile(hDrop, i, filePath, ARRAYSIZE(filePath)) > 0)
                {
                    m_files.push_back(filePath);
                }
            }

            GlobalUnlock(medium.hGlobal);
        }

        ReleaseStgMedium(&medium);
    }

    return hr;
}

// Implementation of IContextMenu::QueryContextMenu
HRESULT IconColorContextMenuHandler::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    if (uFlags & CMF_DEFAULTONLY)
    {
        return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
    }

    UINT uID = idCmdFirst;
    HMENU hSubMenu = CreateMenu();

    for (size_t i = 0; i < m_colors.size(); ++i)
    {
        MENUITEMINFO mii = { 0 };
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID | MIIM_STRING; // Set the MIIM_ID flag
        mii.wID = uID++; // Set the menu item ID
        mii.dwTypeData = const_cast<LPWSTR>(m_colors[i].c_str());
        InsertMenuItem(hSubMenu, 0, TRUE, &mii); // Insert the menu item
    }

    MENUITEMINFO miiSubmenu = { 0 };
    miiSubmenu.cbSize = sizeof(MENUITEMINFO);
    miiSubmenu.fMask = MIIM_SUBMENU | MIIM_STRING; // Set the MIIM_SUBMENU flag
    miiSubmenu.hSubMenu = hSubMenu; // Set the submenu handle
    miiSubmenu.dwTypeData = const_cast<LPWSTR>(L"Color Options"); // Set the submenu title
    InsertMenuItem(hMenu, indexMenu, TRUE, &miiSubmenu); // Insert the submenu

    return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 1);
}


// Definition of ChangeIcon
HRESULT ChangeIcon(PCWSTR pwszPath, const std::wstring& color) {
    return WriteColorToComment(pwszPath, color);
}

IFACEMETHODIMP IconColorContextMenuHandler::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{

    const UINT ID_RED = 100;
    const UINT ID_GREEN = 101;
    const UINT ID_BLUE = 102;
    const UINT ID_YELLOW = 103;
    const UINT ID_PURPLE = 104;
    const UINT ID_ORANGE = 105;
    const UINT ID_NONE = 106;

    // Ensure this is not an offset but the actual command ID
    if (!HIWORD(pici->lpVerb))
    {
        UINT idCmd = LOWORD(pici->lpVerb);

        // Execute the command corresponding to the menu item selected by the user
        switch (idCmd)
        {
            case ID_RED:
                for (const auto& file : m_files)
                    ChangeIcon(file.c_str(), L"Red");
                break; 
            case ID_GREEN:
                for (const auto& file : m_files)
                    ChangeIcon(file.c_str(), L"Green");
                break; 
            case ID_BLUE:
                for (const auto& file : m_files)
                    ChangeIcon(file.c_str(), L"Blue");
                break;               
            case ID_YELLOW:
                for (const auto& file : m_files)
                    ChangeIcon(file.c_str(), L"Yellow");
                break;
            case ID_PURPLE:
                for (const auto& file : m_files)
                    ChangeIcon(file.c_str(), L"Purple");
                break;
            case ID_ORANGE:
                for (const auto& file : m_files)
                    ChangeIcon(file.c_str(), L"Orange");
                break;
            case ID_NONE:
                for (const auto& file : m_files)
                    ChangeIcon(file.c_str(), L"None");
                break;
            default:
                return E_FAIL; // Command ID not recognized
        }
        return S_OK;
    }
    return E_FAIL; // If the command ID is not recognized
}
enum ColorIDs {
    ID_RED = 100,
    ID_GREEN,
    ID_BLUE,
    ID_YELLOW,
    ID_PURPLE,
    ID_ORANGE,
    ID_NONE
};

// Implementation of IconColorContextMenuHandler::AddColorOptions
void IconColorContextMenuHandler::AddColorOptions(HMENU hSubMenu)
{
    MENUITEMINFO mii = { 0 };
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_ID | MIIM_STRING | MIIM_FTYPE;
    mii.wID = ID_RED;
    mii.dwTypeData = const_cast<LPWSTR>(L"Red");
    mii.fType = MFT_STRING;
    InsertMenuItem(hSubMenu, 0, TRUE, &mii);
}



    // Read the color back to confirm the change
    //std::wstring confirmedColor;
    //hr = ReadColorFromMetadata(pwszPath, confirmedColor);
   // if (FAILED(hr)) {
       // return hr; // Return the error code if reading failed
   // }

    // Optionally, compare 'color' and 'confirmedColor' to ensure they match
    // ...

    //return S_OK; // Return success code if everything went well
//}
