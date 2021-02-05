#include "zookbag.h"
#include "logger.h"
#include "zookbag.cpp"


class MyApp : public zookbag
{
public:
	MyApp(const char* data, const char* path, const char* p_path, const char* zk_name, const char* zk_data);
	~MyApp();
	void stop() { _stop = true; }
	void run();
	void wait();

private:
	void work();

private:
	virtual void on_zookeeper_session_connected(const char* path);
	virtual void on_zookeeper_session_connecting(const char* path);
	virtual void on_zookeeper_session_expired(const char *path);
	virtual void on_zookeeper_session_event(int state, const char *path);
	virtual void on_zookeeper_event(int type, int state, const char *path);

private:
	volatile bool _stop;
	string _master_path;				//��������master��zookeeper�ڵ�·��
	string _master_data;			//�ɹ�����Ϊmasterʱ��д��_master_path�����ݣ�����Ӧ���ṩ��ͬ�����ݣ��Է����ж��Լ��Ƿ�����״̬
	string zk_parent_path;
	string zk_node_name;
	string zk_node_data;
	
};