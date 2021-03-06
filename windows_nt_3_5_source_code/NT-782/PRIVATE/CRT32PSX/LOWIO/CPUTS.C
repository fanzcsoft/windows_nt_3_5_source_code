/***
*cputs.c - direct console output
*
*	Copyright (c) 1989-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	defines _cputs() - write string directly to console
*
*Revision History:
*	06-09-89  PHG	Module created, based on asm version
*	03-12-90  GJF	Made calling type _CALLTYPE2 (for now), added #include
*			<cruntime.h> and fixed the copyright. Also, cleaned up
*			the formatting a bit.
*	04-10-90  GJF	Now _CALLTYPE1.
*	06-05-90  SBM	Recoded as pure 32-bit, using new file handle state bits
*	07-24-90  SBM	Removed '32' from API names
*	09-28-90  GJF	New-style function declarator.
*	12-04-90  SRW	Changed to include <oscalls.h> instead of <doscalls.h>
*	12-06-90  SRW	Added _CRUISER_ and _WIN32 conditionals.
*	01-16-91  GJF	ANSI naming.
*	02-19-91  SRW	Adapt to OpenFile/CreateFile changes (_WIN32_)
*	02-25-91  MHL	Adapt to ReadFile/WriteFile changes (_WIN32_)
*	07-26-91  GJF	Took out init. stuff and cleaned up the error
*			handling [_WIN32_].
*	04-19-93  GJF	Use WriteConsole instead of WriteFile.
*
*******************************************************************************/

#include <cruntime.h>
#include <oscalls.h>
#include <os2dll.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>

/*
 * declaration for console handle
 */
extern int _confh;

/***
*int _cputs(string) - put a string to the console
*
*Purpose:
*	Writes the string directly to the console.  No newline
*	is appended.
*
*Entry:
*	char *string - string to write
*
*Exit:
*	Good return = 0
*	Error return = !0
*
*Exceptions:
*
*******************************************************************************/

int _CRTAPI1 _cputs (
	const char *string
	)
{
	ULONG num_written;
	int error = 0;			 /* error occurred? */

	_mlock(_CONIO_LOCK);		 /* acquire console lock */

	/* write string to console file handle */

#ifdef	_CRUISER_

	if (DOSWRITE(_confh, (char*)string, strlen(string), &num_written)) {
		/* OS/2 error, return error indicator */
		error = -1;
	}
#else	/* ndef _CRUISER_ */

#ifdef	_WIN32_

	if ( (_confh == -1) || !WriteConsole( (HANDLE)_confh,
					      (LPVOID)string,
					      strlen(string),
					      &num_written,
					      NULL )
	   )
		/* return error indicator */
                error = -1;

#else	/* ndef _WIN32_ */

#error ERROR - ONLY CRUISER OR WIN32 TARGET SUPPORTED!

#endif	/* _WIN32_ */

#endif	/* _CRUISER_ */

	_munlock(_CONIO_LOCK);		/* release console lock */

	return error;
}
