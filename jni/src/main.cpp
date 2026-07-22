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

bool lastRetriTriggered[20] = {false};
bool autoRetribution = false;
bool AutoRetributionRed = false;
bool AutoRetributionBlue = false;
bool AutoRetributionLord = false;
bool AutoRetributionTurtle = false;
bool AutoRetributionCrab = false;
bool AutoRetributionLito = false;        

float retriTouchX = 1575;
float retriTouchY = 661;

void DrawMonster(ImDrawList *Draw) {
    if (autoRetribution) {
        ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(retriTouchX, retriTouchY), 18.0f, IM_COL32(255, 255, 255, 180), 16);
        ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(retriTouchX, retriTouchY), 18.0f, IM_COL32(0, 0, 0, 255), 16, 2.5f);
    }
    if (abs_ScreenX < abs_ScreenY) return;
    
    float lineSize = abs_ScreenY / 432;
    long a1 = getPtr641(libbase + 0x7641e18); //dari sc gg
    long a2 = getPtr641((a1 + ((0x100 | 0xB8) & 0xFF)));
    long a32 = getPtr641((a2 << 1) >> 1);

    /**
    class BattleManager
    perlu update dari dump.cs*
    **/
    size_t m_LocalPlayerShow = 0x50; //m_LocalPlayerShow
    size_t m_ShowPlayers = 0x78; //m_ShowPlayers
    size_t m_ShowMonsters = 0x80; //m_ShowMonsters
    
    /**
    class ShowEntity 
    perlu update dari dump.cs*
    **/
    size_t m_iType = 0x80; //m_iType

    size_t m_Hp = 0x1ac; //m_Hp
    size_t m_HpMax = 0x1b0; //m_HpMax
    size_t m_bDeath = 0xcd; //m_bDeath
    size_t m_bSameCampType = 0x2b1; //m_bSameCampType
    size_t m_vCachePosition = 0x294; //m_vCachePosition
    /**
    class ShowPlayer 
    perlu update dari dump.cs*
    **/
    size_t m_HeroName = 0x8d8; //m_HeroName
    
    long selfp = getPtr641(a32 + m_LocalPlayerShow); // m_LocalPlayerShow;
    
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
    long monster = getPtr641(getPtr641(a32+m_ShowMonsters)+0x10)+0x20; // 0x70 m_ShowPlayer
    uint stop_monster = Read<uint>(getPtr641(a32+m_ShowMonsters)+0x18); // 0x70 m_ShowPlayer
    
    for (int i = 0; i < stop_monster; i++) {
        auto Objaddr = getPtr641(monster + ((i << 3) / 1));

        if ((Objaddr ^ 0x0) == 0x0) {
            continue;
        }

        auto is_team = Read<bool>(Objaddr + m_bSameCampType); // m_bSameCampType
        if (is_team) {
            continue;
        }

        auto mHeroID = Read<int>(Objaddr + 0x194);        
        auto type = Read<int>(Objaddr + m_iType);
        
        auto death = Read<bool>(Objaddr + m_bDeath); // m_bDeath
        if (death) {
            continue;
        }

        int Health = Read<uint64_t>(Objaddr + m_Hp); // m_hP
        if(Health <= 0)
        {
            continue;
        }
        
        int maxHealth = Read<uint64_t>(Objaddr + m_HpMax); // m_MaxHp
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
             //auto bShowEntityLayer = Read<bool>(Objaddr + 0x32c);               
             bool isImportantMonster = (mHeroID == 2002 || mHeroID == 2220 || mHeroID == 2003 || mHeroID == 2221 || 
                                          mHeroID == 2004 || mHeroID == 2005 || mHeroID == 2222 || mHeroID == 2223);
                
             //bool shouldShow = !bShowEntityLayer || isImportantMonster;               
             //if (!shouldShow) continue;            
           
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

struct MonsterData {
    uintptr_t address;
    Vector3 position;
    float distance;
    int health;
    int maxHP;
    bool isDead;
    bool isVisible;
    bool isValid;
    char name[100];
};

MonsterData monster[20];

int MonsterCount = 0;

uintptr_t Oneself;

void MonsterRetribution() {
    uintptr_t BattleManager = getPtr641(libbase + 0x7641e18);
    BattleManager = getPtr641(BattleManager + 0xB8);
    BattleManager = getPtr641(BattleManager);

    if(!BattleManager) return;
    Oneself = getPtr641(BattleManager + 0x50);
    if(!Oneself) return;

    Vector3 MyPosition;
    vm_readv(Oneself + 0x294, &MyPosition, sizeof(MyPosition));
    
    MonsterCount = 0;
    uintptr_t Showmonster = getPtr641(BattleManager + 0x80);
    if (Showmonster != 0) {
        int monsterCount = Read<int>(Showmonster + 0x18);
        uintptr_t monsterDataPtr = ReadPtr(Showmonster + 0x10);
        if (monsterCount >= 0 && monsterCount <= 100 && monsterDataPtr != 0) {
            uintptr_t monsterDataArray = monsterDataPtr + 0x20;
            int monsterfound = 0;
            for (int i = 0; i < monsterCount && monsterfound < 20; i++) {
                uintptr_t currentMonsterPtr = ReadPtr(monsterDataArray + (i * 8));
                if (currentMonsterPtr == 0) continue;
                int monsterID = Read<int>(currentMonsterPtr + 0x194);
                int monsterHP = Read<int>(currentMonsterPtr + 0x1ac);
                int monsterMaxHP = Read<int>(currentMonsterPtr + 0x1b0);
                Vector3 monsterPos = Read<Vector3>(currentMonsterPtr + 0x294);
                uint8_t deadFlag = Read<uint8_t>(currentMonsterPtr + 0xcd);
                bool mDead = (deadFlag != 0);
                std::string mName = MonsterToString(monsterID);
                if (mName.empty()) {
                    if (monsterID == 2002) mName = "Lord";
                    else if (monsterID == 2003) mName = "Turtle";
                    else continue;
                }
                monster[monsterfound].address   = currentMonsterPtr;
                monster[monsterfound].position  = monsterPos;
                monster[monsterfound].distance  = Vector3::Distance(MyPosition, monsterPos);
                monster[monsterfound].health    = monsterHP;
                monster[monsterfound].maxHP     = monsterMaxHP;
                monster[monsterfound].isDead    = mDead;
                monster[monsterfound].isVisible = true;
                monster[monsterfound].isValid   = true;
                strncpy(monster[monsterfound].name, mName.c_str(), sizeof(monster[monsterfound].name) - 1);
                monster[monsterfound].name[sizeof(monster[monsterfound].name) - 1] = '\0';
                monsterfound++;
            }
            MonsterCount = monsterfound;
        }
    }
}

int CalculateRetriDamage(int Level, int KillWild) {
    if (KillWild < 5) {
        return 600 + (Level - 1) * 80;
    } else {
        return (600 + (Level - 1) * 80) + (300 + (Level - 1) * 40);
    }
}

void CheckAndTriggerRetribution() {
    if (!autoRetribution && !Oneself && MonsterCount <= 0) return;
    int myLevel = Read<int>(Oneself + 0xA60);
    int killWild = Read<int>(Oneself + 0x198);
    int retriDmg = CalculateRetriDamage(myLevel, killWild);
    for (int i = 0; i < MonsterCount; i++) {
        if (!monster[i].isValid || monster[i].isDead) {
            lastRetriTriggered[i] = false;
            continue;
        }
        if (monster[i].distance > 5.0f) {
            lastRetriTriggered[i] = false;
            continue;
        }
        int id = Read<int>(monster[i].address + 0x194);
        bool isTarget = false;
        if (AutoRetributionLord && (id == 2002)) isTarget = true;
        if (AutoRetributionTurtle && (id == 2003)) isTarget = true;
        if (AutoRetributionBlue && (id == 2005)) isTarget = true;
        if (AutoRetributionLito && (id == 2056)) isTarget = true;
        if (AutoRetributionCrab && (id == 2005)) isTarget = true;
        if (AutoRetributionRed && (id == 2004)) isTarget = true;        
        if (!isTarget) {
            lastRetriTriggered[i] = false;
            continue;
        }
        if (monster[i].health <= retriDmg) {
            if (!lastRetriTriggered[i]) {
                Touch_Tap(retriTouchX, retriTouchY);
                lastRetriTriggered[i] = true;
            }
        } else {
            lastRetriTriggered[i] = false;
        }
    }
}

void RoomInfoList() {
    uintptr_t LogicBattleManager = getPtr641(libbase + 0x664d190);
    if (!LogicBattleManager) return;

    long playersList = getPtr641(getPtr641((uintptr_t)LogicBattleManager + 0x78) + 0x10);
    int playerCount = Read<int>(getPtr641((uintptr_t)LogicBattleManager + 0x78) + 0x18);
    if (playerCount <= 0 || !playersList) return;

    long a1 = getPtr641(libbase + 0x664d190);
    long a2 = getPtr641((a1 + ((0x100 | 0xB8) & 0xFF)));
    long a32 = getPtr641((a2 << 1) >> 1);

    long selfp = getPtr641(a32 + 0x50);

    if (!selfp) return;

    uint32_t myTeamCamp = Read<uint32_t>(selfp + 0x30);

    int playerB = 0;
    int playerR = 0;

    for (int i = 0; i < playerCount; i++) {
        long obj = getPtr641(playersList + i * 8);
        if (!obj) continue;

        auto nameObj = *(String**)(obj + 0x40);
        std::string name = (nameObj && nameObj->CString()) ? nameObj->CString() : "Unknown";

        uint64_t lUid = Read<uint64_t>(obj + 0x20);
        uint32_t zoneId = Read<uint32_t>(obj + 0x60);
        std::string uid = std::to_string(lUid) + " (" + std::to_string(zoneId) + ")";

        uint32_t heroid = Read<uint32_t>(obj + 0x4C);
        int spellId = Read<int>(obj + 0x64);
        uint32_t rank = Read<uint32_t>(obj + 0x128);
        uint32_t myth = Read<uint32_t>(obj + 0x1CC);
        uint32_t camp = Read<uint32_t>(obj + 0x30);

        std::string hero = HeroToString(heroid);
        std::string spell = SpellToString(spellId);
        std::string rankStr = RankToString(rank, myth);

        if (camp == myTeamCamp && playerB < 5) {
            PlayerB[playerB++] = { name, uid, hero, spell, rankStr };
        } else if (playerR < 5) {
            PlayerR[playerR++] = { name, uid, hero, spell, rankStr };
        }
    }
}

int MinimapSize = 342;
int MinimapPos = 76;
bool MinimapIcon = true;
bool HideLine = false;

float g_MinimapScale = 74.11f;
float g_Res0_MultX = 1.0f;
float g_Res0_MultY = 1.0f;
float g_Res1_OffsetX = 0.0f;
float g_Res1_OffsetY = 0.0f;
int g_ICSize = 38;

Vector2 WorldToMinimap(Vector3 HeroPosition) {
    float angle = 314.60f * 0.017453292519943295f;
    float angleCos = std::cos(angle);
    float angleSin = std::sin(angle);

    Vector2 Res0;
    Res0.X = ((angleCos * HeroPosition.X - angleSin * (-HeroPosition.Z)) / g_MinimapScale) * g_Res0_MultX;
    Res0.Y = ((angleSin * HeroPosition.Y + angleCos * (-HeroPosition.Z)) / g_MinimapScale) * g_Res0_MultY;

    Vector2 Res1;
    Res1.X = (Res0.X * MinimapSize) + MinimapPos + MinimapSize / 2.0f + g_Res1_OffsetX;
    Res1.Y = (Res0.Y * MinimapSize) + MinimapSize / 2.0f + g_Res1_OffsetY;

    return Res1;
}

void DrawMinimapESP(ImDrawList* draw) {
    if (!MinimapIcon) return;

    long a1 = getPtr641(libbase + 0x7641e18);
    if (!a1) return;

    long a2 = getPtr641(a1 + 0xB8);
    if (!a2) return;

    long a32 = getPtr641(a2);
    if (!a32) return;

    size_t m_ShowPlayers     = 0x78;
    size_t m_bSameCampType   = 0x2b1;
    size_t m_bDeath          = 0xcd;
    size_t m_vCachePosition  = 0x294;

    long showList = getPtr641(a32 + m_ShowPlayers);
    if (!showList) return;

    long playerList = getPtr641(showList + 0x10);
    if (!playerList) return;
    playerList += 0x20;

    uint playerCount = Read<uint>(showList + 0x18);
    for (int i = 0; i < playerCount; i++) {
        long Objaddr = getPtr641(playerList + (i << 3));
        if (!Objaddr) continue;

        if (Read<bool>(Objaddr + m_bSameCampType)) continue;
        if (Read<bool>(Objaddr + m_bDeath)) continue;

        Vector3A pos{};
        vm_readv(Objaddr + m_vCachePosition, &pos, sizeof(pos));
        if (pos.X == 0 && pos.Y == 0 && pos.Z == 0) continue;

        Vector2 minimapPos = WorldToMinimap({ pos.X, pos.Y, pos.Z });
        draw->AddCircleFilled(ImVec2(minimapPos.X, minimapPos.Y), g_ICSize / 2.0f, IM_COL32(255, 0, 0, 255));
    }

    if (!HideLine) {
        draw->AddRect(
            ImVec2(MinimapPos, 0),
            ImVec2(MinimapPos + MinimapSize, MinimapSize),
            IM_COL32(255, 255, 255, 255)
        );
    }
}

void Layout_tick_UI() {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    
    // State untuk expand/collapse dan drag to resize
    static bool isExpanded = true;
    static ImVec2 expandedSize = ImVec2(800, 500);
    static ImVec2 collapsedSize = ImVec2(250, 50);
    static bool isResizing = false;
    static ImVec2 resizeStartSize;
    static ImVec2 resizeStartPos;
    
    // Set ukuran window berdasarkan state
    if (isExpanded) {
        ImGui::SetNextWindowSizeConstraints(ImVec2(400, 300), ImVec2(1200, FLT_MAX));
        ImGui::SetNextWindowSize(expandedSize, ImGuiCond_FirstUseEver);
    } else {
        ImGui::SetNextWindowSize(collapsedSize, ImGuiCond_Always);
    }

    ImGui::Begin(oxorany("ESP Controller"), nullptr, window_flags);
    
    // Dapatkan window properties
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 bottomRight = ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y);
    
    // Resize handle (corner kanan bawah)
    float resizeHandleSize = 20.0f;
    ImVec2 resizeHandleMin = ImVec2(bottomRight.x - resizeHandleSize, bottomRight.y - resizeHandleSize);
    ImVec2 resizeHandleMax = bottomRight;
    
    // Draw resize handle indicator
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(resizeHandleMin, resizeHandleMax, IM_COL32(100, 100, 100, 200));
    drawList->AddRect(resizeHandleMin, resizeHandleMax, IM_COL32(200, 200, 200, 255), 0, 0, 1.0f);
    
    // Check for resize drag
    ImVec2 mousePos = ImGui::GetMousePos();
    if (ImGui::IsMouseHoveringRect(resizeHandleMin, resizeHandleMax)) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
        if (ImGui::IsMouseDown(0)) {
            isResizing = true;
            resizeStartSize = windowSize;
            resizeStartPos = mousePos;
        }
    }
    
    // Handle resize drag
    if (isResizing && ImGui::IsMouseDown(0)) {
        ImVec2 delta = ImVec2(mousePos.x - resizeStartPos.x, mousePos.y - resizeStartPos.y);
        ImVec2 newSize = ImVec2(
            std::max(400.0f, resizeStartSize.x + delta.x),
            std::max(300.0f, resizeStartSize.y + delta.y)
        );
        
        if (isExpanded) {
            expandedSize = newSize;
        } else {
            collapsedSize = newSize;
        }
    } else if (!ImGui::IsMouseDown(0)) {
        isResizing = false;
    }
    
    // Button Expand/Collapse di top-right
    ImVec2 availableSpace = ImGui::GetContentRegionAvail();
    float buttonWidth = 80.0f;
    ImGui::SetCursorPosX(availableSpace.x - buttonWidth - 10.0f);
    
    if (ImGui::Button(isExpanded ? oxorany("Collapse") : oxorany("Expand"), ImVec2(buttonWidth, 0))) {
        isExpanded = !isExpanded;
        if (isExpanded) {
            expandedSize = ImGui::GetWindowSize();
        } else {
            collapsedSize = ImGui::GetWindowSize();
        }
    }

    // Garis pembatas
    ImGui::Separator();

    if (isExpanded) {
        // TAB BAR untuk ESP, Room Info, dan Settings
        if (ImGui::BeginTabBar("####MainTabs")) {

            // ====== TAB ESP ======
            if (ImGui::BeginTabItem(oxorany("ESP"))) {
                ImGui::Spacing();
                ImGui::Text(oxorany("ESP Options:"));
                ImGui::Separator();
                
                ImGui::Checkbox(oxorany("Line to Enemy"), &drawMHealth);
                ImGui::SameLine();
                ImGui::HelpMarker(oxorany("Draw line dari player ke enemy"));
                
                ImGui::Checkbox(oxorany("Hero Icon"), &iconhero);
                ImGui::SameLine();
                ImGui::HelpMarker(oxorany("Tampilkan icon hero di atas enemy"));
                
                ImGui::Checkbox(oxorany("Distance & Hero Name"), &drawMDistance);
                ImGui::SameLine();
                ImGui::HelpMarker(oxorany("Tampilkan jarak dan nama hero"));
                
                ImGui::Checkbox(oxorany("Alert Lord Under Attack"), &drawAlertUnderAttack);
                ImGui::SameLine();
                ImGui::HelpMarker(oxorany("Alert ketika Lord/Turtle di serang"));
                
                ImGui::Spacing();
                ImGui::Text(oxorany("Current FPS: %.1f"), ImGui::GetIO().Framerate);
                
                ImGui::EndTabItem();
            }

            // ====== TAB ROOM INFO ======
            if (ImGui::BeginTabItem(oxorany("Room Info"))) {
                ImGui::Spacing();
                
                if (ImGui::CollapsingHeader(oxorany("Blue Team"), ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Indent();
                    if (PlayerB[0].Name.empty()) {
                        ImGui::Text(oxorany("No data yet"));
                    } else {
                        for (int i = 0; i < 5; i++) {
                            if (PlayerB[i].Name.empty()) break;
                            
                            ImGui::Separator();
                            ImGui::Text(oxorany("Player %d"), i + 1);
                            ImGui::Text(oxorany("Name: %s"), PlayerB[i].Name.c_str());
                            ImGui::Text(oxorany("Hero: %s"), PlayerB[i].Hero.c_str());
                            ImGui::Text(oxorany("Spell: %s"), PlayerB[i].Spell.c_str());
                            ImGui::Text(oxorany("Rank: %s"), PlayerB[i].Rank.c_str());
                        }
                    }
                    ImGui::Unindent();
                }
                
                ImGui::Spacing();
                
                if (ImGui::CollapsingHeader(oxorany("Red Team"), ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Indent();
                    if (PlayerR[0].Name.empty()) {
                        ImGui::Text(oxorany("No data yet"));
                    } else {
                        for (int i = 0; i < 5; i++) {
                            if (PlayerR[i].Name.empty()) break;
                            
                            ImGui::Separator();
                            ImGui::Text(oxorany("Player %d"), i + 1);
                            ImGui::Text(oxorany("Name: %s"), PlayerR[i].Name.c_str());
                            ImGui::Text(oxorany("Hero: %s"), PlayerR[i].Hero.c_str());
                            ImGui::Text(oxorany("Spell: %s"), PlayerR[i].Spell.c_str());
                            ImGui::Text(oxorany("Rank: %s"), PlayerR[i].Rank.c_str());
                        }
                    }
                    ImGui::Unindent();
                }
                
                ImGui::EndTabItem();
            }

            // ====== TAB SETTINGS ======
            if (ImGui::BeginTabItem(oxorany("Settings"))) {
                ImGui::Spacing();
                ImGui::Text(oxorany("UI Settings:"));
                ImGui::Separator();
                
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
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Text(oxorany("Actions:"));
                
                if (ImGui::Button(oxorany("Exit Cheat"), ImVec2(-1, 30))) {
                    main_thread_flag = false;
                }
                
                if (ImGui::Button(oxorany("Unload Cheat"), ImVec2(-1, 30))) {
                    exit(0);
                }
                
                ImGui::Text(oxorany("Drag corner kanan bawah untuk resize UI"));
                
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    } else {
        // Mode collapsed - hanya tampilkan toggle minimal
        ImGui::Text(oxorany("ESP: %s"), drawMDistance ? "ON" : "OFF");
        if (ImGui::IsWindowHovered() && ImGui::IsMouseDoubleClicked(0)) {
            isExpanded = !isExpanded;
        }
    }

    // Draw overlay (minimap dan monster) tetap jalan
    if (MinimapIcon) DrawMinimapESP(ImGui::GetForegroundDrawList());
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
        MonsterRetribution();
        CheckAndTriggerRetribution();
        RoomInfoList();
        drawBegin();
        Layout_tick_UI();
        drawEnd();
        usleep(1000);
    }
    shutdown();
    Touch_Close();
    return 0;
}
