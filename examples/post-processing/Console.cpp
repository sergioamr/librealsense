#include "Console.h"

#include <windows.h>
#include <stdio.h>

CConsole debug;

void MouseEventProc(MOUSE_EVENT_RECORD);
void ResizeEventProc(WINDOW_BUFFER_SIZE_RECORD);
void KeyEventProc(KEY_EVENT_RECORD);
void GetInputEvents(VOID);

//------- Control de teclas pulsadas para debug ------------
char Teclado[4096];

#define ERR(bSuccess) { if(!bSuccess) return; };
#define CHECK(hHandle) { if(hHandle == NULL) return; };

HANDLE hPort = NULL;
HANDLE hReadThread = NULL;
HANDLE hCom = NULL;
UINT uTimerID = 0;

DWORD _stdcall PortReadThread(LPVOID lpvoid);


int WriteActual = 0;
int WritePos = 0;
char BufferWrite[4096];

int Recepcion = 0;
int Escritura = 0;

bool salirport = false;

void Serial_Close() {

	salirport = true;

	//	if (uTimerID>0)
	//		timeKillEvent(uTimerID);

	uTimerID = 0;

	if (hPort != NULL)
		CloseHandle(hPort);

	hPort = NULL;

}

long Serial_read(HANDLE hPort, char *read_buf, long limit_len)
{
	DWORD dwBytesTransferred = 0;
	DWORD dwNumBytesWritten = 0;

	long buf_len = 0;

	BYTE Byte;
	DWORD dwError;

	do
	{
		if (salirport) return (-1);
		ReadFile(hPort, &Byte, 1, &dwBytesTransferred, 0);

		if (dwBytesTransferred == 1) {
			debug.printf(0, 2, " [Recibido :: 0x%x %c] %i     ", Byte, Byte, Recepcion);
			Recepcion++;
			//ProcessChar (Byte);
		}

		if (WriteActual != WritePos) {
			if (!WriteFile(hPort,
				&BufferWrite[WritePos],
				WriteActual - WritePos,
				&dwNumBytesWritten,
				NULL)) {

				dwError = GetLastError();
				debug.printf("! Buffer Error al escribir \n");
			}

			Escritura += dwNumBytesWritten;
			debug.printf(0, 1, " [Buffer Pos:%i Tam:%i] (%i)", WritePos, WriteActual - WritePos, Escritura);
			WritePos += dwNumBytesWritten;

			if (WriteActual == WritePos) {
				WriteActual = WritePos = 0;
			}

		}

	} while (dwBytesTransferred > 0 || dwNumBytesWritten > 0);

	return buf_len;
}

char buffer[1024];

extern "C" void CALLBACK internalCloseClip(UINT id, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	int len = Serial_read(hPort, buffer, sizeof(buffer));
}

int LaunchConsoleTest()
{
	// uTimerID = timeSetEvent(10,50, internalCloseClip,(DWORD) 0,TIME_PERIODIC);

	if (NULL == uTimerID)
		debug.printf(" * Error al lanzar el thread de escuchar puerto");

	return true;
}
/***********************************************************************

PortInitialize (LPTSTR lpszPortName)

***********************************************************************/
BOOL PortInitialize(LPTSTR lpszPortName, int BaudRate)
{

	DWORD dwError;

	DCB PortDCB;
	COMMTIMEOUTS CommTimeouts;

	salirport = false;

	debug.printf("-----------------------------\n");
	debug.printf("* Serial port init %s :: %i \n", lpszPortName, BaudRate);

	hPort = CreateFile(lpszPortName, // Pointer to the name of the port
		GENERIC_READ | GENERIC_WRITE,        // Access (read/write) mode
		FILE_SHARE_READ | FILE_SHARE_WRITE,  // Share mode
		NULL,         // Pointer to the security attribute
		OPEN_EXISTING,// How to open the serial port
		0,            // Port attributes
		NULL);        // Handle to port with attribute
					  // to copy

	if (hPort == INVALID_HANDLE_VALUE)
	{
		debug.printf("! Error al abrir el puerto \n");
		dwError = GetLastError();
		return FALSE;
	}

	PortDCB.DCBlength = sizeof(DCB);

	GetCommState(hPort, &PortDCB);

	PortDCB.BaudRate = BaudRate;              // Current baud
	PortDCB.fBinary = TRUE;               // Binary mode; no EOF check
	PortDCB.fParity = TRUE;               // Enable parity checking.
	PortDCB.fOutxCtsFlow = FALSE;         // No CTS output flow control
	PortDCB.fOutxDsrFlow = FALSE;         // No DSR output flow control
	PortDCB.fDtrControl = DTR_CONTROL_ENABLE;
	// DTR flow control type
	PortDCB.fDsrSensitivity = FALSE;      // DSR sensitivity
	PortDCB.fTXContinueOnXoff = TRUE;     // XOFF continues Tx
	PortDCB.fOutX = FALSE;                // No XON/XOFF out flow control
	PortDCB.fInX = FALSE;                 // No XON/XOFF in flow control
	PortDCB.fErrorChar = FALSE;           // Disable error replacement.
	PortDCB.fNull = FALSE;                // Disable null stripping.
	PortDCB.fRtsControl = RTS_CONTROL_ENABLE;
	// RTS flow control
	PortDCB.fAbortOnError = FALSE;        // Do not abort reads/writes on
										  // error.
	PortDCB.ByteSize = 8;                 // Number of bits/bytes, 4-8
	PortDCB.Parity = NOPARITY;            // 0-4=no,odd,even,mark,space
	PortDCB.StopBits = ONESTOPBIT;        // 0,1,2 = 1, 1.5, 2

										  // Configure the port according to the specifications of the DCB structure.

	if (!SetCommState(hPort, &PortDCB))
	{    // Could not configure the serial port.
		debug.printf("Unable to configure the serial port \n");
		dwError = GetLastError();
		return FALSE;
	}


	GetCommTimeouts(hPort, &CommTimeouts);
	CommTimeouts.ReadIntervalTimeout = 50;
	CommTimeouts.ReadTotalTimeoutMultiplier = 50;
	CommTimeouts.ReadTotalTimeoutConstant = 50;
	CommTimeouts.WriteTotalTimeoutMultiplier = 50;
	CommTimeouts.WriteTotalTimeoutConstant = 50;

	CommTimeouts.ReadIntervalTimeout = 10;
	CommTimeouts.ReadTotalTimeoutMultiplier = 10;
	CommTimeouts.ReadTotalTimeoutConstant = 10;
	CommTimeouts.WriteTotalTimeoutMultiplier = 10;
	CommTimeouts.WriteTotalTimeoutConstant = 50;


	/*
	CommTimeouts.ReadIntervalTimeout = MAXWORD;
	CommTimeouts.ReadTotalTimeoutMultiplier = 0;
	CommTimeouts.ReadTotalTimeoutConstant = 0;
	CommTimeouts.WriteTotalTimeoutMultiplier = 10;
	CommTimeouts.WriteTotalTimeoutConstant = 1000;
	*/
	SetCommTimeouts(hPort, &CommTimeouts);

	SetupComm(hPort, 10240, 10240);

	char cad[1024];
	DWORD dwBytesTransferred = 0;
	ReadFile(hPort, &cad, sizeof(cad), &dwBytesTransferred, 0);

	return TRUE;
}


/***********************************************************************
PortWrite (BYTE Byte)
***********************************************************************/

void PortWrite(BYTE Byte)
{
	//debug.printf ("* Escribiendo |0x%x| Actual %i Pos %i \n",Byte, WriteActual, WritePos);
	BufferWrite[WriteActual++] = Byte;
}

/***********************************************************************
PortClose (HANDLE hCommPort)
***********************************************************************/

BOOL PortClose()
{
	Serial_Close();
	return FALSE;
}


BOOL SetConsoleCXCY(HANDLE hStdout, int cx, int cy)
{
	CONSOLE_SCREEN_BUFFER_INFO	info;
	COORD						coordMax;

	coordMax = GetLargestConsoleWindowSize(hStdout);

	if (cy > coordMax.Y)
		cy = coordMax.Y;

	if (cx > coordMax.X)
		cx = coordMax.X;

	if (!GetConsoleScreenBufferInfo(hStdout, &info))
		return FALSE;

	info.srWindow.Left = 0;
	info.srWindow.Right = info.dwSize.X - 1;
	info.srWindow.Top = 0;
	info.srWindow.Bottom = cy - 1;

	if (cy < info.dwSize.Y)
	{
		if (!SetConsoleWindowInfo(hStdout, TRUE, &info.srWindow))
			return FALSE;

		info.dwSize.Y = cy;

		if (!SetConsoleScreenBufferSize(hStdout, info.dwSize))
			return FALSE;
	}
	else if (cy > info.dwSize.Y)
	{
		info.dwSize.Y = cy;

		if (!SetConsoleScreenBufferSize(hStdout, info.dwSize))
			return FALSE;

		if (!SetConsoleWindowInfo(hStdout, TRUE, &info.srWindow))
			return FALSE;
	}

	if (!GetConsoleScreenBufferInfo(hStdout, &info))
		return FALSE;

	info.srWindow.Left = 0;
	info.srWindow.Right = cx - 1;
	info.srWindow.Top = 0;
	info.srWindow.Bottom = info.dwSize.Y - 1;

	if (cx < info.dwSize.X)
	{
		if (!SetConsoleWindowInfo(hStdout, TRUE, &info.srWindow))
			return FALSE;

		info.dwSize.X = cx;

		if (!SetConsoleScreenBufferSize(hStdout, info.dwSize))
			return FALSE;
	}
	else if (cx > info.dwSize.X)
	{
		info.dwSize.X = cx;

		if (!SetConsoleScreenBufferSize(hStdout, info.dwSize))
			return FALSE;

		if (!SetConsoleWindowInfo(hStdout, TRUE, &info.srWindow))
			return FALSE;
	}

	return TRUE;
}


VOID MouseEventProc(MOUSE_EVENT_RECORD) {
}

VOID ResizeEventProc(WINDOW_BUFFER_SIZE_RECORD) {

}

/*
typedef struct _KEY_EVENT_RECORD {
BOOL bKeyDown;
WORD wRepeatCount;
WORD wVirtualKeyCode;
WORD wVirtualScanCode;
union {
WCHAR UnicodeChar;
CHAR   AsciiChar;
} uChar;
DWORD dwControlKeyState;
} KEY_EVENT_RECORD, *PKEY_EVENT_RECORD;
*/

VOID GetInputEvents(VOID) {

}

int CConsole::Read(const char *dest, int limit) {

	unsigned long tam;

	if (!ReadFile(hConIn, (void *)dest, limit, &tam, 0)) {
		return (0);
	};

	printf(" %i \n", tam);
	return tam;
}

char CConsole::ReadChar(const char* szOutput, ...) {

	if (!hConsole) return false;
	if (!hConIn) return false;

	char		out[256];
	va_list		va;

	// if not parameter set, write a new line
	if (szOutput != NULL)
	{
		va_start(va, szOutput);
		vsprintf(out, szOutput, va);
		va_end(va);
		printf_(out);
	}

	unsigned long tam;
	char buffer[256];
	memset(buffer, 0, sizeof(buffer));

	ReadFile(hConIn, buffer, 1, &tam, 0);

	ClearLine(CY_TAM - 1);

	return (buffer[0]);
}


bool CConsole::ReadConfirmation(const char* szOutput, ...) {


	if (!hConsole) return false;
	if (!hConIn) return false;

	char		out[256];
	va_list		va;

	if (szOutput != NULL)
	{
		va_start(va, szOutput);
		vsprintf(out, szOutput, va);
		va_end(va);
		printf_(out);
	}

	unsigned long tam;
	char buffer[256];
	memset(buffer, 0, sizeof(buffer));

	ReadFile(hConIn, buffer, 1, &tam, 0);

	if (tam == 0)
		return (false);

	strlwr(buffer);

	if (!strcmp(buffer, "yes")) return (true);
	if (!strcmp(buffer, "true")) return (true);
	if (!strcmp(buffer, "1")) return (true);
	if (!strcmp(buffer, "ok")) return (true);
	if (!strcmp(buffer, "on")) return (true);
	if (!strcmp(buffer, "si")) return (true);

	size_t len = strlen(buffer);

	int ref = 0;
	for (size_t t = 0; t < len; t++) {
		if (buffer[t] == ' ') ref++;
		else t = len;
	}

	switch (buffer[ref]) {
	case 's': return (true);
	case 'y': return (true);
	}

	ClearLine(CY_TAM - 1);

	return false;
}

PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo = NULL;

bool CConsole::Create(const char *szTitle, bool bNoClose)
{

	memset(Teclado, 0, sizeof(Teclado));

	if (lpConsoleScreenBufferInfo == NULL)
		lpConsoleScreenBufferInfo = (_CONSOLE_SCREEN_BUFFER_INFO *)malloc(sizeof(_CONSOLE_SCREEN_BUFFER_INFO));

	if (hConsole != NULL)
		return false;

	//GetConsoleProcessList

	if (!AllocConsole()) {
		//FreeConsole ();
		//AllocConsole();
		debug.printf(" * Consola ya creada \n");

		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		hConIn = GetStdHandle(STD_INPUT_HANDLE);

		if (szTitle != NULL)
			SetConsoleTitle(szTitle);

		SetConsoleCXCY(hConsole, CX_TAM, CY_TAM);
		SetPos(0, CY_TAM - 1);

		SetWindowPos((HWND) GetHWND(), HWND_TOPMOST, 2560, 500, 0, 0, SWP_NOSIZE);
		return (true);
		//return false;
	}

	hMutex = CreateMutex(NULL, false, "DebugConsoleMutex");

	hConsole = CreateFile("CONOUT$", GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
	if (hConsole == INVALID_HANDLE_VALUE)
		return false;

	if (SetConsoleMode(hConsole, ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT) == 0)
		return false;


	hConIn = CreateFile("CONIN$", GENERIC_READ | GENERIC_WRITE,

		FILE_SHARE_READ, NULL, OPEN_EXISTING,

		FILE_ATTRIBUTE_NORMAL, 0);

	//SetConsoleMode(hConIn,ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT );
	SetConsoleMode(hConIn, !ENABLE_LINE_INPUT);


	if (bNoClose)
		DisableClose();

	SetConsoleCXCY(hConsole, CX_TAM, CY_TAM);

	Show(true);

	if (szTitle != NULL)
		SetConsoleTitle(szTitle);

	SetPos(0, CY_TAM - 1);
	//SetWindowPos ( GetHWND(), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);
	SetWindowPos((HWND) GetHWND(), HWND_TOPMOST, 2560, 10, 0, 0, SWP_NOSIZE);
	return true;
}

void CConsole::Color(WORD wColor)
{
	CHECK(hConsole);
	if (wColor != NULL)
		SetConsoleTextAttribute(hConsole, wColor);
	else
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // white text on black bg
}

/*
void CConsole:: ErrorLineEnd () {


BOOL fSuccess;
DWORD cWritten;

WORD wColors[1], wColor;


wColors[0] = BACKGROUND_RED;
wColors[1] = BACKGROUND_RED |     // white background
BACKGROUND_GREEN |
BACKGROUND_BLUE;
wColors[2] = BACKGROUND_BLUE;
}
*/


void CConsole::printf_(const char* szOutput, ...)
{

	CHECK(hConsole);

	DWORD		dwWritten;
	char		out[256];
	va_list		va;

	// if not parameter set, write a new line
	if (szOutput == NULL)
		return;
	else
	{
		va_start(va, szOutput);
		vsprintf(out, szOutput, va);
		va_end(va);
	}

	size_t len = strlen(out);
	int next = 0;
	setrgb(0);

	for (size_t t = 0; t < len; t++) {

		switch (out[t]) {
		case '!':	setrgb(16); next = 16; 	break;
		case '-':	setrgb(3); next = 0;  	break;
		case '[':	setrgb(2); next = 6;  	break;
		case ']':	setrgb(2); next = 0; 		break;
		case '|':	setrgb(1); next = 0;  	break;
		case '<':	setrgb(2); next = 0; 		break;
		case '>':	setrgb(1); next = 0;  	break;

			break;
		}

		//SetPos(0, CY_TAM-1);

		WriteConsole(hConsole, out + t, 1, &dwWritten, 0);
		setrgb(next);
	}


}

void CConsole::printf(const char* szOutput, ...)
{

	CHECK(hConsole);

	WaitForSingleObject(hMutex, INFINITE);

	SetPos(0, CY_TAM - 1);

	DWORD		dwWritten;
	char		out[256];
	va_list		va;

	if (Teclado[VK_CONTROL])
		return;

	// if not parameter set, write a new line
	if (szOutput == NULL)
		sprintf(out, "\n");
	// process arguments
	else
	{
		va_start(va, szOutput);
		vsprintf(out, szOutput, va);
		va_end(va);
	}

	// write to the console
	Clear(CY_LOWLIMIT);
	ClearLine(CY_LOWLIMIT - 1);

	int ref = 0;
	// Puede que haya un espacio antes del principio de texto
	//if (out[0]==' ') ref++;

	switch (out[ref]) {
	case '!':	setrgb(16);  	break;
	case '*':	setrgb(6);  	break;
	case '+':	setrgb(2);  	break;
	case '-':	setrgb(3);  	break;
	default:
		setrgb(0);
		break;
	}


	WriteConsole(hConsole, out, strlen(out), &dwWritten, 0);
	setrgb(0);
	ClearLine(CY_TAM - 1);

	ReleaseMutex(hMutex);

}

void CConsole::SetPos(int x, int y)
{
	CHECK(hConsole);
	COORD dwCursorPosition;
	dwCursorPosition.X = x;
	dwCursorPosition.Y = y;
	SetConsoleCursorPosition(hConsole, dwCursorPosition);
}

void CConsole::printf(int x, int y, const char* szOutput, ...)
{
	WaitForSingleObject(hMutex, INFINITE);

	if (Teclado[VK_SHIFT])
		return;

	CHECK(hConsole);

	COORD dwCursorPosition;

	if (lpConsoleScreenBufferInfo == NULL) return;

	GetConsoleScreenBufferInfo(hConsole, lpConsoleScreenBufferInfo);

	DWORD		dwWritten;
	char		out[256];
	va_list		va;

	if (szOutput == NULL)
		sprintf(out, "\n");

	else
	{
		va_start(va, szOutput);
		vsprintf(out, szOutput, va);
		va_end(va);
	}

	size_t len = strlen(out);
	if (out[len - 1] == '\n')
		out[len - 1] = ' ';

	dwCursorPosition.X = x;
	dwCursorPosition.Y = y;
	SetConsoleCursorPosition(hConsole, dwCursorPosition);

	len = strlen(out);
	int next = 0;
	setrgb(0);

	for (size_t t = 0; t < len; t++) {

		switch (out[t]) {
		case '!':	setrgb(16); next = 16; 	break;
		case '-':	setrgb(3); next = 0;  	break;
		case '[':	setrgb(2); next = 6;  	break;
		case ']':	setrgb(2); next = 0; 		break;
		case '|':	setrgb(1); next = 0;  	break;
			break;
		}
		WriteConsole(hConsole, out + t, 1, &dwWritten, 0);
		setrgb(next);
	}

	ReleaseMutex(hMutex);
}

void CConsole::SetTitle(const char *title)
{
	// self-explanatory
	SetConsoleTitle(title);
}

char* CConsole::GetTitle()
{
	static char szWindowTitle[256] = "";
	GetConsoleTitle(szWindowTitle, sizeof(szWindowTitle));

	return szWindowTitle;
}

void *CConsole::GetHWND()
{
	if (hConsole == NULL)
		return NULL;

	return (void *) FindWindow("ConsoleWindowClass", GetTitle());
}

void CConsole::Show(bool bShow)
{
	CHECK(hConsole);

	HWND hWnd = (HWND) GetHWND();
	if (hWnd != NULL)
		ShowWindow(hWnd, SW_HIDE ? SW_SHOW : bShow);
}

void CConsole::DisableClose()
{
	CHECK(hConsole);

	HWND hWnd = (HWND) GetHWND();

	if (hWnd != NULL)
	{
		HMENU hMenu = GetSystemMenu(hWnd, 0);
		if (hMenu != NULL)
		{
			DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
			DrawMenuBar(hWnd);
		}
	}
}

void CConsole::ClearLine(int line)
{
	CHECK(hConsole);

	COORD coordScreen = { 0, line };

	BOOL bSuccess;
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwConSize;

	/* get the number of character cells in the current buffer */

	bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
	ERR(bSuccess);
	dwConSize = csbi.dwSize.X;

	/* fill the entire screen with blanks */

	bSuccess = FillConsoleOutputCharacter(hConsole, (TCHAR) ' ', dwConSize, coordScreen, &cCharsWritten);
	ERR(bSuccess);

	/* get the current text attribute */

	bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
	ERR(bSuccess);

	/* now set the buffer's attributes accordingly */

	bSuccess = FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);
	ERR(bSuccess);

	/* put the cursor at (0, 0) */

	//bSuccess = SetConsoleCursorPosition( hConsole, coordScreen );
	ERR(bSuccess);
}

void CConsole::Clear(int lines)
{
	CHECK(hConsole);

	/***************************************/
	// This code is from one of Microsoft's
	// knowledge base articles, you can find it at
	// http://support.microsoft.com/default.aspx?scid=KB;EN-US;q99261&
	/***************************************/

	COORD coordScreen = { 0, 0 };

	BOOL bSuccess;
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
	DWORD dwConSize;

	/* get the number of character cells in the current buffer */

	bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
	ERR(bSuccess);
	dwConSize = csbi.dwSize.X * lines;

	/* fill the entire screen with blanks */

	bSuccess = FillConsoleOutputCharacter(hConsole, (TCHAR) ' ', dwConSize, coordScreen, &cCharsWritten);
	ERR(bSuccess);

	/* get the current text attribute */

	bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
	ERR(bSuccess);

	/* now set the buffer's attributes accordingly */

	bSuccess = FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);
	ERR(bSuccess);

	/* put the cursor at (0, 0) */

	//bSuccess = SetConsoleCursorPosition( hConsole, coordScreen );
	ERR(bSuccess);

	SetPos(0, CY_TAM - 1);
}

void CConsole::Clear()
{
	CHECK(hConsole);

	/***************************************/
	// This code is from one of Microsoft's
	// knowledge base articles, you can find it at
	// http://support.microsoft.com/default.aspx?scid=KB;EN-US;q99261&
	/***************************************/

	COORD coordScreen = { 0, 0 };

	BOOL bSuccess;
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi; /* to get buffer info */
	DWORD dwConSize;

	/* get the number of character cells in the current buffer */

	bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
	ERR(bSuccess);
	dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

	/* fill the entire screen with blanks */

	bSuccess = FillConsoleOutputCharacter(hConsole, (TCHAR) ' ', dwConSize, coordScreen, &cCharsWritten);
	ERR(bSuccess);

	/* get the current text attribute */

	bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
	ERR(bSuccess);

	/* now set the buffer's attributes accordingly */

	bSuccess = FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);
	ERR(bSuccess);

	/* put the cursor at (0, 0) */

	bSuccess = SetConsoleCursorPosition(hConsole, coordScreen);
	ERR(bSuccess);

	SetPos(0, CY_TAM - 1);

}


HANDLE CConsole::GetHandle()
{


	return hConsole;
}

void CConsole::Close()
{

	printf(" Destruyendo consola \n");
	printf("------------------------------------------------------");
	FreeConsole();
	hConsole = NULL;
};

void setrgb(int color)
{
	switch (color)
	{
	case 0:    // White on Black
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
			FOREGROUND_INTENSITY | FOREGROUND_RED |
			FOREGROUND_GREEN | FOREGROUND_BLUE);
		break;
	case 1:    // Red on Black
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
			FOREGROUND_INTENSITY | FOREGROUND_RED);
		break;
	case 2:    // Green on Black
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
			FOREGROUND_INTENSITY | FOREGROUND_GREEN);
		break;
	case 3:    // Yellow on Black
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
			FOREGROUND_INTENSITY | FOREGROUND_RED |
			FOREGROUND_GREEN);
		break;
	case 4:    // Blue on Black
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
			FOREGROUND_INTENSITY | FOREGROUND_BLUE);
		break;
	case 5:    // Magenta on Black
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
			FOREGROUND_INTENSITY | FOREGROUND_RED |
			FOREGROUND_BLUE);
		break;
	case 6:    // Cyan on Black
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
			FOREGROUND_INTENSITY | FOREGROUND_GREEN |
			FOREGROUND_BLUE);
		break;
	case 7:    // Black on Gray
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
			BACKGROUND_INTENSITY | BACKGROUND_INTENSITY);
		break;
	case 8:    // Black on White
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
			BACKGROUND_INTENSITY | FOREGROUND_INTENSITY |
			BACKGROUND_RED | BACKGROUND_GREEN |
			BACKGROUND_BLUE);
		break;
	case 9:    // Red on White
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
			BACKGROUND_INTENSITY | FOREGROUND_INTENSITY |
			BACKGROUND_RED | BACKGROUND_GREEN |
			BACKGROUND_BLUE |
			FOREGROUND_RED);
		break;
	case 10:    // Green on White
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
			BACKGROUND_INTENSITY | FOREGROUND_INTENSITY |
			BACKGROUND_RED | BACKGROUND_GREEN |
			BACKGROUND_BLUE | FOREGROUND_GREEN);
		break;
	case 11:    // Yellow on White
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
			BACKGROUND_INTENSITY | FOREGROUND_INTENSITY |
			BACKGROUND_RED | BACKGROUND_GREEN |
			BACKGROUND_BLUE | FOREGROUND_RED |
			FOREGROUND_GREEN);
		break;
	case 12:    // Blue on White
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
			BACKGROUND_INTENSITY | FOREGROUND_INTENSITY |
			BACKGROUND_RED | BACKGROUND_GREEN |
			BACKGROUND_BLUE | FOREGROUND_BLUE);
		break;
	case 13:    // Magenta on White
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
			BACKGROUND_INTENSITY | FOREGROUND_INTENSITY |
			BACKGROUND_RED | BACKGROUND_GREEN |
			BACKGROUND_BLUE | FOREGROUND_RED |
			FOREGROUND_BLUE);
		break;
	case 14:    // Cyan on White
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
			BACKGROUND_INTENSITY | FOREGROUND_INTENSITY |
			BACKGROUND_RED | BACKGROUND_GREEN |
			BACKGROUND_BLUE | FOREGROUND_GREEN |
			FOREGROUND_BLUE);
		break;
	case 15:    // White on White
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
			BACKGROUND_INTENSITY | FOREGROUND_INTENSITY |
			BACKGROUND_RED | BACKGROUND_GREEN |
			BACKGROUND_BLUE | FOREGROUND_RED |
			FOREGROUND_GREEN | FOREGROUND_BLUE);
		break;

	case 16:    // White on RED
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
			BACKGROUND_RED |
			FOREGROUND_INTENSITY | FOREGROUND_RED |
			FOREGROUND_GREEN | FOREGROUND_BLUE);
		break;

	default:    // White on Black
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
			FOREGROUND_INTENSITY | FOREGROUND_RED |
			FOREGROUND_GREEN | FOREGROUND_BLUE);
		break;
	}
}