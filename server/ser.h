#include <iostream>
#include <string.h>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <event.h>
#include <jsoncpp/json/json.h>
#include <mysql/mysql.h>
#include <regex>

using namespace std;

enum OP_TYPE
{
    LOGIN = 1,
    REGISTER,
    BROWSE_MINE,
    BOOK,
    BROSE_RESERVATION,
    CANCEL,
    EXIT
};

struct RoomSoltInfo
{
    int roomId;
    std::string roomName;
    std::string location;
    int cap;
    std::string startTime;
    std::string endTime;
    std::string status;
};

struct reservationInfo
{
    std::string roomName;
    std::string reserve_date;
    std::string start_slot;
    std::string end_slot;
    std::string reservation_id;
};

class mysql_client
{
private:
    MYSQL mysql_con;
    string db_name;
    string db_username;
    string db_ips;
    string db_passwd;

private:
    bool mysql_user_begin();
    bool mysql_user_commit();
    bool mysql_user_rollback();

public:
    mysql_client()
    {
        db_ips = "0.0.0.0";
        db_name = "yuuye_DB";
        db_username = "root";
        db_passwd = "111111";
    }
    bool mysql_ConnectServer();
    bool mysql_setQuery_date(const string &queryDate = "CURDATE()");
    bool mysql_Register(const string &tel, const string &passwd, const string &name);
    bool mysql_Login(const string &tel, const string &passwd, string &name, string &userid);
    bool mysql_Brose_Iteroom(Json::Value &val, const string &brose_date);
    std::vector<RoomSoltInfo> queryTodatRoomSlots(const string &brose_date = "CURDATE()");
    bool mysql_book(int userid, int roomid, int start_slot, int end_slot, const string book_date = "CURDATE()");
    std::vector<reservationInfo> mysql_booked_info(int userid);
    bool mysql_mybooled(int userid, Json::Value &val);
    bool mysql_cancel(int reservation_id);

    ~mysql_client()
    {
        mysql_close(&mysql_con);
    }
};

class socket_listen
{
private:
    int sockfd;
    short m_port;
    string m_ips;
    struct event_base *base;

public:
    socket_listen()
    {
        sockfd = -1;
        m_port = 6000;
        m_ips = "0.0.0.0";
    }
    socket_listen(string ips, short port) : m_ips(ips), m_port(port) { sockfd = -1; }
    bool socket_init();
    int Get_sockfd() const { return sockfd; }
    int accept_client();
    void Set_base(struct event_base *base) { this->base = base; }
    struct event_base *Get_base() const { return base; }
    ~socket_listen() {}
};

class socket_con
{
private:
    int c;
    struct event *c_ev;

    Json::Value val;

public:
    socket_con(int fd) : c(fd)
    {
        c_ev = NULL;
    }
    void Set_ev(struct event *ev)
    {
        c_ev = ev;
    }
    void Recv_data();

    void Send_err();
    void Send_ok();

    void User_login();
    void User_register();
    void User_Show_room_Info();
    void User_book();
    void User_Show_myreservation();
    void User_Cancel();
    ~socket_con()
    {
        event_free(c_ev);
        close(c);
    }
};
