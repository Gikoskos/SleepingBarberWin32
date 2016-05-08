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

static LONG total_customers;
static LONG numOfFreeSeats = CUSTOMER_CHAIRS;

static int queue_customer_count = 0;

/* Prototypes for functions with local scope */
static UINT CALLBACK CustomerThread(void *args);
static void IncNumOfCustomers(void);
static void DecNumOfCustomers(void);

customer_data *NewCustomer(int InitialState)
{
    customer_data *new = win_malloc(sizeof(customer_data));
    if (!new) {
        MessageBox(NULL, TEXT("Failed allocating memory for a customer!"), 
                   TEXT("Something happened!"), MB_ICONERROR | MB_OK);
        return (customer_data*)NULL;
    }

    new->haircut_success = FALSE;
    new->queue_num = queue_customer_count++;
    new->state = WAITTING_IN_QUEUE;
    IncNumOfCustomers();
    new->hthrd = (HANDLE)_beginthreadex(NULL, 0, CustomerThread, (LPVOID)new, InitialState, NULL);
    return new;
}

#define BLOCK_UNTIL_TIMEOUT_OR_BREAK(x) \
BREAK_IF_TRUE(WaitForSingleObject(KillAllThreadsEvt, x) == WAIT_OBJECT_0);

static UINT CALLBACK CustomerThread(LPVOID args)
{
    if (!ReadyCustomersSem || !BarberIsReadySem || !KillAllThreadsEvt || !BarberIsDoneSem) {
        return 1;
    }

    customer_data *customer = (customer_data*)args;
    BOOL done = FALSE;
    HANDLE BarberIsReadyOrDieObj[2] = {KillAllThreadsEvt, BarberIsReadySem};
    HANDLE AccessFIFOOrDieObj[2] = {KillAllThreadsEvt, AccessCustomerFIFOMtx};
    HANDLE BarberIsDoneOrDieObj[2] = {KillAllThreadsEvt, BarberIsDoneSem};

    while (!done && (WaitForSingleObject(KillAllThreadsEvt, 0L) == WAIT_TIMEOUT)) {
        SetCustomerState(customer, WAITTING_IN_QUEUE);
#ifndef _DEBUG
        while(!GetBarbershopDoorState()) BLOCK_UNTIL_TIMEOUT_OR_BREAK(50);
#else
        BLOCK_UNTIL_TIMEOUT_OR_BREAK(TIMEOUT);
#endif

        if (GetFreeCustomerSeats() > 0) {
            SetCustomerState(customer, SITTING_IN_WAITING_ROOM);

            DecFreeCustomerSeats();

            for (;;) {
                DWORD dwResult = WaitForMultipleObjects(2, AccessFIFOOrDieObj, FALSE, INFINITE);
                if (dwResult == WAIT_OBJECT_0 + 1) {
                    if (((customer_data*)customer_queue->head->data)->queue_num == customer->queue_num) {
                        printf("HELLO FROM %ld!!!\n", customer->queue_num);
                        break;
                    }
                    ReleaseMutex(AccessCustomerFIFOMtx);
                } else {
                    return 1;
                }
            }

            FIFOdequeue(customer_queue);
            ReleaseMutex(AccessCustomerFIFOMtx);

            ReleaseSemaphore(ReadyCustomersSem, 1, NULL);
            BREAK_IF_TRUE(WaitForMultipleObjects(2, BarberIsReadyOrDieObj, FALSE, INFINITE) == WAIT_OBJECT_0);
            SetCustomerState(customer, GETTING_HAIRCUT);

            BREAK_IF_TRUE(WaitForMultipleObjects(2, BarberIsDoneOrDieObj, FALSE, INFINITE) == WAIT_OBJECT_0);
            customer->haircut_success = done = TRUE;
            printf("Customer %ld is done!\n", customer->queue_num);
        }
    }
    SetCustomerState(customer, CUSTOMER_DONE);
    DecNumOfCustomers();
    printf("total_customers = %ld\n", total_customers);

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

LONG GetNumOfCustomers(void)
{
    LONG retvalue;

    InterlockedExchange(&retvalue, total_customers);
    return retvalue;
}

static void IncNumOfCustomers(void)
{
    InterlockedIncrement(&total_customers);
}

static void DecNumOfCustomers(void)
{
    InterlockedDecrement(&total_customers);
}

LONG GetFreeCustomerSeats(void)
{
    LONG retvalue;

    InterlockedExchange(&retvalue, numOfFreeSeats);
    return retvalue;
}

void IncFreeCustomerSeats(void)
{
    InterlockedIncrement(&numOfFreeSeats);
}

void DecFreeCustomerSeats(void)
{
    InterlockedDecrement(&numOfFreeSeats);
}
