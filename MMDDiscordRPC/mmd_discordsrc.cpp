#include "mmd_discordsrc.h"
#include <windows.h>

#include <iostream>

using namespace std;

#ifdef NDEBUG
#define printf(...) (void)0

#endif // !NDEBUG
#pragma pack(push, 8)
#include "discordsdk/discord.h"
#pragma pack(pop)
bool rpcIsTrue;

// discord

const discord::ClientId client_id = 845620756078919700;
discord::Core* core{};
discord::Activity activity{};
static LPSTR SYS_INFO_STR = NULL;

BOOL settingDiscordRPC() {
    discord::Core::Create(client_id, DiscordCreateFlags_Default, &core);
    if (!core) {
        MessageBoxW(getHWND(), L"初期化が失敗しました。", L"MMDDiscordRPC", MB_OK);
    }
    activity.GetAssets().SetLargeImage("mmd_icon");
    activity.GetAssets().SetLargeText(SYS_INFO_STR == NULL ? "MikuMikuDance" : SYS_INFO_STR);
    activity.SetState(u8"編集中");
    activity.GetAssets().SetSmallText(u8"編集中");
    activity.SetType(discord::ActivityType::Playing);
    core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
        if (result == discord::Result::Ok) {
        //    MessageBoxW(getHWND(), L"有効になりました。", L"MMDDiscordRPC", MB_OK);
        }
        else {
        //    MessageBoxW(getHWND(), L"有効にできませんでした。", L"MMDDiscordRPC", MB_OK);
        }
        });
    return true;
}

void __stdcall timerTick(HWND hWnd, UINT uMsg, UINT_PTR timer_id, DWORD dwTime) {
    if (rpcIsTrue == true) {
        core->RunCallbacks();
    }
}
// -discord

//CIniFile configFile = L"./config.ini";

void MMDDiscordRPCInit()
{
    //if (!PathFileExists(configFile.c_str())) {
    //    WritePrivateProfileString(L"SECTION", L"key_str", L"テスト文字列", configFile.c_str());
    //}
    auto hwnd = getHWND();
    auto menu = GetMenu(hwnd);
    HMENU menuSub = CreatePopupMenu();

    MENUITEMINFOW mii;
    mii.cbSize = sizeof(MENUITEMINFOW);
    mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_SUBMENU;
    mii.fType = MFT_STRING;
    mii.wID = 1;
    mii.hSubMenu = menuSub;
    mii.dwTypeData = L"DiscordRPC";
    InsertMenuItemW(menu, mii.wID, FALSE, &mii);
    mii.fMask = MIIM_ID | MIIM_TYPE;
    mii.fType = MFT_STRING;
    mii.wID = 101;
    mii.dwTypeData = L"RPCの表示切替";
    //InsertMenuItemW(menuSub, mii.wID, FALSE, &mii);
    DrawMenuBar(hwnd);

    rpcIsTrue = true;
    settingDiscordRPC();
    UINT_PTR timer = SetTimer(
        NULL,
        NULL,
        500,
        timerTick
    );
}
class MMDDiscordRPC : public MMDPluginDLL3
{
public:
    void WndProc(const CWPSTRUCT* param) override {
        switch (param->message)
        {
        case WM_COMMAND:
            switch (LOWORD(param->wParam)) {
            case 101:
                MessageBoxW(param->hwnd, L"有効になりました。", L"MMDDiscordRPC", MB_OK);
                break;
            }
        default:
            break;
        }
    };

    void stop() override {
        MessageBoxW(getHWND(), L"stoped", L"MMDDiscordRPC", MB_OK);
    }
    std::pair<bool, LRESULT> WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) { return { false,0 }; };

    const char* getPluginTitle() const override { return mmd_discordrpc; }

    static constexpr char* mmd_discordrpc = "DiscordRPC";

    static MMDDiscordRPC* getObject()
    {
        return dynamic_cast<MMDDiscordRPC*>(mmp::getDLL3Object(mmd_discordrpc));
    }
};

int version() { return 3; }

MMDDiscordRPC* mmdDiscordRPC;

MMDPluginDLL3* create3(IDirect3DDevice9* device)
{
    mmdDiscordRPC;
    MMDDiscordRPCInit();
    return mmdDiscordRPC;
}

