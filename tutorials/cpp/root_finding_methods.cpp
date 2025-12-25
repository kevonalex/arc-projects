#include <iostream>
#include <vector>

float continuous_function(float x, float m = 2, float c = 5) {
    return m*(x-3)*(x-3)*(x-3) + c;
};

int main () {
    std::cout << "Root Finding Methods: Bisection Method\n" << std::endl;
    std::cout << "Use continuous function: y = 2(x-3)^3 + 5" << std::endl;

    std::vector<float> x_array;
    std::vector<float> y_array;

    float y;
    float x_t0 = 1.0;
    float x_T = 21.0;
    std::cout << "We are sure that the root is between " << x_t0 << " and " << x_T << ".\n" << std::endl;
    std::cout << "We need to find the root using the bisection method, so we will need to use a while loop to find the root." << std::endl;
    float x_mid = (x_T - x_t0)/2;
    std::cout << "Two new intervals are: " << x_t0 << "to" << x_mid << ", and" << x_mid << "to" << x_T << std::endl;
    while (x_t0 < x_T) {
        y = continuous_function(x_mid);
        if (y == 0) {
            std::cout << "Y is 0 at x = " << x_mid << ".\n" << std::endl;
        }
        else if (y < 0) {
            x_mid = (x_t0 + x_mid)/2;
        }
        else {
            x_mid = (x_mid + x_T)/2;
        }
    }
    for (float i = -5.0; i < 21.0; i++){        
        y = continuous_function(i);
        // push units into arrays
        x_array.push_back(i);
        y_array.push_back(y);
        std::cout << "X:" << i << " Y:" << y << std::endl;
    }


}