#include "ser.h"

const int LIS_MAX = 10;

bool mysql_client::mysql_ConnectServer()
{
    MYSQL *mysql = mysql_init(&mysql_con);
    if (mysql == nullptr)
    {
        return false;
    }
    mysql = mysql_real_connect(mysql, db_ips.c_str(), db_username.c_str(), db_passwd.c_str(), db_name.c_str(), 3306, NULL, 0);
    if (mysql == nullptr)
    {
        cout << "connect with mysql erro" << endl;
        return false;
    }
    return true;
}

bool mysql_client::mysql_setQuery_date(const string &queryDate)
{
    string datesql;

    if (queryDate == "CURDATE()" || queryDate.empty())
    {
        datesql = "SET @query_date = CURDATE()";
    }
    else
    {
        static const std::regex datePattern(
            R"(^[0-9]{4}-[0-9]{2}-[0-9]{2}$)");

        if (!std::regex_match(queryDate, datePattern))
        {
            cout << "日期格式错误,请使用yyyy-mm-dd";
            return false;
        }
        datesql = "SET @query_date = '" + queryDate + "'";
    }

    if (mysql_query(&mysql_con, datesql.c_str()) != 0)
    {
        cout << "sql set date query is erro" << endl;
        return false;
    }
    return true;
}

bool mysql_client::mysql_Register(const string &tel, const string &passwd, const string &name)
{
    string sql =
        string("insert into user_info(tel, username, passwd, status) values('") +
        tel + string("','") + name + string("','") + passwd + string("',1)");
    cout << "sql :" << sql << endl;
    if (mysql_query(&mysql_con, sql.c_str()) != 0)
    {
        cout << "quert erro" << endl;
        return false;
    }
    return true;
}

bool mysql_client::mysql_Login(const string &tel, const string &password, string &name, string &userid)
{
    string sql1 = string("select username,passwd,userid from user_info where tel=") + tel;
    if (mysql_query(&mysql_con, sql1.c_str()) != 0)
    {
        return false;
    }
    MYSQL_RES *r = mysql_store_result(&mysql_con);
    if (r == NULL)
    {
        return false;
    }
    int num = mysql_num_rows(r);
    if (num == 0)
    {
        mysql_free_result(r);
        return false;
    }
    MYSQL_ROW row = mysql_fetch_row(r);

    string passwd = row[1];
    if (password.compare(passwd) != 0)
    {
        cout << "密码错误" << endl;
        cout << "原始密码：" << password << "输入密码" << passwd << endl;
        mysql_free_result(r);
        return false;
    }
    name = row[0];
    userid = row[2];
    cout << "mysql read row[0] =" << name << "mysql read passwd" << passwd << endl;

    mysql_free_result(r);
    return true;
}

bool mysql_client::mysql_Brose_Iteroom(Json::Value &val, const string &brose_date)
{
    val = Json::Value(Json::arrayValue);
    std::vector<RoomSoltInfo> slots;
    if (brose_date.empty())
        slots = queryTodatRoomSlots();
    else
        slots = queryTodatRoomSlots(brose_date);

    Json::Value tmp;
    for (auto &slot : slots)
    {
        tmp["roomId"] = slot.roomId;
        tmp["roomName"] = slot.roomName;
        tmp["startTime"] = slot.startTime;
        tmp["endTime"] = slot.endTime;
        tmp["roomstatus"] = slot.status;
        val.append(tmp);
    }
    return true;
}

std::vector<RoomSoltInfo> mysql_client::queryTodatRoomSlots(const string &brose_date)
{
    if (!mysql_setQuery_date(brose_date))
    {
        return {};
    }

    const char *sql = R"SQL(

    WITH RECURSIVE slots AS (
            SELECT 0 AS slot_no

            UNION ALL

            SELECT slot_no + 1
            FROM slots
            WHERE slot_no < 29
        )
        SELECT
            r.room_id,
            r.room_name,

            TIME_FORMAT(
                SEC_TO_TIME(8 * 3600 + s.slot_no * 1800),
                '%H:%i'
            ) AS start_time,

            TIME_FORMAT(
                SEC_TO_TIME(8 * 3600 + (s.slot_no + 1) * 1800),
                '%H:%i'
            ) AS end_time,

            CASE
                WHEN r.status <> 1 THEN '已停用'
                WHEN rs.reservation_id IS NOT NULL THEN '已预约'
                ELSE '可预约'
            END AS slot_status

        FROM interview_room AS r

        CROSS JOIN slots AS s

        LEFT JOIN room_slot AS rs
            ON rs.room_id = r.room_id
            AND rs.reserve_date = @query_date
            AND rs.slot_no = s.slot_no

        ORDER BY r.room_id, s.slot_no
    )SQL";

    if (mysql_query(&mysql_con, sql) != 0)
    {
        throw std::runtime_error(mysql_error(&mysql_con));
        return {};
    }

    std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(mysql_store_result(&mysql_con), mysql_free_result);

    if (!result)
    {
        throw std::runtime_error(mysql_error(&mysql_con));
        return {};
    }
    if (mysql_num_rows(result.get()) == 0)
    {
        return {};
    }

    std::vector<RoomSoltInfo> list;
    MYSQL_ROW r;

    while ((r = mysql_fetch_row(result.get())) != nullptr)
    {
        RoomSoltInfo info;
        info.roomId = std::stoi(r[0]);
        info.roomName = r[1] ? r[1] : "";
        info.startTime = r[2] ? r[2] : "";
        info.endTime = r[3] ? r[3] : "";
        info.status = r[4] ? r[4] : "";

        list.push_back(info);
    }

    return list;
}

std::vector<reservationInfo> mysql_client::mysql_booked_info(int userid)
{
    string sql = string("select room_name,reserve_date,start_slot,end_slot,reservation_id from reservation_info r left join interview_room room on r.room_id = room.room_id   where userid =") + to_string(userid);
    if (mysql_query(&mysql_con, sql.c_str()) != 0)
    {
        cout << sql << endl;
        cout << "查询用户id为" << userid << "err" << endl;
        return {};
    }
    std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> result(mysql_store_result(&mysql_con), mysql_free_result);
    if (!result)
    {
        cout << "获取result failed" << endl;
        return {};
    }
    if (mysql_num_rows(result.get()) == 0)
    {
        return {};
    }

    std::vector<reservationInfo> list;
    MYSQL_ROW r;

    while ((r = mysql_fetch_row(result.get())) != nullptr)
    {
        reservationInfo info;
        info.roomName = r[0] ? r[0] : "";
        info.reserve_date = r[1] ? r[1] : "";
        info.start_slot = r[2] ? r[2] : "";
        info.end_slot = r[3] ? r[3] : "";
        info.reservation_id = r[4] ? r[4] : "";

        list.push_back(info);
    }
    return list;
}

bool mysql_client::mysql_mybooled(int userid, Json::Value &val)
{
    val = Json::Value(Json::arrayValue);
    std::vector<reservationInfo> infos = mysql_booked_info(userid);
    Json::Value tmp;
    for (auto &info : infos)
    {
        tmp["room_name"] = info.roomName;
        tmp["reserve_date"] = info.reserve_date;
        tmp["start_slot"] = atoi(info.start_slot.c_str());
        tmp["end_slot"] = atoi(info.end_slot.c_str());
        tmp["reserve_id"] = atoi(info.reservation_id.c_str());
        val.append(tmp);
    }
    return true;
}

bool mysql_client::mysql_book(int userid, int roomid, int start_slot, int end_slot, const string book_date)
{
    mysql_user_begin();
    // 查询

    if (!mysql_setQuery_date(book_date))
    {
        mysql_user_rollback();
        return false;
    }

    string sql1 =
        string("select * from room_slot where reserve_date= @query_date and slot_no >= ") + to_string(start_slot) +
        string(" and slot_no < ") + to_string(end_slot);

    int total_start_sec = 8 * 3600 + start_slot * 1800;
    int total_end_sec = 8 * 3600 + end_slot * 1800;

    int start_h = total_start_sec / 3600;
    int start_m = (total_start_sec % 3600) / 60;
    int end_h = total_end_sec / 3600;
    int end_m = (total_end_sec % 3600) / 60;

    if (mysql_query(&mysql_con, sql1.c_str()) != 0)
    {
        cout << "查询用户想预定日期为：" << book_date << " 开始时间为： " << start_h << ":" << start_m << " ~ " << end_h << ":" << end_m << " 的room " << roomid << "失败" << endl;
        mysql_user_rollback();
        return false;
    }
    MYSQL_RES *r = mysql_store_result(&mysql_con);
    if (r == nullptr)
    {
        cout << "获取结果集失败" << endl;
        mysql_user_rollback();
        return false;
    }
    // 读取结果行数
    int Num = mysql_num_rows(r);
    if (Num != 0)
    {
        cout << "用户预定日期为：" << book_date << " 开始时间为： " << start_h << ":" << start_m << " ~ " << end_h << ":" << end_m << " 的room " << roomid << "已被预定" << endl;
        mysql_user_rollback();
        return false;
    }

    // mysql> insert into reservation_info(reserve_date,room_id,userid,start_slot,end_slot) values(@query_date,2,3,5,7);
    string sql2 = string("insert into reservation_info(reserve_date,room_id,userid,start_slot,end_slot) values(@query_date,") + to_string(roomid) + "," + to_string(userid) + "," + to_string(start_slot) + " , " + to_string(end_slot) + "); ";
    string set_id = string("SET @reserid = LAST_INSERT_ID();");

    if (mysql_query(&mysql_con, sql2.c_str()) != 0)
    {
        cout << "insert into reservation_info err" << endl;
        cout << sql2 << endl;
        mysql_user_rollback();
        return false;
    }

    if (mysql_query(&mysql_con, set_id.c_str()) != 0)
    {
        cout << "set id  err" << endl;
        cout << set_id << endl;
        mysql_user_rollback();
        return false;
    }

    for (int i = start_slot; i < end_slot; i++)
    {
        string tmp_sql = string("insert into room_slot(reserve_date,reservation_id,room_id,slot_no) values(@query_date,@reserid,") + to_string(roomid) + "," + to_string(i) + ")";
        if (mysql_query(&mysql_con, tmp_sql
                                        .c_str()) != 0)
        {
            cout << "insert into room_slot err" << endl;
            cout << tmp_sql << endl;
            mysql_user_rollback();
            return false;
        }
    }
    mysql_user_commit();
    return true;
}

bool mysql_client::mysql_cancel(int reservation_id)
{
    mysql_user_begin();
    string del_slot_sql = string("delete from room_slot where reservation_id = ") + to_string(reservation_id);
    if (mysql_query(&mysql_con, del_slot_sql.c_str()) != 0)
    {
        cout << "insert into reservation_info err" << endl;
        cout << del_slot_sql << endl;
        mysql_user_rollback();
        return false;
    }

    string del_reserve_sql = string("delete from reservation_info where reservation_id = ") + to_string(reservation_id);
    if (mysql_query(&mysql_con, del_reserve_sql.c_str()) != 0)
    {
        cout << "insert into reservation_info err" << endl;
        cout << del_reserve_sql << endl;
        mysql_user_rollback();
        return false;
    }

    mysql_user_commit();
    return true;
}

bool mysql_client::mysql_user_begin()
{
    if (mysql_query(&mysql_con, "begin") != 0)
    {
        return false;
    }
    return true;
}
bool mysql_client::mysql_user_commit()
{
    if (mysql_query(&mysql_con, "commit") != 0)
    {
        return false;
    }
    return true;
}
bool mysql_client::mysql_user_rollback()
{
    if (mysql_query(&mysql_con, "rollback") != 0)
    {
        return false;
    }
    return true;
}

int socket_listen::accept_client()
{
    int c = accept(sockfd, NULL, NULL);
    return c;
}

void socket_con::Send_err()
{
    Json::Value res;
    res["status"] = "ERR";
    send(c, res.toStyledString().c_str(), strlen(res.toStyledString().c_str()), 0);
}
void socket_con::Send_ok()
{
    Json::Value res;
    res["status"] = "OK";
    send(c, res.toStyledString().c_str(), strlen(res.toStyledString().c_str()), 0);
}

void socket_con::User_login()
{
    string tel, username, passwd, userid;
    tel = val["usertel"].asString();
    passwd = val["userpasswd"].asString();
    if (tel.empty() || passwd.empty())
    {
        cout << "has empty" << endl;
        Send_err();
        return;
    }

    mysql_client cli;
    if (!cli.mysql_ConnectServer())
    {
        cout << "mysql connect failed" << endl;
        Send_err();
        return;
    }

    if (!cli.mysql_Login(tel, passwd, username, userid))
    {
        cout << "msyql_Login return false" << endl;
        Send_err();
        return;
    }

    Json::Value value;
    value["status"] = "OK";
    value["username"] = username;
    value["user_id"] = atoi(userid.c_str());
    send(c, value.toStyledString().c_str(), strlen(value.toStyledString().c_str()), 0);

    return;
}

void socket_con::User_register()
{
    string tel, username, passwd;
    tel = val["usertel"].asString();
    username = val["username"].asString();
    passwd = val["userpasswd"].asString();
    if (tel.empty() || username.empty() || passwd.empty())
    {
        Send_err();
    }
    mysql_client cli;
    if (!cli.mysql_ConnectServer())
    {
        Send_err();
        return;
    }
    if (!cli.mysql_Register(tel, passwd, username))
    {
        Send_err();
        return;
    }
    Send_ok();
    return;
}

void socket_con::User_Show_room_Info()
{
    string brose_date = val["brose_date"].asString();

    Json::Value resval;
    mysql_client cli;
    if (!cli.mysql_ConnectServer())
    {
        cout << "数据库连接错误" << endl;
        Send_err();
        return;
    }
    if (!cli.mysql_Brose_Iteroom(resval["data"], brose_date))
    {
        cout << "获取面试间信息错误" << endl;
        Send_err();
        return;
    }
    resval["status"] = "OK";
    send(c, resval.toStyledString().c_str(), strlen(resval.toStyledString().c_str()), 0);
    Json::Reader read;

    return;
}

void socket_con::User_book()
{
    int room_id = val["index"].asInt();
    int user_id = val["user_id"].asInt();
    int start_slot = val["start_slot"].asInt();
    int end_slot = val["end_slot"].asInt();
    string book_date = val["book_date"].asString();

    mysql_client cli;
    if (!cli.mysql_ConnectServer())
    {
        cout << "connect mysql err" << endl;
        return;
    }
    if (!cli.mysql_book(user_id, room_id, start_slot, end_slot, book_date))
    {
        Send_err();
        return;
    }
    Send_ok();
    return;
}

void socket_con::User_Show_myreservation()
{
    int userid = val["userid"].asInt();

    mysql_client cli;
    Json::Value resval;
    if (!cli.mysql_ConnectServer())
    {
        cout << "connect mysql err" << endl;
        return;
    }

    if (!cli.mysql_mybooled(userid, resval["data"]))
    {
        Send_err();
        return;
    }
    resval["status"] = "OK";

    send(c, resval.toStyledString().c_str(), strlen(resval.toStyledString().c_str()), 0);
    return;
}

void socket_con::User_Cancel()
{
    int reserve_id = val["reserve_id"].asInt();
    mysql_client cli;

    if (!cli.mysql_ConnectServer())
    {
        cout << "connect mysql err" << endl;
        return;
    }

    if (!cli.mysql_cancel(reserve_id))
    {
        cout << "cancel erro" << endl;
        Send_err();
        return;
    }
    Send_ok();
    return;
}

void socket_con::Recv_data()
{
    char buff[256] = {0};
    int n = recv(c, buff, 255, 0);
    if (n <= 0)
    {
        cout << "client close connect " << endl;
        delete this;
        return;
    }
    // 测试
    cout << "recv:" << buff << endl;

    Json::Reader Read;
    if (!Read.parse(buff, val))
    {
        cout << "Recv_data:解析json失败" << endl;
        Send_err();
        return;
    }
    int ops = val["type"].asInt();
    switch (ops)
    {
    case LOGIN:
        User_login();
        break;
    case REGISTER:
        User_register();
        break;
    case BROWSE_MINE:
        User_Show_myreservation();
        break;
    case BOOK:
        User_book();
        break;
    case BROSE_RESERVATION:
        User_Show_room_Info();
        break;
    case CANCEL:
        User_Cancel();
        break;
    default:
        break;
    }
}

bool socket_listen::socket_init()
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd)
    {
        return false;
    }
    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(m_port);
    saddr.sin_addr.s_addr = inet_addr(m_ips.c_str());
    int res = bind(sockfd, (struct sockaddr *)&saddr, sizeof(saddr));
    if (-1 == res)
    {
        cout << "bind err" << endl;
        close(sockfd);
        return false;
    }

    res = listen(sockfd, LIS_MAX);

    if (-1 == res)
    {
        return false;
    }

    return true;
}

void SOCK_CON_CALLBACK(int fd, short ev, void *arg)
{
    socket_con *q = (socket_con *)arg;
    if (EV_READ & ev)
    {
        q->Recv_data();
    }
}

void SOCK_LIS_CALLBACK(int sockfd, short ev, void *arg)
{
    socket_listen *p = static_cast<socket_listen *>(arg);
    if (p == nullptr)
    {
        return;
    }
    // 处理事件
    if (ev & EV_READ)
    {
        int c = p->accept_client();
        if (c == -1)
        {
            return;
        }

        cout << "accept: c = " << c << endl;

        socket_con *q = new socket_con(c);

        struct event *c_ev = event_new(p->Get_base(), c, EV_READ | EV_PERSIST, SOCK_CON_CALLBACK, q);
        if (c_ev == NULL)
        {
            close(c);
            delete q;
            return;
        }
        q->Set_ev(c_ev);

        // 添加到libevent
        event_add(c_ev, NULL);
    }
}

int main()
{
    // 创建套接字
    socket_listen sock_ser;
    if (!sock_ser.socket_init())
    {
        cout << "sokcet init err" << endl;
    }

    // 创建livevent base
    struct event_base *base = event_init();
    if (base == nullptr)
    {
        cout << "base null" << endl;
        exit(1);
    }

    // 设置base
    sock_ser.Set_base(base);
    struct event *sock_ev = event_new(base, sock_ser.Get_sockfd(), EV_READ | EV_PERSIST, SOCK_LIS_CALLBACK, &sock_ser);
    event_add(sock_ev, nullptr);

    // 启动事件循环
    event_base_dispatch(base);

    // 释放资源
    event_free(sock_ev);
    event_base_free(base);

    return 0;
}
