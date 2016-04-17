/***********************************************\
*                     draw.c                    *
*           George Koskeridis (C) 2016          *
\***********************************************/

#include "draw.h"

#define CUSTOMER_CHAIRS  5

#define WALL_VERTICES         12
#define DOOR_VERTICES          4
#define BARBERCHAIR_VERTICES   7
#define CHAIR_VERTICES         8

static const POINT lowres_wall_vertices[WALL_VERTICES] = {
    {600, 450}, {600, 30}, {280, 30}, 
    {280, 60}, {290, 60}, {290, 40},
    {590, 40}, {590, 440}, {290, 440}, 
    {290, 130}, {280, 130}, {280, 450}
}, lowres_closed_door_vertices[DOOR_VERTICES] = {
    {300, 130}, {290, 130}, {290, 60}, {300, 60}
}, lowres_opened_door_vertices[DOOR_VERTICES] = {
    {358, 41}, {359, 51}, {290, 60}, {288, 50}
}, lowres_barberchair_vertices[BARBERCHAIR_VERTICES] = {
    {450, 330}, {460, 330}, {430, 390}, {430, 395},
    {390, 395}, {390, 390}, {425, 390}
}, lowres_customerchair_vertices[CUSTOMER_CHAIRS][CHAIR_VERTICES] = {
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
};


static POINT scaled_wall[WALL_VERTICES], scaled_door[DOOR_VERTICES],
             scaled_barberchair[BARBERCHAIR_VERTICES],
             scaled_customerchair[CUSTOMER_CHAIRS][CHAIR_VERTICES];
static BOOL barbershop_door_open = FALSE;
static int scaled_pen_thickness;


#if 0
static POINT RotatePoint(POINT axis, POINT vertex, double angle)
{
    // en.wikipedia.org/wiki/Rotation_matrix
    double tmp_x = ((vertex.x - axis.x) * cos(angle)) - ((vertex.y - axis.y) * sin(angle));
    double tmp_y = ((vertex.x - axis.x) * sin(angle)) + ((vertex.y - axis.y) * cos(angle));

    vertex.x = (LONG)(tmp_x + axis.x);
    vertex.y = (LONG)(tmp_y + axis.y);

    return vertex;
}

//used this to get the (x,y) values for the open door vertices
static void GetRotatedDoorPoints(double angle)
{
    POINT tmp;
    tmp = RotatePoint(lowres_closed_door_vertices[2], lowres_closed_door_vertices[0], angle);
    printf("0 %ld:%ld\n", tmp.x, tmp.y);

    tmp = RotatePoint(lowres_closed_door_vertices[2], lowres_closed_door_vertices[1], angle);
    printf("1 %ld:%ld\n", tmp.x, tmp.y);

    tmp = RotatePoint(lowres_closed_door_vertices[2], lowres_closed_door_vertices[3], angle);
    printf("3 %ld:%ld\n", tmp.x, tmp.y);
}
#endif

void SetBarbershopDoorState(BOOL new_state)
{
    barbershop_door_open = new_state;
}

BOOL GetBarbershopDoorState(void)
{
    return barbershop_door_open;
}

void StretchGraphics(int scaling_idx)
{
    scaled_pen_thickness = scaling_idx + 1;

    for (int i = 0; i < WALL_VERTICES; i++) {
        if (!scaling_idx) {
            scaled_wall[i] = lowres_wall_vertices[i];

            if (i < DOOR_VERTICES) {
                scaled_door[i] = (barbershop_door_open) ? lowres_opened_door_vertices[i] : lowres_closed_door_vertices[i];
            }

            if (i < BARBERCHAIR_VERTICES) {
                scaled_barberchair[i] = lowres_barberchair_vertices[i];
            }

            if (i < CHAIR_VERTICES) {
                for (int j = 0; j < CUSTOMER_CHAIRS; j++)
                    scaled_customerchair[j][i] = lowres_customerchair_vertices[j][i];
            }
        } else {
            double tmp = (double)(lowres_wall_vertices[i].x * (pow(1.25, (double)scaling_idx)));
            scaled_wall[i].x = (LONG)tmp;

            tmp = (double)(lowres_wall_vertices[i].y * (pow(1.25, (double)scaling_idx)));
            scaled_wall[i].y = (LONG)tmp;

            if (i < DOOR_VERTICES) {
                tmp = (double)(((barbershop_door_open) ? lowres_opened_door_vertices[i].x : lowres_closed_door_vertices[i].x)
                               * (pow(1.25, (double)scaling_idx)));
                scaled_door[i].x = (LONG)tmp;

                tmp = (double)(((barbershop_door_open) ? lowres_opened_door_vertices[i].y : lowres_closed_door_vertices[i].y)
                               * (pow(1.25, (double)scaling_idx)));
                scaled_door[i].y = (LONG)tmp;
            }

            if (i < BARBERCHAIR_VERTICES) {
                tmp = (double)(lowres_barberchair_vertices[i].x * (pow(1.25, (double)scaling_idx)));
                scaled_barberchair[i].x = tmp;

                tmp = (double)(lowres_barberchair_vertices[i].y * (pow(1.25, (double)scaling_idx)));
                scaled_barberchair[i].y = tmp;
            }

            if (i < CHAIR_VERTICES) {
                for (int j = 0; j < CUSTOMER_CHAIRS; j++) {
                    tmp = (double)(lowres_customerchair_vertices[j][i].x * (pow(1.25, (double)scaling_idx)));
                    scaled_customerchair[j][i].x = tmp;

                    tmp = (double)(lowres_customerchair_vertices[j][i].y * (pow(1.25, (double)scaling_idx)));
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

    HGDIOBJ prev_pen, prev_brush;

    //background color
    PatBlt(buf->hdc, 0, 0, buf->coords.right, buf->coords.bottom, BLACKNESS);
    FillRect(buf->hdc, &buf->coords, (HBRUSH)GetStockObject(DKGRAY_BRUSH));

    //barbershop
    {
        LOGBRUSH wall_line_attr = {BS_SOLID, RGB_BLACK, NULL};
        HPEN boundary_pen = ExtCreatePen(PS_GEOMETRIC | PS_INSIDEFRAME | PS_ENDCAP_ROUND | PS_JOIN_BEVEL,
                                         scaled_pen_thickness, &wall_line_attr, 0, NULL);
        HBRUSH wallfilling_brush = CreateHatchBrush(HS_DIAGCROSS, RGB_BLACK);
        //draw the walls
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
        Polygon(buf->hdc, scaled_door, DOOR_VERTICES);

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
        for (int j = 0; j < CUSTOMER_CHAIRS; j++)
            Polygon(buf->hdc, scaled_customerchair[j], CHAIR_VERTICES);

        SelectObject(buf->hdc, prev_pen);
        SelectObject(buf->hdc, prev_brush);
        DeleteObject((HGDIOBJ)chair_pen);
        DeleteObject((HGDIOBJ)chair_brush);
    }
}

void UpdateState(void)
{
    
}
