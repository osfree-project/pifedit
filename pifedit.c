/************************  The Qualitas PIF Editor  ***************************
 *									      *
 *	     (C) Copyright 1992, 1993 Qualitas, Inc.  GNU General Public License version 3.    *
 *									      *
 *  MODULE   :	PIFEDIT.C - Main source module for PIFEDIT.EXE	      *
 *									      *
 *  HISTORY  :	Who	When		What				      *
 *		---	----		----				      *
 *		WRL	11 DEC 92	Original revision		      *
 *		WRL	11 MAR 93	Changes for TWT 		      *
 *		RCC	21 JUN 95	Added Ctl3d, tightened code a bit     *
 *									      *
 ******************************************************************************/


#include "main.h"
#include "advanced.h"

typedef UINT (CALLBACK * COMMDLGHOOKPROC)(HWND, UINT, WPARAM, LPARAM);

BOOL	CALLBACK _export PaneMsgProc(HWND hWndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
VOID	Pane_OnCommand(HWND hWnd, UINT id, HWND hWndCtl, WORD codeNotify);
BOOL	Pane_OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam);
VOID	Pane_OnPaint(HWND hWnd);
VOID	Pane_OnPaint_Iconic(HWND hWnd);
VOID	Pane_OnSetFont(HWND hWndCtl, HFONT hFont, BOOL fRedraw);

VOID	InitHelpWindow(HWND hWnd);
VOID	SetHelpText(HWND hWnd, UINT idString);

LRESULT CALLBACK _export NumEdit_SubClassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK _export KeyEdit_SubClassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK _export Button_SubClassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

int	OpenPIF(VOID);
int	SavePIF(VOID);
int	SaveAsPIF(VOID);

int	CheckSave(HWND hWnd);
int	CheckPIFValidity(BOOL fMustBeValid);
int	WritePIF(HWND hWndDlg, LPSTR lpszPIFName);

VOID	MenuFileNew(BOOL fCheck);
VOID	MenuFileOpen(VOID);
int	MenuFileSave(VOID);
int	MenuFileSaveAs(VOID);

VOID	DefaultPIF(VOID);
int	ControlsFromPIF(PPIF pPIF,  HWND hWnd);

BOOL	IsPifFile(LPCSTR lpszFileSpec);
UINT	CALLBACK _export GetOpenFileNameHookProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

VOID	SnapPif(VOID);			// Copy pifModel to pifBackup

BOOL	ValidateHotKey(BOOL fComplain);

void	MakePathName(char *, char *);

#define IDE_HELP	1

#define BUTTON_BKGNDCOLOR RGB(0, 128, 128)
#define ACTIVE_BTN_BORDER_WIDTH 	1

PIFEDIT_GLOBALS Globals;


typedef struct tagCONTROL {
	int	Pane;		// Pane ID
	int	ID;		// Control ID
	BYTE	Type;		// Type of control, EDIT, BUTTON, etc.
	BYTE	Flags;		// EDIT_NUM, etc.
	int	nDefault;	// Default value
	int	nMin;		// Minimum value
	int	nMax;		// Maximum value
	HWND	hWnd;		// HWND of the control
	WNDPROC lpfnOld;	// Original WndProc
} CONTROL, * PCONTROL, FAR * LPCONTROL;

// Control types for CONTROL.Type
// If the control type isn't one of these, it doesn't get the focus,
// and it doesn't have help text.

#define EDIT		1
#define BUTTON		2
#define KEYEDIT 	3
#define LTEXT		4
#define GROUP		5
#define LASTCONTROL	255

// Flags for CONTROL.Flags

#define EDIT_TEXT	0x10			// Text control
#define EDIT_NUM	0x80			// Numeric control
#define EDIT_RANGE	(EDIT_NUM | 0x40)	// Normal range
#define EDIT_OPTVAL	(EDIT_NUM | 0x20)	// Optional -1 is allowed
#define EDIT_NOSEL	0x08			// Kill selection on SETFOCUS

#define BUTTON_HOTGRP	0x80

#define BUTTON_STATE	0x08		// Button is 'down', or 'checked'

CONTROL aControls[] = {
{ 0,	IDD_HELP,	EDIT,	EDIT_NOSEL },

{ 0,	IDB_ENHANCED,	BUTTON },
    { IDD_GENERAL,	IDT_FILENAME,	LTEXT },
    { IDD_GENERAL,	IDE_FILENAME,	EDIT,	EDIT_TEXT },
    { IDD_GENERAL,	IDT_TITLE,	LTEXT },
    { IDD_GENERAL,	IDE_TITLE,	EDIT,	EDIT_TEXT },
    { IDD_GENERAL,	IDT_PARAMS,	LTEXT },
    { IDD_GENERAL,	IDE_PARAMS,	EDIT,	EDIT_TEXT },
    { IDD_GENERAL,	IDT_DIRECTORY,	LTEXT },
    { IDD_GENERAL,	IDE_DIRECTORY,	EDIT,	EDIT_TEXT },
    { IDD_GENERAL,	IDT_VIDMEM,	LTEXT },
    { IDD_GENERAL,	IDB_TEXT,	BUTTON },
    { IDD_GENERAL,	IDB_LOW,	BUTTON },
    { IDD_GENERAL,	IDB_HIGH,	BUTTON },
    { IDD_GENERAL,	IDT_DOSMEM,	LTEXT },
    { IDD_GENERAL,	IDT_DOSMIN,	LTEXT },
    { IDD_GENERAL,	IDE_DOSMIN,	EDIT,	EDIT_OPTVAL,  128,     0,   640 },
    { IDD_GENERAL,	IDT_DOSMAX,	LTEXT },
    { IDD_GENERAL,	IDE_DOSMAX,	EDIT,	EDIT_OPTVAL,  640,     0,   736 },
    { IDD_GENERAL,	IDT_EMSMEM,	LTEXT },
    { IDD_GENERAL,	IDT_EMSMIN,	LTEXT },
    { IDD_GENERAL,	IDE_EMSMIN,	EDIT,	EDIT_RANGE,	0,     0, 16384 },
    { IDD_GENERAL,	IDT_EMSMAX,	LTEXT },
    { IDD_GENERAL,	IDE_EMSMAX,	EDIT,	EDIT_OPTVAL, 1024,     0, 16384 },
    { IDD_GENERAL,	IDT_XMSMEM,	LTEXT },
    { IDD_GENERAL,	IDT_XMSMIN,	LTEXT },
    { IDD_GENERAL,	IDE_XMSMIN,	EDIT,	EDIT_RANGE,	0,     0, 16384 },
    { IDD_GENERAL,	IDT_XMSMAX,	LTEXT },
    { IDD_GENERAL,	IDE_XMSMAX,	EDIT,	EDIT_OPTVAL, 1024,     0, 16384 },
    { IDD_GENERAL,	IDG_DISPLAY,	GROUP },
    { IDD_GENERAL,	IDB_FULLSCREEN, BUTTON },
    { IDD_GENERAL,	IDB_WINDOWED,	BUTTON },
    { IDD_GENERAL,	IDG_EXEC,	GROUP },
    { IDD_GENERAL,	IDB_BACKEXEC,	BUTTON },
    { IDD_GENERAL,	IDB_EXCLEXEC,	BUTTON },
    { IDD_GENERAL,	IDB_ADVANCED,	BUTTON },
    { IDD_GENERAL,	IDB_CLOSEEXIT,	BUTTON },

{ 0,	LASTCONTROL, LASTCONTROL}		// End of table marker
};


/****************************************************************************
 *
 *  FUNCTION :	WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
 *
 *  PURPOSE  :	Main entry point from C runtime startup code
 *
 *  ENTRY    :	HINSTANCE hInstance;	// Handle of current instance
 *		HINSTANCE hPrevInstance;// Handle of previous instance
 *		LPSTR	lpszCmdLine;	// ==> command line tail
 *		int	nCmdShow;	// Parameter to ShowWindow()
 *
 *  RETURNS  :	Return code from PostQuitMessage() or NULL
 *
 ****************************************************************************/

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
	FARPROC	lpfn;
	int 	rc;
	char	szTemp[ 256 ];
	DWORD	dwVersion;

	if (!(GetWinFlags() & WF_PMODE)) { // Real mode
		MessageBoxString(0, hInstance, IDS_REALMODE, MB_OK);
		return (1);
	}

	/* Setup Globals */
	memset(&Globals, 0, sizeof (Globals));
	Globals.hInst = hInstance;
	lstrcpy(Globals.szWindowTitle, "");
	Globals.fUntitled = TRUE;
	Globals.fCheckOnKillFocus = TRUE;

	LoadString(Globals.hInst, IDS_APPNAME, Globals.szAppName, sizeof(Globals.szAppName));
	LoadString(Globals.hInst, IDS_APPTITLE, Globals.szAppTitle, sizeof(Globals.szAppTitle));
	LoadString(Globals.hInst, IDS_UNTITLED, Globals.szUntitled, sizeof(Globals.szUntitled));
	LoadString(Globals.hInst, IDS_FILEMASK, Globals.szFilter, sizeof(Globals.szFilter));

	{
		PSTR	p = Globals.szFilter;

		while (*p) {
			if (*p == '\n') *p = '\0';
			p++;
		}
	}


	// Initialize the COMMDLG OPENFILENAME structure

	Globals.ofn.lStructSize = sizeof(OPENFILENAME);
	Globals.ofn.lpstrFilter = Globals.szFilter;
	Globals.ofn.nFilterIndex = 1;

	Globals.hIcon = LoadIcon(Globals.hInst, MAKEINTRESOURCE(IDI_QIF));

	lpfn = MakeProcInstance((FARPROC) PaneMsgProc, Globals.hInst);
	rc = DialogBox(Globals.hInst, MAKEINTRESOURCE(IDD_FRAME), 0, (DLGPROC) lpfn);
	FreeProcInstance(lpfn);

	if (Globals.hFontHelp)
		DeleteFont(Globals.hFontHelp);

	return (rc);
} /* WinMain */


/****************************************************************************
 *
 *  FUNCTION :	CheckSave(HWND)
 *
 *  PURPOSE  :	Compare original and current .PIF
 *		Ask user if they want to save changes if the two differ
 *
 *  ENTRY    :	HWND	hWnd;		// Parent window handle
 *
 *  RETURNS  :	0	 - No changes or user doesn't want to save 'em
 *		IDCANCEL - User wants to cancel the operation
 *		IDYES	 - User wants to save the changes
 *
 ****************************************************************************/

int CheckSave(HWND hWnd)
{
    register int	rc;
    char	szBuf[128];

    ControlsToPIF(hWnd);

    if (memcmp(&Globals.pifModel, &Globals.pifBackup, sizeof(PIF))) {	// It's changed
	char	szFmt[128];

	LoadString(Globals.hInst, IDS_SAVECHANGES, szFmt, sizeof(szFmt));

	wsprintf(szBuf, szFmt, (LPSTR) (Globals.fUntitled ? Globals.szUntitled : Globals.szFileTitle));

	rc = MessageBox(hWnd, szBuf, Globals.szAppTitle, MB_YESNOCANCEL);
// Returns rc == IDYES, IDNO, or IDCANCEL

	if (rc == IDCANCEL) {		// User pressed CANCEL
	    return (IDCANCEL);
	} else if (rc == IDYES) {	// User pressed YES
	    return (MenuFileSave());
	}
    }

    return (0);
}


/****************************************************************************
 *
 *  FUNCTION :	MenuFileNew(BOOL)
 *
 *  PURPOSE  :	Process the File/New menu selection
 *		Start editting a fresh default .PIF
 *		Reset the window caption
 *
 *  ENTRY    :	BOOL	fCheck; 	// TRUE is CheckSave() required
 *
 *  RETURNS  :	VOID
 *
 ****************************************************************************/

void MenuFileNew(BOOL fCheck)
{
    if (fCheck) {
	if (CheckSave(Globals.hWndDlg) == IDCANCEL)
		return;
    }

    lstrcpy(Globals.szWindowTitle, Globals.szAppTitle);
    lstrcat(Globals.szWindowTitle, " - ");
    lstrcat(Globals.szWindowTitle, Globals.szUntitled);
    SetWindowText(Globals.hWndDlg, Globals.szWindowTitle);

    Globals.fUntitled = TRUE;

    DefaultPIF();

    ControlsFromPIF(&Globals.pifModel, Globals.hWndDlg);
// We don't check the return code from ControlFromPIF() because we just
// created the default PIF models and we should be able to count on something

    SnapPif();
}




/****************************************************************************
 *
 *  FUNCTION :	MenuFileOpen(VOID)
 *
 *  PURPOSE  :	Process the File/Open menu selection
 *
 *  ENTRY    :	VOID
 *
 *  RETURNS  :	VOID
 *
 ****************************************************************************/

VOID MenuFileOpen(VOID)
{
    int	rc;

	PPIF	pPIF = NULL;

    rc = CheckSave(Globals.hWndDlg);
    if (rc == IDCANCEL)
	goto MENUFILEOPEN_EXIT;

	if (OpenPIF()) {	// Open didn't work

	} else {
		pPIF = (PPIF)LocalLock(Globals.hPIF);

		if (!pPIF) {
			MessageBoxString(Globals.hWndDlg, Globals.hInst, IDS_OUTOFMEMORY, MB_OK);
			goto MENUFILEOPEN_EXIT;
		}

		rc = ControlsFromPIF(pPIF, Globals.hWndDlg);

		if (rc) {			// Couldn't set the controls
			MenuFileNew(FALSE); 	// Don't check old contents
			goto MENUFILEOPEN_EXIT;
		}

		ControlsToPIF(Globals.hWndDlg);
		SnapPif();
	}

MENUFILEOPEN_EXIT:
	if (pPIF) { LocalUnlock(Globals.hPIF);  pPIF = NULL; }
	if (Globals.hPIF) { LocalFree(Globals.hPIF);  Globals.hPIF = 0; }

	return;
}


/****************************************************************************
 *
 *  FUNCTION :	MenuFileSave(VOID)
 *
 *  PURPOSE  :	Process the File/Save menu selection
 *
 *  ENTRY    :	VOID
 *
 *  RETURNS  :	VOID
 *
 ****************************************************************************/

int MenuFileSave(VOID)
{
    register int	rc;

    if (Globals.szFile[0] == '\0') {    // Untitled, use SaveAs
		rc = SaveAsPIF();	// Save didn't work
    } else {
		rc = SavePIF(); 	// Save didn't work
    }

// FIXME what if `rc' is set?
    SnapPif();

    return (rc);
}


/****************************************************************************
 *
 *  FUNCTION :	MenuFileSaveAs(VOID)
 *
 *  PURPOSE  :	Process the File/SaveAs menu selection
 *
 *  ENTRY    :	VOID
 *
 *  RETURNS  :	VOID
 *                                                                  
 ****************************************************************************/

int MenuFileSaveAs(VOID)
{
    SaveAsPIF();
// FIXME what is SaveAsPIF() returns an error

    SnapPif();

    return (0);
}


/****************************************************************************
 *
 *  FUNCTION :	CommDlgError(HWND, DWORD)
 *
 *  PURPOSE  :	Convert a COMMDLG error code to a string in a message box
 *
 *  ENTRY    :	HWND	hWnd;		// Parent window handle
 *		DWORD	dwErrorCode;	// COMMDLG error code
 *
 *  RETURNS  :	VOID
 *
 ****************************************************************************/

VOID CommDlgError(HWND hWnd, DWORD dwErrorCode)
{
    register WORD	id;
    char	szBuf[128];		// Buffer for LoadString

    switch (dwErrorCode) {
	case CDERR_DIALOGFAILURE:   id = IDS_COMMDLGERR;	break;
	case CDERR_STRUCTSIZE:	    id = IDS_COMMDLGERR;	break;
	case CDERR_INITIALIZATION:  id = IDS_COMMDLGERR;	break;
	case CDERR_NOTEMPLATE:	    id = IDS_COMMDLGERR;	break;
	case CDERR_NOHINSTANCE:     id = IDS_COMMDLGERR;	break;
	case CDERR_LOADSTRFAILURE:  id = IDS_COMMDLGERR;	break;
	case CDERR_FINDRESFAILURE:  id = IDS_COMMDLGERR;	break;
	case CDERR_LOADRESFAILURE:  id = IDS_COMMDLGERR;	break;
	case CDERR_LOCKRESFAILURE:  id = IDS_COMMDLGERR;	break;

	case CDERR_MEMALLOCFAILURE: id = IDS_OUTOFMEMORY;	break;
	case CDERR_MEMLOCKFAILURE:  id = IDS_OUTOFMEMORY;	break;

	case CDERR_NOHOOK:	    id = IDS_COMMDLGERR;	break;
	case PDERR_SETUPFAILURE:    id = IDS_COMMDLGERR;	break;
	case PDERR_PARSEFAILURE:    id = IDS_COMMDLGERR;	break;
	case PDERR_RETDEFFAILURE:   id = IDS_COMMDLGERR;	break;
	case PDERR_LOADDRVFAILURE:  id = IDS_COMMDLGERR;	break;
	case PDERR_GETDEVMODEFAIL:  id = IDS_COMMDLGERR;	break;
	case PDERR_INITFAILURE:     id = IDS_COMMDLGERR;	break;
	case PDERR_NODEVICES:	    id = IDS_COMMDLGERR;	break;
	case PDERR_NODEFAULTPRN:    id = IDS_COMMDLGERR;	break;
	case PDERR_DNDMMISMATCH:    id = IDS_COMMDLGERR;	break;
	case PDERR_CREATEICFAILURE: id = IDS_COMMDLGERR;	break;
	case PDERR_PRINTERNOTFOUND: id = IDS_COMMDLGERR;	break;
	case CFERR_NOFONTS:	    id = IDS_COMMDLGERR;	break;
	case FNERR_SUBCLASSFAILURE: id = IDS_COMMDLGERR;	break;
	case FNERR_INVALIDFILENAME: id = IDS_COMMDLGERR;	break;
	case FNERR_BUFFERTOOSMALL:  id = IDS_COMMDLGERR;	break;

	case 0: 	// User may have hit CANCEL or we got a random error
	default:
	    return;
    }

    if (!LoadString(Globals.hInst, id , szBuf, sizeof(szBuf))) {
		MessageBox(hWnd, COMMDLGFAIL_STR, NULL, MB_ICONEXCLAMATION);
	return;
    }

    MessageBox(hWnd, szBuf, Globals.szAppTitle, MB_OK);
}

/****************************************************************************
 *
 *  FUNCTION :	IsPifFile(LPCSTR)
 *
 *  PURPOSE  :	Callback for open file common dialog to check on validity
 *		of file before closing the dialog when the user clicks OK.
 *
 *  ENTRY    :	LPCSTR	lpszFileSpec;	// ==> filename
 *
 *  RETURNS  :	TRUE if dialog should close
 *
 ****************************************************************************/

BOOL IsPifFile(LPCSTR lpszFileSpec)
{
    DWORD	dwFileLen;
    WORD	wFileLen;
    int 	rc;
    DWORD	dwVersion;
    PIF 	pif;
    PPIF	pPIF;
    OFSTRUCT	OpenBuff;

    HFILE	hFile = OpenFile(lpszFileSpec, &OpenBuff, OF_READ);

    if ((int) hFile == HFILE_ERROR) return (FALSE);

    dwFileLen = _llseek(hFile, 0L, SEEK_END);
    _llseek(hFile, 0L, SEEK_SET);

    DebugPrintf("%s filesize is %6ld\r\n", lpszFileSpec, dwFileLen);

    dwVersion = GetVersion();
    if ((dwFileLen > sizeof(PIF)) && ((dwVersion & 0xFFFF) < 0x350L)) {

	DebugPrintf("File is too big to be a valid .PIF\r\n", dwFileLen);
	_lclose(hFile);  return (FALSE);

    } else {

	wFileLen = (WORD) dwFileLen;
	if (wFileLen <= sizeof(PIFHDR)) {	// Old-style .PIF
	    DebugPrintf("This is an old Windows PIF");

	    _lclose(hFile);  return (FALSE);
	}
    }

    pPIF = &pif;

    rc = _lread(hFile, (LPBYTE) pPIF, wFileLen);

    if (rc != (int) wFileLen) {
		DebugPrintf("OpenPIF() _lread(%d, %.4X:%.4X, %d) failed\r\n",
		    hFile, SELECTOROF((LPPIF) pPIF), OFFSETOF((LPPIF) pPIF), wFileLen);
		_lclose(hFile);  return (FALSE);
    }

    _lclose(hFile);

    return (TRUE);
}


/****************************************************************************
 *
 *  FUNCTION :	GetOpenFileNameHookProc(HWND, UINT, WPARAM, LPARAM)
 *
 *  PURPOSE  :	COMMDLG message hook procedure
 *		Use to get control when the user clicks 'OK' to check the
 *		validity of the .PIF
 *
 *  ENTRY    :	HWND	hWnd;		// Window handle
 *		UINT	msg;		// WM_xxx message
 *		WPARAM	wParam; 	// Message 16-bit parameter
 *		LPARAM	lParam; 	// Message 32-bit parameter
 *
 *  RETURNS  :	FALSE	- Allow default COMMDLG message processing
 *		TRUE	- Bypass default message processing
 *
 ****************************************************************************/

UINT CALLBACK _export GetOpenFileNameHookProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static LPOPENFILENAME lpofn;
    static UINT 	msgFileOK;

    if (msg == WM_INITDIALOG) {
	lpofn = (LPOPENFILENAME) lParam;
	msgFileOK = (UINT) LOWORD((DWORD) lpofn->lCustData);
    } else if (msg == msgFileOK) {
	if (IsPifFile(lpofn->lpstrFile)) {
	    return (FALSE);	// Yes -- exit dialog
	} else {
	    char	szBuf[128], szFmt[128];

	    LoadString(Globals.hInst, IDS_NOTVALIDPIF , szFmt, sizeof(szFmt));
	    wsprintf(szBuf, szFmt, (LPSTR) lpofn->lpstrFile);
	    MessageBeep(MB_ICONEXCLAMATION);
	    MessageBox(hWnd, szBuf, Globals.szAppTitle, MB_ICONEXCLAMATION | MB_OK);
	    return (TRUE);
	}
    }

    return (FALSE);
}


/****************************************************************************
 *
 *	FUNCTION :	OpenPif(VOID)
 *
 *	PURPOSE  :	Helper function for MenuFileOpen()
 *		Does the dirty work of the COMMDLG GetOpenFileName()
 *		Reads the .PIF file in local memory
 *
 *	ENTRY	:	VOID
 *
 *	RETURNS	:	0		- Normal return
 *				IDABORT	- File system error
 *
 ****************************************************************************/

int OpenPIF(VOID)
{
    int	rc;
    OFSTRUCT	OpenBuff;

    UINT	msgFileOK = RegisterWindowMessage(FILEOKSTRING);

    WORD	wPifLen;

    DWORD	dwPIFLen;

    HCURSOR	hCursorOld = 0;
    HFILE	hFilePIF = 0;

    PPIF	pPIF = NULL;

    Globals.szFile[0] = '\0';

    Globals.ofn.hwndOwner	= Globals.hWndDlg;

    Globals.ofn.lpstrFile	= Globals.szFile;
    Globals.ofn.nMaxFile	= sizeof(Globals.szFile);
    Globals.ofn.lpstrFileTitle	= Globals.szFileTitle;
    Globals.ofn.nMaxFileTitle	= sizeof(Globals.szFileTitle);
    Globals.ofn.lpstrInitialDir = Globals.szDirName;

    Globals.ofn.lpstrDefExt	= "PIF";

    Globals.ofn.Flags		= OFN_PATHMUSTEXIST	|
			  OFN_FILEMUSTEXIST	|
			  OFN_HIDEREADONLY	|
			  OFN_ENABLEHOOK;

    Globals.ofn.lpfnHook	= (COMMDLGHOOKPROC) MakeProcInstance((FARPROC) GetOpenFileNameHookProc, Globals.hInst);
    Globals.ofn.lCustData	= MAKELPARAM(msgFileOK, 0);

    rc = GetOpenFileName(&Globals.ofn);

    FreeProcInstance((FARPROC) Globals.ofn.lpfnHook);

	if (!rc) {
		if (CommDlgExtendedError()) {
			CommDlgError(Globals.hWndDlg, CommDlgExtendedError());
			rc = IDABORT;
		} else {
			rc = IDCANCEL;
		}

		goto OPENPIF_EXIT;
	}

    lstrcpy(Globals.szWindowTitle, Globals.szAppTitle);
    lstrcat(Globals.szWindowTitle, " - ");
    lstrcat(Globals.szWindowTitle, Globals.szFileTitle);
    SetWindowText(Globals.hWndDlg, Globals.szWindowTitle);

    Globals.fUntitled = FALSE;

    strncpy(Globals.szDirName, Globals.szFile, Globals.ofn.nFileOffset-1);
    lstrcpy(Globals.szExtension, Globals.szFile + Globals.ofn.nFileExtension);

/* Switch to the hour glass cursor */
    SetCapture(Globals.hWndDlg);
    hCursorOld = SetCursor(LoadCursor(0, IDC_WAIT));

/* Open and read the file */
    DebugPrintf("OpenPIF(): _lopen(%s, OF_READ)\r\n", (LPSTR) Globals.szFile);
	
    hFilePIF = _lopen(Globals.szFile, OF_READ);

	if ((int) hFilePIF == HFILE_ERROR) {	// Can't open the .PIF
		char	szFmt[80];

		DebugPrintf("OpenPIF() _lopen(%s, OF_READ) failed\r\n", (LPSTR) Globals.szFile);

		LoadString(Globals.hInst, IDS_CANTOPEN , szFmt, sizeof(szFmt));
		MessageBoxPrintf(szFmt, (LPSTR) Globals.szFile);
		rc = IDABORT;  goto OPENPIF_EXIT;
	}

    dwPIFLen = _llseek(hFilePIF, 0L, SEEK_END);
    _llseek(hFilePIF, 0L, SEEK_SET);

    DebugPrintf("%s filesize is %6ld\r\n", (LPSTR) Globals.szFile, dwPIFLen);

	if (dwPIFLen >= 65520L) {
		MessageBoxString(Globals.hWndDlg, Globals.hInst, IDS_TOOBIG, MB_OK);
		rc = IDABORT;  goto OPENPIF_EXIT;
	} else {
		wPifLen = (WORD) dwPIFLen;
		// @todo And??? Why not load old PIFs???
		if (wPifLen <= sizeof(PIFHDR)) {	// Old-style .PIF
			MessageBoxString(Globals.hWndDlg, Globals.hInst, IDS_OLDPIF, MB_OK);
			rc = IDABORT;  goto OPENPIF_EXIT;
		}
	}

	Globals.hPIF = LocalAlloc(LMEM_MOVEABLE, wPifLen);
	if (!Globals.hPIF) {
		MessageBoxString(Globals.hWndDlg, Globals.hInst, IDS_OUTOFMEMORY, MB_OK);
		rc = IDABORT;  goto OPENPIF_EXIT;
	}

    pPIF = (PPIF)LocalLock(Globals.hPIF);
    if (!pPIF) {
		MessageBoxString(Globals.hWndDlg, Globals.hInst, IDS_OUTOFMEMORY, MB_OK);
		rc = IDABORT;  goto OPENPIF_EXIT;
    }

    rc = _lread(hFilePIF, (LPBYTE) pPIF, wPifLen);

    if (rc != (int) wPifLen) {
	MessageBoxPrintf( CANTREAD_STR );
	DebugPrintf("OpenPIF() _lread(%d, %.4X:%.4X, %d) failed\r\n",
		    hFilePIF,
		    SELECTOROF((LPPIF) pPIF),
		    OFFSETOF((LPPIF) pPIF),
		    wPifLen
		    );
	rc = IDABORT;  goto OPENPIF_EXIT;
    }

    if (pPIF) { LocalUnlock(Globals.hPIF); pPIF = NULL; }

OPENPIF_EXIT:
	if (hCursorOld) {
		SetCursor(hCursorOld);
		ReleaseCapture();
	}

	if (pPIF) { LocalUnlock(Globals.hPIF); pPIF = NULL; }

	if (hFilePIF > 0) _lclose(hFilePIF);

	return (rc);
}




/****************************************************************************
 *
 *  FUNCTION :	CheckPIFValidity(BOOL fMustBeValid)
 *
 *  PURPOSE  :	Checks .PIF model for consistent data
 *		Presents a message box to the user if the data is invalid
 *
 *  ENTRY    :	BOOL	fMustBeValid;	// Don't return 0 unless PIF is OK
 *
 *  RETURNS  :	0	- Normal return
 *		IDCANCEL - Data was invalid, but user said go ahead
 *
 *		Assumes ControlToPIF() has already been called
 *
 ****************************************************************************/

int CheckPIFValidity(BOOL fMustBeValid)
{
    int 	n;
    int 	rc;

    char	szPathname[_MAX_PATH];	// Buffer to hold full filename
    char	szDrive[_MAX_DRIVE];	// Drive letter and colon
    char	szDir[_MAX_DIR];	// Directory
    char	szFname[_MAX_FNAME];	// Filename w/o extension
    char	szExt[_MAX_EXT];	// Filename extension

    char	szBuf[128];

// Checkout the program filename
    if (!*SkipWhite(Globals.pifModel.pifHdr.PgmName)) {
	LoadString(Globals.hInst, IDS_FILENAMEREQ , szBuf, sizeof(szBuf));

MSGBOXCOM:
	rc = MessageBox(0, szBuf, Globals.szAppTitle, MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONEXCLAMATION);
CHECK_VALID_COM:
	if (fMustBeValid)
		return (IDCANCEL);
	if (rc == IDCANCEL)
		return (rc);
    }

    _fullpath(szPathname,Globals.pifModel.pifHdr.PgmName, _MAX_PATH);
    _splitpath(szPathname, szDrive, szDir, szFname, szExt);

    n = lstrlen(szFname);
    if (n == 0 || n > 8 || (lstrcmpi(szExt, ".BAT") && lstrcmpi(szExt, ".COM") && lstrcmpi(szExt, ".EXE") )) {
		LoadString(Globals.hInst, IDS_BADFILENAME , szBuf, sizeof(szBuf));

		goto MSGBOXCOM;
    }

    if (!ValidateHotKey(FALSE)) goto CHECK_VALID_COM;

    return (0);
}


/****************************************************************************
 *
 *  FUNCTION :	SavePIF(VOID)
 *
 *  PURPOSE  :	Helper function for SaveAsPIF() and MenuFileSave()
 *
 *  ENTRY    :	VOID
 *
 *  RETURNS  :	Return code from lower-level functions
 *
 ****************************************************************************/

int SavePIF(VOID)
{
    register int	rc;

    ControlsToPIF(Globals.hWndDlg);

    rc = CheckPIFValidity(FALSE);
    if (rc) return (rc);

    return (WritePIF(Globals.hWndDlg, Globals.szFile));
}


/****************************************************************************
 *
 *  FUNCTION :	SaveAsPIF(VOID)
 *
 *  PURPOSE  :	Helper function for MenuFileSave() and MenuFileSaveAs()
 *		Does the dirty work of GetSaveFileName()
 *		Write out the .PIF
 *
 *  ENTRY    :	VOID
 *
 *  RETURNS  :	Return code from lower-level functions
 *
 ****************************************************************************/

int SaveAsPIF(VOID)
{
    Globals.szFile[0] = '\0';

    Globals.ofn.hwndOwner	= Globals.hWndDlg;

    Globals.ofn.lpstrFile	= Globals.szFile;
    Globals.ofn.nMaxFile	= sizeof(Globals.szFile);
    Globals.ofn.lpstrFileTitle	= Globals.szFileTitle;
    Globals.ofn.nMaxFileTitle	= sizeof(Globals.szFileTitle);
    Globals.ofn.lpstrInitialDir = Globals.szDirName;

    Globals.ofn.lpstrDefExt	= "PIF";

	Globals.ofn.Flags		= OFN_PATHMUSTEXIST	|
			  OFN_HIDEREADONLY	|
			  OFN_OVERWRITEPROMPT;

	if (!GetSaveFileName(&Globals.ofn)) {
		if (CommDlgExtendedError() == 0) return (IDCANCEL);

		CommDlgError(Globals.hWndDlg, CommDlgExtendedError());
		return (1);
	}

    lstrcpy(Globals.szWindowTitle, Globals.szAppTitle);
    lstrcat(Globals.szWindowTitle, " - ");
    lstrcat(Globals.szWindowTitle, Globals.szFileTitle);
    SetWindowText(Globals.hWndDlg, Globals.szWindowTitle);

    strncpy(Globals.szDirName, Globals.szFile, Globals.ofn.nFileOffset-1);
    lstrcpy(Globals.szExtension, Globals.szFile + Globals.ofn.nFileExtension);

    Globals.fUntitled = FALSE;

    return (SavePIF());
}


/****************************************************************************
 *
 *  FUNCTION :	PaneMsgProc(HWND, UINT, WPARAM, LPARAM)
 *
 *  PURPOSE  :	Dialog proc for the frame and panes
 *
 *  ENTRY    :	HWND	hWndDlg;	// Dialog window handle
 *		UINT	msg;		// WM_xxx message
 *		WPARAM	wParam; 	// Message 16-bit parameter
 *		LPARAM	lParam; 	// Message 32-bit parameter
 *
 *  RETURNS  :	Non-zero - Message processed
 *		Zero	- DefDlgProc() must process the message
 *
 ****************************************************************************/

BOOL CALLBACK _export PaneMsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	char	szBuf[128];
	WINDOWPLACEMENT wndpl;

	switch (msg) {
	case WM_ACTIVATEAPP:
	    Globals.fCheckOnKillFocus = (BOOL) wParam;
	    break;

	case WM_ACTIVATE:
	    Globals.fCheckOnKillFocus = (wParam != WA_INACTIVE);
	    break;

	case WM_CLOSE:
	    {
		if (IDCANCEL == CheckSave(hWnd)) break;

		Globals.fCheckOnKillFocus = FALSE;	// Stop checking controls

		if (Globals.nActiveDlg) {	// A dialog is still open
		    SetFocus(hWnd);

		    Globals.nActiveDlg = 0;
		}

		EndDialog(hWnd, IDCANCEL);
		Globals.nActiveDlg = 0;
	    }
	    break;

	case WM_COMMAND:
	    Pane_OnCommand(hWnd, (UINT) wParam, (HWND) LOWORD(lParam), (WORD) HIWORD(lParam));
	    return (FALSE);

	case WM_CTLCOLOR:
	    {
		HDC	hDC = (HDC) wParam;
		HWND	hWndChild = (HWND) LOWORD(lParam);

		if (GetDlgItem(hWnd, IDD_HELP) != hWndChild) return FALSE;

		switch ((UINT) HIWORD(lParam)) {
		    case CTLCOLOR_EDIT:
			SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));
			return ((BOOL) (WORD) Globals.hbrHelp);

		    case CTLCOLOR_MSGBOX:
			return ((BOOL) (WORD) Globals.hbrHelp);
		}

		return FALSE;
	    }

	case WM_DESTROY:
	    {

		if (Globals.hbrHelp) { DeleteBrush(Globals.hbrHelp);  Globals.hbrHelp = 0; }

		return (FALSE);
	    }

	case WM_SIZE:
	    if (wParam == SIZE_MINIMIZED) {
			//SetWindowText(hWnd, APPNAME);
	    } else if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED) {
			//SetWindowText(hWnd, szWindowTitle);
	    }

	    return (FALSE);		// Requires default processing

	case WM_ERASEBKGND:
	    if (IsIconic(hWnd)) {
		return (TRUE);		// Pretend we erased it
	    } else {
		return (FALSE); 	// We didn't erase anything
	    }

	case WM_INITDIALOG:
	    return (Pane_OnInitDialog(hWnd, (HWND) wParam, lParam));

	case WM_PAINT:
		if (IsIconic(hWnd)) {
			Pane_OnPaint_Iconic(hWnd);
		} else {
			Pane_OnPaint(hWnd);
		}
	    return (FALSE);

	case WM_SETFONT:
	    Pane_OnSetFont(hWnd, (HFONT) wParam, (BOOL) LOWORD(lParam));
	    return (FALSE);
	}

	return (FALSE);
}



/****************************************************************************
 *
 *  FUNCTION :	Pane_OnCommand(HWND, UINT, HWND, WORD)
 *
 *  PURPOSE  :	Process the WM_COMMAND messages sent to PaneMsgProc()
 *		Processes all the menu selections and button clicks
 *
 *  ENTRY    :	HWND	hWnd;		// Parent window handle
 *		UINT	id;		// Control ID
 *		HWND	hWndCtl;	// Contro window handle
 *		WORD	codeNotify;	// 1- if accelerator, 0 - menu
 *
 *  RETURNS  :	VOID
 *
 ****************************************************************************/

VOID Pane_OnCommand(HWND hWnd, UINT id, HWND hWndCtl, WORD codeNotify)
{
	switch (id) {

	case IDOK:
	case IDCANCEL:
	    MessageBeep(0xFFFF);
	    break;

	case IDM_F_NEW:
	    MenuFileNew(TRUE);
	    break;

	case IDM_F_OPEN:
	    MenuFileOpen();
	    break;

	case IDM_F_SAVE:
	    MenuFileSave();
	    break;

	case IDM_F_SAVEAS:
	    MenuFileSaveAs();
	    break;

	case IDM_F_EXIT:		// File.eXit
	    SendMessage(hWnd, WM_CLOSE, 0, 0L);
	    break;

	case IDM_H_OVERVIEW:
	    //WinHelp( hWnd, szHelpName, HELP_KEY, (DWORD)((char FAR *)HK_OVERVIEW ));
	    break;

	case IDM_H_SEARCH:
	    //WinHelp( hWnd, szHelpName, HELP_PARTIALKEY, (DWORD)((char FAR *)"") );
	    break;

	case IDM_H_TECHSUP:
	    //WinHelp( hWnd, szHelpName, HELP_KEY, (DWORD)((char FAR *)HK_TECHS) );
	    break;

	/* show "about" box */
	case IDM_H_ABOUT:
		ShellAbout(hWnd, Globals.szAppTitle, "Copyright � 1992, 1993 Qualitas, Inc.\n\rCopyright � 2023 osFree. GPLv3.", 0);
		break;

	case IDB_ADVANCED:
	{
		FARPROC lpfn;
		int rc;	
		
		lpfn = MakeProcInstance((FARPROC) AdvancedMsgProc, Globals.hInst);
		rc = DialogBox(Globals.hInst, MAKEINTRESOURCE(IDD_ADVFRAME), 0, (DLGPROC) lpfn);
		FreeProcInstance(lpfn);
		break;
	}

	//case IDB_ENHANCED:
	case IDB_STANDARD:
	{
		FARPROC	lpfnSMMsgProc;

		lpfnSMMsgProc = MakeProcInstance((FARPROC) StandardMsgProc, Globals.hInst);
		DialogBox(Globals.hInst, MAKEINTRESOURCE(IDD_STANDARD), 0, (DLGPROC) lpfnSMMsgProc);
		FreeProcInstance(lpfnSMMsgProc);
		break;
	}

//	default:
// RCC - added kludge 10/15
//		SetHelpText(Globals.hWndDlg, id);
	}
}

/****************************************************************************
 *
 *  FUNCTION :	Pane_OnInitDialog(HWND, HWND, LPARAM)
 *
 *  PURPOSE  :	Process the WM_INITDIALOG messages sent to PaneMsgProc()
 *		Move the window to the position specified in QIFEDIT.INI
 *		Create the dialog panes for GENERAL, MEMORY, etc.
 *		Relocate the help window "EDIT" control
 *		Subclass the individual controls
 *
 *  ENTRY    :	HWND	hWnd;		// Parent window handle
 *		HWND	hWndFocus;	// Control window handle getting focus
 *		LPARAM	lParam; 	// Extra data passed to dialog creator
 *
 *  RETURNS  :	FALSE	- Focus was changed
 *		TRUE	- Focus was not changed
 *
 ****************************************************************************/

BOOL Pane_OnInitDialog(HWND hWnd, HWND hWndFocus, LPARAM lParam)
{
    LOGFONT	lf;
    HFONT	hFontSys;
    HDWP	hdwp;

    char	szBuf[128], szBuf2[128];
    int 	n;
    RECT	rect;

    int 	cxScreen = GetSystemMetrics(SM_CXSCREEN);
    int 	cyScreen = GetSystemMetrics(SM_CYSCREEN);

    int 	cxFrame = GetSystemMetrics(SM_CXFRAME);
    int 	cyCaption = GetSystemMetrics(SM_CYCAPTION);

	WNDPROC	lpfnNumEdit = (WNDPROC) MakeProcInstance((FARPROC) NumEdit_SubClassProc, Globals.hInst);

    WNDPROC	lpfnKeyEdit = (WNDPROC) MakeProcInstance((FARPROC) KeyEdit_SubClassProc, Globals.hInst);

    WNDPROC	lpfnButtonEdit = (WNDPROC) MakeProcInstance((FARPROC) Button_SubClassProc, Globals.hInst);

    Globals.hWndDlg = hWnd;

// Build the font for the help text
    memset(&lf, 0, sizeof(LOGFONT));

    hFontSys = GetStockObject(SYSTEM_FONT);

    GetObject(hFontSys, sizeof(LOGFONT), &lf);

    lf.lfHeight = -10;

    Globals.hFontHelp = CreateFontIndirect(&lf);

// Prepare the panes
    GetClientRect(hWnd, &Globals.rectDlg);

    Globals.cxDlg = (Globals.rectDlg.right  - Globals.rectDlg.left);
    Globals.cyDlg = (Globals.rectDlg.bottom - Globals.rectDlg.top );

    ShowWindow(hWnd, SW_SHOWNORMAL);

    Globals.nActiveDlg = IDD_GENERAL;

    Globals.hbrHelp = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    InitHelpWindow(hWnd);
    ShowWindow(GetDlgItem(hWnd, IDD_HELP), SW_SHOWNORMAL);

// Subclass the "EDIT" controls
// Subclass the hotkey "EDIT" control

    for (n = 0; n < sizeof(aControls)/sizeof(CONTROL); n++) {

	if (aControls[n].Type == EDIT) {
	    aControls[n].lpfnOld = (WNDPROC) SetWindowLong(
					    GetDlgItem(hWnd, aControls[n].ID),
					    GWL_WNDPROC,
					    (LONG) lpfnNumEdit
					    );
	} else if (aControls[n].Type == KEYEDIT) {
	    aControls[n].lpfnOld = (WNDPROC) SetWindowLong(
					    GetDlgItem(hWnd, aControls[n].ID),
					    GWL_WNDPROC,
					    (LONG) lpfnKeyEdit
					    );

	} else {
	    aControls[n].lpfnOld = (WNDPROC) SetWindowLong(
					    GetDlgItem(hWnd, aControls[n].ID),
					    GWL_WNDPROC,
					    (LONG) lpfnButtonEdit
					    );
	}
    }

    MenuFileNew(FALSE);

    SetFocus(GetDlgItem(hWnd, IDE_FILENAME));

    if (GetWinFlags() & WF_STANDARD) {	// Standard mode
		StandardModeBitchBox(hWnd);
    }

    return (FALSE);		// Indicate the focus has been changed
}


/****************************************************************************
 *
 *  FUNCTION :	Pane_OnPaint(HWND)
 *
 *  PURPOSE  :	Process the WM_PAINT messages sent to PaneMsgProc()
 *		Draw the 3-D border around the help window "EDIT" control
 *
 *  ENTRY    :	HWND	hWnd;		// Parent window handle
 *
 *  RETURNS  :	VOID
 *
 ****************************************************************************/

VOID Pane_OnPaint(HWND hWnd)
{
    register int	i;

    RECT	rect;
    RECT	rectWnd;

    HDC 	hDC;
    PAINTSTRUCT ps;

// Create the pens for the lines

    HPEN	hPenOld;
    HPEN	hPenBlack = GetStockObject(BLACK_PEN);
    HPEN	hPenWhite = GetStockObject(WHITE_PEN);
    HPEN	hPenBtnShadow = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));

//    HBRUSH	hbrButton = CreateSolidBrush(BUTTON_BKGNDCOLOR);

    memset(&ps, 0, sizeof(PAINTSTRUCT));

    hDC = BeginPaint(hWnd, &ps);

// Get size of normal buttons for the background
    GetClientRect(hWnd, &rectWnd);


// Paint the help border

// Prepare rect for help window including border and sunken frame
    SetRect(&rect, 0, 0, 0, 160);
    MapDialogRect(hWnd, &rect);
    SetRect(&rect, 0, rect.bottom, Globals.cxDlg, Globals.cyDlg);

// Draw the black separator line
    hPenOld = SelectObject(hDC, hPenBlack);

    MoveTo(hDC, rect.left, rect.top);
    LineTo(hDC, rect.right+1, rect.top);

    SelectObject(hDC, hPenOld);

    rect.top++; 	// Account for the black separator

// Fill in the whole thing COLOR_BTNFACE
    FillRect(hDC, &rect, Globals.hbrHelp);

#define EDITWASTE	1

// Prepare rect for help border
    InflateRect(&rect, -EDITWASTE, -EDITWASTE);

// Paint the upper left highlight on the border
    hPenOld = SelectObject(hDC, hPenBtnShadow);

    for (i = 0; i < EDITBORDER - EDITWASTE; i++) {
	MoveTo(hDC, rect.right-1, rect.top+i);
	LineTo(hDC, rect.left+i, rect.top+i);
	LineTo(hDC, rect.left+i, rect.bottom);
    }

// Paint the bottom right shadow on the border
	SelectObject(hDC, hPenWhite);

	for (i = 0; i < EDITBORDER - EDITWASTE; i++) {
		MoveTo(hDC, rect.left+i, rect.bottom-i-1);
		LineTo(hDC, rect.right-i-1, rect.bottom-i-1);
		LineTo(hDC, rect.right-i-1, rect.top+i);
    }

    SelectObject(hDC, hPenOld);

    DeleteObject(hPenBtnShadow);

    EndPaint(hWnd, &ps);
}


/****************************************************************************
 *
 *  FUNCTION :	Pane_OnPaint_Iconic(HWND)
 *
 *  PURPOSE  :	Process the WM_PAINT messages sent to PaneMsgProc()
 *		when the window is minimized.
 *
 *  ENTRY    :	HWND	hWnd;		// Parent window handle
 *
 *  RETURNS  :	VOID
 *
 ****************************************************************************/


VOID Pane_OnPaint_Iconic(HWND hWnd)
{
    HDC 	hDC;
    PAINTSTRUCT ps;
    int 	nMapModeOld;

    memset(&ps, 0, sizeof(PAINTSTRUCT));

    hDC = BeginPaint(hWnd, &ps);

    nMapModeOld = GetMapMode(hDC);
    if (nMapModeOld != MM_TEXT) SetMapMode(hDC, MM_TEXT);

    DrawIcon(hDC, 0, 0, Globals.hIcon);

    if (nMapModeOld != MM_TEXT) SetMapMode(hDC, nMapModeOld);

    EndPaint(hWnd, &ps);
}


/****************************************************************************
 *
 *  FUNCTION :	Pane_OnSetFont(HWND, HFONT, BOOL)
 *
 *  PURPOSE  :	Process the WM_SETFONT messages sent to PaneMsgProc()
 *		Save the font for later use in creating the panes
 *
 *  ENTRY    :	HWND	hWnd;		// Parent window handle
 *		HFONT	hFont;		// Handle the dialog's font
 *		BOOL	fRedraw;	// Redraw entire dialog/control if TRUE
 *
 *  RETURNS  :	NULL
 *
 ****************************************************************************/

VOID Pane_OnSetFont(HWND hWndCtl, HFONT hFont, BOOL fRedraw)
{
    Globals.hFontDlg = hFont;
}



#define HELPSLOP	4

/****************************************************************************
 *
 *  FUNCTION :	InitHelpWindow(HWND)
 *
 *  PURPOSE  :	Move and resize the help window "EDIT" control
 *
 *  ENTRY    :	HWND	hWnd;		// Parent window handle
 *
 *  RETURNS  :	VOID
 *
 ****************************************************************************/

VOID InitHelpWindow(HWND hWnd)
{
    RECT	rect;

    HWND	hWndHelp = GetDlgItem(hWnd, IDD_HELP);

// Prepare rect for help window including border and sunken frame
    SetRect(&rect, 0, 0, 0, 160);
    MapDialogRect(hWnd, &rect);
    SetRect(&rect, 0, rect.bottom, Globals.cxDlg, Globals.cyDlg);

    MoveWindow(
		hWndHelp,
		EDITBORDER + HELPSLOP,			// X origin
		rect.top + 1 + EDITBORDER,		// Y origin
		Globals.cxDlg - EDITBORDER * 2 - HELPSLOP,	// X size
		(Globals.cyDlg - rect.top) - (EDITBORDER * 2) - 1, // Y size
		TRUE
		);

    SendMessage(hWndHelp, WM_SETFONT, (WPARAM) Globals.hFontHelp, 0L);

    SetHelpText(hWnd, Globals.nActiveDlg);
}


/****************************************************************************
 *
 *  FUNCTION :	SetHelpText(HWND, UINT)
 *
 *  PURPOSE  :	Get the help text for a control and send it to the help window
 *		Lock and load the help text
 *		Extract and free the current help window "EDIT" control handle
 *		Pass the local memory handle to the help window "EDIT" control
 *
 *  ENTRY    :	HWND	hWnd;		// Parent window handle
 *		UINT	idHelp; 	// Resource ID for the control
 *
 *  RETURNS  :	VOID
 *
 *		In order for the EM_GETHANDLE and EM_SETHANDLE to work,
 *		the dialog must be created with DS_LOCALEDIT style.
 *
 ****************************************************************************/

VOID SetHelpText(HWND hWnd, UINT idHelp)
{
	HRSRC	hRsrc;
	HGLOBAL	hGblRsrc;
	HLOCAL	hOld, hNew;

	PSTR	psz;
	LPSTR	lpsz;

	HWND	hWndHelp = GetDlgItem(hWnd, IDD_HELP);

	if (!idHelp)
		return;

	if (idHelp == Globals.idHelpCur)
		return;


	hRsrc = FindResource(Globals.hInst, MAKEINTRESOURCE(idHelp), RT_RCDATA);


	if (hRsrc) {
		hGblRsrc = LoadResource(Globals.hInst, hRsrc);

		if (hGblRsrc) {
			lpsz = (LPSTR) LockResource(hGblRsrc);

			if (lpsz) {
				hNew = LocalAlloc(LMEM_MOVEABLE, lstrlen(lpsz) + 1);
				if (hNew) {
					psz = LocalLock(hNew);


					if (psz) {
						lstrcpy(psz, lpsz);
						LocalUnlock(hNew);

						hOld = (HLOCAL) SendMessage(hWndHelp,
								EM_GETHANDLE,
								(WPARAM) 0,
								(LPARAM) 0L
								);
						if (hOld) LocalFree(hOld);

						Edit_SetHandle(hWndHelp, hNew);
						Edit_SetReadOnly(hWndHelp, TRUE);
					}
				}
				UnlockResource(hGblRsrc);
			}
			FreeResource(hGblRsrc);
		}
	}

    Globals.idHelpCur = idHelp;
}


/****************************************************************************
 *
 *  FUNCTION :	NumEdit_SubClassProc(HWND, UINT, WPARAM, LPARAM)
 *
 *  PURPOSE  :	Window procedure for the subclassed "EDIT" controls
 *		Disallow invalid inputs for numeric fields
 *		Check numeric values for proper range with loosing focus
 *		Kill the selection when a text control gets the focus
 *
 *  ENTRY    :	HWND	hWnd;		// Window handle
 *		UINT	msg;		// WM_xxx message
 *		WPARAM	wParam; 	// Message 16-bit parameter
 *		LPARAM	lParam; 	// Message 32-bit parameter
 *
 *  RETURNS  :	FALSE	- Message has been processed
 *		TRUE	- DefWindowProc() processing required
 *
 *		Most messages are past to the original window proc
 *
 ****************************************************************************/

LRESULT CALLBACK _export NumEdit_SubClassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    char	szBuf[80], szBuf2[80];
    PSTR	p, p2;
    DWORD	dw;
    int 	nNum;
    int 	n, m;
    int 	nNewVal;

    int 	id = GetDlgCtrlID(hWnd);

// Find the correct slot in the CONTROLS array
// FIXME if this fails to find the control ID, we're completely fucked.
    for (n = 0; (n < sizeof(aControls)/sizeof(CONTROL)) && aControls[n].ID != id; n++);

    switch (msg) {

	case WM_KEYDOWN:
	    {
		HWND	hWndHelp = GetDlgItem(Globals.hWndDlg, IDD_HELP);

		if (hWndHelp == GetFocus()) break;

		if (wParam == VK_NEXT && !(HIWORD(lParam) & KF_ALTDOWN)) {

		    SendMessage(hWndHelp, WM_VSCROLL, SB_PAGEDOWN, 0L);
		    return (0);
		} else if (wParam == VK_PRIOR && !(HIWORD(lParam) & KF_ALTDOWN)) {

		    SendMessage(hWndHelp, WM_VSCROLL, SB_PAGEUP, 0L);
		    return (0);
		} else {
		    break;
		}
	    }

	case WM_KILLFOCUS:
// If the field is empty, fill in the default
// Otherwise, check for valid contents

	    if (!(aControls[n].Flags & EDIT_NUM)) break;

	    if (!Globals.fCheckOnKillFocus) break;

	    Edit_GetText(hWnd, szBuf, sizeof(szBuf));

	    p = SkipWhite(szBuf);  p2 = p;
	    if (!*p) {			// Empty -- fill in the default
		wsprintf(szBuf,"%d",aControls[n].nDefault);
		Edit_SetText(hWnd, szBuf);
		break;
	    }

	// Count the minus signs
	    for (m = 0; *p; p++) if (*p == '-') m++;

	    if ( (m > 1) || (m == 1 && (*p2 != '-' || !isdigit(*(p2+1)))) ) {
		char	szFmt[80];
		LRESULT lr;
		UINT	idFmt;

		nNewVal = aControls[n].nMin;
QQQ:
		idFmt = (aControls[n].Flags & (EDIT_OPTVAL-EDIT_NUM)) ? IDS_BADNUM2 : IDS_BADNUM;
		LoadString(Globals.hInst, id, szBuf2, sizeof(szBuf2));
		LoadString(Globals.hInst, idFmt, szFmt, sizeof(szFmt));

		wsprintf(
			szBuf,
			szFmt,
			(LPSTR) szBuf2,
			aControls[n].nMin,
			aControls[n].nMax
			);

		MessageBox(0, szBuf, Globals.szAppTitle, MB_OK);

		lr = CallWindowProc((FARPROC)aControls[n].lpfnOld, hWnd, msg, wParam, lParam);
		SetFocus(hWnd);

		wsprintf(szBuf,"%d",nNewVal);
		Edit_SetText(hWnd, szBuf);
		Edit_SetSel(hWnd, 0, -1);
		return (0);
	    }

	    sscanf(szBuf, "%d", &nNum);

	    if (aControls[n].Flags & (EDIT_OPTVAL - EDIT_NUM) && nNum == -1) {
		break;
	    } else if (nNum < aControls[n].nMin) {
		nNewVal = aControls[n].nMin;
		goto QQQ;
	    } else if (nNum > aControls[n].nMax) {
		nNewVal = aControls[n].nMax;
		goto QQQ;
	    }
	    break;

	case WM_SETFOCUS:
	    SetHelpText(Globals.hWndDlg, id);

	    if (aControls[n].Flags & EDIT_NOSEL) {
			Edit_SetSel(hWnd, 0, 0);
	    }

	    break;

	case WM_CHAR:

// If we get a character that's not a digit, backspace, or delete,
// beep and ignore it.	Otherwise, let the "EDIT" window process it.

	    if (!(aControls[n].Flags & EDIT_NUM)) break;

	    if (!isdigit(wParam) && wParam != '-' && wParam != '\b') {
		MessageBeep(MB_ICONEXCLAMATION);
		return (0);
	    }

	    dw = Edit_GetSel(hWnd);
	    if (LOWORD(dw) != HIWORD(dw)) break;	// Some text selected

	    if (isdigit(wParam) || wParam == '-') {     // Another digit being entered
	// Check for too many digits
		GetWindowText(hWnd, szBuf, sizeof(szBuf));
		wsprintf(szBuf2,"%d",aControls[n].nMax);

		if (lstrlen(szBuf)+1 > lstrlen(szBuf2)) {
		    MessageBeep(MB_ICONEXCLAMATION);
		    return (0);
		}
	    }

	    return (CallWindowProc((FARPROC)aControls[n].lpfnOld, hWnd, msg, wParam, lParam));
    }

    return (CallWindowProc((FARPROC)aControls[n].lpfnOld, hWnd, msg, wParam, lParam));
}


/****************************************************************************
 *
 *  FUNCTION :	KeyEdit_SubClassProc(HWND, UINT, WPARAM, LPARAM)
 *
 *  PURPOSE  :	Window procedure for the special hotkey "EDIT" control
 *		Fills in the name of a keystroke when a key is pressed
 *
 *  ENTRY    :	HWND	hWnd;		// Window handle
 *		UINT	msg;		// WM_xxx message
 *		WPARAM	wParam; 	// Message 16-bit parameter
 *		LPARAM	lParam; 	// Message 32-bit parameter
 *
 *  RETURNS  :	FALSE	- Message has been processed
 *		TRUE	- DefWindowProc() processing required
 *
 *		Most messages are past to the original window proc
 *
 ****************************************************************************/

LRESULT CALLBACK _export KeyEdit_SubClassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    register int	n;

    int 	id = GetDlgCtrlID(hWnd);

// Find the correct slot in the CONTROLS array
// FIXME if this fails to find the control ID, we're completely fucked.
    for (n = 0; (n < sizeof(aControls)/sizeof(CONTROL)) && aControls[n].ID != id; n++);

    switch (msg) {

	case WM_KILLFOCUS:
	    {
		int	nCtrl = GetDlgCtrlID((HWND) wParam);	// Control getting the focus

		if (!Globals.fCheckOnKillFocus) break;

		if (nCtrl != IDB_ALT	&&
		    nCtrl != IDB_CTRL	&&
		    nCtrl != IDB_SHIFT	&&
		    nCtrl != IDE_KEY) {

		    if (!ValidateHotKey(TRUE)) {
			LRESULT lr;

			lr = CallWindowProc((FARPROC)aControls[n].lpfnOld, hWnd, msg, wParam, lParam);
			SetFocus(GetDlgItem(Globals.hWndDlg, IDB_ALT));
			CheckDlgButton(Globals.hWndDlg, IDB_ALT, IsDlgButtonChecked(Globals.hWndDlg, IDB_ALT));
			return (0);
		    }
		}
	    }
	    break;

	case WM_SETFOCUS:
			SetHelpText(Globals.hWndDlg, id);
	    break;

// If we get a character that's not a SHIFT, ALT, or CTRL, save the scancode.
// BACKSPACE and SHIFT+BACKSPACE install 'NONE'.

	case WM_CHAR:			// Ignore translated keystrokes
	    return (0);

	case WM_KEYDOWN:		// A key was pressed
	    {
		char	szBuf[32];
		UINT	vk = (UINT) wParam;

		if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU) {
		    return (CallWindowProc((FARPROC)aControls[n].lpfnOld, hWnd, msg, wParam, lParam) );
		} else if (vk == VK_BACK) {	// Erase or NONE
		    Globals.wHotkeyScancode = 0;
		    Globals.bHotkeyBits = 0;

		    SetWindowText(hWnd, "None");
		} else {
		    Globals.wHotkeyScancode = LOBYTE(HIWORD(lParam));
		    Globals.bHotkeyBits = HIBYTE(HIWORD(lParam));

		    lParam |=  0x02000000;	// Ignore left vs. right CTRL and SHIFT state

		    GetKeyNameText(lParam, szBuf, sizeof(szBuf));
		    SetWindowText(hWnd, szBuf);

		// Set the ALT, CTRL, and SHIFT checkboxes
		    CheckDlgButton(Globals.hWndDlg, IDB_ALT, ((GetKeyState(VK_MENU) & 0x8000) ? 1 : 0) );
		    CheckDlgButton(Globals.hWndDlg, IDB_CTRL, ((GetKeyState(VK_CONTROL) & 0x8000) ? 1 : 0) );
		    CheckDlgButton(Globals.hWndDlg, IDB_SHIFT, ((GetKeyState(VK_SHIFT) & 0x8000) ? 1 : 0) );
		}
	    }
	    return (0);
    }

    return (CallWindowProc((FARPROC)aControls[n].lpfnOld, hWnd, msg, wParam, lParam) );
}


// RCC - 10/16/95
LRESULT CALLBACK _export Button_SubClassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    register int n;
    int id = GetDlgCtrlID(hWnd);

    for (n = 0; (n < sizeof(aControls)/sizeof(CONTROL)) && aControls[n].ID != id; n++)
	;

    if (msg == WM_SETFOCUS)
		SetHelpText(Globals.hWndDlg, id);

    return (CallWindowProc((FARPROC)aControls[n].lpfnOld, hWnd, msg, wParam, lParam) );
}


/****************************************************************************
 *
 *  FUNCTION :	ControlsFromPIF(PPIF, HWND)
 *
 *  PURPOSE  :	Given a .PIF structure, extract the data and send
 *		it to the dialog controls
 *		Displays a message box if an error occurs
 *
 *  ENTRY    :	PPIF	pPIF;		// ==> .PIF structure
 *		HWND	hWnd;		// Parent window handle
 *
 *  RETURNS  :	0	- Normal return
 *		1	- Error
 *
 ****************************************************************************/

// Copy the data to the dialog controls from our .PIF model

int ControlsFromPIF(PPIF pPIF, HWND hWnd)
{
    char szNumBuf[32];
    int 	n;

    PPIFHDR	pPIFHDR = (PPIFHDR) pPIF;
    PPIFSIG	pPifSig = NULL;
    PPIF386	pPif386 = NULL;

// Verify the checksum
    if (pPIFHDR->CheckSum != 0 && pPIFHDR->CheckSum != ComputePIFChecksum(pPIF)) 
	{
		MessageBoxString(hWnd, Globals.hInst, IDS_BADCHECKSUM, MB_OK);
	}

// Find the 386 Enhanced mode section
    pPifSig = (PPIFSIG) (pPIFHDR + 1);

    while ( ((n = lstrcmp(pPifSig->Signature, "WINDOWS 386 3.0")) != 0) &&
	    pPifSig->NextOff != 0xFFFF &&
	    pPifSig->NextOff > 0 &&
	    (PBYTE) pPifSig < (PBYTE) (pPIF+1) ) {

		pPifSig = (PPIFSIG) ((PBYTE) pPIFHDR + pPifSig->NextOff);

    }

	if (n) {	// No 386 Enhanced section -- use the defaults
		MessageBoxString(hWnd, Globals.hInst, IDS_BADCHECKSUM, MB_OK);
		return (1);
	}

	pPif386 = (PPIF386) ((PBYTE) pPIFHDR + pPifSig->DataOff);

	// Fill in the controls

    SetDlgItemText(hWnd, IDE_FILENAME, pPIFHDR->PgmName);

    FixNul(pPIFHDR->Title, sizeof(pPIFHDR->Title));
    SetDlgItemText(hWnd, IDE_TITLE, pPIFHDR->Title);

    SetDlgItemText(hWnd, IDE_PARAMS, pPif386->CmdLine3);
    SetDlgItemText(hWnd, IDE_DIRECTORY, pPIFHDR->StartupDir);

    wsprintf(szNumBuf,"%d",pPif386->DosMin);
    SetDlgItemText(hWnd, IDE_DOSMIN, szNumBuf);

    wsprintf(szNumBuf,"%d",pPif386->DosMax);
    SetDlgItemText(hWnd, IDE_DOSMAX, szNumBuf);

    wsprintf(szNumBuf,"%d",pPif386->EmsMin);
    SetDlgItemText(hWnd, IDE_EMSMIN, szNumBuf);

    wsprintf(szNumBuf,"%d",pPif386->EmsMax);
    SetDlgItemText(hWnd, IDE_EMSMAX, szNumBuf);

    wsprintf(szNumBuf,"%d",pPif386->XmsMin);
    SetDlgItemText(hWnd, IDE_XMSMIN, szNumBuf);

    wsprintf(szNumBuf,"%d",pPif386->XmsMax);
    SetDlgItemText(hWnd, IDE_XMSMAX, szNumBuf);

    CheckDlgButton(hWnd, IDB_FULLSCREEN, pPif386->WinFlags & 0x08 ? 1 : 0);
    CheckDlgButton(hWnd, IDB_WINDOWED, pPif386->WinFlags & 0x08 ? 0 : 1);

    CheckDlgButton(hWnd, IDB_BACKEXEC, pPif386->WinFlags & 0x02 ? 1 : 0);
    CheckDlgButton(hWnd, IDB_EXCLEXEC, pPif386->WinFlags & 0x04 ? 1 : 0);

    CheckDlgButton(hWnd, IDB_FASTPASTE,  pPif386->TaskFlags & 0x0200 ? 1 : 0);
    CheckDlgButton(hWnd, IDB_ALLOWCLOSE, pPif386->WinFlags & 0x01 ? 1 : 0);
    CheckDlgButton(hWnd, IDB_CLOSEEXIT,  pPIFHDR->Flags1 & 0x10 ? 1 : 0);

    CheckDlgButton(hWnd, IDB_TEXT, pPif386->VidFlags & 0x10 ? 1 : 0);
    CheckDlgButton(hWnd, IDB_LOW,  pPif386->VidFlags & 0x20 ? 1 : 0);
    CheckDlgButton(hWnd, IDB_HIGH, pPif386->VidFlags & 0x40 ? 1 : 0);

    CheckDlgButton(hWnd, IDB_EMULATE, pPif386->VidFlags & 0x01 ? 1 : 0);
    CheckDlgButton(hWnd, IDB_RETAIN,  pPif386->VidFlags & 0x80 ? 1 : 0);

    return (0);
}


/****************************************************************************
 *
 *  FUNCTION :	ControlsToPIF(HWND)
 *
 *  PURPOSE  :	Extract the data from the dialog controls and fills the
 *		model .PIF structure
 *
 *  ENTRY    :	HWND	hWnd;		// Parent window handle
 *
 *  RETURNS  :	0	- Normal return
 *
 ****************************************************************************/

VOID ControlsToPIF(HWND hWnd)
{
    char	szNumBuf[32];

    PPIF	pPIF = &Globals.pifModel;

    DefaultPIF();

// Get data out of the controls

    GetDlgItemText(hWnd, IDE_FILENAME, pPIF->pifHdr.PgmName, sizeof(pPIF->pifHdr.PgmName));
    GetDlgItemText(hWnd, IDE_TITLE, pPIF->pifHdr.Title, sizeof(pPIF->pifHdr.Title));
    GetDlgItemText(hWnd, IDE_DIRECTORY, pPIF->pifHdr.StartupDir, sizeof(pPIF->pifHdr.StartupDir));
    GetDlgItemText(hWnd, IDE_PARAMS, pPIF->pif386.CmdLine3, sizeof(pPIF->pif386.CmdLine3));

	if (IsDlgButtonChecked(hWnd, IDB_CLOSEEXIT)) {
		pPIF->pifHdr.Flags1 |= 0x10;
	} else {
		pPIF->pifHdr.Flags1 &= ~0x10;
	}

    GetDlgItemText(hWnd, IDE_DOSMIN, szNumBuf, sizeof(szNumBuf));
    pPIF->pif386.DosMin = atoi(szNumBuf);

    GetDlgItemText(hWnd, IDE_DOSMAX, szNumBuf, sizeof(szNumBuf));
    pPIF->pif386.DosMax = atoi(szNumBuf);

    GetDlgItemText(hWnd, IDE_EMSMIN, szNumBuf, sizeof(szNumBuf));
    pPIF->pif386.EmsMin = atoi(szNumBuf);

    GetDlgItemText(hWnd, IDE_EMSMAX, szNumBuf, sizeof(szNumBuf));
    pPIF->pif386.EmsMax = atoi(szNumBuf);

    GetDlgItemText(hWnd, IDE_XMSMIN, szNumBuf, sizeof(szNumBuf));
    pPIF->pif386.XmsMin = atoi(szNumBuf);

    GetDlgItemText(hWnd, IDE_XMSMAX, szNumBuf, sizeof(szNumBuf));
    pPIF->pif386.XmsMax = atoi(szNumBuf);


    pPIF->pif386.WinFlags &= ~0x08;
    pPIF->pif386.WinFlags |= IsDlgButtonChecked(hWnd, IDB_FULLSCREEN) ? 0x08 : 0;
    pPIF->pif386.WinFlags |= IsDlgButtonChecked(hWnd, IDB_BACKEXEC) ? 0x02 : 0;
    pPIF->pif386.WinFlags |= IsDlgButtonChecked(hWnd, IDB_EXCLEXEC) ? 0x04 : 0;

    pPIF->pif386.WinFlags |= IsDlgButtonChecked(hWnd, IDB_ALLOWCLOSE) ? 0x01 : 0;
    pPIF->pif386.VidFlags = 0x00;
    pPIF->pif386.VidFlags |= IsDlgButtonChecked(hWnd, IDB_TEXT) ? 0x10 : 0;
    pPIF->pif386.VidFlags |= IsDlgButtonChecked(hWnd, IDB_LOW) ? 0x20 : 0;
    pPIF->pif386.VidFlags |= IsDlgButtonChecked(hWnd, IDB_HIGH) ? 0x40 : 0;


// Calculate the checksum for the old-style header
    pPIF->pifHdr.CheckSum = ComputePIFChecksum(pPIF);


    GetDlgItemText(hWnd, IDE_DOSMAX, szNumBuf, sizeof(szNumBuf));

}


/****************************************************************************
 *
 *  FUNCTION :	ValidateHotKey(BOOL fComplain)
 *
 *  PURPOSE  :	Validate the shortcut hotkey
 *
 *  ENTRY    :	BOOL	fComplain;	// Beep and show message box if TRUE
 *
 *  RETURNS  :	TRUE	- HotKey is OK
 *		FALSE	- HotKey is not valid
 *
 *	Rules for shortcut keys:
 *
 *	A shortcut key must have an ALT or CTRL modifier.
 *	SHIFT can also be used, but not alone.
 *
 *	You can't use ESC, ENTER, TAB, SPACEBAR, PRINT SCREEN, or BACKSPACE.
 *
 *	CTRL+Y, ALT+F4, CTRL+SHIFT+F11, CTRL+ALT+7 are examples of valid keys.
 *
 ****************************************************************************/

BOOL	ValidateHotKey(BOOL fComplain)
{
    BOOL	fALT	= IsDlgButtonChecked(Globals.hWndDlg, IDB_ALT);
    BOOL	fCTRL	= IsDlgButtonChecked(Globals.hWndDlg, IDB_CTRL);
//  BOOL	fSHIFT	= IsDlgButtonChecked(hWndDlg, IDB_SHIFT);

// Map the hotkey scancode to a virtual key code
    UINT	VKHotkey = MapVirtualKey(Globals.wHotkeyScancode, 1);

    if (Globals.wHotkeyScancode == 0) return (TRUE);	// 'None' is always valid

    if (!fALT && !fCTRL) goto HOTKEY_IS_BAD;	// Must have ALT or CTRL

    if (VKHotkey != VK_ESCAPE	&&
	VKHotkey != VK_RETURN	&&
	VKHotkey != VK_TAB	&&
	VKHotkey != VK_SPACE	&&
	VKHotkey != VK_SNAPSHOT &&
	VKHotkey != VK_BACK)	return (TRUE);

HOTKEY_IS_BAD:
    if (fComplain) {
	MessageBeep(MB_ICONEXCLAMATION);
	MessageBox(Globals.hWndDlg, BADHOTKEY_STR, HOTKEY_STR, MB_ICONEXCLAMATION);
    }

    return FALSE;
}


/* End of PIFEDIT.C */

