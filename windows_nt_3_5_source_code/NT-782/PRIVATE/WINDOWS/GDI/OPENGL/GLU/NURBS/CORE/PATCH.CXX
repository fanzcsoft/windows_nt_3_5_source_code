/**************************************************************************
 *									  *
 * 		 Copyright (C) 1992, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by ederal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/*
 * patch.c++ - $Revision: 1.1 $
 * 	Derrick Burns - 1992
 */

#include "glimport.h"
#include "mystdio.h"
#include "myassert.h"
#include "mymath.h"
#include "mystring.h"
#include "patch.h"
#include "mapdesc.h"
#include "quilt.h"
#include "nurbscon.h"


/*--------------------------------------------------------------------------
 * Patch - copy patch from quilt and transform control points
 *--------------------------------------------------------------------------
 */

Patch::Patch( Quilt_ptr geo, REAL *pta, REAL *ptb, Patch *n )
{
    mapdesc = geo->mapdesc;
    cullval = mapdesc->isCulling() ? CULL_ACCEPT : CULL_TRIVIAL_ACCEPT;
    notInBbox = mapdesc->isBboxSubdividing() ? 1 : 0;
    needsSampling = mapdesc->isRangeSampling() ? 1 : 0;
    pspec[0].order = geo->qspec[0].order;
    pspec[1].order = geo->qspec[1].order;
    pspec[0].stride = pspec[1].order * MAXCOORDS;
    pspec[1].stride = MAXCOORDS;

    /* transform control points to sampling and culling spaces */
    REAL *ps  = geo->cpts;
    geo->select( pta, ptb );
    ps += geo->qspec[0].offset;
    ps += geo->qspec[1].offset;
    ps += geo->qspec[0].index * geo->qspec[0].order * geo->qspec[0].stride;
    ps += geo->qspec[1].index * geo->qspec[1].order * geo->qspec[1].stride;

    if( needsSampling ) {
	mapdesc->xformSampling( ps, geo->qspec[0].order, geo->qspec[0].stride, 
				geo->qspec[1].order, geo->qspec[1].stride,
			        spts, pspec[0].stride, pspec[1].stride );
    }

    if( cullval == CULL_ACCEPT  ) {
	mapdesc->xformCulling( ps, geo->qspec[0].order, geo->qspec[0].stride, 
			       geo->qspec[1].order, geo->qspec[1].stride,
			       cpts, pspec[0].stride, pspec[1].stride ); 
    }
    
    if( notInBbox ) {
	mapdesc->xformBounding( ps, geo->qspec[0].order, geo->qspec[0].stride, 
			       geo->qspec[1].order, geo->qspec[1].stride,
			       bpts, pspec[0].stride, pspec[1].stride ); 
    }
    
    /* set scale range */
    pspec[0].range[0] = geo->qspec[0].breakpoints[geo->qspec[0].index];
    pspec[0].range[1] = geo->qspec[0].breakpoints[geo->qspec[0].index+1];
    pspec[0].range[2] = pspec[0].range[1] - pspec[0].range[0];

    pspec[1].range[0] = geo->qspec[1].breakpoints[geo->qspec[1].index];
    pspec[1].range[1] = geo->qspec[1].breakpoints[geo->qspec[1].index+1];
    pspec[1].range[2] = pspec[1].range[1] - pspec[1].range[0];

    // may need to subdivide to match range of sub-patch
    if( pspec[0].range[0] != pta[0] ) {
	assert( pspec[0].range[0] < pta[0] );
	Patch lower( *this, 0, pta[0], 0 );
	*this = lower;
    }

    if( pspec[0].range[1] != ptb[0] ) {
	assert( pspec[0].range[1] > ptb[0] );
	Patch upper( *this, 0, ptb[0], 0 );
    }

    if( pspec[1].range[0] != pta[1] ) {
	assert( pspec[1].range[0] < pta[1] );
	Patch lower( *this, 1, pta[1], 0 );
	*this = lower;
    }

    if( pspec[1].range[1] != ptb[1] ) {
	assert( pspec[1].range[1] > ptb[1] );
	Patch upper( *this, 1, ptb[1], 0 );
    }
    checkBboxConstraint();
    next = n;
}

/*--------------------------------------------------------------------------
 * Patch - subdivide a patch along an isoparametric line
 *--------------------------------------------------------------------------
 */

Patch::Patch( Patch& upper, int param, REAL value, Patch *n )
{
    Patch& lower = *this;

    lower.cullval = upper.cullval;
    lower.mapdesc = upper.mapdesc;
    lower.notInBbox = upper.notInBbox;
    lower.needsSampling = upper.needsSampling;
    lower.pspec[0].order = upper.pspec[0].order;
    lower.pspec[1].order = upper.pspec[1].order;
    lower.pspec[0].stride = upper.pspec[0].stride;
    lower.pspec[1].stride = upper.pspec[1].stride;
    lower.next = n;

    /* reset scale range */
    switch( param ) {
	case 0: {
    	    REAL d = (value-upper.pspec[0].range[0]) / upper.pspec[0].range[2];
	    if( needsSampling )
                mapdesc->subdivide( upper.spts, lower.spts, d, pspec[1].order,
                        pspec[1].stride, pspec[0].order, pspec[0].stride );

    	    if( cullval == CULL_ACCEPT ) 
	        mapdesc->subdivide( upper.cpts, lower.cpts, d, pspec[1].order,
                        pspec[1].stride, pspec[0].order, pspec[0].stride );

    	    if( notInBbox ) 
	        mapdesc->subdivide( upper.bpts, lower.bpts, d, pspec[1].order,
                        pspec[1].stride, pspec[0].order, pspec[0].stride );
	    
            lower.pspec[0].range[0] = upper.pspec[0].range[0];
            lower.pspec[0].range[1] = value;
    	    lower.pspec[0].range[2] = value - upper.pspec[0].range[0];
            upper.pspec[0].range[0] = value;
    	    upper.pspec[0].range[2] = upper.pspec[0].range[1] - value;

            lower.pspec[1].range[0] = upper.pspec[1].range[0];
            lower.pspec[1].range[1] = upper.pspec[1].range[1];
    	    lower.pspec[1].range[2] = upper.pspec[1].range[2];
	    break;
	}
	case 1: {
    	    REAL d = (value-upper.pspec[1].range[0]) / upper.pspec[1].range[2];
	    if( needsSampling )
	        mapdesc->subdivide( upper.spts, lower.spts, d, pspec[0].order,
                        pspec[0].stride, pspec[1].order, pspec[1].stride );
    	    if( cullval == CULL_ACCEPT ) 
	        mapdesc->subdivide( upper.cpts, lower.cpts, d, pspec[0].order,
                        pspec[0].stride, pspec[1].order, pspec[1].stride );
    	    if( notInBbox ) 
	        mapdesc->subdivide( upper.bpts, lower.bpts, d, pspec[0].order,
                        pspec[0].stride, pspec[1].order, pspec[1].stride );
            lower.pspec[0].range[0] = upper.pspec[0].range[0];
            lower.pspec[0].range[1] = upper.pspec[0].range[1];
    	    lower.pspec[0].range[2] = upper.pspec[0].range[2];

            lower.pspec[1].range[0] = upper.pspec[1].range[0];
            lower.pspec[1].range[1] = value;
    	    lower.pspec[1].range[2] = value - upper.pspec[1].range[0];
            upper.pspec[1].range[0] = value;
    	    upper.pspec[1].range[2] = upper.pspec[1].range[1] - value;
	    break;
	}
    }

    // inherit bounding box
    if( mapdesc->isBboxSubdividing() && ! notInBbox )
	memcpy( lower.bb, upper.bb, sizeof( bb ) );
	    
    lower.checkBboxConstraint();
    upper.checkBboxConstraint();
}

/*--------------------------------------------------------------------------
 * clamp - clamp the sampling rate to a given maximum
 *--------------------------------------------------------------------------
 */

void
Patch::clamp( void )
{
    if( mapdesc->clampfactor != (REAL)N_NOCLAMPING ) {
	pspec[0].clamp( mapdesc->clampfactor );
	pspec[1].clamp( mapdesc->clampfactor );
    }
}

void 
Patchspec::clamp( REAL clampfactor )
{
    if( sidestep[0] < minstepsize )
        sidestep[0] = clampfactor * minstepsize;
    if( sidestep[1] < minstepsize )
        sidestep[1] = clampfactor * minstepsize;
    if( stepsize < minstepsize )
        stepsize = clampfactor * minstepsize;
}

void 
Patch::checkBboxConstraint( void )
{
    if( notInBbox && 
	mapdesc->bboxTooBig( bpts, pspec[0].stride, pspec[1].stride,
				   pspec[0].order, pspec[1].order, bb ) != 1 ) {
	notInBbox = 0;
    }
}

void
Patch::bbox( void )
{
    if( mapdesc->isBboxSubdividing() )
	mapdesc->surfbbox( bb );
}

/*--------------------------------------------------------------------------
 * getstepsize - compute the sampling density across the patch
 * 		and determine if patch needs to be subdivided
 *--------------------------------------------------------------------------
 */

void
Patch::getstepsize( void )
{
    pspec[0].needsSubdivision = pspec[1].needsSubdivision = 0;

    if( mapdesc->isConstantSampling() ) {
	// fixed number of samples per patch in each direction
	// maxsrate is number of s samples per patch
	// maxtrate is number of t samples per patch
        pspec[0].getstepsize( mapdesc->maxsrate );
        pspec[1].getstepsize( mapdesc->maxtrate );

    } else if( mapdesc->isDomainSampling() ) {
	// maxsrate is number of s samples per unit s length of domain
	// maxtrate is number of t samples per unit t length of domain
        pspec[0].getstepsize( mapdesc->maxsrate * pspec[0].range[2] );
        pspec[1].getstepsize( mapdesc->maxtrate * pspec[1].range[2] );

    } else if( ! needsSampling ) {
	pspec[0].singleStep();
	pspec[1].singleStep();
    } else {
	// upper bound on path length between sample points
        REAL tmp[MAXORDER][MAXORDER][MAXCOORDS];
	const int trstride = sizeof(tmp[0]) / sizeof(REAL);
	const int tcstride = sizeof(tmp[0][0]) / sizeof(REAL); 

	assert( pspec[0].order <= MAXORDER );
    
	/* points have been transformed, therefore they are homogeneous */

	int val = mapdesc->project( spts, pspec[0].stride, pspec[1].stride, 
		 &tmp[0][0][0], trstride, tcstride, 
		 pspec[0].order, pspec[1].order ); 
        if( val == 0 ) {
	    // control points cross infinity, therefore partials are undefined
            pspec[0].getstepsize( mapdesc->maxsrate );
            pspec[1].getstepsize( mapdesc->maxtrate );
        } else {
            REAL t = mapdesc->getProperty( N_PIXEL_TOLERANCE );
	    pspec[0].minstepsize = ( mapdesc->maxsrate > (REAL)0.0 ) ? 
			(pspec[0].range[2] / mapdesc->maxsrate) : (REAL)0.0;
	    pspec[1].minstepsize = ( mapdesc->maxtrate > (REAL)0.0 ) ? 
			(pspec[1].range[2] / mapdesc->maxtrate) : (REAL)0.0;
	    if( mapdesc->isParametricDistanceSampling() ) {
		REAL ssv[2], ttv[2];
		REAL ss = mapdesc->calcPartialVelocity( ssv, &tmp[0][0][0], trstride, tcstride, pspec[0].order, pspec[1].order, 2, 0, pspec[0].range[2], pspec[1].range[2], 0 );
		REAL st = mapdesc->calcPartialVelocity(   0, &tmp[0][0][0], trstride, tcstride, pspec[0].order, pspec[1].order, 1, 1, pspec[0].range[2], pspec[1].range[2], -1 );
		REAL tt = mapdesc->calcPartialVelocity( ttv, &tmp[0][0][0], trstride, tcstride, pspec[0].order, pspec[1].order, 0, 2, pspec[0].range[2], pspec[1].range[2], 1 );
    
		if( ss != (REAL)0.0 && tt != (REAL)0.0 ) {
		    /* printf( "ssv[0] %g ssv[1] %g ttv[0] %g ttv[1] %g\n", 
			ssv[0], ssv[1], ttv[0], ttv[1] ); */
		    REAL ttq = ::sqrtf( (float) ss );
		    REAL ssq = ::sqrtf( (float) tt );
		    REAL ds = ::sqrtf( 4 * t * ttq / ( ss * ttq + st * ssq ) );
		    REAL dt = ::sqrtf( 4 * t * ssq / ( tt * ssq + st * ttq ) );
		    pspec[0].stepsize = ( ds < pspec[0].range[2] ) ? ds : pspec[0].range[2];
		    REAL scutoff = (REAL)2.0 * t / ( pspec[0].range[2] * pspec[0].range[2]);
		    pspec[0].sidestep[0] = (ssv[0] > scutoff) ? ::sqrtf( 2.0 * t / ssv[0] ) : pspec[0].range[2];
		    pspec[0].sidestep[1] = (ssv[1] > scutoff) ? ::sqrtf( 2.0 * t / ssv[1] ) : pspec[0].range[2];
    
		    pspec[1].stepsize = ( dt < pspec[1].range[2] ) ? dt : pspec[1].range[2];
		    REAL tcutoff = (REAL)2.0 * t / ( pspec[1].range[2] * pspec[1].range[2]);
		    pspec[1].sidestep[0] = (ttv[0] > tcutoff) ? ::sqrtf( 2.0 * t / ttv[0] ) : pspec[1].range[2];
		    pspec[1].sidestep[1] = (ttv[1] > tcutoff) ? ::sqrtf( 2.0 * t / ttv[1] ) : pspec[1].range[2];
		} else if( ss != (REAL)0.0 ) {
		    REAL x = pspec[1].range[2] * st;
		    REAL ds = ( ::sqrtf( x * x + 8.0 * t * ss ) - x ) / ss;
		    pspec[0].stepsize = ( ds < pspec[0].range[2] ) ? ds : pspec[0].range[2];
		    REAL scutoff = (REAL)2.0 * t / ( pspec[0].range[2] * pspec[0].range[2]);
		    pspec[0].sidestep[0] = (ssv[0] > scutoff) ? ::sqrtf( 2.0 * t / ssv[0] ) : pspec[0].range[2];
		    pspec[0].sidestep[1] = (ssv[1] > scutoff) ? ::sqrtf( 2.0 * t / ssv[1] ) : pspec[0].range[2];
		    pspec[1].singleStep();
		} else if( tt != (REAL)0.0 ) {
		    REAL x = pspec[0].range[2] * st;
		    REAL dt = ( ::sqrtf( x * x + 8.0 * t * tt ) - x )  / tt;
		    pspec[0].singleStep();
		    REAL tcutoff = (REAL)2.0 * t / ( pspec[1].range[2] * pspec[1].range[2]);
		    pspec[1].stepsize = ( dt < pspec[1].range[2] ) ? dt : pspec[1].range[2];
		    pspec[1].sidestep[0] = (ttv[0] > tcutoff) ? ::sqrtf( 2.0 * t / ttv[0] ) : pspec[1].range[2];
		    pspec[1].sidestep[1] = (ttv[1] > tcutoff) ? ::sqrtf( 2.0 * t / ttv[1] ) : pspec[1].range[2];
		} else {
		    if( (REAL)4.0 * t  > st * pspec[0].range[2] * pspec[1].range[2] ) {
			pspec[0].singleStep();
			pspec[1].singleStep();
		    } else {
			REAL area = (REAL)4.0 * t / st;
			REAL ds = ::sqrtf( area * pspec[0].range[2] / pspec[1].range[2] );
			REAL dt = ::sqrtf( area * pspec[1].range[2] / pspec[0].range[2] );
			pspec[0].stepsize = ( ds < pspec[0].range[2] ) ? ds : pspec[0].range[2];
			pspec[0].sidestep[0] = pspec[0].range[2];
			pspec[0].sidestep[1] = pspec[0].range[2];
	
			pspec[1].stepsize = ( dt < pspec[1].range[2] ) ? dt : pspec[1].range[2];
			pspec[1].sidestep[0] = pspec[1].range[2];
			pspec[1].sidestep[1] = pspec[1].range[2];
		    }
		}
	    } else if( mapdesc->isPathLengthSampling() ) {
		// t is upper bound on path length
		REAL msv[2], mtv[2];
		REAL ms = mapdesc->calcPartialVelocity( msv, &tmp[0][0][0], trstride, tcstride, pspec[0].order, pspec[1].order, 1, 0, pspec[0].range[2], pspec[1].range[2], 0 );
		REAL mt = mapdesc->calcPartialVelocity( mtv, &tmp[0][0][0], trstride, tcstride, pspec[0].order, pspec[1].order, 0, 1, pspec[0].range[2], pspec[1].range[2], 1 );
		if( ms != (REAL)0.0 ) {
		    if( mt != (REAL)0.0 ) {
		    REAL d = t / ( ms * ms + mt * mt );
		    REAL ds = mt * d;
		    REAL dt = ms * d;
			pspec[0].stepsize = ( ds < pspec[0].range[2] ) ? ds : pspec[0].range[2];
			pspec[0].sidestep[0] = ( msv[0] * pspec[0].range[2] > t ) ? (t / msv[0]) : pspec[0].range[2];
			pspec[0].sidestep[1] = ( msv[1] * pspec[0].range[2] > t ) ? (t / msv[1]) : pspec[0].range[2];
	
			pspec[1].stepsize = ( dt < pspec[1].range[2] ) ? dt : pspec[1].range[2];
			pspec[1].sidestep[0] = ( mtv[0] * pspec[1].range[2] > t ) ? (t / mtv[0]) : pspec[1].range[2];
			pspec[1].sidestep[1] = ( mtv[1] * pspec[1].range[2] > t ) ? (t / mtv[1]) : pspec[1].range[2];
		    } else {
			pspec[0].stepsize = ( t < ms * pspec[0].range[2] ) ? (t / ms) : pspec[0].range[2];
			pspec[0].sidestep[0] = ( msv[0] * pspec[0].range[2] > t ) ? (t / msv[0]) : pspec[0].range[2];
			pspec[0].sidestep[1] = ( msv[1] * pspec[0].range[2] > t ) ? (t / msv[1]) : pspec[0].range[2];
	
			pspec[1].singleStep();
		    }
		} else {
		    if( mt != (REAL)0.0 ) {
			pspec[0].singleStep();
    
			pspec[1].stepsize = ( t < mt * pspec[1].range[2] ) ? (t / mt) : pspec[1].range[2];
			pspec[1].sidestep[0] = ( mtv[0] * pspec[1].range[2] > t ) ? (t / mtv[0]) : pspec[1].range[2];
			pspec[1].sidestep[1] = ( mtv[1] * pspec[1].range[2] > t ) ? (t / mtv[1]) : pspec[1].range[2];
		    } else {
			pspec[0].singleStep();
			pspec[1].singleStep();
		    }
		}
	    } else if( mapdesc->isSurfaceAreaSampling() ) {
		// t is the square root of area
		REAL msv[2], mtv[2];
		REAL ms = mapdesc->calcPartialVelocity( msv, &tmp[0][0][0], trstride, tcstride, pspec[0].order, pspec[1].order, 1, 0, pspec[0].range[2], pspec[1].range[2], 0 );
		REAL mt = mapdesc->calcPartialVelocity( mtv, &tmp[0][0][0], trstride, tcstride, pspec[0].order, pspec[1].order, 0, 1, pspec[0].range[2], pspec[1].range[2], 1 );
		if( ms != (REAL)0.0 &&  mt != (REAL)0.0 ) {
			REAL d = (REAL)1.0 / (ms * mt);
			t *= (REAL)M_SQRT2;
			REAL ds = t * ::sqrtf( d * pspec[0].range[2] / pspec[1].range[2] );
			REAL dt = t * ::sqrtf( d * pspec[1].range[2] / pspec[0].range[2] );
			pspec[0].stepsize = ( ds < pspec[0].range[2] ) ? ds : pspec[0].range[2];
			pspec[0].sidestep[0] = ( msv[0] * pspec[0].range[2] > t ) ? (t / msv[0]) : pspec[0].range[2];
			pspec[0].sidestep[1] = ( msv[1] * pspec[0].range[2] > t ) ? (t / msv[1]) : pspec[0].range[2];
	
			pspec[1].stepsize = ( dt < pspec[1].range[2] ) ? dt : pspec[1].range[2];
			pspec[1].sidestep[0] = ( mtv[0] * pspec[1].range[2] > t ) ? (t / mtv[0]) : pspec[1].range[2];
			pspec[1].sidestep[1] = ( mtv[1] * pspec[1].range[2] > t ) ? (t / mtv[1]) : pspec[1].range[2];
		} else {
		    pspec[0].singleStep();
		    pspec[1].singleStep();
		}
	    } else {
		pspec[0].singleStep();
		pspec[1].singleStep();
	    }
	}
    }

/*
    printf( "sidesteps %g %g %g %g, stepsize %g %g\n",
	pspec[0].sidestep[0], pspec[0].sidestep[1],
	pspec[1].sidestep[0], pspec[1].sidestep[1],
	pspec[0].stepsize, pspec[1].stepsize );
*/


    if( mapdesc->minsavings != (REAL)N_NOSAVINGSSUBDIVISION ) {
	REAL savings = (REAL)1./(pspec[0].stepsize * pspec[1].stepsize) ;
	savings-= ((REAL)2./( pspec[0].sidestep[0] + pspec[0].sidestep[1] )) * 
		  ((REAL)2./( pspec[1].sidestep[0] + pspec[1].sidestep[1] ));
    
	savings *= pspec[0].range[2] * pspec[1].range[2];
	if( savings > mapdesc->minsavings ) {
	    pspec[0].needsSubdivision = pspec[1].needsSubdivision = 1;
	}
    }

    if( pspec[0].stepsize < pspec[0].minstepsize )  pspec[0].needsSubdivision =  1;
    if( pspec[1].stepsize < pspec[1].minstepsize )  pspec[1].needsSubdivision =  1;
    needsSampling = (needsSampling ? needsSamplingSubdivision() : 0);
}

void
Patchspec::singleStep()
{
    stepsize =  sidestep[0] =  sidestep[1] = range[2];
}

void 
Patchspec::getstepsize( REAL max ) // max is number of samples for entire patch
{
    stepsize = ( max >= (REAL)1.0 ) ? range[2] / max : range[2];
    sidestep[0]	=  sidestep[1] = minstepsize = stepsize;
}

int
Patch::needsSamplingSubdivision( void )
{
    return (pspec[0].needsSubdivision || pspec[1].needsSubdivision) ? 1 : 0;
}

int
Patch::needsNonSamplingSubdivision( void )
{
    return notInBbox;
}

int
Patch::needsSubdivision( int param )
{
    return pspec[param].needsSubdivision;
}

int
Patch::cullCheck( void )
{
    if( cullval == CULL_ACCEPT ) 
	cullval = mapdesc->cullCheck( cpts, pspec[0].order,  pspec[0].stride,
					    pspec[1].order,  pspec[1].stride );
    return cullval;
}

