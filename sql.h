#include <mysql/mysql.h>
#include <iostream>

using namespace std;

static const string SERVER = "localhost";
static const string USER = "root";
static const string PASSWORD = "netpro2021";
static const string DATABASE = "netpro";

class SQL{
public:
    SQL(){
        conn = mysql_init(NULL);
        if (!mysql_real_connect(conn, SERVER.c_str(), USER.c_str(), PASSWORD.c_str(), DATABASE.c_str(), 0, NULL, 0)) {
            fprintf(stderr, "%s\n", mysql_error(conn));
            exit(1);
        }
    }

    ~SQL(){
        mysql_close(conn);
    }

    bool checkUser(const string& name, const string& password) const {
        string checkSQL = "select id, name, password from Users where name = '" + name + "' and password = '" + password + "';";
        MYSQL_ROW row;
        MYSQL_RES* res = execute(checkSQL);
        bool exist;
        row = mysql_fetch_row(res);
        if(!row)exist = false;
        else exist = true;
        mysql_free_result(res);
        return exist;
    }

    bool createUser(const string& name, const string& password) const {
        string checkSQL = "select id, name, password from Users where name = '" + name + "';";
        MYSQL_ROW row;
        MYSQL_RES* res = execute(checkSQL);
        bool exist;
        row = mysql_fetch_row(res);
        if(row){
            mysql_free_result(res);
            return false;
        }
        mysql_free_result(res);
        string insertSQL = "insert into Users(name, password) values('" + name + "', '" + password + "');";
        // cout << insertSQL << endl;
        res = execute(insertSQL);
        mysql_free_result(res);
        return true;
    }

private:
    MYSQL_RES* execute(const string& sql) const {
        if (mysql_query(conn, sql.c_str())){
            fprintf(stderr, "%s\n", mysql_error(conn));
            exit(1);
        }

        MYSQL_RES* res = mysql_use_result(conn);
        mysql_commit(conn);
        return res;
    }

    MYSQL* conn;

};
