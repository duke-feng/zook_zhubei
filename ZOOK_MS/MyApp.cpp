#include "MyApp.h"

MyApp::MyApp(const char* data, const char* path, const char* p_path, const char* zk_name, const char* zk_data)
	: _stop(false)
{
	_master_path = path;
	zk_parent_path = p_path;
	zk_node_name = zk_name;
	zk_node_data = zk_data;
	if (data != NULL)
		_master_data = data;
}

void MyApp::run()
{
	//启动时竞争master，
	//在成为master之前不能进入工作状态
	while (!_stop)
	{
		int zk_errcode;
		string zk_errmsg;
		//Sleep(5000);
		if (race_master(_master_path, _master_data, &zk_errcode, &zk_errmsg))
		{
			//写进日志 暂时以输出显示
			//成为master后，
			//要让原来的master有足够时间退出master状态
			printf("Race master at %s with %s successfully, sleep for 10 seconds to let the old master quit\n", _master_path.c_str(), _master_data.c_str());
			LOG(INFO) << FormatString("Race master at %s with %s successfully, sleep for 10 seconds to let the old master quit\n", _master_path.c_str(), _master_data.c_str());
			//Sleep(10000);
			printf("Start working now\n");
			LOG(INFO) << "Start working now";

			work();
			if (!_stop)
			{
				//退出work()，表示需要重新竞争master
				printf("Turn to slave from master at %s with %s successfully, stop working now\n", _master_path.c_str(), _master_data.c_str());
				LOG(INFO) << FormatString("Turn to slave from master at %s with %s successfully, stop working now\n", _master_path.c_str(), _master_data.c_str());
			}
		}
		else
		{
			//如果node_exists_exception()返回true，表示已有master，
			//即_master_path已存在，返回false为其它错误，应将错误信息记录到日志
			if (node_exists_exception(zk_errcode))
			{
				char p[512] = {0};
				strcpy(p,_master_path.c_str());
				//Sleep(10000);
				if (get_zk_data(p, 512, true) == "zk_1")		// 通过判断重连的master是否是一样的来判定重连的会话是不是一样的。后续可以换成会话ID
				{
					cout << "服务器意外待机，重连成功继续工作" << endl;
					LOG(INFO) << "服务器意外待机，重连成功继续工作";
					work();
					if (!_stop)
					{
						//退出work()，表示需要重新竞争master
						printf("Turn to slave from master at %s with %s successfully, stop working now\n", _master_path.c_str(), _master_data.c_str());
						LOG(INFO) << FormatString("Turn to slave from master at %s with %s successfully, stop working now\n", _master_path.c_str(), _master_data.c_str());
					}
					//recreate_session();
				}
				else
				{
					printf("A master exists\n");
					LOG(INFO) << "A master exists";
				}
					
					//Sleep(8000);
			}
			else
			{
				printf("Race master at %s with %s failed: (state:%d)(errcode:%d)%s\n", _master_path.c_str(), _master_data.c_str(), get_state(), zk_errcode, zk_errmsg.c_str());
				LOG(INFO) << FormatString("Race master at %s with %s failed: (state:%d)(errcode:%d)%s\n", _master_path.c_str(), _master_data.c_str(), get_state(), zk_errcode, zk_errmsg.c_str());
				if (/*connection_loss_exception(zk_errcode) || */session_expried_exception(zk_errcode) || invalid_handle_exception(zk_errcode))
				{
					printf("To recreate session\n");
					LOG(INFO) << "To recreate session";
					recreate_session();
				}
			}

			//休息2秒后再尝试，不要过频重试，一般情况下1~10秒都是可接受的
			Sleep(2000);
		}
	}

	printf("Exit now\n");
}

void MyApp::wait()
{
}

void MyApp::work()
{
	
		// 要及时检查is_connected()，以防止master失效后同时存在两个master  
		while (!_stop && !is_session_expired())
		{
			if (is_zhu_exist() && is_connected())		//供默认的客户端晋升
			{
				cout << "主zk存在，备应该退出，等待10s让主晋升" << endl;
				LOG(INFO) << "主zk存在，备应该退出，等待10s让主晋升";
				delete_node(_master_path, -1);
				Sleep(10000);
				return;
			}
			else
			{
				Sleep(2000);
				printf("Working with state: %d  ...\n", get_state());
				LOG(INFO) << FormatString("Working with state: %d  ...\n", get_state());
			}
		}
}

void MyApp::on_zookeeper_session_connected(const char* path)
{
	printf("zookeeper_session_connected path: %s\n", path);

	/*const string zk_parent_path = "/nodes";
	const string zk_node_name = "test1";
	const string zk_node_data = "123";*/

	try
	{
		create_node(zk_parent_path, zk_node_name, zk_node_data, ZOO_EPHEMERAL);
		printf("Create %s/%s ok\n", zk_parent_path.c_str(), zk_node_name.c_str());
		LOG(INFO) << FormatString("Create %s/%s ok\n", zk_parent_path.c_str(), zk_node_name.c_str());
	}
	catch (int i)
	{
		cout << "重连成功" << endl;
		LOG(INFO) << "重连成功";
		//printf("Create %s/%s failed: %d\n", zk_parent_path.c_str(), zk_node_name.c_str(), i);
	}
}

void MyApp::on_zookeeper_session_connecting(const char* path)
{
	printf("zookeeper_session_connecting path: %s\n", path);
}

void MyApp::on_zookeeper_session_expired(const char *path)
{
	printf("zookeeper_session_expired path: %s\n", path);
	//exit(1); 最安全的做法，在这里直接退出，通过重新启动方式再次竞争master
}

void MyApp::on_zookeeper_session_event(int state, const char *path)
{
	printf("zookeeper_session_event[state:%d] path: %s\n", state, path);
}

void MyApp::on_zookeeper_event(int type, int state, const char *path)
{
	printf("zookeeper_event [type:%d][state:%d] path: %s\n", type, state, path);

	if (type == 3)
	{
		const int data_size = SIZE_4K;
		const bool keep_watch = true;
		string zk_data;
		const int n = get_zk_data(path, &zk_data, data_size, keep_watch);
		printf("(%d/%zd)%s\n", n, zk_data.size(), zk_data.c_str());
	}
}

MyApp::~MyApp()
{

}