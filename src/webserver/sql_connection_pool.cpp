#include "sql_connection_pool.h"
#include "log.h"

connection_pool::connection_pool(){
    m_CurConn = 0;
    m_FreeConn = 0;
}

connection_pool* connection_pool::Getinstance(){
    static connection_pool connPool;
    return &connPool;
}

//初始化连接池的核心配置，并创建指定数量的 MySQL 连接放入连接池；
void connection_pool::init(std::string url, std::string User, std::string Password, 
                           std::string DataBaseName, int Port, int MaxConn, int close_log)
{
    //初始化数据库信息
    m_url = url;
    m_User = User;
    m_PassWord = Password;
    m_DatabaseName = DataBaseName;
    m_Port = Port;
    m_close_log = close_log;

    //创建MaxConn条数据库连接
    for(int i = 0; i < MaxConn; ++i){
        MYSQL* conn = nullptr;
        conn = mysql_init(conn);

        if(conn == nullptr){
            LOG_ERROR("MySQL Error");
            exit(1);
        }
        conn = mysql_real_connect(conn, url.c_str(), User.c_str(), Password.c_str(), DataBaseName.c_str(), Port, nullptr, 0);

        if(conn == nullptr){
            LOG_ERROR("MySQL Error");
			exit(1);
        }
        connList.push_back(conn);
        ++m_FreeConn;
    }

    reserve = sem(m_FreeConn);

    m_MaxConn = m_FreeConn;
}

//当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL* connection_pool::Getconnection(){                 //获取数据库连接
    MYSQL* conn = nullptr;

    if(0 == connList.size()) return nullptr;

    reserve.wait();
    m_lock.lock();
    conn = connList.front();
    connList.pop_front();

    --m_FreeConn;
    ++m_CurConn;

    m_lock.unlock();
    return conn;
}

bool connection_pool::ReleaseConnection(MYSQL* conn){    //释放连接
    if(nullptr == conn) return false;
    m_lock.lock();

    connList.push_back(conn);
    ++m_FreeConn;
    --m_CurConn;

    m_lock.unlock();
    reserve.post();

    return true;
}

int connection_pool::GetFreeConn(){                      //获取连接
    return m_FreeConn;
}

void connection_pool::DestroyPool(){                     //销毁所有连接
    m_lock.lock();
    if(connList.size() > 0){
        std::list<MYSQL*>::iterator it;
        for (it = connList.begin(); it != connList.end(); ++it){
            MYSQL* con = *it;
            mysql_close(con);
        }
        m_CurConn = 0;
        m_FreeConn = 0;
        connList.clear();
    }
    m_lock.unlock();
}

connection_pool::~connection_pool(){
    DestroyPool();
}