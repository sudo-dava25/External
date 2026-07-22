#include "main.h"
#include <linux/input.h>
#include <linux/uinput.h>
#include <vector>
#include <functional>
#include <cstdio>
#include <unistd.h>
#include <cstdlib>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <fstream>
#include <cstring>
#include <ctime>
#include <malloc.h>
#include <iostream>
#include <fstream>
#include <sys/system_properties.h>
#include <ctime>
#include <string>
#include <iostream>
#include <main.h>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <linux/input.h>
#include <vector>
#include <functional>
#include "Memory/Memory.h"
#include "Memory/PatternScanner.h"

#include "Quaternion.hpp"
#include "Vector2.hpp"
#include "Vector3.hpp"

#include "Includes/Log.h"
#include <vector>
#include "Includes/Offset.h"
#include "Engine/CanvasView.h"
#include "include.h"
#include "Matrix4x4.hpp"
#include "ToString.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "icon/HeroIcons.h"
#include "Decoder64.h"
#include "DrawIconHero.h"

using namespace Memory;

bool main_thread_flag = true;
int abs_ScreenX = 0;
int abs_ScreenY = 0;
bool drawMAddress;
bool drawMBox = true;
bool drawMLine = true;
bool drawMPostion = true;
bool drawMHealth = true;
bool drawMDistance = true;
bool drawMName = true;
bool drawAlertUnderAttack = true;
bool iconhero = true;
float RadiusCir = 50.0f;
long libbase = 0;

std::string fshy(uintptr_t address)
{
    if (!address) return "";

    auto stringLength = Read<uint32_t>(address + 0x10);
    char16_t buffer[255] = { 0 };

    pvm(reinterpret_cast<void *>(address + 0x14), reinterpret_cast<void *>(buffer), static_cast<size_t>(stringLength) * 2, false);

    return utf16_to_utf8(buffer, stringLength);
}

struct RoomPlayerInfo {
    std::string Name;
    std::string UserID;
    std::string Squad;
    std::string Rank;
    std::string Hero;
    std::string Spell;
};

RoomPlayerInfo PlayerB[5];
RoomPlayerInfo PlayerR[5];

struct String {
    char pad_0000[0x10];
    int length;
    wchar_t buffer[1];

    const char* CString() const {
        static char temp[256];
        wcstombs(temp, buffer, length);
        temp[length] = '\0';
        return temp;
    }
};

uintptr_t GetMainCamera() {
    auto main_cam = Read<uintptr_t>(libbase + 0x75dc470);
    if (!main_cam)
        return 0;
    auto main_cam2 = Read<uintptr_t>(main_cam + 0xb8);
    if (!main_cam2)
        return 0;
    auto main_cam3 = Read<uintptr_t>(main_cam2 + 0x8);
    if (!main_cam3)
        return 0;
    return main_cam3;
}

struct Camera {
    Matrix4x4 worldToCameraMatrix;
    Matrix4x4 projectionMatrix;
};

Matrix4x4 _vMatrix;

bool WorldToScreen(Vector3 from, Vector2 *to) {
    auto viewMatrix = _vMatrix.MultiplyPoint(from);
    auto screenPos = Vector3(viewMatrix.X + 1.0f, viewMatrix.Y + 1.0f, viewMatrix.Z + 1.0f) / 2.0f;
    *to = Vector2(screenPos.X * abs_ScreenX, abs_ScreenY - (screenPos.Y * abs_ScreenY));
    return viewMatrix != Vector3::Zero();
}

void FindPoint(Vector2 origin, Vector2 &point, int screenwidth, int screenheight, int length)
{
    float halfScreenWidth = screenwidth / 2.0f;
    float halfScreenHeight = screenheight / 2.0f;
    float halfScreenWidth2 = (screenwidth - length) / 2.0f;
    float halfScreenHeight2 = (screenheight - length) / 2.0f;
    float dx = fabs(origin.X - halfScreenWidth);
    float dy = fabs(origin.Y - halfScreenHeight);
    float rx = (dx != 0) ? halfScreenWidth2 / dx : 0;
    float ry = (dy != 0) ? halfScreenHeight2 / dy : 0;
    float r = fmin(rx, ry);
    point.X = origin.X + (halfScreenWidth - origin.X) * (1.0f - r);
    point.Y = origin.Y + (halfScreenHeight - origin.Y) * (1.0f - r);
}

int ListMonsterId[] = {
        2002,
        2003,
        2004,
        2005,
        2006,
        2008,
        2009,
        2011,
        2012,
        2013,
        2056,
        2059,
        2072,
        2220,
        2221,
        2222,
        2223,
        2224,
        2225,
        2226,
        2227,
        2228,
        2229,
        2230,
        2232,
};

bool bMonster(int iValue) {
    return std::find(std::begin(ListMonsterId), std::end(ListMonsterId), iValue) != std::end(ListMonsterId);
}

void Touch_Tap(int x, int y) {
     Touch_Down((float)x, (float)y);
     usleep(80000);
     Touch_Up();
}

void DrawMonster(ImDrawList *Draw) {
    if (abs_ScreenX < abs_ScreenY) return;
    
    float lineSize = abs_ScreenY / 432;
    long a1 = getPtr641(libbase + 0x7641e18);
    long a2 = getPtr641((a1 + ((0x100 | 0xB8) & 0xFF)));
    long a32 = getPtr641((a2 << 1) >> 1);

    /**
    class BattleManager
    perlu update dari dump.cs*
    **/
    size_t m_LocalPlayerShow = 0x50;
    size_t m_ShowPlayers = 0x78;
    size_t m_ShowMonsters = 0x80;
    
    /**
    class ShowEntity 
    perlu update dari dump.cs*
    **/
    size_t m_iType = 0x80;

    size_t m_Hp = 0x1ac;
    size_t m_HpMax = 0x1b0;
    size_t m_bDeath = 0xcd;
    size_t m_bSameCampType = 0x2b1;
    size_t m_vCachePosition = 0x294;
    /**
    class ShowPlayer 
    perlu update dari dump.cs*
    **/
    size_t m_HeroName = 0x8d8;
    
    long selfp = getPtr641(a32 + m_LocalPlayerShow);
    
    auto main_cam = GetMainCamera();

    auto camera = Read<uintptr_t>(main_cam + 0x10);
    
    auto ViewMatrix = Read<Camera>(camera + 0x5C);
    _vMatrix = ViewMatrix.projectionMatrix * ViewMatrix.worldToCameraMatrix;

    long player = getPtr641(getPtr641(a32+m_ShowPlayers)+0x10)+0x20;
    uint stop_player = Read<uint>(getPtr641(a32+m_ShowPlayers)+0x18);
    
    for (int i = 0; i < stop_player; i++) {
        auto Objaddr = getPtr641(player + ((i << 3) / 1));

        if ((Objaddr ^ 0x0) == 0x0) {
            continue;
        }

        auto is_team = Read<bool>(Objaddr + m_bSameCampType);
        if (is_team) {
            continue;
        }
        auto HeroID = Read<int>(Objaddr + 0x194);     

        auto death = Read<bool>(Objaddr + m_bDeath);
        if (death) {
            continue;
        }

        int Health = Read<uint64_t>(Objaddr + m_Hp);
        if(Health <= 0)
        {
            continue;
        }

        int maxHealth = Read<uint64_t>(Objaddr + m_HpMax);
        if(Health <= 0)
        {
            continue;
        }

        Vector3 Z;
        vm_readv(selfp + m_vCachePosition, &Z, sizeof(Z));
      
        Vector3 D;
        vm_readv(Objaddr + m_vCachePosition, &D, sizeof(D));
        
        Vector2 en_posSc;
        WorldToScreen(D, &en_posSc);
        
        Vector2 loc_posSc;
        WorldToScreen(Z, &loc_posSc);
        
        bool isOutScreen;
        float IconSize = abs_ScreenX / 10.4;
        Vector2 HeroPos = {en_posSc.X, en_posSc.Y};
        Vector2 Res;
        
        if (HeroPos.X < 0 || HeroPos.X > abs_ScreenX || HeroPos.Y < 0 ||
            HeroPos.Y > abs_ScreenY) {
            isOutScreen = true;
            IconSize = abs_ScreenX / 15.6;
            FindPoint(HeroPos, Res, abs_ScreenX, abs_ScreenY, (IconSize / 3));
        } else {
            isOutScreen = false;
            Res = HeroPos;
        }
    
        auto Distance = Vector3::Distance(Z, D);
    
        if (drawMHealth) {
            ImGui::GetForegroundDrawList()->AddLine({loc_posSc.X,loc_posSc.Y}, {en_posSc.X, en_posSc.Y}, ImColor(255, 255, 255), lineSize);
        }
        
        if (iconhero) {
            ImVec2 iconPos(HeroPos.X, HeroPos.Y);
            DrawHeroIcon(ImGui::GetBackgroundDrawList(), iconPos, HeroID, Health, maxHealth);
        }

        if (drawMDistance) {
            std::string s;
            s += std::to_string((int)Distance);
            s += "m | ";
            s += "Health: "+ std::to_string((int)Health);
            s += " | " + fshy(Read<uintptr_t>(Objaddr + m_HeroName));

            auto textSize1 = ImGui::CalcTextSize(s.c_str(), 0, 29);
            绘制字体描边(22.5,HeroPos.X - (textSize1.x / 2), HeroPos.Y,ImColor(248,248,255),s.c_str());
        }

    }
    long monster = getPtr641(getPtr641(a32+m_ShowMonsters)+0x10)+0x20;
    uint stop_monster = Read<uint>(getPtr641(a32+m_ShowMonsters)+0x18);
    
    for (int i = 0; i < stop_monster; i++) {
        auto Objaddr = getPtr641(monster + ((i << 3) / 1));

        if ((Objaddr ^ 0x0) == 0x0) {
            continue;
        }

        auto is_team = Read<bool>(Objaddr + m_bSameCampType);
        if (is_team) {
            continue;
        }

        auto mHeroID = Read<int>(Objaddr + 0x194);        
        auto type = Read<int>(Objaddr + m_iType);
        
        auto death = Read<bool>(Objaddr + m_bDeath);
        if (death) {
            continue;
        }

        int Health = Read<uint64_t>(Objaddr + m_Hp);
        if(Health <= 0)
        {
            continue;
        }
        
        int maxHealth = Read<uint64_t>(Objaddr + m_HpMax);
        if(maxHealth <= 0)
        {
            continue;
        }     
        
        Vector3 ZL;
        vm_readv(selfp + m_vCachePosition, &ZL, sizeof(ZL));
      
        Vector3 Dm;
        vm_readv(Objaddr + m_vCachePosition, &Dm, sizeof(Dm));
        
        Vector2 mon_posSc;
        WorldToScreen(Dm, &mon_posSc);
        
        Vector2 l_posSc;
        WorldToScreen(ZL, &l_posSc);
        
        Vector2 MonPos = {mon_posSc.X, mon_posSc.Y};
        if (MonPos.X < 0 || MonPos.X > abs_ScreenX || MonPos.Y < 0 || MonPos.Y > abs_ScreenY) {
            continue;
        }     
        if (type == 5) {           
           if (mHeroID == 2002 && Health < maxHealth) {
               std::string s = "LORD UNDER ATK!";
               std::string h;
               h += "Health: "+ std::to_string((int)Health);
               Draw->AddText(nullptr,22.5f, ImVec2(abs_ScreenX / 2 - 70.f, 30), ImColor(248,248,255), s.c_str());
               Draw->AddText(nullptr,22.5f, ImVec2(abs_ScreenX / 2 - 70.f, 50), ImColor(248,248,255), h.c_str());               
           }
        
           if (mHeroID == 2003 && Health < maxHealth) {
               std::string s = "TURTLE UNDER ATK!";
               std::string h;
               h += "Health: "+ std::to_string((int)Health);

               Draw->AddText(nullptr,22.5f, ImVec2(abs_ScreenX / 2 - 70.f, 30), ImColor(248,248,255), s.c_str());
               Draw->AddText(nullptr,22.5f, ImVec2(abs_ScreenX / 2 - 70.f, 50), ImColor(248,248,255), h.c_str());               
           }
        }
        if (type == 1) {
            std::string sL = "MINION";
            auto textSize1 = ImGui::CalcTextSize(sL.c_str(), 0, 29); 
            绘制字体描边(22.5,MonPos.X - (textSize1.x / 2), MonPos.Y,ImColor(248,248,255),sL.c_str());
        }
        if (type == 2) {
            if (!bMonster(mHeroID)) continue;               
           
             std::string monsterName = MonsterToString(mHeroID);
             if (monsterName.empty()) continue;             
           
             bool isImportantMonster = (mHeroID == 2002 || mHeroID == 2220 || mHeroID == 2003 || mHeroID == 2221 || 
                                          mHeroID == 2004 || mHeroID == 2005 || mHeroID == 2222 || mHeroID == 2223);
           
             bool isEventMonster = (mHeroID >= 2220 && mHeroID <= 2232);
             ImColor nameColor = isEventMonster ? IM_COL32(255, 215, 0, 255) : IM_COL32(220, 180, 255, 255);     
           
             auto DistanceM = Vector3::Distance(ZL, Dm);         
         
             std::string strName = monsterName;
             if (isEventMonster) {
                 strName = "[EVENT] " + monsterName;
             } 
             
             auto textSize1 = ImGui::CalcTextSize(strName.c_str(), 0, 29); 
             绘制字体描边(22.5,MonPos.X - (textSize1.x / 2), MonPos.Y + 20,nameColor,strName.c_str());
             
             std::string sm;     
             sm += std::to_string((int)DistanceM);
             sm += "m | ";
             sm += "Health: "+ std::to_string((int)Health);      
             auto textSize11 = ImGui::CalcTextSize(sm.c_str(), 0, 29);    
             绘制字体描边(22.5,MonPos.X - (textSize11.x / 2), MonPos.Y,nameColor,sm.c_str());
        }
    }
}

void Layout_tick_UI() {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::SetNextWindowSizeConstraints(ImVec2(800, 0), ImVec2(820, FLT_MAX));

    ImGui::Begin(oxorany("                  MLBB EXTERNAL - STARCOOL"), nullptr, window_flags);

    if (ImGui::BeginTabBar("####")) {

        if (ImGui::BeginTabItem(oxorany("ESP"))) {
            ImGui::Checkbox(oxorany("Line"), &drawMHealth);
            ImGui::Checkbox(oxorany("IconHero"), &iconhero);
            ImGui::Checkbox(oxorany("Distance & Hero Name"), &drawMDistance);
            ImGui::Checkbox(oxorany("Alert Lord Under Attack"), &drawAlertUnderAttack);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(oxorany("Settings"))) {
            static int theme = 0;
            const char* themes[] = { "Dark", "Light", "Classic" };
            if (ImGui::Combo(oxorany("Theme Gui"), &theme, themes, IM_ARRAYSIZE(themes))) {
                if (theme == 0) ImGui::StyleColorsDark();
                if (theme == 1) ImGui::StyleColorsLight();
                if (theme == 2) ImGui::StyleColorsClassic();
            }
            static float opacity = 1.0f;
            ImGui::SliderFloat(oxorany("UI Opacity"), &opacity, 0.1f, 1.0f);
            ImGui::GetStyle().Alpha = opacity;
            ImGui::Text(oxorany("Current FPS: %.1f"), ImGui::GetIO().Framerate);
            if (ImGui::Button(oxorany("Exit Cheat"))) {
                main_thread_flag = false;
            }
            if (ImGui::Button(oxorany("Unload Cheat"))) {
                exit(0);
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    DrawMonster(ImGui::GetForegroundDrawList());
    g_window = ImGui::GetCurrentWindow();
    ImGui::End();
}

__attribute__((visibility("default"))) int main(int argc, char *argv[]) {
    pid = pidof(oxorany("com.mobile.legends:UnityKillsMe"));
    g_pid = pid;
    libbase = GetBase(oxorany("libcsharp.so"));
    printf("Lib: %p \n", libbase);
    screen_config();
    ::abs_ScreenX = (displayInfo.height > displayInfo.width ? displayInfo.height : displayInfo.width);
    ::abs_ScreenY = (displayInfo.height < displayInfo.width ? displayInfo.height : displayInfo.width);
    ::native_window_screen_x = (displayInfo.height > displayInfo.width ? displayInfo.height : displayInfo.width);
    ::native_window_screen_y = (displayInfo.height > displayInfo.width ? displayInfo.height : displayInfo.width);
    if (!initGUI_draw(native_window_screen_x, native_window_screen_y, true)) {
        return -1;
    }
    Touch_Init(displayInfo.width, displayInfo.height, displayInfo.orientation, false);
    ImGui::GetStyle().WindowRounding = 25.0f;
    while (main_thread_flag) {
        drawBegin();
        Layout_tick_UI();
        drawEnd();
        usleep(1000);
    }
    shutdown();
    Touch_Close();
    return 0;
}
