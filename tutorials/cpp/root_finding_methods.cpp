#include <iostream>
#include <vector>

float continuous_function(float x, float m = -2, float c = 5) {
    return m*(x-3)*(x-3)*(x-3) + c;
};

int main () {
    std::cout << "Root Finding Methods: Bisection Method\n" << std::endl;
    std::cout << "Use continuous function: y = 2(x-3)^3 + 5" << std::endl;

    std::vector<float> x_array;
    std::vector<float> y_array;

    float y;
    float x_t0 = 1.0;
    float x_T = 200.0;
    float x_mid = (x_T - x_t0)/2;
    float epsilon = 0.01f;
    float x_delta = (x_T - x_mid);
    float range_min = 0;
    float range_max = 0;
    int decimal_places = 2;
    float multiplier = std::pow(10.0, decimal_places);
    std::cout << "We are sure that the root is between " << x_t0 << " and " << x_T << ".\n" << std::endl;
    std::cout << "We need to find the root using the bisection method, so we will need to use a while loop to find the root." << std::endl;
    std::cout << "Two new intervals are: " << x_t0 << " to " << x_mid << ", and " << x_mid << " to " << x_T << std::endl;
    std::cout << "Our selected uncertainty is " << epsilon << ", once the interval within which the function's root is determined to be is smaller than this value, the process is concluded and the root is 'found'." << std::endl;

    while (x_delta > epsilon) {
        y = continuous_function(x_mid);
        if (y == 0) {
            std::cout << "Y is 0 at x = " << x_mid << ".\n" << std::endl;
            x_delta = 0;
            range_max = range_min = x_mid;
        }
        else if (y < 0) {
            std::cout << "Function output is less than 0, (" << y << ") midpoint is new maximum." << std::endl;
            x_T = x_mid;
            x_mid = (x_t0 + x_T)/2;
            std::cout << "Recomputing midpoint as " << x_mid << "." << std::endl;
            x_delta = (x_T - x_mid);
            std::cout << "Recomputing delta_x: " << x_delta << "." << std::endl;
            range_max = x_T;
            range_min = x_t0;
        }
        else {
            std::cout << "Function output is greater than 0, (" << y << ") midpoint is new minimum." << std::endl;
            x_t0 = x_mid;
            x_mid = (x_t0 + x_T)/2;
            std::cout << "Recomputing midpoint as " << x_mid << "." << std::endl;
            x_delta = (x_T - x_mid);
            std::cout << "Recomputing delta_x: " << x_delta << "." << std::endl;
            range_max = x_T;
            range_min = x_t0;
        }
    }
    if (range_max != range_min) {
        std::cout << "Root is determined to be between " << range_min << " and " << range_max << "." << std::endl;
        float range_mid = (range_min + range_max)/2;

        std::cout << "Root is approx. ~" << round(range_mid * multiplier)/multiplier << std::endl;
    }
    else {
        std::cout << "Root found at " << range_max << std::endl;
    }
    // for (float i = -5.0; i < 21.0; i++){        
    //     y = continuous_function(i);
    //     // push units into arrays
    //     x_array.push_back(i);
    //     y_array.push_back(y);
    //     std::cout << "X:" << i << " Y:" << y << std::endl;
    // }


}