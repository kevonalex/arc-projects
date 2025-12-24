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
    for (float i = -5.0; i < 21.0; i++){        
        y = continuous_function(i);
        // push units into arrays
        x_array.push_back(i);
        y_array.push_back(y);
        std::cout << "X:" << i << " Y:" << y << std::endl;
    }


}