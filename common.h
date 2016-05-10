/***********************************************\
*                    common.h                   *
*           George Koskeridis (C) 2016          *
\***********************************************/

#pragma once

#ifndef __COMMON_H
#define __COMMON_H

//target Windows 7
#ifndef WINVER
# define WINVER 0x0600
#endif
#ifndef _WIN32_WINNT
# define _WIN32_WINNT 0x0600
#endif
#include <SDKDDKVer.h> //API versioning

#ifndef UNICODE
# define UNICODE
#endif

#ifndef _UNICODE
# define _UNICODE
#endif

#define NOCRYPT
#include <windows.h>
#undef NOCRYPT

#include <process.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <tchar.h>
#include <assert.h>

//wrappers for the wordy winapi alloc functions
#define win_free(x) HeapFree(GetProcessHeap(), 0, x)
//this one isn't actually a wrapper for stdlib malloc since malloc doesn't 0 out the allocated bytes
#define win_malloc(x) HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, x)
#define win_realloc(x, y) HeapReAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, x, y)
#define win_calloc(x, y) win_malloc(x * y)

//RGB color macros
#define RGB_WHITE       RGB(255, 255, 255)
#define RGB_BLACK       RGB(0, 0, 0)
#define RGB_ORANGE      RGB(255, 116, 53)
#define RGB_PURPLE      RGB(64, 0, 255)
#define RGB_GREEN       RGB(0, 204, 0)
#define RGB_RED         RGB(255, 0, 0)
#define RGB_BLUE        RGB(0, 128, 255)
#define RGB_YELLOW      RGB(255, 255, 102)
#define RGB_PURPLEBLUE  RGB(64, 55, 145)

#define PRINT_ERR_DEBUG() fprintf(stderr, "Error %lu at line %d and function %s!\n", \
                                  GetLastError(), __LINE__, __func__)


#define IDI_LARGEBARBERWIN_ICON 210
#define IDI_SMALLBARBERWIN_ICON 211

//helper macro used in loops inside the barber and customer thread functions
#define BREAK_IF_TRUE(x) \
if (x) {\
    break;\
}

#define TIMEOUT 2000

#endif //__COMMON_H
