#include "XDataThread.h"



XDataThread::XDataThread()
{

}

XDataThread::~XDataThread()
{

}

void XDataThread::Push( XDATA data )
{
	m_mxData.lock();

	if( m_listData.size() >= m_iMaxDataCnt )
	{
		m_listData.front().Free();
		m_listData.pop_front();
	}

	m_listData.push_back( data );

	m_mxData.unlock();
}

XDATA XDataThread::Pop()
{
	m_mxData.lock();

	if( m_listData.empty() )
	{
		m_mxData.unlock();
		return XDATA();
	}

	XDATA data = m_listData.front();
	m_listData.pop_front();

	m_mxData.unlock();

	return data;
}

void XDataThread::Clear()
{
	m_mxData.lock();

	while( !m_listData.empty() )
	{
		m_listData.front().Free();
		m_listData.pop_front();
	}

	m_mxData.unlock();
}

bool XDataThread::Start()
{
	m_bExit = false;
	QThread::start();

	return true;
}

void XDataThread::Stop()
{
	m_bExit = true;
	wait();
}
