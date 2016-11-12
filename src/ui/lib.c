#include "ui.h"

color_t Colors[14] = {
 {0x00,0x00,0x00}, // Black
 {0xff,0xff,0xff}, // White
 {0x3f,0x3f,0x3f}, // D Grey
 {0x7f,0x7f,0x7f}, // Grey
 {0xbf,0xbf,0xbf}, // L Grey
 {0x7f,0x00,0x00}, // D Red
 {0xff,0x00,0x00}, // Red
 {0xff,0x7f,0x7f}, // L Red
 {0x00,0x7f,0x00}, // D Green
 {0x00,0xff,0x00}, // Green
 {0x7f,0xff,0x7f}, // L Green
 {0x00,0x00,0x7f}, // D Blue
 {0x00,0x00,0xff}, // Blue
 {0x7f,0x7f,0xff}, // L Blue
};

void ui_update()
{
    //SDL_SetRenderDrawColor(rend,0x10,0,0,255);
    //SDL_RenderClear(rend);

    SDL_Rect fntp;
    SDL_Rect scrp;

    int yshift;
    int yshift_25 = FSIZE_Y/4;
    int yshift_50 = FSIZE_Y/2;
    int yshift_75 = yshift_50 + yshift_25;

    int y,x;
    uint8_t c;
    color_t tc;
    color_t bc;

    rect_count=0;
    draw_count=0;

    scrp.x = 0;
    scrp.y = FSIZE_X*(FROWS-1);
    scrp.w = FSIZE_X;
    scrp.h = FSIZE_Y;

    fntp.h = FSIZE_X;
    fntp.w = FSIZE_Y;

    colorsel_t bgc = 0;

    for(y=FROWS;y--;)
    {
        for(x=0;x<FCOLUMNS;x++)
        {
            c=0;
            if(y)
                c = (screen.bgcolor[y-1][x] & CFLAG_YSHIFT) != (last_screen.bgcolor[y-1][x] & CFLAG_YSHIFT);

            if(c || screen.bgcolor[y][x] != last_screen.bgcolor[y][x] ||
               screen.textcolor[y][x] != last_screen.textcolor[y][x] ||
               screen.text[y][x] != last_screen.text[y][x])
            {
                yshift=0;
                bgc = screen.bgcolor[y][x];
                if(bgc & CFLAG_YSHIFT_25)
                    yshift = yshift_25;
                else if(bgc & CFLAG_YSHIFT_50)
                    yshift = yshift_50;
                else if(bgc & CFLAG_YSHIFT_75)
                    yshift = yshift_75;

                if(yshift)
                    scrp.y += yshift;

                // Draw background
                bc = Colors[bgc&0x7f];

                SDL_SetRenderDrawColor(rend,bc.red,bc.green,bc.blue,255);
                SDL_RenderFillRect(rend,&scrp);
                rect_count++;

                // Draw text
                c = (uint8_t)screen.text[y][x];
                if(c != 0x20 && c != 0x00)
                {
                    tc = Colors[screen.textcolor[y][x]&0x7f];
                    //SDL_SetRenderDrawColor(rend,0,0x10,0,255);
                    SDL_SetTextureColorMod(font,tc.red,tc.green,tc.blue);
                    fntp.x = (c%32)*FSIZE_X;
                    fntp.y = (c/32)*FSIZE_Y;
                    SDL_RenderCopy(rend,font,&fntp,&scrp);
                    draw_count++;
                }

                if(yshift)
                    scrp.y -= yshift;

                last_screen.text[y][x] = screen.text[y][x];
                last_screen.bgcolor[y][x] = screen.bgcolor[y][x];
                last_screen.textcolor[y][x] = screen.textcolor[y][x];
            }
            scrp.x += FSIZE_X;
        }
        scrp.x = 0;

        scrp.y -= FSIZE_Y;
    }
}

void ui_refresh()
{
    SDL_SetRenderTarget(rend,NULL);
    SDL_RenderClear(rend);
    SDL_RenderCopy(rend,dispbuf,NULL,NULL);
    SDL_RenderPresent(rend);
}

void ui_color(int y,int x,int h,int w,colorsel_t bg,colorsel_t fg)
{
    int i,j,k,l;
    k=h+y;
    l=w+x;
    for(i=y;i<k;i++)
    {
        for(j=x;j<l;j++)
        {
            screen.textcolor[i][j]=fg;
            screen.bgcolor[i][j]=bg;
        }
    }
}

int ui_init()
{
    set_color(0,0,FROWS,FCOLUMNS,COLOR_D_BLUE,COLOR_N_GREEN);

    SDL_Surface *surface;
    surface = SDL_LoadBMP("font.bmp");
    if(!surface)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Error","could not load 'font.bmp'",NULL);
        return -1;
    }
    FSIZE_X = surface->w/32;
    FSIZE_Y = surface->h/8;

    window = SDL_CreateWindow("QuattroPlay",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,FCOLUMNS*FSIZE_X,FROWS*FSIZE_Y,0);
    rend = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    dispbuf = SDL_CreateTexture(rend,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,FCOLUMNS*FSIZE_X,FROWS*FSIZE_Y);
    #ifdef DEBUG
    SDL_RendererInfo info;
    SDL_GetRendererInfo(rend,&info);
    printf("Using renderer: %s\n",info.name);
    #endif

    SDL_SetRenderDrawColor(rend,0,0,0,255);
    SDL_RenderClear(rend);
    SDL_RenderPresent(rend);

    SDL_SetRenderTarget(rend,dispbuf);
    SDL_SetRenderDrawColor(rend,0,0,0,255);
    SDL_RenderClear(rend);

    SDL_SetColorKey(surface,SDL_TRUE,0);
    font = SDL_CreateTextureFromSurface(rend,surface);

    SDL_FreeSurface(surface);

    return 0;
}

void ui_deinit()
{
    SDL_DestroyTexture(font);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(window);
}

void ui_clear(colorsel_t bg,colorsel_t fg)
{




}
