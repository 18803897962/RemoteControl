#pragma once
#include<atomic>
#include <vector>
class ThreadFuncBase {//�߳̽ӿ�
};
typedef int (ThreadFuncBase::* FUNCTYPE)();//ThreadFuncBase��Ա����ָ��
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
	ThreadFuncBase* thiz;  //ThreadFuncBase����ָ��
	FUNCTYPE func;   //ThreadFuncBase��Ա����ָ��
};

class CMyThread
{//�߳���
public:
	CMyThread() {
		m_hThread = NULL;
	}
	~CMyThread() {
		Stop();
	}
	bool Start() {//true��ʾ�ɹ�
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(&CMyThread::thread_Entry, 0, this);  //�����߳�
		if (m_hThread == INVALID_HANDLE_VALUE) m_bStatus = false;
		return m_bStatus;
	}
	bool Stop() {
		if (m_bStatus == false) return true;
		m_bStatus = false;//��ֹthreadworker����
		return WaitForSingleObject(m_hThread,INFINITY)==WAIT_OBJECT_0;  //����ֵWAIT_OBJECT_0��ʾ�߳��ѽ���
	}
	bool isValid() {//����true��ʾ��Ч false��ʾ�߳��쳣
		if (m_hThread == INVALID_HANDLE_VALUE || m_hThread == NULL) {
			return false;
		}
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;  //��ʾ�߳���ָ��ʱ��ûִ���꣬���߳�����ִ��
	}
	bool UpdateWorker(const ::ThreadWorker& worker=::ThreadWorker()) {
		m_worker.store(worker);
	}
protected:
	virtual int each_step() = 0;//���麯����Ĭ��Ҫ���û�ʵ��  ����ֵ0��ʾ����
private:
	void threadWorker() {//��������
		while (m_bStatus) {
			::ThreadWorker worker = m_worker.load();
			if (worker.isValid()) {
				int ret=worker();   //()���� �൱�ڵ���worker��func
				if (ret != 0) {//����0���;�����־ ����0����������
					CString str;
					str.Format(_T("thread fuond warning code: %d\n"), str);
					OutputDebugString(str);
				}
				if (ret < 0) {//retС��0ʱ����ֹ�߳�ѭ��
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
	bool m_bStatus;//false��ʾ�ر� true��ʾ�߳�����
	std::atomic<::ThreadWorker> m_worker;
};


class MyThreadPoor {
public:
	MyThreadPoor(){}
	~MyThreadPoor() {
		Stop();
		m_threads.clear();
	}
	MyThreadPoor(size_t size) {  //�����̳߳ش�С
		m_threads.resize(size);
	}
	bool Invoke() {
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); i++) {
			if (m_threads[i].Start() == false) {//�߳�����ʧ��
				ret = false;
				break;
			}
		}
		if (ret = false) {//�߳�����ʧ�ܣ�ȫ���ر� �̳߳��У�����߳�û��ȫ�������ɹ�����ô����Ϊ�̳߳���ʧ�ܵ�
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
