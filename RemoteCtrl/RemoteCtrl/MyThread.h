#pragma once
#include<atomic>
#include <vector>
#include<mutex>
#include <windows.h>
#include "pch.h"
class ThreadFuncBase {//线程接口
};
typedef int (ThreadFuncBase::* FUNCTYPE)();//ThreadFuncBase成员函数指针
class ThreadWorker {
public:
	ThreadWorker():thiz(NULL),func(NULL){}
	ThreadWorker(void* obj,FUNCTYPE functype):thiz((ThreadFuncBase*)obj),func(functype) {}
	ThreadWorker(const ThreadWorker& worker) {
		thiz = worker.thiz;
		func = worker.func;
	}
	ThreadWorker& operator=(const ThreadWorker& worker) {
		if (this != &worker) {
			thiz = worker.thiz;
			func = worker.func;
		}
		return *this;
	}
	int operator()() {
		if (isValid()) {
			if (thiz) {
				return (thiz->*func)();
			}
		}
		return -1;
	}
	bool isValid() const {
		return thiz!=NULL && func!=NULL;
	}
	ThreadFuncBase* thiz;  //ThreadFuncBase对象指针
	FUNCTYPE func;   //ThreadFuncBase成员函数指针
};


//worker线程类
class CMyThread  
{//线程类
public:
	CMyThread() {
		m_hThread = NULL;
		m_bStatus = false;
	}
	~CMyThread() {
		Stop();
	}
	bool Start() {//true表示成功
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(&CMyThread::thread_Entry, 0, this);  //开启线程
		if (m_hThread == INVALID_HANDLE_VALUE) m_bStatus = false;
		return m_bStatus;
	}
	bool Stop() {
		if (m_bStatus == false) return true;
		m_bStatus = false;//终止threadworker运行
		DWORD ret = WaitForSingleObject(m_hThread, 1000) == WAIT_OBJECT_0;  //返回值WAIT_OBJECT_0表示线程已结束
		if (ret == WAIT_TIMEOUT) {
			TerminateThread(m_hThread, -1);  //强制结束线程
		}
		UpdateWorker();
		return ret;
	}
	bool isValid(){//返回true表示有效 false表示线程异常
		if (m_hThread == INVALID_HANDLE_VALUE || m_hThread == NULL) {
			return false;
		}
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;  //表示线程在指定时间没执行完，即线程正在执行
	}
	void UpdateWorker(const ::ThreadWorker& worker=::ThreadWorker()) {
		if (m_worker.load() != NULL && m_worker.load()!= &worker) {
			::ThreadWorker* pWorker = m_worker.load();
			TRACE("delete pWorker = %08X m_worker=%08X\r\n", pWorker, m_worker.load());
			m_worker.store(NULL);
			delete pWorker;
		}
		if (m_worker.load() == &worker) return;
		if (!worker.isValid()) {
			m_worker.store(NULL);
			return;
		}
		::ThreadWorker* pworker = new ::ThreadWorker(worker);
		TRACE("new pWorker = %08X m_worker=%08X\r\n", pworker, m_worker.load());
		m_worker.store(new ::ThreadWorker(worker));
	}
	bool IsIdle() {//是否空闲 true表示空闲 false表示正在工作
		if (m_worker.load() == NULL) {
			return true;  //此时为空，空闲状态
		}
		return !m_worker.load()->isValid();  //m_worker首先要存在
	}
protected:
	//virtual int each_step();//纯虚函数，默认要求用户实现  返回值0表示正常
private:
	void threadWorker() {//工作函数
		while (m_bStatus) {
			if (m_worker.load() == NULL) {
				Sleep(1);
				continue;
			}
			::ThreadWorker worker = *m_worker.load();
			if (worker.isValid()) {
				if (WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT) {
					int ret = worker();   //()重载 相当于调用worker的func
					if (ret != 0) {//大于0则发送警告日志 等于0则正常运行
						CString str;
						str.Format(_T("thread fuond warning code: %d\n"), str);
						OutputDebugString(str);
					}
					if (ret < 0) {//ret小于0时则终止线程循环
						//m_worker.store(worker);
						::ThreadWorker* pWorker = m_worker.load();
						m_worker.store(NULL);
						delete pWorker;
					}
				}
			}
			else {
				Sleep(1);
			}
			
		}
	}
	static void thread_Entry(void* arg) {
		CMyThread* thiz = (CMyThread*)arg;
		thiz->threadWorker();
		_endthread();
	}
private:
	HANDLE m_hThread;
	bool m_bStatus;//false表示关闭 true表示线程运行
	std::atomic<::ThreadWorker*> m_worker;
};


class MyThreadPoor {
public:
	MyThreadPoor(){}
	~MyThreadPoor() {
		Stop();
		for (int i = 0; i < m_threads.size(); i++) {
			delete m_threads[i];
			m_threads[i] = NULL;
		}
		m_threads.clear();
	}
	MyThreadPoor(size_t size) {  //设置线程池大小
		m_threads.resize(size);
		for (size_t i = 0; i < size; i++) {
			m_threads[i] = new CMyThread();
		}
	}
	bool Invoke() {
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i]->Start() == false) {//线程启动失败
				ret = false;
				break;
			}
		}
		if (ret = false) {//线程启动失败，全部关闭 线程池中：如果线程没有全部启动成功，那么就认为线程池是失败的
			for (size_t i = 0; i < m_threads.size(); i++) {
				m_threads[i]->Stop();
			}
		}
		return ret;
	}
	void Stop() {
		for (size_t i = 0; i < m_threads.size(); i++) {
			m_threads[i]->Stop();
		}
	}
	int dispatchWorker(const ThreadWorker& worker) {
		int index = -1;
		m_mutex.lock();
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i]->IsIdle()) {
				m_threads[i]->UpdateWorker(worker);
				index = i;
				break;
			}
		}
		m_mutex.unlock();
		return index;//若返回-1，则证明线程池内所有线程都在忙碌，分配失败  其他值则表示所分配的线程下标
	}
	bool CheckThreadValid(size_t index) {  //检查线程是否有效
		if (index < m_threads.size()) {
			return m_threads[index]->isValid();
		}
		return false;
	}

private:
	std::mutex m_mutex;
	std::vector<CMyThread*> m_threads;
};
