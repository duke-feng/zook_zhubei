#include "MyApp.h"
#include <fstream>
#include <tchar.h>


int main(int argc, char* argv[])
{
		zoo_set_debug_level(ZOO_LOG_LEVEL_ERROR);

		char IniFile[256] = { 0 };
		GetCurrentDirectory(256, IniFile);
		strcat_s(IniFile, "\\STG.ini");
		char host[64] = { 0 };
		char path[512] = { 0 };
		char data[512] = { 0 };
		char timeout[512] = { 0 };	//会话等待时间
		char parent_path[256] = { 0 };
		char node_name[256] = { 0 };
		char node_data[512] = { 0 };

		fstream _file;				//查找STG.ini是否存在
		_file.open(IniFile, ios::in);

		if (_file)
		{
			::GetPrivateProfileString(_T("ZOOKEEPER"), _T("ZK_HOST"), _T("Default"), host, sizeof(host), _T(IniFile));
			::GetPrivateProfileString(_T("ZOOKEEPER"), _T("ZK_PATH"), _T("Default"), path, sizeof(path), _T(IniFile));
			::GetPrivateProfileString(_T("ZOOKEEPER"), _T("ZK_DATA"), _T("Default"), data, sizeof(data), _T(IniFile));
			::GetPrivateProfileString(_T("ZOOKEEPER"), _T("TIMEOUT"), _T("Default"), timeout, sizeof(timeout), _T(IniFile));
			::GetPrivateProfileString(_T("ZOOKEEPER"), _T("ZK_PARENT_PATH"), _T("Default"), parent_path, sizeof(parent_path), _T(IniFile));
			::GetPrivateProfileString(_T("ZOOKEEPER"), _T("ZK_NODE_NAME"), _T("Default"), node_name, sizeof(node_name), _T(IniFile));
			::GetPrivateProfileString(_T("ZOOKEEPER"), _T("ZK_NODE_DATA"), _T("Default"), node_data, sizeof(node_data), _T(IniFile));
		}
		else
		{
			cout << "connot find file:STG.ini" << endl;
			exit(1);
		}

		//string data_1 = data;

 		try
 		{
		const string zk_nodes = host;				//123.57.18.90:2181
		const int session_timeout_seconds = atoi(timeout);

		MyApp myapp(data, path, parent_path, node_name, node_data);
		myapp.create_session(zk_nodes, session_timeout_seconds);
		initLogger("log/info.log", "log/warning.log", "log/error.log");
		myapp.run();
		myapp.wait();

		
		}
 		catch (CSyscallException& ex)						//捕捉异常
 		{
 			fprintf(stderr, "%s\n", ex.str().c_str());		//屏幕错误直接输出
 			exit(1);
 		}
 		catch (CException& ex)							
 		{
 			fprintf(stderr, "%s\n", ex.str().c_str());		//屏幕错误直接输出
			exit(1);
		}
return 0;
}
