#pragma once
#include <numeric>

#define MAX_AVERAGE_SIZE 50

class MovingAverager {
private:
    double dataPoints[MAX_AVERAGE_SIZE];
    int capacity;

public:
    MovingAverager(int length, double initValue);

    void pushValue(double dataPoint);

    double getAverage();

    void replaceLastPushedValue(double dataPoint);

    void reset(double initValue);
};
