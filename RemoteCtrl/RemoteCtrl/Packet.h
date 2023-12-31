#pragma once
#include "pch.h"
#include "framework.h"
#pragma pack(push)
#pragma pack(1)
class CPacket   //声明数据包的类
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(WORD nCmd, BYTE* pData, size_t nSize) {   //封包操作，用于将数据封包并发送
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)(strData.c_str()), pData, nSize);
		}
		else strData.clear();
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}
	CPacket(const CPacket& packet) {
		sHead = packet.sHead;
		nLength = packet.nLength;
		sCmd = packet.sCmd;
		strData = packet.strData;
		sSum = packet.sSum;

	}
	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i = 0;   //数据段的偏移量
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) {  //包数据可能不全，或包头未能全部接收，解析失败
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize) {  //避免只解析一半的情况，包未完全接收到就返回，解析失败
			nSize = 0;   //包长度是从控制命令开始计算，到校验位截止的  所以直接计算nLength+i
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;
		//memcpy(&sCmd, &pData + i, sizeof(WORD)); i += 2;

		TRACE("command=%d\r\n", sCmd);
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);//减去控制命令和校验
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;  //nLength是实际数据的长度
		}
		sSum = *(WORD*)(pData + i); i += 2;
		WORD sum = 0;  //进行校验
		for (size_t j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) { //解析成功
			nSize = i;  //此时的i是包的实际长度   ？？？   不一定等于i 因为固定包头不一定是从0开始的
			return;
		}
		nSize = 0;  //解析失败
	}
	~CPacket() {}
	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) {
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}
	int Size() {  //获得包数据的大小
		return nLength + 6;  //nLength是包长度
	}
	const char* Data() {//获取到包数据的char*形式，用于send数据包
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();  //定义一个指针，用于后续修改字符串内容
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}
public:
	WORD sHead=0;  //包头，固定位FEFF
	DWORD nLength=0;  //包长度，控制命令开始到和校验结束
	WORD sCmd=0; //控制命令
	std::string strData;  //数据
	WORD sSum=0;  //和校验
	std::string strOut; //整个包的数据保存在strOut中
};

#pragma pack(pop)

typedef struct MouseEvent {
	MouseEvent() {//默认构造函数
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//动作，包括点击、移动、双击
	WORD nButton;//左键、右键、滚轮
	POINT ptXY;//坐标
}MOUSEEV, * PMOUSEEV;
typedef struct file_info
{
	file_info() { //构造函数
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid; //是否有效
	BOOL IsDirectory;//是否为目录 0否 1是
	BOOL HasNext; //是否含有后续文件 0没有 1有  用于实现找到一个发送一个
	char szFileName[260];
}FILEINFO, * PFILEINFO;