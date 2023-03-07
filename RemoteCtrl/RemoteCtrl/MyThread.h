#pragma once
#include<atomic>
#include <vector>
class ThreadFuncBase {//线程接口
};
typedef int (ThreadFuncBase::* FUNCTYPE)();//ThreadFuncBase成员函数指针
class ThreadWorker {
public:
	ThreadWorker():thiz(NULL),func(NULL){}
	ThreadWorker(ThreadFuncBase* obj,FUNCTYPE functype):thiz(obj),func(functype) {}
	ThreadWorker(const ThreadWorker& worker) {
		thiz = worker.thiz;
		func = worker.func;
	}
	ThreadWorker& operator=(const ThreadWorker& worker) {
		if (this != &worker) {
			this->func = worker.func;
			this->thiz = worker.thiz;
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
	bool isValid() {
		return thiz!=NULL && func!=NULL;
	}
	ThreadFuncBase* thiz;  //ThreadFuncBase对象指针
	FUNCTYPE func;   //ThreadFuncBase成员函数指针
};

class CMyThread
{//线程类
public:
	CMyThread() {
		m_hThread = NULL;
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
		return WaitForSingleObject(m_hThread,INFINITY)==WAIT_OBJECT_0;  //返回值WAIT_OBJECT_0表示线程已结束
	}
	bool isValid() {//返回true表示有效 false表示线程异常
		if (m_hThread == INVALID_HANDLE_VALUE || m_hThread == NULL) {
			return false;
		}
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;  //表示线程在指定时间没执行完，即线程正在执行
	}
	bool UpdateWorker(const ::ThreadWorker& worker=::ThreadWorker()) {
		m_worker.store(worker);
	}
protected:
	virtual int each_step() = 0;//纯虚函数，默认要求用户实现  返回值0表示正常
private:
	void threadWorker() {//工作函数
		while (m_bStatus) {
			::ThreadWorker worker = m_worker.load();
			if (worker.isValid()) {
				int ret=worker();   //()重载 相当于调用worker的func
				if (ret != 0) {//大于0则发送警告日志 等于0则正常运行
					CString str;
					str.Format(_T("thread fuond warning code: %d\n"), str);
					OutputDebugString(str);
				}
				if (ret < 0) {//ret小于0时则终止线程循环
					//m_worker.store(worker);
					m_worker.store(::ThreadWorker());
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
	std::atomic<::ThreadWorker> m_worker;
};


class MyThreadPoor {
public:
	MyThreadPoor(){}
	~MyThreadPoor() {
		Stop();
		m_threads.clear();
	}
	MyThreadPoor(size_t size) {  //设置线程池大小
		m_threads.resize(size);
	}
	bool Invoke() {
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i].Start() == false) {//线程启动失败
				ret = false;
				break;
			}
		}
		if (ret = false) {//线程启动失败，全部关闭 线程池中：如果线程没有全部启动成功，那么就认为线程池是失败的
			for (size_t i = 0; i < m_threads.size(); i++) {
				m_threads[i].Stop();
			}
		}
		return ret;
	}
	void Stop() {
		for (size_t i = 0; i < m_threads.size(); i++) {
			m_threads[i].Stop();
		}
	}
	int dispatchWorker(const ThreadWorker& worker) {

	}
private:
	std::vector<CMyThread> m_threads;
};
