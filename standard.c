#include "main.h"

/****************************************************************************
 *
 *  FUNCTION :	SMMsgProc(HWND, UINT, WPARAM, LPARAM)
 *
 *  PURPOSE  :	Message proc for the Standard mode bitch dialog box
 *
 *  ENTRY    :	HWND	hWnd;		// Window handle
 *		UINT	msg;		// WM_xxx message
 *		WPARAM	wParam; 	// Message 16-bit parameter
 *		LPARAM	lParam; 	// Message 32-bit parameter
 *
 *  RETURNS  :	FALSE	- Message has been processed
 *		TRUE	- DefDlgProc() processing required
 *
 ****************************************************************************/

BOOL CALLBACK _export StandardMsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {

	case WM_CLOSE:
		{
			char	szNumBuf[32];

			PPIF	pPIF = &Globals.pifModel;

			GetDlgItemText(hWnd, IDE_FILENAME, pPIF->pifHdr.PgmName, sizeof(pPIF->pifHdr.PgmName));
			GetDlgItemText(hWnd, IDE_TITLE, pPIF->pifHdr.Title, sizeof(pPIF->pifHdr.Title));
			GetDlgItemText(hWnd, IDE_DIRECTORY, pPIF->pifHdr.StartupDir, sizeof(pPIF->pifHdr.StartupDir));
			GetDlgItemText(hWnd, IDE_PARAMS, pPIF->pifHdr.CmdLineS, sizeof(pPIF->pif386.CmdLine3));

			if (IsDlgButtonChecked(hWnd, IDB_CLOSEEXIT)) {
				pPIF->pifHdr.Flags1 |= 0x10;
			} else {
				pPIF->pifHdr.Flags1 &= ~0x10;
			}

			GetDlgItemText(hWnd, IDE_DOSMIN, szNumBuf, sizeof(szNumBuf));
			pPIF->pifHdr.DosMinS = atoi(szNumBuf);

			GetDlgItemText(hWnd, IDE_XMSMIN, szNumBuf, sizeof(szNumBuf));
			pPIF->pif286.XmsMin = atoi(szNumBuf);

			GetDlgItemText(hWnd, IDE_XMSMAX, szNumBuf, sizeof(szNumBuf));
			pPIF->pif286.XmsMax = atoi(szNumBuf);

			pPIF->pif286.Flags1 |= IsDlgButtonChecked(hWnd, IDB_ALTTAB) ? 0x01 : 0;
			pPIF->pif286.Flags1 |= IsDlgButtonChecked(hWnd, IDB_ALTESC) ? 0x02 : 0;
			pPIF->pif286.Flags1 |= IsDlgButtonChecked(hWnd, IDB_ALTPRTSC) ? 0x04 : 0;
			pPIF->pif286.Flags1 |= IsDlgButtonChecked(hWnd, IDB_PRTSC) ? 0x08 : 0;
			pPIF->pif286.Flags1 |= IsDlgButtonChecked(hWnd, IDB_CTRLESC) ? 0x10 : 0;

			EndDialog(hWnd, IDCANCEL);
		}
		break;


	case WM_INITDIALOG:
		{
			// Copy the data to the dialog controls from our .PIF model
			char szNumBuf[32];
			int 	n;

			PPIFHDR	pPIFHDR = (PPIFHDR) &Globals.pifModel;
			PPIFSIG	pPifSig = NULL;
			PPIF286	pPif286 = NULL;

			// Verify the checksum
			if (pPIFHDR->CheckSum != 0 && pPIFHDR->CheckSum != ComputePIFChecksum(&Globals.pifModel)) 
			{
				MessageBoxString(hWnd, Globals.hInst, IDS_BADCHECKSUM, MB_OK);
			}

			// Find the Standard mode section
			pPifSig = (PPIFSIG) (pPIFHDR + 1);

			while ( ((n = lstrcmp(pPifSig->Signature, "WINDOWS 286 3.0")) != 0) &&
				pPifSig->NextOff != 0xFFFF &&
				pPifSig->NextOff > 0 &&
				(PBYTE) pPifSig < (PBYTE) (&Globals.pifModel+1) ) {

				pPifSig = (PPIFSIG) ((PBYTE) pPIFHDR + pPifSig->NextOff);

			}

			if (n) {	// No Standard mode section -- use the defaults
				MessageBoxString(hWnd, Globals.hInst, IDS_BADCHECKSUM, MB_OK);
				return (1);
			}

			pPif286 = (PPIF286) ((PBYTE) pPIFHDR + pPifSig->DataOff);

			// Fill in the controls

			SetDlgItemText(hWnd, IDE_FILENAME, pPIFHDR->PgmName);

			FixNul(pPIFHDR->Title, sizeof(pPIFHDR->Title));
			SetDlgItemText(hWnd, IDE_TITLE, pPIFHDR->Title);

			SetDlgItemText(hWnd, IDE_PARAMS, pPIFHDR->CmdLineS);
			SetDlgItemText(hWnd, IDE_DIRECTORY, pPIFHDR->StartupDir);

			wsprintf(szNumBuf,"%d",pPIFHDR->DosMinS);
			SetDlgItemText(hWnd, IDE_DOSMIN, szNumBuf);
			
			wsprintf(szNumBuf,"%d",pPif286->XmsMin);
			SetDlgItemText(hWnd, IDE_XMSMIN, szNumBuf);

			wsprintf(szNumBuf,"%d",pPif286->XmsMax);
			SetDlgItemText(hWnd, IDE_XMSMAX, szNumBuf);

			CheckDlgButton(hWnd, IDB_ALTTAB, pPif286->Flags1 & 0x01 ? 1 : 0);
			CheckDlgButton(hWnd, IDB_ALTESC, pPif286->Flags1 & 0x02 ? 1 : 0);
			CheckDlgButton(hWnd, IDB_ALTPRTSC, pPif286->Flags1 & 0x04 ? 1 : 0);
			CheckDlgButton(hWnd, IDB_PRTSC, pPif286->Flags1 & 0x08 ? 1 : 0);
			CheckDlgButton(hWnd, IDB_CTRLESC, pPif286->Flags1 & 0x10 ? 1 : 0);

		}
#if 0
	case WM_COMMAND:
		{
			switch(wParam)
			{
				case IDOK:
				case IDCANCEL:
				{
					EndDialog(hWnd, 0);
					return TRUE;
				}
			}
		}
#endif
	}

	return (FALSE);
}


/****************************************************************************
 *
 *  FUNCTION :	StandardModeBitchBox(HWND)
 *
 *  PURPOSE  :	Puts up a dialog box to warn about Standard mode
 *
 *  ENTRY    :	HWND	hWnd;		// Parent window handle
 *
 *  RETURNS  :	VOID
 *
 ****************************************************************************/

VOID StandardModeBitchBox(HWND hWnd)
{
    FARPROC	lpfnSMMsgProc;

    MessageBeep(MB_ICONEXCLAMATION);

    lpfnSMMsgProc = MakeProcInstance((FARPROC) SMMsgProc, Globals.hInst);

    DialogBox(Globals.hInst, MAKEINTRESOURCE(IDD_STANDARDBITCH), hWnd, (DLGPROC) lpfnSMMsgProc);

    FreeProcInstance(lpfnSMMsgProc);
}


/****************************************************************************
 *
 *  FUNCTION :	SMMsgProc(HWND, UINT, WPARAM, LPARAM)
 *
 *  PURPOSE  :	Message proc for the Standard mode bitch dialog box
 *
 *  ENTRY    :	HWND	hWnd;		// Window handle
 *		UINT	msg;		// WM_xxx message
 *		WPARAM	wParam; 	// Message 16-bit parameter
 *		LPARAM	lParam; 	// Message 32-bit parameter
 *
 *  RETURNS  :	FALSE	- Message has been processed
 *		TRUE	- DefDlgProc() processing required
 *
 ****************************************************************************/

BOOL CALLBACK _export SMMsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
	case WM_COMMAND:
	    switch (wParam) {	// id
		case IDOK:
		case IDCANCEL:
		    EndDialog(hWnd, TRUE);
	    }
	    return (0);

	case WM_INITDIALOG:
	    return (TRUE);
    }

    return (FALSE);
}

