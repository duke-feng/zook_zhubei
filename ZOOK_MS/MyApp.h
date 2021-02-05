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
	string _master_path;				//用来竞争master的zookeeper节点路径
	string _master_data;			//成功竞争为master时，写入_master_path的数据，主备应当提供不同的数据，以方便判断自己是否处于主状态
	string zk_parent_path;
	string zk_node_name;
	string zk_node_data;
	
};