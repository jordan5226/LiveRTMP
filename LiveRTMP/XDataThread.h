#pragma once

#include "XData.h"
#include <QThread>
#include <QMutex>
#include <list>

using namespace std;

class XDataThread : public QThread
{
protected:
	bool				m_bExit = false;
	int					m_iMaxDataCnt = 100;
	list< XDATA >		m_listData;	// FIFO
	QMutex				m_mxData;


public:
	XDataThread();
	virtual ~XDataThread();
	virtual void	Push( XDATA data );	// Push to list tail
	virtual XDATA	Pop();	// Get data ( Clear data buffer by caller, call XDATA::Free )
	virtual bool	Start();
	virtual void	Stop();	// Stop thread, and wait for thread exit ( blocking )
	virtual void	Clear();
};