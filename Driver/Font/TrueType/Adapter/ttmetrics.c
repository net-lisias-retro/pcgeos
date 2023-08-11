/***********************************************************************
 *
 *                      Copyright FreeGEOS-Project
 *
 * PROJECT:	  FreeGEOS
 * MODULE:	  TrueType font driver
 * FILE:	  ttadapter.c
 *
 * AUTHOR:	  Jirka Kunze: December 23 2022
 *
 * REVISION HISTORY:
 *	Date	  Name	    Description
 *	----	  ----	    -----------
 *	12/23/22  JK	    Initial version
 *
 * DESCRIPTION:
 *	Definition of driver function DR_FONT_CHAR_METRICS.
 ***********************************************************************/

#include "ttadapter.h"
#include "ttmetrics.h"
#include "ttadapter.h"
#include "freetype.h"
#include "ttcharmapper.h"
#include <ec.h>


static void CalcTransformMatrix( TextStyle         stylesToImplement,
                                 TransformMatrix*  transMatrix );

/********************************************************************
 *                      TrueType_Char_Metrics
 ********************************************************************
 * SYNOPSIS:	  Return character metrics information in document coords.
 * 
 * PARAMETERS:    character             Character to get metrics of.
 *                info                  Info to return (GCM_info).
 *                *fontInfo              
 *                *outlineData
 *                stylesToImplement
 *                result                Pointer in wich the result will 
 *                                      stored.
 * 
 * RETURNS:       void
 * 
 * SIDE EFFECTS:  none
 * 
 * STRATEGY:      
 * 
 * REVISION HISTORY:
 *      Date      Name      Description
 *      ----      ----      -----------
 *      23/12/22  JK        Initial Revision
 * 
 *******************************************************************/
void _pascal TrueType_Char_Metrics( 
                                   word                 character, 
                                   GCM_info             info, 
                                   const FontInfo*      fontInfo,
	                           const OutlineEntry*  outlineEntry, 
                                   TextStyle            stylesToImplement,
                                   WWFixedAsDWord       pointSize,
                                   dword*               result,
                                   MemHandle            varBlock ) 
{
        TrueTypeOutlineEntry*  trueTypeOutline;
        TransformMatrix        transMatrix;
        word                   charIndex;
        TrueTypeVars*          trueTypeVars;


EC(     ECCheckBounds( (void*)fontInfo ) );
EC(     ECCheckBounds( (void*)outlineEntry ) );
EC(     ECCheckMemHandle( varBlock ) );
EC(     ECCheckStack() );


        /* get trueTypeVar block */
        trueTypeVars = MemLock( varBlock );
EC(     ECCheckBounds( (void*)trueTypeVars ) );

        trueTypeOutline = LMemDerefHandles( MemPtrToHandle( (void*)fontInfo ), outlineEntry->OE_handle );

        if( TrueType_Lock_Face(trueTypeVars, trueTypeOutline) )
                goto Fail;

        CalcTransformMatrix( stylesToImplement, &transMatrix );

        // get TT char index
        charIndex = TT_Char_Index( CHAR_MAP, GeosCharToUnicode( character ) );

        // load glyph
        TT_New_Glyph( FACE, &GLYPH );

        // transform glyphs outline
        TT_Get_Glyph_Outline( GLYPH, &OUTLINE );
        TT_Transform_Outline( &OUTLINE, &transMatrix.TM_matrix );
        TT_Translate_Outline( &OUTLINE, 0, WWFIXEDASDWORD_TO_FIXED26DOT6( transMatrix.TM_scriptY ) );

        // scale glyph
        TT_Set_Instance_CharSize( INSTANCE, ( pointSize >> 10 ) );
        TT_Get_Instance_Metrics( INSTANCE, &INSTANCE_METRICS );

        // get metrics
        TT_Get_Glyph_Metrics( GLYPH, &GLYPH_METRICS );

        switch( info )
        {
                case GCMI_MIN_X:
                case GCMI_MIN_X_ROUNDED:
                        *result = SCALE_WORD( GLYPH_BBOX.xMin, INSTANCE_METRICS.x_scale );
                        break;
                case GCMI_MIN_Y:
                case GCMI_MIN_Y_ROUNDED:
                        *result = SCALE_WORD( GLYPH_BBOX.yMin, INSTANCE_METRICS.y_scale );
                        break;
                case GCMI_MAX_X:
                case GCMI_MAX_X_ROUNDED:
                        *result = SCALE_WORD( GLYPH_BBOX.xMax, INSTANCE_METRICS.x_scale );
                        break;
                case GCMI_MAX_Y:
                case GCMI_MAX_Y_ROUNDED:
                        *result = SCALE_WORD( GLYPH_BBOX.yMax, INSTANCE_METRICS.y_scale );
                        break;
        }

        TT_Done_Glyph( GLYPH );
        TrueType_Unlock_Face( trueTypeVars );

Fail:
        MemUnlock( varBlock );
}


static void CalcTransformMatrix( TextStyle         stylesToImplement,
                                 TransformMatrix*  transMatrix )
{
        /* make unity matrix       */
        transMatrix->TM_matrix.xx = 1L << 16;
        transMatrix->TM_matrix.xy = 0;
        transMatrix->TM_matrix.yx = 0;
        transMatrix->TM_matrix.yy = 1L << 16;
        transMatrix->TM_scriptY   = 0;

        /* fake bold style         */
        if( stylesToImplement & TS_BOLD )
                transMatrix->TM_matrix.xx = BOLD_FACTOR;

        /* fake italic style       */
        if( stylesToImplement & TS_ITALIC )
                transMatrix->TM_matrix.yx = ITALIC_FACTOR;

        /* fake script style       */
        if( stylesToImplement & TS_SUBSCRIPT || stylesToImplement & TS_SUBSCRIPT )
        {      
                transMatrix->TM_matrix.xx = GrMulWWFixed( transMatrix->TM_matrix.xx, SCRIPT_FACTOR );
                transMatrix->TM_matrix.yy = GrMulWWFixed( transMatrix->TM_matrix.yy, SCRIPT_FACTOR );

                if( stylesToImplement & TS_SUBSCRIPT )
                        transMatrix->TM_scriptY = -SCRIPT_SHIFT_FACTOR;
                else
                        transMatrix->TM_scriptY = SCRIPT_SHIFT_FACTOR;
        }
}
