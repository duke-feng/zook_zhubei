#pragma once
//#include "stdafx.h"
#include <iostream>
#include <string>
using namespace std;

//std::string GetPathOrURLShortName(std::string strFullName);


// int _tmain(int argc, _TCHAR* argv[])
// {
// 	std::string strFilePath = "C:/Windows/Dit/test.txt";
// 
// 	std::string strFileName = GetPathOrURLShortName(strFilePath);
// 
// 	cout << strFileName << endl;
// 
// 	system("pause");
// 
// 	return 0;
// }

//************************************
// Method:    string_replace
// FullName:  string_replace
// Access:    public 
// Returns:   void
// Qualifier: 把字符串的strsrc替换成strdst
// Parameter: std::string & strBig
// Parameter: const std::string & strsrc
// Parameter: const std::string & strdst
//************************************
void string_replace(std::string &strBig, const std::string &strsrc, const std::string &strdst)
{
	std::string::size_type pos = 0;
	std::string::size_type srclen = strsrc.size();
	std::string::size_type dstlen = strdst.size();

	while ((pos = strBig.find(strsrc, pos)) != std::string::npos)
	{
		strBig.replace(pos, srclen, strdst);
		pos += dstlen;
	}
}

//************************************
// Method:    GetFileOrURLShortName
// FullName:  GetFileOrURLShortName
// Access:    public 
// Returns:   std::string
// Qualifier: 获取路径或URL的文件名（包括后缀，如 C:\Test\abc.xyz --> abc.xyz）
// Parameter: std::string strFullName
//************************************
std::string extract_filename(std::string strFullName)
{
	if (strFullName.empty())
	{
		return "";
	}

	string_replace(strFullName, "/", "\\");

	std::string::size_type iPos = strFullName.find_last_of('\\') + 1;

	return strFullName.substr(iPos, strFullName.length() - iPos);
}
