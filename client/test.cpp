#include <iostream>
#include <jsoncpp/json/json.h>
#include <string>
#include <cstring>

using namespace  std;


int main(){
    Json::Value val;
    val["name"] = "小王";
    val["age"] = 23;
    char buff[236] = {0};
    strcpy(buff,val.toStyledString().c_str());
    cout<<"buff : "<<buff<<endl;
    
    Json::Value res;
    Json::Reader Read;
    Read.parse(buff,res);

    string name = res["name"].asString();
    int age = res["age"].asInt();

    cout<<"name :"<<name<<"age"<<age<<endl;
    return 0;
}