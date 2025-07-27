#include "pch.h"
#include <Windows.h>
#include <gl/GL.h>
#include "MinHook.h"

#pragma comment(lib, "opengl32.lib")

// -------- Typedefs --------
typedef void (APIENTRY* glColor4f_t)(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
typedef void (APIENTRY* glBegin_t)(GLenum mode);
typedef void (APIENTRY* glEnd_t)();
bool espEnabled = true;


// -------- Original Function Pointers --------
glColor4f_t original_glColor4f = nullptr;
glBegin_t original_glBegin = nullptr;
glEnd_t original_glEnd = nullptr;



// -------- Hooked Functions --------

DWORD WINAPI ToggleThread(LPVOID) {
    while (true) {
        if (GetAsyncKeyState(VK_F1) & 1) {
            espEnabled = !espEnabled;
            OutputDebugStringA(espEnabled ? "[ESP] ON\n" : "[ESP] OFF\n");
        }
        Sleep(100);
    }
    return 0;
}

// Force player models to be red
void APIENTRY my_glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    if (espEnabled) {
        original_glColor4f(r + 1, g, b, 0.9); // force red
    }
    else {
        original_glColor4f(r, g, b, a);
    }
}

// Disable depth testing so models draw through walls
void APIENTRY my_glBegin(GLenum mode) {
    if (espEnabled && (mode == GL_TRIANGLES || mode == GL_QUADS)) {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    original_glBegin(mode);
}

// Re-enable depth testing after drawing
void APIENTRY my_glEnd() {
    if (espEnabled) {
        glEnable(GL_DEPTH_TEST);
    }

    original_glEnd();
}


// -------- Hook Setup Thread --------
DWORD WINAPI HackThread(HMODULE hModule) {

    // Wait until OpenGL is loaded
    while (!GetModuleHandleA("opengl32.dll")) {
        Sleep(100);
    }

    HMODULE hOpenGL = GetModuleHandleA("opengl32.dll");

    // Initialize MinHook
    if (MH_Initialize() != MH_OK)
        return 1;

    // Hook glColor4f
    FARPROC colorAddr = GetProcAddress(hOpenGL, "glColor4f");
    MH_CreateHook((LPVOID)colorAddr, &my_glColor4f, reinterpret_cast<LPVOID*>(&original_glColor4f));
    MH_EnableHook((LPVOID)colorAddr);

    // Hook glBegin
    FARPROC beginAddr = GetProcAddress(hOpenGL, "glBegin");
    MH_CreateHook((LPVOID)beginAddr, &my_glBegin, reinterpret_cast<LPVOID*>(&original_glBegin));
    MH_EnableHook((LPVOID)beginAddr);

    // Hook glEnd
    FARPROC endAddr = GetProcAddress(hOpenGL, "glEnd");
    MH_CreateHook((LPVOID)endAddr, &my_glEnd, reinterpret_cast<LPVOID*>(&original_glEnd));
    MH_EnableHook((LPVOID)endAddr);

    CreateThread(nullptr, 0, ToggleThread, nullptr, 0, nullptr);

    MessageBoxA(NULL, "ESP Wallhack Hooked Successfully!", "CS16Wallhack", MB_OK);
    return 0;
}


// -------- DLL Entry --------
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)HackThread, hModule, 0, NULL);
    }
    return TRUE;
}