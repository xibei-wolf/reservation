#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <unordered_map>

using namespace std;

const int OFFSET = 2;

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

class sock_client
{
private:
    int sockfd;
    string ips;
    short port;

    bool logged_flage;
    bool running;

    string username;
    string userphone;
    int user_id;
    int user_op;
    string usertel;

    Json::Value m_val;

    unordered_map<string, int> slot_map{
        {"8:00", 0},
        {"8:30", 1},
        {"9:00", 2},
        {"9:30", 3},
        {"10:00", 4},
        {"10:30", 5},
        {"11:00", 6},
        {"11:30", 7},
        {"12:00", 8},
        {"12:30", 9},
        {"13:00", 10},
        {"13:30", 11},
        {"14:00", 12},
        {"14:30", 13},
        {"15:00", 14},
        {"15:30", 15},
        {"16:00", 16},
        {"16:30", 17},
        {"17:00", 18},
        {"17:30", 19},
        {"18:00", 20},
        {"18:30", 21},
        {"19:00", 22},
        {"19:30", 23},
        {"20:00", 24},
        {"20:30", 25},
        {"21:00", 26},
        {"21:30", 27},
        {"22:00", 28},
        {"22:30", 29},
    };

private:
    void show_room_info(string &brose_date);

public:
    sock_client()
    {
        sockfd = -1;
        ips = "127.0.0.1";
        port = 6000;
        logged_flage = false;
        user_op = 0;
        running = true;
    }

    sock_client(int port, string ip) : port(port), ips(ip), logged_flage(false), user_op(0), running(true)
    {
        sockfd = -1;
    }

    bool Connect_server();
    void Run();
    void PrintInfo();
    void User_Regiser();
    void User_Login();
    void User_Show_room_Info();
    void User_Reserve();
    void User_Show_myreservation();
    void User_Cancel();

    ~sock_client()
    {
        close(sockfd);
    }
};
