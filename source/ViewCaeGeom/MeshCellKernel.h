#ifndef VIEWCAEGEOM_MESHCELLKERNEL_HEADERFILE
#define VIEWCAEGEOM_MESHCELLKERNEL_HEADERFILE

#include <Standard_Macro.hxx>
#include <array>
#include <vector>
#include <memory>

namespace ViewCaeGeom {
class CellIndexArray {
public:
    std::vector<int> Values;
    int GetNumberOfIds() const { return (int)Values.size(); }
    int GetId(int i) const { return Values.at(i); }
    void SetNumberOfIds(int n) { Values.resize(n); }
    void SetId(int i, int id) { Values.at(i)=id; }
    int InsertNextId(int id) { Values.push_back(id); return (int)Values.size()-1; }
};
class CellPointArray {
public:
    std::vector<std::array<double,3>> Values;
    int GetNumberOfPoints() const { return (int)Values.size(); }
    void SetNumberOfPoints(int n) { Values.resize(n); }
    void SetDataTypeToDouble() {}
    const double* GetPoint(int n) const { return Values.at(n).data(); }
    void GetPoint(int n, double* p) const { const auto& q=Values.at(n); for(int a=0;a<3;++a) {p[a]=q[a];} }
    void SetPoint(int n, const double* p) { Values.at(n)={{p[0],p[1],p[2]}}; }
    void SetPoint(int n, double x,double y,double z) { Values.at(n)={{x,y,z}}; }
    int InsertNextPoint(double x,double y,double z) { Values.push_back({{x,y,z}});return (int)Values.size()-1; }
    int InsertNextPoint(const double* p) { return InsertNextPoint(p[0],p[1],p[2]); }
};
struct CellBoundary { int Type; std::vector<int> Nodes; };
struct CellReference {
    std::vector<std::array<double,3>> Points;
    std::vector<CellBoundary> Edges, Faces;
};
//! Repository-owned finite-element maps and topology for concrete VTK file cells.
//! Coordinates and IDs follow the file format; no VTK runtime or headers are used.
class MeshCellKernel {
public:
    Standard_EXPORT explicit MeshCellKernel(int type=0, int count=-1,
        const std::vector<int>& degrees={}, const std::vector<double>& weights={});
    Standard_EXPORT static std::shared_ptr<MeshCellKernel> InstantiateCell(int type);
    Standard_EXPORT void SetCellType(int type);
    Standard_EXPORT void SetOrder(const std::vector<int>& degrees);
    void SetRationalWeights(const std::vector<double>& weights) { myRational=weights; }
    const std::vector<double>& RationalWeights() const { return myRational; }
    Standard_EXPORT int GetOrder(int axis=0) const;
    int GetCellType() const { return myType; }
    Standard_EXPORT int GetCellDimension() const;
    Standard_EXPORT bool IsLinear() const;
    int GetNumberOfPoints() const { return myPoints.GetNumberOfPoints(); }
    CellPointArray* GetPoints() { return &myPoints; }
    const CellPointArray* GetPoints() const { return &myPoints; }
    CellIndexArray* GetPointIds() { return &myIds; }
    int GetPointId(int n) const { return myIds.GetId(n); }
    MeshCellKernel* GetRepresentativeCell() { return this; }
    Standard_EXPORT int GetNumberOfEdges() const;
    Standard_EXPORT int GetNumberOfFaces() const;
    Standard_EXPORT MeshCellKernel* GetEdge(int n) const;
    Standard_EXPORT MeshCellKernel* GetFace(int n) const;
    Standard_EXPORT const double* GetParametricCoords() const;
    Standard_EXPORT int GetParametricCenter(double* p) const;
    Standard_EXPORT const double* GetBounds() const;
    Standard_EXPORT bool TriangulateLocalIds(int,CellIndexArray* ids) const;
    Standard_EXPORT void InterpolateFunctions(const double* p,double* weights) const;
    Standard_EXPORT void InterpolateDerivs(const double* p,double* derivatives) const;
    Standard_EXPORT void EvaluateLocation(int& sub,const double* p,double* position,double* weights) const;
    Standard_EXPORT int EvaluatePosition(const double* position,double* closest,int& sub,
        double* parameter,double& distance,double* weights) const;
    void SetFaces(const std::vector<std::vector<int>>& faces) { myPolyFaces=faces;myReferenceCount=-1; }
private:
    void prepare() const;
    MeshCellKernel* boundary(const CellBoundary& b, bool face, int index) const;
    int myType;
    CellPointArray myPoints;
    CellIndexArray myIds;
    std::vector<int> myDegrees;
    std::vector<double> myRational;
    std::vector<std::vector<int>> myPolyFaces;
    mutable int myReferenceCount=-1;
    mutable CellReference myReference;
    mutable std::vector<double> myCoordinates;
    mutable std::vector<std::shared_ptr<MeshCellKernel>> myEdges,myFaces;
    mutable std::vector<std::array<int,3>> myMonomials;
    mutable std::vector<double> myInverse;
    mutable std::array<double,6> myBounds;
    mutable std::vector<std::array<int,3>> myCage;
};
}
#endif
