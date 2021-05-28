#include "mmd_discordrpc.h"
#ifdef NDEBUG
#define printf(...) (void)0
#endif // !NDEBUG

using namespace std;

bool rpcIsTrue;
LONG_PTR originWndProc = NULL;
#ifdef _WIN64
#define _LONG_PTR LONG_PTR
#else
#define _LONG_PTR LONG
#endif

HANDLE hModule;
TCHAR iniPath[MAX_PATH + 1];

BOOL APIENTRY DllMain(HANDLE hModuleFunc, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    hModule = hModuleFunc;
    //TCHAR   szModuleFileName[_MAX_PATH] = L"";
    //GetModuleFileName((HMODULE)hModule, szModuleFileName, _MAX_PATH);
    //MessageBox(NULL, szModuleFileName, L"ModuleFileName", MB_OK);
    return TRUE;
}

// discord

const discord::ClientId client_id = 845620756078919700;

discord::Core* core{};
discord::Activity activity{};
static LPSTR SYS_INFO_STR = NULL;

BOOL settingDiscordRPC() {
    discord::Core::Create(client_id, DiscordCreateFlags_NoRequireDiscord, &core);
    if (!core) {
        auto hwnd = getHWND();
        MessageBoxW(hwnd, L"Discordを起動していない可能性があるため、有効にできませんでした。", L"MMDDiscordRPC", MB_OK);
        CheckMenuItem(GetMenu(hwnd), 101, MF_BYCOMMAND | MFS_UNCHECKED);
        rpcIsTrue = false;
    }
    else {
        activity.GetAssets().SetLargeImage("mmd_icon");
        activity.GetAssets().SetLargeText(SYS_INFO_STR == NULL ? "MikuMikuDance" : SYS_INFO_STR);
        activity.SetState(u8"編集中");
        activity.GetAssets().SetSmallText(u8"編集中");
        activity.SetType(discord::ActivityType::Playing);
        core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
            if (result == discord::Result::Ok) {
                // MessageBoxW(getHWND(), L"有効になりました。", L"MMDDiscordRPC", MB_OK);
            }
            else {
                // MessageBoxW(getHWND(), L"有効にできませんでした。", L"MMDDiscordRPC", MB_OK);
            }
            });
    }
    return true;
}

void __stdcall timerTick(HWND hWnd, UINT uMsg, UINT_PTR timer_id, DWORD dwTime) {
    if (rpcIsTrue == true) {
        core->RunCallbacks();
    }
}
// -discord

static LRESULT CALLBACK pluginWndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case 101:
        {
            auto menu = GetMenu(hWnd);
            UINT state = GetMenuState(menu, 101, MF_BYCOMMAND);
            if (state & MFS_CHECKED) {
                CheckMenuItem(menu, 101, MF_BYCOMMAND | MFS_UNCHECKED);
                rpcIsTrue = false;
                WritePrivateProfileString(L"settings", L"switch", L"disable", iniPath);
                delete core;
            }
            else {
                CheckMenuItem(menu, 101, MF_BYCOMMAND | MFS_CHECKED);
                rpcIsTrue = true;
                WritePrivateProfileString(L"settings", L"switch", L"enable", iniPath);
                settingDiscordRPC();
            }
            break;
        }
        case 103: {
            MessageBoxW(hWnd, L"MMDDiscordRPC v0.2\nby Pitan", L"MMDDiscordRPCについて", MB_OK);
            break;
        }
        }
    default:
        break;
    }
    return CallWindowProc((WNDPROC)originWndProc, hWnd, msg, wp, lp);
};

void MMDDiscordRPCInit()
{
    rpcIsTrue = true;
    TCHAR dllPath[MAX_PATH + 1];
    TCHAR pszDrive[_MAX_DRIVE];
    TCHAR pszFolder[_MAX_DIR];
    TCHAR pszFile[_MAX_FNAME];
    TCHAR pszExtent[_MAX_EXT];
    GetModuleFileName((HMODULE)hModule, dllPath, MAX_PATH);
    _tsplitpath_s(dllPath, pszDrive, _MAX_DRIVE, pszFolder, _MAX_DIR, pszFile, _MAX_FNAME, pszExtent, _MAX_EXT);
    _tmakepath_s(iniPath, MAX_PATH, pszDrive, pszFolder, L"config", L"ini");
    if (!PathFileExists(iniPath)) {
        WritePrivateProfileString(L"settings", L"switch", L"enable", iniPath);
    } else {
        TCHAR settings_switch[256];
        GetPrivateProfileString(L"settings", L"switch", L"enable", settings_switch, 256, iniPath);
        if (_tcscmp(settings_switch, L"enable") == 0) {
            rpcIsTrue = true;
        } else {
            rpcIsTrue = false;
        }
    }
    auto hwnd = getHWND();
    auto menu = GetMenu(hwnd);
    HMENU menuSub = CreatePopupMenu();

    MENUITEMINFOW mii;
    mii.cbSize = sizeof(MENUITEMINFOW);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_SUBMENU;
    mii.fType = MFT_STRING;
    mii.wID = 100;
    mii.hSubMenu = menuSub;
    mii.dwTypeData = L"DiscordRPC";
    InsertMenuItemW(menu, mii.wID, FALSE, &mii);

    mii.fMask = MIIM_ID | MIIM_TYPE;
    mii.fType = MFT_STRING;
    mii.wID = 101;
    mii.dwTypeData = L"RPCの表示切替";
    InsertMenuItemW(menuSub, mii.wID, FALSE, &mii);

    mii.fMask = MIIM_ID | MIIM_TYPE;
    mii.fType = MFT_SEPARATOR;
    mii.dwTypeData = NULL;
    mii.wID = 102;
    InsertMenuItemW(menuSub, mii.wID, FALSE, &mii);

    mii.fMask = MIIM_ID | MIIM_TYPE;
    mii.fType = MFT_STRING;
    mii.wID = 103;
    mii.dwTypeData = L"MMDDiscordRPCについて";
    InsertMenuItemW(menuSub, mii.wID, FALSE, &mii);

    DrawMenuBar(hwnd);

    if (rpcIsTrue == true) {
        CheckMenuItem(menu, 101, MF_BYCOMMAND | MFS_CHECKED);
    } else {
        CheckMenuItem(menu, 101, MF_BYCOMMAND | MFS_UNCHECKED);
    }

    settingDiscordRPC();
    UINT_PTR timer = SetTimer(
        NULL,
        NULL,
        500,
        timerTick
    );

    originWndProc = GetWindowLongPtr(hwnd, GWLP_WNDPROC);
    SetWindowLongPtr(hwnd, GWLP_WNDPROC, (_LONG_PTR)pluginWndProc);
}

int version() { return 3; }

MMDPluginDLL3* create3(IDirect3DDevice9* device)
{
    MMDDiscordRPCInit();
    return nullptr;
}

