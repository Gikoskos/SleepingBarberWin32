/***********************************************\
*                   customer.c                  *
*           George Koskeridis (C) 2016          *
\***********************************************/

#include "main.h"
#include "customer.h"
#include "barber.h"
#include "draw.h" //for GetBarbershopDoorState()
#include "FIFOqueue.h"


#define TIMEOUT 2000

#define EXIT_LOOP_IF_ASSERT(x) \
if (x) {\
    break;\
}


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

#define BLOCK_UNTIL_TIMEOUT_OR_BREAK() \
EXIT_LOOP_IF_ASSERT(WaitForSingleObject(KillAllThreadsEvt, TIMEOUT) == WAIT_OBJECT_0);

static UINT CALLBACK CustomerThread(LPVOID args)
{
    if (!ReadyCustomersSem || !BarberIsReadyMtx || !KillAllThreadsEvt) {
        return 1;
    }

    customer_data *customer = (customer_data*)args;
    HANDLE BarberIsReadyOrDieObj[2] = {KillAllThreadsEvt, BarberIsReadyMtx};


    while (GetCustomerState(customer) != CUSTOMER_DONE &&
           WaitForSingleObject(KillAllThreadsEvt, 0L) == WAIT_TIMEOUT) {

        SetCustomerState(customer, WAITTING_IN_QUEUE);
#ifndef _DEBUG
        while(!GetBarbershopDoorState()) Sleep(10);
#else
        BLOCK_UNTIL_TIMEOUT_OR_BREAK();
#endif

        if (GetFreeCustomerSeats() > 0) {

            DecFreeCustomerSeats();

            SetCustomerState(customer, SITTING_IN_WAITING_ROOM);

            ReleaseSemaphore(ReadyCustomersSem, 1, NULL);
            EXIT_LOOP_IF_ASSERT(WaitForMultipleObjects(2, BarberIsReadyOrDieObj, FALSE, INFINITE) == WAIT_OBJECT_0);
            SetCustomerState(customer, GETTING_HAIRCUT);
            BLOCK_UNTIL_TIMEOUT_OR_BREAK();
            SetCustomerState(customer, CUSTOMER_DONE);
        }
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
