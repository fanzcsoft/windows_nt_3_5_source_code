/************************* Module Header *************************************
 * page.c
 *      Functions associated with page concept, such as new frame,
 *      startdoc,  abortdoc etc.
 *
 * HISTORY:
 *  14:25 on Mon 21 Jan 1991    -by-    Lindsay Harris   [lindsayh]
 *      Created it,  based partly on windows control.c
 *
 *
 * Copyright (C) 1991 - 1993  Microsoft Corporation
 *
 ****************************************************************************/

#include        <stddef.h>
#include        <windows.h>
#include        <winddi.h>

#include        "libproto.h"
#include        "win30def.h"
#include        "udmindrv.h"
#include        "udpfm.h"
#include        "uddevice.h"
#include        "udresrc.h"
#include        "pdev.h"
#include        "udresid.h"
#include        "udrender.h"
#include        "fnenabl.h"

#include        "stretch.h"             /* Palette data */


/*
 *    The following is the default order for sending initialising commands
 *  to a printer.  Typically this data is supplied by a minidriver, but if
 *  it follows this order,  the data may be omitted.
 */

static const  short pDefInitOrder[] =
{
    PC_ORD_PAPER_SOURCE,
    PC_ORD_PAPER_DEST,
    PC_ORD_PAPER_SIZE,
    PC_ORD_RESOLUTION,
    PC_ORD_TEXTQUALITY,
    0
};

/*
 *    Local function prototypes.
 */

BOOL   bSendInitialisation( PDEV *, BOOL );
BOOL   bRenderBM( PDEV *, SURFOBJ * );
BOOL   bRenderJNL( PDEV *, SURFOBJ * );

void   vEndPage( PDEV  * );

/*************************** Function Header *******************************
 * DrvStartDoc
 *      Function called to process START_DOC transaction.  That is,  the
 *      start of a document.  Thus it is necessary to send the initialisation
 *      data to the printer, as well as initialise internal data.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  16:07 on Sat 26 Jun 1993    -by-    Lindsay Harris   [lindsayh]
 *      Changed to set the RestartPDEV flag; initialisation done StartPage.
 *
 *  13:31 on Fri 13 Sep 1991    -by-    Lindsay Harris   [lindsayh]
 *      Changed yet again - engine now calls StartDoc/StartPage instead
 *
 *  13:51 on Fri 01 Mar 1991    -by-    Lindsay Harris   [lindsayh]
 *      Changed name & parameters to match DDI spec, ver 0.9
 *
 *  15:02 on Mon 21 Jan 1991    -by-    Lindsay Harris   [lindsayh]
 *      Created it,  based on control.c from Window's unidrv.
 *
 ***************************************************************************/

BOOL
DrvStartDoc( pso, pwszDocName, dwJobId )
SURFOBJ  *pso;          /* The surface of interest */
PWSTR     pwszDocName;  /* Document name */
DWORD     dwJobId;      /* The Spool Job Identification Number */
{
    /*
     *     Simply set the RestartPDEV bit in the UD_PDEV,  then this
     *  will force complete initialisation during DrvStartPage().
     */



    UNREFERENCED_PARAMETER( pwszDocName );
    UNREFERENCED_PARAMETER( dwJobId );


    ((UD_PDEV *)((PDEV *)(pso->dhpdev))->pUDPDev)->fMode |= PF_RESTART_PG;



    return  TRUE;
}

/************************** Function Header *********************************
 * DrvStartPage
 *      Called at the start of each new page.  So send the start a new page
 *      command,  and adjust any red tape that is related.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  16:16 on Sat 26 Jun 1993    -by-    Lindsay Harris   [lindsayh]
 *      Call bSendInitialisation to do the initialising - twice!
 *
 *  13:44 on Fri 13 Sep 1991    -by-    Lindsay Harris   [lindsayh]
 *      Created it,  from DrvStartPage of old DDI spec.
 *
 *****************************************************************************/

BOOL
DrvStartPage( pso )
SURFOBJ   *pso;                 /* The surface of interest!  */
{

    PDEV     *pPDev;
    UD_PDEV  *pUDPDev;             /* UNIDRV's pdev */

    RECTL     rcPage;           /* Page size: for erasing the surface */


    pPDev = (PDEV *)pso->dhpdev;


    if( pPDev->hjnl )
        EngStartBandPage( pso );


    pUDPDev = pPDev->pUDPDev;

    if( pUDPDev->fMode & PF_RESTART_PG )
    {
        /*
         *    There has been a DrvRestartPDev() call,  so we need to call
         *  bSendInitialisation to put the printer into the desired mode.
         */

        if( !bSendInitialisation( pPDev, TRUE ) )
            return   FALSE;
    }


    /*
     *    Most important step is to set the background colour throughout
     *  the bitmap.
     */


    rcPage.top = 0;
    rcPage.left = 0;
    rcPage.bottom = pUDPDev->szlPage.cy;
    rcPage.right = pUDPDev->szlPage.cx;

    EngEraseSurface( pso, &rcPage,
	            ((PAL_DATA *)(pPDev->pPalData))->iWhiteIndex );

    /*
     *  If this is NOT a page printer,  we need to initialise the position
     * sorting functions,  so that we print the page unidirectionally.
     */

    if( (pUDPDev->fMDGeneral & MD_SERIAL) && pUDPDev->cFonts )
    {
        if( !bCreatePS( pPDev ) )
	{
#if DBG
            DbgPrint( "Rasdd!DrvStartPage: Cannot create text sorting areas\n" );
#endif
	    return  FALSE;
        }

    }

    bSendInitialisation( pPDev, FALSE );           /* BEGIN_PAGE and beyond */

    /*
     *    Also set the clip region to the entire page.  The top entry
     *  is especially important for DrvTextOut,  where the value is added
     *  as required.   If not set to 0 here,  then no output appears after
     *  the first page!
     */

    pUDPDev->rcClipRgn.top = 0;
    pUDPDev->rcClipRgn.left = 0;
    pUDPDev->rcClipRgn.right = pUDPDev->szlPage.cx;
    pUDPDev->rcClipRgn.bottom = pUDPDev->szlPage.cy;

    /*
     *   Also set the current position to some illegal position, so that
     *  we make no assumptions about where we are.
     */

    XMoveto( pUDPDev, -1, MV_UPDATE );
    YMoveto( pUDPDev, -1, MV_UPDATE );

    if( pUDPDev->fMode & PF_SEIKO )
    {
        /*
         *   The Seiko printer requires sending the initialisation data
         *  at the start of each page.  So set the restart pdev bit,
         *  so that next time we come in here,  we send the data. Doing
         *  it this way means we do not do it twice for the first page.
         */
        pUDPDev->fMode |= PF_RESTART_PG;
    }

    return  TRUE;
}



/***************************** Function Header *****************************
 * bSendInitialisation
 *      Function to send the initialisation data to the printer.  Called
 *      during DrvStartPage,  so that we send the right stuff before
 *      any data is sent.
 *
 * RETURNS:
 *      TRUE/FALSE,  FALSE being failure to send data/Abort.
 *
 * HISTORY:
 *  Monday  November 29 1993    -by-    Norman Hendley   [normanh]
 *      Enabled setting of page lengths for custom papersizes
 *      Enabled Page Protection command
 *
 *  15:53 on Sat 26 Jun 1993    -by-    Lindsay Harris   [lindsayh]
 *      Moved from DrvStartDoc() to support SETCOPYCOUNT escape.
 *
 ***************************************************************************/

BOOL
bSendInitialisation( pPDev, bBeginPage )
PDEV   *pPDev;               /* The key to all our data */
BOOL    bBeginPage;          /* Set if only process BEGIN_PAGE and later */
{

    int       iCmd;

    const  short    *psID;

    DATAHDR  *pdh;

    UD_PDEV  *pUDPDev;


    /*
     *   Send the job initialisation data to the printer.  The commands required
     *  and the sequence in which they should be sent is included in the
     *  minidriver data.
     */


    pUDPDev = pPDev->pUDPDev;

    pdh = pUDPDev->pdh;

    if( pUDPDev->orgwStartDocCmdOrder == (OCD)NOT_USED )
    {
        /*  No data,  so use the default initialisation list.  */
        psID = pDefInitOrder;
    }
    else
    {
        /*  Minidriver has one,  so use its values.  */

        psID = (short *)((LPSTR)pdh + pdh->loHeap +
		pUDPDev->orgwStartDocCmdOrder);
    }


    /*
     *    If this is not the begin page initialisation, then we need to
     *  send only the latter part of the initialisation sequence.
     */

    if( !bBeginPage )
    {
        /*   Skip until we find PC_ORD_BEGINPAGE  */

        while( *psID && *psID != PC_ORD_BEGINPAGE )
                  ++psID;
    }


    for( ; *psID != 0; psID++ )
    {
        iCmd = -1;               /* Default nothing doing */

        switch( *psID )
        {
        case PC_ORD_ORIENTATION:
            if( pUDPDev->iOrient != DMORIENT_LANDSCAPE )
                iCmd = CMD_PC_PORTRAIT;  /* Portrait unless landscape */
            else
                iCmd = CMD_PC_LANDSCAPE;
            break;

        case PC_ORD_BEGINDOC:
            iCmd = CMD_PC_BEGIN_DOC;
            break;

        case PC_ORD_MULT_COPIES:
            WriteChannel( pUDPDev, CMD_PC_MULT_COPIES, pUDPDev->sCopies );

            break;

        case PC_ORD_DUPLEX:
            if( pUDPDev->sDuplex != DMDUP_SIMPLEX )
                iCmd = CMD_PC_DUPLEX_ON;

            break;

        case PC_ORD_DUPLEX_TYPE:
            if( pUDPDev->sDuplex == DMDUP_VERTICAL )
                iCmd = CMD_PC_DUPLEX_VERT;
            else
                if( pUDPDev->sDuplex == DMDUP_HORIZONTAL )
	iCmd = CMD_PC_DUPLEX_HORZ;
            break;


#if 0
        case PC_ORD_TEXTQUALITY:
            iCmd = CMD_TEXTQUALITY;
            break;
#endif

        case PC_ORD_PAPER_SOURCE:
            iCmd = CMD_PAPERSOURCE;
            break;

        case PC_ORD_PAPER_SIZE:
             WriteChannel( pUDPDev, CMD_PAPERSIZE,
	     ( (pUDPDev->iOrient == DMORIENT_LANDSCAPE ) ?
	     pUDPDev->pfPaper.ptPhys.x : pUDPDev->pfPaper.ptPhys.y )
	     * pUDPDev->Resolution.ptTextScale.y );
             break;

        case PC_ORD_PAPER_DEST:
            iCmd = CMD_PAPERDEST;
            break;

        case PC_ORD_RESOLUTION:
            iCmd = CMD_RES_SELECTRES;
            break;

        case PC_ORD_SETCOLORMODE:
            if( pUDPDev->Resolution.fDump & RES_DM_COLOR )
                iCmd = CMD_DC_GC_SETCOLORMODE;
            break;

        case PC_ORD_PAGEPROTECT:
            iCmd = CMD_PAGEPROTECT;
            break;

        case PC_ORD_BEGINPAGE:
            if( bBeginPage )
                return   TRUE;            /* All done in here */

            iCmd = CMD_PC_BEGIN_PAGE;
            break;

        default:                /* In case it's something we don't know */
            iCmd = -1;
            break;

        }

        if( iCmd >= 0 )
            WriteChannel( pUDPDev, iCmd );
    }

    /*
     *   If there is a palette to download,  now is the time to do it.
     */

    if( pUDPDev->fColorFormat & DC_SEND_PALETTE )
        vSeikoLoadPal( pPDev );


    /*   We want only to do this when needed.  */

    pUDPDev->fMode &= ~PF_RESTART_PG;


    return  TRUE;

}


/***************************** Function Header *****************************
 * DrvSendPage
 *      Called when the user has completed the drawing for this page.
 *      This function corresponds to DEVESC_NEWPAGE.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  13:17 on Fri 01 Mar 1991    -by-    Lindsay Harris   [lindsayh]
 *      Wrote it,  possibly subject to change.
 *
 ***************************************************************************/

BOOL
DrvSendPage( pso )
SURFOBJ   *pso;
{
    /*
     *    Not a great deal to do - we basically get the surface details
     *  and call the output functions.  If this is a surface,  then nothing
     *  needs to be done,  but journal files require more complex
     *  processing.
     */

    BOOL   bRet;                        /* Return value */

    PDEV  *pPDev;                       /* Access to all that is important */


    bRet = FALSE;

    pPDev = (PDEV *) pso->dhpdev;

#if     DBG

    if( pPDev->ulID != PDEV_ID )
    {
        SetLastError( ERROR_INVALID_PARAMETER );

        return  FALSE;
    }
#endif

    switch( pso->iType )
    {
    case  STYPE_BITMAP:         /* Engine managed bitmap */
        bRenderBM( pPDev, pso );
        vEndPage( pPDev );
        bRet = TRUE;

        break;

    case  STYPE_JOURNAL:        /* A journal file: means banding in effect */
        bRet = bRenderJNL( pPDev, pso );
        vEndPage( pPDev );
        break;

#if DBG
    case  STYPE_DEVICE:         /* Device surface - should not happen */
        DbgPrint( "Rasdd!DrvSendPage: STYPE_DEVICE should not happen\n" );
        break;

    default:
        DbgPrint( "Rasdd!DrvSendPage: UNKNOWN SURFACE TYPE\n" );
        break;
#endif
    }

    return  bRet;

}


/************************ Function Header ***********************************
 * bRenderBM
 *      Render an engine managed surface.  This means we do little, as the
 *      surface is passed to us,  and basically we pass it on to the usual
 *      rendering code.
 *
 * RETURNS:
 *      TRUE/FALSE,  whatever is returned from bRender()
 *
 * HISTROY:
 *  14:40 on Mon 01 Jun 1992    -by-    Lindsay Harris   [lindsayh]
 *      First incarnation, now that journalling is complete.
 *
 ****************************************************************************/

BOOL
bRenderBM( pPDev, pso )
PDEV      *pPDev;               /* Access to everything */
SURFOBJ   *pso;                 /* The surface: we want bitmap pointer */
{
    BOOL     bRet;              /* Return code */

    RENDER   RenderData;        /* Rendering data passed to bRender() */


    if( pPDev->pvRenderData == NULL )
        return  FALSE;

    RenderData = *(RENDER *)(pPDev->pvRenderData);


    bRet = FALSE;

    if( bRenderStartPage( pPDev ) )
    {
        bRet = bRender( pPDev, &RenderData, pso->sizlBitmap, pso->pvBits );
       ((RENDER *)(pPDev->pvRenderData))->plrWhite =  RenderData.plrWhite;

    }
          bRenderPageEnd( pPDev );


    return  bRet;
}



/************************ Function Header ***********************************
 * bRenderJNL
 *      Replays a journal file (the surface passed in) in bands to generate
 *      output for the printer.  As a band is completed,  it is passed to
 *      bRender() to send off to the printer.
 *
 * RETURNS:
 *      TRUE/FALSE; FALSE for a failure of journalling or rendering.
 *
 * HISTROY:
 *  14:43 on Mon 01 Jun 1992    -by-    Lindsay Harris   [lindsayh]
 *      First incarnation, now that journalling is complete.
 *
 ****************************************************************************/

BOOL
bRenderJNL( pPDev, pso )
PDEV      *pPDev;               /* Access to everything */
SURFOBJ   *pso;                 /* The surface: we want bitmap pointer */
{

    int       iSwitch;          /* Controls how we change banding rectangle */
    BOOL      bRet;             /* Ultimate return code */
    BOOL      bMore;            /* Set when more to do */
    SIZEL     sizl;
    FLONG     flHooks;          /* Functions we hook */

    UD_PDEV  *pUDPDev;          /* Unidrive'e PDEV */

    SURFOBJ  *psoBand;          /* The bitmap surface */

    RENDER    RenderData;       /* For rendering operations */

#define             SW_DOWN  0              /* No rotation: top to bottom */
#define             SW_LTOR  1              /* Dot matrix rotation */
#define             SW_RTOL  2              /* LaserJet style, CCW rotation */

#if PRINT_INFO
    DbgPrint( "rasdd:  playing back journal file\n" );
#endif

    bRet = FALSE;               /* Default return */
    pUDPDev = pPDev->pUDPDev;   /* For our convenience */

    sizl = pUDPDev->szlBand;    /* Size of existing bitmap */


    if( pPDev->pvRenderData == NULL )
        return  FALSE;          /* Should not happen, nasty if it does */

    if( !bRenderStartPage( pPDev ) )
        return   FALSE;

    RenderData = *((RENDER *)pPDev->pvRenderData);

    if( pUDPDev->fMode & PF_ROTATE )
    {
        /*  We do the rotation, but can be one of two ways! */

        pUDPDev->rcClipRgn.top = 0;
        pUDPDev->rcClipRgn.bottom = pUDPDev->szlPage.cy;

        if( pUDPDev->fMode & PF_CCW_ROTATE )
        {
            /*   LaserJet style rotation */

            pUDPDev->rcClipRgn.left = pUDPDev->szlPage.cx - sizl.cx;
            pUDPDev->rcClipRgn.right = pUDPDev->szlPage.cx;

            iSwitch = SW_RTOL;
        }
        else
        {
            /*  Dot matrix style rotation */

            pUDPDev->rcClipRgn.left = 0;
            pUDPDev->rcClipRgn.right = sizl.cx;

            iSwitch = SW_LTOR;
        }
    }
    else
    {
        /*   Go as is! */
        pUDPDev->rcClipRgn.top = 0;
        pUDPDev->rcClipRgn.left = 0;
        pUDPDev->rcClipRgn.right = sizl.cx;
        pUDPDev->rcClipRgn.bottom = sizl.cy;

        iSwitch = SW_DOWN;
    }


    /*
     *     Hook the functions we process.  We need not hook the textout
     *  function if we have no device fonts,  ALTHOUGH we SHOULD do this
     *  if we can download fonts.
     */

    flHooks = HOOK_STRETCHBLT | HOOK_BITBLT;
    if( pUDPDev->cFonts )
        flHooks |= HOOK_TEXTOUT;

    if( !EngAssociateSurface( (HSURF)pPDev->hbm, pPDev->hdev, flHooks ) )
    {
#if DBG
        DbgPrint( "RasDD!bRenderJNL: EngAssociateSurface() fails\n" );
#endif

        return  FALSE;
    }

    psoBand = EngLockSurface( (HSURF)pPDev->hbm );

    bMore = TRUE;
    do
    {
        /*   First step: replay the journal file to this band! */

        if( !(bRet = EngPlayJournal( psoBand, pso, &pUDPDev->rcClipRgn )) )
        {
            break;
        }

        pUDPDev->pso = pso;        /* May change during EngPlayJournal */
        if( !(bRet = bRender( pPDev, &RenderData, sizl, psoBand->pvBits )) )
        {
            /*   Bad news:  quit now */
            break;
        }

        switch( iSwitch )
        {
        case  SW_DOWN:             /* Moving down the page */
            pUDPDev->rcClipRgn.top += sizl.cy;
            pUDPDev->rcClipRgn.bottom += sizl.cy;

            /*  Make sure we do not run off the bottom  */
            bMore = pUDPDev->rcClipRgn.top < pUDPDev->szlPage.cy;

            if( pUDPDev->rcClipRgn.bottom > pUDPDev->szlPage.cy )
            {
                /*   Less to do,  so the partial band size */
                pUDPDev->rcClipRgn.bottom = pUDPDev->szlPage.cy;
                sizl.cy = pUDPDev->szlPage.cy - pUDPDev->rcClipRgn.top;
            }

            break;

        case  SW_RTOL:             /* LaserJet style: right to left */
            pUDPDev->rcClipRgn.left -= sizl.cx;
            pUDPDev->rcClipRgn.right -= sizl.cx;

            bMore = pUDPDev->rcClipRgn.right > 0;

            if( pUDPDev->rcClipRgn.left < 0 )
            {
                /*  Final band,  so reduce the size */
                pUDPDev->rcClipRgn.left = 0;
                sizl.cx = pUDPDev->rcClipRgn.right;
            }

            break;

        case  SW_LTOR:             /* Dot matrix: left to right */
            pUDPDev->rcClipRgn.left += sizl.cx;
            pUDPDev->rcClipRgn.right += sizl.cx;

            bMore = pUDPDev->rcClipRgn.left < pUDPDev->szlPage.cx;

            if( pUDPDev->rcClipRgn.right > pUDPDev->szlPage.cx )
            {
                /*   Final band - limit the size */
                pUDPDev->rcClipRgn.right = pUDPDev->szlPage.cx;
                sizl.cx = pUDPDev->szlPage.cx - pUDPDev->rcClipRgn.left;
            }

            break;
        }

    } while( bRet && bMore );


    bRenderPageEnd( pPDev );        /* Clean up any dangling rendering ops */


    EngUnlockSurface( psoBand );


    return   bRet;
}

/***************************** Function Header *****************************
 * vEndPage
 *      Called when a page has been rendered.  Mainly used to complete the
 *      page printing,  either by form feeding it from the printer,
 *      or using graphics commands to move to the bottom of page.
 *
 * RETURNS:
 *      Nothing
 *
 * HISTORY:
 *  15:35 on Mon 07 Oct 1991    -by-    Lindsay Harris   [lindsayh]
 *      Insane logic about when to use ptRes.x vs ptRes.y
 *
 *  15:05 on Mon 21 Jan 1991    -by-    Lindsay Harris   [lindsayh]
 *      Based on Window's UNIDRV code.
 *
 **************************************************************************/

void
vEndPage( pPDev )
PDEV  *pPDev;
{

    UD_PDEV   *lpdv;


    lpdv = pPDev->pUDPDev;


    /*
     *   Eject the page or move to the bottom,  as appropriate.  If
     * available,  use the form feed character,  else use graphics mode.
     */

    if( lpdv->fMode & PF_USE_FF )
        WriteChannel( lpdv, CMD_CM_FF );
    else
    {
        /*
         *  The pfPaper.ptRes data has the X, Y coordinates swapped if
         * this page is printed in landscape.  SO,  if we are in landscape,
         * we need to unswap them.
         */

        int        yEnd;                /* Last scan line on page */


        yEnd = lpdv->iOrient == DMORIENT_LANDSCAPE ?
	             lpdv->pfPaper.ptRes.x : lpdv->pfPaper.ptRes.y;

        YMoveto( lpdv,  yEnd, MV_GRAPHICS );


    }

    WriteChannel( lpdv, CMD_PC_ENDPAGE );

    FlushSpoolBuf( lpdv );

    if( pPDev->pPSHeader )
        vFreePS( pPDev );               /* Done with this page */

    return;
}



/************************ Function Header ***********************************
 * DrvEndDoc
 *      The end of the document - simply flush the buffer!
 *
 * RETURNS:
 *      TRUE
 *
 * HISTORY:
 *  13:48 on Fri 13 Sep 1991    -by-    Lindsay Harris   [lindsayh]
 *      Created for new DDI spec + spooler flushing problems.
 *
 *****************************************************************************/

BOOL
DrvEndDoc( pso, fl )
SURFOBJ  *pso;
FLONG     fl;
{

    /*
     *   Grab hold of UNIDRV's pdev structure and flush the buffer!
     */

    UD_PDEV   *lpdv;

    lpdv = ((PDEV *)(pso->dhpdev))->pUDPDev;

    if( fl & ED_ABORTDOC )
        vEndPage( (PDEV *)pso->dhpdev );

    /*  Send the END DOC type commands too!  */

    WriteChannel( lpdv, CMD_PC_ENDDOC );

    FlushSpoolBuf( lpdv );


    vFreeDL( (PDEV *)pso->dhpdev );


    return  TRUE;
}
