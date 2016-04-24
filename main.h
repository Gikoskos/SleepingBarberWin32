/***********************************************\
*                     main.h                    *
*           George Koskeridis (C) 2016          *
\***********************************************/

#pragma once

#ifndef __MAIN_H
#define __MAIN_H

#include "common.h"

#define TOTAL_RESOLUTIONS 3
#define CUSTOMER_CHAIRS  5


typedef struct _backbuffer_data {
    HBITMAP hBitmap;
    HDC hdc;
    RECT coords;
} backbuffer_data;

typedef struct _window_data {
    HWND hwnd; //window handle
    WNDCLASSEX wc; //window class
    LPTSTR title; //window title
    int width, height; //width, height of working area
} window_data;

enum {
    SMALL_WND = 0,
    MEDIUM_WND,
    LARGE_WND
};

extern HANDLE ReadyCustomersSem, BarberIsReadyMtx, KillAllThreadsEvt;

LONG GetFreeCustomerSeats(void);
void IncFreeCustomerSeats(void);
void DecFreeCustomerSeats(void);
#endif //__MAIN_H
