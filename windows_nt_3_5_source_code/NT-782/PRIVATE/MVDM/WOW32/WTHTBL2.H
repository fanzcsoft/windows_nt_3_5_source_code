/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WTHTBL2.h
 *  WOW32 16-bit toolhelp API thunk table
 *
 *  This file is included into the master thunk table.
 *
--*/

    {W32FUN(WK32UNIMPLEMENTEDAPI,           "DUMMYENTRY",           MOD_TOOLHELP,     0)},
    {W32FUN(WK32UNIMPLEMENTEDAPI,           "CLASSFIRST",           MOD_TOOLHELP,      sizeof (CLASSFIRST16))},
    {W32FUN(WK32UNIMPLEMENTEDAPI,           "CLASSNEXT",            MOD_TOOLHELP,      sizeof (CLASSNEXT16))},
