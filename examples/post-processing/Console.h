#ifndef CONSOLE_H
#define CONSOLE_H

#undef UNICODE

typedef void *HANDLE;

int LaunchConsoleTest();

#define CX_TAM  80
#define CY_TAM  50

#define CY_LOWLIMIT 10

//------------- INFO ------------------------
#define CX_INFO 10
#define CY_VUMETER0 0
#define CY_VUMETER1 1
#define CY_VTR1POS 2
#define CY_VTR2POS 3
#define CY_TSERIE 4
#define CY_TSERIE2 5
#define CY_TSERIECOUNT 6

//------------- COMMAND ------------------------
#define CX_CMND 50
#define CY_ESCENARIO 0
#define CY_KEY       1
#define CY_FPS       2
#define CY_MOUSEPOS  3
#define CY_WIPE      4
#define CY_CRONO	 5
#define CY_RECT		 6
#define CY_CODIGO	 7
#define CY_CODIGO1	 8
#define CY_CODIGO2	 9

#undef TRACE
#define TRACE debug.printf

void setrgb(int color);

class CConsole
{
public:
	CConsole()
	{
		hConsole = 0;
	};

public:
	// create the console
	bool   Create(const char* szTitle, bool bNoClose = false);

	//LPPOINT lpCursor_pos;

	// set color for output
	void   Color(unsigned short wColor = 0);
	// write output to console

	char CConsole::ReadChar(const char* szOutput, ...);

	void   printf(const char* szOutput = 0, ...);
	void   printf_(const char* szOutput, ...);

	void   printf(int x, int y, const char* szOutput = 0, ...);

	int Read(const char *dest, int limit);
	bool ReadConfirmation(const char* szOutput, ...);

	// set and get title of console
	void   SetTitle(const char* szTitle);
	void   SetPos(int x, int y);

	char*  GetTitle();

	HANDLE hMutex;


	// get HWND and/or HANDLE of console
	void *GetHWND();
	void *GetHandle();

	// show/hide the console
	void   Show(bool bShow = true);
	// disable the [x] button of the console
	void   DisableClose();
	// clear all output
	void   Clear();
	void   Clear(int lines);
	void   ClearLine(int line);

	// close the console and delete it
	void   Close();

private:
	void *hConsole;
	void *hConIn;
};

extern CConsole debug;

#endif