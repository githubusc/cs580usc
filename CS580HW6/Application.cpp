// Application.cpp: implementation of the Application class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CS580HW.h"
#include "Application.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Application::Application()
{
	m_pAADisplay = NULL;	// the main display (combines all of the anti-aliasing sample displays)
	m_pDisplay = NULL;		// the displays (one per anti-aliasing sample)
	m_pRender = NULL;		// the renderers (one per anti-aliasing sample)
	m_pUserInput = NULL;
	m_pFrameBuffer = NULL;
}

Application::~Application()
{
	if(m_pFrameBuffer != NULL)
		delete m_pFrameBuffer;
}

