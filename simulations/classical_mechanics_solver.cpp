#include <iostream>
#include <cmath>


int main() {

    std::cout << "Classical mechanics solver for simple mechanics." << std::endl;
    // SUVAT equations
    // s = initial displacement
    // u = initial velocity
    // v = final velocity
    // a = accleration
    // t = time elapsed

    // float s = 0.0;
    float u = 0.0;
    float v_down = 0.0;
    float a_down = 9.81;
    float t = 3.0;

    v_down = u + a_down*t;
    std::cout << "Final velocity reached: " << v_down << "m/s" << std::endl;
    return 0;
}