#pragma once
class CTools
{
public:
	static void Dump(BYTE* pData, size_t nSize) {
		std::string strout;
		for (size_t i = 0; i < nSize; i++) {
			char buf[8] = "";
			if (i > 0 && (i % 16 == 0)) strout += "\n";
			snprintf(buf, sizeof(buf), "%02X", pData[i] & 0xFF);
			strout += buf;
		}
		strout += "\n";
		OutputDebugStringA(strout.c_str());
	}
	static bool IsAdmin() {//�ж�Ȩ�ޣ��Ƿ�Ϊ����Ա
		HANDLE hToken = NULL;
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
			ShowError();
			return false;
		}
		TOKEN_ELEVATION eve;
		DWORD len = 0;
		if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == FALSE) {
			ShowError();
			return false;
		}
		CloseHandle(hToken);
		if (len == sizeof(eve)) {
			return eve.TokenIsElevated;  //���TokenIsElevated����ֵ����0�����ʾ��Ϊ����ԱȨ��
		}
		printf("len of Tokeninformation is %d\r\n", len);
		return false;
	}
	static void ShowError() {
		LPWSTR lpMessageBuf = NULL;  //��������
		//strerror(errno)  //��׼C��
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL,
			GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&lpMessageBuf, 0, NULL);
		OutputDebugString(lpMessageBuf);
		MessageBox(NULL, lpMessageBuf, _T("��������"), 0);
		LocalFree(lpMessageBuf);
	}
	static bool RunAsAdmin() {  //���β����� ��������Ա�˻� ���ƿ�����ֻ�ܵ�¼���ؿ���̨
		STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi = { 0 };
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, (LPWSTR)(LPCWSTR)sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
		if (!ret) {
			MessageBox(NULL, sPath, _T("��������ʧ��"), 0);  //TODO��ȥ��������Ϣ
			return false;
		}
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	static BOOL WriteStartup(const CString& strPath) {
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);

		return CopyFile(sPath, strPath, FALSE);//FALSE��ʾ�ļ�����ʱ�͸����ļ�
	}
	//����������ʱ��Ȩ���Ǹ����û�Ȩ�޵�
	//�������Ȩ�޲�һ�£���ᵼ�³�������ʧ��
	//���������Ի���������Ҫ���������dll�⶯̬�⣬���������ʧ��
	//���Խ�dll�⸳ֵ��system32��sysWow64��
	//system32�¶���64λ���򣬶�sysWow64�¶���32λ����
	//ע���ʽ
	static bool WriteRegisterTable(const CString& strPath) {
		CString strSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		BOOL ret = CopyFile(sPath, strPath, FALSE);//FALSE��ʾ�ļ�����ʱ�͸����ļ�
		if (ret == FALSE) {
			MessageBox(NULL, _T("�����ļ�ʧ�ܣ��Ƿ�Ȩ�޲��㣡"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		//ע������
		HKEY hKey = NULL;
		ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &hKey);    //Windows10�±���Ҫ��KEY_WOW64_64KEY
		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("�����Զ���������ʧ�ܣ��Ƿ�Ȩ�޲��㣿\r\n��������ʧ��"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		ret = RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * sizeof(TCHAR));
		if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("�����Զ���������ʧ�ܣ��Ƿ�Ȩ�޲��㣿\r\n��������ʧ��"), _T("����"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
		RegCloseKey(hKey);
	}
	static bool init() {//����MFC��Ŀ��ʼ�� ����
		HMODULE hModule = ::GetModuleHandle(nullptr);
		if (hModule == nullptr) {
			wprintf(L"����: GetModuleHandle ʧ��\n");
			return false;
		}
		if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
		{
			// TODO: �ڴ˴�ΪӦ�ó������Ϊ��д���롣
			wprintf(L"����: MFC ��ʼ��ʧ��\n");
			return false;
		}
		return true;
	}
};

