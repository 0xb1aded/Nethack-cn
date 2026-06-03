/* NetHack 5.0  getline.c   $NHDT-Date: 1701285885 2023/11/29 19:24:45 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.59 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2006. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef TTY_GRAPHICS

#if !defined(MACOS9)
#define NEWAUTOCOMP
#endif

#include "wintty.h"
#include "func_tab.h"

char morc = 0; /* tell the outside world what char you chose */
static boolean suppress_history;
static boolean ext_cmd_getlin_hook(char *);

typedef boolean (*getlin_hook_proc)(char *);

static void hooked_tty_getlin(const char *, char *, getlin_hook_proc);
extern int extcmd_via_menu(void); /* cmd.c */

extern char erase_char, kill_char; /* from appropriate tty.c file */

/*
 * Read a line closed with '\n' into the array char bufp[BUFSZ].
 * (The '\n' is not stored. The string is closed with a '\0'.)
 * Reading can be interrupted by an escape ('\033').  If there is already
 * some text, it is removed and prompting continues as if from the start.
 * However, if there is no text yet (or anymore) then "\033" is returned.
 */
void
tty_getlin(const char *query, char *bufp)
{
    suppress_history = FALSE;
    hooked_tty_getlin(query, bufp, (getlin_hook_proc) 0);
}

static void
hooked_tty_getlin(
    const char *query,
    char *bufp,
    getlin_hook_proc hook)
{
    char *obufp = bufp;
    int c;
    struct WinDesc *cw = wins[WIN_MESSAGE];
    boolean doprev = FALSE;
    char utf8_buffer[5];   /* 扩容至 5，确保容纳 4 字节 UTF-8 + '\0' */
    int utf8_needed = 0;   /* 还需要的UTF-8续字节数 */
    int utf8_count = 0;    /* 已收集的UTF-8字节数 */

    if (ttyDisplay->toplin == TOPLINE_NEED_MORE && !(cw->flags & WIN_STOP))
        more();
    cw->flags &= ~WIN_STOP;
    ttyDisplay->toplin = TOPLINE_SPECIAL_PROMPT;
    ttyDisplay->inread++;

    /*
     * Issue the prompt.
     *
     * custompline() will call vpline() which calls flush_screen() which
     * calls bot(). The core now disables bot() processing while inside
     * getlin, so the screen won't be modified during whatever this prompt
     * is for.
     */
    custompline(OVERRIDE_MSGTYPE | SUPPRESS_HISTORY, "%s ", query);

#ifdef EDIT_GETLIN
    /* bufp is input/output; treat current contents (presumed to be from
       previous getlin()) as default input */
    addtopl(obufp);
    bufp = eos(obufp);
#else
    /* !EDIT_GETLIN: bufp is output only; init it to empty */
    *bufp = '\0';
#endif

    for (;;) {
        (void) fflush(stdout);
        Strcat(strcat(strcpy(gt.toplines, query), " "), obufp);
        term_curs_set(1);
        c = pgetchar();
        term_curs_set(0);

        /* 处理UTF-8多字节字符序列 */
        if (utf8_needed > 0) {
            /* 已在接收多字节字符，期望收到续字节 (10xxxxxx) */
            if ((c & 0xC0) == 0x80) {
                /* 有效的UTF-8续字节 */
                utf8_buffer[utf8_count++] = c;
                utf8_needed--;
                if (utf8_needed == 0) {
                    /* 完整的UTF-8字符已收到，添加到输入缓冲 */
                    if (bufp - obufp < BUFSZ - utf8_count - 1 
                        && bufp - obufp < COLNO) {
                        
#ifdef NEWAUTOCOMP
                        char *i_comp = eos(bufp); /* 记录当前末尾用于补全失败时擦除 */
#endif
                        utf8_buffer[utf8_count] = '\0';
                        
                        /* 修复 1：使用 memcpy 拷贝，并无条件追加 \0 截断字符串 */
                        (void) memcpy(bufp, utf8_buffer, utf8_count);
                        bufp[utf8_count] = '\0';

                        /* 仅安全打印这一个多字节字符本身 */
                        putsyms(utf8_buffer); 
                        bufp += utf8_count;

                        /* 同步原版的补全状态机 */
                        if (hook && (*hook)(obufp)) {
                            putsyms(bufp);
#ifndef NEWAUTOCOMP
                            bufp = eos(bufp);
#else  /* NEWAUTOCOMP */
                            {
                                char *curr;
                                for (curr = bufp; *curr; ++curr)
                                    putsyms("\b");
                            }
#endif /* NEWAUTOCOMP */
                        }
#ifdef NEWAUTOCOMP
                        else if (i_comp > bufp) {
                            char *s_comp = i_comp;
                            /* 若未触发新的补全，擦除历史残余补全字符 */
                            for (; i_comp > bufp; --i_comp)
                                putsyms(" ");
                            for (; s_comp > bufp; --s_comp)
                                putsyms("\b");
                        }
#endif /* NEWAUTOCOMP */
                    }
                }
                continue;  /* 阻止续字节掉进下面的 process_char */
            } else {
                /* 期望的续字节没有收到，丢弃不完整的UTF-8字符 */
                utf8_needed = 0;
                utf8_count = 0;
                goto process_char;
            }
        } else if ((c & 0x80) != 0 && c != '\033' && c != EOF) {
            /* 检测UTF-8首字节 (非ASCII字符) */
            if ((c & 0xE0) == 0xC0) {
                utf8_needed = 1;
            } else if ((c & 0xF0) == 0xE0) {
                utf8_needed = 2;
            } else if ((c & 0xF8) == 0xF0) {
                utf8_needed = 3;
            }
            
            if (utf8_needed > 0) {
                utf8_buffer[0] = c;
                utf8_count = 1;
                continue;  /* 等待续字节 */
            }
        }

    process_char:
        if (c == '\033' || c == EOF) {
            if (c == EOF)
                iflags.term_gone = 1;
            if (c == '\033' && obufp[0] != '\0') {
                obufp[0] = '\0';
                bufp = obufp;
                tty_clear_nhwindow(WIN_MESSAGE);
                cw->maxcol = cw->maxrow;
                addtopl(query);
                addtopl(" ");
                addtopl(obufp);
            } else {
                obufp[0] = '\033';
                obufp[1] = '\0';
                break;
            }
        }
        if (ttyDisplay->intr) {
            ttyDisplay->intr--;
            *bufp = 0;
        }
        if (c == C('p')) { 
            int sav = ttyDisplay->inread;

            ttyDisplay->inread = 0;
            if (iflags.prevmsg_window == 's'
                || (iflags.prevmsg_window == 'c' && !doprev)) {
                if (!doprev)
                    (void) tty_doprev_message(); 
                (void) tty_doprev_message();
                ttyDisplay->inread = sav;
                doprev = TRUE;
                continue;
            } else {
                (void) tty_doprev_message();
                ttyDisplay->inread = sav;
                doprev = FALSE;
                tty_clear_nhwindow(WIN_MESSAGE);
                cw->maxcol = cw->maxrow;
                addtopl(query);
                addtopl(" ");
                *bufp = 0;
                addtopl(obufp);
            }
        } else if (doprev) {
            tty_clear_nhwindow(WIN_MESSAGE);
            cw->maxcol = cw->maxrow;
            doprev = FALSE;
            addtopl(query);
            addtopl(" ");
            *bufp = 0;
            addtopl(obufp);
        }
        if (c == erase_char || c == '\b') {
            if (bufp != obufp) {
#ifdef NEWAUTOCOMP
                char *i;
#endif /* NEWAUTOCOMP */
                char *old_bufp = bufp;
                int bytes_deleted;
                int display_width;
                int d;

                bufp--;
                /* 向后扫描找到UTF-8字符的起点 */
                while (bufp > obufp && (*bufp & 0xC0) == 0x80) {
                    bufp--;
                }
                
                bytes_deleted = (int)(old_bufp - bufp);
                display_width = (bytes_deleted > 1) ? 2 : 1;

#ifndef NEWAUTOCOMP
                for (d = 0; d < display_width; d++) {
                    putsyms("\b \b");
                }
#else                             /* NEWAUTOCOMP */
                for (d = 0; d < display_width; d++) {
                    putsyms("\b");
                }
                for (i = bufp; *i; ++i)
                    putsyms(" ");
                for (; i > bufp; --i)
                    putsyms("\b");
                *bufp = 0;
#endif                            /* NEWAUTOCOMP */
            } else
                tty_nhbell();
        } else if (c == '\n' || c == '\r') {
#ifndef NEWAUTOCOMP
            *bufp = 0;
#endif /* not NEWAUTOCOMP */
            break;
        } else if (((' ' <= (unsigned char) c && c != '\177') || ((unsigned char) c >= 0x80))
                   && (bufp - obufp < BUFSZ - 1 && bufp - obufp < COLNO)) {
#ifdef NEWAUTOCOMP
            char *i = eos(bufp);
#endif /* NEWAUTOCOMP */
            *bufp = c;
            
            /* 修复 2：无条件写入 \0，确保 hook 能拿到正确的前缀 */
            bufp[1] = 0;

            /* 仅打印当前输入的一个英文字符，防止破坏后续缓存 */
            {
                char chbuf[2];
                chbuf[0] = c;
                chbuf[1] = '\0';
                putsyms(chbuf);
            }
            bufp++;
            
            if (hook && (*hook)(obufp)) {
                putsyms(bufp);
#ifndef NEWAUTOCOMP
                bufp = eos(bufp);
#else  /* NEWAUTOCOMP */
                for (i = bufp; *i; ++i)
                    putsyms("\b");
            } else if (i > bufp) {
                char *s = i;

                for (; i > bufp; --i)
                    putsyms(" ");
                for (; s > bufp; --s)
                    putsyms("\b");
#endif /* NEWAUTOCOMP */
            }
        } else if (c == kill_char || c == '\177') { 
#ifndef NEWAUTOCOMP
            while (bufp != obufp) {
                bufp--;
                putsyms("\b \b");
            }
#else  /* NEWAUTOCOMP */
            for (; *bufp; ++bufp)
                putsyms(" ");
            for (; bufp != obufp; --bufp)
                putsyms("\b \b");
            *bufp = 0;
#endif /* NEWAUTOCOMP */
        } else
            tty_nhbell();
    }
    ttyDisplay->toplin = TOPLINE_NON_EMPTY;
    ttyDisplay->inread--;
    clear_nhwindow(WIN_MESSAGE); 

    if (suppress_history) {
        *gt.toplines = '\0';
#ifdef DUMPLOG_CORE
    } else {
        dumplogmsg(gt.toplines);
#endif
    }
}

void
xwaitforspace(const char *s) /* chars allowed besides return */
{
    int c, x = ttyDisplay ? (int) ttyDisplay->dismiss_more : '\n';

    morc = 0;
    while (
#ifdef HANGUPHANDLING
        !program_state.done_hup &&
#endif
        (c = tty_nhgetch()) != EOF) {
        if (c == '\n' || c == '\r')
            break;

        if (iflags.cbreak) {
            if (c == '\033') {
                if (ttyDisplay)
                    ttyDisplay->dismiss_more = 1;
                morc = '\033';
                break;
            }
            if ((s && strchr(s, c)) || c == x || (x == '\n' && c == '\r')) {
                morc = (char) c;
                break;
            }
            tty_nhbell();
        }
    }
}

/*
 * Implement extended command completion by using this hook into
 * tty_getlin.  Check the characters already typed, if they uniquely
 * identify an extended command, expand the string to the whole
 * command.
 *
 * Return TRUE if we've extended the string at base.  Otherwise return FALSE.
 * Assumptions:
 *
 *      + we don't change the characters that are already in base
 *      + base has enough room to hold our string
 */
static boolean
ext_cmd_getlin_hook(char *base)
{
    int *ecmatches;
    int nmatches = extcmds_match(base, ECM_NOFLAGS, &ecmatches);

    if (nmatches == 1) {
        struct ext_func_tab *ec = extcmds_getentry(ecmatches[0]);

        Strcpy(base, ec->ef_txt);
        return TRUE;
    }

    return FALSE; /* didn't match anything */
}

/*
 * Read in an extended command, doing command line completion.  We
 * stop when we have found enough characters to make a unique command.
 */
int
tty_get_ext_cmd(void)
{
    char buf[BUFSZ];
    int nmatches;
    int *ecmatches = 0;
    boolean (*no_hook)(char *base) = (boolean (*)(char *)) 0;
    char extcmd_char[2];

    if (iflags.extmenu)
        return extcmd_via_menu();

    suppress_history = TRUE;
    /* maybe a runtime option?
     * hooked_tty_getlin("#", buf,
     *                   (flags.cmd_comp && !gi.in_doagain)
     *                      ? ext_cmd_getlin_hook
     *                      : (getlin_hook_proc) 0);
     */
    extcmd_char[0] = extcmd_initiator(), extcmd_char[1] = '\0';
    buf[0] = '\0';
    hooked_tty_getlin(extcmd_char, buf,
                      !gi.in_doagain ? ext_cmd_getlin_hook : no_hook);
    (void) mungspaces(buf);

    nmatches = (buf[0] == '\0' || buf[0] == '\033') ? -1
              : extcmds_match(buf, ECM_IGNOREAC | ECM_EXACTMATCH, &ecmatches);
    if (nmatches != 1) {
        if (nmatches != -1)
            pline("%s%.60s: unknown extended command.",
                  visctrl(extcmd_char[0]), buf);
        return -1;
    }

    return ecmatches[0];
}

#endif /* TTY_GRAPHICS */

/*getline.c*/
