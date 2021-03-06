/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright <c> 1993 Microsoft Corporation

Module Name :

	mrshlp.h

Abtract :
	
	Contains private definitions for mrshl.c.

Author : 
	
	David Kays	dkays 	September 1993

Revision History :

--------------------------------------------------------------------*/

#ifndef _MRSHLP_
#define _MRSHLP_

void 
NdrpPointerMarshall( 
	PMIDL_STUB_MESSAGE			pStubMsg, 
	uchar *						pBufferMark,
	uchar *						pMemory, 
	PFORMAT_STRING				pFormat
	);

void 
NdrpConformantArrayMarshall( 
	PMIDL_STUB_MESSAGE			pStubMsg, 
	uchar *						pMemory, 
	PFORMAT_STRING				pFormat
	);

void 
NdrpConformantVaryingArrayMarshall( 
	PMIDL_STUB_MESSAGE			pStubMsg, 
	uchar *						pMemory, 
	PFORMAT_STRING				pFormat
	);

void 
NdrpComplexArrayMarshall( 
	PMIDL_STUB_MESSAGE			pStubMsg, 
	uchar *						pMemory, 
	PFORMAT_STRING				pFormat
	);

void 
NdrpConformantStringMarshall( 
	PMIDL_STUB_MESSAGE			pStubMsg, 
	uchar *						pMemory, 
	PFORMAT_STRING				pFormat
	);

void 
NdrpUnionMarshall( 
	PMIDL_STUB_MESSAGE			pStubMsg, 
	uchar *						pMemory, 
	PFORMAT_STRING				pFormat,
	long						SwitchIs,
	uchar						SwitchType
	);

PFORMAT_STRING
NdrpEmbeddedPointerMarshall( 
	PMIDL_STUB_MESSAGE			pStubMsg, 
	uchar *						pMemory, 
	PFORMAT_STRING				pFormat
	);

PFORMAT_STRING 
NdrpEmbeddedRepeatPointerMarshall( 
	PMIDL_STUB_MESSAGE			pStubMsg, 
	uchar *						pMemory, 
	PFORMAT_STRING				pFormat
	);

ulong 
NdrpComputeConformance(
	PMIDL_STUB_MESSAGE			pStubMsg, 
	uchar *						pMemory, 
	PFORMAT_STRING				pFormat
	);

void 
NdrpComputeVariance(
	PMIDL_STUB_MESSAGE			pStubMsg, 
	uchar *						pMemory, 
	PFORMAT_STRING				pFormat
	);

typedef char * 	(RPC_ENTRY * PMARSHALL_ROUTINE)( 
					PMIDL_STUB_MESSAGE, 
					uchar *, 
					PFORMAT_STRING
				);

typedef void 	(* PPRIVATE_MARSHALL_ROUTINE)( 
					PMIDL_STUB_MESSAGE, 
					uchar *, 
					PFORMAT_STRING
				);
//
// Function table defined in mrshl.c
//
extern PMARSHALL_ROUTINE 			pfnMarshallRoutines[]; 

#endif
