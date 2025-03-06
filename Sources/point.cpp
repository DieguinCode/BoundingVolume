#include "../Libraries/point.h"

ponto2D::ponto2D(): x{0.0}, y{0.0} {}

ponto2D::ponto2D(double x, double y): x{x}, y{y} {}

double ponto2D::distance(const ponto2D& p){
    return std::sqrt(std::pow((p.x - this->x),2) + std::pow((p.y - this->y),2));
}

ponto2D ponto2D::operator+(const ponto2D& p) const{
    
    double x = this->x + p.x;
    double y = this->y + p.y;

    return ponto2D(x,y);
}

ponto2D ponto2D::operator-(const ponto2D& p) const{
    
    double x = this->x - p.x;
    double y = this->y - p.y;

    return ponto2D(x,y);
}
    
ponto2D ponto2D::operator*(const double& s) const{
    double x = this->x * s;
    double y = this->y * s;

    return ponto2D(x,y);
}