#include "IXRtmp.h"
#include <string>

using namespace std;

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <iostream>

#pragma comment( lib, "avcodec.lib" )
#pragma comment( lib, "avformat.lib" )

/// <summary>
/// CXRtmp Definition
/// </summary>
class CXRtmp : public IXRtmp
{
public:
	string					m_strOutUrl = "";
	AVFormatContext*		m_pOutCtx = NULL;			// Output stream RTMP FLV container context
	const AVCodecContext*	m_pVidCodecCtx = NULL;		// Video Encoder Context
	const AVCodecContext*	m_pAudCodecCtx = NULL;		// Audio Encoder Context
	const AVStream*			m_pOutVidStream = NULL;		// Output Video stream
	const AVStream*			m_pOutAudStream = NULL;		// Output Audio stream

public:
	CXRtmp();
	virtual ~CXRtmp();

	virtual bool	Init( const char* lpszOutUrl ) override;
	virtual int		AddStream( const AVCodecContext* lpCtx ) override;
	virtual bool	SendHead() override;
//	virtual bool	SendFrame( AVPacket* lpPac, int iStreamIdx = 0 ) override;
	virtual bool	SendFrame( XDATA dtPac, int iStreamIdx = 0 ) override;
	virtual void	Close() override;
};

/// <summary>
/// CXRtmp Implementation
/// </summary>
CXRtmp::CXRtmp() : IXRtmp()
{

}

CXRtmp::~CXRtmp()
{

}

bool CXRtmp::Init( const char* lpszOutUrl )
{
	this->Close();

	// Create output stream container context
	int iRet = avformat_alloc_output_context2( &m_pOutCtx, NULL, "flv", lpszOutUrl );
	if( iRet != 0 )
	{
		char szBuf[ 1024 ] = { 0 };
		av_strerror( iRet, szBuf, sizeof( szBuf ) - 1 );
		cout << szBuf << endl;
		return false;
	}

	m_strOutUrl = lpszOutUrl;

	if( m_strOutUrl.empty() )
	{
		cout << "Allocate output url buffer failed!" << endl;
		return false;
	}

	return true;
}

int CXRtmp::AddStream( const AVCodecContext* lpCtx )
{
	if( !m_pOutCtx || !lpCtx )
		return -1;

	// Initial output video stream
	AVStream* pOutStream = avformat_new_stream( m_pOutCtx, NULL );
	if( !pOutStream )
	{
		cout << "avformat_new_stream failed!" << endl;
		return -1;
	}

	avcodec_parameters_from_context( pOutStream->codecpar, lpCtx );	// Copy parameters from encoder context
	pOutStream->codecpar->codec_tag = 0;

	if( lpCtx->codec_type == AVMEDIA_TYPE_VIDEO )
	{
		this->m_pVidCodecCtx = lpCtx;
		this->m_pOutVidStream = pOutStream;
	}
	else if( lpCtx->codec_type == AVMEDIA_TYPE_AUDIO )
	{
		this->m_pAudCodecCtx = lpCtx;
		this->m_pOutAudStream = pOutStream;
	}

	cout << "======================" << endl;
	av_dump_format( m_pOutCtx, 0, m_strOutUrl.c_str(), 1 );
	cout << "======================" << endl;

	return pOutStream->index;
}

bool CXRtmp::SendHead()
{
	if( !m_pOutCtx )
		return false;

	// Open RTMP network I/O
	int iRet = avio_open( &m_pOutCtx->pb, m_strOutUrl.c_str(), AVIO_FLAG_WRITE );
	if( iRet != 0 )
	{
		char szBuf[ 1024 ] = { 0 };
		av_strerror( iRet, szBuf, sizeof( szBuf ) - 1 );
		cout << "avio_open: " << szBuf << endl;
		return false;
	}

	// Write container header
	iRet = avformat_write_header( m_pOutCtx, NULL );
	if( iRet < 0 )
	{
		char szBuf[ 1024 ] = { 0 };
		av_strerror( iRet, szBuf, sizeof( szBuf ) - 1 );
		cout << "avformat_write_header: " << szBuf << endl;
		return false;
	}

	return true;
}

bool CXRtmp::SendFrame( XDATA dtPac, int iStreamIdx/* = 0*/ )
{
	if( !dtPac.m_pData || dtPac.m_iSize <= 0 || !m_pOutCtx )
		return false;

	AVPacket* lpPac = ( AVPacket* )dtPac.m_pData;

	if( lpPac->size <= 0 || !lpPac->data )
		return false;

	//
	AVRational tbSrc, tbDst;

	lpPac->stream_index = iStreamIdx;

	if( m_pVidCodecCtx && m_pOutVidStream && ( lpPac->stream_index == m_pOutVidStream->index ) )
	{
		tbSrc = m_pVidCodecCtx->time_base;
		tbDst = m_pOutVidStream->time_base;
	}
	else if( m_pAudCodecCtx && m_pOutAudStream && ( lpPac->stream_index == m_pOutAudStream->index ) )
	{
		tbSrc = m_pAudCodecCtx->time_base;
		tbDst = m_pOutAudStream->time_base;
	}
	else
	{
		return false;
	}

	lpPac->pts = av_rescale_q( lpPac->pts, tbSrc, tbDst );
	lpPac->dts = av_rescale_q( lpPac->dts, tbSrc, tbDst );
	lpPac->duration = av_rescale_q( lpPac->duration, tbSrc, tbDst );
	//lpPac->pos = -1;

	int iRet = av_interleaved_write_frame( m_pOutCtx, lpPac );
	if( iRet == 0 )
	{
		//cout << "#" << flush;
	}

	return true;
}

void CXRtmp::Close()
{
	if( m_pOutCtx )
	{
		avformat_close_input( &m_pOutCtx );
		m_pOutVidStream = NULL;
		m_pOutAudStream = NULL;
	}

	m_pVidCodecCtx = NULL;
	m_pAudCodecCtx = NULL;

	m_strOutUrl.clear();
}

/// <summary>
/// IXRtmp Implementation
/// </summary>
IXRtmp::IXRtmp()
{

}

IXRtmp::~IXRtmp()
{

}

IXRtmp* IXRtmp::Get( unsigned char cIdx/* = 0*/ )
{
	static bool bFirst = true;
	static CXRtmp arrRtmp[ 255 ];

	if( bFirst )
	{
		// 初始化網路庫
		avformat_network_init();

		bFirst = false;
	}

	return &arrRtmp[ cIdx ];
}

