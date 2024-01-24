#pragma once

#include "XData.h"

struct AVFrame;
struct AVPacket;
struct AVCodecContext;

/// <summary>
/// Video/Audio Interface
/// </summary>
class IXMediaEncode
{
// Attributes
public:
	enum X_SAMPLE_FMT
	{
		X_SAMPLE_FMT_NONE = -1,
		X_SAMPLE_FMT_U8,          ///< unsigned 8 bits
		X_SAMPLE_FMT_S16,         ///< signed 16 bits
		X_SAMPLE_FMT_S32,         ///< signed 32 bits
		X_SAMPLE_FMT_FLT,         ///< float
		X_SAMPLE_FMT_DBL,         ///< double

		X_SAMPLE_FMT_U8P,         ///< unsigned 8 bits, planar
		X_SAMPLE_FMT_S16P,        ///< signed 16 bits, planar
		X_SAMPLE_FMT_S32P,        ///< signed 32 bits, planar
		X_SAMPLE_FMT_FLTP,        ///< float, planar
		X_SAMPLE_FMT_DBLP,        ///< double, planar
		X_SAMPLE_FMT_S64,         ///< signed 64 bits
		X_SAMPLE_FMT_S64P,        ///< signed 64 bits, planar

		X_SAMPLE_FMT_NB           ///< Number of sample formats. DO NOT USE if linking dynamically
	};

protected:
	// Video
	int m_iInWidth = 0;
	int m_iInHeight = 0;
	int m_iInPixSize = 0;									// Bytes per pixel
	int m_iOutWidth = 0;
	int m_iOutHeight = 0;

	// Audio
	int m_iNBSamples = 1024;								// Number of audio samples (per channel) described by this frame.
	int m_iSampleRate = 48000;								// Sample rate
	int	m_iChannels = 1;									// Channel numbers
	int	m_iSampleSize = 2;									// Sample data size (Bytes)
	int	m_iInSampleFmt = X_SAMPLE_FMT::X_SAMPLE_FMT_S16;	// Input sample format
	int	m_iOutSampleFmt = X_SAMPLE_FMT::X_SAMPLE_FMT_FLTP;	// Output sample format (Resample)

// Methods
public:
	virtual ~IXMediaEncode();

	virtual void		Close() = 0;

	virtual bool		InitScale( int iInW, int iInH, int iInPixSize, int iOutW, int iOutH ) = 0;	// Initial RGB to YUV scale
	virtual bool		InitResample( int nbSamples, int iSampleRate, int iChannels, int iSampleSize, X_SAMPLE_FMT iInSampleFmt, X_SAMPLE_FMT iOutSampleFmt ) = 0;	// Initial Audio ReSample
	
	virtual bool		InitVideoCodec( int iBitRate, int iFPS ) = 0;	// Initial Video Codec
	virtual bool		InitAudioCodec( int iBitRate ) = 0;				// Initial Audio Codec

	virtual XDATA		RGB2YUV( XDATA dtRGB ) = 0;
	virtual XDATA		EncodeVideo( XDATA dtFrame ) = 0;				// Returned buffer no need to release by user

	virtual XDATA		Resample( XDATA dtAudio ) = 0;
	virtual XDATA		EncodeAudio( XDATA dtFrame ) = 0;				// Returned buffer no need to release by user

	virtual const AVCodecContext* GetVideoCodecContext() = 0;
	virtual const AVCodecContext* GetAudioCodecContext() = 0;

	//
	inline int GetSamplesNumber() { return this->m_iNBSamples; }

	// Factory Create Method
	static IXMediaEncode* Get( unsigned char cIdx = 0 );	// Max 255

protected:
	IXMediaEncode();

};

