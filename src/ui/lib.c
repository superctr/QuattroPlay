#include "../macro.h"

#include "ui.h"
#include "math.h"

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
    uint16_t c;
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
    colorsel_t fgc = 0;
    colorsel_t pbgc= 0;

    for(y=FROWS;y--;)
    {
        for(x=0;x<FCOLUMNS;x++)
        {
            bgc = screen.bgcolor[y][x];
            fgc = screen.textcolor[y][x];
            pbgc= screen.bgcolor[y-1][x];

            c=0;
            if(y)
                c = (pbgc & CFLAG_YSHIFT) != (last_screen.bgcolor[y-1][x] & CFLAG_YSHIFT);
            c|=screen.screen_dirty|screen.dirty[y][x];

            if(c || bgc != last_screen.bgcolor[y][x] ||
               fgc != last_screen.textcolor[y][x] ||
               screen.text[y][x] != last_screen.text[y][x])
            {
                yshift=0;
                if(bgc & CFLAG_YSHIFT_25)
                    yshift = yshift_25;
                else if(bgc & CFLAG_YSHIFT_50)
                    yshift = yshift_50;
                else if(bgc & CFLAG_YSHIFT_75)
                    yshift = yshift_75;

                if(yshift)
                {
                    bc = Colors[pbgc&0x7f];
                    scrp.h = FSIZE_Y-yshift;
                    if(scrp.h)
                    {
                        SDL_SetRenderDrawColor(rend,bc.red,bc.green,bc.blue,255);
                        SDL_RenderFillRect(rend,&scrp);
                    }
                    scrp.h = FSIZE_Y;
                }
                if(pbgc & CFLAG_YSHIFT)
                    screen.dirty[y-1][x]=1;

                scrp.y += yshift;

                // Draw background
                bc = Colors[bgc&0x7f];
                if(bgc&CFLAG_KEYBOARD)
                    bc = Colors[COLOR_BLACK];

                SDL_SetRenderDrawColor(rend,bc.red,bc.green,bc.blue,255);
                SDL_RenderFillRect(rend,&scrp);
                rect_count++;

                // Draw text
                c = screen.text[y][x]&0xff;

                if(c != 0x20 && c != 0x00)
                {
                    // switch to keyboard char set
                    if((fgc & CFLAG_KEYBOARD) && (c&0x80))
                    {
                        c+=0x80;
                        SDL_SetTextureBlendMode(font,SDL_BLENDMODE_NONE);
                    }
                    else
                    {
                        SDL_SetTextureBlendMode(font,SDL_BLENDMODE_BLEND);
                    }
                    tc = Colors[fgc&0x7f];
                    //SDL_SetRenderDrawColor(rend,0,0x10,0,255);
                    SDL_SetTextureColorMod(font,tc.red,tc.green,tc.blue);
                    fntp.x = (c%32)*FSIZE_X;
                    fntp.y = (c/32)*FSIZE_Y;
                    SDL_RenderCopy(rend,font,&fntp,&scrp);
                    draw_count++;
                }

                scrp.y -= yshift;

                last_screen.text[y][x] = screen.text[y][x];
                last_screen.bgcolor[y][x] = bgc;
                last_screen.textcolor[y][x] = fgc;
                screen.dirty[y][x]=0;
            }
            scrp.x += FSIZE_X;
        }
        scrp.x = 0;

        scrp.y -= FSIZE_Y;
    }

    screen.screen_dirty=0;
}

void ui_refresh()
{
    SDL_SetRenderDrawColor(rend,0,0,0,255);
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
    FSIZE_Y = surface->h/10;

    window = SDL_CreateWindow("QuattroPlay",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,FCOLUMNS*FSIZE_X,FROWS*FSIZE_Y,SDL_WINDOW_RESIZABLE);
    rend = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderSetLogicalSize(rend,FCOLUMNS*FSIZE_X,FROWS*FSIZE_Y);
    dispbuf = SDL_CreateTexture(rend,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,FCOLUMNS*FSIZE_X,FROWS*FSIZE_Y);
    #ifdef DEBUG
    SDL_RendererInfo info;
    SDL_GetRendererInfo(rend,&info);
    Q_DEBUG("Using renderer: %s\n",info.name);
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

uint8_t KeyboardPatterns[25][7] = {
    {0x00,0x01,0x02,0x01,0x03,0x00,0x03}, // blank
    {0x04,0x01,0x02,0x01,0x03,0x00,0x03}, // C
    {0x08,0x01,0x02,0x01,0x03,0x00,0x03}, // C#
    {0x0C,0x01,0x02,0x01,0x03,0x00,0x03}, // D
    {0x10,0x05,0x02,0x01,0x03,0x00,0x03}, // D#
    {0x00,0x09,0x02,0x01,0x03,0x00,0x03}, // E
    {0x00,0x0D,0x02,0x01,0x03,0x00,0x03}, // F
    {0x00,0x11,0x06,0x01,0x03,0x00,0x03}, // F#
    {0x00,0x01,0x0A,0x01,0x03,0x00,0x03}, // G
    {0x00,0x01,0x0E,0x01,0x03,0x00,0x03}, // G#
    {0x00,0x01,0x12,0x01,0x03,0x00,0x03}, // A
    {0x00,0x01,0x15,0x05,0x03,0x00,0x03}, // A#
    {0x00,0x01,0x02,0x09,0x03,0x00,0x03}, // B
    {0x00,0x01,0x02,0x0D,0x03,0x00,0x03}, // C
    {0x00,0x01,0x02,0x11,0x07,0x00,0x03}, // C#
    {0x00,0x01,0x02,0x01,0x0B,0x00,0x03}, // D
    {0x00,0x01,0x02,0x01,0x0F,0x00,0x03}, // D#
    {0x00,0x01,0x02,0x01,0x13,0x00,0x03}, // E
    {0x00,0x01,0x02,0x01,0x03,0x04,0x03}, // F
    {0x00,0x01,0x02,0x01,0x03,0x08,0x03}, // F¤
    {0x00,0x01,0x02,0x01,0x03,0x0C,0x03}, // G
    {0x00,0x01,0x02,0x01,0x03,0x10,0x07}, // G#
    {0x00,0x01,0x02,0x01,0x03,0x00,0x0B}, // A
    {0x00,0x01,0x02,0x01,0x03,0x00,0x0F}, // A#
    {0x00,0x01,0x02,0x01,0x03,0x00,0x13}, // B
};

void ui_keyboard(int y,int x,int octaves,int note)
{
    int max = ceil(octaves*3.5);
    int group,tile;
    int index=0;
    int offset=0;
    if(note>0)
    {
        note-=1;
        offset=note/24;
        index=(note%24)+1;
    }
    int i,k;
    for(i=0;i<max;i++)
    {
        group = i/7;
        tile = i%7;
        k = (group==offset) ? index : 0;

        screen.text[y  ][x] = 0x80+KeyboardPatterns[k][tile];
        screen.text[y+1][x] = 0xa0+KeyboardPatterns[k][tile];
        x++;
    }
}

void ui_array(int y,int x,int max,uint8_t* data)
{
    int i;
    for(i=0;i<max;i++)
    {
        screen.text[y  ][x] = 0x96+data[i];
        screen.text[y+1][x] = 0xb6+data[i];
        x++;
    }
}
