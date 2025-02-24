#include "../Libraries/point.h"

ponto2D::ponto2D(): x{0.0}, y{0.0} {}

ponto2D::ponto2D(double x, double y): x{x}, y{y} {}

double ponto2D::distance(const ponto2D& p){
    return std::sqrt(std::pow((p.x - this->x),2) + std::pow((p.y - this->y),2));
}