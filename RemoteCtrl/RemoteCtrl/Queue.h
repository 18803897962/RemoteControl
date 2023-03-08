#pragma once
#include<atomic>
#include "pch.h"
#include <list>
template<class T>
class CQueue
{//�̰߳�ȫ���� ����iocpʵ��
public:
	enum {
		CQNone,
		CQClear,
		CQPush,
		CQPop,
		CQSize
	};
	struct IOCP_Param {
		size_t	nOperator;//����orɾ��
		T Data;
		HANDLE hEvent;
		IOCP_Param(int op, const T& data, HANDLE  hEve=NULL) {
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}
		IOCP_Param() :nOperator(CQNone) {}
	}PPARAM;//Post Param����Ͷ����Ϣ�Ľṹ��
	
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
	~CQueue() {
		if (m_lock == true) return;
		m_lock = true;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);
		WaitForSingleObject(m_hThread, INFINITE);//�ȴ��߳̽���
		if (m_hCompletionPort != NULL) {
			HANDLE tmp = m_hCompletionPort;
			m_hCompletionPort = NULL;
			CloseHandle(tmp);
		}
	}
	bool PushBack(const T& data) {
		if (m_lock == true) return false;//���ڱ�����������push
		IOCP_Param* pParam = new IOCP_Param(CQPush, data);
		bool ret=PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM),
			(ULONG_PTR)pParam, NULL);
		if (ret == false) delete pParam;//postʧ�ܵĻ�����Ҫ�Լ��ͷ��ڴ�
		return ret;
	}
	bool PopFront(T& data) {
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IOCP_Param Param = IOCP_Param(CQPop, data,hEvent);
		if (m_lock == true) {
			if (hEvent) CloseHandle(hEvent);
			return -1;//���ڱ�����������push
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
			return -1;//���ڱ�����������push
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
		if (m_lock == true) return false;//���ڱ�����������push
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM),
			(ULONG_PTR)pParam, NULL);
		if (ret == false) delete pParam;//postʧ�ܵĻ�����Ҫ�Լ��ͷ��ڴ�
		return ret;
	}
private:
	static void threadEntry(void* arg) {
		CQueue<T>* thiz = (CQueue<T>*)arg;
		thiz->threadPlay();
		_endthread();
	}
	void DealPparam(IOCP_Param* pParam) {
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
			if (dwTransfered == 0 || CompletionKey == NULL) {  //�������¼��̣������źŵķ���ʱ���ʹ���iocp�Ѿ��˳�
				printf("thread is prapare to exit!\n");
				break;
			}
			pParam = (IOCP_Param*)CompletionKey;
			DealPparam(pParam);
		}
		while (GetQueuedCompletionStatus(m_hCompletionPort,
			&dwTransfered,
			&CompletionKey,
			&pOverlaped, 0)) { //��ֹ��ʣ����Ϣ����
			if (dwTransfered == 0 || CompletionKey == NULL) {  //�������¼��̣������źŵķ���ʱ���ʹ���iocp�Ѿ��˳�
				printf("thread is prapare to exit!\n");
				continue;
			}
			pParam = (IOCP_Param*)CompletionKey;
			DealPparam(pParam);
		}
		CloseHandle(m_hCompletionPort);
		m_hCompletionPort = NULL;
	}
private:
	std::list<T> m_lstData;
	HANDLE m_hCompletionPort;
	HANDLE m_hThread;
	bool m_IsEnd;//������������
	std::atomic<bool> m_lock;//ԭ�Ӳ������𵽻�����������
};

