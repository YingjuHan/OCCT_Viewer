

#ifndef SENSITIVEVERTEX_HEADERFILE
#define SENSITIVEVERTEX_HEADERFILE

#include <ViewShape/SensitiveShape.h>

#include <Select3D_SensitivePoint.hxx>
#include <TopoDS_Vertex.hxx>

namespace ViewShape
{

    //! Sensitive for picking tessellated TopoDS_Vertex.
    //! This class extends Select3D_SensitivePoint for storing originating vertex.
    class SensitiveVertex : public Select3D_SensitivePoint, public SensitiveShape
    {
    public:
        //! Main constructor.
        Standard_EXPORT SensitiveVertex(const Handle(SelectMgr_EntityOwner) & theOwnerId, const TopoDS_Vertex &theVert);

        //! Destructor.
        Standard_EXPORT virtual ~SensitiveVertex();

        //! Return shape.
        virtual TopoDS_Shape GetShape() const Standard_OVERRIDE { return myVertex; }

        //! Return shape index.
        virtual int GetShapeIndex() const Standard_OVERRIDE { return mySubshapeIndex; }

        //! Set shape index.
        void SetShapeIndex(int theIndex) { mySubshapeIndex = theIndex; }

        //! Return vertex.
        const TopoDS_Vertex &GetVertex() const { return myVertex; }

        //! Checks whether one or more entities of the set overlap current selecting volume.
        Standard_EXPORT virtual Standard_Boolean Matches(SelectBasics_SelectingVolumeManager &theMgr,
                                                         SelectBasics_PickResult &thePickResult) Standard_OVERRIDE;

    private:
        TopoDS_Vertex myVertex;
        int mySubshapeIndex = -1;
    };

} // namespace

#endif // SENSITIVEVERTEX_HEADERFILE
