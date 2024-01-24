#include "IXMediaEncode.h"

extern "C"
{
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <iostream>
#include <string>

#if defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64
#include <windows.h>
#endif

using namespace std;

#pragma comment( lib, "swscale.lib" )
#pragma comment( lib, "swresample.lib" )	// for audio resample
#pragma comment( lib, "avcodec.lib" )
#pragma comment( lib, "avutil.lib" )	// for av_strerror

/// <summary>
/// CXMediaEncode Definition
/// </summary>
class CXMediaEncode : public IXMediaEncode
{
private:
	// Video
	int				m_iVideoPTS = 0;
	SwsContext*		m_pSC = NULL;									// Pixel format converter context
	AVFrame*		m_pOutYuv = NULL;								// Output stream YUV data
	AVCodecContext* m_pVideoCodecCtx = NULL;						// Video Encoder Context
	AVPacket		m_pacFrm = { 0 };								// Encoded Frame Data
	uint8_t*		m_pInRGB[ AV_NUM_DATA_POINTERS ] = { 0 };		// input data for sws_scale
	int				m_arrInRGBSize[ AV_NUM_DATA_POINTERS ] = { 0 };	// input data for sws_scale

	// Audio
	int				m_iAudioPTS = 0;
	SwrContext*		m_pAudSC = NULL;								// Audio resample context
	AVFrame*		m_pPCM = NULL;									// resample output PCM
	AVCodecContext* m_pAudCodecCtx = NULL;							// Audio Encoder Context
	AVPacket		m_pacAudio = { 0 };								// Encoded Audio Data
	uint8_t*		m_pInAudioData[ AV_NUM_DATA_POINTERS ] = { 0 };	// input data for swr_convert

public:
	CXMediaEncode();
	virtual ~CXMediaEncode();

	virtual void		Close() override;
	virtual bool		InitScale( int iInW, int iInH, int iInPixSize, int iOutW, int iOutH ) override;
	virtual bool		InitResample( int nbSamples, int iSampleRate, int iChannels, int iSampleSize, X_SAMPLE_FMT iInSampleFmt, X_SAMPLE_FMT iOutSampleFmt ) override;
	virtual bool		InitVideoCodec( int iBitRate, int iFPS ) override;
	virtual bool		InitAudioCodec( int iBitRate ) override;
	virtual XDATA		RGB2YUV( XDATA dtRGB ) override;
	virtual XDATA		EncodeVideo( XDATA dtFrame ) override;
	virtual XDATA		Resample( XDATA dtAudio ) override;
	virtual XDATA		EncodeAudio( XDATA dtFrame ) override;

	virtual const AVCodecContext* GetVideoCodecContext() override;
	virtual const AVCodecContext* GetAudioCodecContext() override;

	static int GetCPUNumber();

private:
	virtual AVCodecContext* CreateCodec( AVCodecID eCodecID );
	virtual bool			OpenCodec( AVCodecContext** lpCtx );
};

///
/// CXMediaEncode Implementation
/// 
CXMediaEncode::CXMediaEncode() : IXMediaEncode()
{

}

CXMediaEncode::~CXMediaEncode()
{

}

bool CXMediaEncode::InitScale( int iInW, int iInH, int iInPixSize, int iOutW, int iOutH )
{
	//
	m_iInWidth = iInW;
	m_iInHeight = iInH;
	m_iInPixSize = iInPixSize;
	m_iOutWidth = iOutW;
	m_iOutHeight = iOutH;

	// Initial image format converter context( RGB to YUV )
	m_pSC = sws_getCachedContext( m_pSC,
		m_iInWidth, m_iInHeight, AV_PIX_FMT_RGB24,
		m_iOutWidth, m_iOutHeight, AV_PIX_FMT_YUV420P,
		SWS_BICUBIC,	// scaled algorithm
		NULL, NULL, NULL );

	if( !m_pSC )
	{
		cout << "sws_getCachedContext failed!" << endl;
		return false;
	}

	// Initial image format converter data ( RGB to YUV )
	// Initial YUV frame buffer for sws_scale
	m_pOutYuv = av_frame_alloc();
	m_pOutYuv->format = AV_PIX_FMT_YUV420P;
	m_pOutYuv->width = m_iOutWidth;
	m_pOutYuv->height = m_iOutHeight;
	m_pOutYuv->pts = 0;

	int iRet = av_frame_get_buffer( m_pOutYuv, 32 );
	if( iRet != 0 )
	{
		char szBuf[ 1024 ] = { 0 };
		av_strerror( iRet, szBuf, sizeof( szBuf ) - 1 );
		cout << szBuf << endl;
		return false;
	}

	return true;
}

bool CXMediaEncode::InitResample( int nbSamples, int iSampleRate, int iChannels, int iSampleSize, X_SAMPLE_FMT iInSampleFmt, X_SAMPLE_FMT iOutSampleFmt )
{
	//
	m_iNBSamples = nbSamples;
	m_iSampleRate = iSampleRate;
	m_iChannels = iChannels;
	m_iSampleSize = iSampleSize;
	m_iInSampleFmt = iInSampleFmt;
	m_iOutSampleFmt = iOutSampleFmt;

	/// 音頻重採樣
	m_pAudSC = swr_alloc_set_opts( m_pAudSC,
		av_get_default_channel_layout( m_iChannels ), ( AVSampleFormat )m_iOutSampleFmt, m_iSampleRate, // 輸出格式 (Output AAC for H264)
		av_get_default_channel_layout( m_iChannels ), ( AVSampleFormat )m_iInSampleFmt, m_iSampleRate, // 輸入格式
		0, NULL );

	if( !m_pAudSC )
	{
		cout << "swr_alloc_set_opts failed!" << endl;
		return false;
	}

	int iRet = swr_init( m_pAudSC );
	if( iRet != 0 )
	{
		char szBuf[ 1024 ] = { 0 };
		av_strerror( iRet, szBuf, sizeof( szBuf ) - 1 );
		cout << szBuf << endl;
		return false;
	}

	/// 音頻重採樣 - 輸出空間分配
	m_pPCM = av_frame_alloc();
	m_pPCM->format = ( AVSampleFormat )m_iOutSampleFmt;
	m_pPCM->channels = m_iChannels;
	m_pPCM->channel_layout = av_get_default_channel_layout( m_iChannels );
	m_pPCM->nb_samples = m_iNBSamples;    // 一幀音頻一通道的採樣數量

	iRet = av_frame_get_buffer( m_pPCM, 0 ); // 真正給PCM分配存儲空間
	if( iRet != 0 )
	{
		char szBuf[ 1024 ] = { 0 };
		av_strerror( iRet, szBuf, sizeof( szBuf ) - 1 );
		cout << szBuf << endl;
		return false;
	}

	return true;
}

bool CXMediaEncode::InitVideoCodec( int iBitRate, int iFPS )
{
	/// Initial Video Encoder
	m_pVideoCodecCtx = this->CreateCodec( AV_CODEC_ID_H264 );
	if( !m_pVideoCodecCtx )
		return false;

	m_pVideoCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;	// global parameter
	m_pVideoCodecCtx->codec_id = AV_CODEC_ID_H264;
	m_pVideoCodecCtx->thread_count = CXMediaEncode::GetCPUNumber() / 2;
	m_pVideoCodecCtx->bit_rate = iBitRate;	// video size per second after encode: (ex: 50 * 1024 * 8 = 50KB)
	m_pVideoCodecCtx->width = m_iOutWidth;
	m_pVideoCodecCtx->height = m_iOutHeight;
	//m_pVideoCodecCtx->time_base = { 1, iFPS };
	m_pVideoCodecCtx->framerate = { iFPS, 1 };
	m_pVideoCodecCtx->gop_size = 50;	// GOP size, 多少幀一個關鍵幀
	m_pVideoCodecCtx->max_b_frames = 0;
	m_pVideoCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	// Open encoder context
	return this->OpenCodec( &m_pVideoCodecCtx );
}

bool CXMediaEncode::InitAudioCodec( int iBitRate )
{
	/// Initialize Audio Encoder
	m_pAudCodecCtx = this->CreateCodec( AV_CODEC_ID_AAC );
	if( !m_pAudCodecCtx )
		return false;

	m_pAudCodecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;	// global parameter
	m_pAudCodecCtx->thread_count = CXMediaEncode::GetCPUNumber() / 2;
	m_pAudCodecCtx->bit_rate = iBitRate;
	m_pAudCodecCtx->sample_rate = m_iSampleRate;
	m_pAudCodecCtx->sample_fmt = ( AVSampleFormat )m_iOutSampleFmt;
	m_pAudCodecCtx->channels = m_iChannels;
	m_pAudCodecCtx->channel_layout = av_get_default_channel_layout( m_iChannels );

	return this->OpenCodec( &m_pAudCodecCtx );
}

void CXMediaEncode::Close()
{
	// Video
	if( m_pSC )
	{
		sws_freeContext( m_pSC );
		m_pSC = NULL;
	}

	if( m_pOutYuv )
	{
		av_frame_free( &m_pOutYuv );
	}

	if( m_pVideoCodecCtx )
	{
		avcodec_free_context( &m_pVideoCodecCtx );
	}

	m_iVideoPTS = 0;
	av_packet_unref( &m_pacFrm );

	// Audio
	if( m_pAudSC )
	{
		swr_free( &m_pAudSC );
	}

	if( m_pPCM )
	{
		av_frame_free( &m_pPCM );
	}

	if( m_pAudCodecCtx )
	{
		avcodec_free_context( &m_pAudCodecCtx );
	}

	m_iAudioPTS = 0;
	av_packet_unref( &m_pacAudio );
}

XDATA CXMediaEncode::RGB2YUV( XDATA dtRGB )
{
	XDATA dtRet;

	dtRet.m_llPts = dtRGB.m_llPts;

	// Set input Data
	m_pInRGB[ 0 ] = ( uint8_t* )dtRGB.m_pData;
	m_arrInRGBSize[ 0 ] = m_iInWidth * m_iInPixSize;	// bytes in one line

	// RGB to YUV and Scale
	int iHeight = sws_scale( m_pSC, m_pInRGB, m_arrInRGBSize, 0, m_iInHeight, m_pOutYuv->data, m_pOutYuv->linesize );
	if( iHeight <= 0 )
		return dtRet;

	//
	m_pOutYuv->pts = dtRGB.m_llPts;
	dtRet.m_pData = ( char* )m_pOutYuv;

	int* p = m_pOutYuv->linesize;

	while( *p )
	{
		dtRet.m_iSize += ( *p ) * m_iOutHeight;
		p++;
	}

	return dtRet;
}

XDATA CXMediaEncode::EncodeVideo( XDATA dtFrame )
{
	XDATA dtRet;

	if( dtFrame.m_iSize <= 0 || !dtFrame.m_pData )
		return dtRet;

	AVFrame* p = ( AVFrame* )dtFrame.m_pData;

	// 消除引用計數
	av_packet_unref( &m_pacFrm );

	// Encode H264
	//lpFrame->pts = m_iVideoPTS++;

	int iRet = avcodec_send_frame( m_pVideoCodecCtx, p );
	if( iRet != 0 )
		return dtRet;

	iRet = avcodec_receive_packet( m_pVideoCodecCtx, &m_pacFrm );
	if( iRet == 0 && m_pacFrm.size > 0 )
	{
		//cout << " " << pac.size << flush;
	}
	else
	{
		return dtRet;
	}

	//
	dtRet.m_pData = ( char* )&m_pacFrm;
	dtRet.m_iSize = m_pacFrm.size;
	dtRet.m_llPts = dtFrame.m_llPts;

	return dtRet;
}

XDATA CXMediaEncode::Resample( XDATA dtAudio )
{	// 重採樣數據
	XDATA dtRet;

	m_pInAudioData[ 0 ] = ( uint8_t* )dtAudio.m_pData;

	int iLen = swr_convert( m_pAudSC,
		m_pPCM->data, m_pPCM->nb_samples,   // 輸出
		( const uint8_t** )m_pInAudioData, m_pPCM->nb_samples );    // 輸入

	if( iLen <= 0 )
		return dtRet;

	//
	m_pPCM->pts = dtAudio.m_llPts;
	dtRet.m_pData = (char*)m_pPCM;
	dtRet.m_iSize = m_pPCM->nb_samples * m_pPCM->channels * 2;
	dtRet.m_llPts = dtAudio.m_llPts;

	return dtRet;
}

XDATA CXMediaEncode::EncodeAudio( XDATA dtFrame )
{
	XDATA dtRet;

	if( dtFrame.m_iSize <= 0 || !dtFrame.m_pData )
		return dtRet;

	AVFrame* p = ( AVFrame* )dtFrame.m_pData;

	// pts運算
	// 1 / sample_rate = 1個sample花費的秒數
	// nb_sample = 一幀音頻的sample數量 
	// 一幀音頻的秒數sec = 一幀音頻的sample數量 * 1個sample花費的秒數
	//                   = nb_sample * ( 1 / sample_rate )
	//                   = nb_sample / sample_rate
	// timebase = 一幀音頻的秒數sec = timebase.num / timebase.den
	// timebase pts = 一幀音頻的秒數sec * timebase.den
// 	m_pPCM->pts = m_iAudioPTS;
// 	m_iAudioPTS += av_rescale_q( m_pPCM->nb_samples, { 1, m_iSampleRate }, m_pAudCodecCtx->time_base );
	static long long llLastPts = 0;

	if( llLastPts == p->pts )
	{
		p->pts += 1200;
	}

	llLastPts = p->pts;

	// Encode Audio
	int iRet = avcodec_send_frame( m_pAudCodecCtx, p );
	if( iRet != 0 )
		return dtRet;

	av_packet_unref( &m_pacAudio );

	iRet = avcodec_receive_packet( m_pAudCodecCtx, &m_pacAudio );
	if( iRet == 0 && m_pacAudio.size > 0 )
	{
		//cout << " " << m_pacAudio.size << flush;
	}
	else
	{
		return dtRet;
	}

	//
	dtRet.m_pData = (char*)&m_pacAudio;
	dtRet.m_iSize = m_pacAudio.size;
	dtRet.m_llPts = dtFrame.m_llPts;

	return dtRet;
}

const AVCodecContext* CXMediaEncode::GetVideoCodecContext()
{
	return this->m_pVideoCodecCtx;
}

const AVCodecContext* CXMediaEncode::GetAudioCodecContext()
{
	return this->m_pAudCodecCtx;
}

int CXMediaEncode::GetCPUNumber()
{
#if defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64
	SYSTEM_INFO sysinfo;
	GetSystemInfo( &sysinfo );

	return ( int )sysinfo.dwNumberOfProcessors;
#elif defined __linux__
	return ( int )sysconf( _SC_NPROCESSORS_ONLN );
#elif defined __APPLE__
	int numCPU = 0;
	int mib[ 4 ];
	size_t len = sizeof( numCPU );

	// set the mib for hw.ncpu
	mib[ 0 ] = CTL_HW;
	mib[ 1 ] = HW_AVAILCPU;  // alternatively, try HW_NCPU;

						   // get the number of CPUs from the system
	sysctl( mib, 2, &numCPU, &len, NULL, 0 );

	if( numCPU < 1 )
	{
		mib[ 1 ] = HW_NCPU;
		sysctl( mib, 2, &numCPU, &len, NULL, 0 );

		if( numCPU < 1 )
			numCPU = 1;
	}
	return ( int )numCPU;
#else
	return 1;
#endif
}

AVCodecContext* CXMediaEncode::CreateCodec( AVCodecID eCodecID )
{
	// Find encoder
	const AVCodec* pCodec = avcodec_find_encoder( eCodecID );
	if( !pCodec )
	{
		cout << "Can't find encoder! ( Codec: " << eCodecID << " )" << endl;
		return NULL;
	}

	// Create encoder context
	AVCodecContext* pCtx = avcodec_alloc_context3( pCodec );
	if( !pCtx )
	{
		cout << "avcodec_alloc_context3 failed!" << endl;
		return NULL;
	}

	pCtx->time_base = { 1, 1000000 };

	return pCtx;
}

bool CXMediaEncode::OpenCodec( AVCodecContext** lpCtx )
{
	int iRet = avcodec_open2( *lpCtx, NULL, NULL );
	if( iRet != 0 )
	{
		char szBuf[ 1024 ] = { 0 };
		av_strerror( iRet, szBuf, sizeof( szBuf ) - 1 );
		cout << szBuf << endl;

		//
		avcodec_free_context( lpCtx );

		return false;
	}

	return true;
}

/// <summary>
/// IXMediaEncode Implementation
/// </summary>
IXMediaEncode::IXMediaEncode()
{

}

IXMediaEncode::~IXMediaEncode()
{

}

IXMediaEncode* IXMediaEncode::Get( unsigned char cIdx/* = 0*/ )
{
	static CXMediaEncode arrXM[ 255 ];

	return &arrXM[ cIdx ];
}

