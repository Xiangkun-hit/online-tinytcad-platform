#ifndef SQL_CONNECTION_POOL_H
#define SQL_CONNECTION_POOL_H

#include <iostream>
#include <string>
#include <list>
#include <mysql/mysql.h>

#include "locker.h"
#include "log.h"
#include "sem.h"

class connection_pool{
public:
    MYSQL* Getconnection();                 //获取数据库连接
    bool ReleaseConnection(MYSQL* conn);    //释放连接
    int GetFreeConn();                      //获取连接
    void DestroyPool();                     //销毁所有连接

    //局部静态变量单例模式
    static connection_pool* Getinstance();

    //初始化连接池的核心配置，并创建指定数量的 MySQL 连接放入连接池；
    void init(std::string url, std::string User, std::string Password, std::string DataBaseName, 
              int Port, int MaxConn, int close_log);

private:
    // 私有化构造/析构：单例禁止外部创建
    connection_pool();
    ~connection_pool();

    // 禁用拷贝和赋值（单例安全）
    connection_pool(const connection_pool&) = delete;
    connection_pool& operator=(const connection_pool&) = delete;

private:
    // 私有成员变量（连接池内部状态）
    int m_MaxConn;  // 连接池最大连接数（由init的MaxConn参数初始化）
	int m_CurConn;  // 当前已被使用的连接数（每次GetConnection+1，ReleaseConnection-1）
	int m_FreeConn; // 当前空闲的连接数（每次GetConnection-1，ReleaseConnection+1）
	locker m_lock;    // 互斥锁（保护connList的线程安全，避免多线程同时修改连接池容器）
	std::list<MYSQL *> connList; // 连接池容器（存储所有MySQL连接的指针，链表便于快速增删）
	sem reserve;    // 信号量（控制并发获取连接的数量，初始值为最大空闲连接数，保证不超量获取）

public:
    //公有成员变量（连接池配置参数）
    std::string m_url;               // 数据库主机地址（对应init的url参数）
    std::string m_Port;              // 数据库端口号（对应init的Port参数，注意是字符串类型）
    std::string m_User;              // 数据库登录用户名（对应init的User参数）
    std::string m_PassWord;          // 数据库登录密码（对应init的PassWord参数）
    std::string m_DatabaseName;      // 要使用的数据库名（对应init的DataBaseName参数）
    int m_close_log;            // 日志开关（对应init的close_log参数，控制是否输出日志）
};

class connectionRAII{
public:
    connectionRAII(MYSQL** con, connection_pool* connPool);
    ~connectionRAII();

private:
    MYSQL* conRAII;
    connection_pool* poolRAII;
};



#endif