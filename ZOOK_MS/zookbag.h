/*
	��zookeeper�Ĳ��ֹ��ܽ��з�װ
	���ڵ���


	/ Ĭ��zookeeper��־�����stderr��
	// ���Ե���zoo_set_log_stream(FILE*)����������ļ���
	// �����Ե���zoo_set_debug_level(ZooLogLevel)������־���𣡣���
	//
	// �ṩ����zookeeper�������л��ӿںͶ�ȡ���ݵȽӿ�
*/

#pragma once
#pragma warning(disable:4996)
#ifndef _ZOOKBAG_INCLUDE_
#define _ZOOKBAG_INCLUDE_

#define SIZE_4K 256
#define SIZE_8K 512

#include <zookeeper.h>
#include <iostream>
#include "unistd.h"
#include <vector>
#include <string>
#include <time.h>
#include <fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include "exception.h"
#include "syscall_exception.h"

using namespace std;

class zookbag
{
public:
	static bool node_exists_exception(int errcode) { return ZNODEEXISTS == errcode; }			//�ڵ��Ѿ�����(The node already exists)
	static bool node_no_exists_exception(int errcode) { return ZNONODE == errcode; }			//�ڵ㲻����(Node does not exist)
	static bool invalid_handle_exception(int errcode) { return ZINVALIDSTATE == errcode; }		//�Ƿ����״̬(Invliad zhandle state)
	static bool connection_loss_exception(int errcode) { return ZCONNECTIONLOSS == errcode; }	//Zookeeper �ͻ������������ʧȥ����
	static bool not_empty_exception(int errcode) { return ZNOTEMPTY == errcode; }				//�ýڵ����������ӽڵ�(The node has children)
	static bool session_expried_exception(int errcode) { return ZSESSIONEXPIRED == errcode; }	//�Ự����(The session has been expired by the server)
	static bool no_authorization_exception(int errcode) { return ZNOAUTH == errcode; }			//û�о�����Ȩ(Not authenticated)
	static bool invalid_ACL_exception(int errcode) { return ZINVALIDACL == errcode; }			//�Ƿ���ACL(Invalid ACL specified)

public:
	zookbag() throw();
	~zookbag();

	/*
		## ȡ��zk_path������
		zk_data ��� zk_path������
		data_size ����ȡ�����ݴ�С��ע�����ݵ�ʵ�ʴ�С���ܱ�data_sizeָ����ֵ��
		keep_watch �Ƿ񱣳�watch��path
	*/

	// ����ɹ� ��������ʵ�ʴ�С
	// ������� �����쳣
	int get_zk_data(const char * zk_path, string *zk_data, int data_size = SIZE_MAX, bool keep_watch = true) const;
	string get_zk_data(const char * zk_path, int data_size = SIZE_MAX, bool keep_watch = true) const;

	// ���ýڵ������
	void set_zk_data(const string& zk_path, const string& zk_data, struct Stat *stat = NULL, int version = -1);

	// ȡ����connect_zookeeper�Ĳ���ָ����zk_nodes
	const string& get_zk_data() const throw() { return _zk_nodes; }

	// �ж�����Zookeeper�ͻ����Ƿ����
	bool is_zhu_exist() const throw();

	/*
		## ������zookeeper�ĻỰ��session��

		������zookeepr������
		����ע�Ȿ�������ز�����ʾ��zookeeper���ӳɹ���
		ֻ�г�Ա����on_zookeeper_connected()���ص��˲ű�ʾ���ӳɹ�

		zk_nodes �Զ��ŷָ���zookeeper�ڵ㣬�磺192.168.31.239:2181,192.168.31.240:2181
		session_timeout_seconds �Ự��session����ʱʱ������λ�룩
		ע��ʵ�ʵĻỰ��ʱʱ�����Ǻͷ����Э�̺�ȷ���ģ���������session_timeout_seconds������
		�ͷ���˵�������tickTime�йأ��ɵ���get_session_timeout()ȡ��ʵ��ʱ����
		����˻Ự��С��ʱʱ��ΪminSessionTimeout��Ĭ��ֵΪtickTime*2����
		����˻Ự���ʱʱ��ΪmaxSessionTimeout��Ĭ��ֵΪtickTime*20����
		����˵�tickTimeĬ��ֵΪ2000ms��
		session_timeout_seconds�����minSessionTimeout��maxSessionTimeout����Ч��

		�������no port in ... Invalid argument��������Ϊ����zk_nodesû�а����˿ڻ�˿ںŴ���
	*/
	void create_session(const string& zk_nodes, int session_timeout_seconds = 10);

	// �ر���zookeeper������
	void close_session();

	/*
		���´�����zookeeper�ĻỰ��Session����
		������֮ǰ���ȹرպ��ͷ��ѽ������ӵ���Դ�������Ự��

		��ע�⣬
		�ڵ���connect_zookeeper()��reconnect_zookeeper()��
		��Ӧ�����µ���race_master()ȥ������Ϊmaster��
	*/
	void recreate_session();

	// �õ���ǰ���ӵ�zookeeper host    �д��޸�
	string get_connected_host() const throw();
	bool get_connected_host(string* ip, uint16_t* port) const throw();
	
	// ��ȡ�Ự��ʱʱ�� ����λΪ���룩
	// ע��ֻ��is_connected()����trueʱ���ò���Ч
	int get_session_timeout() const;

	// ȡ�õ�ǰ״̬
	int get_state() const throw();

	// ���ص�ǰ�Ƿ���������״̬
	bool is_connected() const throw ();

	// ���ص�ǰ�Ƿ�����������״̬
	bool is_connecting() const throw();

	bool is_associating() const throw();

	// ���ص�ǰ�Ự��session���Ƿ�ʱ
	bool is_session_expired() const throw();

	// �Ƿ���Ȩʧ��״̬
	bool is_auth_failed() const throw();

	// ȡ��ʵ�ʵ�zookeeper session��ʱʱ������λΪ��
	int get_session_timeout_milliseconds() const;

	/*
		������Ϊmaster.
		����is_connected()����true�����ɵ���race_master()
		zk_path ���ھ�����path������Ϊ�գ��Ҹ�path�����Ѿ�����
		path_data �洢��zk_path�����ݣ�������ʶ�浱ǰ��master����˿��Կ���ʹ��IP�������б�ʶ��ֵ
		zk_errcode ������룬���zk_errcode��ֵΪ-110������ʾ����master����node_exists_exception(zk_errcode)����true
		zk_errmsg ������Ϣ

		���ڽ�����zookeeper��ZOO_EPHEMERAL���ʵ�ֻ��⣬û�����ʹ��ZOO_SEQUENCE

		��������ɹ�����true�����򷵻�false
	*/

	bool race_master(const string& zk_path, const string& path_data, int * zk_errcode = NULL, string* zk_errmsg = NULL, const struct ACL_vector *acl = &ZOO_OPEN_ACL_UNSAFE) throw();


	/*
		����һ���ڵ㣬Ҫ�󸸽ڵ�����Ѿ���������Ȩ��

		zk_parent_path ���ڵ��·����ֵ�����ԡ�/����β��������ڵ�Ϊ��/����ֵ��Ϊ�գ����򱨴�bad arguments��

		flags ��ȡֵ��
		 1) 0 ��ͨ�ڵ�
		 2) ZOO_EPHEMERAL ��ʱ�ڵ�
		 3) ZOO_SEQUENCE ˳��ڵ�
		 4) ZOO_EPHEMERAL|ZOO_SEQUENCE ��ʱ˳��ڵ�

		 acl��ȡֵ�����ֵΪNULL�����൱��ȡֵZOO_OPEN_ACL_UNSAFE����
		 1) ZOO_OPEN_ACL_UNSAFE ȫ���ţ�����Ȩ�޿���
		 2) ZOO_READ_ACL_UNSAFE ֻ����
		 3) ZOO_CREATOR_ALL_ACL ������ӵ������Ȩ��
	*/
	// �������쳣CException
	void create_node(const string& zk_parent_path, const string& zk_node_name, const string& zk_node_data, int flags, const struct ACL_vector *acl);
	// �������쳣CException
	void create_node(const string& zk_parent_path, const string& zk_node_name);
	// �������쳣CException
	void create_node(const string& zk_parent_path, const string& zk_node_name, const string& zk_node_data);
	// �������쳣CException
	void create_node(const string& zk_parent_path, const string& zk_node_name, const string& zk_node_data, int flags);
	// �������쳣CException
	void create_node(const string& zk_parent_path, const string& zk_node_name, const string& zk_node_data, const struct ACL_vector *acl);


	/*
		ɾ��һ���ڵ�

		version ���ֵΪ-1�������汾���ֱ��ɾ����������汾����ɾ��ʧ��
	*/
	// �������쳣CException
	void delete_node(const string& zk_path, int version = -1);
	

	/*
		����ACL��Access Control List��

		 acl������ȡֵ��
		 1) ZOO_OPEN_ACL_UNSAFE ȫ���ţ�����Ȩ�޿���
		 2) ZOO_READ_ACL_UNSAFE ֻ����
		 3) ZOO_CREATOR_ALL_ACL ������ӵ������Ȩ��
	*/
	// �������쳣CException
	void set_access_control_list(const string& zk_path, const struct ACL_vector *acl, int version = -1);

	// ��ȡACL
	// �������쳣CException
	void get_access_control_list(const string& zk_path, struct ACL_vector *acl, struct Stat *stat);

	// ȡ��ָ��·���µ������ӽڵ㣬�����ӽڵ����
	// �������쳣CException
	int get_all_children(vector<std::string>* children, const string& zk_path, bool keep_watch = true);


	/*
		��ָ���ڵ�����ݴ洢���ļ�

		data_filepath �洢���ݵ��ļ�
		zk_path ��������zookeeper�ڵ�����·��
	*/
	// �������ݵ��ֽ���
	// �������쳣CSyscallException��CException  �д��޸ĳ�win�µ�
	 int store_data(const string& data_filepath, const string& zk_path, bool keep_watch = true);

public:				// �������ڱ�zk_watcher()���ã������������Ӧ������
	void zookeeper_session_connected(const char* path);
	void zookeeper_session_connecting(const char* path);
	void zookeeper_session_expired(const char *path);
	void zookeeper_session_event(int state, const char *path);
	void zookeeper_event(int type, int state, const char *path);

private:
	// zookeeper session ���ӳɹ��¼�
	virtual void on_zookeeper_session_connected(const char* path) {}

	// zookeeper session ���ڽ��������¼�
	virtual void on_zookeeper_session_connecting(const char* path) {}

	/*
	 zookeeper session �����¼�

	 ���ں���Ե���recreate_session()�ؽ�����zookeeper�ĻỰ
	 ע�������Ӻ�����µ���race_master()����master��
	 �򵥵��������session���ں��˳���ǰ���̣�ͨ�����������ķ�ʽ������master

	 �ر�ע�⣨session���ڵ�ǰ���������Ѿ���������
	 ������ӱ����𣬲��ᴥ��on_zookeeper_session_expired()�¼���
	 ������ѡmasterʱ����������Ҫ�Լ�ά����������µĳ�ʱ��
	 �Ա�����ʱ�ڵ㱻ɾ������Ȼ����Ϊmaster״̬��
	*/
	virtual void on_zookeeper_session_expired(const char *path) {}

	// session ��zookeeper �¼�
	virtual void on_zookeeper_session_event(int state, const char *path) {}

	/*
	 ��session��zookeeper�¼��������������ڣ�
	 1) �ڵ㱻ɾ����stateֵΪ3��typeֵΪ2����typeֵΪZOO_DELETED_EVENT��
	 2) �ڵ�����ݱ��޸ģ�stateֵΪ3��typeֵΪ3����typeֵΪZOO_CHANGED_EVENT��
	*/
	// path �����¼���path���磺/tmp/a
	virtual void on_zookeeper_event(int type, int state, const char *path) {}


private:
	const clientid_t* _zk_clientid;							// �Ự���
	zhandle_t* _zk_handle;									// zookeeper ���
	string _zk_nodes;										// ����zookeeper�Ľڵ��ַ���
	int _expected_session_timeout_seconds;					// �ڴ���zookeeper�Ự��ʱʱ��
	int _physical_session_timeout_seconds;					// ʵ�ʵ�zookeeper�Ự��ʱʱ��
	time_t _start_connect_time;								// ��ʼ����ʱ�䣬 ������ӳɹ� ֵΪ0��
};

#endif

