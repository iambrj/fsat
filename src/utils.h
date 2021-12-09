#ifndef UTILS_H
#define UTILS_H

#include <ctime>
#include <string>
#include <unistd.h>

using namespace std;

const int NAMELEN = 4;

string gen_random(const int len);
int mod(int x);

#endif
