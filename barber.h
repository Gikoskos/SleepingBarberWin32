/***********************************************\
*                    barber.h                   *
*           George Koskeridis (C) 2016          *
\***********************************************/

#pragma once

#ifndef __BARBER_H
#define __BARBER_H

#include "common.h"

typedef struct _barber_data {
    HANDLE hthrd; //the handle of the barber's thread
    POINT coords; //coordinates for the up-left corner of the barber's Bitmap rectangle
    int width, height;
} barber_data;


barber_data *InitBarber(void);
BOOL DeleteBarber(barber_data *to_delete);
#endif //__BARBER_H
