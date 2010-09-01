/*   CS580 HW   */
#include    "stdafx.h"  
#include	"Gz.h"
#include	"disp.h"

#define RGB_MAX_INTENSITY 4095

int GzNewFrameBuffer(char** framebuffer, int width, int height)
{
/* create a framebuffer:
 -- allocate memory for framebuffer : 3 x (sizeof)char x width x height
 -- pass back pointer 
*/
	if( !framebuffer )
		return GZ_FAILURE;

	// allocate 3 chars per pixel to allot for R, G, and B
	*framebuffer = ( char * )calloc( 3 * width * height, sizeof( char ) );
	
	// make sure allocation succeeded
	if( !( *framebuffer ) )
		return GZ_FAILURE;

	return GZ_SUCCESS;
}

int GzNewDisplay(GzDisplay	**display, GzDisplayClass dispClass, int xRes, int yRes)
{

/* create a display:
  -- allocate memory for indicated class and resolution
  -- pass back pointer to GzDisplay object in display
*/
	// check for a bad incoming pointer; don't want to deref it & cause seg fault
	if( !display )
		return GZ_FAILURE;

	// make sure xRes and yRes are valid
	if( xRes < 0 || yRes < 0 || xRes > MAXXRES || yRes > MAXYRES )
		return GZ_FAILURE;

	GzDisplay * disp = new GzDisplay;

	// make sure allocation was successful
	if( !disp )
		return GZ_FAILURE;

	disp->xres = xRes;
	disp->yres = yRes;
	disp->fbuf = ( GzPixel * )calloc( xRes * yRes, sizeof( GzPixel ) );
	disp->open = 1; // non-zero value indicates the display is open
	disp->dispClass = GZ_RGBAZ_DISPLAY;

	// make sure fbuf allocation was successful
	if( !( disp->fbuf ) )
	{
		// first clean up locally allocated memory
		free( disp );
		disp = NULL;

		return GZ_FAILURE;
	}
	
	*display = disp;
	return GZ_SUCCESS;
}


int GzFreeDisplay(GzDisplay	*display)
{
/* clean up, free memory */

	free( display->fbuf );
	display->fbuf = NULL;

	free( display );
	display = NULL;

	return GZ_SUCCESS;
}


int GzGetDisplayParams(GzDisplay *display, int *xRes, int *yRes, GzDisplayClass	*dispClass)
{
/* pass back values for an open display */

	// check for bad pointers
	if( !display || !xRes || !yRes || !dispClass )
		return GZ_FAILURE;

	*xRes = display->xres;
	*yRes = display->yres;
	*dispClass = display->dispClass;

	return GZ_SUCCESS;
}


int GzInitDisplay(GzDisplay	*display)
{
/* set everything to some default values - start a new frame */

	// check for bad pointer
	if( !display )
		return GZ_FAILURE;

	// loop through all GzPixels in the display
	// display rows
	for( int row = 0; row < display->xres; row++ )
	{
		// display columns
		for( int col = 0; col < display->yres; col++ )
		{
			int idx = row * display->yres + col;

			// use brown color for background
			display->fbuf[idx].red = 153 << 4;
			display->fbuf[idx].green = 102 << 4;
			display->fbuf[idx].blue = 54 << 4;

			// make the pixel completely opaque
			display->fbuf[idx].alpha = 100;

			// use maximum Z value so it will be sure to be overwritten later
			display->fbuf[idx].z = INT_MAX;
		}
	}

	return GZ_SUCCESS;
}


int GzPutDisplay(GzDisplay *display, int i, int j, GzIntensity r, GzIntensity g, GzIntensity b, GzIntensity a, GzDepth z)
{
/* write pixel values into the display */
	
	// check for bad pointer
	if( !display )
		return GZ_FAILURE;

	// if i or j is out of bounds, just ignore it
	if( i < 0 || j < 0 || i >= display->xres || j >= display->yres )
		return GZ_SUCCESS;

	// first clamp the RGB values to be within the appropriate range
	if( r < 0 )
		r = 0;
	else if( r > RGB_MAX_INTENSITY )
		r = RGB_MAX_INTENSITY;

	if( g < 0 )
		g = 0;
	else if( g > RGB_MAX_INTENSITY )
		g = RGB_MAX_INTENSITY;

	if( b < 0 )
		b = 0;
	else if( b > RGB_MAX_INTENSITY )
		b = RGB_MAX_INTENSITY;

	// note: i => columns, j => rows
	int idx = display->yres * j + i;

	display->fbuf[idx].red = r;
	display->fbuf[idx].green = g;
	display->fbuf[idx].blue = b;

	display->fbuf[idx].alpha = a;
	display->fbuf[idx].z = z;

	return GZ_SUCCESS;
}


int GzGetDisplay(GzDisplay *display, int i, int j, GzIntensity *r, GzIntensity *g, GzIntensity *b, GzIntensity *a, GzDepth *z)
{
	/* pass back pixel value in the display */
	/* check display class to see what vars are valid */

	// check for bad pointers
	if( !display || !r || !g || !b || !a || !z )
		return GZ_FAILURE;

	// make sure i and j are within appropriate bounds
	if( i < 0 || j < 0 || i >= display->xres || j >= display->yres )
		return GZ_FAILURE;

	int idx = i * display->yres + j;
	*r = display->fbuf[idx].red;
	*g = display->fbuf[idx].green;
	*b = display->fbuf[idx].blue;
	*a = display->fbuf[idx].alpha;
	*z = display->fbuf[idx].z;

	return GZ_SUCCESS;
}


int GzFlushDisplay2File(FILE* outfile, GzDisplay *display)
{

	/* write pixels to ppm file based on display class -- "P6 %d %d 255\r" */

	return GZ_SUCCESS;
}

int GzFlushDisplay2FrameBuffer(char* framebuffer, GzDisplay *display)
{

	/* write pixels to framebuffer: 
		- Put the pixels into the frame buffer
		- Caution: store the pixel to the frame buffer as the order of blue, green, and red 
		- Not red, green, and blue !!!
	*/

	// check for bad pointers
	if( !framebuffer || !display )
		return GZ_FAILURE;

	// display rows
	for( int row = 0; row < display->xres; row++ )
	{
		// display columns
		for( int col = 0; col < display->yres; col++ )
		{
			int fbufIdx = row * display->yres + col;

			char * blueLoc = &( framebuffer[fbufIdx * 3] ); // each idx has 3 chars: B, G, & R
			char * greenLoc = blueLoc + sizeof( char );
			char * redLoc = greenLoc + sizeof( char );

			// we need to reduce the 12-bit RGB intensity values to fit into 8 bits
			char red = display->fbuf[fbufIdx].red >> 4;
			char green = display->fbuf[fbufIdx].green >> 4;
			char blue = display->fbuf[fbufIdx].blue >> 4;

			memcpy( blueLoc, &blue, sizeof( char ) );
			memcpy( greenLoc, &green, sizeof( char ) );
			memcpy( redLoc, &red, sizeof( char ) );
		}
	}

	return GZ_SUCCESS;
}