/************************  The Qualitas PIF Editor  ***************************
 *									      *
 *	     (C) Copyright 1992, 1993 Qualitas, Inc.  GNU General Public License version 3.    *
 *									      *
 *  MODULE   :	PIF.C - PIF parser
 *									      *
 ******************************************************************************/

#define NOCOMM			// Avoid inclusion of bulky COMM driver stuff
#include <windows.h>

#include <string.h>		// For memset

#include "main.h"

extern PIFEDIT_GLOBALS Globals;

/****************************************************************************
 *
 *  FUNCTION :	ComputePIFChecksum(PPIF)
 *
 *  PURPOSE  :	Calculate the checksum on the old-style .PIF header
 *
 *  ENTRY    :	PPIF	pPIF;		// ==> .PIF structure
 *
 *  RETURNS  :	Checksum as a BYTE
 *
 ****************************************************************************/

BYTE ComputePIFChecksum(PPIF pPIF)		// Checksum the PIFHDR
{
    PBYTE	pb = (PBYTE) pPIF;
    PBYTE	pbz = pb + sizeof(PIFHDR);	// Size of old-style header
    BYTE	bSum;

    pb += 2;		// Skip over first unknown byte and checksum

    for (bSum = 0; pb < pbz; pb++) bSum += *pb;

    return (bSum);
}

/****************************************************************************
 *
 *  FUNCTION :	DefaultPIF(VOID)
 *
 *  PURPOSE  :	Fill in the .PIF structure with the standard default values
 *
 *  ENTRY    :	VOID
 *
 *  RETURNS  :	VOID
 *
 ****************************************************************************/

VOID DefaultPIF(VOID)
{
    memset(&Globals.pifModel, 0, sizeof(PIF));

    lstrcpy(Globals.pifModel.pifSigEX.Signature, "MICROSOFT PIFEX");
    Globals.pifModel.pifSigEX.DataOff = 0;					// 0000
    Globals.pifModel.pifSigEX.DataLen = sizeof(PIFHDR); 			// 0171
    Globals.pifModel.pifSigEX.NextOff = sizeof(PIFHDR) + sizeof(PIFSIG);	// 0187

    lstrcpy(Globals.pifModel.pifSig286.Signature, "WINDOWS 286 3.0");
    Globals.pifModel.pifSig286.DataOff = Globals.pifModel.pifSigEX.NextOff + sizeof(PIFSIG);
    Globals.pifModel.pifSig286.DataLen = sizeof(PIF286);
    Globals.pifModel.pifSig286.NextOff = sizeof(PIFHDR) + 2*sizeof(PIFSIG)+ sizeof(PIF286);

    lstrcpy(Globals.pifModel.pifSig386.Signature, "WINDOWS 386 3.0");
    Globals.pifModel.pifSig386.DataOff = Globals.pifModel.pifSig286.NextOff + sizeof(PIFSIG);
    Globals.pifModel.pifSig386.DataLen = sizeof(PIF386);
    Globals.pifModel.pifSig386.NextOff = 0xFFFF;

    Globals.pifModel.pifHdr.Title[0] = '\0';
    Globals.pifModel.pifHdr.PgmName[0] = '\0';
    Globals.pifModel.pifHdr.StartupDir[0] = '\0';
    Globals.pifModel.pifHdr.CmdLineS[0] = '\0';

// TopView fields
    Globals.pifModel.pifHdr.ScreenType = 0x7F;		// Windows doesn't use this
    Globals.pifModel.pifHdr.ScreenPages = 1;		// # of screen pages
    Globals.pifModel.pifHdr.IntVecLow = 0;		// Low	bound of INT vectors
    Globals.pifModel.pifHdr.IntVecHigh = 255;		// High bound of INT vectors

    Globals.pifModel.pifHdr.ScrnRows = 25;		// Windows doesn't use this
    Globals.pifModel.pifHdr.ScrnCols = 80;		// Windows doesn't use this
//  pifModel.pifHdr.RowOffs = 80;		// Windows doesn't use this
//  pifModel.pifHdr.ColOffs = 80;		// Windows doesn't use this

    Globals.pifModel.pifHdr.SystemMem = 7;		// 7 - text, 23 - graphics

    Globals.pifModel.pifHdr.DosMaxS = 640;
    Globals.pifModel.pifHdr.DosMinS = 128;
    Globals.pifModel.pifHdr.Flags1 = 0x10;		// Close window on exit

    Globals.pifModel.pif386.DosMax = 640;
    Globals.pifModel.pif386.DosMin = 128;

    Globals.pifModel.pif386.ForePrio = 100;
    Globals.pifModel.pif386.BackPrio = 50;

    Globals.pifModel.pif386.EmsMax = 1024;
    Globals.pifModel.pif386.EmsMin = 0;
    Globals.pifModel.pif386.XmsMax = 1024;
    Globals.pifModel.pif386.XmsMin = 0;

    Globals.pifModel.pif386.WinFlags = 0x08;		// Fullscreen
    Globals.pifModel.pif386.TaskFlags = 0x0200 | 0x0010; // Allow faste paste
						// Detect idle time
    Globals.pifModel.pif386.VidFlags = 0x1F;		// Video mode text
						// Emulate text mode
						// No monitor ports
    Globals.pifModel.pif386.CmdLine3[0] = '\0';

    Globals.pifModel.pifHdr.CheckSum = ComputePIFChecksum(&Globals.pifModel);

}

/****************************************************************************
 *
 *  FUNCTION :	SnapPif(VOID)
 *
 *  PURPOSE  :	Copy the .PIF model to a backup for CheckSave()
 *		inspection later
 *
 *  ENTRY    :	VOID
 *
 *  RETURNS  :	VOID
 *
 ****************************************************************************/

VOID SnapPif(VOID)
{
    memcpy(&Globals.pifBackup, &Globals.pifModel, sizeof(PIF));
}

/****************************************************************************
 *
 *  FUNCTION :	WritePIF(LPSTR, LPSTR)
 *
 *  PURPOSE  :	Helper function for MenuFileSave(), etc.
 *		Does the dirty work of the COMMDLG GetSaveileName()
 *		Writes the .PIF file from local memory
 *
 *  ENTRY    :	LPSTR	lpszPIFName;	// ==> .PIF filespec
 *
 *  RETURNS  :	0	- Normal return
 *		IDABORT - File system error
 *
 ****************************************************************************/

int WritePIF(HWND hWndDlg, LPSTR lpszPIFName)
{
    register int	rc;
    HCURSOR	hCursorOld;
    OFSTRUCT	OpenBuff;

    HFILE	hFilePIF = 0;

/* Switch to the hour glass cursor */

    SetCapture(hWndDlg);
    hCursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

/* Open and write the file */

    DebugPrintf("SavePIF(): OpenFile(%s, OF_READ)\r\n", lpszPIFName);

    hFilePIF = OpenFile(lpszPIFName, &OpenBuff, OF_CREATE | OF_WRITE);

    if ((int) hFilePIF != HFILE_ERROR) {

	ControlsToPIF(hWndDlg);

	rc = _lwrite(hFilePIF, (LPVOID) (PPIF) &Globals.pifModel, sizeof(PIF));
	if (rc != sizeof(PIF)) {
	    DebugPrintf("SavePIF() _lwrite(hFilePIF, (LPVOID) (PPIF) &pifModel, sizeof(PIF)) failed %d\r\n", rc);
	    rc = IDABORT;  goto WRITEPIF_EXIT;
	}
    } else {	/* Couldn't save the file */
		DebugPrintf("SavePIF() OpenFile(%s, &OpenBuff, OF_CREATE | OF_WRITE) failed\r\n", lpszPIFName);
		rc = IDABORT;  goto WRITEPIF_EXIT;

    }

    rc = 0;

WRITEPIF_EXIT:
    SetCursor(hCursorOld);
    ReleaseCapture();

    if (hFilePIF > 0) _lclose(hFilePIF);

    return (rc);
}

// PIFClass implementation

// Methods

BYTE PIFClass_ComputeChecksum(LPPIFClass self)
{
    PBYTE	pb = (PBYTE) &self->Pif;
    PBYTE	pbz = pb + sizeof(PIFHDR);	// Size of old-style header
    BYTE	bSum;

    pb += 2;		// Skip over first unknown byte and checksum

    for (bSum = 0; pb < pbz; pb++) bSum += *pb;

    return (bSum);
}

int PIFClass_Read(LPPIFClass self, LPSTR lpszPIFName)
{
    HFILE hFilePIF;
	WORD wPifLen;
	int rc=IDABORT;

	if ((hFilePIF=_lopen(lpszPIFName, OF_READ)) != HFILE_ERROR) 
	{	
		if ((wPifLen=_llseek(hFilePIF, 0L, SEEK_END))<sizeof(self->Pif))
		{
			_llseek(hFilePIF, 0L, SEEK_SET);

			if (_lread(hFilePIF, (LPBYTE)&self->Pif, wPifLen)==wPifLen)	rc=0;
		}
		_lclose(hFilePIF);
	}
	
	return rc;
}

// Attributes
VOID PIFClass__set_Title(LPPIFClass self, LPSTR lpszPIFName)
{
	lstrcpy(self->Pif.pifHdr.Title, lpszPIFName);
	AnsiToOem(self->Pif.pifHdr.Title, self->Pif.pifHdr.Title);
	FixNul(self->Pif.pifHdr.Title, sizeof(self->Pif.pifHdr.Title));
}

LPSTR PIFClass__get_Title(LPPIFClass self)
{
	//@todo oemtoansi here
	return self->Pif.pifHdr.Title;
}

VOID PIFClass__set_DOSMin(LPPIFClass self, WORD wValue)
{
	self->Pif.pifHdr.DosMinS=wValue;
}

WORD PIFClass__get_DOSMin(LPPIFClass self)
{
	return self->Pif.pifHdr.DosMinS;
}

VOID PIFClass__set_DOSMax(LPPIFClass self, WORD wValue)
{
	self->Pif.pifHdr.DosMaxS=wValue;
}

WORD PIFClass__get_DOSMax(LPPIFClass self)
{
	return self->Pif.pifHdr.DosMaxS;
}

VOID PIFClass__set_ProgramName(LPPIFClass self, LPSTR lpszPIFName)
{
	lstrcpy(self->Pif.pifHdr.PgmName, lpszPIFName);
	AnsiToOem(self->Pif.pifHdr.PgmName, self->Pif.pifHdr.PgmName);
	FixNul(self->Pif.pifHdr.PgmName, sizeof(self->Pif.pifHdr.PgmName));
}

LPSTR PIFClass__get_ProgramName(LPPIFClass self)
{
	//@todo oemtoansi here
	return self->Pif.pifHdr.PgmName;
}

VOID PIFClass__set_Resident(LPPIFClass self, BOOL bValue)
{
	if (bValue) self->Pif.pifHdr.Flags1 |= fResident;
}

BOOL PIFClass__get_Resident(LPPIFClass self)
{
	return (self->Pif.pifHdr.Flags1 & fResident) == fResident;
}

VOID PIFClass__set_Graphics(LPPIFClass self, BOOL bValue)
{
	if (bValue) self->Pif.pifHdr.Flags1 |= fGraphics;
}

BOOL PIFClass__get_Graphics(LPPIFClass self)
{
	return (self->Pif.pifHdr.Flags1 & fGraphics) == fGraphics;
}

VOID PIFClass__set_NoSwitch(LPPIFClass self, BOOL bValue)
{
	if (bValue) self->Pif.pifHdr.Flags1 |= fNoSwitch;
}

BOOL PIFClass__get_NoSwitch(LPPIFClass self)
{
	return (self->Pif.pifHdr.Flags1 & fNoSwitch) == fNoSwitch;
}

VOID PIFClass__set_NoGrab(LPPIFClass self, BOOL bValue)
{
	if (bValue) self->Pif.pifHdr.Flags1 |= fNoGrab;
}

BOOL PIFClass__get_NoGrab(LPPIFClass self)
{
	return (self->Pif.pifHdr.Flags1 & fNoGrab) == fNoGrab;
}

VOID PIFClass__set_Destroy(LPPIFClass self, BOOL bValue)
{
	if (bValue) self->Pif.pifHdr.Flags1 |= fDestroy;
}

BOOL PIFClass__get_Destroy(LPPIFClass self)
{
	return (self->Pif.pifHdr.Flags1 & fDestroy) == fDestroy;
}

VOID PIFClass__set_COM2(LPPIFClass self, BOOL bValue)
{
	if (bValue) self->Pif.pifHdr.Flags1 |= fCOM2;
}

BOOL PIFClass__get_COM2(LPPIFClass self)
{
	return (self->Pif.pifHdr.Flags1 & fCOM2) == fCOM2;
}

VOID PIFClass__set_COM1(LPPIFClass self, BOOL bValue)
{
	if (bValue) self->Pif.pifHdr.Flags1 |= fCOM1;
}

BOOL PIFClass__get_COM1(LPPIFClass self)
{
	return (self->Pif.pifHdr.Flags1 & fCOM1) == fCOM1;
}

VOID PIFClass__set_StartupDir(LPPIFClass self, LPSTR lpszPIFName)
{
	lstrcpy(self->Pif.pifHdr.StartupDir, lpszPIFName);
	AnsiToOem(self->Pif.pifHdr.StartupDir, self->Pif.pifHdr.StartupDir);
	FixNul(self->Pif.pifHdr.StartupDir, sizeof(self->Pif.pifHdr.StartupDir));
}

LPSTR PIFClass__get_StartupDir(LPPIFClass self)
{
	//@todo oemtoansi here
	return self->Pif.pifHdr.StartupDir;
}

VOID PIFClass__set_CmdLine(LPPIFClass self, LPSTR lpszPIFName)
{
	lstrcpy(self->Pif.pifHdr.CmdLineS, lpszPIFName);
	AnsiToOem(self->Pif.pifHdr.CmdLineS, self->Pif.pifHdr.CmdLineS);
	FixNul(self->Pif.pifHdr.CmdLineS, sizeof(self->Pif.pifHdr.CmdLineS));
}

LPSTR PIFClass__get_CmdLine(LPPIFClass self)
{
	//@todo oemtoansi here
	return self->Pif.pifHdr.CmdLineS;
}

VOID PIFClass__set_ScreenType(LPPIFClass self, BYTE bValue)
{
	self->Pif.pifHdr.ScreenType=bValue;
}

BYTE PIFClass__get_ScreenType(LPPIFClass self)
{
	return self->Pif.pifHdr.ScreenType;
}

VOID PIFClass__set_ScreenPages(LPPIFClass self, BYTE bValue)
{
	self->Pif.pifHdr.ScreenPages=bValue;
}

BYTE PIFClass__get_ScreenPages(LPPIFClass self)
{
	return self->Pif.pifHdr.ScreenPages;
}

VOID PIFClass__set_IntVecLow(LPPIFClass self, BYTE bValue)
{
	self->Pif.pifHdr.IntVecLow=bValue;
}

BYTE PIFClass__get_IntVecLow(LPPIFClass self)
{
	return self->Pif.pifHdr.IntVecLow;
}

VOID PIFClass__set_IntVecHigh(LPPIFClass self, BYTE bValue)
{
	self->Pif.pifHdr.IntVecHigh=bValue;
}

BYTE PIFClass__get_IntVecHigh(LPPIFClass self)
{
	return self->Pif.pifHdr.IntVecHigh;
}

VOID PIFClass__set_ScrnRows(LPPIFClass self, BYTE bValue)
{
	self->Pif.pifHdr.ScrnRows=bValue;
}

BYTE PIFClass__get_ScrnRows(LPPIFClass self)
{
	return self->Pif.pifHdr.ScrnRows;
}

VOID PIFClass__set_ScrnCols(LPPIFClass self, BYTE bValue)
{
	self->Pif.pifHdr.ScrnCols=bValue;
}

BYTE PIFClass__get_ScrnCols(LPPIFClass self)
{
	return self->Pif.pifHdr.ScrnCols;
}

VOID PIFClass__set_RowOffs(LPPIFClass self, BYTE bValue)
{
	self->Pif.pifHdr.RowOffs=bValue;
}

BYTE PIFClass__get_RowOffs(LPPIFClass self)
{
	return self->Pif.pifHdr.RowOffs;
}

VOID PIFClass__set_ColOffs(LPPIFClass self, BYTE bValue)
{
	self->Pif.pifHdr.ColOffs=bValue;
}

BYTE PIFClass__get_ColOffs(LPPIFClass self)
{
	return self->Pif.pifHdr.ColOffs;
}

VOID PIFClass__set_SystemMem(LPPIFClass self, WORD wValue)
{
	self->Pif.pifHdr.SystemMem=wValue;
}

WORD PIFClass__get_SystemMem(LPPIFClass self)
{
	return self->Pif.pifHdr.SystemMem;
}

VOID PIFClass__set_FullScreen(LPPIFClass self, BOOL bValue)
{
	if (bValue) self->Pif.pifHdr.Flags2 |= fFullScreen;
}

BOOL PIFClass__get_FullScreen(LPPIFClass self)
{
	return (self->Pif.pifHdr.Flags2 & fFullScreen) == fFullScreen;
}

VOID PIFClass__set_Foreground(LPPIFClass self, BOOL bValue)
{
	if (bValue) self->Pif.pifHdr.Flags2 |= fForeground;
}

BOOL PIFClass__get_Foreground(LPPIFClass self)
{
	return (self->Pif.pifHdr.Flags2 & fForeground) == fForeground;
}

VOID PIFClass__set_CoProcessor(LPPIFClass self, BOOL bValue)
{
	if (bValue) self->Pif.pifHdr.Flags2 |= fCoProcessor;
}

BOOL PIFClass__get_CoProcessor(LPPIFClass self)
{
	return (self->Pif.pifHdr.Flags2 & fCoProcessor) == fCoProcessor;
}

VOID PIFClass__set_Keyboard(LPPIFClass self, BOOL bValue)
{
	if (bValue) self->Pif.pifHdr.Flags2 |= fKeyboard;
}

BOOL PIFClass__get_Keyboard(LPPIFClass self)
{
	return (self->Pif.pifHdr.Flags2 & fKeyboard) == fKeyboard;
}

// Constructor
LPPIFClass PIFClassNew(VOID)
{
	LPPIFClass rc;
	
	// Allocate object memory
	rc=(LPPIFClass)GlobalAlloc(GMEM_MOVEABLE, sizeof(PIFClass));
	
	// Initialize methods
	rc->ComputeChecksum=&PIFClass_ComputeChecksum;
	rc->Read=&PIFClass_Read;
	
	// Initialize attributes
	rc->_set_Title=&PIFClass__set_Title;
	rc->_get_Title=&PIFClass__get_Title;
	rc->_set_DOSMin=&PIFClass__set_DOSMin;
	rc->_get_DOSMin=&PIFClass__get_DOSMin;
	rc->_set_DOSMax=&PIFClass__set_DOSMax;
	rc->_get_DOSMax=&PIFClass__get_DOSMax;
	rc->_set_ProgramName=&PIFClass__set_ProgramName;
	rc->_get_ProgramName=&PIFClass__get_ProgramName;
	rc->_set_Resident=&PIFClass__set_Resident;
	rc->_get_Resident=&PIFClass__get_Resident;
	rc->_set_Graphics=&PIFClass__set_Graphics;
	rc->_get_Graphics=&PIFClass__get_Graphics;
	rc->_set_NoSwitch=&PIFClass__set_NoSwitch;
	rc->_get_NoSwitch=&PIFClass__get_NoSwitch;
	rc->_set_NoGrab=&PIFClass__set_NoGrab;
	rc->_get_NoGrab=&PIFClass__get_NoGrab;
	rc->_set_Destroy=&PIFClass__set_Destroy;
	rc->_get_Destroy=&PIFClass__get_Destroy;
	rc->_set_COM2=&PIFClass__set_COM2;
	rc->_get_COM2=&PIFClass__get_COM2;
	rc->_set_COM1=&PIFClass__set_COM1;
	rc->_get_COM1=&PIFClass__get_COM1;
	rc->_set_StartupDir=&PIFClass__set_StartupDir;
	rc->_get_StartupDir=&PIFClass__get_StartupDir;
	rc->_set_CmdLine=&PIFClass__set_CmdLine;
	rc->_get_CmdLine=&PIFClass__get_CmdLine;
	rc->_set_ScreenType=&PIFClass__set_ScreenType;
	rc->_get_ScreenType=&PIFClass__get_ScreenType;
	rc->_set_ScreenPages=&PIFClass__set_ScreenPages;
	rc->_get_ScreenPages=&PIFClass__get_ScreenPages;
	rc->_set_IntVecLow=&PIFClass__set_IntVecLow;
	rc->_get_IntVecLow=&PIFClass__get_IntVecLow;
	rc->_set_IntVecHigh=&PIFClass__set_IntVecHigh;
	rc->_get_IntVecHigh=&PIFClass__get_IntVecHigh;
	rc->_set_ScrnRows=&PIFClass__set_ScrnRows;
	rc->_get_ScrnRows=&PIFClass__get_ScrnRows;
	rc->_set_ScrnCols=&PIFClass__set_ScrnCols;
	rc->_get_ScrnCols=&PIFClass__get_ScrnCols;
	rc->_set_RowOffs=&PIFClass__set_RowOffs;
	rc->_get_RowOffs=&PIFClass__get_RowOffs;
	rc->_set_ColOffs=&PIFClass__set_ColOffs;
	rc->_get_ColOffs=&PIFClass__get_ColOffs;
	rc->_set_SystemMem=&PIFClass__set_SystemMem;
	rc->_get_SystemMem=&PIFClass__get_SystemMem;
	rc->_set_FullScreen=&PIFClass__set_FullScreen;
	rc->_get_FullScreen=&PIFClass__get_FullScreen;
	rc->_set_Foreground=&PIFClass__set_Foreground;
	rc->_get_Foreground=&PIFClass__get_Foreground;
	rc->_set_CoProcessor=&PIFClass__set_CoProcessor;
	rc->_get_CoProcessor=&PIFClass__get_CoProcessor;
	rc->_set_Keyboard=&PIFClass__set_Keyboard;
	rc->_get_Keyboard=&PIFClass__get_Keyboard;

	// Initialize data
	rc->Mode=0;			// Default is a Real mode
	
	return rc;
}

