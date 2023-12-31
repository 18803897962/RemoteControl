#pragma once
#include<atomic>
#include "pch.h"
#include <list>
#include "Queue.h"
#include "MyThread.h"
template<class T>
class CQueue
{//线程安全队列 利用iocp实现
public:
	enum {
		CQNone,
		CQClear,
		CQPush,
		CQPop,
		CQSize
	};
	struct IOCP_Param {
		size_t	nOperator;//插入or删除
		T Data;
		HANDLE hEvent;
		IOCP_Param(int op, const T& data, HANDLE  hEve=NULL) {
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}
		IOCP_Param() :nOperator(CQNone) {}
	}PPARAM;//Post Param用于投递信息的结构体
	
public:
	CQueue() {
		m_lock = false;
		m_hCompletionPort= CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompletionPort != NULL) {
			m_hThread= (HANDLE)_beginthread(
				&CQueue<T>::threadEntry,
				0, this);
		}
	}
	virtual ~CQueue() {
		if (m_lock == true) return;
		m_lock = true;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);
		WaitForSingleObject(m_hThread, INFINITE);//等待线程结束
		if (m_hCompletionPort != NULL) {
			HANDLE tmp = m_hCompletionPort;
			m_hCompletionPort = NULL;
			CloseHandle(tmp);
		}
	}
	bool PushBack(const T& data) {
		if (m_lock == true) return false;//正在被析构，不能push
		IOCP_Param* pParam = new IOCP_Param(CQPush, data);
		bool ret=PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM),
			(ULONG_PTR)pParam, NULL);
		if (ret == false) delete pParam;//post失败的话，需要自己释放内存
		return ret;
	}
	virtual bool PopFront(T& data) {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IOCP_Param Param = IOCP_Param(CQPop, data,hEvent);
		if (m_lock == true) {
			if (hEvent) CloseHandle(hEvent);
			return false;//正在被析构，不能push
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM),
			(ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return false;
		}
		ret=WaitForSingleObject(hEvent, INFINITE)==WAIT_OBJECT_0;
		if (ret) {
			data = Param.Data;
		}
		return ret;
	}
	size_t Size() {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IOCP_Param Param = IOCP_Param(CQSize, T(), hEvent);
		if (m_lock == true) {
			if (hEvent) CloseHandle(hEvent);
			return -1;//正在被析构，不能push
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM),
			(ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return -1;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			return Param.nOperator;
		}
		return -1;
	}
	bool Clear() {
		IOCP_Param* pParam = new IOCP_Param(CQClear, T());
		if (m_lock == true) return false;//正在被析构，不能push
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM),
			(ULONG_PTR)pParam, NULL);
		if (ret == false) delete pParam;//post失败的话，需要自己释放内存
		return ret;
	}
protected:
	static void threadEntry(void* arg) {
		CQueue<T>* thiz = (CQueue<T>*)arg;
		thiz->threadPlay();
		_endthread();
	}
	virtual void DealPparam(IOCP_Param* pParam) {
		switch (pParam->nOperator)
		{
		case CQPush:
			m_lstData.push_back(pParam->Data);
			delete pParam;
			break;
		case CQPop: {
			//std::string str;
			if (m_lstData.size() > 0) {
				pParam->Data = m_lstData.front();
				m_lstData.pop_front();
			}
			if (pParam->hEvent != NULL) {
				SetEvent(pParam->hEvent);
			}
		}
			break;
		case CQClear:
			m_lstData.clear();
			delete pParam;
			break;
		case CQSize:
			pParam->nOperator = m_lstData.size();
			if (pParam->hEvent != NULL) {
				SetEvent(pParam->hEvent);
			}
			break;
		default:
			OutputDebugString(_T("unknown opeartor code"));
			break;
		}
	}
	void threadPlay() {
		IOCP_Param* pParam = NULL;
		DWORD dwTransfered = 0;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* pOverlaped = 0;
		while (GetQueuedCompletionStatus(m_hCompletionPort, 
			&dwTransfered,
			&CompletionKey, 
			&pOverlaped, INFINITE)) {
			if (dwTransfered == 0 || CompletionKey == NULL) {  //主动按下键盘，结束信号的发送时，就代表iocp已经退出
				printf("thread is prapare to exit!\n");
				break;
			}
			pParam = (IOCP_Param*)CompletionKey;
			DealPparam(pParam);
		}
		while (GetQueuedCompletionStatus(m_hCompletionPort,
			&dwTransfered,
			&CompletionKey,
			&pOverlaped, 0)) { //防止有剩余信息传来
			if (dwTransfered == 0 || CompletionKey == NULL) {  //主动按下键盘，结束信号的发送时，就代表iocp已经退出
				printf("thread is prapare to exit!\n");
				continue;
			}
			pParam = (IOCP_Param*)CompletionKey;
			DealPparam(pParam);
		}
		CloseHandle(m_hCompletionPort);
		m_hCompletionPort = NULL;
	}
protected:
	std::list<T> m_lstData;
	HANDLE m_hCompletionPort;
	HANDLE m_hThread;
	bool m_IsEnd;//队列正在析构
	std::atomic<bool> m_lock;//原子操作，起到互斥锁的作用
};



template<class T>
class CSendQueue:public CQueue<T>, public ThreadFuncBase{
public:
	typedef int (ThreadFuncBase::* MYCALLBACK)(T& data);
	CSendQueue(ThreadFuncBase* obj, MYCALLBACK callback):CQueue<T>() ,m_base(obj),m_callback(callback){
		m_thread.Start();
		m_thread.UpdateWorker(::ThreadWorker(this, (FUNCTYPE)&CSendQueue<T>::threadTick));
	}
	virtual ~CSendQueue() {
		m_base = NULL;
		m_callback = NULL;
		m_thread.Stop();
	};
protected:
	virtual bool PopFront(T& data) { return false; }
	bool PopFront() {
		typename CQueue<T>::IOCP_Param* Param =new typename CQueue<T>::IOCP_Param(CQueue<T>::CQPop, T());
		if (CQueue<T>::m_lock == true) {
			delete Param;
			return false;//正在被析构，不能push
		}
		bool ret = PostQueuedCompletionStatus(CQueue<T>::m_hCompletionPort, sizeof(*Param),
			(ULONG_PTR)&Param, NULL);
		if (ret == false) {
			delete Param;
			return false;
		}
		
		return ret;
	}
	int threadTick() {
		if (WaitForSingleObject(CQueue<T>::m_hThread, 0) != WAIT_TIMEOUT) 
			return 0;
		if (CQueue<T>::m_lstData.size() > 0) {
			PopFront();
		}
		Sleep(1);
		return 0;
	}
	virtual void DealPparam(typename CQueue<T>::IOCP_Param* pParam) {
		switch (pParam->nOperator)
		{
		case CQueue<T>::CQPush:
			CQueue<T>::m_lstData.push_back(pParam->Data);
			delete pParam;
			break;
		case CQueue<T>::CQPop: {
			//std::string str;
			if (CQueue<T>::m_lstData.size() > 0) {
				pParam->Data = CQueue<T>::m_lstData.front();
				if((m_base->*m_callback)(pParam->Data)==0)//调用函数
					CQueue<T>::m_lstData.pop_front();
			}
			delete pParam;
		}
			break;
		case CQueue<T>::CQClear:
			CQueue<T>::m_lstData.clear();
			delete pParam;
			break;
		case CQueue<T>::CQSize:
			pParam->nOperator = CQueue<T>::m_lstData.size();
			if (pParam->hEvent != NULL) {
				SetEvent(pParam->hEvent);
			}
			break;
		default:
			OutputDebugString(_T("unknown opeartor code"));
			break;
		}
	}
private:
	ThreadFuncBase* m_base;
	MYCALLBACK m_callback;
	CMyThread m_thread;
};

typedef  CSendQueue<std::vector<char>>::MYCALLBACK SendCallBack;