#include <ViewCaeGeom/MeshVtkAnalytic.h>
#include <TColgp_Array1OfPnt.hxx>
#include <TColgp_Array2OfPnt.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColStd_Array2OfReal.hxx>
#include <math_Gauss.hxx>
#include <array>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
using ViewCaeGeom::MeshCellKernel;
using HPoint = std::array<double,4>;
double choose(int n, int k)
{
    double value = 1;
    for (int a = 1; a <= k; ++a) { value *= (double)(n-a+1)/a; }
    return value;
}
double bernstein(int n, int k, double u)
{
    return choose(n,k)*std::pow(u,k)*std::pow(1-u,n-k);
}
std::vector<HPoint> controls(const std::vector<HPoint>& samples)
{
    const int degree = (int)samples.size()-1;
    math_Matrix matrix(1,degree+1,1,degree+1);
    for (int j = 0; j <= degree; ++j) {
        for (int i = 0; i <= degree; ++i) { matrix(j+1,i+1) = bernstein(degree,i,(double)j/degree); }
    }
    math_Gauss solver(matrix,1e-15);
    if (!solver.IsDone()) { throw std::runtime_error("Singular VTK Bernstein conversion"); }
    std::vector<HPoint> result(samples.size());
    math_Vector rhs(1,degree+1), solution(1,degree+1);
    for (int axis = 0; axis < 4; ++axis) {
        for (int n = 0; n <= degree; ++n) { rhs(n+1) = samples[n][axis]; }
        solver.Solve(rhs,solution);
        for (int n = 0; n <= degree; ++n) { result[n][axis] = solution(n+1); }
    }
    return result;
}
struct Map {
    MeshCellKernel* Cell;
    int U = 0;
    int V = 0;
    bool Triangle = false;
    const std::vector<double>* Weights = nullptr;
    double WeightScale = 1;
    double Tolerance = 0;
    explicit Map(MeshCellKernel* cell) : Cell(cell)
    {
        const int type = cell->GetCellType();
        switch (type) {
            case 3: U = 1; break;
            case 21: U = 2; break;
            case 35: U = 3; break;
            case 5: U = V = 1; Triangle = true; break;
            case 8: case 9: U = V = 1; break;
            case 22: U = V = 2; Triangle = true; break;
            case 23: case 28: case 30: U = V = 2; break;
            case 34: U = V = 3; Triangle = true; break;
            case 68: case 75: U = cell->GetOrder(0); break;
            case 69: case 76:
                U = V = cell->GetOrder(); Triangle = true; break;
            case 70: case 77: {
                const auto quad = cell;
                U = quad->GetOrder(0); V = quad->GetOrder(1); break;
            }
            default: return;
        }
        if (U < 1 || U > 6 || V > 6) { U = V = 0; return; }
        if (type>=75 && !cell->RationalWeights().empty()) { Weights=&cell->RationalWeights(); }
        if (Weights) {
            if (Weights->size() != cell->GetNumberOfPoints()) { U = V = 0; return; }
            WeightScale = 0;
            double minimum = std::numeric_limits<double>::infinity();
            for (size_t n = 0; n < Weights->size(); ++n) {
                const double weight = Weights->at(n);
                if (!std::isfinite(weight) || weight <= 0) {
                    throw std::invalid_argument("VTK analytic weights must be finite and positive");
                }
                WeightScale = std::max(WeightScale,weight); minimum = std::min(minimum,weight);
            }
            // The converted control net requires normalized weights above 1e-12.
            // Reject this representation before endpoint sampling can throw, so
            // analytic-preferred geometry retains its explicit numerical fallback.
            if (minimum/WeightScale <= 1e-12) { U = V = 0; return; }
        }
        const auto bounds = cell->GetBounds();
        double extent = 0, coordinate = 0;
        for (int a = 0; a < 3; ++a) {
            extent += std::pow(bounds[2*a+1]-bounds[2*a],2);
            coordinate = std::max({coordinate,std::abs(bounds[2*a]),std::abs(bounds[2*a+1])});
        }
        Tolerance = std::max(1e-12,std::sqrt(extent)*1e-10+coordinate*64*std::numeric_limits<double>::epsilon());
    }
    HPoint sample(double u, double v) const
    {
        // A Duffy chart gives a rectangular rational patch with one degenerate edge for triangles.
        double q[] = {u,Triangle ? (1-u)*v : v,0}, p[3]; int sub = 0;
        std::vector<double> basis((size_t)Cell->GetNumberOfPoints());
        Cell->EvaluateLocation(sub,q,p,basis.data());
        double denominator = 1;
        if (Weights) {
            denominator = 0;
            const auto coordinates = Cell->GetParametricCoords();
            for (size_t n = 0; n < Cell->GetNumberOfPoints(); ++n) {
                const int i = (int)std::llround(coordinates[3*n]*U);
                const int j = V ? (int)std::llround(coordinates[3*n+1]*V) : 0;
                const double b = Triangle ? choose(U,i)*choose(U-i,j)*std::pow(q[0],i)*std::pow(q[1],j)*std::pow(1-q[0]-q[1],U-i-j)
                    : bernstein(U,i,q[0])*(V ? bernstein(V,j,q[1]) : 1);
                denominator += b*(Weights->at(n)/WeightScale);
            }
        }
        HPoint result = {{p[0]*denominator,p[1]*denominator,p[2]*denominator,denominator}};
        for (double value : result) { if (!std::isfinite(value)) { throw std::runtime_error("Nonfinite VTK homogeneous map"); } }
        if (denominator <= 1e-14) { throw std::runtime_error("VTK homogeneous map exceeds analytic weight precision"); }
        return result;
    }
    gp_Pnt point(double u, double v) const
    {
        const auto h = sample(u,v); return gp_Pnt(h[0]/h[3],h[1]/h[3],h[2]/h[3]);
    }
};
}

namespace ViewCaeGeom {
Handle(Geom_BezierSurface) VtkAnalyticSurface(MeshCellKernel* cell, Bnd_Box& bounds, double& residual)
{
    Map map(cell); if (!map.U || !map.V) { return nullptr; }
    std::vector<std::vector<HPoint>> net((size_t)map.U+1,std::vector<HPoint>((size_t)map.V+1));
    for (int v = 0; v <= map.V; ++v) {
        std::vector<HPoint> samples;
        for (int u = 0; u <= map.U; ++u) { samples.push_back(map.sample((double)u/map.U,(double)v/map.V)); }
        const auto row = controls(samples);
        for (int u = 0; u <= map.U; ++u) { net[u][v] = row[u]; }
    }
    TColgp_Array2OfPnt poles(1,map.U+1,1,map.V+1); TColStd_Array2OfReal weights(1,map.U+1,1,map.V+1);
    Bnd_Box hull;
    for (int u = 0; u <= map.U; ++u) {
        const auto column = controls(net[u]);
        for (int v = 0; v <= map.V; ++v) {
            const auto& p = column[v]; if (p[3] <= 1e-12) { return nullptr; }
            const gp_Pnt point(p[0]/p[3],p[1]/p[3],p[2]/p[3]);
            poles(u+1,v+1) = point; weights(u+1,v+1) = p[3]; hull.Add(point);
        }
    }
    Handle(Geom_BezierSurface) result = new Geom_BezierSurface(poles,weights);
    double error = 0;
    for (double u : {0.07,0.23,0.51,0.79,0.93}) {
        for (double v : {0.11,0.37,0.63,0.89}) { error = std::max(error,result->Value(u,v).Distance(map.point(u,v))); }
    }
    if (error > map.Tolerance) { return nullptr; }
    residual = std::max(residual,error); bounds.Add(hull); return result;
}

Handle(Geom_BezierCurve) VtkAnalyticCurve(MeshCellKernel* cell, Bnd_Box& bounds, double& residual)
{
    Map map(cell); if (!map.U || map.V) { return nullptr; }
    std::vector<HPoint> samples;
    for (int u = 0; u <= map.U; ++u) { samples.push_back(map.sample((double)u/map.U,0)); }
    const auto net = controls(samples);
    TColgp_Array1OfPnt poles(1,map.U+1); TColStd_Array1OfReal weights(1,map.U+1); Bnd_Box hull;
    for (int u = 0; u <= map.U; ++u) {
        const auto& p = net[u]; if (p[3] <= 1e-12) { return nullptr; }
        const gp_Pnt point(p[0]/p[3],p[1]/p[3],p[2]/p[3]); poles(u+1) = point; weights(u+1) = p[3]; hull.Add(point);
    }
    Handle(Geom_BezierCurve) result = new Geom_BezierCurve(poles,weights);
    double error = 0;
    for (double u : {0.07,0.23,0.51,0.79,0.93}) { error = std::max(error,result->Value(u).Distance(map.point(u,0))); }
    if (error > map.Tolerance) { return nullptr; }
    residual = std::max(residual,error); bounds.Add(hull); return result;
}
}
