
#ifndef _SimMeshDraw_DrawableMesh_HeaderFile
#define _SimMeshDraw_DrawableMesh_HeaderFile

#include <Draw_Drawable3D.hxx>
#include <Standard_TypeDef.hxx>

class Draw_Display;
class SimMesh_Model;

//! Defines drawable mesh wrapping for SimMesh_Model
class SimMeshDraw_DrawableMesh : public Draw_Drawable3D {
public:
    Standard_EXPORT SimMeshDraw_DrawableMesh(const Handle(SimMesh_Model)& theModel);

    Standard_EXPORT void DrawOn(Draw_Display& theDisp) const Standard_OVERRIDE;

    Standard_EXPORT void Whatis(Draw_Interpretor& theI) const Standard_OVERRIDE;

    Standard_EXPORT void Dump(Standard_OStream& theS) const Standard_OVERRIDE;

    Standard_EXPORT Handle(Draw_Drawable3D) Copy() const Standard_OVERRIDE;

    const Handle(SimMesh_Model)& GetModel() const { return myModel; }

    DEFINE_STANDARD_RTTI_INLINE(SimMeshDraw_DrawableMesh, Draw_Drawable3D)

private:
    Handle(SimMesh_Model) myModel;
};

#endif // _SimMeshDraw_DrawableMesh_HeaderFile
