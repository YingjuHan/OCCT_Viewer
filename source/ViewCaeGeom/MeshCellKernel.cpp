#include <ViewCaeGeom/MeshCellKernel.h>
#include <ViewCaeGeom/MeshCellTables.h>
#include <RWCae/MeshCellTopology.h>
#include <Graphic3d_Vec3.hxx>
#include <math_Gauss.hxx>
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <numeric>
#include <stdexcept>

namespace ViewCaeGeom {
namespace {
using P = std::array<double, 3>;
using V = Graphic3d_Vec3d;
V vec(const P &p) {
    return V(p[0], p[1], p[2]);
}
P point(const V &p) {
    return {{p.x(), p.y(), p.z()}};
}
P mix(const P &a, const P &b, double t) {
    return point(vec(a) * (1 - t) + vec(b) * t);
}
double power(double x, int n) {
    double v = 1;
    for (int i = 0; i < n; ++i) {
        v *= x;
    }
    return v;
}
double lagrange(int p, int i, double t) {
    double v = 1;
    for (int j = 0; j <= p; ++j) {
        if (i != j) {
            v *= (p * t - j) / (i - j);
        }
    }
    return v;
}
double simplexFactor(int p, int i, double t) {
    double v = 1;
    for (int j = 0; j < i; ++j) {
        v *= (p * t - j) / (i - j);
    }
    return v;
}
double bernsteinProduct(int degree, const std::array<int, 4> &counts, const std::array<double, 4> &coordinates) {
    // Compute the complete product in log space: separate factorials overflow
    // above degree 170 even when the final Bernstein coefficient is small.
    double logarithm = std::lgamma(degree + 1.0), sign = 1;
    for (int a = 0; a < 4; ++a) {
        const int count = counts[a];
        logarithm -= std::lgamma(count + 1.0);
        if (!count) {
            continue;
        }
        const double value = coordinates[a];
        if (value == 0) {
            return 0;
        }
        logarithm += count * std::log(std::abs(value));
        if (value < 0 && count % 2) {
            sign = -sign;
        }
    }
    return sign * std::exp(logarithm);
}
double bernstein(int p, int i, double t) {
    return bernsteinProduct(p, {{i, p - i, 0, 0}}, {{t, 1 - t, 0, 0}});
}
// Recursive shells preserve the file-format triangle ordering at every degree.
std::vector<P> triangle(int order) {
    if (!order) {
        return {{{0, 0, 0}}};
    }
    std::vector<P> result;
    for (int inner = 0, n = order; n >= 0; ++inner, n -= 3) {
        if (!n) {
            result.push_back({{(double)inner / order, (double)inner / order, 0}});
            break;
        }
        P corners[] = {{{(double)inner / order, (double)inner / order, 0}},
                       {{(double)(inner + n) / order, (double)inner / order, 0}},
                       {{(double)inner / order, (double)(inner + n) / order, 0}}};
        result.insert(result.end(), corners, corners + 3);
        for (int e = 0; e < 3; ++e) {
            for (int k = 1; k < n; ++k) {
                result.push_back(mix(corners[e], corners[(e + 1) % 3], (double)k / n));
            }
        }
    }
    return result;
}
std::vector<P> tetra(int order) {
    std::vector<P> result;
    const int edges[6][2] = {{0, 1}, {1, 2}, {2, 0}, {0, 3}, {1, 3}, {2, 3}};
    const int faceAxes[4][3] = {{0, 2, 3}, {2, 0, 1}, {2, 1, 3}, {1, 0, 3}}, missing[] = {1, 3, 0, 2};
    for (int inner = 0, n = order; n >= 0; ++inner, n -= 4) {
        const double lo = (double)inner / order, hi = (double)(inner + n) / order;
        if (!n) {
            result.push_back({{lo, lo, lo}});
            break;
        }
        const P corners[] = {{{lo, lo, lo}}, {{hi, lo, lo}}, {{lo, hi, lo}}, {{lo, lo, hi}}};
        result.insert(result.end(), corners, corners + 4);
        for (const auto &e : edges) {
            for (int k = 1; k < n; ++k) {
                result.push_back(mix(corners[e[0]], corners[e[1]], (double)k / n));
            }
        }
        if (n >= 3) {
            for (int f = 0; f < 4; ++f) {
                for (const auto &q : triangle(n - 3)) {
                    const double bary[] = {q[0] * (n - 3), q[1] * (n - 3), (1 - q[0] - q[1]) * (n - 3)};
                    double b[4] = {};
                    b[missing[f]] = inner;
                    for (int a = 0; a < 3; ++a) {
                        b[faceAxes[f][a]] = inner + 1 + bary[a];
                    }
                    result.push_back({{b[0] / order, b[1] / order, b[2] / order}});
                }
            }
        }
    }
    return result;
}
std::vector<P> tensor(int p, int q, int r, int dimension) {
    auto base = FixedCellReference(dimension == 1 ? 3 : dimension == 2 ? 9 : 12);
    auto result = base.Points;
    if (dimension == 1) {
        for (int i = 1; i < p; ++i) {
            result.push_back({{(double)i / p, 0, 0}});
        }
        return result;
    }
    for (const auto &e : base.Edges) {
        const auto a = base.Points[e.Nodes[0]], b = base.Points[e.Nodes[1]];
        int axis = 0;
        for (int k = 0; k < 3; ++k) {
            if (a[k] != b[k]) {
                axis = k;
            }
        }
        const int n = axis == 0 ? p : axis == 1 ? q : r;
        for (int k = 1; k < n; ++k) {
            result.push_back(mix(a, b, (double)k / n));
        }
    }
    if (dimension == 2) {
        for (int j = 1; j < q; ++j) {
            for (int i = 1; i < p; ++i) {
                result.push_back({{(double)i / p, (double)j / q, 0}});
            }
        }
        return result;
    }
    // Tensor face and body DOFs are ordered by the varying axes, first axis fastest.
    const int orders[] = {p, q, r};
    for (int fixed = 0; fixed < 3; ++fixed) {
        for (int side = 0; side < 2; ++side) {
            const int a = fixed == 0 ? 1 : 0, b = fixed == 2 ? 1 : 2;
            for (int j = 1; j < orders[b]; ++j) {
                for (int i = 1; i < orders[a]; ++i) {
                    P v = {{0, 0, 0}};
                    v[fixed] = side;
                    v[a] = (double)i / orders[a];
                    v[b] = (double)j / orders[b];
                    result.push_back(v);
                }
            }
        }
    }
    for (int k = 1; k < r; ++k) {
        for (int j = 1; j < q; ++j) {
            for (int i = 1; i < p; ++i) {
                result.push_back({{(double)i / p, (double)j / q, (double)k / r}});
            }
        }
    }
    return result;
}
std::vector<P> wedge(int p, int r) {
    auto base = FixedCellReference(13);
    auto result = base.Points;
    for (const auto &e : base.Edges) {
        const auto a = base.Points[e.Nodes[0]], b = base.Points[e.Nodes[1]];
        const int n = a[2] == b[2] ? p : r;
        for (int k = 1; k < n; ++k) {
            result.push_back(mix(a, b, (double)k / n));
        }
    }
    std::vector<P> interior;
    for (int j = 1; j < p; ++j) {
        for (int i = 1; i < p - j; ++i) {
            interior.push_back({{(double)i / p, (double)j / p, 0}});
        }
    }
    for (int side = 0; side < 2; ++side) {
        for (auto v : interior) {
            v[2] = side;
            result.push_back(v);
        }
    }
    for (int e = 0; e < 3; ++e) {
        for (int k = 1; k < r; ++k) {
            for (int i = 1; i < p; ++i) {
                auto v = mix(base.Points[e], base.Points[(e + 1) % 3], (double)i / p);
                v[2] = (double)k / r;
                result.push_back(v);
            }
        }
    }
    for (int k = 1; k < r; ++k) {
        for (auto v : interior) {
            v[2] = (double)k / r;
            result.push_back(v);
        }
    }
    return result;
}
std::vector<double> pyramidBasis(const double *p) {
    const double x = 2 * p[0] - 1, y = 2 * p[1] - 1, z = 2 * p[2] - 1, h = 1 - z;
    const double inv = std::abs(h) > 1e-14 ? 1 / h : 0;
    const double a = 1 - x * x * inv * inv, b = 1 - y * y * inv * inv;
    // Rational quadratic pyramid space, enriched by base, side and body bubbles.
    return {1,
            x,
            y,
            z,
            x * z,
            y * z,
            x * x,
            y * y,
            z * z,
            x * y,
            x * y * inv,
            x * x * y * inv,
            x * y * y * inv,
            h * h * a * b,
            z * y * (h + y) * a,
            z * x * (h + x) * b,
            z * (-y) * (h - y) * a,
            z * (-x) * (h - x) * b,
            z * h * a * b};
}
std::vector<double> enrichedBasis(int type, int, const double *p) {
    const double x = p[0], y = p[1], z = p[2], t = 1 - x - y - z;
    const int f = type >= 75 ? type - 75 : type >= 68 ? type - 68 : 1;
    if (f == 1) {
        return {1, x, y, x * x, x * y, y * y, x * y * (1 - x - y)};
    }
    if (f == 5) {
        auto b = enrichedBasis(69, 7, p);
        std::vector<double> result;
        for (int k = 0; k < 3; ++k) {
            for (double v : b) {
                result.push_back(v * power(z, k));
            }
        }
        return result;
    }
    return {1,     x,     y,         z,         x * x,     x * y,     x * z,        y * y,
            y * z, z * z, x * y * t, x * z * t, x * y * z, y * z * t, x * y * z * t};
}
bool enriched(int type, int n) {
    const int f = type >= 75 ? type - 75 : type >= 68 ? type - 68 : -1;
    return type == 34 || (f == 1 && n == 7) || (f == 3 && n == 15) || (f == 5 && n == 21);
}
std::vector<double> inverse(const std::vector<double> &matrix, int n) {
    std::vector<double> a((size_t)n * n * 2, 0);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            a[(size_t)i * 2 * n + j] = matrix[(size_t)i * n + j];
        }
        a[(size_t)i * 2 * n + n + i] = 1;
    }
    for (int k = 0; k < n; ++k) {
        int pivot = k;
        for (int i = k + 1; i < n; ++i) {
            if (std::abs(a[(size_t)i * 2 * n + k]) > std::abs(a[(size_t)pivot * 2 * n + k])) {
                pivot = i;
            }
        }
        if (std::abs(a[(size_t)pivot * 2 * n + k]) < 1e-14) {
            throw std::invalid_argument("Singular finite-element basis");
        }
        if (pivot != k) {
            for (int j = 0; j < 2 * n; ++j) {
                std::swap(a[(size_t)k * 2 * n + j], a[(size_t)pivot * 2 * n + j]);
            }
        }
        const double d = a[(size_t)k * 2 * n + k];
        for (int j = 0; j < 2 * n; ++j) {
            a[(size_t)k * 2 * n + j] /= d;
        }
        for (int i = 0; i < n; ++i) {
            if (i != k) {
                const double c = a[(size_t)i * 2 * n + k];
                for (int j = 0; j < 2 * n; ++j) {
                    a[(size_t)i * 2 * n + j] -= c * a[(size_t)k * 2 * n + j];
                }
            }
        }
    }
    std::vector<double> result((size_t)n * n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            result[(size_t)i * n + j] = a[(size_t)i * 2 * n + n + j];
        }
    }
    return result;
}
// Ear clipping in the dominant plane; concave polygon faces retain their boundary.
std::vector<std::array<int, 3>> triangulate(const std::vector<P> &points) {
    const int n = (int)points.size();
    if (n < 3) {
        return {};
    }
    V normal(0);
    for (int i = 0; i < n; ++i) {
        normal += V::Cross(vec(points[i]), vec(points[(i + 1) % n]));
    }
    int drop = 0;
    for (int a = 1; a < 3; ++a) {
        if (std::abs(normal[a]) > std::abs(normal[drop])) {
            drop = a;
        }
    }
    const int x = (drop + 1) % 3, y = (drop + 2) % 3;
    double area = 0;
    for (int i = 0; i < n; ++i) {
        area += points[i][x] * points[(i + 1) % n][y] - points[i][y] * points[(i + 1) % n][x];
    }
    const double sign = area >= 0 ? 1 : -1;
    auto cross = [&](int a, int b, int c) {
        return sign * ((points[b][x] - points[a][x]) * (points[c][y] - points[a][y]) -
                       (points[b][y] - points[a][y]) * (points[c][x] - points[a][x]));
    };
    std::vector<int> ring(n);
    std::iota(ring.begin(), ring.end(), 0);
    std::vector<std::array<int, 3>> result;
    while (ring.size() > 3) {
        bool clipped = false;
        for (size_t i = 0; i < ring.size(); ++i) {
            const int a = ring[(i + ring.size() - 1) % ring.size()], b = ring[i], c = ring[(i + 1) % ring.size()];
            if (cross(a, b, c) <= 1e-15) {
                continue;
            }
            bool contains = false;
            for (int k : ring) {
                if (k != a && k != b && k != c && cross(a, b, k) > -1e-14 && cross(b, c, k) > -1e-14 &&
                    cross(c, a, k) > -1e-14) {
                    contains = true;
                    break;
                }
            }
            if (contains) {
                continue;
            }
            result.push_back({{a, b, c}});
            ring.erase(ring.begin() + i);
            clipped = true;
            break;
        }
        if (!clipped) {
            throw std::invalid_argument("Polygon boundary is degenerate or self-intersecting");
        }
    }
    result.push_back({{ring[0], ring[1], ring[2]}});
    return result;
}
void polygonWeights(const double *p, const std::vector<P> &points, double *weights) {
    const int n = (int)points.size();
    std::fill(weights, weights + n, 0);
    V normal(0);
    for (int i = 0; i < n; ++i) {
        normal += V::Cross(vec(points[i]), vec(points[(i + 1) % n]));
    }
    if (normal.Modulus() < 1e-15) {
        throw std::invalid_argument("Degenerate polygon interpolation");
    }
    normal /= normal.Modulus();
    const V query(p[0], p[1], p[2]);
    std::vector<V> d;
    std::vector<double> length, half(n);
    for (int i = 0; i < n; ++i) {
        d.push_back(vec(points[i]) - query);
        length.push_back(d.back().Modulus());
        if (length.back() < 1e-12) {
            weights[i] = 1;
            return;
        }
    }
    for (int i = 0; i < n; ++i) {
        const int j = (i + 1) % n;
        const double c = d[i].Dot(d[j]), s = V::Cross(d[i], d[j]).Dot(normal);
        if (std::abs(s) < 1e-14 * length[i] * length[j] && c < 0) {
            weights[i] = length[j] / (length[i] + length[j]);
            weights[j] = 1 - weights[i];
            return;
        }
        half[i] = s / (length[i] * length[j] + c);
    }
    double sum = 0;
    for (int i = 0; i < n; ++i) {
        weights[i] = (half[(i + n - 1) % n] + half[i]) / length[i];
        sum += weights[i];
    }
    if (!std::isfinite(sum) || std::abs(sum) < 1e-15) {
        throw std::runtime_error("Polygon mean-value interpolation failed");
    }
    for (int i = 0; i < n; ++i) {
        weights[i] /= sum;
    }
}
// Projection onto a segment/triangle, including the closest boundary point.
double simplexPosition(const double *p, const std::vector<P> &vertices, double *weights, P &closest) {
    const int n = (int)vertices.size();
    const V query(p[0], p[1], p[2]);
    if (n == 2) {
        const V a = vec(vertices[0]), d = vec(vertices[1]) - a;
        const double length = d.SquareModulus();
        const double t = length > 1e-30 ? std::clamp((query - a).Dot(d) / length, 0., 1.) : 0.;
        weights[0] = 1 - t;
        weights[1] = t;
        closest = point(a + d * t);
        return (vec(closest) - query).SquareModulus();
    }
    const V a = vec(vertices[0]), u = vec(vertices[1]) - a, v = vec(vertices[2]) - a, d = query - a;
    const double uu = u.Dot(u), uv = u.Dot(v), vv = v.Dot(v), du = d.Dot(u), dv = d.Dot(v), det = uu * vv - uv * uv;
    if (det > 1e-30) {
        const double r = (du * vv - dv * uv) / det, t = (dv * uu - du * uv) / det;
        if (r >= -1e-12 && t >= -1e-12 && r + t <= 1 + 1e-12) {
            weights[0] = 1 - r - t;
            weights[1] = r;
            weights[2] = t;
            closest = point(a + u * r + v * t);
            return (vec(closest) - query).SquareModulus();
        }
    }
    double best = std::numeric_limits<double>::infinity();
    for (int i = 0; i < 3; ++i) {
        double w[2];
        P q;
        const double dist = simplexPosition(p, {vertices[i], vertices[(i + 1) % 3]}, w, q);
        if (dist < best) {
            best = dist;
            closest = q;
            std::fill(weights, weights + 3, 0);
            weights[i] = w[0];
            weights[(i + 1) % 3] = w[1];
        }
    }
    return best;
}
void cageWeights(const double *p, const std::vector<P> &points, const std::vector<std::array<int, 3>> &triangles,
                 double *weights) {
    const int n = (int)points.size();
    std::fill(weights, weights + n, 0);
    std::vector<V> unit;
    std::vector<double> length;
    for (int i = 0; i < n; ++i) {
        V d = vec(points[i]) - V(p[0], p[1], p[2]);
        length.push_back(d.Modulus());
        if (length.back() < 1e-12) {
            weights[i] = 1;
            return;
        }
        unit.push_back(d / length.back());
    }
    // Triangular-cage 3-D mean-value coordinates (spherical triangle formula).
    for (const auto &face : triangles) {
        double theta[3], sine[3], cosine[3], signedSine[3];
        for (int a = 0; a < 3; ++a) {
            const auto &u = unit[face[(a + 1) % 3]], &v = unit[face[(a + 2) % 3]];
            sine[a] = V::Cross(u, v).Modulus();
            theta[a] = std::atan2(sine[a], u.Dot(v));
        }
        const double half = (theta[0] + theta[1] + theta[2]) * .5;
        if (std::acos(-1.) - half < 1e-8) {
            double bary[3];
            P closest;
            const double dist = simplexPosition(p, {points[face[0]], points[face[1]], points[face[2]]}, bary, closest);
            if (dist < 1e-14) {
                std::fill(weights, weights + n, 0);
                for (int a = 0; a < 3; ++a) {
                    weights[face[a]] = bary[a];
                }
                return;
            }
        }
        const double determinant = unit[face[0]].Dot(V::Cross(unit[face[1]], unit[face[2]]));
        bool valid = true;
        for (int a = 0; a < 3; ++a) {
            const double denom = sine[(a + 1) % 3] * sine[(a + 2) % 3];
            if (std::abs(denom) < 1e-16) {
                valid = false;
                break;
            }
            const auto &u = unit[face[a]], &v = unit[face[(a + 1) % 3]], &z = unit[face[(a + 2) % 3]];
            cosine[a] = std::clamp(V::Cross(u, v).Dot(V::Cross(u, z)) / denom, -1., 1.);
            // The scalar triple product retains the near-face signed sine without
            // subtracting nearly equal numbers in sqrt(1-cosine*cosine).
            signedSine[a] = determinant / denom;
            if (std::abs(signedSine[a]) < 1e-12) {
                valid = false;
                break;
            }
        }
        if (!valid) {
            continue;
        }
        for (int a = 0; a < 3; ++a) {
            const int b = (a + 1) % 3, c = (a + 2) % 3;
            weights[face[a]] +=
                (theta[a] - cosine[b] * theta[c] - cosine[c] * theta[b]) / (length[face[a]] * sine[b] * signedSine[c]);
        }
    }
    const double sum = std::accumulate(weights, weights + n, 0.);
    if (!std::isfinite(sum) || std::abs(sum) < 1e-14) {
        throw std::runtime_error("Polyhedron mean-value interpolation failed");
    }
    for (int i = 0; i < n; ++i) {
        weights[i] /= sum;
    }
}
std::vector<CellBoundary> convexHull(const std::vector<P> &p) {
    const int n = (int)p.size();
    if (n < 4) {
        throw std::invalid_argument("Convex point set needs four points");
    }
    int b = 1, c = -1, d = -1;
    double best = 0;
    const V a = vec(p[0]);
    for (int i = 1; i < n; ++i) {
        const double v = (vec(p[i]) - a).SquareModulus();
        if (v > best) {
            best = v;
            b = i;
        }
    }
    best = 0;
    for (int i = 1; i < n; ++i) {
        const double v = V::Cross(vec(p[b]) - a, vec(p[i]) - a).SquareModulus();
        if (v > best) {
            best = v;
            c = i;
        }
    }
    if (c < 0) {
        throw std::invalid_argument("Collinear convex point set");
    }
    const V normal = V::Cross(vec(p[b]) - a, vec(p[c]) - a);
    best = 0;
    for (int i = 1; i < n; ++i) {
        const double v = std::abs(normal.Dot(vec(p[i]) - a));
        if (v > best) {
            best = v;
            d = i;
        }
    }
    if (d < 0 || best < 1e-15) {
        throw std::invalid_argument("Coplanar convex point set");
    }
    const V center = (a + vec(p[b]) + vec(p[c]) + vec(p[d])) / 4;
    std::vector<CellBoundary> faces;
    auto face = [&](int i, int j, int k) {
        if (V::Cross(vec(p[j]) - vec(p[i]), vec(p[k]) - vec(p[i])).Dot(center - vec(p[i])) > 0) {
            std::swap(j, k);
        }
        return CellBoundary{5, {i, j, k}};
    };
    faces = {face(0, b, c), face(0, d, b), face(b, d, c), face(c, d, 0)};
    for (int i = 0; i < n; ++i) {
        if (i == 0 || i == b || i == c || i == d) {
            continue;
        }
        std::map<std::pair<int, int>, int> horizon;
        std::vector<CellBoundary> kept;
        for (const auto &f : faces) {
            const auto &v = f.Nodes;
            const V norm = V::Cross(vec(p[v[1]]) - vec(p[v[0]]), vec(p[v[2]]) - vec(p[v[0]]));
            if (norm.Dot(vec(p[i]) - vec(p[v[0]])) <= 1e-12 * norm.Modulus()) {
                kept.push_back(f);
                continue;
            }
            for (int e = 0; e < 3; ++e) {
                auto key = std::make_pair(v[e], v[(e + 1) % 3]);
                auto opposite = std::make_pair(key.second, key.first);
                if (horizon.erase(opposite) == 0) {
                    horizon[key] = 1;
                }
            }
        }
        for (const auto &e : horizon) {
            kept.push_back(face(e.first.first, e.first.second, i));
        }
        faces = std::move(kept);
    }
    return faces;
}
} // namespace
MeshCellKernel::MeshCellKernel(int type, int count, const std::vector<int> &degrees, const std::vector<double> &weights)
    : myType(type), myDegrees(degrees), myRational(weights) {
    if (RWCae::MeshCellTopology::Dimension(type) < 0) {
        throw std::invalid_argument("Unsupported concrete cell type");
    }
    if (count < 0) {
        count = std::max(0, RWCae::MeshCellTopology::FixedSize(type));
    }
    myPoints.SetNumberOfPoints(count);
    myIds.SetNumberOfIds(count);
    std::iota(myIds.Values.begin(), myIds.Values.end(), 0);
}
std::shared_ptr<MeshCellKernel> MeshCellKernel::InstantiateCell(int type) {
    return RWCae::MeshCellTopology::Dimension(type) < 0 ? nullptr : std::make_shared<MeshCellKernel>(type);
}
void MeshCellKernel::SetCellType(int type) {
    myType = type;
    myReferenceCount = -1;
}
void MeshCellKernel::SetOrder(const std::vector<int> &degrees) {
    myDegrees = degrees;
    myReferenceCount = -1;
}
int MeshCellKernel::GetCellDimension() const {
    return RWCae::MeshCellTopology::Dimension(myType);
}
bool MeshCellKernel::IsLinear() const {
    return myType < 21 || myType == 41 || myType == 42;
}
int MeshCellKernel::GetOrder(int axis) const {
    if (!myDegrees.empty()) {
        return myDegrees.at(axis);
    }
    if (myType < 68) {
        return myType == 35 ? 3 : IsLinear() ? 1 : 2;
    }
    const int f = myType >= 75 ? myType - 75 : myType - 68, n = GetNumberOfPoints();
    for (int p = 1; p <= 1024; ++p) {
        const uint64_t size = f == 0   ? p + 1
                              : f == 1 ? (uint64_t)(p + 1) * (p + 2) / 2
                              : f == 2 ? (uint64_t)(p + 1) * (p + 1)
                              : f == 3 ? (uint64_t)(p + 1) * (p + 2) * (p + 3) / 6
                              : f == 4 ? (uint64_t)(p + 1) * (p + 1) * (p + 1)
                                       : (uint64_t)(p + 1) * (p + 1) * (p + 2) / 2;
        if (size == (uint64_t)n || (p == 2 && ((f == 1 && n == 7) || (f == 3 && n == 15) || (f == 5 && n == 21)))) {
            return p;
        }
        if (size > (uint64_t)n) {
            break;
        }
    }
    throw std::invalid_argument("Cell count does not define an isotropic order");
}
void MeshCellKernel::prepare() const {
    const int n = GetNumberOfPoints();
    if (myReferenceCount == n) {
        return;
    }
    myReferenceCount = -1;
    myEdges.clear();
    myFaces.clear();
    myInverse.clear();
    myMonomials.clear();
    if (!RWCae::MeshCellTopology::ValidSize(myType, n, myDegrees)) {
        throw std::invalid_argument("Incorrect cell node count/degrees");
    }
    myReference = FixedCellReference(myType);
    if (myType >= 68) {
        const int f = myType >= 75 ? myType - 75 : myType - 68, p = GetOrder(0), q = GetOrder(1), r = GetOrder(2);
        const int baseTypes[] = {3, 5, 9, 10, 12, 13};
        myReference = FixedCellReference(baseTypes[f]);
        myReference.Points = f == 0   ? tensor(p, 0, 0, 1)
                             : f == 1 ? triangle(p)
                             : f == 2 ? tensor(p, q, 0, 2)
                             : f == 3 ? tetra(p)
                             : f == 4 ? tensor(p, q, r, 3)
                                      : wedge(p, r);
        if (f == 1 && n == 7) {
            myReference.Points.push_back({{1. / 3, 1. / 3, 0}});
        }
        if (f == 3 && n == 15) {
            myReference.Points.insert(myReference.Points.end(), {{{1. / 3, 1. / 3, 0}},
                                                                 {{1. / 3, 0, 1. / 3}},
                                                                 {{1. / 3, 1. / 3, 1. / 3}},
                                                                 {{0, 1. / 3, 1. / 3}},
                                                                 {{.25, .25, .25}}});
        }
        if (f == 5 && n == 21) {
            myReference.Points.insert(myReference.Points.begin() + 15, {{{1. / 3, 1. / 3, 0}}, {{1. / 3, 1. / 3, 1}}});
            myReference.Points.push_back({{1. / 3, 1. / 3, .5}});
        }
        if ((int)myReference.Points.size() != n) {
            throw std::invalid_argument("Higher-order reference cardinality mismatch");
        }
        auto enrich = [&](CellBoundary &b, bool face) {
            const auto old = b.Nodes;
            const int dim = face ? 2 : 1;
            const int family = dim == 1 ? 0 : old.size() == 3 ? 1 : 2;
            const auto a = myReference.Points[old[0]], u = myReference.Points[old[1]],
                       v = dim == 2 ? myReference.Points[old.back()] : a;
            int pu = p, pv = p;
            if (f == 2 || f == 4) {
                for (int axis = 0; axis < 3; ++axis) {
                    if (a[axis] != u[axis]) {
                        pu = axis == 0 ? p : axis == 1 ? q : r;
                    }
                    if (a[axis] != v[axis]) {
                        pv = axis == 0 ? p : axis == 1 ? q : r;
                    }
                }
            }
            if (f == 5) {
                if (a[2] != u[2]) {
                    pu = r;
                }
                if (a[2] != v[2]) {
                    pv = r;
                }
            }
            auto coordinates = family == 0 ? tensor(pu, 0, 0, 1) : family == 1 ? triangle(pu) : tensor(pu, pv, 0, 2);
            if (f == 3 && n == 15 && family == 1) {
                coordinates.push_back({{1. / 3, 1. / 3, 0}});
            }
            if (f == 5 && n == 21 && family == 1) {
                coordinates.push_back({{1. / 3, 1. / 3, 0}});
            }
            b.Nodes.clear();
            b.Type = (myType >= 75 ? 75 : 68) + family;
            for (const auto &c : coordinates) {
                V mapped = vec(a) + (vec(u) - vec(a)) * c[0] + (vec(v) - vec(a)) * c[1];
                int found = -1;
                for (int i = 0; i < n; ++i) {
                    if ((vec(myReference.Points[i]) - mapped).Modulus() < 1e-10) {
                        found = i;
                        break;
                    }
                }
                if (found < 0) {
                    throw std::invalid_argument("Missing high-order boundary node");
                }
                b.Nodes.push_back(found);
            }
        };
        for (auto &e : myReference.Edges) {
            enrich(e, false);
        }
        for (auto &fce : myReference.Faces) {
            enrich(fce, true);
        }
    } else if (myType == 2 || myType == 4 || myType == 6 || myType == 7 || myType == 36 || myType == 41 ||
               myType == 42) {
        myReference.Points = myPoints.Values;
        if (myType == 41) {
            myReference.Faces = convexHull(myPoints.Values);
        }
        if (myType == 42) {
            for (const auto &f : myPolyFaces) {
                myReference.Faces.push_back({7, f});
            }
        }
        if (myType == 7 || myType == 36) {
            const int corners = myType == 36 ? n / 2 : n;
            for (int i = 0; i < corners; ++i) {
                myReference.Edges.push_back(myType == 36 ? CellBoundary{21, {i, (i + 1) % corners, i + corners}}
                                                         : CellBoundary{3, {i, (i + 1) % corners}});
            }
        }
    }
    myCage.clear();
    if (myType == 42 || myType == 41) {
        for (const auto &f : myReference.Faces) {
            std::vector<P> points;
            for (int id : f.Nodes) {
                points.push_back(myPoints.Values.at(id));
            }
            for (const auto &t : triangulate(points)) {
                myCage.push_back({{f.Nodes[t[0]], f.Nodes[t[1]], f.Nodes[t[2]]}});
            }
        }
        using Edge = std::pair<int, int>;
        std::map<Edge, std::vector<std::pair<size_t, int>>> neighbors;
        for (size_t t = 0; t < myCage.size(); ++t) {
            for (int a = 0; a < 3; ++a) {
                const int u = myCage[t][a], v = myCage[t][(a + 1) % 3];
                neighbors[std::minmax(u, v)].push_back({t, u < v ? 1 : -1});
            }
        }
        for (const auto &e : neighbors) {
            if (e.second.size() != 2) {
                throw std::invalid_argument("Polyhedron boundary is not a closed manifold");
            }
        }
        std::vector<int> signs(myCage.size(), 0);
        for (size_t seed = 0; seed < signs.size(); ++seed) {
            if (signs[seed]) {
                continue;
            }
            signs[seed] = 1;
            std::vector<size_t> pending = {seed};
            while (!pending.empty()) {
                const auto t = pending.back();
                pending.pop_back();
                for (int a = 0; a < 3; ++a) {
                    const int u = myCage[t][a], v = myCage[t][(a + 1) % 3];
                    const auto &pair = neighbors.at(std::minmax(u, v));
                    const auto &neighbor = pair[pair[0].first == t ? 1 : 0];
                    const int expected = -signs[t] * (u < v ? 1 : -1) * neighbor.second;
                    if (!signs[neighbor.first]) {
                        signs[neighbor.first] = expected;
                        pending.push_back(neighbor.first);
                    } else if (signs[neighbor.first] != expected) {
                        throw std::invalid_argument("Polyhedron boundary is not orientable");
                    }
                }
            }
        }
        for (size_t t = 0; t < signs.size(); ++t) {
            if (signs[t] < 0) {
                std::swap(myCage[t][1], myCage[t][2]);
            }
        }
    }
    myCoordinates.clear();
    for (const auto &p : myReference.Points) {
        myCoordinates.insert(myCoordinates.end(), p.begin(), p.end());
    }
    myEdges.resize(myReference.Edges.size());
    myFaces.resize(myReference.Faces.size());
    // A small exact nodal polynomial basis handles the fixed serendipity and mixed-order families.
    if ((myType < 68 && myType != 0 && myType != 1 && myType != 2 && myType != 4 && myType != 6 && myType != 7 &&
         myType != 14 && myType != 15 && myType != 27 && myType != 36 && myType != 41 && myType != 42) ||
        myType == 37 || enriched(myType, n)) {
        if (myType != 37 && !enriched(myType, n)) {
            const int dim = GetCellDimension();
            for (int k = 0; k <= (dim == 3 ? 2 : 0); ++k) {
                for (int j = 0; j <= (dim >= 2 ? 2 : 0); ++j) {
                    for (int i = 0; i <= (myType == 35 ? 3 : 2); ++i) {
                        bool use = false;
                        if (myType == 3 || myType == 21 || myType == 35) {
                            use = i <= GetOrder();
                        }
                        if (myType == 5 || myType == 22) {
                            use = i + j <= (myType == 5 ? 1 : 2);
                        }
                        if (myType == 10 || myType == 24) {
                            use = i + j + k <= (myType == 10 ? 1 : 2);
                        }
                        if (myType == 8 || myType == 9) {
                            use = i <= 1 && j <= 1;
                        }
                        if (myType == 11 || myType == 12) {
                            use = i <= 1 && j <= 1 && k <= 1;
                        }
                        if (myType == 13) {
                            use = i + j <= 1 && k <= 1;
                        }
                        if (myType == 23) {
                            use = (i > 1 ? i : 0) + (j > 1 ? j : 0) <= 2;
                        }
                        if (myType == 25) {
                            use = (i > 1 ? i : 0) + (j > 1 ? j : 0) + (k > 1 ? k : 0) <= 2;
                        }
                        if (myType == 28 || myType == 29) {
                            use = true;
                        }
                        if (myType == 30) {
                            use = j <= 1;
                        }
                        if (myType == 31) {
                            use = i + j <= 2 && k <= 1;
                        }
                        if (myType == 32) {
                            use = i + j <= 2;
                        }
                        if (myType == 26) {
                            use = (i + j <= 2 && k <= 1) || (i + j <= 1 && k == 2);
                        }
                        if (myType == 33) {
                            use = (i > 1 ? i : 0) + (j > 1 ? j : 0) <= 2;
                        }
                        if (myType == 34) {
                            use = i + j <= 2 || (i == 2 && j == 1);
                        }
                        if (myType == 16) {
                            use = i <= 2 && j <= 1 && k <= 1;
                        }
                        if (use) {
                            myMonomials.push_back({{i, j, k}});
                        }
                    }
                }
            }
            if ((int)myMonomials.size() != n) {
                throw std::invalid_argument("Incorrect fixed FE basis size");
            }
        }
        std::vector<double> matrix((size_t)n * n);
        for (int i = 0; i < n; ++i) {
            const auto &q = myReference.Points[i];
            const auto basis = myType == 37          ? pyramidBasis(q.data())
                               : enriched(myType, n) ? enrichedBasis(myType, n, q.data())
                                                     : std::vector<double>();
            for (int j = 0; j < n; ++j) {
                const auto e = basis.empty() ? myMonomials[j] : std::array<int, 3>{{0, 0, 0}};
                matrix[(size_t)i * n + j] =
                    !basis.empty() ? basis[j] : power(q[0], e[0]) * power(q[1], e[1]) * power(q[2], e[2]);
            }
        }
        myInverse = inverse(matrix, n);
    }
    myReferenceCount = n;
}
const double *MeshCellKernel::GetParametricCoords() const {
    prepare();
    return myCoordinates.empty() ? nullptr : myCoordinates.data();
}
int MeshCellKernel::GetNumberOfEdges() const {
    prepare();
    return (int)myReference.Edges.size();
}
int MeshCellKernel::GetNumberOfFaces() const {
    prepare();
    return (int)myReference.Faces.size();
}
MeshCellKernel *MeshCellKernel::boundary(const CellBoundary &b, bool face, int index) const {
    auto &entry = (face ? myFaces : myEdges).at(index);
    if (entry) {
        return entry.get();
    }
    std::vector<int> degrees;
    if (b.Type >= 68) {
        const auto coords = b.Type == 68 || b.Type == 75 ? tensor((int)b.Nodes.size() - 1, 0, 0, 1) : std::vector<P>();
        if (!coords.empty()) {
            degrees = {(int)b.Nodes.size() - 1, 0, 0};
        } else if (b.Type == 69 || b.Type == 76) {
            degrees = {GetOrder(), GetOrder(), 0};
        } else {
            const auto &a = myReference.Points[b.Nodes[0]], u = myReference.Points[b.Nodes[1]],
                       v = myReference.Points[b.Nodes[3]];
            int p = GetOrder(), q = GetOrder();
            for (int axis = 0; axis < 3; ++axis) {
                if (a[axis] != u[axis]) {
                    p = GetOrder(axis);
                }
                if (a[axis] != v[axis]) {
                    q = GetOrder(axis);
                }
            }
            degrees = {p, q, 0};
        }
    }
    std::vector<double> rational;
    if (!myRational.empty()) {
        for (int id : b.Nodes) {
            rational.push_back(myRational.at(id));
        }
    }
    entry = std::make_shared<MeshCellKernel>(b.Type, (int)b.Nodes.size(), degrees, rational);
    for (size_t n = 0; n < b.Nodes.size(); ++n) {
        entry->myPoints.Values[n] = myPoints.Values.at(b.Nodes[n]);
        entry->myIds.Values[n] = myIds.Values.at(b.Nodes[n]);
    }
    return entry.get();
}
MeshCellKernel *MeshCellKernel::GetEdge(int n) const {
    prepare();
    return boundary(myReference.Edges.at(n), false, n);
}
MeshCellKernel *MeshCellKernel::GetFace(int n) const {
    prepare();
    return boundary(myReference.Faces.at(n), true, n);
}
const double *MeshCellKernel::GetBounds() const {
    for (int a = 0; a < 3; ++a) {
        myBounds[2 * a] = std::numeric_limits<double>::infinity();
        myBounds[2 * a + 1] = -myBounds[2 * a];
    }
    for (const auto &p : myPoints.Values) {
        for (int a = 0; a < 3; ++a) {
            myBounds[2 * a] = std::min(myBounds[2 * a], p[a]);
            myBounds[2 * a + 1] = std::max(myBounds[2 * a + 1], p[a]);
        }
    }
    return myBounds.data();
}
int MeshCellKernel::GetParametricCenter(double *p) const {
    prepare();
    p[0] = p[1] = p[2] = 0;
    const int n = (int)myReference.Points.size();
    for (const auto &q : myReference.Points) {
        for (int a = 0; a < 3; ++a) {
            p[a] += q[a] / n;
        }
    }
    return 0;
}
bool MeshCellKernel::TriangulateLocalIds(int, CellIndexArray *ids) const {
    prepare();
    ids->Values.clear();
    const int n = GetNumberOfPoints(), dim = GetCellDimension();
    auto emit = [&](std::initializer_list<int> list) { ids->Values.insert(ids->Values.end(), list); };
    if (dim == 0) {
        return true;
    }
    if (dim == 1) {
        if (myType == 4) {
            for (int i = 1; i < n; ++i) {
                emit({i - 1, i});
            }
        } else {
            emit({0, 1});
        }
        return true;
    }
    if (dim == 2) {
        if (myType == 6) {
            for (int i = 2; i < n; ++i) {
                if (i % 2) {
                    emit({i - 1, i - 2, i});
                } else {
                    emit({i - 2, i - 1, i});
                }
            }
            return true;
        }
        if (myType == 7) {
            for (const auto &t : triangulate(myPoints.Values)) {
                emit({t[0], t[1], t[2]});
            }
            return true;
        }
        const int corners = myType == 36 ? n / 2 : (int)myReference.Edges.size();
        for (int i = 1; i < corners - 1; ++i) {
            if (myType == 8) {
                emit({0, i == 1 ? 1 : 3, i == 1 ? 3 : 2});
            } else {
                emit({0, i, i + 1});
            }
        }
        return true;
    }
    const int base = myType >= 68 ? (myType >= 75 ? myType - 75 : myType - 68) : -1;
    if (myType == 10 || myType == 24 || base == 3) {
        emit({0, 1, 2, 3});
        return true;
    }
    if (myType == 13 || myType == 26 || myType == 31 || myType == 32 || base == 5) {
        emit({0, 1, 2, 3});
        emit({1, 2, 3, 4});
        emit({2, 3, 4, 5});
        return true;
    }
    if (myType == 12 || myType == 25 || myType == 29 || myType == 33 || base == 4 || myType == 11) {
        const int order[] = {0, 1, myType == 11 ? 3 : 2, myType == 11 ? 2 : 3,
                             4, 5, myType == 11 ? 7 : 6, myType == 11 ? 6 : 7};
        const int tets[6][4] = {{0, 1, 2, 6}, {0, 2, 3, 6}, {0, 3, 7, 6}, {0, 7, 4, 6}, {0, 4, 5, 6}, {0, 5, 1, 6}};
        for (const auto &t : tets) {
            emit({order[t[0]], order[t[1]], order[t[2]], order[t[3]]});
        }
        return true;
    }
    if (myType == 14 || myType == 27 || myType == 37) {
        emit({0, 1, 2, 4});
        emit({0, 2, 3, 4});
        return true;
    }
    if (myType == 15 || myType == 16) {
        const int m = n / 2;
        for (int i = 1; i < m - 1; ++i) {
            emit({0, i, i + 1, m});
            emit({i, i + 1, m, m + i});
            emit({i + 1, m, m + i, m + i + 1});
        }
        return true;
    }
    if (myType == 41 || myType == 42) {
        for (const auto &f : myReference.Faces) {
            std::vector<P> p;
            for (int id : f.Nodes) {
                p.push_back(myPoints.Values[id]);
            }
            for (const auto &t : triangulate(p)) {
                if (f.Nodes[t[0]] != 0 && f.Nodes[t[1]] != 0 && f.Nodes[t[2]] != 0) {
                    emit({0, f.Nodes[t[0]], f.Nodes[t[1]], f.Nodes[t[2]]});
                }
            }
        }
        return true;
    }
    return false;
}
void MeshCellKernel::InterpolateFunctions(const double *p, double *w) const {
    prepare();
    const int n = GetNumberOfPoints();
    std::fill(w, w + n, 0);
    if (!n) {
        return;
    }
    if (myType == 1) {
        w[0] = 1;
        return;
    }
    if (myType == 7 || myType == 36) {
        if (myType == 7) {
            polygonWeights(p, myPoints.Values, w);
        } else {
            std::vector<P> ring;
            const int m = n / 2;
            for (int i = 0; i < m; ++i) {
                ring.push_back(myPoints.Values[i]);
                ring.push_back(myPoints.Values[i + m]);
            }
            std::vector<double> b(n);
            polygonWeights(p, ring, b.data());
            for (int i = 0; i < m; ++i) {
                w[i] = b[2 * i];
                w[i + m] = b[2 * i + 1];
            }
        }
        return;
    }
    if (myType == 14) {
        w[0] = (1 - p[0]) * (1 - p[1]) * (1 - p[2]);
        w[1] = p[0] * (1 - p[1]) * (1 - p[2]);
        w[2] = p[0] * p[1] * (1 - p[2]);
        w[3] = (1 - p[0]) * p[1] * (1 - p[2]);
        w[4] = p[2];
        return;
    }
    if (myType == 27) {
        const double r = 2 * p[0] - 1, s = 2 * p[1] - 1, t = 2 * p[2] - 1;
        const int signs[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
        for (int i = 0; i < 4; ++i) {
            const double a = signs[i][0] * r, b = signs[i][1] * s;
            w[i] = -(1 + a) * (1 + b) * (1 - t) *
                   (4 - 3 * a - 3 * b + 2 * a * b + 2 * t - a * t - b * t + 2 * a * b * t) / 16;
            w[9 + i] = (1 + a) * (1 + b) * (1 - t * t) / 4;
        }
        w[4] = t * (1 + t) / 2;
        w[5] = (1 - r * r) * (1 - s) * (1 - t) * (2 + s + s * t) / 8;
        w[7] = (1 - r * r) * (1 + s) * (1 - t) * (2 - s - s * t) / 8;
        w[6] = (1 + r) * (1 - s * s) * (1 - t) * (2 - r - r * t) / 8;
        w[8] = (1 - r) * (1 - s * s) * (1 - t) * (2 + r + r * t) / 8;
        return;
    }
    if (myType == 15) {
        // Wachspress coordinates use exact reference edges instead of rounded coefficients.
        const int m = n / 2;
        std::vector<double> area(m), corner(m);
        double sum = 0;
        auto cross = [](const P &a, const P &b, const P &c) {
            return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
        };
        const P q = {{p[0], p[1], 0}};
        for (int i = 0; i < m; ++i) {
            area[i] = cross(myReference.Points[i], myReference.Points[(i + 1) % m], q);
            corner[i] =
                cross(myReference.Points[(i + m - 1) % m], myReference.Points[i], myReference.Points[(i + 1) % m]);
        }
        // Products avoid edge/vertex divisions while retaining the rational FE basis.
        for (int i = 0; i < m; ++i) {
            w[i] = corner[i];
            for (int j = 0; j < m; ++j) {
                if (j != i && j != (i + m - 1) % m) {
                    w[i] *= area[j];
                }
            }
            sum += w[i];
        }
        if (std::abs(sum) < 1e-20) {
            throw std::runtime_error("Pentagonal FE denominator is singular");
        }
        for (int i = 0; i < m; ++i) {
            const double b = w[i] / sum;
            w[i] = b * (1 - p[2]);
            w[i + m] = b * p[2];
        }
        return;
    }
    if (myType >= 68 && !enriched(myType, n)) {
        const int f = myType >= 75 ? myType - 75 : myType - 68, order = GetOrder(), q = GetOrder(1), r = GetOrder(2);
        const bool bezier = myType >= 75;
        for (int i = 0; i < n; ++i) {
            const auto &v = myReference.Points[i];
            const int ix = (int)std::llround(v[0] * order),
                      iy = (int)std::llround(v[1] * (f == 2 || f == 4 ? q : order)), iz = (int)std::llround(v[2] * r);
            if (f == 0 || f == 2 || f == 4) {
                w[i] = bezier ? bernstein(order, ix, p[0]) : lagrange(order, ix, p[0]);
                if (f != 0) {
                    w[i] *= bezier ? bernstein(q, iy, p[1]) : lagrange(q, iy, p[1]);
                }
                if (f == 4) {
                    w[i] *= bezier ? bernstein(r, iz, p[2]) : lagrange(r, iz, p[2]);
                }
            } else {
                const int z = f == 3 ? (int)std::llround(v[2] * order) : 0, rest = order - ix - iy - z;
                w[i] = bezier ? bernsteinProduct(order, {{ix, iy, z, rest}},
                                                 {{p[0], p[1], p[2], 1 - p[0] - p[1] - (f == 3 ? p[2] : 0)}})
                              : simplexFactor(order, ix, p[0]) * simplexFactor(order, iy, p[1]) *
                                    simplexFactor(order, z, p[2]) *
                                    simplexFactor(order, rest, 1 - p[0] - p[1] - (f == 3 ? p[2] : 0));
                if (f == 5) {
                    w[i] *= bezier ? bernstein(r, iz, p[2]) : lagrange(r, iz, p[2]);
                }
            }
        }
        if (bezier && !myRational.empty()) {
            double sum = 0, scale = *std::max_element(myRational.begin(), myRational.end());
            for (int i = 0; i < n; ++i) {
                w[i] *= myRational.at(i) / scale;
                sum += w[i];
            }
            if (!std::isfinite(sum) || std::abs(sum) < 1e-30) {
                throw std::runtime_error("Rational FE denominator is singular");
            }
            for (int i = 0; i < n; ++i) {
                w[i] /= sum;
            }
        }
        return;
    }
    if (!myInverse.empty()) {
        const auto basis = myType == 37          ? pyramidBasis(p)
                           : enriched(myType, n) ? enrichedBasis(myType, n, p)
                                                 : std::vector<double>();
        for (int j = 0; j < n; ++j) {
            const auto e = basis.empty() ? myMonomials[j] : std::array<int, 3>{{0, 0, 0}};
            const double v = !basis.empty() ? basis[j] : power(p[0], e[0]) * power(p[1], e[1]) * power(p[2], e[2]);
            for (int i = 0; i < n; ++i) {
                w[i] += v * myInverse[(size_t)j * n + i];
            }
        }
        return;
    }
    throw std::invalid_argument("Cell requires world-coordinate interpolation");
}
void MeshCellKernel::InterpolateDerivs(const double *p, double *d) const {
    const int n = GetNumberOfPoints();
    std::vector<double> plus(n), minus(n);
    constexpr double step = 1e-6;
    for (int a = 0; a < 3; ++a) {
        double q[] = {p[0], p[1], p[2]};
        q[a] += step;
        InterpolateFunctions(q, plus.data());
        q[a] -= 2 * step;
        InterpolateFunctions(q, minus.data());
        for (int i = 0; i < n; ++i) {
            d[(size_t)a * n + i] = (plus[i] - minus[i]) / (2 * step);
        }
    }
}
void MeshCellKernel::EvaluateLocation(int &, const double *p, double *position, double *w) const {
    if (myType == 41 || myType == 42) {
        std::fill(w, w + GetNumberOfPoints(), 1. / GetNumberOfPoints());
    } else {
        InterpolateFunctions(p, w);
    }
    position[0] = position[1] = position[2] = 0;
    for (int i = 0; i < GetNumberOfPoints(); ++i) {
        for (int a = 0; a < 3; ++a) {
            position[a] += w[i] * myPoints.Values[i][a];
        }
    }
}
int MeshCellKernel::EvaluatePosition(const double *p, double *closest, int &sub, double *parameter, double &distance,
                                     double *w) const {
    prepare();
    const int n = GetNumberOfPoints(), dim = GetCellDimension();
    sub = 0;
    if (myType == 7 || myType == 36) {
        InterpolateFunctions(p, w);
        for (int a = 0; a < 3; ++a) {
            closest[a] = p[a];
            parameter[a] = p[a];
        }
        distance = 0;
        return 1;
    }
    if (dim == 0) {
        if (!n) {
            distance = 0;
            return 1;
        }
        int best = 0;
        distance = std::numeric_limits<double>::infinity();
        for (int i = 0; i < n; ++i) {
            const double d = (vec(myPoints.Values[i]) - V(p[0], p[1], p[2])).SquareModulus();
            if (d < distance) {
                distance = d;
                best = i;
            }
        }
        std::fill(w, w + n, 0);
        w[best] = 1;
        myPoints.GetPoint(best, closest);
        return distance < 1e-14;
    }
    if (myType == 3 || myType == 5 || myType == 4 || myType == 6) {
        std::vector<std::vector<int>> primitives;
        if (myType == 3) {
            primitives = {{0, 1}};
        } else if (myType == 5) {
            primitives = {{0, 1, 2}};
        } else if (myType == 4) {
            for (int i = 1; i < n; ++i) {
                primitives.push_back({i - 1, i});
            }
        } else {
            for (int i = 2; i < n; ++i) {
                primitives.push_back({i - 2, i - 1, i});
            }
        }
        distance = std::numeric_limits<double>::infinity();
        std::fill(w, w + n, 0);
        for (size_t i = 0; i < primitives.size(); ++i) {
            const auto &ids = primitives[i];
            std::vector<P> points;
            for (int id : ids) {
                points.push_back(myPoints.Values[id]);
            }
            double weights[3];
            P q;
            const double d = simplexPosition(p, points, weights, q);
            if (d < distance) {
                distance = d;
                sub = (int)i;
                std::fill(w, w + n, 0);
                for (size_t j = 0; j < ids.size(); ++j) {
                    w[ids[j]] = weights[j];
                }
                for (int a = 0; a < 3; ++a) {
                    closest[a] = q[a];
                }
                parameter[0] = weights[1];
                parameter[1] = ids.size() == 3 ? weights[2] : 0;
                parameter[2] = 0;
            }
        }
        return std::isfinite(distance) ? distance < 1e-14 ? 1 : 0 : -1;
    }
    if (myType == 41 || myType == 42) {
        if (myType == 42) {
            cageWeights(p, myPoints.Values, myCage, w);
        } else {
            CellIndexArray tets;
            TriangulateLocalIds(0, &tets);
            bool found = false;
            for (size_t t = 0; t < tets.Values.size(); t += 4) {
                const int *ids = tets.Values.data() + t;
                const V origin = vec(myPoints.Values[ids[0]]), x = vec(myPoints.Values[ids[1]]) - origin,
                        y = vec(myPoints.Values[ids[2]]) - origin, z = vec(myPoints.Values[ids[3]]) - origin,
                        d = V(p[0], p[1], p[2]) - origin;
                const double det = x.Dot(V::Cross(y, z));
                if (std::abs(det) < 1e-18) {
                    continue;
                }
                const double a = d.Dot(V::Cross(y, z)) / det, b = x.Dot(V::Cross(d, z)) / det,
                             c = x.Dot(V::Cross(y, d)) / det;
                if (a >= -1e-8 && b >= -1e-8 && c >= -1e-8 && a + b + c <= 1 + 1e-8) {
                    std::fill(w, w + n, 0);
                    w[ids[0]] = 1 - a - b - c;
                    w[ids[1]] = a;
                    w[ids[2]] = b;
                    w[ids[3]] = c;
                    found = true;
                    break;
                }
            }
            if (!found) {
                cageWeights(p, myPoints.Values, myCage, w);
            }
        }
        for (int a = 0; a < 3; ++a) {
            closest[a] = 0;
            parameter[a] = p[a];
            for (int i = 0; i < n; ++i) {
                closest[a] += myPoints.Values[i][a] * w[i];
            }
        }
        distance = V(closest[0] - p[0], closest[1] - p[1], closest[2] - p[2]).SquareModulus();
        return std::isfinite(distance) ? 1 : -1;
    }
    GetParametricCenter(parameter);
    std::vector<double> derivatives((size_t)n * 3);
    double mapped[3];
    for (int iteration = 0; iteration < 60; ++iteration) {
        EvaluateLocation(sub, parameter, mapped, w);
        V residual(mapped[0] - p[0], mapped[1] - p[1], mapped[2] - p[2]);
        if (residual.Modulus() < 1e-12) {
            break;
        }
        InterpolateDerivs(parameter, derivatives.data());
        V columns[] = {V(0), V(0), V(0)};
        for (int a = 0; a < dim; ++a) {
            for (int i = 0; i < n; ++i) {
                columns[a] += vec(myPoints.Values[i]) * derivatives[(size_t)a * n + i];
            }
        }
        math_Matrix matrix(1, dim, 1, dim);
        math_Vector rhs(1, dim), delta(1, dim);
        for (int a = 0; a < dim; ++a) {
            rhs(a + 1) = -columns[a].Dot(residual);
            for (int b = 0; b < dim; ++b) {
                matrix(a + 1, b + 1) = columns[a].Dot(columns[b]);
            }
        }
        math_Gauss solver(matrix, 1e-22);
        if (!solver.IsDone()) {
            break;
        }
        solver.Solve(rhs, delta);
        double length = 0;
        for (int a = 0; a < dim; ++a) {
            parameter[a] += delta(a + 1);
            length += delta(a + 1) * delta(a + 1);
        }
        if (length < 1e-26) {
            break;
        }
    }
    EvaluateLocation(sub, parameter, closest, w);
    distance = V(closest[0] - p[0], closest[1] - p[1], closest[2] - p[2]).SquareModulus();
    return std::isfinite(distance) ? distance < 1e-14 ? 1 : 0 : -1;
}
} // namespace ViewCaeGeom
