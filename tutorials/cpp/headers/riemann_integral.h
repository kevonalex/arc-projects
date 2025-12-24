#include <iostream>
#include <cmath>
#include <vector>

float continuous_function(float x, float m, float c) {
    return m * x + c;
}

int main() {
    std::cout << "Riemann Integral -- First Principles" << std::endl;
    std::cout << "This demonstration will show the calculation of the Riemann Integral from first principles." << std::endl;
    // first, declare the function to calculate the integral of
    float x = 5;
    float m = -1;
    float c = 0;

    float y = continuous_function(x,m,c);
    std::cout << "Result: " << y << std::endl; 
    float x_0 = 5;
    float x_T = 0;
    // int y_0 = continuous_function(x_0,m,c);
    // int y_T = continuous_function(x_T,m,c);
    
    std::vector<float> x_array;
    std::vector<float> y_array;
    x_array.push_back(0);
    y_array.push_back(continuous_function(0,m,c));
    
    
    float no_of_partitions = 1000;
    float h = (x_T - x_0)/no_of_partitions;
    std::cout << "Partititon size: " << h << std::endl;

    std::cout << "Initialising xt as initial x at time t=0" << std::endl;
    float xt = x_0;
    // float yt = y_0;
    float area = 0.0;

    for (int i = 1; i < no_of_partitions + 1; i++) {
        std::cout << "Partititon number: " << i << std::endl;
        float xi = xt + h;
        float yt = continuous_function(xt, m, c);
        // float yi = continuous_function(xi, m, c);
        area += yt * h;
        std::cout << "Total area: " << area << std::endl;
        // std::cout << "X("<< i - 1 << ":" << i << "): " << xi << std::endl;
        // std::cout << "Y(" << i - 1 << ":" << i << "): " << yi << std::endl;
        // x_array.push_back(xi);
        // y_array.push_back(yi);
        xt = xi;
    }
    std::cout << "Area computed. Final total: " << area << std::endl;

    return 0;
}