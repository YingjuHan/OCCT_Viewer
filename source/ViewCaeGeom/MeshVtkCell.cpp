#include <ViewCaeGeom/MeshVtkCell.h>
#include <ViewCaeGeom/MeshVtkAnalytic.h>
#include <RWCae/MeshAdaptorsIterator.h>
#include <vtkCellData.h>
#include <vtkBezierQuadrilateral.h>
#include <vtkBezierTriangle.h>
#include <vtkDoubleArray.h>
#include <vtkIdList.h>
#include <vtkIntArray.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolygon.h>
#include <vtkMeanValueCoordinatesInterpolator.h>
#include <vtkTriangle.h>
#include <math_Gauss.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <gp_Pln.hxx>
#include <vtkUnstructuredGrid.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <stdexcept>

namespace ViewCaeGeom {
MeshVtkCell::MeshVtkCell(const Handle(RWCae::MeshData)& mesh, RWCae::MeshEntityId index,
    MeshGeometry* source, double tolerance, const MeshCutGeometryOptions& options, int minimumDepth)
    : myCell(vtkSmartPointer<vtkGenericCell>::New()), myTolerance(tolerance), myOptions(options), myMinimumDepth(minimumDepth)
{
    const auto& indexed = mesh->IndexedElements()[index];
    RWCae::MeshData::GenericCell topology;
    if (indexed.Type == RWCae::MeshElemType::Generic) { topology = mesh->GenericCells().at(indexed.Index); }
    else {
        const std::map<RWCae::MeshElemType,int> types = {{RWCae::MeshElemType::Elem0d,1},
            {RWCae::MeshElemType::Triangle,5},{RWCae::MeshElemType::Triangle2nd,22},
            {RWCae::MeshElemType::Quad,9},{RWCae::MeshElemType::Quad2nd,23},{RWCae::MeshElemType::Polygon,7},
            {RWCae::MeshElemType::Tetrahedron,10},{RWCae::MeshElemType::Tetrahedron2nd,24},
            {RWCae::MeshElemType::Hexahedron,12},{RWCae::MeshElemType::Hexahedron2nd,25},
            {RWCae::MeshElemType::Prism,13},{RWCae::MeshElemType::Prism2nd,26},
            {RWCae::MeshElemType::Pyramid,14},{RWCae::MeshElemType::Pyramid2nd,27},{RWCae::MeshElemType::Polyhedron,42}};
        topology.Type = types.at(indexed.Type); mesh->ElementNodes(index,topology.Nodes);
        if (topology.Type == 42) {
            RWCae::MeshAdaptorsIterator3dElems::Switch(indexed.Type,[&](const auto& adaptor) {
                const auto& element = adaptor.GetElement(indexed.Index);
                for (RWCae::MeshElemEntityId f = 0; f < adaptor.NbFaces(element); ++f) {
                    std::vector<RWCae::MeshEntityId> nodes; RWCae::MeshElemType type;
                    adaptor.FaceNodes(element,f,nodes,type); topology.Faces.push_back(std::move(nodes));
                }
                return true;
            },mesh);
        }
    }
    myNodes = topology.Nodes;
    auto concrete = vtkSmartPointer<vtkCell>::Take(vtkGenericCell::InstantiateCell(topology.Type));
    if (!concrete) { throw std::invalid_argument("VTK cell type "+std::to_string(topology.Type)+" is not concrete in the installed SDK"); }
    vtkNew<vtkUnstructuredGrid> grid;
    vtkNew<vtkPoints> points; points->SetDataTypeToDouble();
    std::map<RWCae::MeshEntityId,vtkIdType> local;
    std::vector<vtkIdType> ids;
    for (auto node : myNodes) {
        auto p = mesh->Nodes()[node]; source->ModifyNode(node,p);
        for (int a = 0; a < 3; ++a) { if (!std::isfinite(p[a])) { throw std::invalid_argument("Nonfinite cell coordinates"); } }
        ids.push_back(points->InsertNextPoint(p.x(),p.y(),p.z())); local[node] = ids.back();
    }
    grid->SetPoints(points);
    if (topology.Type == 42) {
        std::vector<vtkIdType> faces;
        for (const auto& face : topology.Faces) {
            faces.push_back((vtkIdType)face.size());
            for (auto node : face) { faces.push_back(local.at(node)); }
        }
        grid->InsertNextCell(42,(vtkIdType)ids.size(),ids.data(),(vtkIdType)topology.Faces.size(),faces.data());
    } else { grid->InsertNextCell(topology.Type,(vtkIdType)ids.size(),ids.data()); }
    if (!topology.Degrees.empty()) {
        vtkNew<vtkIntArray> degrees; degrees->SetNumberOfComponents(3);
        degrees->InsertNextTypedTuple(topology.Degrees.data()); grid->GetCellData()->SetHigherOrderDegrees(degrees);
    }
    if (!topology.RationalWeights.empty()) {
        vtkNew<vtkDoubleArray> weights;
        for (double weight : topology.RationalWeights) { weights->InsertNextValue(weight); }
        grid->GetPointData()->SetRationalWeights(weights);
    }
    grid->GetCell(0,myCell);
    if (topology.Type == 42) {
        // Triangular MVC cages reproduce affine fields on concave polygonal faces too.
        myMvcFaces = vtkSmartPointer<vtkIdList>::New();
        vtkNew<vtkIdList> triangles;
        std::vector<std::array<vtkIdType,3>> cage;
        for (int f = 0; f < myCell->GetNumberOfFaces(); ++f) {
            auto face = myCell->GetFace(f);
            if (!face->TriangulateLocalIds(0,triangles)) { throw std::runtime_error("Cannot triangulate polyhedron MVC cage"); }
            for (vtkIdType t = 0; t < triangles->GetNumberOfIds(); t += 3) {
                cage.push_back({{face->GetPointId(triangles->GetId(t)),
                    face->GetPointId(triangles->GetId(t+1)),face->GetPointId(triangles->GetId(t+2))}});
            }
        }
        // VTK face triangulation does not guarantee consistent triangle winding.
        using Edge = std::pair<vtkIdType,vtkIdType>;
        std::map<Edge,std::vector<std::pair<size_t,int>>> edges;
        for (size_t t = 0; t < cage.size(); ++t) {
            for (int a = 0; a < 3; ++a) {
                const auto u = cage[t][a], v = cage[t][(a+1)%3];
                edges[std::minmax(u,v)].push_back({t,u < v ? 1 : -1});
            }
        }
        for (const auto& edge : edges) {
            if (edge.second.size() != 2) { throw std::runtime_error("Polyhedron boundary is not a closed manifold"); }
        }
        std::vector<int> orientation(cage.size(),0);
        for (size_t seed = 0; seed < cage.size(); ++seed) {
            if (orientation[seed]) { continue; }
            orientation[seed] = 1;
            std::vector<size_t> pending = {seed};
            while (!pending.empty()) {
                const auto t = pending.back(); pending.pop_back();
                for (int a = 0; a < 3; ++a) {
                    const auto u = cage[t][a], v = cage[t][(a+1)%3];
                    const auto& pair = edges.at(std::minmax(u,v));
                    const auto& neighbor = pair[pair[0].first == t ? 1 : 0];
                    const int expected = -orientation[t]*(u < v ? 1 : -1)*neighbor.second;
                    if (!orientation[neighbor.first]) { orientation[neighbor.first] = expected; pending.push_back(neighbor.first); }
                    else if (orientation[neighbor.first] != expected) { throw std::runtime_error("Polyhedron boundary is not orientable"); }
                }
            }
        }
        for (size_t t = 0; t < cage.size(); ++t) {
            if (orientation[t] < 0) { std::swap(cage[t][1],cage[t][2]); }
            for (auto id : cage[t]) { myMvcFaces->InsertNextId(id); }
        }
    }
    if (auto polygon = vtkPolygon::SafeDownCast(myCell->GetRepresentativeCell())) { polygon->SetUseMVCInterpolation(true); }
    if (Dimension() == 3) {
        if (topology.Type == 41) {
            // ConvexPointSet exposes tetrahedralization, but no GetFace() boundary.
            vtkNew<vtkIdList> tetrahedra;
            if (!myCell->TriangulateLocalIds(0,tetrahedra) || tetrahedra->GetNumberOfIds()%4) {
                throw std::runtime_error("Cannot tetrahedralize convex point set");
            }
            std::map<std::array<vtkIdType,3>,std::pair<int,std::array<vtkIdType,3>>> faces;
            const int indices[4][3] = {{0,2,1},{0,1,3},{1,2,3},{2,0,3}};
            for (vtkIdType t = 0; t < tetrahedra->GetNumberOfIds(); t += 4) {
                for (const auto& face : indices) {
                    std::array<vtkIdType,3> triangleIds = {{tetrahedra->GetId(t+face[0]),tetrahedra->GetId(t+face[1]),tetrahedra->GetId(t+face[2])}};
                    auto key = triangleIds; std::sort(key.begin(),key.end());
                    auto& entry = faces[key]; ++entry.first; entry.second = triangleIds;
                }
            }
            for (const auto& face : faces) {
                if (face.second.first != 1) { continue; }
                std::array<Graphic3d_Vec3d,3> triangle;
                for (int a = 0; a < 3; ++a) {
                    const auto p = myCell->GetPoints()->GetPoint(face.second.second[a]); triangle[a] = Graphic3d_Vec3d(p[0],p[1],p[2]);
                }
                Face record; record.Type = 5; record.FirstTriangle = Triangles.size(); record.NbTriangles = 1;
                for (auto id : face.second.second) { record.Nodes.push_back(myNodes[(size_t)id]); }
                for (int a = 0; a < 3; ++a) { record.Edges.push_back({record.Nodes[a],record.Nodes[(a+1)%3]}); }
                Faces.push_back(std::move(record));
                Triangles.push_back(triangle);
            }
        } else {
            for (int f = 0; f < myCell->GetNumberOfFaces(); ++f) {
                auto face = myCell->GetFace(f);
                if (!topology.RationalWeights.empty()) {
                    vtkDoubleArray* weights = nullptr;
                    if (auto quad = vtkBezierQuadrilateral::SafeDownCast(face)) { weights = quad->GetRationalWeights(); }
                    if (auto triangle = vtkBezierTriangle::SafeDownCast(face)) { weights = triangle->GetRationalWeights(); }
                    if (weights) {
                        // Some VTK wedge versions write faceId instead of the local face-node index.
                        weights->SetNumberOfValues(face->GetNumberOfPoints());
                        for (vtkIdType n = 0; n < face->GetNumberOfPoints(); ++n) {
                            weights->SetValue(n,topology.RationalWeights.at((size_t)face->GetPointId(n)));
                        }
                    }
                }
                Face record; record.Type = face->GetCellType(); record.FirstTriangle = Triangles.size();
                for (vtkIdType n = 0; n < face->GetNumberOfPoints(); ++n) { record.Nodes.push_back(myNodes.at((size_t)face->GetPointId(n))); }
                for (int e = 0; e < face->GetNumberOfEdges(); ++e) {
                    auto edge = face->GetEdge(e);
                    std::vector<RWCae::MeshEntityId> nodes;
                    double p[3], q[3]; edge->GetPoints()->GetPoint(0,p); edge->GetPoints()->GetPoint(1,q);
                    const Graphic3d_Vec3d direction(q[0]-p[0],q[1]-p[1],q[2]-p[2]);
                    const double length = direction.Modulus(), epsilon = std::max(1e-7,tolerance*1e-5);
                    for (vtkIdType n = 0; n < edge->GetNumberOfPoints(); ++n) {
                        nodes.push_back(myNodes.at((size_t)edge->GetPointId(n)));
                        const auto r = edge->GetPoints()->GetPoint(n);
                        const Graphic3d_Vec3d d(r[0]-p[0],r[1]-p[1],r[2]-p[2]);
                        const double position = length > epsilon ? d.Dot(direction)/(length*length) : 0;
                        record.StraightSided &= length > epsilon && position >= -1e-8 && position <= 1+1e-8
                            && Graphic3d_Vec3d::Cross(d,direction).Modulus()/length <= epsilon;
                    }
                    std::sort(nodes.begin(),nodes.end()); record.Edges.push_back(std::move(nodes));
                }
                std::sort(record.Edges.begin(),record.Edges.end());
                if (myOptions.Mode != MeshCutGeometryMode::Numerical) {
                    record.Surface = VtkAnalyticSurface(face,AnalyticBounds,AnalyticResidual);
                }
                SourceApproximate |= record.Surface.IsNull() && (!face->IsLinear() || face->GetCellType() == 9);
                tessellate(face,tolerance); record.NbTriangles = Triangles.size()-record.FirstTriangle;
                Faces.push_back(std::move(record));
            }
        }
    } else if (Dimension() == 0) {
        for (vtkIdType n = 0; n < myCell->GetNumberOfPoints(); ++n) {
            const double* p = myCell->GetPoints()->GetPoint(n); Points.emplace_back(p[0],p[1],p[2]);
        }
    } else if (topology.Type == 36) {
        SourceApproximate = true;
        if (myOptions.Mode != MeshCutGeometryMode::Numerical) {
            for (int e = 0; e < myCell->GetNumberOfEdges(); ++e) {
                const auto curve = VtkAnalyticCurve(myCell->GetEdge(e),AnalyticBounds,AnalyticResidual);
                if (curve.IsNull()) { BoundaryCurves.clear(); break; }
                BoundaryCurves.push_back(curve);
            }
            if (!BoundaryCurves.empty()) {
                const gp_Pnt a = BoundaryCurves[0]->Value(0), b = BoundaryCurves[0]->Value(1), c = BoundaryCurves[1]->Value(1);
                const gp_Vec normal = gp_Vec(a,b).Crossed(gp_Vec(a,c));
                bool planar = normal.Magnitude() > 1e-14;
                if (planar) {
                    const gp_Pln plane(a,gp_Dir(normal));
                    for (const auto& curve : BoundaryCurves) {
                        for (int p = 1; p <= curve->NbPoles(); ++p) { planar &= plane.Distance(curve->Pole(p)) <= 1e-7; }
                    }
                }
                if (planar) { SourceApproximate = false; }
                else { BoundaryCurves.clear(); AnalyticBounds.SetVoid(); }
            }
        }
        // Quadratic polygons have world-coordinate MVC interpolation, not a parametric chart.
        for (int e = 0; e < myCell->GetNumberOfEdges(); ++e) { tessellate(myCell->GetEdge(e),tolerance); }
        vtkNew<vtkPolygon> polygon;
        polygon->GetPoints()->SetDataTypeToDouble();
        for (const auto& segment : Segments) {
            const auto& p = segment[0];
            const auto id = polygon->GetPoints()->InsertNextPoint(p.x(),p.y(),p.z());
            polygon->GetPointIds()->InsertNextId(id);
        }
        Segments.clear(); tessellate(polygon,tolerance);
    } else {
        if (myOptions.Mode != MeshCutGeometryMode::Numerical) {
            if (Dimension() == 1) { Curve = VtkAnalyticCurve(myCell->GetRepresentativeCell(),AnalyticBounds,AnalyticResidual); }
            else { Surface = VtkAnalyticSurface(myCell->GetRepresentativeCell(),AnalyticBounds,AnalyticResidual); }
        }
        SourceApproximate = Curve.IsNull() && Surface.IsNull()
            && (!myCell->IsLinear() || topology.Type == 9);
        tessellate(myCell,tolerance);
    }
    if (SourceApproximate && Dimension() == 3) {
        // Do not mix exact and faceted sides of the same curved boundary.
        for (auto& face : Faces) { face.Surface.Nullify(); }
        AnalyticBounds.SetVoid();
    }
    if (SourceApproximate && myOptions.Mode == MeshCutGeometryMode::AnalyticRequired) {
        throw std::invalid_argument("Analytic VTK boundary is unsupported for cell type "+std::to_string(topology.Type));
    }
}

Graphic3d_Vec3d MeshVtkCell::InteriorPoint() const
{
    double q[3], p[3];
    int sub = myCell->GetParametricCenter(q);
    std::vector<double> weights(myNodes.size());
    myCell->EvaluateLocation(sub,q,p,weights.data());
    return Graphic3d_Vec3d(p[0],p[1],p[2]);
}

MeshInterpolatedNode MeshVtkCell::Origin(const Graphic3d_Vec3d& position) const
{
    const double p[] = {position.x(),position.y(),position.z()};
    double closest[3], parametric[3], distance;
    int sub = 0;
    std::vector<double> weights(myNodes.size());
    const int type = myCell->GetCellType();
    int inside;
    if (type == 7 || type == 36) {
        myCell->InterpolateFunctions(p,weights.data()); inside = 1; distance = 0;
        if (type == 36 && !BoundaryCurves.empty()) {
            for (size_t e = 0; e < BoundaryCurves.size(); ++e) {
                GeomAPI_ProjectPointOnCurve projection(gp_Pnt(p[0],p[1],p[2]),BoundaryCurves[e],0,1);
                if (!projection.NbPoints() || projection.LowerDistance() > 1e-8) { continue; }
                auto edge = myCell->GetEdge((int)e);
                double q[] = {projection.LowerDistanceParameter(),0,0}, mapped[3], edgeWeights[3]; int localSub = 0;
                edge->EvaluateLocation(localSub,q,mapped,edgeWeights);
                std::fill(weights.begin(),weights.end(),0);
                for (int n = 0; n < 3; ++n) { weights[(size_t)edge->GetPointId(n)] = edgeWeights[n]; }
                break;
            }
        }
    } else { inside = myCell->EvaluatePosition(p,closest,sub,parametric,distance,weights.data()); }
    if (myMvcFaces) {
        // MVC is ill-conditioned on cage edges; use the containing boundary triangle there.
        bool onBoundary = false;
        vtkNew<vtkTriangle> triangle;
        for (vtkIdType t = 0; t < myMvcFaces->GetNumberOfIds(); t += 3) {
            for (int a = 0; a < 3; ++a) { triangle->GetPoints()->SetPoint(a,myCell->GetPoints()->GetPoint(myMvcFaces->GetId(t+a))); }
            double barycentric[3], cp[3], q[3], squared;
            int localSub = 0;
            const int location = triangle->EvaluatePosition(p,cp,localSub,q,squared,barycentric);
            if (location >= 0 && squared <= 1e-14) {
                std::fill(weights.begin(),weights.end(),0.0);
                for (int a = 0; a < 3; ++a) { weights[(size_t)myMvcFaces->GetId(t+a)] = barycentric[a]; }
                inside = 1; distance = 0; onBoundary = true; break;
            }
        }
        if (!onBoundary) { vtkMeanValueCoordinatesInterpolator::ComputeInterpolationWeights(p,myCell->GetPoints(),myMvcFaces,weights.data()); }
    }
    const bool refine = type != 7 && type != 36 && type != 41 && type != 42 && type != 4 && type != 6 && Dimension() > 0;
    if (!refine && (inside == -1 || !std::isfinite(distance) || (inside == 0 && distance > myTolerance*myTolerance*4))) {
        throw std::runtime_error("VTK shape-function evaluation failed for source cell "+std::to_string(myCell->GetCellType()));
    }
    if (refine) {
        if (inside == -1 || !std::isfinite(distance)) { sub = myCell->GetParametricCenter(parametric); }
        // Refine VTK's inverse-map seed: several quadratic cells stop at rendering-level precision.
        const int dimension = Dimension(), count = (int)myNodes.size();
        std::vector<double> derivatives((size_t)count*3);
        for (int iteration = 0; iteration < 24; ++iteration) {
            double positionOnCell[3]; myCell->EvaluateLocation(sub,parametric,positionOnCell,weights.data());
            const Graphic3d_Vec3d residual(positionOnCell[0]-p[0],positionOnCell[1]-p[1],positionOnCell[2]-p[2]);
            if (residual.Modulus() < 1e-11) { break; }
            Graphic3d_Vec3d columns[3] = {Graphic3d_Vec3d(0),Graphic3d_Vec3d(0),Graphic3d_Vec3d(0)};
            if (type >= 68) {
                // Differentiate the actual map, including anisotropic order and rational weights.
                constexpr double step = 1e-6;
                for (int a = 0; a < dimension; ++a) {
                    double q[] = {parametric[0],parametric[1],parametric[2]}, plus[3], minus[3];
                    q[a] += step; myCell->EvaluateLocation(sub,q,plus,weights.data());
                    q[a] -= 2*step; myCell->EvaluateLocation(sub,q,minus,weights.data());
                    columns[a] = Graphic3d_Vec3d(plus[0]-minus[0],plus[1]-minus[1],plus[2]-minus[2])/(2*step);
                }
            } else {
                myCell->InterpolateDerivs(parametric,derivatives.data());
                for (int a = 0; a < dimension; ++a) {
                    for (int n = 0; n < count; ++n) {
                        const auto q = myCell->GetPoints()->GetPoint(n);
                        columns[a] += Graphic3d_Vec3d(q[0],q[1],q[2])*derivatives[(size_t)a*count+n];
                    }
                }
            }
            math_Matrix matrix(1,dimension,1,dimension);
            math_Vector rhs(1,dimension), step(1,dimension);
            for (int a = 0; a < dimension; ++a) {
                rhs(a+1) = -columns[a].Dot(residual);
                for (int b = 0; b < dimension; ++b) { matrix(a+1,b+1) = columns[a].Dot(columns[b]); }
            }
            math_Gauss solve(matrix,1e-20);
            if (!solve.IsDone()) { break; }
            solve.Solve(rhs,step);
            double length = 0;
            for (int a = 0; a < dimension; ++a) { parametric[a] += step(a+1); length += step(a+1)*step(a+1); }
            if (length < 1e-24) { break; }
        }
        double mapped[3]; myCell->EvaluateLocation(sub,parametric,mapped,weights.data());
        if (Graphic3d_Vec3d(mapped[0]-p[0],mapped[1]-p[1],mapped[2]-p[2]).Modulus() > myTolerance*2) {
            throw std::runtime_error("VTK inverse-map refinement did not converge for type "+std::to_string(type)
                +" at "+std::to_string(p[0])+","+std::to_string(p[1])+","+std::to_string(p[2]));
        }
    }
    MeshInterpolatedNode result((int)myNodes.size());
    double sum = 0;
    for (double weight : weights) {
        if (!std::isfinite(weight)) { throw std::runtime_error("Nonfinite VTK interpolation weight"); }
        sum += weight;
    }
    if (type == 15) {
        // VTK's legacy pentagonal-prism coefficients are rounded; restore affine precision.
        math_Matrix gram(1,4,1,4,0.0);
        math_Vector residual(1,4), correction(1,4);
        const double target[] = {1.0,p[0],p[1],p[2]};
        for (int a = 0; a < 4; ++a) { residual(a+1) = target[a]; }
        for (size_t n = 0; n < weights.size(); ++n) {
            const auto q = myCell->GetPoints()->GetPoint((vtkIdType)n);
            const double basis[] = {1.0,q[0],q[1],q[2]};
            for (int a = 0; a < 4; ++a) {
                residual(a+1) -= weights[n]*basis[a];
                for (int b = 0; b < 4; ++b) { gram(a+1,b+1) += basis[a]*basis[b]; }
            }
        }
        math_Gauss solve(gram,1e-20);
        if (!solve.IsDone()) { throw std::runtime_error("Degenerate pentagonal prism"); }
        solve.Solve(residual,correction); sum = 0;
        for (size_t n = 0; n < weights.size(); ++n) {
            const auto q = myCell->GetPoints()->GetPoint((vtkIdType)n);
            const double delta = correction(1)+q[0]*correction(2)+q[1]*correction(3)+q[2]*correction(4);
            if (std::abs(delta) > 1e-4) { throw std::runtime_error("Pentagonal-prism shape function residual exceeds correction budget"); }
            weights[n] += delta; sum += weights[n];
        }
    }
    if (std::abs(sum-1) > 1e-5) { throw std::runtime_error("VTK interpolation does not form a partition of unity"); }
    for (size_t n = 0; n < weights.size(); ++n) {
        if (std::abs(weights[n]) > 1e-14) { result.AddBaseNode(myNodes[n],weights[n]/sum); }
    }
    return result;
}

void MeshVtkCell::tessellate(vtkCell* cell, double tolerance)
{
    if (cell && cell->GetCellDimension() == 2 && !cell->IsLinear()
        && (cell->GetNumberOfEdges() == 3 || cell->GetNumberOfEdges() == 4)) {
        // A flat, straight-sided FE face needs no parametric-spacing tessellation.
        double a[3], b[3], c[3];
        cell->GetPoints()->GetPoint(0,a); cell->GetPoints()->GetPoint(1,b); cell->GetPoints()->GetPoint(2,c);
        const Graphic3d_Vec3d origin(a[0],a[1],a[2]), u(b[0]-a[0],b[1]-a[1],b[2]-a[2]), v(c[0]-a[0],c[1]-a[1],c[2]-a[2]);
        auto normal = Graphic3d_Vec3d::Cross(u,v);
        bool flat = normal.Modulus() > 1e-15;
        if (flat) { normal /= normal.Modulus(); }
        const double epsilon = std::max(1e-9,tolerance*1e-5);
        for (vtkIdType n = 0; flat && n < cell->GetNumberOfPoints(); ++n) {
            const auto p = cell->GetPoints()->GetPoint(n);
            flat = std::abs(Graphic3d_Vec3d(p[0]-a[0],p[1]-a[1],p[2]-a[2]).Dot(normal)) <= epsilon;
        }
        for (int e = 0; flat && e < cell->GetNumberOfEdges(); ++e) {
            auto edge = cell->GetEdge(e);
            double p[3], q[3]; edge->GetPoints()->GetPoint(0,p); edge->GetPoints()->GetPoint(1,q);
            const Graphic3d_Vec3d start(p[0],p[1],p[2]), direction(q[0]-p[0],q[1]-p[1],q[2]-p[2]);
            const double length = direction.Modulus();
            if (length < epsilon) { flat = false; break; }
            for (vtkIdType n = 0; flat && n < edge->GetNumberOfPoints(); ++n) {
                const auto r = edge->GetPoints()->GetPoint(n);
                const Graphic3d_Vec3d d(r[0]-p[0],r[1]-p[1],r[2]-p[2]);
                const double t = d.Dot(direction)/(length*length);
                flat = Graphic3d_Vec3d::Cross(d,direction).Modulus()/length <= epsilon && t >= -1e-8 && t <= 1+1e-8;
            }
        }
        if (flat) {
            vtkNew<vtkPolygon> polygon; polygon->GetPoints()->SetDataTypeToDouble();
            for (int n = 0; n < cell->GetNumberOfEdges(); ++n) {
                polygon->GetPoints()->InsertNextPoint(cell->GetPoints()->GetPoint(n)); polygon->GetPointIds()->InsertNextId(n);
            }
            tessellate(polygon,tolerance); return;
        }
    }
    vtkNew<vtkIdList> simplices;
    if (!cell || !cell->TriangulateLocalIds(0,simplices)) {
        throw std::runtime_error("VTK cell boundary cannot be triangulated");
    }
    const bool parametric = !cell->IsLinear() || cell->GetCellType() == 9 || cell->GetCellType() == 8;
    double* coordinates = parametric ? cell->GetParametricCoords() : nullptr;
    if (parametric && !coordinates) { throw std::runtime_error("VTK cell lacks parametric coordinates for controlled tessellation"); }
    auto evaluate = [&](const Graphic3d_Vec3d& q) {
        double input[] = {q.x(),q.y(),q.z()}, output[3]; int sub = 0;
        std::vector<double> weights((size_t)cell->GetNumberOfPoints());
        cell->EvaluateLocation(sub,input,output,weights.data());
        for (double value : output) { if (!std::isfinite(value)) { throw std::runtime_error("Nonfinite VTK parametric map"); } }
        return Graphic3d_Vec3d(output[0],output[1],output[2]);
    };
    auto sample = [&](vtkIdType id) {
        const double* p = coordinates ? coordinates+3*id : cell->GetPoints()->GetPoint(id);
        return Graphic3d_Vec3d(p[0],p[1],p[2]);
    };
    auto checkBudget = [&] {
        if (Triangles.size()+Segments.size() >= myOptions.MaxSourcePrimitives) {
            throw std::runtime_error("VTK source primitive budget exceeded");
        }
    };
    using Triangle = std::array<Graphic3d_Vec3d,3>;
    auto faceError = [&](const Triangle& q) {
        std::array<Graphic3d_Vec3d,3> p, mid, qp;
        double error = 0;
        for (int n = 0; n < 3; ++n) { p[n] = parametric ? evaluate(q[n]) : q[n]; }
        for (int n = 0; n < 3; ++n) {
            qp[n] = (q[n]+q[(n+1)%3])*0.5;
            mid[n] = parametric ? evaluate(qp[n]) : qp[n];
            error = std::max(error,(mid[n]-(p[n]+p[(n+1)%3])*0.5).Modulus());
            if (parametric) {
                for (double fraction : {0.25,0.75}) {
                    error = std::max(error,(evaluate(q[n]*(1-fraction)+q[(n+1)%3]*fraction)
                        -(p[n]*(1-fraction)+p[(n+1)%3]*fraction)).Modulus());
                }
            }
        }
        if (parametric) {
            error = std::max(error,(evaluate((q[0]+q[1]+q[2])/3)-(p[0]+p[1]+p[2])/3).Modulus());
        }
        return error;
    };
    auto children = [](const Triangle& q) {
        const auto a = (q[0]+q[1])*0.5, b = (q[1]+q[2])*0.5, c = (q[2]+q[0])*0.5;
        return std::array<Triangle,4>{{{{q[0],a,c}},{{a,q[1],b}},{{c,b,q[2]}},{{a,b,c}}}};
    };
    std::function<int(const Triangle&,int)> requiredDepth;
    requiredDepth = [&](const Triangle& q, int depth) {
        const double error = faceError(q);
        if (error > tolerance) {
            if (depth >= myOptions.FaceSubdivisionLimit) {
                throw std::runtime_error("VTK geometric tessellation tolerance not reached within subdivision budget");
            }
            int result = depth;
            for (const auto& child : children(q)) { result = std::max(result,requiredDepth(child,depth+1)); }
            return result;
        }
        return depth;
    };
    std::function<void(const Triangle&,int)> emitFace;
    emitFace = [&](const Triangle& q, int remaining) {
        if (remaining) { for (const auto& child : children(q)) { emitFace(child,remaining-1); } }
        else {
            checkBudget();
            Triangles.push_back({{parametric ? evaluate(q[0]) : q[0],parametric ? evaluate(q[1]) : q[1],parametric ? evaluate(q[2]) : q[2]}});
        }
    };
    std::function<void(const Graphic3d_Vec3d&,const Graphic3d_Vec3d&,int)> line;
    line = [&](const auto& a, const auto& b, int depth) {
        const auto p = parametric ? evaluate(a) : a, q = parametric ? evaluate(b) : b;
        const auto middle = parametric ? evaluate((a+b)*0.5) : (a+b)*0.5;
        double error = (middle-(p+q)*0.5).Modulus();
        if (parametric) {
            error = std::max({error,(evaluate(a*0.75+b*0.25)-(p*0.75+q*0.25)).Modulus(),
                (evaluate(a*0.25+b*0.75)-(p*0.25+q*0.75)).Modulus()});
        }
        if (error > tolerance) {
            if (depth >= myOptions.LineSubdivisionLimit) { throw std::runtime_error("VTK line tessellation tolerance not reached"); }
            line(a,(a+b)*0.5,depth+1); line((a+b)*0.5,b,depth+1);
        } else { checkBudget(); Segments.push_back({{p,q}}); }
    };
    const int size = cell->GetCellDimension()+1;
    if (size < 2 || size > 3 || simplices->GetNumberOfIds()%size != 0) { throw std::runtime_error("Invalid VTK boundary simplices"); }
    if (size == 2) {
        for (vtkIdType n = 0; n < simplices->GetNumberOfIds(); n += 2) { line(sample(simplices->GetId(n)),sample(simplices->GetId(n+1)),0); }
        return;
    }
    std::vector<Triangle> seeds;
    const int corners = cell->GetNumberOfEdges();
    if (parametric && (corners == 3 || corners == 4)) {
        // Corner-based dyadic grids agree on shared FE edges, regardless of face orientation.
        int order[] = {0,1,2,3};
        if (cell->GetCellType() == 8) { std::swap(order[2],order[3]); }
        if (corners == 3) { seeds.push_back({{sample(0),sample(1),sample(2)}}); }
        else {
            int minimum = 0;
            for (int n = 1; n < 4; ++n) {
                if (myNodes.at((size_t)cell->GetPointId(order[n])) < myNodes.at((size_t)cell->GetPointId(order[minimum]))) { minimum = n; }
            }
            const int a = order[minimum], b = order[(minimum+1)%4], c = order[(minimum+2)%4], d = order[(minimum+3)%4];
            seeds.push_back({{sample(a),sample(b),sample(c)}}); seeds.push_back({{sample(a),sample(c),sample(d)}});
        }
    } else {
        for (vtkIdType n = 0; n < simplices->GetNumberOfIds(); n += 3) {
            seeds.push_back({{sample(simplices->GetId(n)),sample(simplices->GetId(n+1)),sample(simplices->GetId(n+2))}});
        }
    }
    int depth = 0;
    for (const auto& seed : seeds) { depth = std::max(depth,requiredDepth(seed,0)); }
    if (parametric && (!cell->IsLinear() || depth > 0)) { depth = std::max(depth,myMinimumDepth); }
    FaceDepth = std::max(FaceDepth,depth);
    for (const auto& seed : seeds) { emitFace(seed,depth); }
}
}
