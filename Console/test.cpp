#include <iostream>
#include <set>

using namespace std;

struct Point {
    int x, y;

    bool operator<(const Point &pt) const {
        cout << &pt << endl;
        cout << x << y << endl;
        cout << pt.x << pt.y << endl;
        return (x < pt.x) && (y < pt.y);
    }
};

int main() {
    set<struct Point> z;
    struct Point a, b, c;
    struct Point *w = &a;
    a.x = 1;
    a.y = 2;
    b.x = 1;
    b.y = 1;
    c.x = 1;
    c.y = 1;
    z.insert(a);
    z.insert(b);
    z.insert(c);
    cout << (*(z.begin())).x << endl;
    cout << (*(w)).x << endl;
}
