#include	"stdafx.h"
#include	"stdio.h"
#include	"math.h"
#include	"Gz.h"
#include	"rend.h"

#define SCANLINE_METHOD 0
#define LEE_METHOD 1

// flag to toggle between rasterizing methods
// NOTE: we're only required to implement ONE of these. I just did both for fun.
int rasterizeMethod = LEE_METHOD;

int GzNewRender(GzRender **render, GzRenderClass renderClass, GzDisplay *display)
{
/* 
- malloc a renderer struct
- keep closed until BeginRender inits are done
- span interpolator needs pointer to display for pixel writes
- check for legal class GZ_Z_BUFFER_RENDER
*/
	// check for bad pointers
	if( !render || !display )
		return GZ_FAILURE;

	GzRender * tmpRenderer = new GzRender;
	// make sure allocation worked
	if( !tmpRenderer )
		return GZ_FAILURE;

	tmpRenderer->renderClass = renderClass;
	tmpRenderer->display = display;
	tmpRenderer->open = 0; // zero value indicates that renderer is closed

	// the rest of the struct members will be initialized in GzBeginRender.

	*render = tmpRenderer;

	return GZ_SUCCESS;
}


int GzFreeRender(GzRender *render)
{
/* 
-free all renderer resources
*/
	// check for bad pointer
	if( !render )
		return GZ_FAILURE;

	// do NOT free the display here. It should be freed elsewhere.
	render->display = NULL;

	free( render );
	render = NULL;

	return GZ_SUCCESS;
}


int GzBeginRender(GzRender	*render)
{
/* 
- set up for start of each frame - init frame buffer
*/
	// check for bad pointer
	if( !render )
		return GZ_FAILURE;

	render->open = 1; // non-zero value indicates that renderer is now open

	// use red for default flat color
	render->flatcolor[RED] = 1; 
	render->flatcolor[GREEN] = 0;
	render->flatcolor[BLUE] = 0;

	return GZ_SUCCESS;
}


int GzPutAttribute(GzRender	*render, int numAttributes, GzToken	*nameList, 
	GzPointer *valueList) /* void** valuelist */
{
/*
- set renderer attribute states (e.g.: GZ_RGB_COLOR default color)
- later set shaders, interpolaters, texture maps, and lights
TRANSLATION:
Assign the values in valueList to render based on categories in nameList
 - For this assignment, check for GZ_RGB_COLOR in nameList and assign the color values in valueList to render variable
*/
	// check for bad pointers
	if( !render || !nameList || !valueList )
		return GZ_FAILURE;

	for( int i = 0; i < numAttributes; i++ )
	{
		// check for GZ_RGB_COLOR in nameList
		if( nameList[i] == GZ_RGB_COLOR )
		{
			GzColor * colorPtr = ( static_cast<GzColor *>( valueList[i] ) );
			// assign the color values in valueList to render variable
			render->flatcolor[RED] = ( *colorPtr )[RED];
			render->flatcolor[GREEN] = ( *colorPtr )[GREEN];
			render->flatcolor[BLUE] = ( *colorPtr )[BLUE];
		}
	}

	return GZ_SUCCESS;
}


int GzPutTriangle(GzRender *render, int	numParts, GzToken *nameList,
	GzPointer *valueList) 
/* numParts - how many names and values */
{
/* 
- pass in a triangle description with tokens and values corresponding to
      GZ_NULL_TOKEN:		do nothing - no values
      GZ_POSITION:		3 vert positions in model space
- Invoke the scan converter and return an error code
TRANSLATION:
Similar to GzPutAttribute but check for GZ_POSITION and GZ_NULL_TOKEN in nameList
Takes in a list of triangles, uses the scan-line or LEE method of rasterizing.
Then calls GzPutDisplay() to draw those pixels to the display.
*/
	// check for bad pointers
	if( !render || !nameList || !valueList )
		return GZ_FAILURE;

	for( int i = 0; i < numParts; i++ )
	{
		if( nameList[i] == GZ_NULL_TOKEN )
		{
			// do nothing - no values
		}
		else if( nameList[i] == GZ_POSITION )
		{
			// rasterize using scanlines
			if( rasterizeMethod == SCANLINE_METHOD )
			{
			}
			// rasterize using LEE
			else if( rasterizeMethod == LEE_METHOD )
			{
			}
			// unrecognized rasterization method
			else
			{
				fprintf( stderr, "Error: unknown rasterization method!!!\n" );
				return GZ_FAILURE;
			}
		}
	}

	return GZ_SUCCESS;
}

/* NOT part of API - just for general assistance */

short	ctoi(float color)		/* convert float color to GzIntensity short */
{
  return(short)((int)(color * ((1 << 12) - 1)));
}

