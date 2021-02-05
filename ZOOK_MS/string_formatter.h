#pragma once
// #include <zookeeper.h>
// #include <string>
// 
// 
// using namespace std;

//string FormatString(const char *lpcszFormat, ...)   //字符串格式化函数
//{
//	char *pszStr = NULL;
//	if (NULL != lpcszFormat)
//	{
//		va_list marker = NULL;
//		va_start(marker, lpcszFormat); //初始化变量参数
//		size_t nLength = _vscprintf(lpcszFormat, marker) + 1; //获取格式化字符串长度
//		pszStr = new char[nLength];
//		memset(pszStr, '\0', nLength);
//		_vsnprintf_s(pszStr, nLength, nLength, lpcszFormat, marker);
//		va_end(marker); //重置变量参数
//	}
//	string strResult(pszStr);
//	delete[]pszStr;
//	return strResult;
//}