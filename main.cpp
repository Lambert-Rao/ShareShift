//
// Created by lambert on 23-3-26.
//

#include "Logging.h"
class A
{
public:
    void operator ()(int a, int b)
    {
        cout << a << b << endl;
    }
};
int main()
{
    A a;
    a(1,2);
    return 0;

}