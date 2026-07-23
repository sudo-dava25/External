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
#include <sys/stat.h>
#include <openssl/md5.h>
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

// ==================== TRIAL SYSTEM ====================
#define TRIAL_FILE "/data/local/tmp/.volks_trial"
#define TRIAL_DURATION_DAYS 3
#define ENCRYPTION_KEY 0xAB

std::string GetDeviceHWID() {
    char prop_build[PROP_VALUE_MAX];
    char prop_serial[PROP_VALUE_MAX];
    char prop_model[PROP_VALUE_MAX];
    
    __system_property_get("ro.build.fingerprint", prop_build);
    __system_property_get("ro.serialno", prop_serial);
    __system_property_get("ro.product.model", prop_model);
    
    std::string combined = std::string(prop_build) + std::string(prop_serial) + std::string(prop_model);
    
    unsigned char hash[MD5_DIGEST_LENGTH];
    MD5((unsigned char*)combined.c_str(), combined.length(), hash);
    
    char hex[33];
    for(int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        sprintf(hex + (i * 2), "%02x", hash[i]);
    }
    hex[32] = '\0';
    
    return std::string(hex);
}

std::string XorEncrypt(std::string data, char key) {
    std::string result = data;
    for(size_t i = 0; i < data.length(); i++) {
        result[i] = data[i] ^ key;
    }
    return result;
}

bool CheckTrial() {
    std::string hwid = GetDeviceHWID();
    
    std::ifstream trialFile(TRIAL_FILE);
    if (!trialFile.good()) {
        return false;
    }
    
    std::string encryptedData;
    std::getline(trialFile, encryptedData);
    trialFile.close();
    
    std::string decrypted = XorEncrypt(encryptedData, ENCRYPTION_KEY);
    
    size_t separator = decrypted.find('|');
    if (separator == std::string::npos) {
        return false;
    }
    
    std::string savedHWID = decrypted.substr(0, separator);
    time_t expiryTime = std::stoll(decrypted.substr(separator + 1));
    
    if (savedHWID != hwid) {
        return false;
    }
    
    time_t currentTime = time(nullptr);
    if (currentTime > expiryTime) {
        remove(TRIAL_FILE);
        return false;
    }
    
    return true;
}

bool ActivateTrial(const std::string& activationKey) {
    const std::string SECRET_KEY = "VOLKS2024SECRET123";
    
    if (activationKey != SECRET_KEY) {
        return false;
    }
    
    std::string hwid = GetDeviceHWID();
    time_t expiryTime = time(nullptr) + (TRIAL_DURATION_DAYS * 24 * 60 * 60);
    
    std::string data = hwid + "|" + std::to_string(expiryTime);
    std::string encrypted = XorEncrypt(data, ENCRYPTION_KEY);
    
    std::ofstream trialFile(TRIAL_FILE);
    if (!trialFile.good()) {
        return false;
    }
    trialFile << encrypted;
    trialFile.close();
    
    chmod(TRIAL_FILE, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    
    return true;
}

std::string GenerateActivationKey(const std::string& userHWID, int durationDays) {
    const std::string SECRET_KEY = "VOLKS2024SECRET123";
    
    std::string keyBase = userHWID + SECRET_KEY + std::to_string(durationDays);
    
    unsigned char hash[MD5_DIGEST_LENGTH];
    MD5((unsigned char*)keyBase.c_str(), keyBase.length(), hash);
    
    char key[17];
    for(int i = 0; i < 8; i++) {
        sprintf(key + (i * 2), "%02x", hash[i]);
    }
    key[16] = '\0';
    
    std::string finalKey = "VOLKS-";
    finalKey += std::string(key).substr(0, 4) + "-";
    finalKey += std::string(key).substr(4, 4) + "-";
    finalKey += std::string(key).substr(8, 4) + "-";
    finalKey += std::string(key).substr(12, 4);
    
    for(auto& c : finalKey) {
        c = toupper(c);
    }
    
    return finalKey;
}

int GetTrialRemainingHours() {
    std::ifstream trialFile(TRIAL_FILE);
    if (!trialFile.good()) return 0;
    
    std::string encryptedData;
    std::getline(trialFile, encryptedData);
    trialFile.close();
    
    std::string decrypted = XorEncrypt(encryptedData, ENCRYPTION_KEY);
    size_t separator = decrypted.find('|');
    if (separator == std::string::npos) return 0;
    
    time_t expiryTime = std::stoll(decrypted.substr(separator + 1));
    time_t currentTime = time(nullptr);
    
    int remaining = (expiryTime - currentTime) / 3600;
    return remaining > 0 ? remaining : 0;
}

// ==================== VARIABLES ====================
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

bool drawMonsterName = true;
bool drawMonsterDistance = true;
bool drawMonsterHealth = true;
bool drawMonsterAlert = true;

long libbase = 0;

bool lastRetriTriggered[20] = {false};
bool autoRetribution = false;
bool AutoRetributionBuff = false;
bool AutoRetributionBoss = false;
bool AutoRetributionRed = false;
bool AutoRetributionBlue = false;
bool AutoRetributionLord = false;
bool AutoRetributionTurtle = false;

float retriTouchX = 1575.0f;
float retriTouchY = 661.0f;

// Floating Button State
bool showFloatingButton = true;
ImVec2 floatingButtonPos = ImVec2(100, 300);
ImVec2 floatingButtonSize = ImVec2(85, 85);
bool isDraggingFloating = false;
ImVec2 dragOffset = ImVec2(0, 0);
int floatingTargetMode = 0;

// Trial state
bool isTrialValid = false;
char activationKeyInput[256] = "";
bool showActivationPopup = true;

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
     usleep(20000);
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
    int killWild = Read<int>(Oneself + 0xA38);
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
        if (AutoRetributionBoss && (id == 2002 || id == 2003)) isTarget = true;
        if (AutoRetributionBuff && (id == 2004 || id == 2005)) isTarget = true;
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

void DrawFloatingRetriButton() {
    if (!showFloatingButton || !autoRetribution) return;
    
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    ImGuiIO& io = ImGui::GetIO();
    
    floatingButtonSize = ImVec2(85, 85);
    float cornerRadius = 12.0f;
    
    ImVec2 buttonMin = floatingButtonPos;
    ImVec2 buttonMax = ImVec2(floatingButtonPos.x + floatingButtonSize.x, 
                               floatingButtonPos.y + floatingButtonSize.y);
    
    ImVec2 buttonCenter = ImVec2(floatingButtonPos.x + floatingButtonSize.x / 2,
                                  floatingButtonPos.y + floatingButtonSize.y / 2);
    
    ImVec2 mousePos = io.MousePos;
    
    bool isHovering = (mousePos.x >= buttonMin.x - 10 && mousePos.x <= buttonMax.x + 10 &&
                       mousePos.y >= buttonMin.y - 10 && mousePos.y <= buttonMax.y + 10);
    
    static bool wasDragging = false;
    static ImVec2 dragStartPos;
    static float dragDistance = 0;
    
    if (isHovering && ImGui::IsMouseClicked(0)) {
        isDraggingFloating = false;
        wasDragging = false;
        dragStartPos = mousePos;
        dragDistance = 0;
    }
    
    if (ImGui::IsMouseDown(0) && isHovering) {
        ImVec2 delta = ImVec2(mousePos.x - dragStartPos.x, mousePos.y - dragStartPos.y);
        dragDistance = sqrt(delta.x * delta.x + delta.y * delta.y);
        
        if (dragDistance > 5.0f) {
            isDraggingFloating = true;
            wasDragging = true;
            
            floatingButtonPos.x = mousePos.x - floatingButtonSize.x / 2;
            floatingButtonPos.y = mousePos.y - floatingButtonSize.y / 2;
            
            if (floatingButtonPos.x < 0) floatingButtonPos.x = 0;
            if (floatingButtonPos.y < 0) floatingButtonPos.y = 0;
            if (floatingButtonPos.x > abs_ScreenX - floatingButtonSize.x) 
                floatingButtonPos.x = abs_ScreenX - floatingButtonSize.x;
            if (floatingButtonPos.y > abs_ScreenY - floatingButtonSize.y) 
                floatingButtonPos.y = abs_ScreenY - floatingButtonSize.y;
        }
    }
    
    if (ImGui::IsMouseReleased(0) && isHovering && !wasDragging && dragDistance <= 5.0f) {
        floatingTargetMode = (floatingTargetMode == 0) ? 1 : 0;
        
        if (floatingTargetMode == 0) {
            AutoRetributionBuff = true;
            AutoRetributionBoss = false;
        } else {
            AutoRetributionBuff = false;
            AutoRetributionBoss = true;
        }
    }
    
    if (!ImGui::IsMouseDown(0)) {
        isDraggingFloating = false;
        wasDragging = false;
    }
    
    ImColor bgColor, borderColor;
    const char* label;
    
    if (floatingTargetMode == 0) {
        bgColor = IM_COL32(255, 140, 0, 230);
        borderColor = IM_COL32(255, 180, 50, 255);
        label = "BUFF";
    } else {
        bgColor = IM_COL32(220, 30, 30, 230);
        borderColor = IM_COL32(255, 80, 80, 255);
        label = "BOSS";
    }
    
    if (isHovering) {
        bgColor.Value.w = 255;
        if (floatingTargetMode == 0) {
            bgColor = IM_COL32(255, 160, 20, 255);
        } else {
            bgColor = IM_COL32(255, 40, 40, 255);
        }
    }
    
    draw->AddRectFilled(buttonMin, buttonMax, bgColor, cornerRadius);
    draw->AddRect(buttonMin, buttonMax, borderColor, cornerRadius, 0, 2.5f);
    
    ImVec2 textSize = ImGui::CalcTextSize(label);
    ImVec2 textPos = ImVec2(buttonCenter.x - textSize.x / 2,
                             buttonCenter.y - textSize.y / 2);
    
    draw->AddText(ImVec2(textPos.x + 1, textPos.y + 1), IM_COL32(0, 0, 0, 100), label);
    draw->AddText(textPos, IM_COL32(255, 255, 255, 255), label);
}

void DrawActivationPopup() {
    if (!showActivationPopup) return;
    
    ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(abs_ScreenX/2 - 200, abs_ScreenY/2 - 125), ImGuiCond_Always);
    
    ImGui::Begin("Trial Activation", &showActivationPopup, 
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | 
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 0.84f, 0.0f, 1.0f), "VOLKS MLBB External Cheat");
    ImGui::Separator();
    ImGui::Spacing();
    
    std::string hwid = GetDeviceHWID();
    ImGui::Text("Your HWID:");
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "%s", hwid.c_str());
    
    ImGui::Spacing();
    ImGui::Text("Activation Key:");
    ImGui::InputText("##actkey", activationKeyInput, 256);
    
    ImGui::Spacing();
    
    if (ImGui::Button("Activate", ImVec2(-1, 35))) {
        if (ActivateTrial(std::string(activationKeyInput))) {
            isTrialValid = true;
            showActivationPopup = false;
        } else {
            ImGui::OpenPopup("Invalid Key");
        }
    }
    
    if (ImGui::BeginPopupModal("Invalid Key", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Invalid activation key!");
        ImGui::Spacing();
        if (ImGui::Button("OK", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                       "Contact admin to get activation key");
    
    ImGui::End();
}

void DrawTrialInfo() {
    int remainingHours = GetTrialRemainingHours();
    if (remainingHours <= 0) {
        isTrialValid = false;
        showActivationPopup = true;
        return;
    }
    
    int days = remainingHours / 24;
    int hours = remainingHours % 24;
    
    char info[128];
    if (days > 0) {
        sprintf(info, "TRIAL: %d day(s) %d hour(s) remaining", days, hours);
    } else {
        sprintf(info, "TRIAL: %d hour(s) remaining", hours);
    }
    
    ImVec2 textSize = ImGui::CalcTextSize(info);
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    
    draw->AddRectFilled(ImVec2(abs_ScreenX/2 - textSize.x/2 - 10, 10),
                        ImVec2(abs_ScreenX/2 + textSize.x/2 + 10, 40),
                        IM_COL32(0, 0, 0, 180), 8.0f);
    
    ImColor textColor;
    if (remainingHours < 24) {
        textColor = IM_COL32(255, 100, 100, 255);
    } else {
        textColor = IM_COL32(100, 255, 100, 255);
    }
    
    draw->AddText(ImVec2(abs_ScreenX/2 - textSize.x/2, 18), textColor, info);
}

void DrawMonster(ImDrawList *Draw) {
    if (!isTrialValid) {
        return;
    }
    
    DrawTrialInfo();
    
    if (autoRetribution) {
        ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(retriTouchX, retriTouchY), 18.0f, IM_COL32(255, 255, 255, 180), 16);
        ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(retriTouchX, retriTouchY), 18.0f, IM_COL32(0, 0, 0, 255), 16, 2.5f);
    }

    DrawFloatingRetriButton();

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

void Layout_tick_UI() {
    if (!isTrialValid) {
        DrawActivationPopup();
        return;
    }
    
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    
    static ImVec2 windowSize = ImVec2(715, 400);
    static bool isResizing = false;
    static ImVec2 resizeStartSize;
    static ImVec2 resizeStartPos;
    
    ImGui::SetNextWindowSizeConstraints(ImVec2(100, 100), ImVec2(715, 400));
    ImGui::SetNextWindowSize(ImVec2(715, 400), ImGuiCond_FirstUseEver);

    ImGui::Begin(oxorany("VOLKS External / @volksive"), nullptr, window_flags);
    
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
            ImGui::Text(oxorany("Current FPS: %.1f"), ImGui::GetIO().Framerate);
            ImGui::Separator();
            ImGui::Checkbox(oxorany("ESP Box"), &drawESPBox);
            ImGui::Checkbox(oxorany("ESP Health Bar"), &drawHealthBar);
            ImGui::Checkbox(oxorany("ESP Line"), &drawMHealth);
            ImGui::Checkbox(oxorany("ESP Hero Icon"), &iconhero);
            ImGui::Checkbox(oxorany("ESP Distance"), &drawDistance);
            ImGui::Checkbox(oxorany("ESP Health"), &drawHealth);
            ImGui::Checkbox(oxorany("ESP Hero Name"), &drawHeroName);
            
            
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(oxorany("ESP Monster"))) {
            ImGui::Separator();
            ImGui::Text(oxorany("Display Settings:"));
            ImGui::Checkbox(oxorany("ESP Monster Name"), &drawMonsterName);
            ImGui::Checkbox(oxorany("ESP Monster Distance"), &drawMonsterDistance);
            ImGui::Checkbox(oxorany("ESP Monster Health"), &drawMonsterHealth);
            
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
            ImGui::Checkbox(oxorany("Blue & Red Buff"), &AutoRetributionBuff);
            ImGui::Checkbox(oxorany("Lord & Turtle"), &AutoRetributionBoss);
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text(oxorany("Floating Button:"));
            ImGui::Checkbox(oxorany("Show Floating Button"), &showFloatingButton);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                               oxorany("Tap: Switch Target | Drag: Move"));
            
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem(oxorany("Settings"))) {
    ImGui::Spacing();
    ImGui::Text(oxorany("UI Settings:"));
    ImGui::Separator();
    
    static int theme = 2;
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
    
    if (ImGui::Button(oxorany("Unload Cheat"), ImVec2(-1, 50))) {
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
    ImGui::GetStyle().ScrollbarSize = 25.0f;
    ImGui::StyleColorsClassic();
    
    isTrialValid = CheckTrial();
    
    while (main_thread_flag) {
        if (isTrialValid) {
            MonsterRetribution();
            CheckAndTriggerRetribution();
        }
        drawBegin();
        Layout_tick_UI();
        drawEnd();
        usleep(250);
    }
    shutdown();
    Touch_Close();
    return 0;
}
