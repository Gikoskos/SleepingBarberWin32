/***********************************************\
*                     main.c                    *
*           George Koskeridis (C) 2016          *
\***********************************************/

#include "main.h"
#include "customer.h"
#include "barber.h"
#include "draw.h"


#ifdef _DEBUG
typedef struct _dll_func_address {
    HMODULE to_load;
    void (*DrawBufferToWindow)(HWND, backbuffer_data*);
    void (*DrawToBuffer)(backbuffer_data*);
    void (*UpdateState)(LONG, int*, int, BOOL*, BOOL, BOOL, BOOL);
    void (*CleanupGraphics)(void);
    void (*ScaleGraphics)(int);
} dll_func_address;

static dll_func_address drawdll_func = {NULL, NULL, NULL, NULL, NULL};
#endif


HANDLE ReadyCustomersSem = NULL;
HANDLE BarberIsReadySem = NULL;
HANDLE BarberIsDoneSem = NULL;
HANDLE AccessCustomerFIFOMtx = NULL;
HANDLE KillAllThreadsEvt = NULL;

FIFOqueue *customer_queue;


int curr_resolution = SMALL_WND, prev_resolution = SMALL_WND;


//the resolutions here have to go from smaller to bigger for the resizing animation to work
POINT resolutions[TOTAL_RESOLUTIONS] = {
    {640, 480},
    {800, 600},
    {1000, 750}
};

//is TRUE when the game is running and false when it's not
static BOOL running = FALSE;
static HINSTANCE g_hInst;
static BOOL animations_enabled = FALSE;


/* Prototypes for functions with local scope */
static backbuffer_data *NewBackbuffer(HWND hwnd);
static BOOL DeleteBackbuffer(backbuffer_data *to_delete);
static HWND CreateSleepingBarberWindow(WNDCLASSEX *wc, LPCTSTR window_title,
                                       LPCTSTR class_name, int window_width,
                                       int window_height, LPVOID WinProcParam);
static BOOL ResizeWindow(HWND hwnd, int new_size, int old_size);
static inline void HandleMessages(HWND hwnd);
static inline BOOL ConvertWinToClientResolutions(void);
static LRESULT CALLBACK MainWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


static backbuffer_data *NewBackbuffer(HWND hwnd)
{
    if (!hwnd)
        return (backbuffer_data*)NULL;

    HDC hdc = GetDC(hwnd);
    if (!hdc)
        return (backbuffer_data*)NULL;

    backbuffer_data *new = win_malloc(sizeof(backbuffer_data));
    if (new == NULL) {
        ReleaseDC(hwnd, hdc);
        return (backbuffer_data*)NULL;
    }

    GetClientRect(hwnd, &new->coords);

    new->hdc = CreateCompatibleDC(hdc);
    if (!(new->hdc)) {
        win_free(new);
        ReleaseDC(hwnd, hdc);
        return (backbuffer_data*)NULL;
    }

#ifdef _DEBUG
    printf("New client area: %ldx%ld\n", new->coords.right, new->coords.bottom);
#endif
    new->hBitmap = CreateCompatibleBitmap(hdc, new->coords.right, new->coords.bottom);
    if (!(new->hBitmap)) {
        DeleteDC(new->hdc);
        win_free(new);
        ReleaseDC(hwnd, hdc);
        return (backbuffer_data*)NULL;
    }

    HGDIOBJ temp = SelectObject(new->hdc, new->hBitmap);
    DeleteObject(temp);
    ReleaseDC(hwnd, hdc);
    return new;
}

static BOOL DeleteBackbuffer(backbuffer_data *to_delete)
{
    BOOL retvalue = FALSE;

    if (to_delete != NULL) {
        HGDIOBJ prev = SelectObject(to_delete->hdc, to_delete->hBitmap);
        DeleteObject(prev);
        retvalue = ((to_delete->hBitmap != NULL) ? DeleteObject((HGDIOBJ)to_delete->hBitmap) : TRUE) ? TRUE : FALSE;
        retvalue = ((to_delete->hdc != NULL) ? DeleteDC(to_delete->hdc) : TRUE) ? TRUE : FALSE;
        win_free(to_delete);
    }
    return retvalue;
}

static HWND CreateSleepingBarberWindow(WNDCLASSEX *wc, LPCTSTR window_title,
                                       LPCTSTR class_name, int window_width, 
                                       int window_height, LPVOID WinProcParam)
{
    HWND retvalue = NULL;

    wc->cbSize = sizeof(WNDCLASSEX);
    wc->style = wc->cbClsExtra = wc->cbWndExtra = 0;
    wc->lpfnWndProc = MainWindowProcedure;
    wc->hInstance = g_hInst;
    wc->hIconSm = (HICON)LoadImage(g_hInst, MAKEINTRESOURCEW(IDI_SMALLBARBERWIN_ICON),
                                   IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                   GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    wc->hIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCEW(IDI_LARGEBARBERWIN_ICON),
                                 IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                 GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    wc->hCursor = LoadCursor(NULL, IDC_ARROW);
    wc->hbrBackground = NULL;
    wc->lpszMenuName = NULL;
    wc->lpszClassName = class_name;

    if (!RegisterClassEx(wc)) {
        MessageBox(NULL, TEXT("Main Window class registration Failed!"), TEXT("Something happened!"), MB_ICONERROR | MB_OK);
        return retvalue;
    }

    retvalue = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, class_name, window_title,
                              WS_BORDER | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                              CW_USEDEFAULT, CW_USEDEFAULT, window_width, window_height,
                              NULL, NULL, g_hInst, WinProcParam);

    return retvalue;
}

static BOOL ResizeWindow(HWND hwnd, int new_size, int old_size)
{
    if ((new_size == old_size) || !hwnd) return FALSE;

    int i, j, width, height;
    POINT up_to;

    width = resolutions[old_size].x;
    height = resolutions[old_size].y;
    up_to.x = labs(resolutions[new_size].x - resolutions[old_size].x);
    up_to.y = labs(resolutions[new_size].y - resolutions[old_size].y);

#ifdef _DEBUG //disable the window resizing animation in debugging versions
    HDWP winnum = BeginDeferWindowPos(1);
    //no idea why 'i' needs to be less or equal than 'up_to.x' and 
    //'j' just less than 'up_to.y', but the resolutions work correctly like this
    for (i = 1, j = 1; (i <= up_to.x) || (j < up_to.y);) {
#else
    for (i = 1, j = 1; (i <= up_to.x) || (j < up_to.y);) {
        HDWP winnum = BeginDeferWindowPos(1);
#endif
        if (!winnum) continue;

        if (old_size < new_size) {
            DeferWindowPos(winnum, hwnd, HWND_TOP, 0, 0, width + i, height + j,
                           SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
        } else {
            DeferWindowPos(winnum, hwnd, HWND_TOP, 0, 0, width - i, height - j,
                           SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
        }

        i += (i <= up_to.x) ? 1 : 0;
        j += (j < up_to.y) ? 1 : 0;
#ifdef _DEBUG
    }
    EndDeferWindowPos(winnum);
#else
        EndDeferWindowPos(winnum);
    }
#endif

    return TRUE;
}

static inline BOOL ConvertWinToClientResolutions(void)
{
    for (int i = 0; i < TOTAL_RESOLUTIONS; i++) {
        RECT tmp = {.left = 0, .top = 0,
                    .right = resolutions[i].x,
                    .bottom = resolutions[i].y};
#ifdef _DEBUG
        printf("%ldx%ld converted to ", tmp.right, tmp.bottom);
#endif
        if (!AdjustWindowRectEx(&tmp, WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                FALSE, WS_EX_OVERLAPPEDWINDOW)) {
            return FALSE;
        }
        resolutions[i].x = tmp.right - tmp.left;
        resolutions[i].y = tmp.bottom - tmp.top;
#ifdef _DEBUG
        printf("%ldx%ld\n", resolutions[i].x, resolutions[i].y);
#endif
    }
    return TRUE;
}

static inline void HandleMessages(HWND hwnd)
{
    MSG msg;

    if (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static LRESULT CALLBACK MainWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#define IDM_EXIT       2000
#define IDM_ABOUT      2001
#define IDM_ANIMATIONS 2002
#define IDM_SMALL_RES  3000
#define IDM_MEDIUM_RES 3001
#define IDM_LARGE_RES  3002
    HMENU ResolutionMenu;
    POINT cursor_pos;

    switch (msg) {
        case WM_CREATE:
        {
            break;
        }
        case WM_KEYUP:
        {
            switch (wParam) {
                case VK_ESCAPE:
                    SendMessage(hwnd, WM_CLOSE, (WPARAM)0, (LPARAM)0);
                    break;
                case VK_SPACE:
                case 0x4F: //'O' key
                    SetBarbershopDoorState(!GetBarbershopDoorState());
#ifdef _DEBUG
                    drawdll_func.ScaleGraphics(curr_resolution);
#else
                    ScaleGraphics(curr_resolution);
#endif
                    break;
                default:
                    break;
            }
            break;
        }
#if 0
        case WM_MOUSEMOVE:
        {
            cursor_pos.x = GET_X_LPARAM(lParam);
            cursor_pos.y = GET_Y_LPARAM(lParam);

            fprintf(stderr, "x: %ld, y: %ld\n", cursor_pos.x, cursor_pos.y);
        }
#endif
        case WM_PAINT:
        {
            HDC hdc;
            PAINTSTRUCT ps;

            if ((hdc = BeginPaint(hwnd, &ps))) {
                EndPaint(hwnd, &ps);
            }
            break;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
                case IDM_ANIMATIONS:
#ifdef _DEBUG
                    MessageBox(hwnd, TEXT("Animations don't work in debugging mode."),
                               TEXT("Error"), MB_OK | MB_ICONERROR);
#else
                    animations_enabled = !animations_enabled;
#endif
                    break;
                case IDM_ABOUT:
                    MessageBox(hwnd, TEXT("            ")
                               TEXT("SleepingBarber Win32\n")
                               TEXT("        ")
                               TEXT("George Koskeridis (c) 2016 "),
                               TEXT("About"), MB_OK);
                    break;
                case IDM_EXIT:
                    SendMessage(hwnd, WM_CLOSE, (WPARAM)0, (LPARAM)0);
                    break;
                case IDM_SMALL_RES:
                case IDM_MEDIUM_RES:
                case IDM_LARGE_RES:
                    prev_resolution = curr_resolution;
                    curr_resolution = LOWORD(wParam) - 3000;
                    break;
                default:
                    break;
            }
            switch (HIWORD(wParam)) {
            }
            break;
        }
        case WM_RBUTTONUP:
        {
            ResolutionMenu = CreatePopupMenu();

            AppendMenu(ResolutionMenu, MF_STRING | MF_GRAYED, 0, TEXT("Window size"));
            AppendMenu(ResolutionMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(ResolutionMenu,
                       MF_STRING | ((curr_resolution == SMALL_WND) ? MF_CHECKED : MF_UNCHECKED),
                       IDM_SMALL_RES, TEXT("Low (640x480)"));
            AppendMenu(ResolutionMenu,
                       MF_STRING | ((curr_resolution == MEDIUM_WND) ? MF_CHECKED : MF_UNCHECKED),
                       IDM_MEDIUM_RES, TEXT("Medium (800x600)"));
            AppendMenu(ResolutionMenu,
                       MF_STRING | ((curr_resolution == LARGE_WND) ? MF_CHECKED : MF_UNCHECKED),
                       IDM_LARGE_RES, TEXT("High (1000x750)"));

            AppendMenu(ResolutionMenu, MF_STRING | MF_GRAYED | MF_MENUBREAK, 0, TEXT("Settings"));
            AppendMenu(ResolutionMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(ResolutionMenu,
                       MF_STRING | ((animations_enabled) ? MF_CHECKED : MF_UNCHECKED),
                       IDM_ANIMATIONS, TEXT("Animations"));

            AppendMenu(ResolutionMenu, MF_STRING | MF_GRAYED | MF_MENUBREAK, 0, TEXT("Help"));
            AppendMenu(ResolutionMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(ResolutionMenu, MF_STRING | MF_UNCHECKED, IDM_ABOUT, TEXT("About"));
            AppendMenu(ResolutionMenu, MF_STRING | MF_UNCHECKED, IDM_EXIT, TEXT("Exit"));
            GetCursorPos(&cursor_pos);
            TrackPopupMenuEx(ResolutionMenu,
                             TPM_CENTERALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION,
                             cursor_pos.x, cursor_pos.y, hwnd, NULL);
            break;
        }
        case WM_CLOSE:
        case WM_DESTROY:
        {
            running = FALSE;
            DestroyWindow(hwnd);
            PostQuitMessage(0);
            break;
        }
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return TRUE;
}

#ifdef _DEBUG
static BOOL LoadDrawDLL(void)
{
    LPCTSTR drawdll_loaded_fname = TEXT("draw-loaded-0.dll");

    if (!CopyFile(TEXT("draw.dll"), drawdll_loaded_fname, FALSE)) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            fprintf(stderr, "Copying of DLL failed! File not found.\n");
        } else if (GetLastError() == ERROR_ACCESS_DENIED) {
            fprintf(stderr, "Copying of DLL failed! Access denied.\n");
        } else if (GetLastError() == ERROR_SHARING_VIOLATION) {
            fprintf(stderr, "Copying of DLL failed! File is used by another process.\n");
        } else {
            fprintf(stderr, "Copying of DLL failed! Error code %lu\n", GetLastError());
        }
        return FALSE;
    }

    if ((drawdll_func.to_load = LoadLibrary(drawdll_loaded_fname)) != NULL) {
        drawdll_func.DrawBufferToWindow = (void(*)(HWND, backbuffer_data*))GetProcAddress(drawdll_func.to_load, "DrawBufferToWindow");
        drawdll_func.DrawToBuffer = (void(*)(backbuffer_data*))GetProcAddress(drawdll_func.to_load, "DrawToBuffer");
        drawdll_func.UpdateState = (void(*)(LONG, int*, int, BOOL*, BOOL, BOOL, BOOL))GetProcAddress(drawdll_func.to_load, "UpdateState");
        drawdll_func.ScaleGraphics = (void(*)(int))GetProcAddress(drawdll_func.to_load, "ScaleGraphics");
        drawdll_func.CleanupGraphics = (void(*)(void))GetProcAddress(drawdll_func.to_load, "CleanupGraphics");

        if (drawdll_func.ScaleGraphics) drawdll_func.ScaleGraphics(curr_resolution);
        return TRUE;
    } else {
        fprintf(stderr, "Failed loading library! Errcode %ld\n", GetLastError());
    }

    return FALSE;
}

static FILETIME GetDrawDLLLastWriteTime()
{
    FILETIME retvalue = {0, 0};
    WIN32_FIND_DATA drawdll_data;
    HANDLE hdrawdll = FindFirstFile(TEXT("draw.dll"), &drawdll_data);
    if (hdrawdll != INVALID_HANDLE_VALUE) {
        FindClose(hdrawdll);
        retvalue = drawdll_data.ftLastWriteTime;
    } else {
        fprintf(stderr, "Failed loading draw.dll! Errcode %ld\n", GetLastError());
    }
    return retvalue;
}
#endif

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    const TCHAR *ReadyCustomersSemaphoreName = _T("ReadyCustomersSem"),
                *BarberIsReadySemaphoreName = _T("BarberIsReadySem"),
                *BarberIsDoneSemaphoreName = _T("BarberIsDoneSem"),
                *AccessCustomerFIFOMutexName = _T("AccessCustomerFIFOMtx");
    const int initial_customers = 6;
    LONG prev_total_customers;

    window_data SBarberMainWindow = {
        .title = TEXT("SleepingBarber Win32")
    };
    backbuffer_data *backbuff = NULL;

    const int TICKS_PER_SECOND = 20;
    const int SKIP_TICKS = 1000 / TICKS_PER_SECOND;
    const int MAX_FRAMESKIP = 10;
    DWORD next_game_tick = GetTickCount();
    int loops;

#ifdef _DEBUG
    FILETIME currLastWriteTime, prevLastWriteTime;
    currLastWriteTime = prevLastWriteTime = GetDrawDLLLastWriteTime();

    if (!LoadDrawDLL())
        return 1;
#endif

    ReadyCustomersSem = CreateSemaphore(NULL, 0, CUSTOMER_CHAIRS, ReadyCustomersSemaphoreName);
    BarberIsReadySem = CreateSemaphore(NULL, 0, 1, BarberIsReadySemaphoreName);
    BarberIsDoneSem = CreateSemaphore(NULL, 0, 1, BarberIsDoneSemaphoreName);
    AccessCustomerFIFOMtx = CreateMutex(NULL, FALSE, AccessCustomerFIFOMutexName);
    KillAllThreadsEvt = CreateEvent(NULL, TRUE, FALSE, NULL);

    customer_queue = newFIFOqueue();

    barber_data *barber = InitBarber(0);
    if (!barber || !ReadyCustomersSem || !BarberIsReadySem || !AccessCustomerFIFOMtx ||
        !customer_queue || !BarberIsDoneSem) {
        PRINT_ERR_DEBUG();
        return 1;
    }

    customer_data **customer_array = win_malloc(sizeof(customer_data) * (size_t)initial_customers);
    int customer_array_idx = 0;
    for (int i = 0; i < initial_customers; i++) {
        customer_array[i] = NewCustomer(0);
        FIFOenqueue(customer_queue, (void*)customer_array[i]);
    }
    prev_total_customers = GetNumOfCustomers();
    //data to pass to the UpdateState() function to know the current state of each
    //drawable character and whether to animate their movement or not
    int *customer_states = win_malloc(sizeof(int) * (size_t)prev_total_customers);
    int barber_state = (int)GetBarberState(barber);
    BOOL *animate_customer = win_malloc(sizeof(BOOL) * (size_t)prev_total_customers);
    BOOL animate_barber = FALSE;


    if (!ConvertWinToClientResolutions()) {
        MessageBox(NULL, TEXT("Failed converting resolutions!"), TEXT("Something happened!"), MB_ICONERROR | MB_OK);
        return 1;
    }
    SBarberMainWindow.width = resolutions[curr_resolution].x;
    SBarberMainWindow.height = resolutions[curr_resolution].y;
#ifdef _DEBUG
    drawdll_func.ScaleGraphics(curr_resolution);
#else
    ScaleGraphics(curr_resolution);
#endif

    g_hInst = hInstance;

    SBarberMainWindow.hwnd = CreateSleepingBarberWindow(&SBarberMainWindow.wc,
                                    SBarberMainWindow.title, TEXT("SleepingBarberWin32Class"),
                                    SBarberMainWindow.width, SBarberMainWindow.height, NULL);
    if (!SBarberMainWindow.hwnd) {
        MessageBox(NULL, TEXT("Window creation failed!"), TEXT("Something happened!"), MB_ICONERROR | MB_OK);
        return 1;
    }

    ShowWindow(SBarberMainWindow.hwnd, nCmdShow);
    UpdateWindow(SBarberMainWindow.hwnd);
    running = TRUE;
    backbuff = NewBackbuffer(SBarberMainWindow.hwnd);

    while (running) {
        if (curr_resolution != prev_resolution) {
            if (!DeleteBackbuffer(backbuff)) {
                fprintf(stderr, "Error deleting backbuff!\n");
                return 1;
            }
#ifdef _DEBUG
            if (drawdll_func.ScaleGraphics) drawdll_func.ScaleGraphics(curr_resolution);
#else
            ScaleGraphics(curr_resolution);
#endif
            ResizeWindow(SBarberMainWindow.hwnd, curr_resolution, prev_resolution);
            backbuff = NewBackbuffer(SBarberMainWindow.hwnd);
            prev_resolution = curr_resolution;
        }
#ifdef _DEBUG
        currLastWriteTime = GetDrawDLLLastWriteTime();
        if (CompareFileTime(&currLastWriteTime, &prevLastWriteTime) && currLastWriteTime.dwLowDateTime) {
            //don't leak memory while reloading the DLL
            if (drawdll_func.CleanupGraphics) drawdll_func.CleanupGraphics();
            Sleep(200); //waiting for the DLL to compile. If it hasn't finished compiling
                        //when it's getting loaded, the program will crash
            FreeLibrary(drawdll_func.to_load);
            printf("RELOADING\n");

            if (!LoadDrawDLL()) PRINT_ERR_DEBUG();

            currLastWriteTime = GetDrawDLLLastWriteTime();
            prevLastWriteTime.dwLowDateTime = currLastWriteTime.dwLowDateTime;
            prevLastWriteTime.dwHighDateTime = currLastWriteTime.dwHighDateTime;
        }

        if (drawdll_func.DrawToBuffer) drawdll_func.DrawToBuffer(backbuff);
        if (drawdll_func.DrawBufferToWindow) drawdll_func.DrawBufferToWindow(SBarberMainWindow.hwnd, backbuff);
#else
        DrawToBuffer(backbuff);
        DrawBufferToWindow(SBarberMainWindow.hwnd, backbuff);
#endif
        loops = 0;

        while (GetTickCount() > next_game_tick && loops < MAX_FRAMESKIP) {
            HandleMessages(SBarberMainWindow.hwnd);

            //we check to see if the number of customers has changed
            LONG tmp = GetNumOfCustomers();
            if (tmp != prev_total_customers) {
                if (tmp - prev_total_customers < 0) {//if there are less customers than before
                    for (; customer_array_idx <= prev_total_customers - tmp - 1; customer_array_idx++) {
                        //printf("deleted customer %ld\n", customer_array[customer_array_idx]->queue_num);
                        DeleteCustomer(*(customer_array + customer_array_idx)); //we free the customers who finished
                    }
                    prev_total_customers = tmp; //the prev customers counter gets updated 
                                                //with the current number of customers

                } else { //if for some reason the customers are increased while the program is running
                         //allocate more memory for the new customers
                    win_realloc(customer_array, sizeof(customer_data) * (size_t)prev_total_customers);
                }

                win_realloc(customer_states, sizeof(int) * (size_t)prev_total_customers);
                win_realloc(animate_customer, sizeof(BOOL) * (size_t)prev_total_customers);
            }
            tmp = GetBarberState(barber);
            if (tmp != barber_state) {
                barber_state = tmp;
                animate_barber = TRUE;
            } else {
                animate_barber = FALSE;
            }

            for (int i = 0; i < prev_total_customers; i++) {
                if (*(customer_array + customer_array_idx + i) != NULL) {
                    int tmp = (int)GetCustomerState(*(customer_array + customer_array_idx + i));
                    if (customer_states[i] != tmp) {
                        customer_states[i] = tmp;
                        animate_customer[i] = TRUE;
                    } else {
                        animate_customer[i] = FALSE;
                    }
                } else {
                    //unreachable
                    customer_states[i] = CUSTOMER_DONE;
                }
            }
#ifdef _DEBUG
            if (drawdll_func.UpdateState) drawdll_func.UpdateState(prev_total_customers, customer_states, 
                                                                   barber_state, animate_customer,
                                                                   animate_barber, animations_enabled,
                                                                   GetBarbershopDoorState());
#else
            UpdateState(prev_total_customers, customer_states, barber_state, animate_customer,
                        animate_barber, animations_enabled, GetBarbershopDoorState());
#endif
            next_game_tick += SKIP_TICKS;
            loops++;
        }
    }
    //stop the running threads
    SetEvent(KillAllThreadsEvt);

                /* Cleanup goes here */
    for (int i = customer_array_idx; i <= prev_total_customers; i++) {
        if (*(customer_array + i)) {
            WaitForSingleObject((*(customer_array + i))->hthrd, INFINITE);
            DeleteCustomer(*(customer_array + i));
        }
    }
    win_free(customer_array);
    win_free(customer_states);
    win_free(animate_customer);
    deleteFIFOqueue(customer_queue, DONT_DELETE_DATA);
    if (!DeleteBarber(barber)) fprintf(stderr, "Error freeing up barber resources!\n");

#ifdef _DEBUG
    drawdll_func.CleanupGraphics();
    FreeLibrary(drawdll_func.to_load);
    DeleteFile(TEXT("draw-loaded-0.dll"));
#else
    CleanupGraphics();
#endif

    if (!DeleteBackbuffer(backbuff)) fprintf(stderr, "Error deleting backbuff!\n");

    return 0;
}
