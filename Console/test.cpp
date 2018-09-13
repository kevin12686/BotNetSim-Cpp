#include <iostream>
#include <vector>
#include <set>
#include <sstream>

using namespace std;

typedef struct POINT {
    string ip;
    string port;
} Point;

Point *ptr = NULL;

vector<Point *> ps;

vector<string> split(const string &str, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenstream(str);
    while (getline(tokenstream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

void a() {
    Point *temp = new Point;
    temp->ip = "haha";
    temp->port = "ccc";
    ps.push_back(temp);
}


int main() {
    cout << ps.size() << endl;
    a();
    cout << ps.size() << endl;

    vector<Point *>::iterator it_i;
    for (it_i = ps.begin(); it_i != ps.end();ps.erase(it_i)) {
        cout << (*it_i)->ip << endl;
        cout << (*it_i)->port << endl;
        delete (*it_i);
    }
}