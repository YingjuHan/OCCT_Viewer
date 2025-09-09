#ifndef SENSITIVEPOLYEDGE_HEADERFILE
#define SENSITIVEPOLYEDGE_HEADERFILE

#include <ViewShape/SensitiveShape.h>

#include <Prs3d_Drawer.hxx>
#include <Select3D_SensitivePoly.hxx>
#include <SelectMgr_Selection.hxx>
#include <TopoDS_Edge.hxx>

namespace ViewShape
{

    //! Sensitive for picking tessellated TopoDS_Edge.
    //! This class extends Select3D_SensitivePoly for storing originating edge.
    class SensitivePolyEdge : public Select3D_SensitivePoly, public SensitiveShape
    {
    public:
        //! Create edge sensitive.
        Standard_EXPORT static Handle(SensitivePolyEdge) GetEdgeSensitive(const TopoDS_Edge &theEdge,
                                                                          const Handle(SelectMgr_EntityOwner) & theOwner,
                                                                          const Handle(Prs3d_Drawer) & theDrawer,
                                                                          const double theDefl,
                                                                          const Handle(SelectMgr_Selection) & theTmpSel);

    public:
        //! Main constructor.
        Standard_EXPORT SensitivePolyEdge(const Handle(SelectMgr_EntityOwner) & theOwnerId,
                                          const TopoDS_Edge &theEdge,
                                          const Handle(TColgp_HArray1OfPnt) & thePoints);

        //! Destructor.
        Standard_EXPORT virtual ~SensitivePolyEdge();

        //! Return shape.
        virtual TopoDS_Shape GetShape() const Standard_OVERRIDE { return myEdge; }

        //! Return shape index.
        virtual int GetShapeIndex() const Standard_OVERRIDE { return mySubshapeIndex; }

        //! Set shape index.
        void SetShapeIndex(int theIndex) { mySubshapeIndex = theIndex; }

        //! Return edge.
        const TopoDS_Edge &GetEdge() const { return myEdge; }

        //! Checks whether one or more entities of the set overlap current selecting volume.
        Standard_EXPORT virtual Standard_Boolean Matches(SelectBasics_SelectingVolumeManager &theMgr,
                                                         SelectBasics_PickResult &thePickResult) Standard_OVERRIDE;

    private:
        TopoDS_Edge myEdge;
        int mySubshapeIndex = -1;
    };

} // namespace

#endif // SENSITIVEPOLYEDGE_HEADERFILE