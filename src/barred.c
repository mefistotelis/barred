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
    Colr_Header,
    Colr_HotKey,
};

enum UserCommands {
    UCmd_None = 0,
    UCmd_Quit,
    UCmd_Help,
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

int     P_ioresult;
#define _SETIO(cond,ior)          (P_ioresult = (cond) ? 0 : (ior))

#define FileNotFound     10
#define FileNotOpen      13
#define FileWriteError   38
#define BadInputFormat   14
#define EndOfFile        30

int a; /*Pozycja w linii*/
int b; /*Linia(na ekranie) w ktorej stoi kursor*/
int O; /*Ilosc linii powyzej ekranu*/

int C, J, K;

/** Jak bardzo trzeba zmienić zawartość ekranu(odświerzyć)
 0-  nic nie robić
 1-  odświerzyć pasek stanu(górny)
 2-  odświerzyć cały wyświetlony plik
 4-  nieużyte
 8-  aktualną linię
 16- aktualną linię i poniżej
 32- linię pierwszą, resztę przesuń w dół
 64- linię ostatnią, resztę przesuń w górę
 128-odświerzyć pasek menu(dolny)*/
unsigned char redrawFlags;

unsigned char Opcje;
/*1- Zamieniaj 0 na spację
 */
long IVal;

char Nam1[256];

//REMOVE
int TextAttr;
int GetMaxX = 80;
int GetMaxY = 40;
int wherex = 0, wherey = 0;

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

unsigned long maxpos(FILE * f)
{
    fseek(f, 0, SEEK_END);
    unsigned long ppos = ftell(f);
    unsigned long len = (unsigned long)ftell(f);
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
    case Colr_Header:
        wcolor_set(w, 3, NULL);
        wattroff(w, A_BOLD);
        break;
    case Colr_HotKey:
        wcolor_set(w, 2, NULL);
        wattroff(w, A_BOLD);
        break;
    case Colr_Default:
    default:
        wcolor_set(w, 0, NULL);
        wattroff(w, A_BOLD);
        break;
    }
}

void print_help(void)
{
    wmove(win_ct, 0, 0);
    wprintw(win_ct, "\n     BArrEd                             (c) by Tomasz Lis\n\n");
    wprintw(win_ct, "This program allows to view and edit binary files which contain\n");
    wprintw(win_ct, "array of constant-width records, like databases used in games.\n\n");
    wprintw(win_ct, "  Usage:\n");
    wprintw(win_ct, "     barred [-s <number>] [-n <number>] <file>\n");
    wprintw(win_ct, "  where:\n");
    wprintw(win_ct, " [file]   - File name to edit,\n");
    wprintw(win_ct, " -s <number> - Index of the byte within file which will be used as first.\n");
    wprintw(win_ct, " -n <number> - Amount of bytes per line.\n");
    wprintw(win_ct, "               Editing:\n");
    wprintw(win_ct, " <up arrow>/<down arrow>    -Linia w g\303\263r\304\231/w d\303\263\305\202\n");
    wprintw(win_ct, " <left arrow>/<right arrow> -Przesuwanie w tek\305\233cie\n");
    wprintw(win_ct, " <home>/<end>               -Na pocz\304\205tek/koniec pliku\n");
    wprintw(win_ct, " <pagr up>/<page down>      -Przesuwa widok o stron\304\231 wy\305\274ej/ni\305\274ej\n");
    wprintw(win_ct, "  <F1>                      -Wy\305\233wietla ten ekran pomocy\n");
    wprintw(win_ct, "  <F2>/<F3>                 -Zmiejszenie/zwi\304\231kszenie ilo\305\233ci\n");
    wprintw(win_ct, "                             znak\303\263w na jedn\304\205 lini\304\231\n");
    wprintw(win_ct, "  <F4>/<F5>                 -Kopiowanie linii/znaku\n");
    wprintw(win_ct, "  <F6>/<F7>                 -Zwi\304\231ksza/zmiejsza wielko\305\233\304\207 parametru [number]\n");
    wprintw(win_ct, "        <Return>           -Podanie nowej warto\305\233ci bajtu.\n");
}

static void EKRAN(unsigned int chrcode, unsigned short chrarrtib, int pos_x, int pos_y)
{
    wmove(win_ct, pos_y, pos_x) == -1 ? -1 : waddrawch(win_ct, chrcode);//|chrarrtib
}

void draw_footer_stdmenu(void)
{
    wmove(win_ft, 0, 0);
    wsimplcolor(win_ft, Colr_Default);
    wprintw(win_ft, "F1");
    wsimplcolor(win_ft, Colr_HotKey);
    wprintw(win_ft, "Help");
    wsimplcolor(win_ft, Colr_Default);
    wprintw(win_ft, " F2");
    wsimplcolor(win_ft, Colr_HotKey);
    wprintw(win_ft, "Shrink ");
    wsimplcolor(win_ft, Colr_Default);
    wprintw(win_ft, "F3");
    wsimplcolor(win_ft, Colr_HotKey);
    wprintw(win_ft, "Widen ");
    wsimplcolor(win_ft, Colr_Default);
    wprintw(win_ft, "F4");
    wsimplcolor(win_ft, Colr_HotKey);
    wprintw(win_ft, "CpLine");
    wsimplcolor(win_ft, Colr_Default);
    wprintw(win_ft, "F5");
    wsimplcolor(win_ft, Colr_HotKey);
    wprintw(win_ft, "CpChar");
    wsimplcolor(win_ft, Colr_Default);
    wprintw(win_ft, "F6");
    wsimplcolor(win_ft, Colr_HotKey);
    wprintw(win_ft, "-Byte");
    wsimplcolor(win_ft, Colr_Default);
    wprintw(win_ft, "F7");
    wsimplcolor(win_ft, Colr_HotKey);
    wprintw(win_ft, "+Byte");
    wsimplcolor(win_ft, Colr_Default);
    wprintw(win_ft, "F8");
    wsimplcolor(win_ft, Colr_HotKey);
    wprintw(win_ft, "Search");
    wsimplcolor(win_ft, Colr_Default);
    wprintw(win_ft, "\033\331");
    wsimplcolor(win_ft, Colr_HotKey);
    wprintw(win_ft, "ASCII");
    wsimplcolor(win_ft, Colr_Default);
    wprintw(win_ft, " F10");
    wsimplcolor(win_ft, Colr_HotKey);
    wprintw(win_ft, "Exit");
    wrefresh(win_ft);
}

void draw_header_statusbar(void)
{
    wmove(win_hd, 0, 0);
    wsimplcolor(win_hd, Colr_Header);
    wprintw(win_hd, "%80c", ' ');
    wmove(win_hd, 0, 1);
    wprintw(win_hd, "BArrEd: %s", ifname);
    wmove(win_hd, 0, 68);
    wprintw(win_hd, "%12d ch/ln", num_cols);
    wmove(win_hd, 0, 78);
    if (num_cols > 80)
        wprintw(win_hd, "->");
    else
        wprintw(win_hd, "  ");
    wrefresh(win_hd);
}

static void StanUpdate()
{
    wsimplcolor(win_hd, Colr_Header);
    wmove(win_hd, 0, 36);
    wprintw(win_hd, "Byte %6d", num_cols * (O + b - 2) + skip_bytes + a);
    wprintw(win_hd, " ASCII ");
    if (num_cols * (O + b - 2) + skip_bytes + a - 1 >= maxpos(ifile))
    {
        wprintw(win_hd, "EOF         ");
        return;
    }
    char Y, Z;
    fseek(ifile, num_cols * (O + b - 2) + skip_bytes + a - 2, 0);
    fread(&Z, 1, 1, ifile);
    wprintw(win_hd, "%3d", Z);
    if (feof(ifile))
        return;
    fread(&Y, 1, 1, ifile);
    C = Z * 16;
    wprintw(win_hd, " I2:%5d", C * 16 + Y);
}

static void printFileLine(NrLini)
    unsigned char NrLini;
{
    wsimplcolor(win_ct, Colr_WorkArea);
    if (fseek(ifile, (NrLini + O) * num_cols + skip_bytes - 1, 0) != 0)
        fseek(ifile, maxpos(ifile) - 1, 0);
    for (J = 1; J <= GetMaxX; J++)
    {
        if (J <= num_cols)
        {
            char X;
            if (feof(ifile))
                X = '=';
            else
                fread(&X, 1, 1, ifile);
            if (X == '\0' && (Opcje & 1) > 0)
                X = ' ';
            EKRAN(X, 27, (int) J, NrLini + 1);
        }
        else
            EKRAN(32, 7, (int) J, NrLini + 1);
    }
}

static void parseCmdOptions(int argc, char *argv[])
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

static bool initScreen(void)
{
    ripoffline(1,NULL);
    ripoffline(-1,NULL);
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
    win_ft = newwin(        1, GetMaxX, GetMaxY-1, 0);

    init_pair(1,  COLOR_CYAN,  COLOR_BLUE);
    init_pair(2,  COLOR_BLACK, COLOR_CYAN);
    init_pair(3,  COLOR_BLUE,  COLOR_CYAN);

    raw();
    keypad(stdscr, TRUE);

    O = 0;
    a = 1;
    b = 2;
    redrawFlags = Draw_Header|Draw_Footer|Draw_CntntAll;
    Opcje = 1;

    return TRUE;
}

static bool initInputFile(void)
{
    wmove(win_ct, 0, 0);
    wprintw(win_ct, "BArrEd initial test . . .\n");
    wrefresh(win_ct);
    ifile = fopen(ifname, "rw");
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

static void drawScreen()
{
    if ((redrawFlags > 0) & (!kbhit()))
    {
        if ((redrawFlags & Draw_CntntMvDn) > 0)
        {
            redrawFlags |= Draw_CntntAll;
            /*wmove(win_ct, 1, 24);
            DelLine();
            wmove(win_ct, 1, 2);
            InsLine();
            WyswietlLinie(0);*/
        }
        if ((redrawFlags & Draw_CntntMvUp) > 0)
        {
            redrawFlags |= Draw_CntntAll;
            /*wmove(win_ct, 1, 2);
            DelLine();
            wmove(win_ct, 1, 24);
            InsLine();
            WyswietlLinie(22);*/
        }
        if ((redrawFlags & Draw_Header) > 0)
            draw_header_statusbar();
        if ((redrawFlags & Draw_CntntAll) > 0)
        {
            for (IVal = 0; IVal < GetMaxY-2; IVal++)
                printFileLine((int) IVal);
        }
        if ((redrawFlags & Draw_CntntCur) > 0)
            printFileLine((int) (b - 2));
        if ((redrawFlags & Draw_CntntBlw) > 0)
        {
            for (IVal = b - 2; IVal < GetMaxY-2; IVal++)
                printFileLine((int) IVal);
        }
        if ((redrawFlags & Draw_Footer) > 0)
            draw_footer_stdmenu();
        redrawFlags = Draw_Nothing;
    }
    StanUpdate();
    wmove(win_ct, (int) a, (int) b);
    wsimplcolor(win_ct, Colr_WorkArea);
}

void shutdown()
{
    if (ifile != NULL)
        fclose(ifile);
    ifile = NULL;
    wsimplcolor(win_hd, Colr_Default);
    wsimplcolor(win_ct, Colr_Default);
    wsimplcolor(win_ft, Colr_Default);
    wclear(win_ft);
    wrefresh(win_ft);
    wrefresh(win_hd);
    wrefresh(win_ct);
    delwin(win_hd);
    delwin(win_ft);
    //delwin(win_ct);
    endwin();
    printf("\n   BArrEd ver 2.01\n");
    printf("      Author:    Tomasz Lis\n");
    printf("                            (c) Tomasz Lis 1997-2000\n");
}

int keyCommand(int key)
{
    switch (key)
    {
    case KEY_F(10):
    case '\033': // ESC
        return UCmd_Quit;
    case KEY_F(1):
        return UCmd_Help;
    case KEY_F(3):
        return UCmd_ColsWiden;
    case KEY_F(2):
        return UCmd_ColsShrink;
    case KEY_F(4):
        return UCmd_CopyLine;
    case KEY_F(5):
        return UCmd_CopyChar;
    case KEY_F(6):
        return UCmd_SkipStartInc;
    case KEY_F(7):
        return UCmd_SkipStartDec;
    case KEY_F(8):
        return UCmd_Search;
    case KEY_ENTER:
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
    return UCmd_None;
}

static void SRCHSTR()
{
    char NAM2[256];
    char X;

    *NAM2 = '\0';
    fputs(Nam1, stdout);
    X = toupper(getch());
    if ((X == '\0') & kbhit())
    {
        X = getch();
        return;
    }
    if (X == '\033')
    {
        X = '\001';
        return;
    }
    if (X != '\015')
    {
        do
        {
            if (X == 8)
            {
                if (*Nam1 != '\0')
                {
                    Nam1[strlen(Nam1)] = '\0';
                    wmove(win_ct, (int) (wherex - 1), (int) wherey);
                    putchar(' ');
                    wmove(win_ct, (int) (wherex - 1), (int) wherey);
                }
            }
            else
            {
                if (strlen(Nam1) < 42)
                {
                    putchar(X);
                    sprintf(Nam1 + strlen(Nam1), "%c", X);
                }
            }
            X = toupper(getch());
            if ((X == '\0') & kbhit())
            {
                X = getch();
                return;
            }
            if (X == '\033')
            {
                X = '\001';
                return;
            }
        }
        while (X != (char) 13);
    }
    _SETIO(fseek(ifile, num_cols * (O + b - 2) + skip_bytes + a - 1, 0) == 0, EndOfFile);
    if (P_ioresult != 0)
        return;
    wsimplcolor(win_ft, Colr_HotKey);
    wmove(win_ct, 1, (int) wherey);
    wprintw(win_ft, "   Wyszukiwanie . . . ");
    while (!feof(ifile))
    {
        fread(&X, 1, 1, ifile);
        sprintf(NAM2 + strlen(NAM2), "%c", toupper(X));
        while (strlen(NAM2) > strlen(Nam1))
            memmove(NAM2, NAM2 + 1, sizeof(NAM2) - 1);
        if (strcmp(Nam1, NAM2))
            continue;
        a = ftell(ifile) - skip_bytes - strlen(Nam1) + 2;
        O = a / num_cols;
        if (O < 21)
        {
            b = O + 2;
            O = 0;
        }
        else
        {
            O -= 20;
            b = 22;
        }
        a %= num_cols;
        return;
    }
}

static void executeCommand(int cmd, int ch)
{
    switch (cmd)
    {
    case UCmd_Quit:
        break;
    case UCmd_Help:
        redrawFlags = Draw_CntntAll|Draw_Header|Draw_Footer;
        wsimplcolor(win_hd, Colr_WorkArea);
        wclear(win_ct);
        wsimplcolor(win_ct, Colr_Header);
        wprintw(win_hd, "Information regarding the Binary Arrays Editor tool                                           ");
        wsimplcolor(win_ct, Colr_WorkArea);
        print_help();
        putchar('\n');
        wsimplcolor(win_ft, Colr_Default);
        wprintw(win_ft, "                          ");
        wsimplcolor(win_ft, Colr_HotKey);
        wprintw(win_ft, "--->  Press any key <---");
        wsimplcolor(win_ft, Colr_Default);
        wprintw(win_ft, "                              ");
        wrefresh(win_ct);
        wrefresh(win_hd);
        getch();
        break;
    case UCmd_ColsWiden:
        redrawFlags = Draw_CntntAll|Draw_Header;
        num_cols++;
        if (O + b - 2 < a)
            a += 2 - b - O;
        else
        {
            IVal = (num_cols - 1) * (O + b - 2) + a;
            b = IVal / num_cols - O + 2;
            a = IVal % num_cols;
            if (a > num_cols)
                a = num_cols;
            while (b < 2)
            {
                O--;
                b++;
            }
        }
        break;
    case UCmd_ColsShrink:
        if (num_cols > 1)
        {
            redrawFlags = Draw_CntntAll|Draw_Header;
            num_cols--;
        }
        break;
    case UCmd_CopyLine:
        if (((num_cols * (O + b - 2) + skip_bytes + a + num_cols - 1 < maxpos(ifile))))
        {
            char Y, Z;
            fseek(ifile, num_cols * (O + b - 2) + skip_bytes + a - 2, 0);
            fread(&Y, 1, 1, ifile);
            IVal = 0;
            do
            {
                fseek(ifile, ftell(ifile) + num_cols - 1, 0);
                fread(&Z, 1, 1, ifile);
                IVal++;
            }
            while (!((Z != Y) | (ftell(ifile) + num_cols + 1 > maxpos(ifile))));
            if (IVal >= 24 - b)
                return;
            fseek(ifile, ftell(ifile) - 1, 0);
            fwrite(&Y, 1, 1, ifile);
            redrawFlags = Draw_CntntBlw;
        }
        break;
    case UCmd_CopyChar:
        if (num_cols * (O + b - 2) + skip_bytes + a + num_cols < maxpos(ifile))
        {
            long FORLIM;
            FORLIM = num_cols;
            for (IVal = 1; IVal <= FORLIM; IVal++)
            {
                char Y;
                redrawFlags = Draw_CntntBlw;
                fseek(ifile, num_cols * (O + b - 2) + skip_bytes + IVal - 2, 0);
                fread(&Y, 1, 1, ifile);
                fseek(ifile, num_cols * (O + b - 1) + skip_bytes + IVal - 2, 0);
                fwrite(&Y, 1, 1, ifile);
            }
        }
        break;
    case UCmd_SkipStartInc:
        if (skip_bytes < maxpos(ifile))
        {
            redrawFlags = Draw_CntntAll;
            skip_bytes++;
            if (a > 1)
                a--;
            else
            {
                if (b > 2)
                {
                    b--;
                    a = num_cols;
                }
                else
                {
                    if (O > 0)
                    {
                        O--;
                        a = num_cols;
                    }
                }
            }
        }
        break;
    case UCmd_SkipStartDec:
        if (skip_bytes > 0)
        {
            redrawFlags = Draw_CntntAll;
            skip_bytes--;
            if (a < num_cols)
                a++;
            else
            {
                if (b < 24)
                {
                    b++;
                    a = 1;
                }
                else
                {
                    if (O * num_cols + skip_bytes < maxpos(ifile))
                    {
                        redrawFlags = Draw_CntntAll;
                        O++;
                        a = 1;
                    }
                }
            }
        }
        break;
    case UCmd_Search:
        redrawFlags = Draw_CntntAll|Draw_Header|Draw_Footer;
        wmove(win_ct, 1, 25);
        wsimplcolor(win_ft, Colr_HotKey);
        wprintw(win_ct, "Search for string:");
        wsimplcolor(win_ft, Colr_Default);
        putchar('[');
        for (IVal = 1; IVal <= 42; IVal++)
            putchar(' ');
        putchar(']');
        wmove(win_ct, 25, 25);
        SRCHSTR();
        break;
    case UCmd_EnterASCII:
        if ((num_cols * (O + b - 2) + skip_bytes + a <= maxpos(ifile)))
        {
            redrawFlags = Draw_CntntCur|Draw_CntntMvDn|Draw_Header|Draw_Footer;
            char Y;
            wmove(win_ct, 1, 25);
            wsimplcolor(win_ft, Colr_HotKey);
            wprintw(win_ct, "           ");
            wsimplcolor(win_ft, Colr_Default);
            fseek(ifile, num_cols * (O + b - 2) + skip_bytes + a - 2, 0);
            fread(&Y, 1, 1, ifile);
            wprintw(win_ct, "%3d", Y);
            wsimplcolor(win_ft, Colr_HotKey);
            wprintw(win_ct, "|--->                                                  ");
            wsimplcolor(win_ft, Colr_Default);
            wprintw(win_ct, " ESC");
            wmove(win_ct, 20, 25);
            wprintw(win_ct, "   ");
            wmove(win_ct, (int) a, (int) b);
            putchar(Y);
            wmove(win_ct, 20, 25);
            _SETIO(scanf("%ld", &IVal) == 1, (feof(stdin) != 0) ? EndOfFile : BadInputFormat);
            _SETIO(scanf("%*[^\n]") == 0, (feof(stdin) != 0) ? EndOfFile : BadInputFormat);
            _SETIO(getchar() != EOF, EndOfFile);
            if (P_ioresult != 0)
                return;
            Y = (char) IVal;
            fseek(ifile, ftell(ifile) - 1, 0);
            fwrite(&Y, 1, 1, ifile);
        }
        break;
    case UCmd_EnterType:
        fseek(ifile, num_cols * (O + b - 2) + skip_bytes + a - 2, 0);
        fwrite(&ch, 1, 1, ifile);
        putchar(ch);
        break;
    case UCmd_MoveUp:
        if (b > 2)
            b--;
        else
        {
            if (O > 0)
            {
                redrawFlags = Draw_CntntMvDn;
                O--;
            }
        }
        break;
    case UCmd_MoveDown:
        if (b < 24)
            b++;
        else
        {
            if ((O + 1) * num_cols + skip_bytes < maxpos(ifile))
            {
                redrawFlags = Draw_CntntMvUp;
                O++;
            }
        }
        break;
    case UCmd_MoveLeft:
        if (a > 1)
            a--;
        else
        {
            if (b > 2)
            {
                b--;
                a = num_cols;
            }
            else
            {
                if (O > 0)
                {
                    redrawFlags = Draw_CntntMvDn;
                    O--;
                    a = num_cols;
                }
            }
        }
        break;
    case UCmd_MoveRight:
        if (a < num_cols)
            a++;
        else
        {
            if (b < 24)
            {
                b++;
                a = 1;
            }
            else
            {
                if (O * num_cols + skip_bytes < maxpos(ifile))
                {
                    redrawFlags = Draw_CntntMvUp;
                    O++;
                    a = 1;
                }
            }
        }
        break;
    case UCmd_MovePgUp:
        redrawFlags = Draw_CntntAll;
        if (O > 23)
            O -= 22;
        else
            O = 0;
        break;
    case UCmd_MovePgDn:
        redrawFlags = Draw_CntntAll;
        if ((O + 23) * num_cols + skip_bytes < maxpos(ifile))
            O += 22;
        else
            O = (maxpos(ifile) - skip_bytes) / num_cols;
        break;
    case UCmd_MoveHome:
        redrawFlags = Draw_CntntAll;
        O = 0;
        break;
    case UCmd_MoveEnd:
        redrawFlags = Draw_CntntAll;
        O = (maxpos(ifile) - skip_bytes) / num_cols;
        break;
    case UCmd_None:
    default:
        break;
    }
}

int main(int argc, char *argv[])
{
    ifile = NULL;
    parseCmdOptions(argc, argv);
    initScreen();
    if (!initInputFile()) {
        shutdown();
        abort();
    }
    int ch,cmd;
    do
    {
        drawScreen();
        ch = getch();
        cmd = keyCommand(ch);
        executeCommand(cmd, ch);
    }
    while (cmd != UCmd_Quit);
    shutdown();
    exit(EXIT_SUCCESS);
}
