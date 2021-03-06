/*
Desura is the leading indie game distribution platform
Copyright (C) 2011 Mark Chandler (Desura Net Pty Ltd)

$LicenseInfo:firstyear=2014&license=lgpl$
Copyright (C) 2014, Linden Research, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License only.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see <http://www.gnu.org/licenses/>
or write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
$/LicenseInfo$
*/

#ifndef DESURA_IPCPIPEBASE_H
#define DESURA_IPCPIPEBASE_H
#ifdef _WIN32
#pragma once
#endif

#include "util_thread/BaseThread.h"
#include "IPCPipeHelper.h"

namespace IPC
{
class IPCManager;

class LoopbackProcessor
{
public:
	virtual void loopbackMessage(const char* message, uint32 size, uint32 managerId)=0;
};

#ifdef NIX
	typedef void (*SendFn)(void* obj, const char* buff, size_t size);
#endif
	
//! Base Class for pipe server and pipe client
//!
class PipeBase : public Thread::BaseThread, public LoopbackProcessor
{
public:
	//! Constuctor
	//!
	//! @param pipeName Name of pipe
	//! @param threadName Name of thread (debugging)
	//!
	PipeBase(const char* pipeName, const char* threadName);
	~PipeBase();

	//! Adds a message to the loop back interface
	//!
	//! @param message Message to send back
	//! @param size Message size
	//! @param managerId manager to send to
	//!
	virtual void loopbackMessage(const char* message, uint32 size, uint32 managerId);

protected:
	//! ThreadBase inherit
	virtual void run();

	//! Finish reading from pipe
	//!
	//! @param data Pipe Data
	//! @param mng IPC Manager
	//!
	void finishRead(PipeData* data, IPCManager* mng);

	//! Perform overlap io read
	//!
	//! @param data Pipe Data
	//! @param mng IPC Manager
	//!
	bool performRead(PipeData* data, IPCManager* mng);

	//! Perform overlap io write
	//!
	//! @param data Pipe Data
	//! @param mng IPC Manager
	//!
	bool performWrite(PipeData* data, IPCManager* mng);

	//! Process io events
	//!
	void processEvents();

	//! Process loop back events
	//!
	void processLoopback();

	//! Init pipes (for server create, for client connect)
	//!
	virtual void setUpPipes()=0;

	//! Get the manager for a pipe instance
	//!
	//! @param index Event index
	//!
	virtual IPCManager* getManager(uint32 index)=0;

	
#ifdef WIN32
	//! Get the pipe data for a pipe instance
	//!
	//! @param index Event index
	//!
	virtual PipeData* getData(uint32 index)=0;

	//! Disconnect a broken pipe and reconnect it
	//!
	//! @param index Event index
	//!
	virtual void disconnectAndReconnect(uint32 i)=0;

	//! Fet number of events in the event array
	//!
	//! @param index Event index
	//!
	virtual uint32 getNumEvents()=0;

#else
	bool itemsWaiting();

	void setSendCallback(void* obj, SendFn funct);
public:
	void recvMessage(const char* buffer, size_t size);
protected:

	//! Stop thread
	//!
	virtual void onStop();
#endif
	

#ifdef WIN32
	gcString m_szRecvName;	//!< Pipe Recever name
	gcString m_szSendName;	//!< Pipe Sender name

	HANDLE m_hEventsArr[512];  //!< Event array
	
#else
	
	void* m_pSendObj;
	SendFn sendMsg;
	
	
	std::mutex m_RecvLock;
	std::deque<PipeData*> m_vRecvBuffer;
	
	::Thread::WaitCondition m_WaitCond;
	
#endif
	
	std::mutex m_LoopbackLock;
	
	class LoopbackInfo
	{
	public:
		LoopbackInfo()
		{
			buffer = nullptr;
			size = 0;
			id = 0;
		}

		~LoopbackInfo()
		{
			safe_delete(buffer);
		}

		uint32 id;
		uint32 size;
		char* buffer;
	};

	std::deque<LoopbackInfo*> m_vLoopback;
};


}

#endif //DESURA_IPCPIPEBASE_H
