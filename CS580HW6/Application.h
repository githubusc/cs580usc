// Application.h: interface for the Application class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_APPLICATION_H__3387B79A_B69F_491D_B782_81D9CAFAAB0F__INCLUDED_)
#define AFX_APPLICATION_H__3387B79A_B69F_491D_B782_81D9CAFAAB0F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Gz.h"
#include "disp.h"
#include "rend.h"

#define	AAKERNEL_SIZE	6

class Application  
{
public:
	Application();
	virtual ~Application();
	
public:
	GzDisplay* m_pAADisplay;	// the combined (anti-aliased) display that will be written out to the frame buffer
	GzDisplay** m_pDisplay;		// the display - one display needed per anti-aliasing sample
	GzRender**  m_pRender;		// the renderer - one renderer needed per anti-aliasing sample
	GzInput*   m_pUserInput;
	char* m_pFrameBuffer;	// Frame Buffer 
	int   m_nWidth;			// width of Frame Buffer
	int   m_nHeight;		// height of Frame Buffer

	virtual int Render()=0; // Pass user input data and call renderer
};

#endif // !defined(AFX_APPLICATION_H__3387B79A_B69F_491D_B782_81D9CAFAAB0F__INCLUDED_)
