// Application6.cpp: implementation of the Application6 class.
//
//////////////////////////////////////////////////////////////////////

/*
 * application test code for homework assignment #5
*/

#include "stdafx.h"
#include "CS580HW.h"
#include "Application6.h"
#include "Gz.h"
#include "disp.h"
#include "rend.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define INFILE  "ppot.asc"
#define OUTFILE "output.ppm"

float AAFilter[AAKERNEL_SIZE][3] =			/* X, Y, coef */
{
	-0.52, 0.38, 0.128,
	0.41, 0.56, 0.119,
	0.27, 0.08, 0.294,
	-0.17, -0.29, 0.249,
	0.58, -0.55, 0.104,
	-0.31, -0.71, 0.106
};

extern int tex_fun(float u, float v, GzColor color); /* image texture function */
extern int ptex_fun(float u, float v, GzColor color); /* procedural texture function */

void shade(GzCoord norm, GzCoord color);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Application6::Application6()
{

}

Application6::~Application6()
{
	
}

int Application6::Initialize()
{
	GzCamera	camera;  
	int		    xRes, yRes, dispClass;	/* display parameters */ 

	GzToken		nameListShader[9]; 	    /* shader attribute names */
	GzPointer   valueListShader[9];		/* shader attribute pointers */
	GzToken     nameListLights[10];		/* light info */
	GzPointer   valueListLights[10];
	GzToken		nameListAntiAliasing[2];	/* anti aliasing attribute names */
	GzPointer	valueListAntiAliasing[2];	/* anti aliasing attribute pointers */
	int			shaderType, interpStyle;
	float		specpower;
	int		status; 
 
	status = 0; 

	/* 
	 * Allocate memory for user input
	 */
	m_pUserInput = new GzInput;

	/* 
	 * initialize the display and the renderer 
	 */ 
 	m_nWidth = 256;		// frame buffer and display width
	m_nHeight = 256;    // frame buffer and display height

/* Translation matrix */
GzMatrix	scale = 
{ 
	3.25,	0.0,	0.0,	0.0, 
	0.0,	3.25,	0.0,	-3.25, 
	0.0,	0.0,	3.25,	3.5, 
	0.0,	0.0,	0.0,	1.0 
}; 
 
GzMatrix	rotateX = 
{ 
	1.0,	0.0,	0.0,	0.0, 
	0.0,	.7071,	.7071,	0.0, 
	0.0,	-.7071,	.7071,	0.0, 
	0.0,	0.0,	0.0,	1.0 
}; 
 
GzMatrix	rotateY = 
{ 
	.866,	0.0,	-0.5,	0.0, 
	0.0,	1.0,	0.0,	0.0, 
	0.5,	0.0,	.866,	0.0, 
	0.0,	0.0,	0.0,	1.0 
}; 

	/* Light */
	GzLight	light1 = { {-0.7071, 0.7071, 0}, {0.5, 0.5, 0.9} };
	GzLight	light2 = { {0, -0.7071, -0.7071}, {0.9, 0.2, 0.3} };
	GzLight	light3 = { {0.7071, 0.0, -0.7071}, {0.2, 0.7, 0.3} };
	GzLight	ambientlight = { {0, 0, 0}, {0.3, 0.3, 0.3} };

	/* Material property */
	GzColor specularCoefficient = { 0.3, 0.3, 0.3 };
	GzColor ambientCoefficient = { 0.1, 0.1, 0.1 };
	GzColor diffuseCoefficient = {0.7, 0.7, 0.7};

	// set up the frame buffer used by the application
	status |= GzNewFrameBuffer(&m_pFrameBuffer, m_nWidth, m_nHeight);

	status |= GzNewDisplay(&m_pAADisplay, GZ_RGBAZ_DISPLAY, m_nWidth, m_nHeight);

	status |= GzGetDisplayParams(m_pAADisplay, &xRes, &yRes, &dispClass); 
	 
	status |= GzInitDisplay(m_pAADisplay); 

	// one renderer and one display need to be set up per anit-aliasing sample - make space for them
	m_pRender = ( GzRender ** )malloc( AAKERNEL_SIZE * sizeof( GzRender * ) );
	m_pDisplay = ( GzDisplay ** )malloc( AAKERNEL_SIZE * sizeof( GzDisplay * ) );

	// set up each renderer and framebuffer
	for( int idx = 0; idx < AAKERNEL_SIZE; idx++ )
	{
		status |= GzNewDisplay(&( m_pDisplay[idx] ), GZ_RGBAZ_DISPLAY, m_nWidth, m_nHeight);

		status |= GzGetDisplayParams(m_pDisplay[idx], &xRes, &yRes, &dispClass); 
		 
		status |= GzInitDisplay(m_pDisplay[idx]); 
	 
		status |= GzNewRender(&( m_pRender[idx] ), GZ_Z_BUFFER_RENDER, m_pDisplay[idx]);

	#if 1 	/* set up app-defined camera if desired, else use camera defaults */
		camera.position[X] = -3;
		camera.position[Y] = -25;
		camera.position[Z] = -4;

		camera.lookat[X] = 7.8;
		camera.lookat[Y] = 0.7;
		camera.lookat[Z] = 6.5;

		camera.worldup[X] = -0.2;
		camera.worldup[Y] = 1.0;
		camera.worldup[Z] = 0.0;

		camera.FOV = 63.7;              /* degrees *              /* degrees */

		status |= GzPutCamera(m_pRender[idx], &camera); 
	#endif 

		/* Start Renderer */
		status |= GzBeginRender(m_pRender[idx]);

	/* 
	  renderer is ready for frame --- define lights and shader at start of frame 
	*/

			/*
			 * Tokens associated with light parameters
			 */
			nameListLights[0] = GZ_DIRECTIONAL_LIGHT;
			valueListLights[0] = (GzPointer)&light1;
			nameListLights[1] = GZ_DIRECTIONAL_LIGHT;
			valueListLights[1] = (GzPointer)&light2;
			nameListLights[2] = GZ_DIRECTIONAL_LIGHT;
			valueListLights[2] = (GzPointer)&light3;
			status |= GzPutAttribute(m_pRender[idx], 3, nameListLights, valueListLights);

			nameListLights[0] = GZ_AMBIENT_LIGHT;
			valueListLights[0] = (GzPointer)&ambientlight;
			status |= GzPutAttribute(m_pRender[idx], 1, nameListLights, valueListLights);

			/*
			 * Tokens associated with shading 
			 */
			nameListShader[0]  = GZ_DIFFUSE_COEFFICIENT;
			valueListShader[0] = (GzPointer)diffuseCoefficient;

		/* 
		* Select either GZ_COLOR or GZ_NORMALS as interpolation mode  
		*/
			nameListShader[1]  = GZ_INTERPOLATE;
			interpStyle = GZ_NORMALS;         /* Phong shading */
			valueListShader[1] = (GzPointer)&interpStyle;

			nameListShader[2]  = GZ_AMBIENT_COEFFICIENT;
			valueListShader[2] = (GzPointer)ambientCoefficient;
			nameListShader[3]  = GZ_SPECULAR_COEFFICIENT;
			valueListShader[3] = (GzPointer)specularCoefficient;
			nameListShader[4]  = GZ_DISTRIBUTION_COEFFICIENT;
			specpower = 32;
			valueListShader[4] = (GzPointer)&specpower;

			nameListShader[5]  = GZ_TEXTURE_MAP;
	#if 0   /* set up null texture function or valid pointer */
			valueListShader[5] = (GzPointer)0;
	#else
			valueListShader[5] = (GzPointer)(tex_fun);	/* or use ptex_fun */
	#endif
			status |= GzPutAttribute(m_pRender[idx], 6, nameListShader, valueListShader);

			/*
			 * Tokens associated with anti-aliasing
			 */
			nameListAntiAliasing[0] = GZ_AASHIFTX;
			valueListAntiAliasing[0] = ( GzPointer )&( AAFilter[idx][X] );
			nameListAntiAliasing[1] = GZ_AASHIFTX;
			valueListAntiAliasing[1]= ( GzPointer )&( AAFilter[idx][Y] );
			status |= GzPutAttribute(m_pRender[idx], 2, nameListAntiAliasing, valueListAntiAliasing);

		status |= GzPushMatrix(m_pRender[idx], scale);  
		status |= GzPushMatrix(m_pRender[idx], rotateY); 
		status |= GzPushMatrix(m_pRender[idx], rotateX); 
	} // end for loop settin up all framebuffers and renderers

	if (status) exit(GZ_FAILURE); 

	if (status) 
		return(GZ_FAILURE); 
	else 
		return(GZ_SUCCESS); 
}

int Application6::Render() 
{
	GzToken		nameListTriangle[3]; 	/* vertex attribute names */
	GzPointer	valueListTriangle[3]; 	/* vertex attribute pointers */
	GzCoord		vertexList[3];	/* vertex position coordinates */ 
	GzCoord		normalList[3];	/* vertex normals */ 
	GzTextureIndex  	uvList[3];		/* vertex texture map indices */ 
	char		dummy[256]; 
	int			status; 


	for( int idx = 0; idx < AAKERNEL_SIZE; idx++ )
	{
		/* Initialize Display */
		status |= GzInitDisplay(m_pDisplay[idx]); 
	}
	
	/* 
	* Tokens associated with triangle vertex values 
	*/ 
	nameListTriangle[0] = GZ_POSITION; 
	nameListTriangle[1] = GZ_NORMAL; 
	nameListTriangle[2] = GZ_TEXTURE_INDEX;  

	// I/O File open
	FILE *infile;
	if( (infile  = fopen( INFILE , "r" )) == NULL )
	{
         AfxMessageBox( "The input file was not opened\n" );
		 return GZ_FAILURE;
	}

	FILE *outfile;
	if( (outfile  = fopen( OUTFILE , "wb" )) == NULL )
	{
         AfxMessageBox( "The output file was not opened\n" );
		 return GZ_FAILURE;
	}
	
	/* 
	* Walk through the list of triangles, set color 
	* and render each triangle 
	*/ 
	while( fscanf(infile, "%s", dummy) == 1) { 	/* read in tri word */
		fscanf(infile, "%f %f %f %f %f %f %f %f", 
		&(vertexList[0][0]), &(vertexList[0][1]),  
		&(vertexList[0][2]), 
		&(normalList[0][0]), &(normalList[0][1]), 	
		&(normalList[0][2]), 
		&(uvList[0][0]), &(uvList[0][1]) ); 
		fscanf(infile, "%f %f %f %f %f %f %f %f", 
		&(vertexList[1][0]), &(vertexList[1][1]), 	
		&(vertexList[1][2]), 
		&(normalList[1][0]), &(normalList[1][1]), 	
		&(normalList[1][2]), 
		&(uvList[1][0]), &(uvList[1][1]) ); 
		fscanf(infile, "%f %f %f %f %f %f %f %f", 
		&(vertexList[2][0]), &(vertexList[2][1]), 	
		&(vertexList[2][2]), 
		&(normalList[2][0]), &(normalList[2][1]), 	
		&(normalList[2][2]), 
		&(uvList[2][0]), &(uvList[2][1]) ); 

		/* 
		 * Set the value pointers to the first vertex of the 	
		 * triangle, then feed it to the renderer 
		 * NOTE: this sequence matches the nameList token sequence
		 */ 
		 valueListTriangle[0] = (GzPointer)vertexList; 
		 valueListTriangle[1] = (GzPointer)normalList; 
		 valueListTriangle[2] = (GzPointer)uvList; 

		 // need to pass the triangle information to EACH renderer
		for( int idx = 0; idx < AAKERNEL_SIZE; idx++ )
		{
			GzPutTriangle(m_pRender[idx], 3, nameListTriangle, valueListTriangle); 
		}
	} 

	// now we can combine the anti-aliasing samples into a single display
	combineDisplays();
	GzFlushDisplay2File(outfile, m_pAADisplay); 	/* write out or update display to file*/
	GzFlushDisplay2FrameBuffer(m_pFrameBuffer, m_pAADisplay);	// write out or update display to frame buffer

	/* 
	 * Close file
	 */ 

	if( fclose( infile ) )
      AfxMessageBox( "The input file was not closed\n" );

	if( fclose( outfile ) )
      AfxMessageBox( "The output file was not closed\n" );
 
	if (status) 
		return(GZ_FAILURE); 
	else 
		return(GZ_SUCCESS); 
}

int Application6::Clean()
{
	/* 
	 * Clean up and exit 
	 */ 
	int	status = 0; 

	for( int idx = 0; idx < AAKERNEL_SIZE; idx++ )
	{
		status |= GzFreeRender(m_pRender[idx]); 
		status |= GzFreeDisplay(m_pDisplay[idx]);
	}

	delete m_pRender;
	delete m_pDisplay;
	delete m_pAADisplay;
	
	if (status) 
		return(GZ_FAILURE); 
	else 
		return(GZ_SUCCESS);
}

int Application6::combineDisplays()
{
	if( !m_pAADisplay )
		return GZ_FAILURE;

	for( int idx = 0; idx < AAKERNEL_SIZE; idx++ )
	{
		if( ! m_pDisplay[idx] )
			return GZ_FAILURE;
	}

	// display rows (Y coords)
	for( int row = 0; row < m_pAADisplay->xres; row++ )
	{
		// display columns (X coords)
		for( int col = 0; col < m_pAADisplay->yres; col++ )
		{
			int fbufIdx = col + ( row * m_pAADisplay->xres );
			m_pAADisplay->fbuf[fbufIdx].red = 0;
			m_pAADisplay->fbuf[fbufIdx].green = 0;
			m_pAADisplay->fbuf[fbufIdx].blue = 0;
			m_pAADisplay->fbuf[fbufIdx].alpha = 0;

			for( int idx = 0; idx < AAKERNEL_SIZE; idx++ )
			{
				m_pAADisplay->fbuf[fbufIdx].red += m_pDisplay[idx]->fbuf[fbufIdx].red * AAFilter[idx][2];
				m_pAADisplay->fbuf[fbufIdx].green += m_pDisplay[idx]->fbuf[fbufIdx].green * AAFilter[idx][2];
				m_pAADisplay->fbuf[fbufIdx].blue += m_pDisplay[idx]->fbuf[fbufIdx].blue * AAFilter[idx][2];
				m_pAADisplay->fbuf[fbufIdx].alpha += m_pDisplay[idx]->fbuf[fbufIdx].alpha * AAFilter[idx][2];
			}
		}
	}

	return GZ_SUCCESS;
}


