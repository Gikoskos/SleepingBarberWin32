/***********************************************\
*                   customer.c                  *
*           George Koskeridis (C) 2016          *
\***********************************************/

#include "main.h"
#include "customer.h"
#include "draw.h"
#include "barber.h"
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
    HANDLE ReadyCustomersSem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, ReadyCustomersSemaphoreName);
    HANDLE BarberIsReadyMtx = OpenMutex(MUTEX_ALL_ACCESS, FALSE, BarberIsReadyMutexName);
    HANDLE WRAccessToSeatsMtx = OpenMutex(MUTEX_ALL_ACCESS, FALSE, WRAccessToSeatsMutexName);

    if (!ReadyCustomersSem || !BarberIsReadyMtx || !WRAccessToSeatsMtx) {
        PRINT_ERR_DEBUG();
       _endthreadex(1);
       return 1;
    }
#define TIMEOUT 1000
    //Sleep(TIMEOUT);
    while (GetCustomerState(customer) != CUSTOMER_DONE) {
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
            //Sleep(TIMEOUT);
            ReleaseSemaphore(ReadyCustomersSem, 1, NULL);
            ReleaseMutex(WRAccessToSeatsMtx);
            WaitForSingleObject(BarberIsReadyMtx, INFINITE);
            SetCustomerState(customer, GETTING_HAIRCUT);
        } else {
            ReleaseMutex(WRAccessToSeatsMtx);
        }

        SetCustomerState(customer, CUSTOMER_DONE);
    }
    InterlockedDecrement(&total_customers);
    CloseHandle(ReadyCustomersSem);
    CloseHandle(BarberIsReadyMtx);
    CloseHandle(WRAccessToSeatsMtx);
    _endthreadex(0);
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
