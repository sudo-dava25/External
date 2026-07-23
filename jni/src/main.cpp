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
bool iconhero = true;
bool drawESPBox = false;
bool drawHealthBar = true;
bool drawDistance = true;
bool drawHealth = true;
bool drawHeroName = true;

// ESP Monster Controls
bool enableESPMonster = false;
bool drawMonsterName = true;
bool drawMonsterDistance = true;
bool drawMonsterHealth = true;
bool drawMonsterAlert = true;

long libbase = 0;

bool lastRetriTriggered[20] = {false};
bool autoRetribution = false;
bool AutoRetributionRed = false;
bool AutoRetributionBlue = false;
bool AutoRetributionLord = false;
bool AutoRetributionTurtle = false;

float retriTouchX = 1575.0f;
float retriTouchY = 661.0f;

std::string fshy(uintptr_t address)
{
    if (!address) return "";

    auto stringLength = Read<uint32_t>(address + 0x10);
    char16_t buffer[255] = { 0 };

    pvm(reinterpret_cast<void *>(address + 0x14), reinterpret_cast<void *>(buffer), static_cast<size_t>(stringLength) * 2, false);

    return utf16_to_utf8(buffer, stringLength);
}



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
        2002, 2003, 2004, 2005, 2006, 2008, 2009, 2011, 2012, 2013,
        2056, 2059, 2072, 2220, 2221, 2222, 2223, 2224, 2225, 2226,
        2227, 2228, 2229, 2230, 2232,
};

bool bMonster(int iValue) {
    return std::find(std::begin(ListMonsterId), std::end(ListMonsterId), iValue) != std::end(ListMonsterId);
}

void Touch_Tap(int x, int y) {
     Touch_Down((float)x, (float)y);
     usleep(30000);  // Reduced from 80ms to 30ms for faster response
     Touch_Up();
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
    return 750 + (Level * 150);
}

void CheckAndTriggerRetribution() {
    if (!autoRetribution || !Oneself || MonsterCount <= 0) return;
    int myLevel = Read<int>(Oneself + 0x198);
    int killWild = Read<int>(Oneself + 0xa38);
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
        if (AutoRetributionRed && (id == 2004)) isTarget = true;        
        if (!isTarget) {
            lastRetriTriggered[i] = false;
            continue;
        }
        if (monster[i].health <= retriDmg) {
            if (!lastRetriTriggered[i]) {
                Touch_Tap((int)retriTouchX, (int)retriTouchY);
                lastRetriTriggered[i] = true;
            }
        } else {
            lastRetriTriggered[i] = false;
        }
    }
}

void DrawMonster(ImDrawList *Draw) {
    if (autoRetribution) {
        ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(retriTouchX, retriTouchY), 18.0f, IM_COL32(255, 255, 255, 180), 16);
        ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(retriTouchX, retriTouchY), 18.0f, IM_COL32(0, 0, 0, 255), 16, 2.5f);
    }

    if (abs_ScreenX < abs_ScreenY) return;
    
    float lineSize = abs_ScreenY / 432;
    long a1 = getPtr641(libbase + 0x7641e18);
    long a2 = getPtr641((a1 + ((0x100 | 0xB8) & 0xFF)));
    long a32 = getPtr641((a2 << 1) >> 1);

    size_t m_LocalPlayerShow = 0x50;
    size_t m_ShowPlayers = 0x78;
    size_t m_ShowMonsters = 0x80;
    
    size_t m_iType = 0x80;
    size_t m_Hp = 0x1ac;
    size_t m_HpMax = 0x1b0;
    size_t m_bDeath = 0xcd;
    size_t m_bSameCampType = 0x2b1;
    size_t m_vCachePosition = 0x294;
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
        if(Health <= 0) {
            continue;
        }

        int maxHealth = Read<uint64_t>(Objaddr + m_HpMax);
        if(Health <= 0) {
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
        float IconSize = abs_ScreenX / 80.0f;
        Vector2 HeroPos = {en_posSc.X, en_posSc.Y - 20.0f};
        Vector2 Res;
        
        if (HeroPos.X < 0 || HeroPos.X > abs_ScreenX || HeroPos.Y < 0 ||
            HeroPos.Y > abs_ScreenY) {
            isOutScreen = true;
            IconSize = abs_ScreenX / 120.0f;
            FindPoint(HeroPos, Res, abs_ScreenX, abs_ScreenY, (IconSize / 3));
        } else {
            isOutScreen = false;
            Res = HeroPos;
        }
    
        auto Distance = Vector3::Distance(Z, D);
    
        if (drawMHealth) {
            ImGui::GetForegroundDrawList()->AddLine({loc_posSc.X,loc_posSc.Y}, {en_posSc.X, en_posSc.Y}, ImColor(255, 255, 255), lineSize);
        }
        
        if (drawESPBox) {
            float boxWidth = 125.0f;
            float boxHeight = 200.0f;
            
            ImVec2 boxMin = ImVec2(en_posSc.X - boxWidth / 2, en_posSc.Y - boxHeight / 2 - 50);
            ImVec2 boxMax = ImVec2(en_posSc.X + boxWidth / 2, en_posSc.Y + boxHeight / 2 - 50);
            
            ImColor boxColor = is_team ? ImColor(0, 255, 0) : ImColor(255, 0, 0);
            ImGui::GetForegroundDrawList()->AddRect(boxMin, boxMax, boxColor, 0, 0, 2.0f);
            
            if (drawHealthBar) {
                float barWidth = 6.0f;
                float healthPercent = (float)Health / (float)maxHealth;
                
                ImVec2 healthBarMin = ImVec2(boxMax.x + 3.0f, boxMin.y);
                ImVec2 healthBarMax = ImVec2(boxMax.x + 3.0f + barWidth, boxMax.y);
                
                ImGui::GetForegroundDrawList()->AddRectFilled(healthBarMin, healthBarMax, ImColor(0, 0, 0));
                
                ImVec2 healthFillMin = ImVec2(healthBarMin.x, boxMax.y - (boxHeight * healthPercent));
                ImVec2 healthFillMax = healthBarMax;
                
                ImColor healthColor;
                if (healthPercent > 0.5f) {
                    healthColor = ImColor(0, 255, 0);
                } else if (healthPercent > 0.25f) {
                    healthColor = ImColor(255, 255, 0);
                } else {
                    healthColor = ImColor(255, 0, 0);
                }
                ImGui::GetForegroundDrawList()->AddRectFilled(healthFillMin, healthFillMax, healthColor);
                ImGui::GetForegroundDrawList()->AddRect(healthBarMin, healthBarMax, ImColor(255, 255, 255), 0, 0, 1.0f);
            }
        }
        
        if (iconhero) {
            ImVec2 iconPos(HeroPos.X, HeroPos.Y);
            DrawHeroIcon(ImGui::GetBackgroundDrawList(), iconPos, HeroID, Health, maxHealth, 25.0f);
        }

        if (drawDistance || drawHealth || drawHeroName) {
            std::string s;
            int activeCount = 0;
            
            if (drawDistance) {
                s += std::to_string((int)Distance);
                s += "m";
                activeCount++;
            }
            
            if (drawHealth) {
                if (!s.empty()) s += " | ";
                s += "Health: "+ std::to_string((int)Health);
                activeCount++;
            }
            
            if (drawHeroName) {
                if (!s.empty()) s += " | ";
                s += fshy(Read<uintptr_t>(Objaddr + m_HeroName));
                activeCount++;
            }

            if (!s.empty()) {
                auto textSize1 = ImGui::CalcTextSize(s.c_str(), 0, 29);
                float offsetX = (activeCount == 1) ? 0.0f : 35.0f;
                绘制字体描边(23.5, HeroPos.X - (textSize1.x / 2) + offsetX, HeroPos.Y + 40.0f, ImColor(248,248,255), s.c_str());
            }
        }
    }

    // ESP MONSTER - Hanya tampil jika enableESPMonster = true
    if (enableESPMonster) {
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
            if(Health <= 0) {
                continue;
            }
            
            int maxHealth = Read<uint64_t>(Objaddr + m_HpMax);
            if(maxHealth <= 0) {
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
               if (drawMonsterAlert) {
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
               
                 bool isEventMonster = (mHeroID >= 2220 && mHeroID <= 2232);
                 ImColor nameColor = isEventMonster ? IM_COL32(255, 215, 0, 255) : IM_COL32(220, 180, 255, 255);     
               
                 auto DistanceM = Vector3::Distance(ZL, Dm);         
             
                 if (drawMonsterName) {
                     std::string strName = monsterName;
                     if (isEventMonster) {
                         strName = "[EVENT] " + monsterName;
                     } 
                     
                     auto textSize1 = ImGui::CalcTextSize(strName.c_str(), 0, 29); 
                     绘制字体描边(22.5,MonPos.X - (textSize1.x / 2), MonPos.Y + 20,nameColor,strName.c_str());
                 }
                 
                 if (drawMonsterDistance || drawMonsterHealth) {
                     std::string sm;
                     
                     if (drawMonsterDistance) {
                         sm += std::to_string((int)DistanceM);
                         sm += "m";
                     }
                     
                     if (drawMonsterHealth) {
                         if (!sm.empty()) sm += " | ";
                         sm += "Health: "+ std::to_string((int)Health);
                     }
                     
                     if (!sm.empty()) {
                         auto textSize11 = ImGui::CalcTextSize(sm.c_str(), 0, 29);    
                         绘制字体描边(22.5,MonPos.X - (textSize11.x / 2), MonPos.Y,nameColor,sm.c_str());
                     }
                 }
            }
        }
    }
}

void Layout_tick_UI() {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    
    static ImVec2 windowSize = ImVec2(600, 450);
    static bool isResizing = false;
    static ImVec2 resizeStartSize;
    static ImVec2 resizeStartPos;
    
    ImGui::SetNextWindowSizeConstraints(ImVec2(500, 350), ImVec2(700, 600));
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_FirstUseEver);

    ImGui::Begin(oxorany("VOLKS External @volksive"), nullptr, window_flags);
    
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 currentWindowSize = ImGui::GetWindowSize();
    ImVec2 bottomRight = ImVec2(windowPos.x + currentWindowSize.x, windowPos.y + currentWindowSize.y);
    
    float resizeHandleSize = 20.0f;
    ImVec2 resizeHandleMin = ImVec2(bottomRight.x - resizeHandleSize, bottomRight.y - resizeHandleSize);
    ImVec2 resizeHandleMax = bottomRight;
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(resizeHandleMin, resizeHandleMax, IM_COL32(100, 100, 100, 200));
    drawList->AddRect(resizeHandleMin, resizeHandleMax, IM_COL32(200, 200, 200, 255), 0, 0, 1.0f);
    
    ImVec2 mousePos = ImGui::GetMousePos();
    if (ImGui::IsMouseHoveringRect(resizeHandleMin, resizeHandleMax)) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
        if (ImGui::IsMouseDown(0)) {
            isResizing = true;
            resizeStartSize = currentWindowSize;
            resizeStartPos = mousePos;
        }
    }
    
    if (isResizing && ImGui::IsMouseDown(0)) {
        ImVec2 delta = ImVec2(mousePos.x - resizeStartPos.x, mousePos.y - resizeStartPos.y);
        ImVec2 newSize = ImVec2(
            std::max(500.0f, resizeStartSize.x + delta.x),
            std::max(350.0f, resizeStartSize.y + delta.y)
        );
        newSize.x = std::min(700.0f, newSize.x);
        newSize.y = std::min(600.0f, newSize.y);
        windowSize = newSize;
    } else if (!ImGui::IsMouseDown(0)) {
        isResizing = false;
    }

    ImGui::Separator();

    if (ImGui::BeginTabBar("####MainTabs")) {

        if (ImGui::BeginTabItem(oxorany("ESP"))) {
            ImGui::Spacing();
            ImGui::Text(oxorany("ESP Options:"));
            ImGui::Separator();
            
            ImGui::Checkbox(oxorany("Line"), &drawMHealth);
            ImGui::Checkbox(oxorany("Hero Icon"), &iconhero);
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text(oxorany("Info Display:"));
            ImGui::Checkbox(oxorany("Show Distance"), &drawDistance);
            ImGui::Checkbox(oxorany("Show Health"), &drawHealth);
            ImGui::Checkbox(oxorany("Show Hero Name"), &drawHeroName);
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text(oxorany("Additional ESP:"));
            ImGui::Checkbox(oxorany("ESP Box"), &drawESPBox);
            ImGui::Checkbox(oxorany("Health Bar"), &drawHealthBar);
            
            ImGui::Spacing();
            ImGui::Text(oxorany("Current FPS: %.1f"), ImGui::GetIO().Framerate);
            
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(oxorany("ESP Monster"))) {
            ImGui::Spacing();
            ImGui::Text(oxorany("Monster ESP Options:"));
            ImGui::Separator();
            
            ImGui::Checkbox(oxorany("Enable Monster ESP"), &enableESPMonster);
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text(oxorany("Display Settings:"));
            ImGui::Checkbox(oxorany("Show Monster Name"), &drawMonsterName);
            ImGui::Checkbox(oxorany("Show Monster Distance"), &drawMonsterDistance);
            ImGui::Checkbox(oxorany("Show Monster Health"), &drawMonsterHealth);
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text(oxorany("Alerts:"));
            ImGui::Checkbox(oxorany("Alert When Lord/Turtle Under Attack"), &drawMonsterAlert);
            
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(oxorany("Retribution"))) {
            ImGui::Spacing();
            ImGui::Text(oxorany("Auto Retribution:"));
            ImGui::Separator();
            
            ImGui::Checkbox(oxorany("Enable Auto Retri"), &autoRetribution);
            ImGui::SliderFloat(oxorany("Retri Touch X"), &retriTouchX, 0.0f, 3000.0f, "%.0f");
            ImGui::SliderFloat(oxorany("Retri Touch Y"), &retriTouchY, 0.0f, 1500.0f, "%.0f");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text(oxorany("Target Selection:"));
            ImGui::Checkbox(oxorany("Red Buff"), &AutoRetributionRed);
            ImGui::Checkbox(oxorany("Blue Buff"), &AutoRetributionBlue);
            ImGui::Checkbox(oxorany("Lord"), &AutoRetributionLord);
            ImGui::Checkbox(oxorany("Turtle"), &AutoRetributionTurtle);
            
            ImGui::EndTabItem();
        }

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
            ImGui::Spacing();
            
            if (ImGui::Button(oxorany("Unload Cheat"), ImVec2(-1, 45))) {
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
    ImGui::GetStyle().ScrollbarSize = 13.0f;
    while (main_thread_flag) {
        MonsterRetribution();
        CheckAndTriggerRetribution();
        drawBegin();
        Layout_tick_UI();
        drawEnd();
        usleep(500);  // Reduced from 1000us to 500us for faster polling
    }
    shutdown();
    Touch_Close();
    return 0;
}
