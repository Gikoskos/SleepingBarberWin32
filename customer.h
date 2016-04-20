/***********************************************\
*                   customer.h                  *
*           George Koskeridis (C) 2016          *
\***********************************************/

#pragma once

#ifndef __CUSTOMER_H
#define __CUSTOMER_H

#include "common.h"

enum customer_state {
    
    WAITING_ROOM,
    GETTING_HAIRCUT
};

typedef struct _customer {
    HANDLE hthrd; //the handle of the customer's thread
    enum customer_state state;
} customer;

#endif //__CUSTOMER_H
