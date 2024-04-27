#include <QtCore\QCoreApplication>
#include <iostream>
#include "IXMediaEncode.h"
#include "IXRtmp.h"
#include "IXAudioRecord.h"
#include "IXVideoCapture.h"

using namespace std;

// LiveRTMP.exe -camera -output rtmp://192.168.1.126/live
// LiveRTMP.exe -src rtsp://192.168.1.134:8554/webcam.h264 -output rtmp://192.168.1.126/live

IXVideoCapture* pVidRec = NULL;
IXAudioRecord* pAudRec = NULL;
IXMediaEncode* pXMedia = NULL;
IXRtmp* pRtmp = NULL;
char* pszOut = NULL;

int StopLive()
{
	if( pVidRec )
		pVidRec->StopRecord();

	if( pAudRec )
		pAudRec->StopRecord();

	if( pXMedia )
		pXMedia->Close();

	if( pRtmp )
		pRtmp->Close();

	if( pszOut )
		delete pszOut;

	getchar();

	return -1;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    //
    int iNBSamples = 1024;
    int iSampleRate = 48000;
    int iChannels   = 1;
    int iSampleSize = 2;    // 2 Bytes
	//const char* pszIn = "rtsp://192.168.1.134:8554/webcam.h264";	// Run simple rtsp server, and use ffmpeg to push camera video as rtsp stream
    //const char* pszOut = "rtmp://192.168.1.126/live";

    // Encoder and "Pixel format converter"/"ReSampler"
	pXMedia = IXMediaEncode::Get( 0 );
	if( !pXMedia )
		return -1;

    /// 1. Open Camera
    pVidRec = IXVideoCapture::Get( 0 );

	string strCmdline( "" );

	for( int i = 0, iFind = -1, iPart = 0; i < argc; ++i )
	{
		//  Get Command Line
		strCmdline = argv[ i ];

		if( 0 == iPart )
		{
			// Find camera
			iFind = strCmdline.find( "-camera", 0 );

			if( iFind >= 0 )
			{	// Open camera
				if( !pVidRec->Init( 0 ) )
					return -1;

				++iPart;

				continue;
			}

			// Find source stream
			iFind = strCmdline.find( "-src", 0 );

			if( iFind >= 0 )
			{
				if( ++i < argc )
				{	// Open stream
					if( !pVidRec->Init( argv[ i ] ) )
						return -1;

					++iPart;
				}

				continue;
			}
		}
		else
		{
			// Find Output
			iFind = strCmdline.find( "-output", iFind );

			if( iFind >= 0 )
			{
				if( ++i < argc )
				{
					int iLen = ::strlen( argv[ i ] ) + 1;

					if( iLen > 1 )
					{
						pszOut = new char[ iLen ];

						if( pszOut )
							::strcpy( pszOut, argv[ i ] );
						else
							return StopLive();

						break;
					}
					else
					{
						return StopLive();
					}
				}
			}
		}
	}

    cout << "Open camera success!" << endl;

    /// 3.1 Initial RGB to YUV scale
	if( !pXMedia->InitScale( pVidRec->m_iSrcWidth, pVidRec->m_iSrcHeight, 3, 1280, 960 ) )
	{
		cout << "InitScale failed!" << endl;
		return StopLive();
	}

	// 4.1 Initial encode video context
    if( !pXMedia->InitVideoCodec( 512 * 1024 * 8, pVidRec->m_iFPS ) )
    {
        cout << "InitVideoCodec failed!" << endl;
		return StopLive();
	}

    /// 1. QT音頻開始錄製
    pAudRec = IXAudioRecord::Get();
    if( !pAudRec->StartRecord( IXAudioRecord::XSAMPLE_FORMAT_INT16, iNBSamples, iSampleRate, iChannels ) )
	{
        cout << "IXAudioRecord StartRecord failed!" << endl;
		return StopLive();
	}

    /// 3.1 音頻重採樣
    if( !pXMedia->InitResample( iNBSamples, iSampleRate, iChannels, iSampleSize, IXMediaEncode::X_SAMPLE_FMT_S16, IXMediaEncode::X_SAMPLE_FMT_FLTP ) )
    {
		return StopLive();
    }

    /// 4.1 Initialize Audio Encoder
    if( !pXMedia->InitAudioCodec( 40000 ) )
    {
		return StopLive();
    }

    const AVCodecContext* pAudCodecCtx = pXMedia->GetAudioCodecContext();

	/// 5. 配置Container和Audio Stream
	// Create output stream container context
	pRtmp = IXRtmp::Get( 0 );
	if( !pRtmp )
	{
		return StopLive();
	}

	if( !pRtmp->Init( pszOut ) )
	{
		return StopLive();
	}

	// Initial output video/audio stream
	int iVideoStreamIdx = pRtmp->AddStream( pXMedia->GetVideoCodecContext() );
	if( iVideoStreamIdx < 0 )
	{
		cout << "Add video stream failed!" << endl;
		return StopLive();
	}

	int iAudioStreamIdx = pRtmp->AddStream( pXMedia->GetAudioCodecContext() );
    if( iAudioStreamIdx < 0 )
	{
        cout << "Add Audio stream failed!" << endl;
		return StopLive();
	}

	/// 6. Open RTMP network I/O
    if( !pRtmp->SendHead() )
	{
		return StopLive();
	}

    //
	AVFrame* pYUV = NULL;
    AVFrame* lpPCM = NULL;
    AVPacket* lpPkt = NULL;
    XDATA dtVideo, dtAudio, dtPCM, dtPkt, dtYUV;
	
	// Reset data queue, sync video and audio
	pVidRec->Clear();
	pAudRec->Clear();
	long long llBeginTime = ::GetCurTime();

    for( ;; )
    {
        // 2. 一次讀取一幀音視頻
        dtVideo = pVidRec->Pop();
        dtAudio = pAudRec->Pop();
        if( dtVideo.m_iSize <= 0 && dtAudio.m_iSize <= 0 )
        {
            QThread::msleep( 1 );
            continue;
        }

		// 已經讀一幀數據
        if( dtAudio.m_iSize > 0 )
        {	// Process Audio
			dtAudio.m_llPts -= llBeginTime;

			// 3.2 重採樣數據
			dtPCM = pXMedia->Resample( dtAudio );
			dtAudio.Free();

			// 4.2 Encode Audio
			dtPkt = pXMedia->EncodeAudio( dtPCM );
			if( dtPkt.m_iSize > 0 )
			{
				// 7. Push Stream
				if( pRtmp->SendFrame( dtPkt, iAudioStreamIdx ) )
				{
					cout << "@";
				}
			}
        }

		if( dtVideo.m_iSize > 0 )
		{	// Process Video
			dtVideo.m_llPts -= llBeginTime;

			// 3.2 RGB to YUV
			dtYUV = pXMedia->RGB2YUV( dtVideo );
			dtVideo.Free();
			if( !dtYUV.m_pData ) continue;

			// 4.2 Encode H264
			dtPkt = pXMedia->EncodeVideo( dtYUV );
			if( !dtPkt.m_pData ) continue;

			// 7. Push Stream
			if( pRtmp->SendFrame( dtPkt, iVideoStreamIdx ) )
			{
				cout << "#";
			}
		}
    }

	StopLive();

	return a.exec();
}
