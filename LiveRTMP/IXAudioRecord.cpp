#include "IXAudioRecord.h"
#include <QAudioSource>
#include <QMediaDevices>
#include <QIODevice>
#include <QMutex>
#include <iostream>
#include <list>

using namespace std;

/// <summary>
/// CXAudioRecord Definition
/// </summary>
class CXAudioRecord : public IXAudioRecord
{
protected:
	QAudioSource*		m_pAudSrc = NULL;
	QIODevice*			m_pIODev = NULL;
	QAudioFormat		m_audFmt;

public:
	virtual bool		StartRecord( XSAMPLE_FORMAT eSampleFormat = XSAMPLE_FORMAT_INT16, int iNBSamples = 1024, int iSampleRate = 48000, int iChannels = 1 ) override;	// Start record
	virtual bool		StopRecord() override;	// Stop record
	virtual void		run();
};

/// <summary>
/// CXAudioRecord Implementation
/// </summary>
bool CXAudioRecord::StartRecord( XSAMPLE_FORMAT eSampleFormat/* = XSAMPLE_FORMAT_INT16*/, int iNBSamples/* = 1024*/, int iSampleRate/* = 48000*/, int iChannels/* = 1*/ )
{
	this->StopRecord();

	// Set Data
	m_eSampleFormat = eSampleFormat;
	m_iNBSamples = iNBSamples;
	m_iSampleRate = iSampleRate;
	m_iChannels = iChannels;

	switch( m_eSampleFormat )
	{
	case IXAudioRecord::XSAMPLE_FORMAT_UINT8:
		m_iSampleSize = 1;
		break;
	case IXAudioRecord::XSAMPLE_FORMAT_INT16:
		m_iSampleSize = 2;
		break;
	case IXAudioRecord::XSAMPLE_FORMAT_INT32:
		m_iSampleSize = 4;
		break;
	case IXAudioRecord::XSAMPLE_FORMAT_FLOAT:
		m_iSampleSize = 4;
		break;
	case IXAudioRecord::XSAMPLE_FORMAT_NSAMPLEFORMATS:
		m_iSampleSize = 6;
		break;
	default:
		m_iSampleSize = 2;
		break;
	}

	// Start recording audio
	m_audFmt.setSampleRate( iSampleRate );
	m_audFmt.setSampleFormat( ( QAudioFormat::SampleFormat )m_eSampleFormat );
	m_audFmt.setChannelCount( iChannels );

	QAudioDevice audDev = QMediaDevices::defaultAudioInput();   // Original: QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
	if( !audDev.isFormatSupported( m_audFmt ) )
	{
		cout << "Audio format is not support." << endl;
		m_audFmt = audDev.preferredFormat();
	}

	// Start Thread
	this->Start();

	//
	for( int i = 0; i < 3; ++i )
	{
		wait( 500 );

		if( m_pAudSrc && m_pIODev )
			return true;
		else
			continue;
	}

	return false;
}

bool CXAudioRecord::StopRecord()
{
	// Stop Thread
	this->Stop();

	//
	if( m_pAudSrc )
		m_pAudSrc->stop();

	if( m_pIODev )
		m_pIODev->close();

	m_pAudSrc = NULL;

	return true;
}

void CXAudioRecord::run()
{
	m_pAudSrc = new QAudioSource( m_audFmt ); // Original: QAudioInput
	if( !m_pAudSrc )
	{
		cout << "Initial QAudioSource failed!" << endl;
		return;
	}

	m_pIODev = m_pAudSrc->start();  // Start record audio
	if( !m_pIODev )
	{
		cout << "QAudioSource start failed!" << endl;
		return;
	}

	//
	int iReadSize = m_iNBSamples * m_iChannels * m_iSampleSize; // 一次讀取一幀音頻的Size(Bytes)
	int iSize = 0, iLen = 0;
	char* pBuf = new char[ iReadSize ];

	while( !m_bExit )
	{
		/// Read record audio
		// 一次讀取一幀音頻
		qsizetype size = m_pAudSrc->bytesAvailable();
		if( size < iReadSize )
		{
			QThread::msleep( 10 );
			continue;
		}
		/*else if( size > iReadSize )
		{
			if( pBuf )
				delete[] pBuf;

			pBuf = new char[ size ];
			iReadSize = size;
		}*/

		// Read
		iSize = 0;

		while( iSize != iReadSize )
		{
			iLen = m_pIODev->read( pBuf + iSize, iReadSize - iSize );
			if( iLen < 0 )
				break;

			iSize += iLen;
		}

		if( iSize != iReadSize )
			continue;

		// Already read audio of 1 frame
		this->Push( XDATA( pBuf, iReadSize, ::GetCurTime() ) );
	}

	//
	delete[] pBuf;
	pBuf = NULL;
}

/// <summary>
/// IXAudioRecord Implementation
/// </summary>
IXAudioRecord::IXAudioRecord()
{

}

IXAudioRecord::~IXAudioRecord()
{

}

IXAudioRecord* IXAudioRecord::Get( XAUDTYPE eType /*= XAUDTYPE_QT*/, unsigned char idx /*= 0 */ )
{
	static CXAudioRecord oRec[ 255 ];
	return &oRec[ idx ];
}
