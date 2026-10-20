#include <ViewCaeGeom/MeshVtkCell.h>
#include <ViewCaeGeom/MeshVtkAnalytic.h>
#include <RWCae/MeshAdaptorsIterator.h>
#include <math_Gauss.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <gp_Pln.hxx>

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <stdexcept>

namespace ViewCaeGeom {
MeshVtkCell::MeshVtkCell(const Handle(RWCae::MeshData)& mesh, RWCae::MeshEntityId index,
    MeshGeometry* source, double tolerance, const MeshCutGeometryOptions& options, int minimumDepth, bool prepareBoundary)
    : myTolerance(tolerance), myOptions(options), myMinimumDepth(minimumDepth)
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
    myCell=std::make_shared<MeshCellKernel>(topology.Type,(int)myNodes.size(),topology.Degrees,topology.RationalWeights);
    std::map<RWCae::MeshEntityId,int> local;
    for(size_t n=0;n<myNodes.size();++n) {
        auto p=mesh->Nodes()[myNodes[n]];source->ModifyNode(myNodes[n],p);
        for(int a=0;a<3;++a) {if(!std::isfinite(p[a])) {throw std::invalid_argument("Nonfinite cell coordinates");}}
        myCell->GetPoints()->SetPoint((int)n,p.x(),p.y(),p.z());local[myNodes[n]]=(int)n;
    }
    if(topology.Type==42) {
        std::vector<std::vector<int>> faces;
        for(const auto& face:topology.Faces) {std::vector<int> ids;for(auto node:face) {ids.push_back(local.at(node));}faces.push_back(std::move(ids));}
        myCell->SetFaces(faces);
    }
    if (!prepareBoundary) { return; }
    if (Dimension() == 3) {
            for (int f = 0; f < myCell->GetNumberOfFaces(); ++f) {
                auto face = myCell->GetFace(f);
                Face record; record.Type = face->GetCellType(); record.FirstTriangle = Triangles.size();
                for (int n = 0; n < face->GetNumberOfPoints(); ++n) { record.Nodes.push_back(myNodes.at((size_t)face->GetPointId(n))); }
                for (int e = 0; e < face->GetNumberOfEdges(); ++e) {
                    auto edge = face->GetEdge(e);
                    std::vector<RWCae::MeshEntityId> nodes;
                    double p[3], q[3]; edge->GetPoints()->GetPoint(0,p); edge->GetPoints()->GetPoint(1,q);
                    const Graphic3d_Vec3d direction(q[0]-p[0],q[1]-p[1],q[2]-p[2]);
                    const double length = direction.Modulus(), epsilon = std::max(1e-7,tolerance*1e-5);
                    for (int n = 0; n < edge->GetNumberOfPoints(); ++n) {
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
    } else if (Dimension() == 0) {
        for (int n = 0; n < myCell->GetNumberOfPoints(); ++n) {
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
        auto polygon=std::make_unique<MeshCellKernel>(7,0);
        polygon->GetPoints()->SetDataTypeToDouble();
        for (const auto& segment : Segments) {
            const auto& p = segment[0];
            const auto id = polygon->GetPoints()->InsertNextPoint(p.x(),p.y(),p.z());
            polygon->GetPointIds()->InsertNextId(id);
        }
        Segments.clear(); tessellate(polygon.get(),tolerance);
    } else {
        if (myOptions.Mode != MeshCutGeometryMode::Numerical) {
            if (Dimension() == 1) { Curve = VtkAnalyticCurve(myCell->GetRepresentativeCell(),AnalyticBounds,AnalyticResidual); }
            else { Surface = VtkAnalyticSurface(myCell->GetRepresentativeCell(),AnalyticBounds,AnalyticResidual); }
        }
        SourceApproximate = Curve.IsNull() && Surface.IsNull()
            && (!myCell->IsLinear() || topology.Type == 9);
        tessellate(myCell.get(),tolerance);
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
    const int type=myCell->GetCellType();
    if(type==2 || type==4 || type==6 || type==7 || type==36 || type==41 || type==42) {
        Graphic3d_Vec3d p(0);for(int n=0;n<myCell->GetNumberOfPoints();++n) {const auto q=myCell->GetPoints()->GetPoint(n);p+=Graphic3d_Vec3d(q[0],q[1],q[2]);}
        return myCell->GetNumberOfPoints() ? p/myCell->GetNumberOfPoints() : p;
    }
    double q[3], p[3];
    int sub = myCell->GetParametricCenter(q);
    std::vector<double> weights(myNodes.size());
    myCell->EvaluateLocation(sub,q,p,weights.data());
    return Graphic3d_Vec3d(p[0],p[1],p[2]);
}

std::vector<std::vector<RWCae::MeshEntityId>> MeshVtkCell::BoundaryNodes() const
{
    std::vector<std::vector<RWCae::MeshEntityId>> result;
    const int dimension = Dimension();
    if (dimension == 1) {
        result = {{myNodes.front()},{myNodes.at(myCell->GetCellType() == 4 ? myNodes.size()-1 : 1)}};
    } else if (dimension == 2 || dimension == 3) {
        if (dimension == 3 && !Faces.empty()) {
            for (const auto& face : Faces) { result.push_back(face.Nodes); }
        } else {
            const int count = dimension == 3 ? myCell->GetNumberOfFaces() : myCell->GetNumberOfEdges();
            for (int i = 0; i < count; ++i) {
                auto boundary = dimension == 3 ? myCell->GetFace(i) : myCell->GetEdge(i);
                std::vector<RWCae::MeshEntityId> nodes;
                for (int n = 0; n < boundary->GetNumberOfPoints(); ++n) {
                    nodes.push_back(myNodes.at((size_t)boundary->GetPointId(n)));
                }
                result.push_back(std::move(nodes));
            }
        }
    }
    return result;
}

std::vector<std::array<Graphic3d_Vec3d,4>> MeshVtkCell::ParameterSimplices() const
{
    const int type = myCell->GetCellType();
    if (type == 14 || type == 27 || type == 33) {
        // VTK pyramid FE maps use a cube chart collapsed at z=1. Its nodal
        // triangulation is a physical pyramid and does not cover this chart.
        // Type 33 also inherits a triangulation containing virtual local IDs.
        const Graphic3d_Vec3d p[] = {Graphic3d_Vec3d(0,0,0),Graphic3d_Vec3d(1,0,0),Graphic3d_Vec3d(1,1,0),Graphic3d_Vec3d(0,1,0),
            Graphic3d_Vec3d(0,0,1),Graphic3d_Vec3d(1,0,1),Graphic3d_Vec3d(1,1,1),Graphic3d_Vec3d(0,1,1)};
        return {{{p[0],p[1],p[2],p[6]}},{{p[0],p[2],p[3],p[6]}},{{p[0],p[3],p[7],p[6]}},
                {{p[0],p[7],p[4],p[6]}},{{p[0],p[4],p[5],p[6]}},{{p[0],p[5],p[1],p[6]}}};
    }
    const auto coordinates = myCell->GetParametricCoords();
    const int count = Dimension()+1;
    auto ids=std::make_unique<CellIndexArray>();
    if (!coordinates || !myCell->TriangulateLocalIds(0,ids.get()) || ids->GetNumberOfIds()%count) {
        throw std::invalid_argument("Isosurface cell has no simplex parameter chart");
    }
    std::vector<std::array<Graphic3d_Vec3d,4>> result;
    for (int n = 0; n < ids->GetNumberOfIds(); n += count) {
        std::array<Graphic3d_Vec3d,4> simplex;
        for (int a = 0; a < count; ++a) {
            const auto id = ids->GetId(n+a);
            if (id < 0 || id >= (int)myNodes.size()) { throw std::invalid_argument("Invalid isosurface chart triangulation"); }
            simplex[a] = Graphic3d_Vec3d(coordinates[3*id],coordinates[3*id+1],coordinates[3*id+2]);
        }
        result.push_back(simplex);
    }
    return result;
}

MeshInterpolatedNode MeshVtkCell::EvaluateParameter(const Graphic3d_Vec3d& parameter, Graphic3d_Vec3d& position) const
{
    double q[] = {parameter.x(),parameter.y(),parameter.z()}, p[3]; int sub = 0;
    std::vector<double> weights(myNodes.size());
    myCell->EvaluateLocation(sub,q,p,weights.data()); position = Graphic3d_Vec3d(p[0],p[1],p[2]);
    MeshInterpolatedNode result((int)myNodes.size()); double sum = 0;
    for (size_t n = 0; n < weights.size(); ++n) {
        if (!std::isfinite(weights[n])) { throw std::runtime_error("Nonfinite isosurface FE weight"); }
        sum += weights[n]; result.AddBaseNode(myNodes[n],weights[n]);
    }
    if (!std::isfinite(p[0]) || !std::isfinite(p[1]) || !std::isfinite(p[2]) || std::abs(sum-1) > 1e-8) {
        throw std::runtime_error("Invalid isosurface FE map");
    }
    return result;
}

MeshInterpolatedNode MeshVtkCell::Origin(const Graphic3d_Vec3d& position, Graphic3d_Vec3d* parameter) const
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
    const bool refine = type != 7 && type != 36 && type != 41 && type != 42 && type != 4 && type != 6 && Dimension() > 0;
    if (!refine && (inside == -1 || !std::isfinite(distance) || (inside == 0 && distance > myTolerance*myTolerance*4))) {
        throw std::runtime_error("VTK shape-function evaluation failed for source cell "+std::to_string(myCell->GetCellType()));
    }
    if (refine) {
        if (inside == -1 || !std::isfinite(distance)) { sub = myCell->GetParametricCenter(parametric); }
        // Refine the kernel seed to the original FE-map tolerance used by cuts.
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
    if (std::abs(sum-1) > 1e-5) { throw std::runtime_error("VTK interpolation does not form a partition of unity"); }
    for (size_t n = 0; n < weights.size(); ++n) {
        if (std::abs(weights[n]) > 1e-14) { result.AddBaseNode(myNodes[n],weights[n]/sum); }
    }
    if (parameter) { *parameter = Graphic3d_Vec3d(parametric[0],parametric[1],parametric[2]); }
    return result;
}

std::array<Graphic3d_Vec3d,3> MeshVtkCell::Differential(
    const Graphic3d_Vec3d& position, const MeshGeometry* mapped) const
{
    const int type = myCell->GetCellType();
    if (type == 4 || type == 6 || type == 7 || type == 36 || type == 41 || type == 42 || Dimension() == 0) {
        throw std::invalid_argument("Material Jacobian requires a parametric FE cell");
    }
    Graphic3d_Vec3d parameter; Origin(position,&parameter);
    const double q[] = {parameter.x(),parameter.y(),parameter.z()};
    std::vector<double> derivatives(myNodes.size()*3);
    myCell->InterpolateDerivs(q,derivatives.data());
    std::array<Graphic3d_Vec3d,3> result = {{Graphic3d_Vec3d(0),Graphic3d_Vec3d(0),Graphic3d_Vec3d(0)}};
    for (size_t n = 0; n < myNodes.size(); ++n) {
        Graphic3d_Vec3d p;
        if (mapped) {
            auto point = const_cast<MeshGeometry*>(mapped)->GetMeshData()->Nodes()[myNodes[n]];
            mapped->ModifyNode(myNodes[n],point); p = Graphic3d_Vec3d(point);
        } else { const auto point = myCell->GetPoints()->GetPoint((int)n); p = Graphic3d_Vec3d(point[0],point[1],point[2]); }
        for (int a = 0; a < Dimension(); ++a) { result[a] += p*derivatives[a*myNodes.size()+n]; }
    }
    return result;
}

void MeshVtkCell::tessellate(MeshCellKernel* cell, double tolerance)
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
        for (int n = 0; flat && n < cell->GetNumberOfPoints(); ++n) {
            const auto p = cell->GetPoints()->GetPoint(n);
            flat = std::abs(Graphic3d_Vec3d(p[0]-a[0],p[1]-a[1],p[2]-a[2]).Dot(normal)) <= epsilon;
        }
        for (int e = 0; flat && e < cell->GetNumberOfEdges(); ++e) {
            auto edge = cell->GetEdge(e);
            double p[3], q[3]; edge->GetPoints()->GetPoint(0,p); edge->GetPoints()->GetPoint(1,q);
            const Graphic3d_Vec3d start(p[0],p[1],p[2]), direction(q[0]-p[0],q[1]-p[1],q[2]-p[2]);
            const double length = direction.Modulus();
            if (length < epsilon) { flat = false; break; }
            for (int n = 0; flat && n < edge->GetNumberOfPoints(); ++n) {
                const auto r = edge->GetPoints()->GetPoint(n);
                const Graphic3d_Vec3d d(r[0]-p[0],r[1]-p[1],r[2]-p[2]);
                const double t = d.Dot(direction)/(length*length);
                flat = Graphic3d_Vec3d::Cross(d,direction).Modulus()/length <= epsilon && t >= -1e-8 && t <= 1+1e-8;
            }
        }
        if (flat) {
            auto polygon=std::make_unique<MeshCellKernel>(7,0); polygon->GetPoints()->SetDataTypeToDouble();
            for (int n = 0; n < cell->GetNumberOfEdges(); ++n) {
                polygon->GetPoints()->InsertNextPoint(cell->GetPoints()->GetPoint(n)); polygon->GetPointIds()->InsertNextId(n);
            }
            tessellate(polygon.get(),tolerance); return;
        }
    }
    auto simplices=std::make_unique<CellIndexArray>();
    if (!cell || !cell->TriangulateLocalIds(0,simplices.get())) {
        throw std::runtime_error("VTK cell boundary cannot be triangulated");
    }
    const bool parametric = !cell->IsLinear() || cell->GetCellType() == 9 || cell->GetCellType() == 8;
    const double* coordinates = parametric ? cell->GetParametricCoords() : nullptr;
    if (parametric && !coordinates) { throw std::runtime_error("VTK cell lacks parametric coordinates for controlled tessellation"); }
    auto evaluate = [&](const Graphic3d_Vec3d& q) {
        double input[] = {q.x(),q.y(),q.z()}, output[3]; int sub = 0;
        std::vector<double> weights((size_t)cell->GetNumberOfPoints());
        cell->EvaluateLocation(sub,input,output,weights.data());
        for (double value : output) { if (!std::isfinite(value)) { throw std::runtime_error("Nonfinite VTK parametric map"); } }
        return Graphic3d_Vec3d(output[0],output[1],output[2]);
    };
    auto sample = [&](int id) {
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
        for (int n = 0; n < simplices->GetNumberOfIds(); n += 2) { line(sample(simplices->GetId(n)),sample(simplices->GetId(n+1)),0); }
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
        for (int n = 0; n < simplices->GetNumberOfIds(); n += 3) {
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
