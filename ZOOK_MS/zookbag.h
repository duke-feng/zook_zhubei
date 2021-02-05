/*
	对zookeeper的部分功能进行封装
	便于调用


	/ 默认zookeeper日志输出到stderr，
	// 可以调用zoo_set_log_stream(FILE*)设置输出到文件中
	// 还可以调用zoo_set_debug_level(ZooLogLevel)控制日志级别！！！
	//
	// 提供基于zookeeper的主备切换接口和读取数据等接口
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
	static bool node_exists_exception(int errcode) { return ZNODEEXISTS == errcode; }			//节点已经存在(The node already exists)
	static bool node_no_exists_exception(int errcode) { return ZNONODE == errcode; }			//节点不存在(Node does not exist)
	static bool invalid_handle_exception(int errcode) { return ZINVALIDSTATE == errcode; }		//非法句柄状态(Invliad zhandle state)
	static bool connection_loss_exception(int errcode) { return ZCONNECTIONLOSS == errcode; }	//Zookeeper 客户端与服务器端失去连接
	static bool not_empty_exception(int errcode) { return ZNOTEMPTY == errcode; }				//该节点具有自身的子节点(The node has children)
	static bool session_expried_exception(int errcode) { return ZSESSIONEXPIRED == errcode; }	//会话过期(The session has been expired by the server)
	static bool no_authorization_exception(int errcode) { return ZNOAUTH == errcode; }			//没有经过授权(Not authenticated)
	static bool invalid_ACL_exception(int errcode) { return ZINVALIDACL == errcode; }			//非法的ACL(Invalid ACL specified)

public:
	zookbag() throw();
	~zookbag();

	/*
		## 取得zk_path的数据
		zk_data 存放 zk_path的数据
		data_size 最多获取的数据大小，注意数据的实际大小可能比data_size指定的值大
		keep_watch 是否保持watch该path
	*/

	// 如果成功 返回数据实际大小
	// 如果出错 则抛异常
	int get_zk_data(const char * zk_path, string *zk_data, int data_size = SIZE_MAX, bool keep_watch = true) const;
	string get_zk_data(const char * zk_path, int data_size = SIZE_MAX, bool keep_watch = true) const;

	// 设置节点的数据
	void set_zk_data(const string& zk_path, const string& zk_data, struct Stat *stat = NULL, int version = -1);

	// 取函数connect_zookeeper的参数指定的zk_nodes
	const string& get_zk_data() const throw() { return _zk_nodes; }

	// 判断主的Zookeeper客户端是否存在
	bool is_zhu_exist() const throw();

	/*
		## 创建与zookeeper的会话（session）

		建立与zookeepr的连接
		但请注意本函数返回并不表示和zookeeper连接成功，
		只有成员函数on_zookeeper_connected()被回调了才表示连接成功

		zk_nodes 以逗号分隔的zookeeper节点，如：192.168.31.239:2181,192.168.31.240:2181
		session_timeout_seconds 会话（session）超时时长（单位秒）
		注意实际的会话超时时长，是和服务端协商后确定的，并不仅由session_timeout_seconds决定，
		和服务端的配置项tickTime有关，可调用get_session_timeout()取得实际时长。
		服务端会话最小超时时长为minSessionTimeout（默认值为tickTime*2），
		服务端会话最大超时时长为maxSessionTimeout（默认值为tickTime*20），
		服务端的tickTime默认值为2000ms，
		session_timeout_seconds会间于minSessionTimeout和maxSessionTimeout才有效。

		如果报错“no port in ... Invalid argument”，是因为参数zk_nodes没有包含端口或端口号错误
	*/
	void create_session(const string& zk_nodes, int session_timeout_seconds = 10);

	// 关闭与zookeeper的连接
	void close_session();

	/*
		重新创建与zookeeper的会话（Session），
		重连接之前会先关闭和释放已建立连接的资源（包括会话）

		请注意，
		在调用connect_zookeeper()或reconnect_zookeeper()后，
		都应当重新调用race_master()去竞争成为master。
	*/
	void recreate_session();

	// 得到当前连接的zookeeper host    有待修改
	string get_connected_host() const throw();
	bool get_connected_host(string* ip, uint16_t* port) const throw();
	
	// 获取会话超时时长 （单位为毫秒）
	// 注意只有is_connected()返回true时调用才有效
	int get_session_timeout() const;

	// 取得当前状态
	int get_state() const throw();

	// 返回当前是否处于已连接状态
	bool is_connected() const throw ();

	// 返回当前是否处于正在连接状态
	bool is_connecting() const throw();

	bool is_associating() const throw();

	// 返回当前会话（session）是否超时
	bool is_session_expired() const throw();

	// 是否授权失败状态
	bool is_auth_failed() const throw();

	// 取得实际的zookeeper session超时时长，单位为秒
	int get_session_timeout_milliseconds() const;

	/*
		竞争成为master.
		函数is_connected()返回true，方可调用race_master()
		zk_path 用于竞争的path，不能为空，且父path必须已经存在
		path_data 存储到zk_path的数据，可用于识辨当前的master，因此可以考虑使用IP或其它有标识的值
		zk_errcode 出错代码，如果zk_errcode的值为-110（）表示已有master，即node_exists_exception(zk_errcode)返回true
		zk_errmsg 出错信息

		由于仅基于zookeeper的ZOO_EPHEMERAL结点实现互斥，没有组合使用ZOO_SEQUENCE

		如果竞争成功返回true，否则返回false
	*/

	bool race_master(const string& zk_path, const string& path_data, int * zk_errcode = NULL, string* zk_errmsg = NULL, const struct ACL_vector *acl = &ZOO_OPEN_ACL_UNSAFE) throw();


	/*
		创建一个节点，要求父节点必须已经存在且有权限

		zk_parent_path 父节点的路径，值不能以“/”结尾，如果父节点为“/”则值需为空，否则报错“bad arguments”

		flags 可取值：
		 1) 0 普通节点
		 2) ZOO_EPHEMERAL 临时节点
		 3) ZOO_SEQUENCE 顺序节点
		 4) ZOO_EPHEMERAL|ZOO_SEQUENCE 临时顺序节点

		 acl可取值（如果值为NULL，则相当于取值ZOO_OPEN_ACL_UNSAFE）：
		 1) ZOO_OPEN_ACL_UNSAFE 全开放，不做权限控制
		 2) ZOO_READ_ACL_UNSAFE 只读的
		 3) ZOO_CREATOR_ALL_ACL 创建者拥有所有权限
	*/
	// 出错抛异常CException
	void create_node(const string& zk_parent_path, const string& zk_node_name, const string& zk_node_data, int flags, const struct ACL_vector *acl);
	// 出错抛异常CException
	void create_node(const string& zk_parent_path, const string& zk_node_name);
	// 出错抛异常CException
	void create_node(const string& zk_parent_path, const string& zk_node_name, const string& zk_node_data);
	// 出错抛异常CException
	void create_node(const string& zk_parent_path, const string& zk_node_name, const string& zk_node_data, int flags);
	// 出错抛异常CException
	void create_node(const string& zk_parent_path, const string& zk_node_name, const string& zk_node_data, const struct ACL_vector *acl);


	/*
		删除一个节点

		version 如果值为-1，则不做版本检查直接删除，否则因版本不致删除失败
	*/
	// 出错抛异常CException
	void delete_node(const string& zk_path, int version = -1);
	

	/*
		设置ACL（Access Control List）

		 acl参数可取值：
		 1) ZOO_OPEN_ACL_UNSAFE 全开放，不做权限控制
		 2) ZOO_READ_ACL_UNSAFE 只读的
		 3) ZOO_CREATOR_ALL_ACL 创建者拥有所有权限
	*/
	// 出错抛异常CException
	void set_access_control_list(const string& zk_path, const struct ACL_vector *acl, int version = -1);

	// 获取ACL
	// 出错抛异常CException
	void get_access_control_list(const string& zk_path, struct ACL_vector *acl, struct Stat *stat);

	// 取得指定路径下的所有子节点，返回子节点个数
	// 出错抛异常CException
	int get_all_children(vector<std::string>* children, const string& zk_path, bool keep_watch = true);


	/*
		将指定节点的数据存储到文件

		data_filepath 存储数据的文件
		zk_path 数据所在zookeeper节点完整路径
	*/
	// 返回数据的字节数
	// 出错抛异常CSyscallException和CException  有待修改成win下的
	 int store_data(const string& data_filepath, const string& zk_path, bool keep_watch = true);

public:				// 仅局限于被zk_watcher()调用，其它情况均不应当调用
	void zookeeper_session_connected(const char* path);
	void zookeeper_session_connecting(const char* path);
	void zookeeper_session_expired(const char *path);
	void zookeeper_session_event(int state, const char *path);
	void zookeeper_event(int type, int state, const char *path);

private:
	// zookeeper session 连接成功事件
	virtual void on_zookeeper_session_connected(const char* path) {}

	// zookeeper session 正在建立连接事件
	virtual void on_zookeeper_session_connecting(const char* path) {}

	/*
	 zookeeper session 过期事件

	 过期后可以调用recreate_session()重建立与zookeeper的会话
	 注意重连接后得重新调用race_master()竞争master，
	 简单点的做法是session过期后退出当前进程，通过重新启动的方式来竞争master

	 特别注意（session过期的前提是连接已经建立）：
	 如果连接被挂起，不会触发on_zookeeper_session_expired()事件，
	 当用于选master时，调用者需要自己维护这种情况下的超时，
	 以避免临时节点被删除后仍然保持为master状态。
	*/
	virtual void on_zookeeper_session_expired(const char *path) {}

	// session 类zookeeper 事件
	virtual void on_zookeeper_session_event(int state, const char *path) {}

	/*
	 非session类zookeeper事件，包括但不限于：
	 1) 节点被删除（state值为3，type值为2，即type值为ZOO_DELETED_EVENT）
	 2) 节点的数据被修改（state值为3，type值为3，即type值为ZOO_CHANGED_EVENT）
	*/
	// path 触发事件的path，如：/tmp/a
	virtual void on_zookeeper_event(int type, int state, const char *path) {}


private:
	const clientid_t* _zk_clientid;							// 会话编号
	zhandle_t* _zk_handle;									// zookeeper 句柄
	string _zk_nodes;										// 连接zookeeper的节点字符串
	int _expected_session_timeout_seconds;					// 期待的zookeeper会话超时时长
	int _physical_session_timeout_seconds;					// 实际的zookeeper会话超时时长
	time_t _start_connect_time;								// 开始连接时间， 如果连接成功 值为0；
};

#endif

