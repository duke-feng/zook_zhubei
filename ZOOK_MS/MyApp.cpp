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
	//����ʱ����master��
	//�ڳ�Ϊmaster֮ǰ���ܽ��빤��״̬
	while (!_stop)
	{
		int zk_errcode;
		string zk_errmsg;
		//Sleep(5000);
		if (race_master(_master_path, _master_data, &zk_errcode, &zk_errmsg))
		{
			//д����־ ��ʱ�������ʾ
			//��Ϊmaster��
			//Ҫ��ԭ����master���㹻ʱ���˳�master״̬
			printf("Race master at %s with %s successfully, sleep for 10 seconds to let the old master quit\n", _master_path.c_str(), _master_data.c_str());
			LOG(INFO) << FormatString("Race master at %s with %s successfully, sleep for 10 seconds to let the old master quit\n", _master_path.c_str(), _master_data.c_str());
			//Sleep(10000);
			printf("Start working now\n");
			LOG(INFO) << "Start working now";

			work();
			if (!_stop)
			{
				//�˳�work()����ʾ��Ҫ���¾���master
				printf("Turn to slave from master at %s with %s successfully, stop working now\n", _master_path.c_str(), _master_data.c_str());
				LOG(INFO) << FormatString("Turn to slave from master at %s with %s successfully, stop working now\n", _master_path.c_str(), _master_data.c_str());
			}
		}
		else
		{
			//���node_exists_exception()����true����ʾ����master��
			//��_master_path�Ѵ��ڣ�����falseΪ��������Ӧ��������Ϣ��¼����־
			if (node_exists_exception(zk_errcode))
			{
				char p[512] = {0};
				strcpy(p,_master_path.c_str());
				//Sleep(10000);
				if (get_zk_data(p, 512, true) == "zk_1")		// ͨ���ж�������master�Ƿ���һ�������ж������ĻỰ�ǲ���һ���ġ��������Ի��ɻỰID
				{
					cout << "��������������������ɹ���������" << endl;
					LOG(INFO) << "��������������������ɹ���������";
					work();
					if (!_stop)
					{
						//�˳�work()����ʾ��Ҫ���¾���master
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

			//��Ϣ2����ٳ��ԣ���Ҫ��Ƶ���ԣ�һ�������1~10�붼�ǿɽ��ܵ�
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
	
		// Ҫ��ʱ���is_connected()���Է�ֹmasterʧЧ��ͬʱ��������master  
		while (!_stop && !is_session_expired())
		{
			if (is_zhu_exist() && is_connected())		//��Ĭ�ϵĿͻ��˽���
			{
				cout << "��zk���ڣ���Ӧ���˳����ȴ�10s��������" << endl;
				LOG(INFO) << "��zk���ڣ���Ӧ���˳����ȴ�10s��������";
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
		cout << "�����ɹ�" << endl;
		LOG(INFO) << "�����ɹ�";
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
	//exit(1); �ȫ��������������ֱ���˳���ͨ������������ʽ�ٴξ���master
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