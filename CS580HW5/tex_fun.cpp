/* Texture functions for cs580 GzLib	*/
#include    "stdafx.h" 
#include	"stdio.h"
#include	"math.h"
#include	"Gz.h"

GzColor	*image;
int xs, ys;
int reset = 1;

#define	IMG_ARRAY( x, y, xres )	( x + ( y * xres ) )	/* simplify image indexing */

/* Image texture function */
int tex_fun(float u, float v, GzColor color)
{
  unsigned char		pixel[3];
  unsigned char     dummy;
  char  		foo[8];
  int   		i, j;
  FILE			*fd;

  if (reset) {          /* open and load texture file */
    fd = fopen ("texture", "rb");
    if (fd == NULL) {
      fprintf (stderr, "texture file not found\n");
      exit(-1);
    }
    fscanf (fd, "%s %d %d %c", foo, &xs, &ys, &dummy);
    image = (GzColor*)malloc(sizeof(GzColor)*(xs+1)*(ys+1));
    if (image == NULL) {
      fprintf (stderr, "malloc for texture image failed\n");
      exit(-1);
    }

    for (i = 0; i < xs*ys; i++) {	/* create array of GzColor values */
      fread(pixel, sizeof(pixel), 1, fd);
      image[i][RED] = (float)((int)pixel[RED]) * (1.0 / 255.0);
      image[i][GREEN] = (float)((int)pixel[GREEN]) * (1.0 / 255.0);
      image[i][BLUE] = (float)((int)pixel[BLUE]) * (1.0 / 255.0);
      }

    reset = 0;          /* init is done */
	fclose(fd);
  }

/* bounds-test u,v to make sure nothing will overflow image array bounds */
/* determine texture cell corner values and perform bilinear interpolation */
/* set color to interpolated GzColor value and return */

  // u and v should vary in range [0,1]. Clamp values if they are outside of bounds.
  if( u < 0 )
	  u = 0;
  if( u > 1 )
	  u = 1;
  if( v < 0 )
	  v = 0;
  if( v > 1 )
	  v = 1;

  // compute texel index. Note that the maximum texel X is (xs - 1) and the maximum texel Y is (xy - 1)
  float texelX = u * ( xs - 1);
  float texelY = v * ( ys - 1);

  // find texel that form square around this texture coordinate location (e.g. texture cell corner values)
  float x_min = floor( texelX );
  float x_max = ceil( texelX );
  float y_min = floor( texelY );
  float y_max = ceil( texelY );

  /*
   * The variable names below correspond to this layout:
   *	color0				color1
   *	(x_min, y_min)		(x_max, y_min)
   *    
   *
   *    color2				color3
   *	(x_min, y_max)		(x_max, y_max)
   */
  // now retrieve the colors at the four cell corner values
  GzColor color0, color1, color2, color3;
  memcpy( color0, image[( int )IMG_ARRAY( x_min, y_min, xs )], sizeof( GzColor ) );
  memcpy( color1, image[( int )IMG_ARRAY( x_max, y_min, xs )], sizeof( GzColor ) );
  memcpy( color2, image[( int )IMG_ARRAY( x_min, y_max, xs )], sizeof( GzColor ) );
  memcpy( color3, image[( int )IMG_ARRAY( x_max, y_max, xs )], sizeof( GzColor ) );

  // now we need to perform bilinear interpolation on these colors. First interpolate the rows
  GzColor colorTopRow, colorBottomRow;
  float left_weight = x_max - texelX;
  float top_weight = y_max - texelY;
  for( i = 0; i < 3; i++ )
  {
	  // interpolate this color component for the top and bottom row
	  colorTopRow[i] = ( left_weight * color0[i] ) + ( ( 1 - left_weight ) * color1[i] );
	  colorBottomRow[i] = ( left_weight * color2[i] ) + ( ( 1 - left_weight ) * color3[i] );

	  // now interpolate between the top and bottom row
	  color[i] = ( top_weight * colorTopRow[i] ) + ( ( 1 - top_weight ) * colorBottomRow[i] );
  }

  return GZ_SUCCESS;
}


/* Procedural texture function */
int ptex_fun(float u, float v, GzColor color)
{
	return 0;
}

