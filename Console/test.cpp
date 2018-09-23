#include <iostream>
#include <vector>
#include <set>
#include <ctime>
#include <sstream>
#include <windows.h>
#include <random>
#include <chrono>

using namespace std;

vector<string> split(const string &str, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenstream(str);
    while (getline(tokenstream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

int main() {
    set<int> s;
    s.insert(1);
    s.insert(2);
    cout << (s.find(1) == s.end()) << endl;
}