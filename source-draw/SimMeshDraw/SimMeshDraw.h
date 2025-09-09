
#ifndef _SimMeshDraw_HeaderFile
#define _SimMeshDraw_HeaderFile

#include <Draw_Interpretor.hxx>
#include <Standard.hxx>
#include <Standard_DefineAlloc.hxx>
#include <Standard_Handle.hxx>

//! Draw plugin for SimMesh.
class SimMeshDraw {
public:
    DEFINE_STANDARD_ALLOC

    //! Loads SimMesh interface commands into the Draw interpretor
    Standard_EXPORT static void InitCommands(Draw_Interpretor& theCommands);

    //! Loads all Draw commands of TKSimMeshDraw. Used for plugin.
    Standard_EXPORT static void Factory(Draw_Interpretor& theDI);
};

#endif // _SimMeshDraw_HeaderFile