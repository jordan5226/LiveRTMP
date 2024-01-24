#pragma once

#include <string>

extern "C"
{
#include <libavutil/time.h>
}

/// <summary>
/// Get Current Time
/// </summary>
extern long long GetCurTime();

/// <summary>
/// Media Data
/// </summary>
typedef struct tagXDATA
{
	int			m_iSize = 0;
	char*		m_pData = NULL;
	long long	m_llPts = 0;

	tagXDATA()
	{
	}

	tagXDATA( char* lpData, int iSize, long long llPts )
	{
		m_iSize = iSize;

		if( lpData )
		{
			m_pData = new char[ iSize ];
			::memcpy_s( m_pData, sizeof( char ) * iSize, lpData, sizeof( char ) * iSize );
		}

		this->m_llPts = llPts;
	}

	virtual ~tagXDATA()
	{
	}

	void Free()
	{
		if( m_pData )
		{
			delete m_pData;
			m_pData = NULL;
		}

		m_iSize = 0;
	}
} XDATA, *LPXDATA;
