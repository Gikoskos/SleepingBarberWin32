/***********************************************\
*                     draw.c                    *
*           George Koskeridis (C) 2016          *
\***********************************************/

#include "draw.h"
#include "barber.h"
#include "customer.h"
#include "main.h"

#define WALL_VERTICES         12
#define DOOR_VERTICES          4
#define BARBERCHAIR_VERTICES   7
#define CHAIR_VERTICES         8

#define TOTAL_BARBER_PEN_STYLES 6

//animated graphics
static RECT barber_graphic, *customer_graphic = NULL;
static int animation_delta = 10;


//unchanged graphics
static const POINT lowres_wall_vertices[WALL_VERTICES] = { //barbershop wall
    {600, 450}, {600, 30}, {280, 30},
    {280, 60}, {290, 60}, {290, 40},
    {590, 40}, {590, 440}, {290, 440},
    {290, 130}, {280, 130}, {280, 450}
}, lowres_closed_door_vertices[DOOR_VERTICES] = { //door when it's closed
    {300, 130}, {290, 130}, {290, 60}, {300, 60}
}, lowres_opened_door_vertices[DOOR_VERTICES] = { //door when it's open
    {358, 41}, {359, 51}, {290, 60}, {288, 50}
}, lowres_barberchair_vertices[BARBERCHAIR_VERTICES] = { //barber's chair
    {450, 330}, {460, 330}, {430, 390}, {430, 395},
    {390, 395}, {390, 390}, {425, 390}
}, lowres_customerchair_vertices[CUSTOMER_CHAIRS][CHAIR_VERTICES] = { //chairs for the customers; waiting room
    {
        {525, 45}, {588, 45}, {588, 105}, {525, 105},
        {525, 95}, {578, 95}, {578, 55}, {525, 55}
    },
    {
        {525, 110}, {588, 110}, {588, 170}, {525, 170},
        {525, 160}, {578, 160}, {578, 120}, {525, 120}
    },
    {
        {525, 175}, {588, 175}, {588, 235}, {525, 235},
        {525, 225}, {578, 225}, {578, 185}, {525, 185}
    },
    {
        {525, 240}, {588, 240}, {588, 300}, {525, 300},
        {525, 290}, {578, 290}, {578, 250}, {525, 250}
    },
    {
        {525, 305}, {588, 305}, {588, 365}, {525, 365},
        {525, 355}, {578, 355}, {578, 315}, {525, 315}
    }
}, lowres_character_dimension = {40, 40}; //width and height of the barber and customers' graphics


static POINT scaled_wall[WALL_VERTICES],
             scaled_opened_door[DOOR_VERTICES],
             scaled_closed_door[DOOR_VERTICES],
             scaled_barberchair[BARBERCHAIR_VERTICES],
             scaled_customerchair[CUSTOMER_CHAIRS][CHAIR_VERTICES],
             scaled_character_dimension;

//are automatically 0'd out so no need to FALSE all the fields
static BOOL barbershop_door_state = FALSE, chair_occupied[CUSTOMER_CHAIRS];

static int scaled_pen_thickness, scaled_font_size;

const int barber_pen_styles[TOTAL_BARBER_PEN_STYLES] = {PS_DOT, PS_DASHDOT, PS_DASHDOTDOT, PS_DASHDOT, PS_DOT, PS_SOLID};
static int curr_barber_pen_style;

static int current_numofcustomers;

/* Prototypes for functions with local scope */
static RECT GetOnBarberChairRect(void);
static RECT GetNextToBarberChairRect(void);
static RECT GetOnEmptyCustomerChairRect(void);
static RECT GetNextToWaitingRoomRect(void);
static RECT GetCustomerQueuePositionRect(UINT position_idx);
static RECT GetInvalidPositionRect(void);
#if 0
static POINT RotatePoint(POINT axis, POINT vertex, double angle);
static void GetRotatedDoorPoints(double angle);


static POINT RotatePoint(POINT axis, POINT vertex, double angle)
{
    // en.wikipedia.org/wiki/Rotation_matrix
    double tmp_x = ((vertex.x - axis.x) * cos(angle)) - ((vertex.y - axis.y) * sin(angle));
    double tmp_y = ((vertex.x - axis.x) * sin(angle)) + ((vertex.y - axis.y) * cos(angle));

    vertex.x = (LONG)(tmp_x + axis.x);
    vertex.y = (LONG)(tmp_y + axis.y);

    return vertex;
}

//I used this to get the (x,y) values for lowres_opened_door_vertices
static void GetRotatedDoorPoints(double angle)
{
    POINT tmp = RotatePoint(lowres_closed_door_vertices[2], lowres_closed_door_vertices[0], angle);
    printf("0 %ld:%ld\n", tmp.x, tmp.y);

    tmp = RotatePoint(lowres_closed_door_vertices[2], lowres_closed_door_vertices[1], angle);
    printf("1 %ld:%ld\n", tmp.x, tmp.y);

    tmp = RotatePoint(lowres_closed_door_vertices[2], lowres_closed_door_vertices[3], angle);
    printf("3 %ld:%ld\n", tmp.x, tmp.y);
}
#endif

void ScaleGraphics(int scaling_exp)
{
    const double scaling_base = 1.25;

    animation_delta = (int)(10 * (pow(scaling_base, (double)scaling_exp)));

    scaled_pen_thickness = scaling_exp + 1;
    scaled_font_size = (int)(14 * (pow(scaling_base, (double)scaling_exp)));
    scaled_character_dimension.x = (int)(lowres_character_dimension.x * (pow(scaling_base, (double)scaling_exp)));
    scaled_character_dimension.y = (int)(lowres_character_dimension.y * (pow(scaling_base, (double)scaling_exp)));

    for (int i = 0; i < WALL_VERTICES; i++) {
        if (!scaling_exp) {
            scaled_wall[i] = lowres_wall_vertices[i];

            if (i < DOOR_VERTICES) {
                scaled_opened_door[i] = lowres_opened_door_vertices[i];
                scaled_closed_door[i] = lowres_closed_door_vertices[i];
            }

            if (i < BARBERCHAIR_VERTICES) {
                scaled_barberchair[i] = lowres_barberchair_vertices[i];
            }

            if (i < CHAIR_VERTICES) {
                for (int j = 0; j < CUSTOMER_CHAIRS; j++)
                    scaled_customerchair[j][i] = lowres_customerchair_vertices[j][i];
            }
        } else {
            double tmp = (double)(lowres_wall_vertices[i].x * (pow(scaling_base, (double)scaling_exp)));
            scaled_wall[i].x = (LONG)tmp;

            tmp = (double)(lowres_wall_vertices[i].y * (pow(scaling_base, (double)scaling_exp)));
            scaled_wall[i].y = (LONG)tmp;

            if (i < DOOR_VERTICES) {
                tmp = (double)(lowres_opened_door_vertices[i].x * (pow(scaling_base, (double)scaling_exp)));
                scaled_opened_door[i].x = (LONG)tmp;

                tmp = (double)(lowres_opened_door_vertices[i].y * (pow(scaling_base, (double)scaling_exp)));
                scaled_opened_door[i].y = (LONG)tmp;

                tmp = (double)(lowres_closed_door_vertices[i].x * (pow(scaling_base, (double)scaling_exp)));
                scaled_closed_door[i].x = (LONG)tmp;

                tmp = (double)(lowres_closed_door_vertices[i].y * (pow(scaling_base, (double)scaling_exp)));
                scaled_closed_door[i].y = (LONG)tmp;
            }

            if (i < BARBERCHAIR_VERTICES) {
                tmp = (double)(lowres_barberchair_vertices[i].x * (pow(scaling_base, (double)scaling_exp)));
                scaled_barberchair[i].x = tmp;

                tmp = (double)(lowres_barberchair_vertices[i].y * (pow(scaling_base, (double)scaling_exp)));
                scaled_barberchair[i].y = tmp;
            }

            if (i < CHAIR_VERTICES) {
                for (int j = 0; j < CUSTOMER_CHAIRS; j++) {
                    tmp = (double)(lowres_customerchair_vertices[j][i].x * (pow(scaling_base, (double)scaling_exp)));
                    scaled_customerchair[j][i].x = tmp;

                    tmp = (double)(lowres_customerchair_vertices[j][i].y * (pow(scaling_base, (double)scaling_exp)));
                    scaled_customerchair[j][i].y = tmp;
                }
            }
        }
    }
}

void DrawBufferToWindow(HWND to, backbuffer_data *from)
{
    HDC hdc = GetDC(to);

    if (!hdc) return;

    BitBlt(hdc, 0, 0, from->coords.right, from->coords.bottom, from->hdc, 0, 0, SRCCOPY);
    ReleaseDC(to, hdc);
}

void DrawToBuffer(backbuffer_data *buf)
{
    if (buf == NULL) return;

    HGDIOBJ prev_pen, prev_brush, prev_font;

    //background color
    PatBlt(buf->hdc, 0, 0, buf->coords.right, buf->coords.bottom, BLACKNESS);
    FillRect(buf->hdc, &buf->coords, (HBRUSH)GetStockObject(DKGRAY_BRUSH));

    //text
    {
        TCHAR text1[] = _T("B = barber  |  C = customers  |  Close door with spacebar or 'O'"),
              text2[] = _T("B = barber  |  C = customers  |  Open door with spacebar or 'O'");
        HFONT apx_font = CreateFont(scaled_font_size, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    ANTIALIASED_QUALITY, FF_DONTCARE, _T("Verdana"));
        prev_font = SelectFont(buf->hdc, apx_font);

        SetTextColor(buf->hdc, RGB_WHITE);
        SetBkColor(buf->hdc, GetPixel(buf->hdc, 0, 0));

        TextOut(buf->hdc, 10, 10, (barbershop_door_state) ? text1 : text2,
                (barbershop_door_state) ? ARRAYSIZE(text1) : ARRAYSIZE(text2));

        SelectFont(buf->hdc, prev_font);
        DeleteFont(apx_font);
    }

    //draw the barbershop walls
    {
        LOGBRUSH wall_line_attr = {BS_SOLID, RGB_BLACK, NULL};
        HPEN boundary_pen = ExtCreatePen(PS_GEOMETRIC | PS_INSIDEFRAME | PS_ENDCAP_ROUND | PS_JOIN_BEVEL,
                                         scaled_pen_thickness, &wall_line_attr, 0, NULL);
        HBRUSH wallfilling_brush = CreateHatchBrush(HS_DIAGCROSS, RGB_BLACK);
        prev_pen = SelectObject(buf->hdc, boundary_pen);
        prev_brush = SelectObject(buf->hdc, wallfilling_brush);

        SetBkColor(buf->hdc, RGB_PURPLE);
        Polygon(buf->hdc, scaled_wall, WALL_VERTICES);

        SelectObject(buf->hdc, prev_brush);
        DeleteObject((HGDIOBJ)wallfilling_brush);
    }

    //draw the door with the same pen but different brush
    {
        HBRUSH doorfilling_brush = CreateSolidBrush(RGB_GREEN);
        prev_brush = SelectObject(buf->hdc, doorfilling_brush);

        Polygon(buf->hdc, (barbershop_door_state) ? scaled_opened_door : scaled_closed_door, DOOR_VERTICES);

        SelectObject(buf->hdc, prev_brush);
        prev_pen = SelectObject(buf->hdc, prev_pen);
        DeleteObject((HGDIOBJ)doorfilling_brush);
        DeleteObject((HGDIOBJ)prev_pen);
    }

    //draw the barber chair
    {
        HPEN chair_pen = CreatePen(PS_DOT, 1, RGB_BLACK);
        HBRUSH chair_brush = CreateSolidBrush(RGB_GREEN);
        prev_pen = SelectObject(buf->hdc, chair_pen);
        prev_brush = SelectObject(buf->hdc, chair_brush);

        SetBkColor(buf->hdc, RGB_GREEN);
        Polygon(buf->hdc, scaled_barberchair, BARBERCHAIR_VERTICES);

        SelectObject(buf->hdc, prev_pen);
        SelectObject(buf->hdc, prev_brush);
        DeleteObject((HGDIOBJ)chair_pen);
        DeleteObject((HGDIOBJ)chair_brush);
    }

    //draw the customer chairs
    {
        HPEN chair_pen = CreatePen(PS_SOLID, (scaled_pen_thickness - 1), RGB_BLACK);
        HBRUSH chair_brush = CreateSolidBrush(RGB_ORANGE);
        prev_pen = SelectObject(buf->hdc, chair_pen);
        prev_brush = SelectObject(buf->hdc, chair_brush);

        SetBkColor(buf->hdc, RGB_GREEN);
        for (int i = 0; i < CUSTOMER_CHAIRS; i++)
            Polygon(buf->hdc, scaled_customerchair[i], CHAIR_VERTICES);

        SelectObject(buf->hdc, prev_pen);
        SelectObject(buf->hdc, prev_brush);
        DeleteObject((HGDIOBJ)chair_pen);
        DeleteObject((HGDIOBJ)chair_brush);
    }

    //draw the characters
    {
        TCHAR barber_text[] = _T(" B"), customer_text[] = _T(" C");
        HPEN barber_pen = CreatePen(barber_pen_styles[curr_barber_pen_style], 1, RGB_RED),
             customer_pen = CreatePen(PS_SOLID, 1, RGB_YELLOW);
        HBRUSH barber_brush = CreateSolidBrush(RGB_PURPLEBLUE), 
               customer_brush = CreateSolidBrush(RGB_BLUE);
        HFONT character_font = CreateFont(scaled_font_size + 10, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE,
                                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                          ANTIALIASED_QUALITY, FF_DONTCARE, _T("Verdana"));
        prev_font = SelectFont(buf->hdc, character_font);
        prev_pen = SelectObject(buf->hdc, barber_pen);
        prev_brush = SelectObject(buf->hdc, barber_brush);

        //draw the barber
        Ellipse(buf->hdc, barber_graphic.left, barber_graphic.top, 
                barber_graphic.right, barber_graphic.bottom);
        SetBkColor(buf->hdc, RGB_PURPLEBLUE);
        DrawText(buf->hdc, barber_text, ARRAYSIZE(barber_text), &barber_graphic,
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        //draw the customers
        SelectObject(buf->hdc, customer_brush);
        DeleteObject((HGDIOBJ)barber_brush);

        SelectObject(buf->hdc, customer_pen);
        DeleteObject((HGDIOBJ)barber_pen);
        for (int i = 0; i < current_numofcustomers; i++) {
            Ellipse(buf->hdc, customer_graphic[i].left, customer_graphic[i].top, 
                    customer_graphic[i].right, customer_graphic[i].bottom);
            SetBkColor(buf->hdc, RGB_BLUE);
            DrawText(buf->hdc, customer_text, ARRAYSIZE(customer_text),
                     &customer_graphic[i], DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        SelectFont(buf->hdc, prev_font);
        SelectObject(buf->hdc, prev_pen);
        SelectObject(buf->hdc, prev_brush);
        DeleteFont(character_font);
        DeleteObject((HGDIOBJ)customer_brush);
        DeleteObject((HGDIOBJ)customer_pen);
    }
}

static int GetNumOfEmptyChairs(void)
{
    int retvalue = 0;

    for (int i = 0; i < CUSTOMER_CHAIRS; i++)
        if (chair_occupied[i] == FALSE) retvalue++;

    return retvalue;
}

static int GetNextEmptyChairIndex(void)
{
    for (int i = 0; i < CUSTOMER_CHAIRS; i++) {
        if (chair_occupied[i] == FALSE) {
            chair_occupied[i] = TRUE;
            return i;
        }
    }

    return -1;
}

static RECT GetOnBarberChairRect(void)
{
    RECT retvalue;

    retvalue.left = scaled_barberchair[0].x - scaled_character_dimension.x * 1.5;
    retvalue.top = scaled_barberchair[0].y + scaled_character_dimension.y / 3;
    retvalue.right = retvalue.left + scaled_character_dimension.x;
    retvalue.bottom = retvalue.top + scaled_character_dimension.y;
    return retvalue;
}

static RECT GetNextToBarberChairRect(void)
{
    RECT retvalue;

    retvalue.left = scaled_barberchair[4].x - scaled_character_dimension.x * 1.09;
    retvalue.top = scaled_barberchair[4].y - 1.6*scaled_character_dimension.y;
    retvalue.right = retvalue.left + scaled_character_dimension.x;
    retvalue.bottom = retvalue.top + scaled_character_dimension.y;
    return retvalue;
}

static RECT GetOnEmptyCustomerChairRect(void)
{
    RECT retvalue;

    SetRectEmpty(&retvalue);

    if (!GetNumOfEmptyChairs()) return retvalue;

    int empty_chair = GetNextEmptyChairIndex();

    if (empty_chair < 0) return GetInvalidPositionRect();

    chair_occupied[empty_chair] = TRUE;
    //adding a scaled value here to push the character into the chair for all resolutions
    retvalue.left = scaled_customerchair[empty_chair][CHAIR_VERTICES - 1].x + scaled_character_dimension.x / 3;
    retvalue.top = scaled_customerchair[empty_chair][CHAIR_VERTICES - 1].y;
    retvalue.right = retvalue.left + scaled_character_dimension.x;
    retvalue.bottom = retvalue.top + scaled_character_dimension.y;
    
    return retvalue;
}

static RECT GetNextToWaitingRoomRect(void)
{
    RECT retvalue;

    retvalue.left = scaled_customerchair[1][3].x - 2*scaled_character_dimension.x;
    retvalue.top = scaled_customerchair[1][3].y;
    retvalue.right = retvalue.left + scaled_character_dimension.x;
    retvalue.bottom = retvalue.top + scaled_character_dimension.y;
    return retvalue;
}

static RECT GetCustomerQueuePositionRect(UINT position_idx)
{
    RECT shop_queue_start = {
        .top = ((scaled_wall[10].y + scaled_wall[3].y)/2 - scaled_character_dimension.y/2),
        .left = (scaled_wall[10].x - scaled_character_dimension.x*1.2 - 
                                     scaled_character_dimension.x*1.2*position_idx)
    };

    shop_queue_start.right = shop_queue_start.left + scaled_character_dimension.x;
    shop_queue_start.bottom = shop_queue_start.top + scaled_character_dimension.y;

    return shop_queue_start;
}

static RECT GetInvalidPositionRect(void)
{
    RECT retvalue;

    retvalue.left = retvalue.right = -scaled_character_dimension.x;
    retvalue.top = retvalue.bottom = -scaled_character_dimension.y;
    return retvalue;
}

void UpdateState(LONG num_of_customers, int *customer_states, int barber_state,
                 BOOL *animate_customer, BOOL animate_barber, BOOL enable_animations,
                 BOOL barbershop_door_is_open)
{
    //these values are used in animating the characters
    static BOOL start_animating_customer[8]; //using an automatic array for now
    static BOOL start_animating_barber;

    barbershop_door_state = barbershop_door_is_open;

    for (int i = 0; i < CUSTOMER_CHAIRS; i++) chair_occupied[i] = FALSE;

    if ((num_of_customers != current_numofcustomers) && (num_of_customers >= 0)) {
        //CleanupGraphics();
        current_numofcustomers = num_of_customers;

        if (current_numofcustomers) {
            if (!customer_graphic) {
                customer_graphic = win_malloc(sizeof(RECT)*current_numofcustomers);
            } else {
                customer_graphic = win_realloc(customer_graphic, sizeof(RECT)*current_numofcustomers);
            }
        }
    }

    RECT next_to_draw;
    curr_barber_pen_style = TOTAL_BARBER_PEN_STYLES - 1;
    switch (barber_state) {
        case SLEEPING:
            next_to_draw = GetOnBarberChairRect();
            break;
        case CUTTING_HAIR:
            curr_barber_pen_style = (curr_barber_pen_style < TOTAL_BARBER_PEN_STYLES - 1) ? (curr_barber_pen_style + 1) : 0;
            next_to_draw = GetNextToBarberChairRect();
            break;
        case CHECKING_WAITING_ROOM:
            next_to_draw = GetNextToWaitingRoomRect();
            break;
        default:
            next_to_draw = GetInvalidPositionRect();
            break;
    }
    //if the position of the barber changed
    if (EqualRect(&barber_graphic, &next_to_draw) == FALSE) {
        //we check to see if animations are enabled before we can move him
        if (enable_animations) {
            start_animating_barber = TRUE;
        } else {
            CopyRect(&barber_graphic, &next_to_draw);
        }
    }

    if (start_animating_barber && enable_animations) {
        int connected_points = 0;

        if (barber_graphic.left < next_to_draw.left - animation_delta)
            barber_graphic.left += animation_delta;
        else if (barber_graphic.left > next_to_draw.left + animation_delta)
            barber_graphic.left -= animation_delta;
        else
            connected_points++;

        if (barber_graphic.top < next_to_draw.top - animation_delta)
            barber_graphic.top += animation_delta;
        else if (barber_graphic.top > next_to_draw.top + animation_delta)
            barber_graphic.top -= animation_delta;
        else
            connected_points++;

        if (barber_graphic.right < next_to_draw.right - animation_delta)
            barber_graphic.right += animation_delta;
        else if (barber_graphic.right > next_to_draw.right + animation_delta)
            barber_graphic.right -= animation_delta;
        else
            connected_points++;

        if (barber_graphic.bottom < next_to_draw.bottom - animation_delta)
            barber_graphic.bottom += animation_delta;
        else if (barber_graphic.bottom > next_to_draw.bottom + animation_delta)
            barber_graphic.bottom -= animation_delta;
        else
            connected_points++;

        if (connected_points == 4) {
            CopyRect(&barber_graphic, &next_to_draw);
            start_animating_barber = FALSE;
        }
    }

    //TODO: fix empty chair count
    int next_queue_pos = 0;
    for (int i = 0; i < current_numofcustomers; i++) {
        switch (customer_states[i]) {
            case WAITTING_IN_QUEUE:
                next_to_draw = GetCustomerQueuePositionRect(next_queue_pos++);
                break;
            case WAKING_UP_BARBER:
                next_to_draw = GetNextToBarberChairRect();
                break;
            case SITTING_IN_WAITING_ROOM:
                next_to_draw = GetOnEmptyCustomerChairRect();
                if (!next_to_draw.left) //failsafe
                    next_to_draw = GetInvalidPositionRect(); //unreachable
                break;
            case GETTING_HAIRCUT:
                next_to_draw = GetOnBarberChairRect();
                break;
            case CUSTOMER_DONE:
            default:
                next_to_draw = GetInvalidPositionRect();
                break;
        }

        if (EqualRect(&customer_graphic[i], &next_to_draw) == FALSE) {
            if (enable_animations) {
                start_animating_customer[i] = TRUE;
            } else {
                CopyRect(&customer_graphic[i], &next_to_draw);
            }
        }

        if (start_animating_customer[i] && enable_animations) {
            int connected_points = 0;

            if (customer_graphic[i].left < next_to_draw.left - animation_delta)
                customer_graphic[i].left += animation_delta;
            else if (customer_graphic[i].left > next_to_draw.left + animation_delta)
                customer_graphic[i].left -= animation_delta;
            else
                connected_points++;

            if (customer_graphic[i].top < next_to_draw.top - animation_delta)
                customer_graphic[i].top += animation_delta;
            else if (customer_graphic[i].top > next_to_draw.top + animation_delta)
                customer_graphic[i].top -= animation_delta;
            else
                connected_points++;

            if (customer_graphic[i].right < next_to_draw.right - animation_delta)
                customer_graphic[i].right += animation_delta;
            else if (customer_graphic[i].right > next_to_draw.right + animation_delta)
                customer_graphic[i].right -= animation_delta;
            else
                connected_points++;

            if (customer_graphic[i].bottom < next_to_draw.bottom - animation_delta)
                customer_graphic[i].bottom += animation_delta;
            else if (customer_graphic[i].bottom > next_to_draw.bottom + animation_delta)
                customer_graphic[i].bottom -= animation_delta;
            else
                connected_points++;

            if (connected_points == 4) {
                CopyRect(&customer_graphic[i], &next_to_draw);
                start_animating_customer[i] = FALSE;
            }
        }
    }
}

void CleanupGraphics(void)
{
    if (customer_graphic) win_free(customer_graphic);

    customer_graphic = NULL;
}
