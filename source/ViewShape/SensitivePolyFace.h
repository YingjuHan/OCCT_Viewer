
#ifndef SENSITIVEPOLYFACE_HEADERFILE
#define SENSITIVEPOLYFACE_HEADERFILE

#include <ViewShape/SensitiveShape.h>

#include <Select3D_SensitiveTriangulation.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>

namespace ViewShape
{

    //! Sensitive for picking tessellated TopoDS_Face.
    //! This class extends Select3D_SensitiveTriangulation for storing originating face.
    class SensitivePolyFace : public Select3D_SensitiveTriangulation, public SensitiveShape
    {
    public:
        //! Main constructor.
        Standard_EXPORT SensitivePolyFace(const Handle(SelectMgr_EntityOwner) & theOwnerId,
                                          const TopoDS_Face &theFace,
                                          const Handle(Poly_Triangulation) & theTrg,
                                          const TopLoc_Location &theInitLoc);

        //! Destructor.
        Standard_EXPORT virtual ~SensitivePolyFace();

        //! Return shape.
        virtual TopoDS_Shape GetShape() const Standard_OVERRIDE { return myFace; }

        //! Return shape index.
        virtual int GetShapeIndex() const Standard_OVERRIDE { return mySubshapeIndex; }

        //! Set shape index.
        void SetShapeIndex(int theIndex) { mySubshapeIndex = theIndex; }

        //! Return face.
        const TopoDS_Face &GetFace() const { return myFace; }

        //! Find edge to highlight presentation.
        Standard_EXPORT bool DetectClosestEdge(const TopoDS_Shape &theDetShape,
                                               const gp_Pnt &thePickPoint,
                                               const double thePickTol,
                                               TopoDS_Edge &theResEdge) const;

        //! Checks whether one or more entities of the set overlap current selecting volume.
        Standard_EXPORT virtual Standard_Boolean Matches(SelectBasics_SelectingVolumeManager &theMgr,
                                                         SelectBasics_PickResult &thePickResult) Standard_OVERRIDE;

    private:
        TopoDS_Face myFace;
        int mySubshapeIndex = -1;
    };

} // namespace ViewShape

#endif // SENSITIVEPOLYFACE_HEADERFILE
