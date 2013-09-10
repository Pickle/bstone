//
//	ID Engine
//	ID_US_1.c - User Manager - General routines
//	v1.1d1
//	By Jason Blochowiak
//	Hacked up for Catacomb 3D
//

//
//	This module handles dealing with user input & feedback
//
//	Depends on: Input Mgr, View Mgr, some variables from the Sound, Caching,
//		and Refresh Mgrs, Memory Mgr for background save/restore
//
//	Globals:
//		ingame - Flag set by game indicating if a game is in progress
//      abortgame - Flag set if the current game should be aborted (if a load
//			game fails)
//		loadedgame - Flag set if a game was loaded
//		abortprogram - Normally nil, this points to a terminal error message
//			if the program needs to abort
//		restartgame - Normally set to gd_Continue, this is set to one of the
//			difficulty levels if a new game should be started
//		PrintX, PrintY - Where the User Mgr will print (global coords)
//		WindowX,WindowY,WindowW,WindowH - The dimensions of the current
//			window
//

#include "id_heads.h"

#pragma	hdrstop



// BBi
SDL_TimerID sys_timer_id = 0;
// BBi

#define VW_UpdateScreen() 	VH_UpdateScreen()
void VH_UpdateScreen();
void in_handle_events();


//	Global variables
		char		*abortprogram;
//		boolean		NoWait;
		Uint16		PrintX,PrintY;
		Uint16		WindowX,WindowY,WindowW,WindowH;

		US_CursorStruct US_CustomCursor;				// JAM
		boolean use_custom_cursor=false;				// JAM

//	Internal variables
#define	ConfigVersion	1

char		* US_ParmStrings[] = {"TEDLEVEL","NOWAIT"},
					* US_ParmStrings2[] = {"COMP","NOCOMP"};
boolean		US_Started;

		boolean		Button0,Button1,
					CursorBad;
		Sint16			CursorX,CursorY;

		void		(*USL_MeasureString)(const char*, Uint16*, Uint16*) = VW_MeasurePropString,
					(*USL_DrawString)(const char*) = VWB_DrawPropString;

		SaveGame	Games[MaxSaveGames];

		HighScore	Scores[MaxScores] =
					{
						{"JAM PRODUCTIONS INC.",	10000,1,0},
						{"",								10000,1,0},
						{"JERRY JONES",				10000,1,0},
						{"MICHAEL MAYNARD",	  		10000,1,0},
						{"JAMES T. ROW",				10000,1,0},
						{"",								10000,1,0},
						{"",								10000,1,0},
						{"TO REGISTER CALL",			10000,1,0},
						{" 1-800-GAME123",			10000,1,0},
						{"",								10000,1,0},
					};

//	Internal routines

//	Public routines

///////////////////////////////////////////////////////////////////////////
//
//	USL_HardError() - Handles the Abort/Retry/Fail sort of errors passed
//			from DOS.
//
///////////////////////////////////////////////////////////////////////////
Sint16
USL_HardError(Uint16 errval,Sint16 ax,Sint16 bp,Sint16 si)
{
// FIXME
#if 0
#define IGNORE  0
#define RETRY   1
#define	ABORT   2
extern	void	ShutdownId(void);

static	char		buf[32];
static	WindowRec	wr;
		int			di;
		char		c,*s,*t;

	di = _DI;

	if (ax < 0)
		s = "DEVICE ERROR";
	else
	{
		if ((di & 0x00ff) == 0)
			s = "DRIVE ~ IS WRITE PROTECTED";
		else
			s = "ERROR ON DRIVE ~";
		for (t = buf;*s;s++,t++)	// Can't use sprintf()
			if ((*t = *s) == '~')
				*t = (ax & 0x00ff) + 'A';
		*t = '\0';
		s = buf;
	}

	c = peekb(0x40,0x49);	// Get the current screen mode
	if ((c < 4) || (c == 7))
		goto oh_kill_me;

	// DEBUG - handle screen cleanup

   fontnumber = 4;
	US_SaveWindow(&wr);
	US_CenterWindow(30,3);
	US_CPrint(s);
	US_CPrint("(R)ETRY or (A)BORT?");
	VW_UpdateScreen();
	IN_ClearKeysDown();

asm	sti	// Let the keyboard interrupts come through

	while (true)
	{
		switch (IN_WaitForASCII())
		{
		case key_Escape:
		case 'a':
		case 'A':
			goto oh_kill_me;
			break;
		case key_Return:
		case key_Space:
		case 'r':
		case 'R':
			US_ClearWindow();
			VW_UpdateScreen();
			US_RestoreWindow(&wr);
			return(RETRY);
			break;
		}
	}

oh_kill_me:
	abortprogram = s;
	ShutdownId();
	fprintf(stderr,"TERMINAL ERROR: %s\n",s);
	return(ABORT);

#undef	IGNORE
#undef	RETRY
#undef	ABORT
#endif // 0

    return 2;
}

///////////////////////////////////////////////////////////////////////////
//
//	US_Shutdown() - Shuts down the User Mgr
//
///////////////////////////////////////////////////////////////////////////
void
US_Shutdown(void)
{
	if (!US_Started)
		return;

    // BBi
    SDL_RemoveTimer(sys_timer_id);
    SDL_QuitSubSystem(SDL_INIT_TIMER);
    // BBi

	US_Started = false;
}

///////////////////////////////////////////////////////////////////////////
//
//	US_CheckParm() - checks to see if a string matches one of a set of
//		strings. The check is case insensitive. The routine returns the
//		index of the string that matched, or -1 if no matches were found
//
///////////////////////////////////////////////////////////////////////////
Sint16 US_CheckParm(char *parm,char * * strings)
{
	char	cp,cs,
			*p,*s;
	Sint16		i;

	while (!isalpha(*parm))	// Skip non-alphas
		parm++;

	for (i = 0;*strings && **strings;i++)
	{
		for (s = *strings++,p = parm,cs = cp = 0;cs == cp;)
		{
			cs = *s++;
			if (!cs)
				return(i);
			cp = *p++;

			if (isupper(cs))
				cs = tolower(cs);
			if (isupper(cp))
				cp = tolower(cp);
		}
	}
	return(-1);
}

//	Window/Printing routines

#if 0	

///////////////////////////////////////////////////////////////////////////
//
//	US_SetPrintRoutines() - Sets the routines used to measure and print
//		from within the User Mgr. Primarily provided to allow switching
//		between masked and non-masked fonts
//
///////////////////////////////////////////////////////////////////////////
void
US_SetPrintRoutines(void (*measure)(char *,Uint16 *,Uint16 *),void (*print)(char *))
{
	USL_MeasureString = measure;
	USL_DrawString = print;
}

#endif 

///////////////////////////////////////////////////////////////////////////
//
//	US_Print() - Prints a string in the current window. Newlines are
//		supported.
//
///////////////////////////////////////////////////////////////////////////
void
US_Print(char *s)
{
    std::vector<char> buffer(
        s, s + std::string::traits_type::length(s) + 1);
    s = &buffer[0];

	char	c,*se;
	Uint16	w,h;

	while (*s)
	{
		se = s;
		while ((c = *se) && (c != '\n'))
			se++;
		*se = '\0';

		USL_MeasureString(s,&w,&h);
		px = PrintX;
		py = PrintY;
		USL_DrawString(s);

		s = se;
		if (c)
		{
			*se = c;
			s++;

			PrintX = WindowX;
			PrintY += h;
		}
		else
			PrintX += w;
	}
}

///////////////////////////////////////////////////////////////////////////
//
//	US_PrintUnsigned() - Prints an unsigned long
//
///////////////////////////////////////////////////////////////////////////
void
US_PrintUnsigned(Uint32 n)
{
	char	buffer[32];

	US_Print(ultoa(n,buffer,10));
}


#if 0
///////////////////////////////////////////////////////////////////////////
//
//	US_PrintSigned() - Prints a signed long
//
///////////////////////////////////////////////////////////////////////////
void
US_PrintSigned(Sint32 n)
{
	char	buffer[32];

	US_Print(ltoa(n,buffer,10));
}
#endif


///////////////////////////////////////////////////////////////////////////
//
//	USL_PrintInCenter() - Prints a string in the center of the given rect
//
///////////////////////////////////////////////////////////////////////////
void
USL_PrintInCenter(char *s,Rect r)
{
	Uint16	w,h,
			rw,rh;

	USL_MeasureString(s,&w,&h);
	rw = r.lr.x - r.ul.x;
	rh = r.lr.y - r.ul.y;

	px = r.ul.x + ((rw - w) / 2);
	py = r.ul.y + ((rh - h) / 2);
	USL_DrawString(s);
}

///////////////////////////////////////////////////////////////////////////
//
//	US_PrintCentered() - Prints a string centered in the current window.
//
///////////////////////////////////////////////////////////////////////////
void
US_PrintCentered(char *s)
{
	Rect	r;

	r.ul.x = WindowX;
	r.ul.y = WindowY;
	r.lr.x = r.ul.x + WindowW;
	r.lr.y = r.ul.y + WindowH;

	USL_PrintInCenter(s,r);
}

///////////////////////////////////////////////////////////////////////////
//
//	US_CPrintLine() - Prints a string centered on the current line and
//		advances to the next line. Newlines are not supported.
//
///////////////////////////////////////////////////////////////////////////
void
US_CPrintLine(char *s)
{
	Uint16	w,h;

	USL_MeasureString(s,&w,&h);

	if (w > WindowW)
		US1_ERROR(US_CPRINTLINE_WIDTH);
	px = WindowX + ((WindowW - w) / 2);
	py = PrintY;
	USL_DrawString(s);
	PrintY += h;
}

///////////////////////////////////////////////////////////////////////////
//
//	US_CPrint() - Prints a string in the current window. Newlines are
//		supported.
//
///////////////////////////////////////////////////////////////////////////
void
US_CPrint(char *s)
{
    std::vector<char> buffer(
        s,
        s + std::string::traits_type::length(s) + 1);

    s = &buffer[0];

	char	c,*se;

	while (*s)
	{
		se = s;
		while ((c = *se) && (c != '\n'))
			se++;
		*se = '\0';

		US_CPrintLine(s);

		s = se;
		if (c)
		{
			*se = c;
			s++;
		}
	}
}

///////////////////////////////////////////////////////////////////////////
//
//	US_ClearWindow() - Clears the current window to white and homes the
//		cursor
//
///////////////////////////////////////////////////////////////////////////
void
US_ClearWindow(void)
{
	VWB_Bar(WindowX,WindowY,WindowW,WindowH,0xEF);
	PrintX = WindowX;
	PrintY = WindowY;
}

///////////////////////////////////////////////////////////////////////////
//
//	US_DrawWindow() - Draws a frame and sets the current window parms
//
///////////////////////////////////////////////////////////////////////////
void
US_DrawWindow(Uint16 x,Uint16 y,Uint16 w,Uint16 h)
{
	Uint16	i,
			sx,sy,sw,sh;

	WindowX = x * 8;
	WindowY = y * 8;
	WindowW = w * 8;
	WindowH = h * 8;

	PrintX = WindowX;
	PrintY = WindowY;

	sx = (x - 1) * 8;
	sy = (y - 1) * 8;
	sw = (w + 1) * 8;
	sh = (h + 1) * 8;

	US_ClearWindow();

	VWB_DrawTile8(sx,sy,0),VWB_DrawTile8(sx,sy + sh,5);
	for (i = sx + 8;i <= sx + sw - 8;i += 8)
		VWB_DrawTile8(i,sy,1),VWB_DrawTile8(i,sy + sh,6);
	VWB_DrawTile8(i,sy,2),VWB_DrawTile8(i,sy + sh,7);

	for (i = sy + 8;i <= sy + sh - 8;i += 8)
		VWB_DrawTile8(sx,i,3),VWB_DrawTile8(sx + sw,i,4);
}

///////////////////////////////////////////////////////////////////////////
//
//	US_CenterWindow() - Generates a window of a given width & height in the
//		middle of the screen
//
///////////////////////////////////////////////////////////////////////////
void
US_CenterWindow(Uint16 w,Uint16 h)
{
	US_DrawWindow(((MaxX / 8) - w) / 2,((MaxY / 8) - h) / 2,w,h);
}

///////////////////////////////////////////////////////////////////////////
//
//	US_SaveWindow() - Saves the current window parms into a record for
//		later restoration
//
///////////////////////////////////////////////////////////////////////////
void
US_SaveWindow(WindowRec *win)
{
	win->x = WindowX;
	win->y = WindowY;
	win->w = WindowW;
	win->h = WindowH;

	win->px = PrintX;
	win->py = PrintY;
}

///////////////////////////////////////////////////////////////////////////
//
//	US_RestoreWindow() - Sets the current window parms to those held in the
//		record
//
///////////////////////////////////////////////////////////////////////////
void
US_RestoreWindow(WindowRec *win)
{
	WindowX = win->x;
	WindowY = win->y;
	WindowW = win->w;
	WindowH = win->h;

	PrintX = win->px;
	PrintY = win->py;
}

//	Input routines

///////////////////////////////////////////////////////////////////////////
//
//	USL_XORICursor() - XORs the I-bar text cursor. Used by US_LineInput()
//
///////////////////////////////////////////////////////////////////////////
static void
USL_XORICursor(Sint16 x,Sint16 y,char *s,Uint16 cursor)
{
	static	boolean	status;		// VGA doesn't XOR...
	char	buf[MaxString];
	Sint16		temp;
	Uint16	w,h;

	strcpy(buf,s);
	buf[cursor] = '\0';
	USL_MeasureString(buf,&w,&h);

	px = x + w - 1;
	py = y;

	VL_WaitVBL(1);				

	if (status^=1)
	{
		USL_DrawString("\x80");
	}
	else
	{
		temp = fontcolor;
		fontcolor = backcolor;
		USL_DrawString("\x80");
		fontcolor = temp;
	}

}


// JAM BEGIN - New Function
///////////////////////////////////////////////////////////////////////////
//
//	USL_CustomCursor() - Toggle Displays the custom text cursor.
//	Used by US_LineInput()
//
///////////////////////////////////////////////////////////////////////////
static void USL_CustomCursor(Sint16 x,Sint16 y,char *s,Uint16 cursor)
{
	static	boolean	status;		// VGA doesn't XOR...
	char	buf[MaxString];
	Sint16		temp,temp_font;
	Uint16	w,h;

	strcpy(buf,s);
	buf[cursor] = '\0';
	USL_MeasureString(buf,&w,&h);

	px = x + w - 1;
	py = y;

	temp = fontcolor;
	temp_font = fontnumber;

	fontnumber = US_CustomCursor.font_number;

	if (status^=1)
		fontcolor = US_CustomCursor.cursor_color;
	else
		fontcolor = backcolor;

	VL_WaitVBL(1);

	USL_DrawString(&US_CustomCursor.cursor_char);
	fontcolor = temp;
	fontnumber = temp_font;

}
// JAM END - New Function

///////////////////////////////////////////////////////////////////////////
//
//	US_LineInput() - Gets a line of user input at (x,y), the string defaults
//		to whatever is pointed at by def. Input is restricted to maxchars
//		chars or maxwidth pixels wide. If the user hits escape (and escok is
//		true), nothing is copied into buf, and false is returned. If the
//		user hits return, the current string is copied into buf, and true is
//		returned
//
///////////////////////////////////////////////////////////////////////////
boolean US_LineInput(Sint16 x,Sint16 y,char *buf,char *def,boolean escok,
				Sint16 maxchars,Sint16 maxwidth)
{
	boolean		redraw,
				cursorvis,cursormoved,
				done,result;
	ScanCode	sc;
	char		c,
				s[MaxString],olds[MaxString];
	Uint16		i,
				cursor,
				w,h,
				len,temp;
	Uint32	lasttime;

	if (def)
		strcpy(s,def);
	else
		*s = '\0';
	*olds = '\0';
	cursor = strlen(s);
	cursormoved = redraw = true;

	cursorvis = done = false;
	lasttime = TimeCount;
	LastASCII = key_None;
	LastScan = sc_None;

	while (!done)
	{
		if (cursorvis)
		{
			if (use_custom_cursor)						// JAM
				USL_CustomCursor(x,y,s,cursor);				// JAM
			else
				USL_XORICursor(x,y,s,cursor);
		}

// FIXME
#if 0
	asm	pushf
	asm	cli
#endif // 0

        in_handle_events();

		sc = LastScan;
		LastScan = sc_None;
		c = LastASCII;
		LastASCII = key_None;

// FIXME
#if 0
	asm	popf
#endif // 0

		switch (sc)
		{
#if 0				

		case sc_LeftArrow:
			if (cursor)
				cursor--;
			c = key_None;
			cursormoved = true;
         redraw = use_custom_cursor;		// JAM -
			break;
		case sc_RightArrow:
			if (s[cursor])
				cursor++;
			c = key_None;
			cursormoved = true;
         redraw = use_custom_cursor;		// JAM -
			break;
		case sc_Home:
			cursor = 0;
			c = key_None;
			cursormoved = true;
         redraw = use_custom_cursor;		// JAM -
			break;
		case sc_End:
			cursor = strlen(s);
			c = key_None;
			cursormoved = true;
			break;

#endif 	

		case sc_Return:
			strcpy(buf,s);
			done = true;
			result = true;
			c = key_None;
			break;
		case sc_Escape:
			if (escok)
			{
				done = true;
				result = false;
			}
			c = key_None;
			break;

		case sc_BackSpace:
			if (cursor)
			{
				strcpy(s + cursor - 1,s + cursor);
				cursor--;
				redraw = true;
			}
			c = key_None;
			cursormoved = true;
			break;

#if 0		  
		case sc_Delete:
			if (s[cursor])
			{
				strcpy(s + cursor,s + cursor + 1);
				redraw = true;
			}
			c = key_None;
			cursormoved = true;
			break;
#endif

		case 0x4c:	// Keypad 5
		case sc_UpArrow:
		case sc_DownArrow:
		case sc_PgUp:
		case sc_PgDn:
		case sc_Insert:
			c = key_None;
			break;
		}

		if (c)
		{
			len = strlen(s);
			USL_MeasureString(s,&w,&h);

			if	( isprint(c) &&	(len < MaxString - 1)
				&&	((!maxchars) || (len < maxchars))
				&&	((!maxwidth) || (w < maxwidth)))
			{
				for (i = len + 1;i > cursor;i--)
					s[i] = s[i - 1];
				s[cursor++] = c;
				redraw = true;
			}
		}

		if (redraw)
		{
			px = x;
			py = y;
			temp = fontcolor;
			fontcolor = backcolor;
			USL_DrawString(olds);
			fontcolor = temp;
			strcpy(olds,s);

			px = x;
			py = y;
			USL_DrawString(s);

			redraw = false;
		}

		if (cursormoved)
		{
			cursorvis = false;
			lasttime = TimeCount - TickBase;

			cursormoved = false;
		}

		if (TimeCount - lasttime > TickBase / 2)
		{
			lasttime = TimeCount;

			cursorvis ^= true;
		}

		if (cursorvis)
			if (use_custom_cursor)						// JAM
				USL_CustomCursor(x,y,s,cursor);				// JAM
			else
				USL_XORICursor(x,y,s,cursor);

		VW_UpdateScreen();
	}

	if (cursorvis)
		if (use_custom_cursor)						// JAM
			USL_CustomCursor(x,y,s,cursor);				// JAM
		else
			USL_XORICursor(x,y,s,cursor);

	if (!result)
	{
		px = x;
		py = y;
		USL_DrawString(olds);
	}
	VW_UpdateScreen();

	IN_ClearKeysDown();
	return(result);
}

