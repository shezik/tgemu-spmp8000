/* text.c - bitmap font rendering routines; part of the TGEmu SPMP port
 *
 * Copyright (C) 2012, Ulrich Hecht <ulrich.hecht@gmail.com>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */


#include "text.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgame.h>

#define CHINESE

static uint8_t *asc12_font = 0;
#ifdef CHINESE
static uint16_t *hzx12_font = 0;
#include "hzktable.c"
#endif

int load_fonts(void)
{
    int fd = open("/Rom/mw/fonts/CHINESE/ASC12", O_RDONLY);
    if (!fd) {
        return -1;
    }
    struct stat st;
    fstat(fd, &st);
    asc12_font = malloc(st.st_size);
    if (!asc12_font || read(fd, asc12_font, st.st_size) != st.st_size) {
        return -2;
    }
    close(fd);

#ifdef CHINESE
    fd = open("/Rom/mw/fonts/CHINESE/HZX12", O_RDONLY);
    if (!fd) {
        return -3;
    }
    fstat(fd, &st);
    hzx12_font = malloc(st.st_size);
    if (!hzx12_font || read(fd, hzx12_font, st.st_size) != st.st_size) {
        return -4;
    }
    close(fd);
#endif
    return 0;
}

void free_fonts(void)
{
    free(asc12_font);
    asc12_font = 0;
#ifdef CHINESE
    free(hzx12_font);
    hzx12_font = 0;
#endif
}

int draw_character(uint32_t codepoint, int x, int y)
{
    return draw_character_ex(gDisplayDev->getShadowBuffer(),
                             gDisplayDev->getWidth(), codepoint, x, y);
}

int draw_character_ex(uint16_t *buf, int width, uint32_t codepoint, int x, int y)
{
    uint16_t *fb = buf + width * y + x;
    int i, j;
    if (codepoint < 256) {
render_asc:
        for (i = 0; i < 12; i++) {
            uint8_t line = asc12_font[codepoint * 12 + i];
            for (j = 0; j < 8; j++) {
                fb[j] = (line & 0x80) ? 0xffff : 0;
                line <<= 1;
            }
            fb += width;
        }
        return 8;
    }
#ifdef CHINESE
    else if (codepoint >= 0x4e00 && codepoint < 0x10000) {
        /* convert unicode to Big5 codepoint */
        for (i = 0; i < 13710; i++) {
            if (hzk2uni[i] == codepoint) {
                codepoint = i;
                break;
            }
        }
        if (i == 13710) {
            codepoint = 1;
            goto render_asc;
        }

        for (i = 0; i < 12; i++) {
            uint16_t line = hzx12_font[codepoint * 12 + i];
            line = (line >> 8) | (line << 8);
            for (j = 0; j < 16; j++) {
                fb[j] = (line & 0x8000) ? 0xffff : 0;
                line <<= 1;
            }
            fb += width;
        }
        return 16;
    }
#endif
    else {
        codepoint = 1;
        goto render_asc;
    }
}

int render_text(const char *t, int x, int y)
{
    return render_text_ex(gDisplayDev->getShadowBuffer(), gDisplayDev->getWidth(), t, x, y);
}

int render_text_ex(uint16_t *buf, int width, const char *t, int x, int y)
{
    int old_x = x;
    const uint8_t *text = (uint8_t *)t;
    while (*text) {
        uint32_t codepoint;
        if (*text >= 0x80) {
            if ((*text & 0x40) == 0) {
                /* invalid UTF-8 */
                text++;
                continue;
            }
            uint8_t first = text[0];
            uint8_t bytes = 0;
            while (first & 0x80) {
                bytes++;
                first <<= 1;
            }
            if (bytes > 6) {
                /* invalid UTF-8 */
                text++;
                continue;
            }
            first >>= bytes;
            int i;
            codepoint = first;
            for (i = 1; i < bytes; i++) {
                codepoint = (codepoint << 6) | (text[i] & 0x3f);
            }
            text += bytes;
        }
        else {
            codepoint = *text;
            text++;
        }
        x += draw_character_ex(buf, width, codepoint, x, y);
    }
    return x - old_x;
}
