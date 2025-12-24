#include <windows.h>
#include <stdio.h>
#include <atomic>
#include <thread>
#include "dllmain.h"
#include "keyboard.h"


#define TOGGLE_KEY VK_ADD
#define TOGGLE_INVINCIBLE_KEY VK_MULTIPLY
#define TOGGLE_HORSE_INVINCIBLE_KEY VK_DIVIDE


static std::atomic<bool> VEHICLE_DENSITY_MULTIPLIER(false);
static std::atomic<bool> PLAYER_INVINCIBLE(false);
static std::atomic<bool> HORSE_INVINCIBLE(false);


static bool PLAYER_INVINCIBLE_WAS_ON = false;
static bool HORSE_INVINCIBLE_WAS_ON = false;


static std::atomic<bool> g_RunKeyThread(true);
static std::thread g_KeyThread;

static void OpenConsoleAndPrint(void)
{
    if (AllocConsole())
    {
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        printf("[Script] console opened\n");
    }
}


static void KeyThreadFunc()
{
    while (g_RunKeyThread.load())
    {

        if (IsKeyJustUp(TOGGLE_KEY, true))
        {
            VEHICLE_DENSITY_MULTIPLIER.store(!VEHICLE_DENSITY_MULTIPLIER.load());
            printf("[KeyThread] VEHICLE_DENSITY_MULTIPLIER -> %s\n", VEHICLE_DENSITY_MULTIPLIER.load() ? "ON" : "OFF");
        }

        if (IsKeyJustUp(TOGGLE_INVINCIBLE_KEY, true))
        {
            bool newv = !PLAYER_INVINCIBLE.load();
            PLAYER_INVINCIBLE.store(newv);
            printf("[KeyThread] PLAYER_INVINCIBLE -> %s\n", newv ? "ON" : "OFF");
        }

        if (IsKeyJustUp(TOGGLE_HORSE_INVINCIBLE_KEY, true))
        {
            bool newv = !HORSE_INVINCIBLE.load();
            HORSE_INVINCIBLE.store(newv);
            printf("[KeyThread] HORSE_INVINCIBLE -> %s\n", newv ? "ON" : "OFF");
        }
    }
}


void ScriptMain()
{
    OpenConsoleAndPrint();

    while (true)
    {
        WAIT(0);
        
        bool vehicleDensity = VEHICLE_DENSITY_MULTIPLIER.load();
        bool playerInv = PLAYER_INVINCIBLE.load();
        bool horseInv = HORSE_INVINCIBLE.load();

        Ped playerPed = PLAYER::PLAYER_PED_ID();

        if (playerInv)
        {
            PLAYER::SET_PLAYER_INVINCIBLE(PLAYER::PLAYER_ID(), TRUE);
            PLAYER_INVINCIBLE_WAS_ON = true;
        }
        else
        {
            if (PLAYER_INVINCIBLE_WAS_ON)
            {
                PLAYER::SET_PLAYER_INVINCIBLE(PLAYER::PLAYER_ID(), FALSE);
                PLAYER_INVINCIBLE_WAS_ON = false;
            }
        }



        if (horseInv)
        {
            if (PED::IS_PED_ON_MOUNT(playerPed))
            {
                Ped horse = PED::GET_MOUNT(playerPed);
                if (ENTITY::DOES_ENTITY_EXIST(horse))
                {
                    ENTITY::SET_ENTITY_INVINCIBLE(horse, TRUE);
                }
            }
            HORSE_INVINCIBLE_WAS_ON = true;
        }
        else
        {
            if (HORSE_INVINCIBLE_WAS_ON)
            {
                if (PED::IS_PED_ON_MOUNT(playerPed))
                {
                    Ped horse = PED::GET_MOUNT(playerPed);
                    if (ENTITY::DOES_ENTITY_EXIST(horse))
                    {
                        ENTITY::SET_ENTITY_INVINCIBLE(horse, FALSE);
                    }
                }
                HORSE_INVINCIBLE_WAS_ON = false;
            }
        }


        if (vehicleDensity)
        {
            VEHICLE::SET_VEHICLE_DENSITY_MULTIPLIER_THIS_FRAME(0);
        }
    }
}


BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        g_RunKeyThread.store(true);
        try
        {
            g_KeyThread = std::thread(KeyThreadFunc);
        }
        catch (...)
        {
            g_RunKeyThread.store(false);
        }

        scriptRegister(hInstance, ScriptMain);
        keyboardHandlerRegister(OnKeyboardMessage);
        break;

    case DLL_PROCESS_DETACH:
        g_RunKeyThread.store(false);
        if (g_KeyThread.joinable())
        {
            g_KeyThread.join();
        }

        scriptUnregister(hInstance);
        keyboardHandlerUnregister(OnKeyboardMessage);
        break;
    }
    return TRUE;
}
