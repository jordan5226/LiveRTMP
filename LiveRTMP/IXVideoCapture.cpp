#include "IXVideoCapture.h"
#include <iostream>
#include <string>
#include <opencv2\highgui.hpp>

using namespace std;
using namespace cv;

#if _DEBUG
#pragma comment( lib, "opencv_world470d.lib" )
#else
#pragma comment( lib, "opencv_world470.lib" )
#endif

/// <summary>
/// CXVideoCapture Definition
/// </summary>
class CXVideoCapture : public IXVideoCapture
{
protected:
	VideoCapture	m_cam;

public:
	virtual bool Init( const int iCamIdx = 0 ) override;
	virtual bool Init( const char* lpszInUrl ) override;
	virtual void StopRecord() override;

	virtual void run();

};

/// <summary>
/// CXVideoCapture Implement
/// </summary>
bool CXVideoCapture::Init( const int iCamIdx /*= 0 */ )
{
	/// 1. Open local camera stream by opencv
	m_cam.open( iCamIdx );
	if( !m_cam.isOpened() )
	{
		cout << "CXVideoCapture::Init - Open camera failed!" << endl;
		return false;
	}

	cout << "CXVideoCapture::Init - Open camera success! ( CamIdx: " << iCamIdx << " )" << endl << endl;

	// Get Info
	m_iSrcWidth  = m_cam.get( CAP_PROP_FRAME_WIDTH );
	m_iSrcHeight = m_cam.get( CAP_PROP_FRAME_HEIGHT );
	m_iFPS       = m_cam.get( CAP_PROP_FPS );

	if( m_iFPS == 0 ) m_iFPS = 25;

	// Start video recording thread
	this->Start();

	return true;
}

bool CXVideoCapture::Init( const char* lpszInUrl )
{
	Mat matFrame;

	/// 1. Open RTSP video stream by opencv
	m_cam.open( lpszInUrl );
	if( !m_cam.isOpened() )
	{
		cout << "CXVideoCapture::Init - Open camera failed!" << endl;
		return false;
	}

	cout << "CXVideoCapture::Init - Open camera success! ( URL: '" << lpszInUrl << "' )" << endl << endl;

	// Get FPS
	int iFPS = m_cam.get( CAP_PROP_FPS );
	time_t tStart = 0, tEnd = 0;

	time( &tStart );

	for( int i = 0; i < 100; ++i )
	{
		m_cam.read( matFrame );
		cout << "\rCalculating FPS: " << i + 1 << "%";
	}

	time( &tEnd );

	double lfSec = difftime( tEnd, tStart );
	iFPS = 100 / lfSec;

	cout << endl << "FPS: " << iFPS << endl;

	// Get Info
	m_iSrcWidth  = m_cam.get( CAP_PROP_FRAME_WIDTH );
	m_iSrcHeight = m_cam.get( CAP_PROP_FRAME_HEIGHT );

	// Start video recording thread
	this->Start();

	return true;
}

void CXVideoCapture::StopRecord()
{
	if( m_cam.isOpened() )
		m_cam.release();
}

void CXVideoCapture::run()
{
	Mat matFrame;

	while( !m_bExit )
	{
		if( !m_cam.read( matFrame ) )
		{
			msleep( 1 );
			continue;
		}

		if( matFrame.empty() )
		{
			msleep( 1 );
			continue;
		}

		XDATA data( (char*)matFrame.data, matFrame.cols * matFrame.rows * matFrame.elemSize(), ::GetCurTime() );
		this->Push( data );
	}
}

/// <summary>
/// IXVideoCapture Implementation
/// </summary>
IXVideoCapture::IXVideoCapture()
{

}

IXVideoCapture::~IXVideoCapture()
{

}

IXVideoCapture* IXVideoCapture::Get( unsigned char idx/* = 0*/ )
{
	static CXVideoCapture oRec[ 255 ];
	return &oRec[ idx ];
}

