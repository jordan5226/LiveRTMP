#pragma once

#include "XDataThread.h"

class IXVideoCapture : public XDataThread
{
public:
	int	m_iSrcWidth = 0;
	int	m_iSrcHeight = 0;
	int	m_iFPS = 0;

public:
	virtual ~IXVideoCapture();
	virtual bool Init( const int iCamIdx = 0 ) = 0;
	virtual bool Init( const char* lpszInUrl ) = 0;
	virtual void StopRecord() = 0;

	static IXVideoCapture* Get( unsigned char idx = 0 );

protected:
	IXVideoCapture();
};

