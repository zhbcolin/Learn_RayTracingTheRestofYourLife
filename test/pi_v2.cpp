#include "../src/rtweekend.h"

#include <iostream>
#include <iomanip>
#include <math.h>
#include <stdlib.h>

int main() {
    int inside_circle = 0;
    int runs = 0;
    std::cout << std::fixed << std::setprecision(12);
    while(true) {
        ++runs;
        auto x = random_double(-1, 1);
        auto y = random_double(-1, 1);
        if(x*x + y*y < 1)
            ++inside_circle;
        if(runs % 100000 == 0)
            std::cout << "Estimate of Pi = "
                      << 4*double(inside_circle) / runs
                      << '\n';
    }
}