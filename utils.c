#include "main.h"

int MessageBoxString(HWND hWnd, HINSTANCE hInst, UINT uID, UINT uType)
{
	char	szBuf[128];

	LoadString(hInst, uID, szBuf, sizeof(szBuf));
	return MessageBox(hWnd, szBuf, Globals.szAppTitle, uType);
}

void FAR cdecl MessageBoxPrintf(LPSTR szFormat, ...)
{
    char ach[256];
    int  s,d;

    s = wvsprintf(ach, szFormat, (LPSTR) (&szFormat + 1));

    for (d = sizeof(ach) - 1; s >= 0; s--) {
	if ((ach[d--] = ach[s]) == '\n')
	    ach[d--] = '\r';
    }

    MessageBox(0, ach+d+1, "MessageBoxPrintf()", MB_OK);
}

#ifdef DEBUG
void FAR cdecl DebugPrintf(LPSTR szFormat, ...)
{
    char ach[256];
    int  s,d;

    if (!GetSystemMetrics(SM_DEBUG)) return;

    s = wvsprintf(ach, szFormat, (LPSTR) (&szFormat + 1));

    for (d = sizeof(ach) - 1; s >= 0; s--) {
	if ((ach[d--] = ach[s]) == '\n')
	    ach[d--] = '\r';
    }

    OutputDebugString(ach+d+1);
}


#endif	/* #ifdef DEBUG */

// make a file name from a directory name by appending '\' (if necessary)
//   and then appending the filename
void MakePathName(register char *dname, char *pszFileName)
{
	register int length;

	length = lstrlen( dname );

	if ((*dname) && (strchr( "/\\:", dname[length-1] ) == NULL ))
		lstrcat( dname, "\\" );

	lstrcat( dname, pszFileName );
}

/****************************************************************************
 *
 *  FUNCTION :	FixNul(PSTR, int)
 *
 *  PURPOSE  :	NUL-terminate string before last whitespace
 *		Used to remove trailing whitespace in buffers
 *
 *  ENTRY    :	PSTR	psz;		// ==> string
 *		int	n;		// lstrlen(string)
 *
 *  RETURNS  :	VOID
 *
 ****************************************************************************/

// Walk backward from the end of the string to the first non-whitespace or NUL

VOID FixNul(register PSTR psz, int n)
{
    PSTR p = psz + n - 1;

    while (p >= psz && (*p == '\0' || *p == ' ')) p--;

    *(p+1) = '\0';
}
