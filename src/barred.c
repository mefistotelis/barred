/** @file barred.c
 *     Binary Array Editor.
 * @par Purpose:
 *     Allows to view and edit binary files which contain array of constant-width records.
 * @par Comment:
 *     None.
 * @author   Tomasz Lis
 * @date     10 Jan 1997 - 20 Feb 2015
 * @par  Copying and copyrights:
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 */

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>

#include "barred_version.h"

#define KEY_USR_ESCAPE '\033'
#define KEY_USR_SPACE '\040'
#define KEY_USR_ENTER '\015'
#define KEY_USR_BACKSPC '\010'

#define SEARCH_STR_MAX_LEN 42

enum ModeStates {
    Mode_None = 0,
    Mode_Help,
    Mode_ArrEdit,
};

enum SubModeAEStates {
    SMdAE_Normal = 0,
    SMdAE_Search,
    SMdAE_EnterCode,
};

enum SubModeHlStates {
    SMdHl_Normal = 0,
};

enum RedrawFlags {
    Draw_Nothing    = 0x00,
    Draw_Header     = 0x01,
    Draw_CntntAll   = 0x02,
    Draw_CntntCur   = 0x08,
    Draw_CntntBlw   = 0x10,
    Draw_CntntMvDn  = 0x20,
    Draw_CntntMvUp  = 0x40,
    Draw_Footer     = 0x80,
};

enum SimpleColors {
    Colr_Default = 0,
    Colr_WorkArea,
    Colr_WorkEdge,
    Colr_Header,
    Colr_HotKey,
    Colr_Input,
};

enum DrawOptions {
    DOpt_Standard        = 0x00,
    DOpt_NullCharAsSpace = 0x01,
};

enum UserCommands {
    UCmd_None = 0,
    UCmd_Quit,
    UCmd_ModeHelp,
    UCmd_ModeArrEd,
    UCmd_ColsWiden,
    UCmd_ColsShrink,
    UCmd_CopyLine,
    UCmd_CopyChar,
    UCmd_SkipStartInc,
    UCmd_SkipStartDec,
    UCmd_Search,
    UCmd_EnterASCII,
    UCmd_EnterType,
    UCmd_MoveUp,
    UCmd_MoveDown,
    UCmd_MoveLeft,
    UCmd_MoveRight,
    UCmd_MovePgUp,
    UCmd_MovePgDn,
    UCmd_MoveHome,
    UCmd_MoveEnd,
};

enum HotKeyFlags {
    HKF_Default    = 0x00,
    HKF_DrawLoPrio = 0x01,
    HKF_DrawZrPrio = 0x02,
};

struct HotKey {
    int keycode;
    int cmd;
    unsigned short flags;
    char *key_text;
    char *cmd_text;
};

/** Verbosity level; higher level prints more messages. */
static int verbosity = 0;
/** Input file name. */
char *ifname = NULL;
/** Input file handle. */
FILE *ifile = NULL;

/** Window which contains file content. */
WINDOW * win_ct;
/** Footer Window which contains keys line. */
WINDOW * win_ft;
/** Header Window which contains info line. */
WINDOW * win_hd;

/** Amount of characters displayed in a line. */
int num_cols;
/** Amount of bytes skipped at the beginning of the file. */
int skip_bytes;

/** Cursor position - column within line. */
int cur_col;
/** Cursor position - line number (on screen). */
int cur_row;
/** Amount of lines above the screen. */
int top_row;
/** Amount of columns left to the screen. */
int left_col;

#define max(a,b) \
  ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b; })

#define min(a,b) \
  ({ __typeof__ (a) _a = (a); \
      __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b; })

#define file_pos_at_screen_pos(scr_row, scr_col) (((long long)top_row + scr_row) * num_cols + skip_bytes + left_col + scr_col)
#define screen_row_at_file_pos(file_pos) ((((long long)file_pos - skip_bytes) / num_cols) - top_row)
#define screen_col_at_file_pos(file_pos) ((((long long)file_pos - skip_bytes) % num_cols) - left_col)

int C, K;

/**
 * Editing mode, from Mode_* enums.
 */
unsigned char editMode;

/**
 * Editing sub-mode, from SMd*_* enums.
 * Which enumeration contain correct values, depends on editMode.
 */
unsigned char subMode;

/** How much of the screen needs redrawing.
 * Stores flags from Draw_* enums.
 */
unsigned char redrawFlags;

/**
 * Drawing options, flags from DOpt_* enums.
 */
unsigned char drawOptions;

long IVal;

char Nam1[256];

int GetMaxX = 80;
int GetMaxY = 40;

int kbhit(void)
{
    int result;
    nodelay(stdscr, TRUE);
    int ch = getch();

    if (ch != ERR) {
        ungetch(ch);
        result = 1;
    } else {
        result = 0;
    }
    nodelay(stdscr, FALSE);
    return result;
}

long long maxpos(FILE * f)
{
    fseek(f, 0, SEEK_END);
    long long ppos = ftell(f);
    long long len = ftell(f);
    fseek(f, ppos, SEEK_SET);
    return len;
}

void wsimplcolor(WINDOW *w, int c)
{
    switch (c)
    {
    case Colr_WorkArea:
        wcolor_set(w, 1, NULL);
        wattron(w, A_BOLD);
        break;
    case Colr_WorkEdge:
        wcolor_set(w, 1, NULL);
        wattroff(w, A_BOLD);
        break;
    case Colr_Header:
        wcolor_set(w, 3, NULL);
        wattroff(w, A_BOLD);
        break;
    case Colr_HotKey:
        wcolor_set(w, 2, NULL);
        wattroff(w, A_BOLD);
        break;
    case Colr_Input:
        wcolor_set(w, 4, NULL);
        wattroff(w, A_BOLD);
        break;
    case Colr_Default:
    default:
        wcolor_set(w, 0, NULL);
        wattroff(w, A_BOLD);
        break;
    }
}

void wsimplclear(WINDOW *w, int c)
{
    wclear(w);
    switch (c)
    {
    case Colr_WorkArea:
    case Colr_WorkEdge:
        wbkgd(w,COLOR_PAIR(1));
        break;
    case Colr_Header:
        wbkgd(w,COLOR_PAIR(3));
        break;
    case Colr_HotKey:
        wbkgd(w,COLOR_PAIR(2));
        break;
    case Colr_Input:
        wbkgd(w,COLOR_PAIR(4));
        break;
    case Colr_Default:
    default:
        wbkgd(w,COLOR_PAIR(0));
        break;
    }
}

void print_help(void)
{
    wsimplcolor(win_ct, Colr_WorkArea);
    wsimplclear(win_ct, Colr_WorkArea);
    wmove(win_ct, 0, 0);
    wprintw(win_ct, "\n     %s (%s)  by Tomasz Lis\n\n",FILE_DESCRIPTION,INTERNAL_NAME);
    wprintw(win_ct, "This program allows to view and edit binary files which contain\n");
    wprintw(win_ct, "array of constant-width records, like databases used in games.\n\n");
    wprintw(win_ct, "  Usage:\n");
    wprintw(win_ct, "     barred [-s <number>] [-n <number>] <file>\n");
    wprintw(win_ct, "  where:\n");
    wprintw(win_ct, " [file]   - File name to edit,\n");
    wprintw(win_ct, " -s <number> - Index of the byte within file which will be used as first.\n");
    wprintw(win_ct, " -n <number> - Amount of bytes per line.\n");
    wprintw(win_ct, "               Editing:\n");
    wprintw(win_ct, " <up arrow>/<down arrow>    -Move cursor up/down\n");
    wprintw(win_ct, " <left arrow>/<right arrow> -Move cursor left/right\n");
    wprintw(win_ct, " <home>/<end>               -Move to beginning/end of file\n");
    wprintw(win_ct, " <pagr up>/<page down>      -Move the view one page higher/lower\n");
    wprintw(win_ct, "  <F1>                      -Shows this help screen\n");
    wprintw(win_ct, "  <F2>/<F3>                 -Decrease/increase amount of chars per line\n");
    wprintw(win_ct, "  <F4>/<F5>                 -Copy line/character to below\n");
    wprintw(win_ct, "  <F6>/<F7>                 -Increase/decrease value of the [number] parameter\n");
    wprintw(win_ct, "    <Return>                -Enter integer ASCII code to replace the char.\n");
}

static void print_content_char(unsigned int chrcode, unsigned short color, int pos_x, int pos_y)
{
    wsimplcolor(win_ct, color);
    wmove(win_ct, pos_y, pos_x) == -1 ? -1 : waddrawch(win_ct, chrcode);
}

const struct HotKey hkeys_arredit[] = {
    {KEY_F(1), UCmd_ModeHelp,       HKF_Default, "F1",  "Help"},
    {KEY_F(2), UCmd_ColsShrink,     HKF_Default, "F2",  "Shrink"},
    {KEY_F(3), UCmd_ColsWiden,      HKF_Default, "F3",  "Widen"},
    {KEY_F(4), UCmd_CopyLine,       HKF_Default, "F4",  "CpLine"},
    {KEY_F(5), UCmd_CopyChar,       HKF_Default, "F5",  "CpChar"},
    {KEY_F(6), UCmd_SkipStartInc,   HKF_Default, "F6",  "-Byte"},
    {KEY_F(7), UCmd_SkipStartDec,   HKF_Default, "F7",  "+Byte"},
    {KEY_F(8), UCmd_Search,         HKF_Default, "F8",  "Search"},
    {KEY_USR_ENTER,UCmd_EnterASCII, HKF_Default, "\033\331","ASCII"},
    {KEY_ENTER,UCmd_EnterASCII,  HKF_DrawZrPrio, "\033\331","ASCII"},
    {KEY_UP,   UCmd_MoveUp,      HKF_DrawZrPrio, "","RowUp"},
    {KEY_DOWN, UCmd_MoveDown,    HKF_DrawZrPrio, "","RowDown"},
    {KEY_LEFT, UCmd_MoveLeft,    HKF_DrawZrPrio, "","ColLeft"},
    {KEY_RIGHT,UCmd_MoveRight,   HKF_DrawZrPrio, "","ColRight"},
    {KEY_PPAGE,UCmd_MovePgUp,    HKF_DrawZrPrio, "PgUp","PgUp"},
    {KEY_NPAGE,UCmd_MovePgDn,    HKF_DrawZrPrio, "PgDn","PgDn"},
    {KEY_HOME, UCmd_MoveHome,    HKF_DrawZrPrio, "Home","Start"},
    {KEY_END,  UCmd_MoveEnd,     HKF_DrawZrPrio, "End","End"},
    {KEY_F(10),UCmd_Quit,           HKF_Default, "F10", "Exit"},
    {KEY_USR_ESCAPE,UCmd_Quit,   HKF_DrawZrPrio, "Esc", "Exit"},
};

const struct HotKey hkeys_help[] = {
    {KEY_F(1), UCmd_ModeArrEd,    HKF_Default, "F1",  "Back"},
    {KEY_USR_ESCAPE,UCmd_ModeArrEd,HKF_Default,"Esc", "Back"},
    {KEY_F(10),UCmd_Quit,         HKF_Default, "F10", "Exit"},
};

void draw_footer_hotkeys(const struct HotKey * hkeys, int num_hotkeys)
{
    //wsimplclear(win_ft, Colr_Default); -- no need to clear - we will fill the whole area when drawing
    wmove(win_ft, 0, 0);
    // Compute excess space beyond the minimal size of the bar
    int i, ikey;
    int excess_space = GetMaxX;
    ikey = 0;
    for (i = 0; i < num_hotkeys; i++)
    {
        const struct HotKey *hk;
        hk = &hkeys[i];
        if ((hk->flags & HKF_DrawZrPrio) != 0)
            continue;
        excess_space -= strlen(hk->key_text);
        excess_space -= strlen(hk->cmd_text);
        ikey++;
    }
    int num_drawn_hotkeys = ikey;
    if (excess_space < 0)
        excess_space = 0;
    ikey = 0;
    for (i = 0; i < num_hotkeys; i++)
    {
        const struct HotKey *hk;
        hk = &hkeys[i];
        if ((hk->flags & HKF_DrawZrPrio) != 0)
            continue;
        wsimplcolor(win_ft, Colr_Input);
        if ((hk->key_text[0] > 0) && (hk->key_text[0] < 32)) {
            waddrawch(win_ft, hk->key_text[0]);
            wprintw(win_ft, "%s",hk->key_text+1);
        } else {
            wprintw(win_ft, "%s",hk->key_text);
        }
        wsimplcolor(win_ft, Colr_HotKey);
        wprintw(win_ft, "%s%*c\n", hk->cmd_text, excess_space*(ikey+1)/num_drawn_hotkeys - excess_space*(ikey)/num_drawn_hotkeys, ' ');
        ikey++;
    }
}

void draw_header_statusbar(void)
{
    wsimplcolor(win_hd, Colr_Header);
    wsimplclear(win_hd, Colr_Header);
    wmove(win_hd, 0, 1);
    wprintw(win_hd, "%s: %s", INTERNAL_NAME, ifname);
    wmove(win_hd, 0, GetMaxX-14);
    wprintw(win_hd, "%6d ch/ln", num_cols);
    wmove(win_hd, 0, GetMaxX-2);
    if (num_cols > GetMaxX)
        wprintw(win_hd, "->");
    else
        wprintw(win_hd, "  ");
}

void draw_header_help(void)
{
    wsimplcolor(win_hd, Colr_Header);
    wsimplclear(win_hd, Colr_Header);
    wmove(win_hd, 0, 1);
    wprintw(win_hd, "Information regarding the %s tool",FILE_DESCRIPTION);
}

void draw_header_status_update(void)
{
    wsimplcolor(win_hd, Colr_Header);
    wmove(win_hd, 0, 36);
    wprintw(win_hd, "Byte %6u", file_pos_at_screen_pos(cur_row, cur_col));
    wprintw(win_hd, " ASCII ");
    if (fseek(ifile, file_pos_at_screen_pos(cur_row, cur_col), SEEK_SET) != 0) {
        wprintw(win_hd, "EOF         ");
        return;
    }
    unsigned char nxchr, curchr;
    fread(&curchr, 1, 1, ifile);
    wprintw(win_hd, "%3u", (unsigned int)curchr);
    if (feof(ifile))
        return;
    fread(&nxchr, 1, 1, ifile);
    unsigned int val_ui;
    val_ui = ((unsigned int)curchr << 8) + nxchr;
    wprintw(win_hd, " UI16be:%5u", val_ui);
    val_ui = ((unsigned int)nxchr << 8) + curchr;
    wprintw(win_hd, " UI16le:%5u", val_ui);
}

/** Move cursor so that it is placed on given byte.
 *
 * @param file_pos
 */
void place_cursor_at_byte(long long file_pos)
{
    cur_row = screen_row_at_file_pos(file_pos);
    if (cur_row < 0) {
        top_row += cur_row;
        cur_row = 0;
    } else if (cur_row > GetMaxY-1) {
        top_row += cur_row - (GetMaxY-1);
        cur_row = GetMaxY-1;
    }
    cur_col = screen_col_at_file_pos(file_pos);
    if (cur_col < 0) {
        left_col += cur_col;
        cur_col = 0;
    } else if (cur_col > GetMaxX-1) {
        left_col += cur_col - (GetMaxX-1);
        cur_col = GetMaxX-1;
    }
}

void print_arred_line(unsigned short row_idx)
{
    //wsimplcolor(win_ct, Colr_WorkArea);
    if (fseek(ifile, file_pos_at_screen_pos(row_idx, 0), SEEK_SET) != 0)
        fseek(ifile, 0, SEEK_END);
    int col_idx;
    for (col_idx = 0; col_idx < GetMaxX; col_idx++)
    {
        unsigned char outchr;
        int colr;
        if (col_idx < num_cols)
        {
            if (fread(&outchr, 1, 1, ifile) == 1) {
                colr = Colr_WorkArea;
            } else {
                outchr = '=';
                colr = Colr_WorkEdge;
            }
            if (outchr == '\0' && (drawOptions & DOpt_NullCharAsSpace) > 0)
                outchr = ' ';
        } else {
            outchr = ' ';
            colr = Colr_Input;
        }
        print_content_char(outchr, colr, col_idx, row_idx);
    }
}

void parse_cmd_options(int argc, char *argv[])
{
    int c;
    int i;

    num_cols = 60;
    skip_bytes = 0;

    while (1)
    {
        static struct option long_options[] =
        {
            /* These options set a flag. */
            {"v",   no_argument,       &verbosity, 1},
            {"vv",  no_argument,       &verbosity, 2},
            {"vvv", no_argument,       &verbosity, 3},
            /* These options don’t set a flag.
               We distinguish them by their indices. */
            {"help",    no_argument,       0, 'h'},
            {"skip",    required_argument, 0, 's'},
            {"columns", required_argument, 0, 'n'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, argv, "h",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
          break;

        switch (c)
        {
        case 0:
            /* If this option set a flag, do nothing else now. */
            if (long_options[option_index].flag != 0)
              break;
            if (verbosity >= 2)
            {
                printf("option '%s'", long_options[option_index].name);
                if (optarg)
                  printf(" with arg '%s'", optarg);
                printf("\n");
            }
            break;

        case 'n':
            if (verbosity >= 2)
                printf("option '%s' with value '%s'\n", long_options[option_index].name, optarg);
            i = sscanf(optarg, "%d", &num_cols);
            if (i < 1) {
                fprintf(stderr, "Cannot interpret amount of bytes per column from command line.");
                abort();
            }
            break;
        case 's':
            if (verbosity >= 2)
                printf("option '%s' with value '%s'\n", long_options[option_index].name, optarg);
            i = sscanf(optarg, "%d", &skip_bytes);
            if (i < 1) {
                fprintf(stderr, "Cannot interpret amount of bytes to skip from command line.");
                abort();
            }
            break;

        case 'h':
            if (verbosity >= 2)
                printf("option '%s'\n", long_options[option_index].name);
            exit(EXIT_SUCCESS);
        case '?':
            /* getopt_long already printed an error message. */
            abort();

          default:
            abort();
        }
    }

    if (verbosity)
        printf("verbosity set to %d\n",(int)verbosity);

    if (optind < argc)
    {
        ifname = strdup(argv[optind]);
        optind++;
    } else
    {
        ifname = malloc(1024);
        printf("Enter file name:");
        gets(ifname);
    }

    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        if (verbosity)
        {
            printf("excessive non-option arguments: ");
            while (optind < argc)
                printf("%s ", argv[optind++]);
            putchar('\n');
        }
        fprintf(stderr, "Command line options contain excessive arguments.");
        abort();
    }

    if ((ifname == NULL) || (strlen(ifname) < 1))
    {
        fprintf(stderr, "No File name to edit.");
        abort();
    }
}

int dummyripinit(WINDOW *win, int columns)
{ return 0; }

bool init_screen(void)
{
    ripoffline(1,dummyripinit);
    ripoffline(-1,dummyripinit);
    if ((win_ct = initscr()) == NULL) {
        fprintf(stderr, "Error initialising ncurses.\n");
        return FALSE;
    }

    noecho();
    nonl();
    start_color();
    getmaxyx(stdscr, GetMaxY, GetMaxX);

    win_hd = newwin(        1, GetMaxX,         0, 0);
    //win_ct = newwin(GetMaxY-2, GetMaxX,         1, 0);
    win_ft = newwin(        1, GetMaxX, GetMaxY+1, 0);

    init_pair(1,  COLOR_CYAN,  COLOR_BLUE);
    init_pair(2,  COLOR_BLACK, COLOR_CYAN);
    init_pair(3,  COLOR_BLUE,  COLOR_CYAN);
    init_pair(4,  COLOR_WHITE, COLOR_BLACK);

    raw();
    keypad(stdscr, TRUE);

    top_row = 0;
    left_col = 0;
    cur_col = 0;
    cur_row = 0;
    editMode = Mode_ArrEdit;
    subMode = SMdAE_Normal;
    redrawFlags = Draw_Header|Draw_Footer|Draw_CntntAll;
    drawOptions = DOpt_NullCharAsSpace;

    return TRUE;
}

static bool init_input_file(void)
{
    wmove(win_ct, 0, 0);
    wprintw(win_ct, "%s initial test . . .\n",INTERNAL_NAME);
    wrefresh(win_ct);
    ifile = fopen(ifname, "r+b");
    if (ifile == NULL)
    {
        int i;
        i = errno;
        wprintw(win_ct, "Error opening file: %s\n", strerror(i));
        wrefresh(win_ct);
        return FALSE;
    }
    wprintw(win_ct, "Input file opened successfully.");
    wrefresh(win_ct);
    return TRUE;
}

void draw_screen(void)
{
    if (redrawFlags != Draw_Nothing)
    {
        if (kbhit()) {
            redrawFlags |= Draw_CntntAll;
            return;
        }
        // If redrawing all, make sure other redraw flags are not set
        if ((redrawFlags & Draw_CntntAll) != 0) {
            redrawFlags &= ~(Draw_CntntCur|Draw_CntntBlw|Draw_CntntMvDn|Draw_CntntMvUp);
        }
        switch (editMode)
        {
        case Mode_ArrEdit:
            if ((redrawFlags & Draw_CntntMvDn) != 0)
            {
                // Insert line at top and draw only that new line
                wmove(win_ct, 0, 0);
                winsdelln(win_ct, 1);
                wmove(win_ct, 0, 0);
                print_arred_line(0);
            }
            if ((redrawFlags & Draw_CntntMvUp) != 0)
            {
                // Delete top line and redraw bottom line only
                wmove(win_ct, 0, 0);
                winsdelln(win_ct, -1);
                wmove(win_ct, GetMaxY-1, 0);
                print_arred_line(GetMaxY-1);
            }
            if ((redrawFlags & Draw_CntntCur) != 0) {
                print_arred_line(cur_row);
            }
            if ((redrawFlags & Draw_CntntBlw) != 0) {
                int irow;
                for (irow = cur_row; irow < GetMaxY; irow++)
                    print_arred_line(irow);
            }
            if ((redrawFlags & Draw_Header) != 0) {
                draw_header_statusbar();
            }
            if ((redrawFlags & Draw_CntntAll) != 0) {
                int irow;
                for (irow = 0; irow < GetMaxY; irow++)
                    print_arred_line(irow);
            }
            if ((redrawFlags & Draw_Footer) != 0) {
                draw_footer_hotkeys(hkeys_arredit, sizeof(hkeys_arredit)/sizeof(struct HotKey));
                wrefresh(win_ft);
            }
            break;
        case Mode_Help:
            if ((redrawFlags & Draw_CntntMvDn) != 0) {
                redrawFlags |= Draw_CntntAll;
            }
            if ((redrawFlags & Draw_CntntMvUp) != 0) {
                redrawFlags |= Draw_CntntAll;
            }
            if ((redrawFlags & Draw_CntntCur) != 0) {
                redrawFlags |= Draw_CntntAll;
            }
            if ((redrawFlags & Draw_CntntBlw) != 0) {
                redrawFlags |= Draw_CntntAll;
            }
            if ((redrawFlags & Draw_Header) != 0) {
                draw_header_help();
            }
            if ((redrawFlags & Draw_CntntAll) != 0) {
                print_help();
            }
            if ((redrawFlags & Draw_Footer) != 0) {
                draw_footer_hotkeys(hkeys_help, sizeof(hkeys_help)/sizeof(struct HotKey));
                wrefresh(win_ft);
            }
            break;
        default:
            break;
        }
        redrawFlags = Draw_Nothing;
    }
    // Update status bar even if no redraw was requested
    switch (editMode)
    {
    case Mode_ArrEdit:
        draw_header_status_update();
        break;
    default:
        break;
    }
    wrefresh(win_hd);
    wmove(win_ct, cur_row, cur_col);
    wsimplcolor(win_ct, Colr_WorkArea);
}

void shutdown(void)
{
    if (ifile != NULL)
        fclose(ifile);
    ifile = NULL;
    wsimplcolor(win_hd, Colr_Default);
    wsimplcolor(win_ct, Colr_Default);
    wsimplcolor(win_ft, Colr_Default);
    wsimplclear(win_ft, Colr_Default);
    wrefresh(win_ft);
    wrefresh(win_hd);
    wrefresh(win_ct);
    delwin(win_hd);
    delwin(win_ft);
    //delwin(win_ct);
    endwin();
    printf("\n   %s ver %s\n",INTERNAL_NAME,VER_STRING);
    printf("                by Tomasz Lis 1997-2015\n");
}

int key_command(int key)
{
    // Recognize hotkeys from array data.
    const struct HotKey * hkeys;
    int num_hotkeys;
    switch (editMode)
    {
    case Mode_ArrEdit:
        hkeys = hkeys_arredit;
        num_hotkeys = sizeof(hkeys_arredit)/sizeof(struct HotKey);
        break;
    case Mode_Help:
        hkeys = hkeys_help;
        num_hotkeys = sizeof(hkeys_help)/sizeof(struct HotKey);
        break;
    default:
        hkeys = hkeys_help;
        num_hotkeys = 0;
        break;
    }
    int ikey;
    for (ikey = 0; ikey < num_hotkeys; ikey++)
    {
        const struct HotKey *hk;
        hk = &hkeys[ikey];
        if (hk->keycode == key)
            return hk->cmd;
    }
    // Recognize additional per-mode keys
    switch (editMode)
    {
    case Mode_ArrEdit:
        switch (key)
        {
        case KEY_F(10):
        case KEY_USR_ESCAPE:
            return UCmd_Quit;
        case KEY_ENTER:
        case KEY_USR_ENTER:
            return UCmd_EnterASCII;
        case KEY_UP:
            return UCmd_MoveUp;
        case KEY_DOWN:
            return UCmd_MoveDown;
        case KEY_LEFT:
            return UCmd_MoveLeft;
        case KEY_RIGHT:
            return UCmd_MoveRight;
        case KEY_PPAGE:
            return UCmd_MovePgUp;
        case KEY_NPAGE:
            return UCmd_MovePgDn;
        case KEY_HOME:
            return UCmd_MoveHome;
        case KEY_END:
            return UCmd_MoveEnd;
        default:
            break;
        }
        // Normal 8-bit keys may be interpreted as typing within the file
        if ((key >= 1) && (key <= 255))
            return UCmd_EnterType;
        return UCmd_None;
    case Mode_Help:
        switch (key)
        {
        case KEY_F(10):
            return UCmd_Quit;
        case KEY_F(1):
        case KEY_USR_ESCAPE:
        case KEY_USR_SPACE:
            return UCmd_ModeArrEd;
        default:
            break;
        }
        return UCmd_None;
    }
    return UCmd_None;
}

void cmd_search_string(void)
{
    char X;
    int startx, starty;

    getyx(win_ft, starty, startx);
    startx += 1;

    wprintw(win_ft,"[%*c]",SEARCH_STR_MAX_LEN,' ');
    wmove(win_ft, starty, startx);

    wprintw(win_ft, "%s", Nam1);
    while (1)
    {
        wrefresh(win_ft);
        X = getch();
        if (X == KEY_USR_ESCAPE) {
            return;
        }
        if ((X == KEY_ENTER) || (X == KEY_USR_ENTER)) {
            break;
        }
        if ((X == KEY_BACKSPACE) || (X == KEY_USR_BACKSPC))
        {
            if (Nam1[0] != '\0')
            {
                Nam1[strlen(Nam1)-1] = '\0';
                int wherex, wherey;
                getyx(win_ft, wherey, wherex);
                wmove(win_ft, wherey, wherex-1);
                waddch(win_ft,' ');
                wmove(win_ft, wherey, wherex-1);
            }
        } else
        if (strlen(Nam1) < SEARCH_STR_MAX_LEN)
        {
            waddch(win_ft,X);
            sprintf(Nam1 + strlen(Nam1), "%c", X);
        }
    }

    char NAM2[256];
    NAM2[0] = '\0';
    if (fseek(ifile, file_pos_at_screen_pos(cur_row, cur_col), SEEK_SET) != 0)
        return;
    wsimplcolor(win_ft, Colr_HotKey);
    wmove(win_ct, starty, 0);
    wprintw(win_ft, "   Searching . . . ");
    while (!feof(ifile))
    {
        fread(&X, 1, 1, ifile);
        sprintf(NAM2 + strlen(NAM2), "%c", X);
        while (strlen(NAM2) > strlen(Nam1))
            memmove(NAM2, NAM2 + 1, sizeof(NAM2) - 1);
        if (strcmp(Nam1, NAM2) != 0)
            continue;
        long long file_pos = ftell(ifile) - skip_bytes - strlen(Nam1);
        place_cursor_at_byte(max(file_pos,skip_bytes));
        return;
    }
}

void execute_command(int cmd, int ch)
{
    switch (cmd)
    {
    case UCmd_Quit:
        break;
    case UCmd_ModeHelp:
        editMode = Mode_Help;
        subMode = SMdHl_Normal;
        redrawFlags |= Draw_CntntAll|Draw_Header|Draw_Footer;
        break;
    case UCmd_ModeArrEd:
        editMode = Mode_ArrEdit;
        subMode = SMdAE_Normal;
        redrawFlags |= Draw_CntntAll|Draw_Header|Draw_Footer;
        break;
    case UCmd_ColsWiden:
        {
            redrawFlags |= Draw_CntntAll|Draw_Header;
            long long file_pos;
            file_pos = file_pos_at_screen_pos(cur_row, cur_col);
            num_cols++;
            place_cursor_at_byte(max(file_pos,skip_bytes));
        }
        break;
    case UCmd_ColsShrink:
        if (num_cols > 1)
        {
            redrawFlags |= Draw_CntntAll|Draw_Header;
            long long file_pos;
            file_pos = file_pos_at_screen_pos(cur_row, cur_col);
            num_cols--;
            place_cursor_at_byte(max(file_pos,skip_bytes));
        }
        break;
    case UCmd_CopyChar:
        if (file_pos_at_screen_pos(cur_row+1, cur_col) < maxpos(ifile))
        {
            char dst_chr, src_chr;
            fseek(ifile, file_pos_at_screen_pos(cur_row, cur_col), SEEK_SET);
            // Find line at which the byte is not copied yet
            fread(&dst_chr, 1, 1, ifile);
            IVal = 0;
            while (fseek(ifile, num_cols - 1, SEEK_CUR) == 0)
            {
                fread(&src_chr, 1, 1, ifile);
                IVal++;
                if (src_chr != dst_chr) break;
            }
            // Only allow to copy to lines visible on screen
            if ((IVal < 1) || (IVal >= GetMaxY - cur_row))
                break;
            // Copy one char
            if (fseek(ifile, -1, SEEK_CUR) == 0) {
                fwrite(&dst_chr, 1, 1, ifile);
            }
            redrawFlags |= Draw_CntntBlw;
        }
        break;
    case UCmd_CopyLine:
        if (file_pos_at_screen_pos(cur_row, cur_col + num_cols) < maxpos(ifile))
        {
            for (IVal = 0; IVal < num_cols; IVal++)
            {
                char buf;
                redrawFlags |= Draw_CntntBlw;
                fseek(ifile, file_pos_at_screen_pos(cur_row, IVal), SEEK_SET);
                fread(&buf, 1, 1, ifile);
                if (fseek(ifile, file_pos_at_screen_pos(cur_row+1, IVal), SEEK_SET) == 0) {
                    fwrite(&buf, 1, 1, ifile);
                }
            }
        }
        break;
    case UCmd_SkipStartInc:
        if (skip_bytes < maxpos(ifile)-1)
        {
            redrawFlags |= Draw_CntntAll;
            long long file_pos;
            file_pos = file_pos_at_screen_pos(cur_row, cur_col);
            skip_bytes++;
            place_cursor_at_byte(max(file_pos,skip_bytes));
        }
        break;
    case UCmd_SkipStartDec:
        if (skip_bytes > 0)
        {
            redrawFlags |= Draw_CntntAll;
            long long file_pos;
            file_pos = file_pos_at_screen_pos(cur_row, cur_col);
            skip_bytes--;
            place_cursor_at_byte(max(file_pos,skip_bytes));
        }
        break;
    case UCmd_Search:
        redrawFlags |= Draw_CntntAll|Draw_Header|Draw_Footer;
        subMode = SMdAE_Search;
        wsimplclear(win_ft, Colr_HotKey);
        wmove(win_ft, 0, 25);
        wsimplcolor(win_ft, Colr_HotKey);
        wprintw(win_ft, "Search for string:");
        wsimplcolor(win_ft, Colr_Input);
        cmd_search_string();
        subMode = SMdAE_Normal;
        break;
    case UCmd_EnterASCII: {
        unsigned char echr;
        if (fseek(ifile, file_pos_at_screen_pos(cur_row,cur_col), SEEK_SET) != 0)
            break;
        if (fread(&echr, 1, 1, ifile) != 1)
            break;
        redrawFlags |= Draw_CntntCur|Draw_Header|Draw_Footer;
        subMode = SMdAE_EnterCode;
        wsimplclear(win_ft, Colr_HotKey);
        wmove(win_ft, 0, 11);
        wsimplcolor(win_ft, Colr_Input);
        wprintw(win_ft, "%3d", (int)echr);
        wsimplcolor(win_ft, Colr_HotKey);
        wprintw(win_ft, "|--->");
        wsimplcolor(win_ft, Colr_Input);
        wprintw(win_ft, " ESC");
        wmove(win_ft, 0, 20);
        wprintw(win_ft, "   ");
        wmove(win_ft, cur_col, cur_row);
        wmove(win_ft, 0, 20);
        echo();
        wrefresh(win_ft);
        if (wscanw(win_ft, "%ld%*[^\n]", &IVal) != 1) {
            noecho();
            break;
        }
        noecho();
        echr = IVal;
        subMode = SMdAE_Normal;
        if (fseek(ifile, ftell(ifile) - 1, SEEK_SET) == 0)
            fwrite(&echr, 1, 1, ifile);
        };break;
    case UCmd_EnterType:
        if (fseek(ifile, file_pos_at_screen_pos(cur_row,cur_col), SEEK_SET) == 0)
            fwrite(&ch, 1, 1, ifile);
        redrawFlags |= Draw_CntntCur;
        break;
    case UCmd_MoveUp:
        if (cur_row > 0) {
            cur_row--;
        } else
        if (top_row > 0) {
            redrawFlags |= Draw_CntntMvDn;
            top_row--;
        }
        break;
    case UCmd_MoveDown:
        if (cur_row < GetMaxY-1) {
            cur_row++;
        } else
        if (file_pos_at_screen_pos(1,0) < maxpos(ifile)) {
            redrawFlags |= Draw_CntntMvUp;
            top_row++;
        }
        break;
    case UCmd_MoveLeft:
        if (cur_col > 0) {
            cur_col--;
        } else
        if (cur_row > 0) {
            cur_row--;
            cur_col = num_cols-1;
        } else
        if (top_row > 0) {
            redrawFlags |= Draw_CntntMvDn;
            top_row--;
            cur_col = num_cols-1;
        }
        break;
    case UCmd_MoveRight:
        if (cur_col < num_cols) {
            cur_col++;
        } else
        if (cur_row < GetMaxY-1) {
            cur_row++;
            cur_col = 0;
        } else
        if (file_pos_at_screen_pos(0,0) < maxpos(ifile)) {
            redrawFlags |= Draw_CntntMvUp;
            top_row++;
            cur_col = 0;
        }
        break;
    case UCmd_MovePgUp:
        redrawFlags |= Draw_CntntAll;
        if (top_row > GetMaxY-1) {
            top_row -= GetMaxY-1;
        } else {
            top_row = 0;
        }
        break;
    case UCmd_MovePgDn:
        redrawFlags |= Draw_CntntAll;
        if ((top_row + GetMaxY-1) * num_cols + skip_bytes < maxpos(ifile)) {
            top_row += GetMaxY-1;
        } else {
            top_row = (maxpos(ifile) - skip_bytes) / num_cols;
        }
        break;
    case UCmd_MoveHome:
        redrawFlags |= Draw_CntntAll;
        top_row = 0;
        break;
    case UCmd_MoveEnd:
        redrawFlags |= Draw_CntntAll;
        top_row = (maxpos(ifile) - skip_bytes) / num_cols;
        break;
    case UCmd_None:
    default:
        break;
    }
}

int main(int argc, char *argv[])
{
    ifile = NULL;
    parse_cmd_options(argc, argv);
    init_screen();
    if (!init_input_file()) {
        shutdown();
        abort();
    }
    int ch,cmd;
    do
    {
        draw_screen();
        ch = getch();
        cmd = key_command(ch);
        execute_command(cmd, ch);
    }
    while (cmd != UCmd_Quit);
    shutdown();
    exit(EXIT_SUCCESS);
}
