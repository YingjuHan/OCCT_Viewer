#ifndef RWCAETEST_DRAWABLEMESHDATA_HEADERFILE
#define RWCAETEST_DRAWABLEMESHDATA_HEADERFILE

#include <RWCae/MeshData.h>

#include <ViewBaseTest/ViewerDrawable.h>

#include <AIS_InteractiveContext.hxx>
#include <DBRep_DrawableShape.hxx>
#include <NCollection_List.hxx>

namespace RWCaeTest {

//! Drawable holding CAE mesh data object.
class DrawableMeshData : public DBRep_DrawableShape, public ViewBaseTest::ViewerDrawable {
public:
    //! Constructor.
    Standard_EXPORT explicit DrawableMeshData(const Handle(RWCae::MeshData)& theData);

    //! Destructor.
    Standard_EXPORT virtual ~DrawableMeshData();

    //! Display in AXO viewer.
    Standard_EXPORT virtual void DrawOn(Draw_Display& theDisp) const override;

    //! Print drawable presentation information.
    Standard_EXPORT virtual void Whatis(Draw_Interpretor& theDI) const override;

    //! Return shape data.
    const Handle(RWCae::MeshData)& MeshData() const { return myMeshData; }

    //! Toggle using of test custom geometry for mesh presentations.
    void SetTestCacheGenGeom(bool theToTest) { myToTestCacheGenGeom = theToTest; }

public:
    //! Display drawable presentation in 3D Viewer.
    Standard_EXPORT virtual bool Display(
        const PrsMgr_DisplayStatus           theStatus,
        const Handle(AIS_InteractiveObject)& theOldPrs = Handle(AIS_InteractiveObject)()) const override;

    //! Print drawable presentation information.
    Standard_EXPORT virtual void PrintInfo(Standard_OStream& theStream) const override;

protected:
    Handle(RWCae::MeshData) myMeshData;

    bool myToTestCacheGenGeom = false;
};

}

#endif // RWCAETEST_DRAWABLEMESHDATA_HEADERFILE
