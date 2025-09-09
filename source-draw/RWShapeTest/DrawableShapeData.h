#ifndef RWSHAPETEST_DRAWABLESHAPEDATA_HEADERFILE
#define RWSHAPETEST_DRAWABLESHAPEDATA_HEADERFILE

#include <RWShape/ShapeData.h>

#include <ViewBaseTest/ViewerDrawable.h>

#include <AIS_InteractiveContext.hxx>
#include <DBRep_DrawableShape.hxx>
#include <NCollection_List.hxx>

namespace RWShapeTest {

//! Drawable holding shape data object.
class DrawableShapeData : public DBRep_DrawableShape, public ViewBaseTest::ViewerDrawable {
public:
    //! Load deferred data.
    Standard_EXPORT static bool LoadData(const NCollection_List<Handle(DrawableShapeData)>& theObjects,
                                         const bool                                         theIsMulti,
                                         const bool                                         theIsAsync);

public:
    //! Constructor.
    Standard_EXPORT explicit DrawableShapeData(const Handle(RWShape::ShapeData)& theData);

    //! Destructor.
    Standard_EXPORT virtual ~DrawableShapeData();

    //! Display in AXO viewer.
    Standard_EXPORT virtual void DrawOn(Draw_Display& theDisp) const override;

    //! Print drawable presentation information.
    Standard_EXPORT virtual void Whatis(Draw_Interpretor& theDI) const override;

    //! Return shape data.
    const Handle(RWShape::ShapeData)& ShapeData() const { return myShapeData; }

public:
    //! Display drawable presentation in 3D Viewer.
    Standard_EXPORT virtual bool Display(
        const PrsMgr_DisplayStatus           theStatus,
        const Handle(AIS_InteractiveObject)& theOldPrs = Handle(AIS_InteractiveObject)()) const override;

    //! Print drawable presentation information.
    Standard_EXPORT virtual void PrintInfo(Standard_OStream& theStream) const override;

protected:
    Handle(RWShape::ShapeData) myShapeData;
};

}

#endif // RWSHAPETEST_DRAWABLESHAPEDATA_HEADERFILE