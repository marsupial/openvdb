#pragma once

typedef struct PezConfigRec
{
    const char* Title;
    int Width;
    int Height;
    int Multisampling;
    int VerticalSync;
} PezConfig;

#ifdef __cplusplus
extern "C" {
#endif

// Implemented by the application code:
PezConfig PezGetConfig();
void PezInitialize(int w, int h);
void PezRender();
int  PezUpdate(unsigned int microseconds);
void PezHandleMouse(int x, int y, int action);
void PezHandleKey(char c);
void PezResize(int w, int h);

// Implemented by the platform layer:
const char* PezResourcePath();
void PezDebugString(const char* pStr, ...);
void PezDebugStringW(const wchar_t* pStr, ...);
void PezFatalError(const char* pStr, ...);
void PezFatalErrorW(const wchar_t* pStr, ...);
#define PezCheckCondition(A, ...) if (!(A)) { PezFatalError(__VA_ARGS__); }
#define PezCheckConditionW(A, ...) if (!(A)) { PezFatalErrorW(__VA_ARGS__); }

const char* PezGetAssetsFolder();

#ifdef __cplusplus
}
#endif

// Various constants and macros:
#define TwoPi (6.28318531f)
#define Pi (3.14159265f)
#define countof(A) (sizeof(A) / sizeof(A[0]))
enum MouseFlag {
  PEZ_DOWN        = 1 << 0,
  PEZ_UP          = 1 << 1,
  PEZ_MOVE        = 1 << 2,
  PEZ_DOUBLECLICK = 1 << 3,
  PEZ_SCROLL      = 1 << 4,

  PEZ_M_MOUSE     = 1 << 5,
  PEZ_R_MOUSE     = 1 << 6,
  
  PEZ_ALT_KEY     = 1 << 7,
  PEZ_CTL_KEY     = 1 << 8,
  PEZ_CMD_KEY     = 1 << 9,
  PEZ_SFT_KEY     = 1 << 10,
};
