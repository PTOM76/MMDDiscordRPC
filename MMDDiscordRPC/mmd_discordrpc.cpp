#include "mmd_discordrpc.h"

using namespace mmp;
using namespace std;

UINT DiscordRPCMenu = createWM_APP_ID();
UINT SwitchMenu = createWM_APP_ID();
UINT SettingMenu = createWM_APP_ID();
UINT SettingMenu_Title = createWM_APP_ID();
UINT Separator = createWM_APP_ID();
UINT AboutMenu = createWM_APP_ID();

auto started_time = std::chrono::system_clock::now();
bool RPCEnable;
bool stateWindowTitleBool;
LONG_PTR originWndProc = NULL;
#ifdef _WIN64
#define _LONG_PTR LONG_PTR
#else
#define _LONG_PTR LONG
#endif

HANDLE hModule;
TCHAR iniPath[MAX_PATH + 1];

BOOL APIENTRY DllMain(HANDLE hModuleFunc, DWORD ul_reason_for_call,
    LPVOID lpReserved) {
    hModule = hModuleFunc;
    return TRUE;
}

// discord

const discord::ClientId client_id = 845620756078919700;

discord::Core* discordCore{};
discord::Activity discordActivity{};
static LPSTR SYS_INFO_STR = NULL;

BOOL settingDiscordRPC() {
    discord::Core::Create(client_id, DiscordCreateFlags_NoRequireDiscord,
        &discordCore);
    if (!discordCore) {
        HWND hWnd = getHWND();
        MessageBox(
            hWnd,
            L"Discordを起動していない可能性があるため、有効にできませんでした。",
            L"MMDDiscordRPC", MB_OK);
        CheckMenuItem(GetMenu(hWnd), SwitchMenu, MF_BYCOMMAND | MFS_UNCHECKED);
        RPCEnable = false;
        delete discordCore;
        return false;
    }
    else {
        discordActivity.GetAssets().SetLargeImage("mmd_icon");
        discordActivity.GetAssets().SetLargeText(
            SYS_INFO_STR == NULL ? "MikuMikuDance" : SYS_INFO_STR);
        discordActivity.SetState(u8"編集中");
        discordActivity.GetAssets().SetSmallText(u8"編集中");
        discordActivity.GetTimestamps().SetStart(
            std::chrono::duration_cast<std::chrono::seconds>(
                started_time.time_since_epoch())
            .count());
        discordActivity.SetType(discord::ActivityType::Playing);
        discordCore->ActivityManager().UpdateActivity(
            discordActivity, [](discord::Result result) {
                if (result == discord::Result::Ok) {
                    // MessageBox(getHWND(), L"有効になりました。", L"MMDDiscordRPC",
                    // MB_OK);
                }
                else {
                    // MessageBox(getHWND(), L"有効にできませんでした。",
                    // L"MMDDiscordRPC", MB_OK);
                }
            });
    }
    return true;
}

void __stdcall timerTick(HWND hWnd, UINT uMsg, UINT_PTR timer_id,
    DWORD dwTime) {
    if (RPCEnable) {
        // size_t i;
        if (stateWindowTitleBool) {
            char title[512];
            GetWindowTextA(getHWND(), title, sizeof(title));
            discordActivity.GetAssets().SetSmallText(title);
            discordActivity.SetState(title);
            discordCore->ActivityManager().UpdateActivity(
                discordActivity, [](discord::Result result) {});
        }
        else {
            discordActivity.SetState(u8"編集中");
            discordCore->ActivityManager().UpdateActivity(
                discordActivity, [](discord::Result result) {});
        }
        discordCore->RunCallbacks();
    }
}
// -discord

static LRESULT CALLBACK pluginWndProc(HWND hWnd, UINT msg, WPARAM wp,
    LPARAM lp) {
    switch (msg) {
        case WM_COMMAND: {
            WORD menuId = LOWORD(wp);
            if (menuId == (WORD)SwitchMenu) {
                auto menu = GetMenu(hWnd);
                UINT state = GetMenuState(menu, SwitchMenu, MF_BYCOMMAND);
                if (state & MFS_CHECKED) {
                    CheckMenuItem(menu, SwitchMenu, MF_BYCOMMAND | MFS_UNCHECKED);
                    RPCEnable = false;
                    WritePrivateProfileString(L"settings", L"switch", L"disable",
                        iniPath);
                    delete discordCore;
                }
                else {
                    CheckMenuItem(menu, SwitchMenu, MF_BYCOMMAND | MFS_CHECKED);
                    RPCEnable = true;
                    WritePrivateProfileString(L"settings", L"switch", L"enable", iniPath);
                    settingDiscordRPC();
                }
                break;
            }
            else if (menuId == (WORD)SettingMenu_Title) {
                auto menu = GetMenu(hWnd);
                UINT state = GetMenuState(menu, SettingMenu_Title, MF_BYCOMMAND);
                if (state & MFS_CHECKED) {
                    CheckMenuItem(menu, SettingMenu_Title, MF_BYCOMMAND | MFS_UNCHECKED);
                    stateWindowTitleBool = false;
                    WritePrivateProfileString(L"settings", L"stateWindowTitle", L"false",
                        iniPath);
                }
                else {
                    CheckMenuItem(menu, SettingMenu_Title, MF_BYCOMMAND | MFS_CHECKED);
                    stateWindowTitleBool = true;
                    WritePrivateProfileString(L"settings", L"stateWindowTitle", L"true",
                        iniPath);
                }
                break;
            }
            else if (menuId == (WORD)AboutMenu) {
                MessageBox(hWnd, L"MMDDiscordRPC v0.4\nby Pitan",
                    L"MMDDiscordRPCについて", MB_OK);
                break;
            }
        }
        default:
            break;
    }
    return CallWindowProc((WNDPROC)originWndProc, hWnd, msg, wp, lp);
};

void MMDDiscordRPCInit() {
    RPCEnable = true;
    stateWindowTitleBool = true;
    TCHAR dllPath[MAX_PATH + 1];
    TCHAR pszDrive[_MAX_DRIVE];
    TCHAR pszFolder[_MAX_DIR];
    TCHAR pszFile[_MAX_FNAME];
    TCHAR pszExtent[_MAX_EXT];
    GetModuleFileName((HMODULE)hModule, dllPath, MAX_PATH);
    _tsplitpath_s(dllPath, pszDrive, _MAX_DRIVE, pszFolder, _MAX_DIR, pszFile,
        _MAX_FNAME, pszExtent, _MAX_EXT);
    _tmakepath_s(iniPath, MAX_PATH, pszDrive, pszFolder, L"config", L"ini");
    if (!PathFileExists(iniPath)) {
        WritePrivateProfileString(L"settings", L"switch", L"enable", iniPath);
        WritePrivateProfileString(L"settings", L"stateWindowTitle", L"true",
            iniPath);
    }
    else {
        TCHAR settings_switch[256];
        TCHAR stateWindowTitle[256];
        GetPrivateProfileString(L"settings", L"switch", L"enable", settings_switch,
            256, iniPath);
        GetPrivateProfileString(L"settings", L"stateWindowTitle", L"true",
            stateWindowTitle, 256, iniPath);
        if (_tcscmp(settings_switch, L"enable") == 0) {
            RPCEnable = true;
        }
        else {
            RPCEnable = false;
        }
        if (_tcscmp(stateWindowTitle, L"true") == 0) {
            stateWindowTitleBool = true;
        }
        else {
            stateWindowTitleBool = false;
        }
    }
    HWND hWnd = getHWND();
    HMENU hMenu = GetMenu(hWnd);
    HMENU SubMenu = CreatePopupMenu();
    HMENU SettingSubMenu = CreatePopupMenu();

    MENUITEMINFOW mii;
    mii.cbSize = sizeof(MENUITEMINFOW);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_SUBMENU;
    mii.fType = MFT_STRING;
    mii.wID = DiscordRPCMenu;
    mii.hSubMenu = SubMenu;
    mii.dwTypeData = L"DiscordRPC";
    InsertMenuItemW(hMenu, mii.wID, FALSE, &mii);

    mii.fMask = MIIM_ID | MIIM_TYPE;
    mii.fType = MFT_STRING;
    mii.wID = SwitchMenu;
    mii.dwTypeData = L"RPCの表示切替";
    InsertMenuItemW(SubMenu, mii.wID, FALSE, &mii);

    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_SUBMENU;
    mii.fType = MFT_STRING;
    mii.wID = SettingMenu;
    mii.hSubMenu = SettingSubMenu;
    mii.dwTypeData = L"RPCの表示設定";
    InsertMenuItemW(SubMenu, mii.wID, FALSE, &mii);

    mii.fMask = MIIM_ID | MIIM_TYPE;
    mii.fType = MFT_STRING;
    mii.wID = SettingMenu_Title;
    mii.dwTypeData = L"ウィンドウのタイトル名をアクティビティへ表示する";
    InsertMenuItemW(SettingSubMenu, mii.wID, FALSE, &mii);

    mii.fMask = MIIM_ID | MIIM_TYPE;
    mii.fType = MFT_SEPARATOR;
    mii.dwTypeData = NULL;
    mii.wID = Separator;
    InsertMenuItemW(SubMenu, mii.wID, FALSE, &mii);

    mii.fMask = MIIM_ID | MIIM_TYPE;
    mii.fType = MFT_STRING;
    mii.wID = AboutMenu;
    mii.dwTypeData = L"MMDDiscordRPCについて";
    InsertMenuItemW(SubMenu, mii.wID, FALSE, &mii);

    DrawMenuBar(hWnd);

    if (RPCEnable) {
        CheckMenuItem(hMenu, SwitchMenu, MF_BYCOMMAND | MFS_CHECKED);
    }
    else {
        CheckMenuItem(hMenu, SwitchMenu, MF_BYCOMMAND | MFS_UNCHECKED);
    }

    if (stateWindowTitleBool) {
        CheckMenuItem(hMenu, SettingMenu_Title, MF_BYCOMMAND | MFS_CHECKED);
    }
    else {
        CheckMenuItem(hMenu, SettingMenu_Title, MF_BYCOMMAND | MFS_UNCHECKED);
    }

    settingDiscordRPC();
    UINT_PTR timer = SetTimer(NULL, NULL, 500, timerTick);

    originWndProc = GetWindowLongPtr(hWnd, GWLP_WNDPROC);
    SetWindowLongPtr(hWnd, GWLP_WNDPROC, (_LONG_PTR)pluginWndProc);
}

class mmd_discordrpc : public MMDPluginDLL1 {
    HMODULE classModule;

    public:
        mmd_discordrpc() : classModule((HMODULE)hModule) { MMDDiscordRPCInit(); }
};

MMD_PLUGIN_API int version() { return 1; }

MMD_PLUGIN_API MMDPluginDLL1* create1(IDirect3DDevice9* device) {
    return new mmd_discordrpc;
}

MMD_PLUGIN_API void destroy1(MMDPluginDLL1* p) { delete p; }
