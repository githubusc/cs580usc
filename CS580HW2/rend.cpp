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

enum EdgeType
{
	COLORED, // for left & top edges
	UNCOLORED
};

typedef struct
{
	GzCoord start;
	GzCoord end;
	EdgeType type;
} Edge;

/*** HELPER FUNCTION DECLARATIONS ***/
int sortByXCoord( const void * c1, const void * c2 ); // helper function for getting bounding box around tri (used with qsort)
int sortByYThenXCoord( const void * c1, const void * c2 ); // helper function for sorting tri verts (used with qsort)
bool orderTriVertsCCW( Edge edges[3], GzCoord * verts ); 
bool getTriBoundingBox( float * minX, float * maxX, float * minY, float * maxY, GzCoord * verts );
bool getLineEqnCoeff( float * A, float * B, float * C, Edge edge );
bool crossProd( GzCoord * result, const Edge edge1, const Edge edge2 );
short ctoi( float color );

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
			GzCoord * verts = static_cast<GzCoord *>( valueList[i] );

			// rasterize this triangle
			switch( rasterizeMethod )
			{
			// rasterize using scanlines
			case SCANLINE_METHOD:
				AfxMessageBox( "Error: don't know how to rasterize with scanlines yet.\n" );
				return GZ_FAILURE;
				break;
			// rasterize using LEE
			case LEE_METHOD:
				// get bounding box around triangle
				float minX, maxX, minY, maxY;
				if( !getTriBoundingBox( &minX, &maxX, &minY, &maxY, verts ) )
					return GZ_FAILURE;

				// use convention of orienting all edges to point in counter-clockwise direction
				Edge edges[3];
				if( !orderTriVertsCCW( edges, verts ) )
					return GZ_FAILURE;	

				// calculate 2D line equations coefficients for all edges
				float edge0A, edge0B, edge0C, edge1A, edge1B, edge1C, edge2A, edge2B, edge2C;
				getLineEqnCoeff( &edge0A, &edge0B, &edge0C, edges[0] );
				getLineEqnCoeff( &edge1A, &edge1B, &edge1C, edges[1] );
				getLineEqnCoeff( &edge2A, &edge2B, &edge2C, edges[2] );

				// need to create triangle plane eqn for all pixels that will be rendered in order to interpolate Z
				// A general plane equation has four terms: Ax + By + Cz + D = 0
				// Cross-product of two tri edges produces (A,B,C) vector
				// (X,Y,Z)0 X (X,Y,Z)1 = (A,B,C) = norm to plane of edges (and tri)
				// Solve for D at any vertex, using (A,B,C) from above
				GzCoord crossProduct;
				if( !( crossProd( &crossProduct, edges[0], edges[1] ) ) )
					return GZ_FAILURE;

				float planeA, planeB, planeC, planeD;
				planeA = crossProduct[X];
				planeB = crossProduct[Y];
				planeC = crossProduct[Z];
				planeD = -( planeA * verts[0][X] + planeB * verts[0][Y] + planeC * verts[0][Z] );

				int startX, startY, endX, endY;
				// make sure starting pixel value is non-negative
				startX = max( static_cast<int>( ceil( minX ) ), 0 );
				startY = max( static_cast<int>( ceil( minY ) ), 0 );
				// make sure ending pixel value is within display size
				endX = min( static_cast<int>( floor( maxX ) ), render->display->xres );
				endY = min( static_cast<int>( floor( maxY ) ), render->display->yres );

				// now walk through all pixels within bounding box and rasterize
				// Y coords are rows 
				for( int pixelY = startY; pixelY <= endY; pixelY++ )
				{
					// Y coords are columns
					for( int pixelX = startX; pixelX <= endX; pixelX++ )
					{
						bool onShadedEdge = false;

						// test this pixel against edge0
						float edge0Result = edge0A * pixelX + edge0B * pixelY + edge0C;
						if( edge0Result == 0 )
						{
							if( edges[0].type == COLORED )
								onShadedEdge = true;
							else
								continue; // pixel is on a triangle edge that should not be shaded
						}
						else if( edge0Result < 0 ) // negative value means it's not in the triangle
							continue;
						// end edge0 test

						// if we know we're going to shade the pixel, don't test against any more edges
						if( !onShadedEdge ) 
						{
							// test this pixel against edge1
							float edge1Result = edge1A * pixelX + edge1B * pixelY + edge1C;
							if( edge1Result == 0 )
							{
								if( edges[1].type == COLORED )
									onShadedEdge = true;
								else
									continue; // pixel is on a triangle edge that should not be shaded
							}
							else if( edge1Result < 0 ) // negative value means it's not in the triangle
								continue;
							// end edge1 test
						}

						// if we know we're going to shade the pixel, don't test against any more edges
						if( !onShadedEdge )
						{
							// test this pixel against edge2
							float edge2Result = edge2A * pixelX + edge2B * pixelY + edge2C;
							if( edge2Result == 0 )
							{
								if( edges[2].type == COLORED )
									onShadedEdge = true;
								else
									continue; // pixel is on a triangle edge that should not be shaded
							}
							else if( edge2Result < 0 ) // negative value means it's not in the triangle
								continue;
							// end edge2 test
						}

						// if we're here, the pixel should be shaded. 
						// First interpolate Z and check value against z-buffer
						// Ax + By + Cz + D = 0 => z = -( Ax + By + D ) / C
						float interpZ = -( planeA * pixelX + planeB * pixelY  + planeD ) / planeC;

						// don't render pixels of triangles that reside behind camera
						if( interpZ < 0 )
							continue;

						GzIntensity r, g, b, a;
						GzDepth z;
						// returns a non-zero value for failure
						if( GzGetDisplay( render->display, pixelX, pixelY, &r, &g, &b, &a, &z ) )
						{
							return GZ_FAILURE;
						}

						// if the interpolated z value is smaller than the current z value, write pixel to framebuffer
						if( interpZ < z )
						{
							if( GzPutDisplay( render->display, pixelX, pixelY, 
								              ctoi( render->flatcolor[RED] ),
											  ctoi( render->flatcolor[GREEN] ),
											  ctoi( render->flatcolor[BLUE] ),
											  a, // just duplicate existing alpha value for now
											  ( GzDepth )interpZ ) )
							{
								AfxMessageBox( "Error: could not put new values into display in GzPutTriangle()!\n" );
								return GZ_FAILURE;
							}
						}
					} // end column for loop (X)
				} // end row for loop (Y)

				break;
			// unrecognized rasterization method
			default:
				AfxMessageBox( "Error: unknown rasterization method!!!\n" );
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

bool orderTriVertsCCW( Edge edges[3], GzCoord * verts )
{
	if( !verts )
	{
		AfxMessageBox( "Error: verts are NULL in orderTriVertsCCW()!\n" );
		return false;
	}

	// first sort vertices by Y coordinate (low to high)
	qsort( verts, 3, sizeof( GzCoord ), sortByYThenXCoord );

	/*	
	 * now the CCW oriented edges are either:
	 *    0-1, 1-2, 2-0
	 * OR
	 *    0-2, 2-1, 1-0
	 * depending on which are left and right edges.
	 *
	 * NOTE: We need to determine left/right & top/bottom edges to establish triangle edge ownership.
	 *       We'll use the convention that a triangle owns all left & top edges, but not right & bottom edges.
	 *
	 * Algorithm to do this:
	 * IF NO 2 VERTS HAVE SAME Y COORD:
	 * - Find point along edge 0-2 that has the same Y value as vert 1
	 * - Compare X value at this point (x') to X value at vert 1 (x1)
	 *   x' < x1 => edges with vert 1 must be right edges
	 *   x' > x1 => edges with vert 2 must be left edges
	 *   x' = x1 shouldn't happen if no 2 verts have the exact same Y coordinate
	 * IF ANY 2 VERTS HAVE SAME Y COORD: (similar algorithm, but for top/bottom edges)
	 * - We can arbitrarily determine the orientation of the edges and which are colored/uncolored.
	 * - Call the verts that make up the horizontal edge v1 & v2, and the other vert v3.
	 *   v3.y < v1.y => CCW edges: v1 -> v2 (uncolored), v2 -> v3 (uncolored), v3 -> v1 (colored)
	 *   v3.y > v1.y => CCW edges: v2 -> v1 (colored), v1 -> v3 (colored), v3 -> v2 (uncolored)
	 */

	// check to see if any two y-coords are equal; since they're ordered by Y first, equal Y coords will be adjacent
	if( verts[0][Y] == verts[1][Y] ) // 2 Y coords are equal
	{
		// since verts are sorted by Y first, we know that 3rd vert has a greater Y than the other 2
		memcpy( edges[0].start, verts[1], sizeof( GzCoord ) );
		memcpy( edges[0].end, verts[0], sizeof( GzCoord ) );
		edges[0].type = COLORED;

		memcpy( edges[1].start, verts[0], sizeof( GzCoord ) );
		memcpy( edges[1].end, verts[2], sizeof( GzCoord ) );
		edges[1].type = COLORED;

		memcpy( edges[2].start, verts[2], sizeof( GzCoord ) );
		memcpy( edges[2].end, verts[1], sizeof( GzCoord ) );
		edges[2].type = UNCOLORED;
	}
	else if( verts[1][Y] == verts[2][Y] ) // 2 Y coords are equal
	{
		// since verts are sorted by Y first, we know that the 1st vert has a smaller Y than the other 2
		memcpy( edges[0].start, verts[0], sizeof( GzCoord ) );
		memcpy( edges[0].end, verts[1], sizeof( GzCoord ) );
		edges[0].type = COLORED;

		memcpy( edges[1].start, verts[1], sizeof( GzCoord ) );
		memcpy( edges[1].end, verts[2], sizeof( GzCoord ) );
		edges[1].type = UNCOLORED;

		memcpy( edges[2].start, verts[2], sizeof( GzCoord ) );
		memcpy( edges[2].end, verts[0], sizeof( GzCoord ) );
		edges[2].type = UNCOLORED;
	}
	else // all 3 Y coords are distinct
	{
		// formulate line equation for v0 - v2
		float slope = ( verts[2][Y] - verts[0][Y] ) / ( verts[2][X] - verts[0][X] );
		// use slope to find point along v0 - v2 line with same y coord as v1's y coord
		// y - y1 = m( x - x1 ) => x = ( ( y - y1 )/m ) + x1. We'll use vert0 for x1 and y1.
		float midpointX = ( ( verts[1][Y] - verts[0][Y] ) / slope ) + verts[0][X];

		// midpointX is less than v1's x, so all edges touching v1 are uncolored
		if( midpointX < verts[1][X] )
		{
			memcpy( edges[0].start, verts[0], sizeof( GzCoord ) );
			memcpy( edges[0].end, verts[2], sizeof( GzCoord ) );
			edges[0].type = COLORED;

			memcpy( edges[1].start, verts[2], sizeof( GzCoord ) );
			memcpy( edges[1].end, verts[1], sizeof( GzCoord ) );
			edges[1].type = UNCOLORED;

			memcpy( edges[2].start, verts[1], sizeof( GzCoord ) );
			memcpy( edges[2].end, verts[0], sizeof( GzCoord ) );
			edges[2].type = UNCOLORED;
		}
		// midpoint X is greater than v1's x, so all edges touching v1 are colored
		else if( midpointX > verts[1][X] )
		{
			memcpy( edges[0].start, verts[0], sizeof( GzCoord ) );
			memcpy( edges[0].end, verts[1], sizeof( GzCoord ) );
			edges[0].type = COLORED;

			memcpy( edges[1].start, verts[1], sizeof( GzCoord ) );
			memcpy( edges[1].end, verts[2], sizeof( GzCoord ) );
			edges[1].type = COLORED;

			memcpy( edges[2].start, verts[2], sizeof( GzCoord ) );
			memcpy( edges[2].end, verts[0], sizeof( GzCoord ) );
			edges[2].type = UNCOLORED;
		}
		// they are exactly equal. this shouldn't happen.
		else
		{
			AfxMessageBox( "Error: midpointX is exactly equal to vert1's x in orderTriVertsCCW!\n" );
			return false;
		}
	}

	return true;
}

int sortByYThenXCoord( const void * c1, const void * c2 )
{
	GzCoord * coord1 = ( GzCoord * )c1;
	GzCoord * coord2 = ( GzCoord * )c2;

	// find the difference in Y coordinate values
	float yDiff = ( *coord1 )[Y] - ( *coord2 )[Y];

	// need to return an int, so just categorize by sign
	if( yDiff < 0 )
		return -1;
	else if( yDiff > 0 )
		return 1;
	else
	{
		// Y coordinates are exactly equal. Now sort by x coord.
		float xDiff = ( *coord1 )[X] - ( *coord2 )[X];
		if( xDiff < 0 )
			return -1;
		else if( xDiff > 0 )
			return 1;
		else
			return 0; // this means we're dealing with an axis-aligned right triangle
	}
}

int sortByXCoord( const void * c1, const void * c2 )
{
	GzCoord * coord1 = ( GzCoord * )c1;
	GzCoord * coord2 = ( GzCoord * )c2;

	float xDiff = ( *coord1 )[X] - ( *coord2 )[X];
	if( xDiff < 0 )
		return -1;
	else if( xDiff > 0 )
		return 1;
	else
		return 0;
}

bool getTriBoundingBox( float * minX, float * maxX, float * minY, float * maxY, GzCoord * verts )
{
	if( !minX || !maxX || !minY || !maxY || !verts )
	{
		AfxMessageBox( "Error: received bad pointer in getTriBoundingBox()!\n" );
		return false;
	}

	qsort( verts, 3, sizeof( GzCoord ), sortByXCoord );
	*minX = verts[0][X];
	*maxX = verts[2][X];

	qsort( verts, 3, sizeof( GzCoord ), sortByYThenXCoord );
	*minY = verts[0][Y];
	*maxY = verts[2][Y];

	return true;
}

bool getLineEqnCoeff( float * A, float * B, float * C, Edge edge )
{
	if( !A || !B || !C )
	{
		AfxMessageBox( "Error: received bad pointer in getLineEqnCoeff()!\n" );
		return false;
	}

	// recall: start point of vector is the tail, end point of vector is the head

	/*
	 * Algorithm:
	 * - Define tail as (X,Y) and head as (X + dX, Y + dY).
	 * - Edge Equation:  E(x,y) = dY (x-X) - dX (y-Y)
	 *                          = 0 for points (x,y) on line
	 *                          = + for points in half-plane to right/below line (assuming CCW orientation)
	 *                          = - for points in half-plane to left/above line (assuming CCW orientation)
	 * - Use above definition and cast into general form of 2D line equation Ax + By + C = 0
	 *   dYx - dYX - dXy + dXY = 0    (multiply terms)
	 *   dYx + (-dXy) + (dXY - dYX) = 0   (collect terms) 
	 *   A = dY
	 *   B = -dX
	 *   C = dXY – dYX    (Compute A,B,C from edge verts)
	 */

	float varX = edge.start[0];
	float varY = edge.start[1];

	float dX = edge.end[0] - varX;
	float dY = edge.end[1] - varY;

	*A = dY;
	*B = -dX;
	*C = ( dX * varY ) - ( dY * varX );

	return true;
}

bool crossProd( GzCoord * result, const Edge edge1, const Edge edge2 )
{
	if( !result )
	{
		AfxMessageBox( "Error: cannot compute cross product - bad pointer!\n" );
		return false;
	}

	GzCoord vec1, vec2;

	// define edge1 vector
	vec1[0] = edge1.end[0] - edge1.start[0];
	vec1[1] = edge1.end[1] - edge1.start[1];
	vec1[2] = edge1.end[2] - edge1.start[2];

	// define edge2 vector
	vec2[0] = edge2.end[0] - edge2.start[0];
	vec2[1] = edge2.end[1] - edge2.start[1];
	vec2[2] = edge2.end[2] - edge2.start[2];

	// compute cross product
	( *result )[X] = vec1[Y] * vec2[Z] - vec1[Z] * vec2[Y];
	( *result )[Y] = -( vec1[X] * vec2[Z] - vec1[Z] * vec2[X] );
	( *result )[Z] = vec1[X] * vec2[Y] - vec1[Y] * vec2[X];

	return true;
}