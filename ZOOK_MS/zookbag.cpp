#include "zookbag.h"

inline string ipv6_tostring(struct in6_addr addrip)			// ipv6_转string
{
	char * ip;
	unsigned int intIP;
	memcpy(&intIP, &addrip, sizeof(unsigned int));
	int a = (intIP >> 24) & 0xFF;
	int b = (intIP >> 16) & 0xFF;
	int c = (intIP >> 8) & 0xFF;
	int d = intIP & 0xFF;
	if ((ip = (char *)malloc(16 * sizeof(char))) == NULL)
	{
		return NULL;
	}
	//sprintf(ip, "%d.%d.%d.%d", d, c, b, a);
	return ip;
}

inline char* to_string(struct in_addr addrip)				// in_addr 转 string（char*）
{
	char * ip;
	unsigned int intIP;
	memcpy(&intIP, &addrip, sizeof(unsigned int));
	int a = (intIP >> 24) & 0xFF;
	int b = (intIP >> 16) & 0xFF;
	int c = (intIP >> 8) & 0xFF;
	int d = intIP & 0xFF;
	if ((ip = (char *)malloc(16 * sizeof(char))) == NULL)
	{
		return NULL;
	}
	//sprintf(ip, "%d.%d.%d.%d", d, c, b, a);
	return ip;
}

inline string FormatString(const char *lpcszFormat, ...)   //字符串格式化函数
{
	char *pszStr = NULL;
	if (NULL != lpcszFormat)
	{
		va_list marker = NULL;
		va_start(marker, lpcszFormat); //初始化变量参数
		size_t nLength = _vscprintf(lpcszFormat, marker) + 1; //获取格式化字符串长度
		pszStr = new char[nLength];
		memset(pszStr, '\0', nLength);
		_vsnprintf_s(pszStr, nLength, nLength, lpcszFormat, marker);
		va_end(marker); //重置变量参数
	}
	string strResult(pszStr);
	delete[]pszStr;
	return strResult;
}



inline static void zk_watcher(zhandle_t* zh, int type, int state, const char * path, void * contect)
{
	zookbag* self = static_cast<zookbag*> (contect);

	if (type != ZOO_SESSION_EVENT)
	{
		//cout << "1" << endl;
		self->zookeeper_event(type, state, path);
	}
	else
	{
		/*
		 0: 初始状态（也就是没有状态的状态）
		 1: ZOO_CONNECTING_STATE
		 2: ZOO_ASSOCIATING_STATE
		 3: ZOO_CONNECTED_STATE
		 -112: ZOO_EXPIRED_SESSION_STATE
		 -113: ZOO_AUTH_FAILED_STATE
		 999: NOTCONNECTED_STATE_DEF

		 状态间转换：
			ZOO_CONNECTING_STATE
		 -> ZOO_ASSOCIATING_STATE
		 -> ZOO_CONNECTED_STATE/ZOO_EXPIRED_SESSION_STATE
		 -> ZOO_AUTH_FAILED_STATE
		*/

		if (ZOO_EXPIRED_SESSION_STATE == state)
		{
			//	需要重新调用 zookeeper_init ，简单点可以退出当前进程重启
			self->zookeeper_session_expired(path);
		}
		else if (ZOO_CONNECTED_STATE == state)
		{
			// zookeeper_init成功时type为ZOO_SESSION_EVENT，state为ZOO_CONNECTED_STATE
			self->zookeeper_session_connected(path);
		}
		else if (ZOO_CONNECTING_STATE == state)
		{
			cout << "connecting" << endl;
			self->zookeeper_session_connecting(path);
		}
		else
		{
			cout << "session" << endl;
			self->zookeeper_session_event(state, path);
		}
	}

	// zoo_set_watcher returns the previous watcher function
	//(void)zoo_set_watcher(zh, zk_watcher);
}

inline zookbag::zookbag() throw()
	: _zk_clientid(NULL), _zk_handle(NULL),
	_expected_session_timeout_seconds(10),
	_physical_session_timeout_seconds(0),
	_start_connect_time(0)
{
}

inline zookbag::~zookbag()
{
	close_session();
}

inline int zookbag::get_zk_data(const char * zk_path, string *zk_data, int data_size, bool keep_watch) const
{
	const int watch = keep_watch ? 1 : 0;
	const int data_size_ = (data_size < 1) ? 1 : data_size;
	struct Stat stat;
	int datalen = data_size_;

	zk_data->resize(datalen);
	const int errcode = zoo_get(_zk_handle, zk_path, watch, const_cast<char*>(zk_data->data()), &datalen, &stat);
	if (ZOK == errcode)
	{
		zk_data->resize(datalen);
		return stat.dataLength;
	}
	else
	{
		// 抛出异常,异常日志
		THROW_EXCEPTION(FormatString("get path://%s data failed: %s", zk_path, zerror(errcode)), errcode);
		//printf("get path://%s data failed: %s", zk_path, zerror(errcode));
	}
}

inline string zookbag::get_zk_data(const char* zk_path, int data_size, bool keep_watch) const
{
	string zk_data;
	(void)get_zk_data(zk_path, &zk_data, data_size, keep_watch);
	return zk_data;
}

inline void zookbag::set_zk_data(const string& zk_path, const string& zk_data, struct Stat *stat, int version)
{
	int errcode;
	if (NULL == stat)
		errcode = zoo_set(_zk_handle, zk_path.c_str(), zk_data.data(), static_cast<int>(zk_data.size()), version);
	else
		errcode = zoo_set2(_zk_handle, zk_path.c_str(), zk_data.data(), static_cast<int>(zk_data.size()), version, stat);

	if (errcode != ZOK)
	{
		THROW_EXCEPTION(FormatString("set path://%s data failed: %s", zk_path.c_str(), zerror(errcode)), errcode);
		printf("set path://%s data failed: %s", zk_path.c_str(), zerror(errcode));
	}
}

inline bool zookbag::is_zhu_exist() const throw()
{
	bool i = true;
	if (zoo_exists(_zk_handle, "/nodes/test", NULL, NULL) == ZNONODE)
		i = false;
	return i;
}


inline void zookbag::create_session(const string& zk_nodes, int session_timeout_seconds)
{
	if (zk_nodes.empty())
	{
		THROW_EXCEPTION("parameter[zk_nodes] of connect_zookeeper is empty", EINVAL);
		printf("parameter[zk_nodes] of connect_zookeeper is empty");			// 如果创建的ip地址为空
	}
	else
	{
		_expected_session_timeout_seconds = session_timeout_seconds * 1000;
		_zk_nodes = zk_nodes;

		// 当连接不上时，会报如下错误（默认输出到stderr，可通过zoo_set_log_stream(FILE*)输出到文件）：
		// zk retcode=-4, errno=111(Connection refused): server refused to accept the client
		_zk_handle = zookeeper_init(_zk_nodes.c_str(), zk_watcher, _expected_session_timeout_seconds, _zk_clientid, this, 0);
		if (NULL == _zk_handle)
		{
			THROW_EXCEPTION(FormatString("connect nodes://%s failed: %s", _zk_nodes.c_str(), strerror(errno)), errno);
			printf("connect nodes://%s failed: %s", _zk_nodes.c_str(), strerror(errno));
		}
	}
}

inline void zookbag::close_session()
{
	if (_zk_handle != NULL)
	{
		int errcode = zookeeper_close(_zk_handle);

		if (errcode != ZOK)
		{
			// 出错可能是因为内存不足或网络超时等
			THROW_EXCEPTION(FormatString("close nodes://%s failed: %s", _zk_nodes.c_str(), zerror(errcode)), errcode);
			printf("close nodes://%s failed: %s", _zk_nodes.c_str(), zerror(errcode));
		}
		else
		{
			_zk_handle = NULL;
			_zk_clientid = NULL;
		}
	}
}

inline void zookbag::recreate_session()
{
	close_session();
	_zk_handle = zookeeper_init(_zk_nodes.c_str(), zk_watcher, _expected_session_timeout_seconds, _zk_clientid, this, 0);

	if (NULL == _zk_handle)
	{
		THROW_EXCEPTION(FormatString("reconnect nodes://%s failed: %s", _zk_nodes.c_str(), strerror(errno)), errno);
		printf("reconnect nodes://%s failed: %s", _zk_nodes.c_str(), strerror(errno));
	}
}

//  修改

inline string zookbag::get_connected_host() const throw ()
{
	std::string ip;
	uint16_t port = 0;
	get_connected_host(&ip, &port);
	return FormatString("%s:%u", ip.c_str(), port);
}

inline bool zookbag::get_connected_host(string* ip, uint16_t* port) const throw ()
{
	struct sockaddr_in6 addr_in6;
	socklen_t addr_len = sizeof(addr_in6);
	if (NULL == zookeeper_get_connected_host(_zk_handle, reinterpret_cast<struct sockaddr*>(&addr_in6), &addr_len))
	{
		return false;
	}

	const struct sockaddr* addr = reinterpret_cast<struct sockaddr*>(&addr_in6);
	if (AF_INET == addr->sa_family)
	{
		const struct sockaddr_in* addr_in = reinterpret_cast<const struct sockaddr_in*>(addr);
		*ip = to_string(addr_in->sin_addr);
		*port = htons(addr_in->sin_port);
	}
	else if (AF_INET6 == addr->sa_family)
	{
		*ip = ipv6_tostring(addr_in6.sin6_addr);
		*port = htons(addr_in6.sin6_port);
	}
	else
	{
		return false;
	}

	return true;
}



inline int zookbag::get_session_timeout() const
{
	return zoo_recv_timeout(_zk_handle);
}

inline int zookbag::get_state() const throw ()
{
	return zoo_state(_zk_handle);
}

inline bool zookbag::is_connected() const throw ()
{
	// 3: ZOO_CONNECTED_STATE
	const int state = zoo_state(_zk_handle);
	return (ZOO_CONNECTED_STATE == state);
}

inline bool zookbag::is_connecting() const throw()
{
	// 1: ZOO_CONNECTING_STATE
	const int state = zoo_state(_zk_handle);
	return (ZOO_CONNECTING_STATE == state);
}

inline bool zookbag::is_session_expired() const throw()
{
	// -112: ZOO_EXPIRED_SESSION_STATE
	const int state = zoo_state(_zk_handle);

	if ((ZOO_EXPIRED_SESSION_STATE == state))
	{
		return true;
	}
	else if (_start_connect_time > 0)
	{
		// 如果是zookeeper服务端被挂起，连接永久不会触发session的expired	通过判断时间可以知道是否过了会话时间
		const time_t current_time = time(NULL);

		if (current_time > _start_connect_time)
		{
			// 增加1秒的安全值
			const int session_timeout_seconds = get_session_timeout_milliseconds();
			return static_cast<int>(current_time - _start_connect_time) > (session_timeout_seconds / 1000 - 1);
		}
	}

	return false;
}

inline bool zookbag::is_auth_failed() const throw()
{
	// -113: ZOO_AUTH_FAILED_STATE
	const int state = zoo_state(_zk_handle);		//通过判断句柄的状态来判断状态的改变，会话过期只能服务器发送，正在连接断开默认重连
	return (ZOO_AUTH_FAILED_STATE == state);
}

inline int zookbag::get_session_timeout_milliseconds() const
{
	return zoo_recv_timeout(_zk_handle);
}

inline bool zookbag::race_master(const string& zk_path, const string& path_data, int* zk_errcode, string* zk_errmsg, const struct ACL_vector *acl) throw ()
{
	// ZOO_EPHEMERAL|ZOO_SEQUENCE
	// 调用之前，需要确保_zk_path的父路径已存在
	// (-4)connection loss，比如为zookeeper_init指定了无效的hosts（一个有效的host也没有）
	const int errcode_ = zoo_create(_zk_handle, zk_path.c_str(), path_data.c_str(), static_cast<int>(path_data.size()), acl, ZOO_EPHEMERAL, NULL, 0);

	if (ZOK == errcode_)
	{
		if (zk_errcode != NULL)
			*zk_errcode = errcode_;
		if (zk_errmsg != NULL)
			*zk_errmsg = "SUCCESS";
		return true;
	}
	else
	{
		if (zk_errcode != NULL)
			*zk_errcode = errcode_;
		if (zk_errmsg != NULL)
			*zk_errmsg = zerror(errcode_);
		// ZOK operation completed successfully
		// ZNONODE the parent node does not exist
		// ZNODEEXISTS the node already exists
		return false;
	}
}

inline void zookbag::create_node(const string& zk_parent_path, const string& zk_node_name, const string& zk_node_data, int flags, const struct ACL_vector *acl)
{
	const struct ACL_vector *acl_ = (NULL == acl) ? &ZOO_OPEN_ACL_UNSAFE : acl;
	const string& zk_path = zk_parent_path + string("/") + zk_node_name;
	const int errcode = zoo_create(_zk_handle, zk_path.c_str(), zk_node_data.c_str(), static_cast<int>(zk_node_data.size()), acl_, flags, NULL, 0);
	if (errcode != ZOK)
	{
		//throw(FormatString("create path://%s failed: %s", zk_path.c_str(), zerror(errcode)), errcode);
		throw 1;
		// printf("create path://%s failed: %s", zk_path.c_str(), zerror(errcode));
	}
}

inline void zookbag::create_node(const string& zk_parent_path, const string& zk_node_name)
{
	const string zk_node_data;
	const int flags = -1;
	const struct ACL_vector *acl = NULL;
	create_node(zk_parent_path, zk_node_name, zk_node_data, flags, acl);
}

inline void zookbag::create_node(const string& zk_parent_path, const string& zk_node_name, const string& zk_node_data)
{
	const int flags = -1;
	const struct ACL_vector *acl = NULL;
	create_node(zk_parent_path, zk_node_name, zk_node_data, flags, acl);
}

inline void zookbag::create_node(const string& zk_parent_path, const string& zk_node_name, const string& zk_node_data, int flags)
{
	const struct ACL_vector *acl = NULL;
	create_node(zk_parent_path, zk_node_name, zk_node_data, flags, acl);
}

inline void zookbag::create_node(const string& zk_parent_path, const string& zk_node_name, const string& zk_node_data, const struct ACL_vector *acl)
{
	const int flags = -1;
	create_node(zk_parent_path, zk_node_name, zk_node_data, flags, acl);
}

inline void zookbag::delete_node(const string& zk_path, int version)
{
	const int errcode = zoo_delete(_zk_handle, zk_path.c_str(), version);
	if (errcode != ZOK)
	{
		THROW_EXCEPTION(FormatString("delete path://%s failed: %s", zk_path.c_str(), zerror(errcode)), errcode);
		printf("delete path://%s failed: %s", zk_path.c_str(), zerror(errcode));
	}
}

inline void zookbag::set_access_control_list(const std::string& zk_path, const struct ACL_vector *acl, int version)
{
	const int errcode = zoo_set_acl(_zk_handle, zk_path.c_str(), version, acl);
	if (errcode != ZOK)
	{
		THROW_EXCEPTION(FormatString("set path://%s ACL failed: %s", zk_path.c_str(), zerror(errcode)), errcode);
		printf("set path://%s ACL failed: %s", zk_path.c_str(), zerror(errcode));
	}
}

inline void zookbag::get_access_control_list(const string& zk_path, struct ACL_vector *acl, struct Stat *stat)
{
	const int errcode = zoo_get_acl(_zk_handle, zk_path.c_str(), acl, stat);
	if (errcode != ZOK)
	{
		THROW_EXCEPTION(FormatString("get path://%s ACL failed: %s", zk_path.c_str(), zerror(errcode)), errcode);
		printf("get path://%s ACL failed: %s", zk_path.c_str(), zerror(errcode));
	}
}

inline int zookbag::get_all_children(vector<string>* children, const string& zk_path, bool keep_watch)
{
	struct String_vector strings;
	const int watch = keep_watch ? 1 : 0;
	const int errcode = zoo_get_children(_zk_handle, zk_path.c_str(), watch, &strings);
	if (errcode != ZOK)
	{
		THROW_EXCEPTION(FormatString("get path://%s children failed: %s", zk_path.c_str(), zerror(errcode)), errcode);
		printf("get path://%s children failed: %s", zk_path.c_str(), zerror(errcode));
	}

	children->resize(strings.count);
	for (int i = 0; i < strings.count; ++i)
	{
		(*children)[i] = strings.data[i];
		//free(strings.data[i]);
	}
	deallocate_String_vector(&strings);
	return static_cast<int>(children->size());
}


//修改成 win 下的  暂时没有调用
inline int zookbag::store_data(const string& data_filepath, const string& zk_path, bool keep_watch)
{
	int errcode = 0;
	string zk_data;
	int n = get_zk_data(zk_path.c_str(), &zk_data, SIZE_8K);
	if (n > SIZE_8K)
		n = get_zk_data(zk_path.c_str(), &zk_data, n);

	const int fd = open(data_filepath.c_str(), O_WRONLY | O_CREAT | O_TRUNC);			//删除第三个参数FILE_DEFAULT_PERM
	if (-1 == fd)
	{
		errcode = errno;
		THROW_SYSCALL_EXCEPTION(FormatString("open file://%s with write|create|truncate failed: %s", data_filepath.c_str(), strerror(errcode)), errcode, "open");
		printf("open file://%s with write|create|truncate failed: %s", data_filepath.c_str(), strerror(errcode));
	}
	else
	{
		const int n = write(fd, zk_data.data(), zk_data.size());

		if (n != static_cast<int>(zk_data.size()))
		{
			errcode = errno;
			close(fd);
			THROW_SYSCALL_EXCEPTION(FormatString("write file://%s failed: %s", data_filepath.c_str(), strerror(errcode)), errcode, "write");
			printf("write file://%s failed: %s", data_filepath.c_str(), strerror(errcode));
		}
		else
		{
			if (close(fd) != 0)
			{
				errcode = errno;
				THROW_SYSCALL_EXCEPTION(FormatString("close file://%s failed: %s", data_filepath.c_str(), strerror(errcode)), errcode, "close");
				printf("close file://%s failed: %s", data_filepath.c_str(), strerror(errcode));
			}

			return n;
		}
	}
}


inline void zookbag::zookeeper_session_connected(const char* path)
{
	_zk_clientid = zoo_client_id(_zk_handle);
	_physical_session_timeout_seconds = get_session_timeout_milliseconds();
	_start_connect_time = 0;
	this->on_zookeeper_session_connected(path);
}

inline void zookbag::zookeeper_session_connecting(const char* path)
{
	_start_connect_time = time(NULL);
	this->on_zookeeper_session_connecting(path);
}

inline void zookbag::zookeeper_session_expired(const char* path)
{
	this->on_zookeeper_session_expired(path);
}

inline void zookbag::zookeeper_session_event(int state, const char *path)
{
	this->on_zookeeper_session_event(state, path);
}

inline void zookbag::zookeeper_event(int type, int state, const char *path)
{
	this->on_zookeeper_event(type, state, path);
}

