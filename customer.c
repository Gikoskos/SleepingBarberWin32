/***********************************************\
*                   customer.c                  *
*           George Koskeridis (C) 2016          *
\***********************************************/

#include "main.h"
#include "customer.h"
#include "barber.h"
#include "draw.h" //for GetBarbershopDoorState()
#include "FIFOqueue.h"


extern LONG total_customers;

/* Prototypes for functions with local scope */
static UINT CALLBACK CustomerThread(void *args);



customer_data *NewCustomer(int InitialState)
{
    customer_data *new = win_malloc(sizeof(customer_data));
    if (!new) {
        MessageBox(NULL, TEXT("Failed allocating memory for a customer!"), 
                   TEXT("Something happened!"), MB_ICONERROR | MB_OK);
        return (customer_data*)NULL;
    }

    new->state = WAITTING_IN_QUEUE;
    InterlockedIncrement(&total_customers);
    new->hthrd = (HANDLE)_beginthreadex(NULL, 0, CustomerThread, (LPVOID)new, InitialState, NULL);
    return new;
}

static UINT CALLBACK CustomerThread(LPVOID args)
{
    customer_data *customer = (customer_data*)args;

    if (!ReadyCustomersSem || !BarberIsReadyMtx || !WRAccessToSeatsMtx || !KillAllThreadsEvt) {
        return 1;
    }

    //Sleep(TIMEOUT);
    while (GetCustomerState(customer) != CUSTOMER_DONE &&
           WaitForSingleObject(KillAllThreadsEvt, 0L) == WAIT_TIMEOUT) {
        SetCustomerState(customer, WAITTING_IN_QUEUE);
#ifndef _DEBUG
        while(!GetBarbershopDoorState()) Sleep(10);
#endif
        WaitForSingleObject(WRAccessToSeatsMtx, INFINITE);
        if (numOfFreeSeats > 0) {
            numOfFreeSeats--;
            if (numOfFreeSeats == CUSTOMER_CHAIRS - 1) {
                SetCustomerState(customer, WAKING_UP_BARBER);
            } else {
                SetCustomerState(customer, SITTING_IN_WAITING_ROOM);
            }
            ReleaseSemaphore(ReadyCustomersSem, 1, NULL);
            ReleaseMutex(WRAccessToSeatsMtx);
            WaitForSingleObject(BarberIsReadyMtx, INFINITE);
            WAIT_UNTIL_TIMEOUT_OR_DIE(1);
            SetCustomerState(customer, GETTING_HAIRCUT);
            WAIT_UNTIL_TIMEOUT_OR_DIE(1);
        } else {
            ReleaseMutex(WRAccessToSeatsMtx);
        }

        SetCustomerState(customer, CUSTOMER_DONE);
    }
    InterlockedDecrement(&total_customers);

    return 0;
}

LONG GetCustomerState(customer_data *customer)
{
    if (!customer) return CUSTOMER_DONE;

    LONG retvalue;

    InterlockedExchange(&retvalue, customer->state);
    return retvalue;
}

void SetCustomerState(customer_data *customer, LONG new_state)
{
    if (customer) {
        InterlockedExchange(&customer->state, new_state);
        /*AcquireSRWLockExclusive(&customer->srw_lock);
        customer->state = new_state;
        ReleaseSRWLockExclusive(&customer->srw_lock);*/
    }
}

BOOL DeleteCustomer(customer_data *to_delete)
{
    BOOL retvalue = FALSE;

    if (to_delete != NULL) {
        retvalue = ((to_delete->hthrd != NULL) ? CloseHandle(to_delete->hthrd) : TRUE) ? TRUE : FALSE;
        win_free(to_delete);
        to_delete = NULL;
    }
    return retvalue;
}
