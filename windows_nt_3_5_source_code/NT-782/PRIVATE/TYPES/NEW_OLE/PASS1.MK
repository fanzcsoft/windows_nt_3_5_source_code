############################################################################
#
#   Microsoft Windows
#   Copyright (C) Microsoft Corporation, 1992 - 1993.
#   All rights reserved.
#
############################################################################

!include types.mk

BUILDMSG        =

TDLTYPES        = $(OLE_TDLTYPES)

PROXYTYPES      = $(OLE_PROXYTYPES)

NONPROXYTYPES   = $(OLE_NONPROXYTYPES)

BASE_TYPES      = $(OLE_BASE_TYPES)

BASE_HFILES     = $(OLE_BASE_HFILES)

TDLFILES        = $(TDLTYPES:.type=.tdl)

TYPES           = $(PROXYTYPES) $(NONPROXYTYPES)

IDLFILES        = $(TYPES:.type=.idl) $(BASE_TYPES:.type=.idl)

BASE_HFILES     = $(BASE_TYPES:.type=.h)

USE_OLE_MC      = 1

IDLFLAGS        = /import defined_single

ITFS_BASE       = ole2

ITFS_DIR        = $(HCOPY)

CLEANFILES      = $(ITFS_DIR)\ole2.h

!include ..\makefile.inc
!include $(DEPENDFILE)

all: $(MIDL_INCS)

