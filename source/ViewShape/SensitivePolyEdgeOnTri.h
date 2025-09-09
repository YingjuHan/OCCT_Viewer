#ifndef SENSITIVEPOLYEDGEONTRI_HEADERFILE
#define SENSITIVEPOLYEDGEONTRI_HEADERFILE

#include <ViewShape/SensitiveShape.h>

#include <Graphic3d_Vec2.hxx>
#include <Poly_PolygonOnTriangulation.hxx>
#include <Poly_Triangulation.hxx>
#include <Select3D_SensitiveSet.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <TopoDS_Edge.hxx>

namespace ViewShape
{

    //! Sensitive for picking TopoDS_Edge polygon defined on triangulation nodes.
    class SensitivePolyEdgeOnTri : public Select3D_SensitiveSet, public SensitiveShape
    {
    public:
        //! Return sensitive on edge or NULL if no polygon on triangulation is defined.
        Standard_EXPORT static Handle(SensitivePolyEdgeOnTri) GetEdgeSensitive(
            const TopoDS_Edge &theEdge,
            const Handle(SelectMgr_EntityOwner) & theOwnerId);

    public:
        //! Main constructor.
        Standard_EXPORT SensitivePolyEdgeOnTri(const Handle(SelectMgr_EntityOwner) & theOwnerId,
                                               const TopoDS_Edge &theEdge,
                                               const Handle(Poly_PolygonOnTriangulation) & thePolyOnTri,
                                               const Handle(Poly_Triangulation) & theTrg,
                                               const TopLoc_Location &theInitLoc);

        //! Connected constructor.
        Standard_EXPORT SensitivePolyEdgeOnTri(const Handle(SelectMgr_EntityOwner) & theOwnerId,
                                               const TopoDS_Edge &theEdge,
                                               const Handle(Poly_PolygonOnTriangulation) & thePolyOnTri,
                                               const Handle(Poly_Triangulation) & theTrg,
                                               const TopLoc_Location &theInitLoc,
                                               const gp_Pnt &theCOG);

        //! Destructor.
        Standard_EXPORT virtual ~SensitivePolyEdgeOnTri();

    public:
        //! Return shape.
        virtual TopoDS_Shape GetShape() const Standard_OVERRIDE { return myEdge; }

        //! Return shape index.
        virtual int GetShapeIndex() const Standard_OVERRIDE { return mySubshapeIndex; }

        //! Set shape index.
        void SetShapeIndex(int theIndex) { mySubshapeIndex = theIndex; }

        //! Return edge.
        const TopoDS_Edge &GetEdge() const { return myEdge; }

        //! Return polygon.
        const Handle(Poly_PolygonOnTriangulation) & Polygon() const { return myPolyOnTri; }

        //! Return triangulation.
        const Handle(Poly_Triangulation) & Triangulation() const { return myTriangul; }

        //! Return edge location.
        const TopLoc_Location &GetInitLocation() const { return myInitLocation; }

    public:
        //! Get last detected segment.
        Standard_EXPORT bool LastDetectedSegment(Graphic3d_Vec2i &theSeg) const;

        //! Get last detected segment.
        Standard_EXPORT bool LastDetectedSegment(Graphic3d_Vec2i &theSeg, gp_Pnt theTriNodes[2]) const;

        //! Return index of last detected segment within [1..NbSegs] range, or -1 if undefined.
        int LastDetectedSegmentIndex() const
        {
            return (myDetectedIdx != -1 && !myBVHPrimIndexes.IsNull()) ? myBVHPrimIndexes->Value(myDetectedIdx) + 1 : -1;
        }

    public:
        //! Returns the amount of nodes in polygon.
        virtual Standard_Integer NbSubElements() const Standard_OVERRIDE
        {
            return !myPolyOnTri.IsNull() ? myPolyOnTri->NbNodes() : 0;
        }

        //! Return connected copy.
        Standard_EXPORT Handle(Select3D_SensitiveEntity) GetConnected() Standard_OVERRIDE;

        //! Returns the length of array of edges.
        virtual Standard_Integer Size() const Standard_OVERRIDE
        {
            return !myPolyOnTri.IsNull() ? myPolyOnTri->NbNodes() / m_nbSegNodes2 : 0;
        }

        //! Returns geometry center of triangle/edge with index theIdx
        //! in array along the given axis theAxis
        Standard_EXPORT virtual Standard_Real Center(const Standard_Integer theIdx,
                                                     const Standard_Integer theAxis) const Standard_OVERRIDE;

        //! Returns bounding box of triangle/edge with index theIdx
        Standard_EXPORT virtual Select3D_BndBox3d Box(const Standard_Integer theIdx) const Standard_OVERRIDE;

        //! Swaps items with indexes theIdx1 and theIdx2 in array
        Standard_EXPORT virtual void Swap(const Standard_Integer theIdx1, const Standard_Integer theIdx2) Standard_OVERRIDE;

        //! Returns bounding box of the triangulation. If location
        //! transformation is set, it will be applied
        Standard_EXPORT virtual Select3D_BndBox3d BoundingBox() Standard_OVERRIDE;

        //! Returns center of triangulation. If location transformation
        //! is set, it will be applied
        virtual gp_Pnt CenterOfGeometry() const Standard_OVERRIDE { return myCDG3D; }

        //! Returns true if the shape corresponding to the entity has init location
        virtual Standard_Boolean HasInitLocation() const Standard_OVERRIDE { return !myInitLocation.IsIdentity(); }

        //! Returns inversed location transformation matrix if the shape corresponding
        //! to this entity has init location set. Otherwise, returns identity matrix.
        virtual gp_GTrsf InvInitLocation() const Standard_OVERRIDE { return myInvInitLocation; }

        //! Checks whether one or more entities of the set overlap current selecting volume.
        Standard_EXPORT virtual Standard_Boolean Matches(SelectBasics_SelectingVolumeManager &theMgr,
                                                         SelectBasics_PickResult &thePickResult) Standard_OVERRIDE;

    protected:
        //! Inner function for transformation application
        //! to bounding box of the triangulation
        Standard_EXPORT Select3D_BndBox3d applyTransformation();

        //! Compute bounding box.
        Standard_EXPORT void computeBoundingBox();

    private:
        //! Checks whether the element with index theIdx
        //! overlaps the current selecting volume.
        Standard_EXPORT virtual Standard_Boolean overlapsElement(SelectBasics_PickResult &thePickResult,
                                                                 SelectBasics_SelectingVolumeManager &theMgr,
                                                                 Standard_Integer theElemIdx,
                                                                 Standard_Boolean theIsFullInside) Standard_OVERRIDE;

        //! Checks whether the entity with index theIdx
        //! is inside the current selecting volume.
        Standard_EXPORT virtual Standard_Boolean elementIsInside(SelectBasics_SelectingVolumeManager &theMgr,
                                                                 Standard_Integer theElemIdx,
                                                                 Standard_Boolean theIsFullInside) Standard_OVERRIDE;

        //! Calculates distance from the 3d projection
        //! of used-picked screen point to center of the geometry.
        Standard_EXPORT virtual Standard_Real distanceToCOG(SelectBasics_SelectingVolumeManager &theMgr) Standard_OVERRIDE;

    private:
        //! Number of nodes in a segment.
        static constexpr int m_nbSegNodes2 = 2;

    private:
        TopoDS_Edge myEdge;
        Handle(Poly_PolygonOnTriangulation) myPolyOnTri;
        Handle(Poly_Triangulation) myTriangul;
        TopLoc_Location myInitLocation;
        gp_GTrsf myInvInitLocation;
        gp_Pnt myCDG3D; //!< Center of the whole triangulation

        int mySubshapeIndex = -1;

        Handle(NCollection_Shared<TColStd_Array1OfInteger>)
            myBVHPrimIndexes; //!< Indexes of edges or triangles for BVH build

        mutable Select3D_BndBox3d myBndBox; //!< Bounding box of the whole triangulation
    };

} // namespace ViewShape

#endif // SENSITIVEPOLYEDGEONTRI_HEADERFILE
