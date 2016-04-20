/***********************************************\
*                    barber.h                   *
*           George Koskeridis (C) 2016          *
\***********************************************/

#pragma once

#ifndef __BARBER_H
#define __BARBER_H

#include "common.h"

enum barber_state {
    SLEEPING,
    CUTTING_HAIR,
    GET_NEXT_CUSTOMER
};

typedef struct _barber_data {
    HANDLE hthrd; //the handle of the barber's thread
    enum barber_state state;
} barber_data;


barber_data *InitBarber(void);
BOOL DeleteBarber(barber_data *to_delete);
#endif //__BARBER_H
