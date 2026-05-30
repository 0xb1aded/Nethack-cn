/* NetHack 5.0  rip.c   $NHDT-Date: 1597967808 2020/08/20 23:56:48 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.33 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2017. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* Defining TEXT_TOMBSTONE causes genl_outrip() to exist, but it doesn't
   necessarily have to be used by a binary with multiple window-ports */

#if defined(TTY_GRAPHICS) || defined(X11_GRAPHICS) || defined(GEM_GRAPHICS) \
    || defined(DUMPLOG) || defined(CURSES_GRAPHICS) || defined(SHIM_GRAPHICS) \
    || defined(AMII_GRAPHICS)
#define TEXT_TOMBSTONE
#endif
#if defined(mac) || defined(__BEOS__)
#ifndef TEXT_TOMBSTONE
#define TEXT_TOMBSTONE
#endif
#endif

#ifdef TEXT_TOMBSTONE
staticfn void center(int, char *);

#ifndef NH320_DEDICATION
/* A normal tombstone for end of game display. */
static const char *const rip_txt[] = {
    "                       ----------",
    "                      /          \\",
    "                     /    REST    \\",
    "                    /      IN      \\",
    "                   /     PEACE      \\",
    "                  /                  \\",
    "                  |                  |", /* Name of player */
    "                  |                  |", /* Amount of $ */
    "                  |                  |", /* Type of death */
    "                  |                  |", /* . */
    "                  |                  |", /* . */
    "                  |                  |", /* . */
    "                  |       1001       |", /* Real year of death */
    "                 *|     *  *  *      | *",
    "        _________)/\\\\_//(\\/(/\\)/\\//\\/|_)_______", 0
};
#define STONE_LINE_CENT 28 /* char[] element of center of stone face */
#else                      /* NH320_DEDICATION */
/* NetHack 3.2.x displayed a dual tombstone as a tribute to Izchak. */
static const char *const rip_txt[] = {
    "              ----------                      ----------",
    "             /          \\                    /          \\",
    "            /    REST    \\                  /    This    \\",
    "           /      IN      \\                /  release of  \\",
    "          /     PEACE      \\              /   NetHack is   \\",
    "         /                  \\            /   dedicated to   \\",
    "         |                  |            |  the memory of   |",
    "         |                  |            |                  |",
    "         |                  |            |  Izchak Miller   |",
    "         |                  |            |   1935 - 1994    |",
    "         |                  |            |                  |",
    "         |                  |            |     Ascended     |",
    "         |       1001       |            |                  |",
    "      *  |     *  *  *      | *        * |      *  *  *     | *",
    (" _____)/\\|\\__//(\\/(/\\)/\\//\\/|_)___"
     "_____)/|\\\\_/_/(\\/(/\\)/\\/\\/|_)____"),
    0
};
#define STONE_LINE_CENT 19 /* char[] element of center of stone face */
#endif                     /* NH320_DEDICATION */
#define STONE_LINE_LEN  16 /* # chars that fit on one line
                            * (note 1 ' ' border)           */
#define NAME_LINE  6 /* *char[] line # for player name */
#define GOLD_LINE  7 /* *char[] line # for amount of gold */
#define DEATH_LINE 8 /* *char[] line # for death description */
#define YEAR_LINE 12 /* *char[] line # for year */


/* 优化后的汉字和拉丁字母统计逻辑（移除了对边框 '|' 的依赖） */
int howmanykanji(char *s)
{
    int i = 0, kanji = 0;
    while(s[i] != '\0')
    {
        /* 简单判断 UTF-8 多字节字符的起点 */
        if (s[i] < 0)
        {
            kanji++;
            i += 3; /* UTF-8 中文占用 3 字节 */
        }
        else 
        {
            i++;
        }
    }
    return kanji;
}

int howmanyromaji(char *s)
{
    int i = 0, romaji = 0;
    while(s[i] != '\0')
    {
        if (s[i] < 0)
        {
            i += 3; 
        }
        else 
        {
            romaji++;
            i++;
        }
    }
    return romaji;
}


/* 彻底重构的 center 函数：使用 Sprintf 重新拼装，杜绝越界并实现完美对齐 */
staticfn void
center(int line, char *text)
{
    char buf[BUFSZ];
    int visual_len, left_pad, right_pad;
    int max_width = 18; /* 墓碑内部左右 '|' 之间的总宽度 */

    /* 1. 计算文本的视觉宽度 (中文算作2宽度，西文算作1) */
    visual_len = howmanyromaji(text) + 2 * howmanykanji(text);

    /* 2. 处理超长文本，防止宽度超出破坏对齐 */
    if (visual_len > max_width) {
        visual_len = max_width; 
    }

    /* 3. 计算左右需要补充的空格数以实现居中 */
    left_pad = (max_width - visual_len) / 2;
    right_pad = max_width - visual_len - left_pad;

    /* 4. 动态组装字符串：
     * "                  |" 长度为19 (18个缩进空格 + 1个左边框)
     * %*s: C语言动态填充 left_pad 个空格
     * %s:  插入文本内容 (不受底层字节数影响，终端自适应渲染)
     * %*s: C语言动态填充 right_pad 个空格
     * |:   最后加上右边框
     */
    Sprintf(buf, "                  |%*s%s%*s|", 
            left_pad, "", text, right_pad, "");

    /* 5. 释放原有的行内存，重新分配新内存并赋值，彻底杜绝越界闪退 */
    free((genericptr_t) gr.rip[line]);
    gr.rip[line] = dupstr(buf);
}


void
genl_outrip(winid tmpwin, int how, time_t when)
{
    char **dp;
    char buf[BUFSZ];
    int x;
    int line;
    long cash;
    int year;

    gr.rip = dp = (char **) alloc(sizeof(rip_txt));
    for (x = 0; rip_txt[x]; ++x)
        dp[x] = dupstr(rip_txt[x]);
    dp[x] = (char *) 0;

    /* Put name on stone */
    Sprintf(buf, "%.*s", (int) STONE_LINE_LEN, svp.plname);
    center(NAME_LINE, buf);

    /* Put $ on stone */
    cash = max(gd.done_money, 0L);
    
    /* arbitrary upper limit; */
    if (cash > 99999999L)
        cash = 99999999L;
    Sprintf(buf, "%ld Au", cash);
    center(GOLD_LINE, buf);

    /* Put type of death on stone */
    formatkiller(buf, sizeof buf, how, FALSE);
    center(DEATH_LINE, buf);

    /* Put year on stone */
    year = (when > 0L) ? (int)(((when) / 31556926L) + 1970L) : 1970;
    Sprintf(buf, "%4d", year);
    center(YEAR_LINE, buf);

    /* Display the generated tombstone array to the window */
    putstr(tmpwin, 0, "");
    for (line = 0; dp[line]; line++) {
        putstr(tmpwin, 0, dp[line]);
    }
    putstr(tmpwin, 0, "");

    /* Free all dynamically allocated memory to prevent memory leaks */
    for (line = 0; dp[line]; line++) {
        free((genericptr_t) dp[line]);
    }
    free((genericptr_t) dp);
    gr.rip = 0;
}
#endif /* TEXT_TOMBSTONE */