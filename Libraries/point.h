#pragma once
#include <cmath>

struct ponto2D{

    double x;
    double y;

    ponto2D();
    ponto2D(double x, double y);

    double distance(const ponto2D& p);
};