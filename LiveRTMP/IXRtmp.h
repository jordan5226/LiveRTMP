#pragma once

#include "XData.h"

struct AVCodecContext;
struct AVPacket;

class IXRtmp
{
public:
	virtual ~IXRtmp();

	virtual bool	Init( const char* lpszOutUrl ) = 0;				// Initial container context
	virtual int		AddStream( const AVCodecContext* lpCtx ) = 0;	// Add Video/Audio Stream. Return -1, if failed, return stream index if success.
	virtual bool	SendHead() = 0;									// Open RTMP network I/O; Send container header
//	virtual bool	SendFrame( AVPacket* lpPac, int iStreamIdx = 0 ) = 0;			// Push Stream
	virtual bool	SendFrame( XDATA dtPac, int iStreamIdx = 0 ) = 0;				// Push Stream


	virtual void	Close() = 0;

	// Factory Create Method
	static IXRtmp* Get( unsigned char cIdx = 0 );

protected:
	IXRtmp();
};

