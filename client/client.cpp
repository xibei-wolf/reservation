#include "client.h"
#include <limits>
#include <jsoncpp/json/json.h>
#include <unordered_map>
#include <regex>
#include <ctime>
#include <sstream>
#include <chrono>
#include <iomanip>

bool sock_client::Connect_server()
{
    this->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        cout << "create socket failed" << endl;
        return false;
    }

    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = inet_addr(ips.c_str());

    int res = ::connect(sockfd, reinterpret_cast<struct sockaddr *>(&saddr), sizeof(saddr));
    if (res == -1)
    {
        cout << "connect failed" << endl;
        return false;
    }
    cout << "connect to server success " << endl;
    return true;
}

void sock_client::PrintInfo()
{
    if (logged_flage)
    {
        cout << "----------------已登录-----------------" << username << "---" << endl;
        cout << "1:查看我的预约" << '\t' << "2:预约 " << '\t' << "3 : 查看面试间信息" << '\t' << " 4 : 取消预约" << '\t' << " 5退出 - " << endl;
        cout << "----------------------------------------" << endl;
        cout << "请输入选项编号" << endl;
        cin >> user_op;
        user_op += OFFSET;
    }
    else
    {
        cout << "------未登录----游客-----" << endl;
        cout << "-----1:登录" << '\t' << " 2:注册" << '\t' << " 3:退出-----" << endl;
        cout << "----------------------------------------" << endl;
        cout << "请输入选编号" << endl;
        cin >> user_op;
        if (user_op == 3)
        {
            user_op = EXIT;
        }
    }
    if (!cin)
    {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "非法输入，请输入数字！\n";
        user_op = -1;
    }
}

void sock_client::show_room_info(string &brose_date)
{
    Json::Value val;
    val["type"] = BROSE_RESERVATION;

    cout << "是否要选择日期？不选择默认今日面试间信息：(y/n)" << endl;
    cin >> brose_date;

    if (brose_date.compare("y") == 0)
    {
        cout << "请输入yyyy-mm-dd格式的日期" << endl;
        cin >> brose_date;
        static const std::regex datePattern(R"(^[0-9]{4}-[0-9]{2}-[0-9]{2}$)");

        while (!std::regex_match(brose_date, datePattern))
        {
            cout << "日期格式错误,请使用yyyy-mm-dd,若无需日期，则按'n'退出选择日期";
            cin >> brose_date;
            if (brose_date.compare("n") == 0)
                break;
        }
        val["brose_date"] = brose_date;
    }
    else
    {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm *local = std::localtime(&t);

        std::ostringstream oss;
        oss << std::put_time(local, "%Y-%m-%d"); // %Y=4位年 %m月(01~12) %d日
        brose_date = oss.str();
    }

    send(sockfd, val.toStyledString().c_str(), strlen(val.toStyledString().c_str()), 0);

    char buff[1024 * 1024] = {0};
    recv(sockfd, buff, 1024 * 1024, 0);

    m_val.clear();
    Json::Reader read;
    if (!read.parse(buff, m_val))
    {
        cout << "解析JSON失败" << endl;
        return;
    }

    if (m_val["status"].asString().compare("OK") != 0)
    {
        cout << "recv:" << buff << endl;
        cout << "获取面试间信息失败！" << endl;
        return;
    }

    const Json::Value &slots = m_val["data"];

    if (!slots.isArray())
    {
        std::cout << "data字段不是数组" << endl;
        return;
    }
    if (slots.empty())
    {
        cout << "暂无面试间信息" << endl;
        return;
    }

    unordered_map<int, vector<vector<string>>> map;
    for (auto &slot : slots)
    {
        if (slot["roomstatus"].asString().compare("已预约") == 0)
            continue;

        int roomId = slot["roomId"].asInt();

        if (map.find(roomId) == map.end())
        {
            map[roomId].push_back({slot["startTime"].asString(), slot["endTime"].asString()});
        }
        else
        {
            vector<string> &backslot = map[roomId].back();
            if (backslot.back() == slot["startTime"].asString())
            {
                backslot.back() = slot["endTime"].asString();
            }
            else
            {
                map[roomId].push_back({slot["startTime"].asString(), slot["endTime"].asString()});
            }
        }
    }

    for (auto &m : map)
    {
        cout << "=======roomid :" << m.first << "=============" << endl;
        cout << " 空闲时段：";
        int i = 1;
        for (auto &vec : m.second)
        {
            cout << vec[0] << "~" << vec[1] << endl;
        }
        cout << endl;
    }
    cout << endl;
}

void sock_client::User_Show_room_Info()
{
    string brose_date;
    show_room_info(brose_date);
}

void sock_client::User_Reserve()
{
    string book_date;
    show_room_info(book_date);

    cout << "请选择" << book_date << "日，想要预定的面试间" << endl;
    int index;
    cin >> index;
    // index 有效性检查

    string str_start, str_end;
    cout << "请选择想要预定的开始时段" << endl;
    cin >> str_start;
    cout << "请选择想要预定的结束时段" << endl;
    cin >> str_end;

    Json::Value val;
    val["type"] = BOOK;
    val["index"] = index;
    val["user_id"] = user_id;
    val["start_slot"] = slot_map[str_start];
    val["end_slot"] = slot_map[str_end];
    val["book_date"] = book_date;

    send(sockfd, val.toStyledString().c_str(), strlen(val.toStyledString().c_str()), 0);

    char buff[256] = {0};
    int n = recv(sockfd, buff, 255, 0);
    if (n <= 0)
    {
        cout << "ser close" << endl;
        return;
    }
    val.clear();
    Json::Reader read;
    if (!read.parse(buff, val))
    {
        cout << "json 解析失败" << endl;
        return;
    }

    string st = val["status"].asString();
    if (st.compare("OK") != 0)
    {
        cout << "预定失败！" << endl;
        return;
    }
    cout << "预定成功" << endl;
    return;
}

void sock_client::Run()
{

    while (running)
    {
        PrintInfo();
        switch (user_op)
        {
        case LOGIN:
            User_Login();
            break;
        case REGISTER:
            User_Regiser();
            break;
        case BROWSE_MINE:
            User_Show_myreservation();
            break;
        case BOOK:
            User_Reserve();
            break;
        case BROSE_RESERVATION:
            User_Show_room_Info();
            break;
        case CANCEL:
            User_Cancel();
            break;
        case EXIT:
            running = false;
        default:
            cout << "输入无效" << endl;
            break;
        }
    }
}

void sock_client::User_Regiser()
{
    cout << "------注册用户--------" << endl;
    cout << "请输入手机号码" << endl;
    cin >> usertel;
    cout << "请输入用户名" << endl;
    cin >> username;
    string passwd, tmp;
    cout << "请输入密码" << endl;
    cin >> passwd;
    cout << "请再次输入密码" << endl;
    cin >> tmp;

    if (passwd.empty() || username.empty())
    {
        cout << "手机或用户名不能为空" << endl;
        return;
    }
    if (passwd.compare(tmp) != 0)
    {
        cout << "两次密码不一致" << endl;
        return;
    }
    Json::Value val;
    val["type"] = REGISTER;
    val["usertel"] = usertel;
    val["username"] = username;
    val["userpasswd"] = passwd;
    send(sockfd, val.toStyledString().c_str(), strlen(val.toStyledString().c_str()), 0);
    char buff[256] = {0};
    if (recv(sockfd, buff, 255, 0) <= 0)
    {
        cout << "ser close" << endl;
        return;
    }
    val.clear();
    Json::Reader Read;
    if (!Read.parse(buff, val))
    {
        cout << "json 解析失败" << endl;
    }
    string s = val["status"].asString();
    if (s.compare("OK") != 0)
    {
        cout << "注册失败！" << endl;
        return;
    }
    cout << "注册成功" << endl;
    return;
}

void sock_client::User_Login()
{
    string tel, passwd;
    cout << "请输入手机号码" << endl;
    cin >> tel;
    cout << "请输入密码" << endl;
    cin >> passwd;
    Json::Value val;
    val["type"] = LOGIN;
    val["usertel"] = tel;
    val["userpasswd"] = passwd;

    send(sockfd, val.toStyledString().c_str(), strlen(val.toStyledString().c_str()), 0);
    char buff[126] = {0};
    int n = recv(sockfd, buff, 125, 0);
    if (n <= 0)
    {
        cout << "ser close" << endl;
        return;
    }

    Json::Reader read;
    if (!read.parse(buff, val))
    {
        cout << "解析json失败" << endl;
        return;
    }

    string st = val["status"].asString();
    cout << "recv json" << val.toStyledString() << endl;
    if (st.compare("OK") != 0)
    {
        cout << "登录失败！" << endl;
        return;
    }

    logged_flage = true;
    username = val["username"].asString();
    userphone = tel;
    user_id = val["user_id"].asInt();

    cout << "登录成功" << "账号名为：" << username << endl;
    return;
}

void sock_client::User_Show_myreservation()
{
    Json::Value val;
    val["type"] = BROWSE_MINE;
    val["userid"] = user_id;

    send(sockfd, val.toStyledString().c_str(), strlen(val.toStyledString().c_str()), 0);
    char buff[1024] = {0};
    recv(sockfd, buff, 1024, 0);
    Json::Reader read;
    if (!read.parse(buff, val))
    {
        cout << "json 解析失败！" << endl;
        return;
    }
    if (val["status"].asString().compare("OK") != 0)
    {
        cout << "获取已预定信息失败" << endl;
        return;
    }
    const Json::Value &reservations = val["data"];
    if (!reservations.isArray())
    {
        std::cout << "data字段不是数组" << endl;
        return;
    }
    if (reservations.empty())
    {
        cout << "暂无预定信息" << endl;
        return;
    }
    cout << "==========我的预约============" << endl;
    for (auto &res : reservations)
    {
        int reserve_id = res["reserve_id"].asInt();
        int start_slot = res["start_slot"].asInt();
        int end_slot = res["end_slot"].asInt();
        int totalstart = start_slot * 1800 + 8 * 3600;
        int totalend = end_slot * 1800 + 8 * 3600;
        string start_min = totalstart % 3600 ? "30" : "00";
        string end_min = totalend % 3600 ? "30" : "00";
        cout << "预约id:" << reserve_id << " " << res["reserve_date"].asString() << "\t" << to_string(totalstart / 3600) << ":" << start_min << "~" << to_string(totalend / 3600) << " :" << end_min << endl;
    }
}

void sock_client::User_Cancel()
{
    User_Show_myreservation();
    int reserve_id;
    cout << "======请输入需要取消的预约id========" << endl;
    cin >> reserve_id;
    Json::Value val;
    val["type"] = CANCEL;
    val["reserve_id"] = reserve_id;
    send(sockfd, val.toStyledString().c_str(), strlen(val.toStyledString().c_str()), 0);
    char buff[128] = {0};
    Json::Reader read;
    recv(sockfd, buff, 127, 0);
    if (!read.parse(buff, val))
    {
        cout << "json 解析失败" << endl;
        return;
    }
    if (val["status"].compare("OK") != 0)
    {
        cout << "取消失败" << endl;
        return;
    }
    cout << "取消成功" << endl;
}

int main()
{
    sock_client cli;
    if (!cli.Connect_server())
    {
        exit(1);
    }
    cli.Run();
    exit(0);
}
