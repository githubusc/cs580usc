/* CS580 Homework 3 */

#include	"stdafx.h"
#include	"stdio.h"
#include	"math.h"
#include	"Gz.h"
#include	"rend.h"

#define PI 3.14159265

// to help with matrix transform stack
#define EMPTY_STACK -1
#define XFORM_PERSP_TO_SCREEN 0
#define XFORM_IMAGE_TO_PERSP 1

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
// from HW2
int sortByXCoord( const void * c1, const void * c2 ); // helper function for getting bounding box around tri (used with qsort)
int sortByYThenXCoord( const void * c1, const void * c2 ); // helper function for sorting tri verts (used with qsort)
bool orderTriVertsCCW( Edge edges[3], GzCoord * verts ); 
bool getTriBoundingBox( float * minX, float * maxX, float * minY, float * maxY, GzCoord * verts );
bool getLineEqnCoeff( float * A, float * B, float * C, Edge edge );
bool crossProd( GzCoord result, const Edge edge1, const Edge edge2 );
bool rasterizeLEE( GzRender * render, GzCoord * verts );
// from HW3
bool constructCameraXforms( GzRender * render );
bool constructXsp( GzRender * render );
bool clearStack( GzRender * render );
bool matrixMultiply( const GzMatrix mat1, const GzMatrix mat2, GzMatrix matProduct );
bool posVectorToScreenSpace( const GzMatrix mat, const GzCoord vector, GzCoord result );
float vectorDot( const GzCoord vec1, const GzCoord vec2 );
bool vectorCross( const GzCoord vec1, const GzCoord vec2, GzCoord crossProduct );
bool vectorSub( const GzCoord vec1, const GzCoord vec2, GzCoord difference ); 
bool vectorScale( const GzCoord vector, float scaleFactor, GzCoord result );
bool negateVector( const GzCoord origVec, GzCoord negatedVec );
bool normalize( GzCoord vector );
bool triangleOutsideImagePlane( GzRender * render, GzCoord * verts );
// from Professor
short ctoi( float color );
/*** END HELPER FUNCTION DECLARATIONS ***/

int GzRotXMat(float degree, GzMatrix mat)
{
// Create rotate matrix : rotate along x axis
// Pass back the matrix using mat value
/*
 * X-Rotation matrix format:
 * 
 *	1		0			0			0	
 *	0		cos(theta)	-sin(theta)	0
 *	0		sin(theta)	cos(theta)	0
 *	0		0			0			1
 */

	// row 0
	mat[0][0] = 1;
	mat[0][1] = 0;
	mat[0][2] = 0;
	mat[0][3] = 0;

	// row 1
	mat[1][0] = 0;
	mat[1][1] = cos( degree );
	mat[1][2] = -sin( degree );
	mat[1][3] = 0;

	// row 2
	mat[2][0] = 0;
	mat[2][1] = sin( degree );
	mat[2][2] = cos( degree );
	mat[2][3] = 0;

	// row 3
	mat[3][0] = 0;
	mat[3][1] = 0;
	mat[3][2] = 0;
	mat[3][3] = 1;

	return GZ_SUCCESS;
}


int GzRotYMat(float degree, GzMatrix mat)
{
// Create rotate matrix : rotate along y axis
// Pass back the matrix using mat value
/*
 * Y-Rotation matrix format:
 * 
 *	cos(theta)	0			sin(theta)	0	
 *	0			1			0			0
 *	-sin(theta)	0			cos(theta)	0
 *	0			0			0			1
 */

	// row 0
	mat[0][0] = cos( degree );
	mat[0][1] = 0;
	mat[0][2] = sin( degree );
	mat[0][3] = 0;

	// row 1
	mat[1][0] = 0;
	mat[1][1] = 1;
	mat[1][2] = 0;
	mat[1][3] = 0;

	// row 2
	mat[2][0] = -sin( degree );
	mat[2][1] = 0;
	mat[2][2] = cos( degree );
	mat[2][3] = 0;

	// row 3
	mat[3][0] = 0;
	mat[3][1] = 0;
	mat[3][2] = 0;
	mat[3][3] = 1;

	return GZ_SUCCESS;
}


int GzRotZMat(float degree, GzMatrix mat)
{
// Create rotate matrix : rotate along z axis
// Pass back the matrix using mat value
/*
 * Z-Rotation matrix format:
 * 
 *	cos(theta)	-sin(theta)	0		0			
 *	sin(theta)	cos(theta)	0		0
 *	0			0			1		0
 *	0			0			0		1	
 */

	// row 0
	mat[0][0] = cos( degree );
	mat[0][1] = -sin( degree );
	mat[0][2] = 0;
	mat[0][3] = 0;

	// row 1
	mat[1][0] = sin( degree );
	mat[1][1] = cos( degree );
	mat[1][2] = 0;
	mat[1][3] = 0;

	// row 2
	mat[2][0] = 0;
	mat[2][1] = 0;
	mat[2][2] = 1;
	mat[2][3] = 0;

	// row 3
	mat[3][0] = 0;
	mat[3][1] = 0;
	mat[3][2] = 0;
	mat[3][3] = 1;

	return GZ_SUCCESS;
}


int GzTrxMat(GzCoord translate, GzMatrix mat)
{
// Create translation matrix
// Pass back the matrix using mat value
/*
 * Translation matrix format:
 *	1	0	0	Tx
 *	0	1	0	Ty
 *	0	0	1	Tz
 *	0	0	0	1
 */

	// row 0
	mat[0][0] = 1;
	mat[0][1] = 0;
	mat[0][2] = 0;
	mat[0][3] = translate[X];

	// row 1
	mat[1][0] = 0;
	mat[1][1] = 1;
	mat[1][2] = 0;
	mat[1][3] = translate[Y];

	// row 2
	mat[2][0] = 0;
	mat[2][1] = 0;
	mat[2][2] = 1;
	mat[2][3] = translate[Z];

	// row 3
	mat[3][0] = 0;
	mat[3][1] = 0;
	mat[3][2] = 0;
	mat[3][3] = 1;

	return GZ_SUCCESS;
}


int GzScaleMat(GzCoord scale, GzMatrix mat)
{
// Create scaling matrix
// Pass back the matrix using mat value
/*
 * Scaling matrix format:
 *	Sx	0	0	0
 *	0	Sy	0	0
 *	0	0	Sz	0
 *	0	0	0	1
 */

	// row 0
	mat[0][0] = scale[X];
	mat[0][1] = 0;
	mat[0][2] = 0;
	mat[0][3] = 0;

	// row 1
	mat[1][0] = 0;
	mat[1][1] = scale[Y];
	mat[1][2] = 0;
	mat[1][3] = 0;

	// row 2
	mat[2][0] = 0;
	mat[2][1] = 0;
	mat[2][2] = scale[Z];
	mat[2][3] = 0;

	// row 3
	mat[3][0] = 0;
	mat[3][1] = 0;
	mat[3][2] = 0;
	mat[3][3] = 1;

	return GZ_SUCCESS;
}


//----------------------------------------------------------
// Begin main functions

int GzNewRender(GzRender **render, GzRenderClass renderClass, GzDisplay	*display)
{
/*  
- malloc a renderer struct 
- keep closed until all inits are done 
- setup Xsp and anything only done once <-- this actually relies on the camera, so wait until GzBeginRender
- span interpolator needs pointer to display 
- check for legal class GZ_Z_BUFFER_RENDER 
- init default camera 
*/ 

	// check for bad pointers
	if( !render || !display )
		return GZ_FAILURE;

	// make sure render class is legal
	if( renderClass != GZ_Z_BUFFER_RENDER )
		return GZ_FAILURE;

	/* MALLOC A RENDERER STRUCT */
	GzRender * tmpRenderer = new GzRender;
	// make sure allocation worked
	if( !tmpRenderer )
		return GZ_FAILURE;

	tmpRenderer->renderClass = renderClass;
	tmpRenderer->display = display;
	tmpRenderer->open = 0; // zero value indicates that renderer is closed

	
	/* DONE MALLOCING RENDERER STRUCT */

	// init default camera - must be done BEFORE calling constructXsp
	tmpRenderer->camera.FOV = DEFAULT_FOV;
	// default lookat is (0,0,0)
	tmpRenderer->camera.lookat[X] = tmpRenderer->camera.lookat[Y] = tmpRenderer->camera.lookat[Z] = 0; 
	// camera position is the image plane origin
	tmpRenderer->camera.position[X] = DEFAULT_IM_X;
	tmpRenderer->camera.position[Y] = DEFAULT_IM_Y;
	tmpRenderer->camera.position[Z] = DEFAULT_IM_Z;
	// default world up is (0,1,0)
	tmpRenderer->camera.worldup[X] = tmpRenderer->camera.worldup[Z] = 0;
	tmpRenderer->camera.worldup[Y] = 1;

	// initialize default ambient light to be off
	tmpRenderer->ambientlight.color[RED] = 0.0f;
	tmpRenderer->ambientlight.color[BLUE] = 0.0f;
	tmpRenderer->ambientlight.color[GREEN] = 0.0f;

	// right now, no other lights have been added (ambient light does not count in light count)
	tmpRenderer->numlights = 0;

	// initialize default Ka, Kd, & Ks
	GzColor defaultKa = DEFAULT_AMBIENT, defaultKd = DEFAULT_DIFFUSE, defaultKs = DEFAULT_SPECULAR;
	tmpRenderer->Ka[RED] = defaultKa[RED];
	tmpRenderer->Ka[GREEN] = defaultKa[GREEN];
	tmpRenderer->Ka[BLUE] = defaultKa[BLUE];
	tmpRenderer->Kd[RED] = defaultKd[RED];
	tmpRenderer->Kd[GREEN] = defaultKd[GREEN];
	tmpRenderer->Kd[BLUE] = defaultKd[BLUE];
	tmpRenderer->Ks[RED] = defaultKs[RED];
	tmpRenderer->Ks[GREEN] = defaultKs[GREEN];
	tmpRenderer->Ks[BLUE] = defaultKs[BLUE];

	// initialize default specular power
	tmpRenderer->spec = DEFAULT_SPEC;

	// use flat shading as the default interpolation mode
	tmpRenderer->interp_mode = GZ_RGB_COLOR;

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


int GzBeginRender(GzRender *render)
{
/*  
- set up for start of each frame - clear frame buffer 
- compute Xiw and projection xform Xpi from camera definition 
- init Ximage - put Xsp at base of stack, push on Xpi and Xiw 
- now stack contains Xsw and app can push model Xforms if it wants to. 
*/ 
	// check for bad pointer
	if( !render )
		return GZ_FAILURE;	

	// use red for default flat color
	render->flatcolor[RED] = 1; 
	render->flatcolor[GREEN] = 0;
	render->flatcolor[BLUE] = 0;

	// Calculate Xsp, Xpi, and Xiw
	if( !constructCameraXforms( render ) )
		return GZ_FAILURE;

	if( !constructXsp( render ) )
		return GZ_FAILURE;

	// empty the transform stack
	clearStack( render );

	int status = GZ_SUCCESS;

	// put Xsp at the base of the stack
	status |= GzPushMatrix( render, render->Xsp );

	// push Xpi onto the stack
	status |= GzPushMatrix( render, render->camera.Xpi );

	// push Xiw onto the stack
	status |= GzPushMatrix( render, render->camera.Xiw );

	render->open = 1; // non-zero value indicates that renderer is now open

	return status;
}

int GzPutCamera(GzRender *render, GzCamera *camera)
{
/*
- overwrite renderer camera structure with new camera definition
*/
	if( !render || !camera )
		return GZ_FAILURE;

	memcpy( &( render->camera ), camera, sizeof( GzCamera ) );

	return GZ_SUCCESS;	
}

int GzPushMatrix(GzRender *render, GzMatrix	matrix)
{
/*
- push a matrix onto the Ximage geometry stack
- check for stack overflow
*/
	if( !render || render->matlevel == MATLEVELS )
		return GZ_FAILURE;

	// if the geometry stack is empty, just put this matrix directly onto the stack (no multiplication necessary)
	if( render->matlevel == EMPTY_STACK )
	{
		// copy the matrix into the geometry stack
		memcpy( render->Ximage[render->matlevel + 1], matrix, sizeof( GzMatrix ) );
	}
	else
	{
		// the matrix we push onto the geometry stack should be the top of the stack multiplied by the new transform (on the right)
		GzMatrix xformProduct;
		matrixMultiply( render->Ximage[render->matlevel], matrix, xformProduct );

		// copy the matrix into the geometry stack
		memcpy( render->Ximage[render->matlevel + 1], xformProduct, sizeof( GzMatrix ) );
	}

	// another matrix has been pushed onto the geometry stack
	render->matlevel++;

	// now push the appropriate matrix on the normals transform stack
	// if this is the Xsp or Xpi matrix, DON'T include it on the normals transform stack. Push the identity matrix instead.
	if( render->matlevel == XFORM_PERSP_TO_SCREEN || render->matlevel == XFORM_IMAGE_TO_PERSP )
	{
		GzMatrix identityMatrix;

		// row 0
		identityMatrix[0][0] = 1;
		identityMatrix[0][1] = 0;
		identityMatrix[0][2] = 0;
		identityMatrix[0][3] = 0;
		// row 1
		identityMatrix[1][0] = 0;
		identityMatrix[1][1] = 1;
		identityMatrix[1][2] = 0;
		identityMatrix[1][3] = 0;
		// row 2
		identityMatrix[2][0] = 0;
		identityMatrix[2][1] = 0;
		identityMatrix[2][2] = 1;
		identityMatrix[2][3] = 0;
		// row 3
		identityMatrix[3][0] = 0;
		identityMatrix[3][1] = 0;
		identityMatrix[3][2] = 0;
		identityMatrix[3][3] = 1;

		// copy the identity matrix into the normals transform stack
		memcpy( render->Xnorm[render->matlevel], identityMatrix, sizeof( GzMatrix ) );
	}
	// this is NOT the Xsp or Xpi matrix, so include it on the normals transform stack. 
	else
	{
		// Note: Transforms and rotations are NOT allowed on the normals transform stack.
		//       It may only contain unitary rotations and uniform scaling.
		//       Thus we must pre-process the matrix being pushed on the stack to make sure it fits these criteria.
		GzMatrix processedMatrix;

		// row 0 - last column has no translation
		processedMatrix[0][0] = matrix[0][0];
		processedMatrix[0][1] = matrix[0][1];
		processedMatrix[0][2] = matrix[0][2];
		processedMatrix[0][3] = 0;
		// row 1 - last column has no translation
		processedMatrix[1][0] = matrix[1][0];
		processedMatrix[1][1] = matrix[1][1];
		processedMatrix[1][2] = matrix[1][2];
		processedMatrix[1][3] = 0;
		// row 2 - last column has no translation
		processedMatrix[2][0] = matrix[2][0];
		processedMatrix[2][1] = matrix[2][1];
		processedMatrix[2][2] = matrix[2][2];
		processedMatrix[2][3] = 0;
		// row 3 - last column has no translation
		processedMatrix[3][0] = matrix[3][0];
		processedMatrix[3][1] = matrix[3][1];
		processedMatrix[3][2] = matrix[3][2];
		processedMatrix[3][3] = 1;

		// We must now ensure that the upper 3x3 matrix is a unitary rotation matrix. (i.e. the length of any row or column is 1)
		// Since the only allowed matrices here are rotations and UNIFORM scales, 
		// we can do this by finding the length of any one row or column (they will all be the same)
		// and dividing each component of the upper 3x3 matrix by that length.
		// So choose ANY column or row and we can get K = 1 / sqrt(a^2 + b^2 + c^2)
		float normFactor = 1 / sqrt( matrix[0][0] * matrix[0][0] + matrix[1][0] * matrix[1][0] + matrix[2][0] * matrix[2][0] );
		// loop through upper 3 rows
		for( int row = 0; row < 3; row++ )
		{
			// loop through upper 3 columns
			for( int col = 0; col < 3; col++ )
			{
				// normalize this component of upper 3x3 matrix
				processedMatrix[row][col] *= normFactor;
			}
		}

		// the matrix we push onto the geometry stack should be the top of the stack multiplied by the new transform (on the right)
		GzMatrix xformProduct;
		matrixMultiply( render->Xnorm[render->matlevel - 1], processedMatrix, xformProduct );

		// copy the matrix into the geometry stack
		memcpy( render->Xnorm[render->matlevel], xformProduct, sizeof( GzMatrix ) );
	}

	return GZ_SUCCESS;
}

int GzPopMatrix(GzRender *render)
{
/*
- pop a matrix off the Ximage stack
- check for stack underflow
*/
	if( !render || render->matlevel == EMPTY_STACK )
		return GZ_FAILURE;

	// a matrix hss been popped off of the stack
	render->matlevel--;

	return GZ_SUCCESS;
}


int GzPutAttribute(GzRender	*render, int numAttributes, GzToken	*nameList, 
	GzPointer	*valueList) /* void** valuelist */
{
/*
- set renderer attribute states (e.g.: GZ_RGB_COLOR default color)
- later set shaders, interpolaters, texture maps, and lights
*/
	
	// check for bad pointers
	if( !render || !nameList || !valueList )
		return GZ_FAILURE;

	for( int i = 0; i < numAttributes; i++ )
	{
		switch( nameList[i] )
		{
		case GZ_RGB_COLOR: // set default flat-shader color
			GzColor * colorPtr;
			colorPtr = ( static_cast<GzColor *>( valueList[i] ) );
			// assign the color values in valueList to render variable
			render->flatcolor[RED] = ( *colorPtr )[RED];
			render->flatcolor[GREEN] = ( *colorPtr )[GREEN];
			render->flatcolor[BLUE] = ( *colorPtr )[BLUE];
			break;
		case GZ_INTERPOLATE: // shader interpolation mode 
			int * interpModePtr;
			interpModePtr = ( static_cast<int *>( valueList[i] ) );
			render->interp_mode = *interpModePtr;
			break;
		case GZ_DIRECTIONAL_LIGHT: // add a directional light
			// only add a light if we have room for another one
			if( render->numlights < MAX_LIGHTS )
			{
				GzColor * directionalLight;
				directionalLight = ( static_cast<GzColor *>( valueList[i] ) );
				render->lights[render->numlights].color[RED] = ( *directionalLight )[RED];
				render->lights[render->numlights].color[GREEN] = ( *directionalLight )[GREEN];
				render->lights[render->numlights].color[BLUE] = ( *directionalLight )[BLUE];

				// another light has been added
				render->numlights++;
			}
			break;
		case GZ_AMBIENT_LIGHT: // set ambient light color
			GzColor * ambientLight;
			ambientLight = ( static_cast<GzColor *>( valueList[i] ) );
			render->ambientlight.color[RED] = ( *ambientLight )[RED];
			render->ambientlight.color[GREEN] = ( *ambientLight )[GREEN];
			render->ambientlight.color[BLUE] = ( *ambientLight )[BLUE];
			break;
		case GZ_AMBIENT_COEFFICIENT: // Ka ambient reflectance coef's
			GzColor * coeffKa;
			coeffKa = ( static_cast<GzColor *>( valueList[i] ) );
			render->Ka[RED] = ( *coeffKa )[RED];
			render->Ka[GREEN] = ( *coeffKa )[GREEN];
			render->Ka[BLUE] = ( *coeffKa )[BLUE];
			break;
		case GZ_DIFFUSE_COEFFICIENT: // Kd diffuse reflectance coef's
			GzColor * coeffKd;
			coeffKd = ( static_cast<GzColor *>( valueList[i] ) );
			render->Kd[RED] = ( *coeffKd )[RED];
			render->Kd[GREEN] = ( *coeffKd )[GREEN];
			render->Kd[BLUE] = ( *coeffKd )[BLUE];
			break;
		case GZ_SPECULAR_COEFFICIENT: // Ks ambient reflectance coef's
			GzColor * coeffKs;
			coeffKs = ( static_cast<GzColor *>( valueList[i] ) );
			render->Ks[RED] = ( *coeffKs )[RED];
			render->Ks[GREEN] = ( *coeffKs )[GREEN];
			render->Ks[BLUE] = ( *coeffKs )[BLUE];
			break; 
		case GZ_DISTRIBUTION_COEFFICIENT: // specular power
			float * specPower;
			specPower = ( static_cast<float *>( valueList[i] ) );
			render->spec = *specPower;
			break;
		}
	}

	return GZ_SUCCESS;
}

int GzPutTriangle(GzRender	*render, int numParts, GzToken *nameList, 
				  GzPointer	*valueList)
/* numParts : how many names and values */
{
/*  
- pass in a triangle description with tokens and values corresponding to 
      GZ_POSITION: 3 vert positions in model space 
	  GZ_NORMAL: 3 vert normals in model space
	  GZ_TEXTURE_INDEX: u,v texture coordinates for 3 verts (not handled in this assignment)
- Xform positions of verts  
- Clip - just discard any triangle with verts behind view plane 
       - test for triangles with all three verts off-screen 
- invoke triangle rasterizer  
TRANSLATION:
Similar to GzPutAttribute but check for GZ_POSITION and GZ_NULL_TOKEN in nameList
Takes in a list of triangles and transforms each vertex from model space to screen space.
Then uses the scan-line or LEE method of rasterizing.
Then calls GzPutDisplay() to draw those pixels to the display.
*/ 
	// check for bad pointers
	if( !render || !nameList || !valueList )
		return GZ_FAILURE;

	// allocate space for the three vertices of this triangle in screen space
	GzCoord * screenSpaceVerts = ( GzCoord * )malloc( 3 * sizeof( GzCoord ) );
	
	// prepare for pointers to 3 vertices and 3 normals
	GzCoord * modelSpaceVerts = 0;
	GzCoord * modelSpaceNormals = 0;

	for( int i = 0; i < numParts; i++ )
	{
		switch( nameList[i] )
		{
		case GZ_NULL_TOKEN:
			// do nothing - no values
			break;
		case GZ_POSITION:
			modelSpaceVerts = static_cast<GzCoord *>( valueList[i] );
			
			bool triBehindViewPlane;
			triBehindViewPlane = false;
			
			for( int vertIdx = 0; vertIdx < 3; vertIdx++ )
			{
				// first we need to transform the position into screen space
				posVectorToScreenSpace( render->Ximage[render->matlevel], modelSpaceVerts[vertIdx], screenSpaceVerts[vertIdx] );

				// discard entire triangle if this vert is behind the view plane
				if( screenSpaceVerts[vertIdx][Z] < 0 )
				{
					triBehindViewPlane = true;
					break;
				}
			}
			
			// we don't need to rasterize this triangle if it's behind the view plane or if it's completely outside of the image plane
			if( triBehindViewPlane || triangleOutsideImagePlane( render, screenSpaceVerts ) )
				return GZ_SUCCESS;
			
			break; // end case: GZ_POSITION
		case GZ_NORMAL:
			modelSpaceNormals = static_cast<GzCoord *>( valueList[i] );
			break; // end case: GZ_NORMAL
		case GZ_TEXTURE_INDEX:
			break; // end case: GZ_TEXTURE_INDEX
		} // end switch( nameList[i] )
	} // end loop over numparts

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
		if( !rasterizeLEE( render, screenSpaceVerts ) )
			return GZ_FAILURE;
		break;
	// unrecognized rasterization method
	default:
		AfxMessageBox( "Error: unknown rasterization method!!!\n" );
		return GZ_FAILURE;
	} // end switch( rasterizeMethod )

	// now clean up after ourselves - free the 
	free( screenSpaceVerts );
	screenSpaceVerts = 0;

	return GZ_SUCCESS;
}

/* NOT part of API - just for general assistance */

short	ctoi(float color)		/* convert float color to GzIntensity short */
{
  return(short)((int)(color * ((1 << 12) - 1)));
}

/*** BEGIN HW2 FUNCTIONS ***/

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

bool crossProd( GzCoord result, const Edge edge1, const Edge edge2 )
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
	result[X] = vec1[Y] * vec2[Z] - vec1[Z] * vec2[Y];
	result[Y] = -( vec1[X] * vec2[Z] - vec1[Z] * vec2[X] );
	result[Z] = vec1[X] * vec2[Y] - vec1[Y] * vec2[X];

	return true;
}

bool rasterizeLEE( GzRender * render, GzCoord * verts )
{
	// get bounding box around triangle
	float minX, maxX, minY, maxY;
	if( !getTriBoundingBox( &minX, &maxX, &minY, &maxY, verts ) )
		return false;

	// use convention of orienting all edges to point in counter-clockwise direction
	Edge edges[3];
	if( !orderTriVertsCCW( edges, verts ) )
		return false;	

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
	if( !( crossProd( crossProduct, edges[0], edges[1] ) ) )
		return false;

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
				return false;
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
					return false;
				}
			}
		} // end column for loop (X)
	} // end row for loop (Y)

	return true;
}

/*** END HW2 FUNCTIONS ***/

/*** BEGIN HW3 FUNCTIONS ***/

bool clearStack( GzRender * render )
{
	if( !render )
		return false;

	render->matlevel = EMPTY_STACK;
	return true;
}

bool matrixMultiply( const GzMatrix mat1, const GzMatrix mat2, GzMatrix matProduct )
{
	/* 
	 * Recall that when we multiply two 4x4 matrices A & B:
	 *		( AB )i,j = summation for k = 1 to k = 4 of ( A )i,k * ( B )k,j
	 */

	// initialize all elements of the matrix to 0
	for( int row = 0; row < 4; row++ )
	{
		for( int col = 0; col < 4; col++ )
		{
			matProduct[row][col] = 0;
		}
	}

	for( int row = 0; row < 4; row++ )
	{
		for( int col = 0; col < 4; col++ )
		{
			for( int sumIdx = 0; sumIdx < 4; sumIdx++ )
			{
				matProduct[row][col] += mat1[row][sumIdx] * mat2[sumIdx][col];
			}
		}
	}

	return true;
}

bool posVectorToScreenSpace( const GzMatrix mat, const GzCoord vector, GzCoord result )
{
	float tempResult[4];
	float tempVector[4];

	tempVector[X] = vector[X];
	tempVector[Y] = vector[Y];
	tempVector[Z] = vector[Z];
	tempVector[3] = 1;

	for( int i = 0; i < 4; i++ )
		tempResult[i] = 0;
	
	for( int idx = 0; idx < 4; idx++ )
	{
		for( int sumIdx = 0; sumIdx < 4; sumIdx++ )
		{
			tempResult[idx] += mat[idx][sumIdx] * tempVector[sumIdx];
		}
	}

	result[X] = tempResult[X] / tempResult[3];
	result[Y] = tempResult[Y] / tempResult[3];
	result[Z] = tempResult[Z] / tempResult[3];

	return true;
}

float vectorDot( const GzCoord vec1, const GzCoord vec2 )
{
	return vec1[X] * vec2[X] + vec1[Y] * vec2[Y] + vec1[Z] * vec2[Z];
}

bool vectorCross( const GzCoord vec1, const GzCoord vec2, GzCoord crossProduct )
{
	crossProduct[X] = vec1[Y] * vec2[Z] - vec1[Z] * vec2[Y];
	crossProduct[Y] = vec1[Z] * vec2[X] - vec1[X] * vec2[Z];
	crossProduct[Z] = vec1[X] * vec2[Y] - vec1[Y] * vec2[X];
	return true;
}

bool negateVector( const GzCoord origVec, GzCoord negatedVec )
{
	negatedVec[X] = -origVec[X];
	negatedVec[Y] = -origVec[Y];
	negatedVec[Z] = -origVec[Z];

	return true;
}

bool vectorSub( const GzCoord vec1, const GzCoord vec2, GzCoord difference )
{
	difference[X] = vec1[X] - vec2[X];
	difference[Y] = vec1[Y] - vec2[Y];
	difference[Z] = vec1[Z] - vec2[Z];
	return true;
}

bool vectorScale( const GzCoord vector, float scaleFactor, GzCoord result )
{
	result[X] = vector[X] * scaleFactor;
	result[Y] = vector[Y] * scaleFactor;
	result[Z] = vector[Z] * scaleFactor;
	return true;
}

bool normalize( GzCoord vector )
{
	float magnitude = sqrt( vector[X] * vector[X] + vector[Y] * vector[Y] + vector[Z] * vector[Z] );
	vector[X] /= magnitude;
	vector[Y] /= magnitude;
	vector[Z] /= magnitude;
	return true;
}

bool constructXsp( GzRender * render )
{
	// check for bad pointers
	if( !render || !render->display )
		return false;

	// NOTE: camera should be initialized FIRST!!!

	/* SETUP Xsp 
	 * Format:
	 *	xs/2	0		0		xs/2
	 *	0		-ys/2	0		ys/2
	 *	0		0		Zmax/d	0
	 *	0		0		0		1
	 *
	 * Note that Zmax is the highest possible Z-value (e.g. depth value).
	 * d comes from the camera's field of view [1/d = tan( FOV /2 ), so d = 1 / ( tan( FOV / 2 ) )]. 
	 */

	// since FOV is in degrees, we must first convert to radians.
	float radianFOV = ( float )( render->camera.FOV * ( PI / 180 ) );
	float d = 1 / tan( radianFOV / 2 );

	// row 0
	render->Xsp[0][0] = render->display->xres / ( float )2; // upcast the denominator to maintain accuracy
	render->Xsp[0][1] = 0;
	render->Xsp[0][2] = 0;
	render->Xsp[0][3] = render->display->xres / ( float )2; // upcast the denominator to maintain accuracy

	// row 1
	render->Xsp[1][0] = 0;
	render->Xsp[1][1] = -render->display->yres / ( float )2; // upcast the denominator to maintain accuracy
	render->Xsp[1][2] = 0;
	render->Xsp[1][3] = render->display->yres / ( float )2; // upcast the denominator to maintain accuracy

	// row 2
	render->Xsp[2][0] = 0;
	render->Xsp[2][1] = 0;
	render->Xsp[2][2] = INT_MAX / d;
	render->Xsp[2][3] = 0;

	// row 3
	render->Xsp[3][0] = 0;
	render->Xsp[3][1] = 0;
	render->Xsp[3][2] = 0;
	render->Xsp[3][3] = 1;
	/* DONE SETTING UP Xsp */

	return true;
}

bool constructCameraXforms( GzRender * render )
{
	// check for bad pointers
	if( !render || !render->display )
		return false;
	
	// NOTE: the camera MUST be initialized BEFORE this function is called!!!

	// recall that d comes from the camera's field of view [1/d = tan( FOV /2 ), so d = 1 / ( tan( FOV / 2 ) )]. 
	// since FOV is in degrees, we must first convert to radians.
	float radianFOV = ( float )( render->camera.FOV * ( PI / 180 ) );
	float d = 1 / tan( radianFOV / 2 );

	// Construct Xpi
	/*
	 * Format of Xpi:
	 *	1	0	0	0
	 *	0	1	0	0
	 *	0	0	1	0
	 *	0	0	1/d	1
	 */
	// row 0
	render->camera.Xpi[0][0] = 1;
	render->camera.Xpi[0][1] = 0;
	render->camera.Xpi[0][2] = 0;
	render->camera.Xpi[0][3] = 0;

	// row 1
	render->camera.Xpi[1][0] = 0;
	render->camera.Xpi[1][1] = 1;
	render->camera.Xpi[1][2] = 0;
	render->camera.Xpi[1][3] = 0;

	// row 2
	render->camera.Xpi[2][0] = 0;
	render->camera.Xpi[2][1] = 0;
	render->camera.Xpi[2][2] = 1;
	render->camera.Xpi[2][3] = 0;

	// row 3
	render->camera.Xpi[3][0] = 0;
	render->camera.Xpi[3][1] = 0;
	render->camera.Xpi[3][2] = 1 / d;
	render->camera.Xpi[3][3] = 1;

	// Construct Xiw
	/*
	 * We'll build Xiw by building Xwi and taking its inverse.
	 * We already know the camera position (c) and look at point (l) in world space.
	 * The camera's z axis in world space is therefore 
	 *		Z = ( c->l ) / || ( c->l ) ||. 
	 *		Observe that c->l is ( l - c ).
	 * We know the camera's up vector (up) in world space.
	 * up' (the up vector in image space) is therefore
	 *		up' = up - dot( up, Z ) * Z
	 * Thus the camera's Y vector in image space is
	 *		Y = up' / || up' ||
	 * Finally, we use a left-handed coordinate system for the camera. Therefore
	 *		X = ( Y x Z )
	 *
	 * So we now have our Xwi transform:
	 *		Xx	Yx	Zx	Cx
	 *		Xy	Yy	Zy	Cy
	 *		Xz	Yz	Zz	Cz
	 *		0	0	0	1
	 * Observe that this matrix has the form 
	 *		Xwi = T * R
	 * Thus we can derive Xiw:
	 *		Xiw = inv( Xwi ) = inv( T * R ) = inv( R ) * inv( T )
	 * That is, Xiw looks like this:
	 *		Xx	Xy	Xz	dot( -X, c )
	 *		Yx	Yy	Yz	dot( -Y, c )
	 *		Zx	Zy	Zz	dot( -Z, c )
	 *		0	0	0	1
	 * Note that for any vectors A and B, dot( -A, B ) = -dot( A, B ).
	 */

	GzCoord camX, camY, camZ, upDotCamZTimesCamZ;

	// first build camera Z vector
	vectorSub( render->camera.lookat, render->camera.position, camZ );
	normalize( camZ );

	// now build camera Y vector
	float upDotCamZ = vectorDot( render->camera.worldup, camZ );
	vectorScale( camZ, upDotCamZ, upDotCamZTimesCamZ );
	vectorSub( render->camera.worldup, upDotCamZTimesCamZ, camY );
	normalize( camY );

	// now build camera X vector
	vectorCross( camY, camZ, camX ); // observe that crossing 2 unit vectors yields a unit vector, so we don't need to normalize

	// row 0
	render->camera.Xiw[0][0] = camX[X];
	render->camera.Xiw[0][1] = camX[Y];
	render->camera.Xiw[0][2] = camX[Z];
	render->camera.Xiw[0][3] = -vectorDot( camX, render->camera.position );

	// row 1
	render->camera.Xiw[1][0] = camY[X];
	render->camera.Xiw[1][1] = camY[Y];
	render->camera.Xiw[1][2] = camY[Z];
	render->camera.Xiw[1][3] = -vectorDot( camY, render->camera.position );

	// row 2
	render->camera.Xiw[2][0] = camZ[X];
	render->camera.Xiw[2][1] = camZ[Y];
	render->camera.Xiw[2][2] = camZ[Z];
	render->camera.Xiw[2][3] = -vectorDot( camZ, render->camera.position );

	// row 3
	render->camera.Xiw[3][0] = 0;
	render->camera.Xiw[3][1] = 0;
	render->camera.Xiw[3][2] = 0;
	render->camera.Xiw[3][3] = 1;

	return true;
}

bool triangleOutsideImagePlane( GzRender * render, GzCoord * verts )
{
	// NOTE: verts should be give in screen space coordinates
	if( !render || !render->display || !verts )
		return true; // no image plane is available!

	// all vertices are above image plane (e.g all Y coords are < 0)
	if( verts[0][Y] < 0 && verts[1][Y] < 0 && verts[2][Y] < 0 )
		return true;
	// all vertices are below image plane (e.g all Y coords are > yres)
	else if( verts[0][Y] > render->display->yres && verts[1][Y] > render->display->yres && verts[2][Y] > render->display->yres )
		return true;
	// all vertices are to left of image plane (e.g all X coords are < 0)
	else if( verts[0][X] < 0 && verts[1][X] < 0 && verts[2][X] < 0 )
		return true;
	// all vertices are to right of image plane (e.g all X coords are > xres)
	else if( verts[0][X] > render->display->xres && verts[1][X] > render->display->xres && verts[2][X] > render->display->xres )
		return true;
	// we can't determine the triangle is completely outside the image plane
	else
		return false;
}