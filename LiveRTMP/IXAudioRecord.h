#pragma once

#include "XDataThread.h"

/// <summary>
/// 
/// </summary>
class IXAudioRecord : public XDataThread
{
public:
	enum XAUDTYPE
	{
		XAUDTYPE_QT = 0,
	};

	enum XSAMPLE_FORMAT
	{
		XSAMPLE_FORMAT_UNKNOWN,
		XSAMPLE_FORMAT_UINT8,
		XSAMPLE_FORMAT_INT16,
		XSAMPLE_FORMAT_INT32,
		XSAMPLE_FORMAT_FLOAT,
		XSAMPLE_FORMAT_NSAMPLEFORMATS
	};

protected:
	XSAMPLE_FORMAT	m_eSampleFormat = XSAMPLE_FORMAT_INT16;		// Sample Format
	int				m_iNBSamples = 1024;						// Number of audio samples (per channel) described by this frame.
	int				m_iSampleRate = 48000;						// Sample rate
	int				m_iChannels = 1;							// Channel numbers
	int				m_iSampleSize = 2;							// Sample data size (Bytes)


public:
	virtual ~IXAudioRecord();
	virtual bool		StartRecord( XSAMPLE_FORMAT eSampleFormat = XSAMPLE_FORMAT_INT16, int iNBSamples = 1024, int iSampleRate = 48000, int iChannels = 1 ) = 0;	// Start record
	virtual bool		StopRecord() = 0;		// Stop record

	static IXAudioRecord* Get( XAUDTYPE eType = XAUDTYPE_QT, unsigned char idx = 0 );

protected:
	IXAudioRecord();
};
