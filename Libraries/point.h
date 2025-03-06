#pragma once
#include <cmath>

struct ponto2D{

    double x;
    double y;

    ponto2D operator+(const ponto2D& p) const;
    ponto2D operator-(const ponto2D& p) const;
    ponto2D operator*(const double& s) const;

    ponto2D();
    ponto2D(double x, double y);

    double distance(const ponto2D& p);
};