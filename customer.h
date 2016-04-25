/***********************************************\
*                   customer.h                  *
*           George Koskeridis (C) 2016          *
\***********************************************/

#pragma once

#ifndef __CUSTOMER_H
#define __CUSTOMER_H

#include "common.h"


enum customer_state {
    WAITTING_IN_QUEUE = 0x0000aaaa,
    WAKING_UP_BARBER,
    SITTING_IN_WAITING_ROOM,
    GETTING_HAIRCUT,
    CUSTOMER_DONE
};

typedef struct _customer_data {
    HANDLE hthrd; //the handle of the customer's thread
    LONG state; //the customer's state
    LONG queue_num; //the number of the customer as he is created
    //1st customer created with NewCustomer() is 1, 2nd is 2, 3rd is 3 etc
    BOOL haircut_success;
} customer_data;

customer_data *NewCustomer(int InitialState);
LONG GetCustomerState(customer_data *customer);
void SetCustomerState(customer_data *customer, LONG new_state);

BOOL DeleteCustomer(customer_data *to_delete);
#endif //__CUSTOMER_H
