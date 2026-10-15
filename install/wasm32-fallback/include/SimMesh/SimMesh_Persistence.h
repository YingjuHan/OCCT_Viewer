#ifndef SIMMESH_PERSISTENCE_HEADERFILE
#define SIMMESH_PERSISTENCE_HEADERFILE

#include <Standard_Handle.hxx>
#include <Standard_TypeDef.hxx>

class SimMesh_Model;

//! The set of static methods to load/save a mesh model from/to a file.
class SimMesh_Persistence {
public:
    //! Version of persistence.
    static constexpr int32_t CurrentVersion = 1;

public:
    //! Saves a mesh model to a file
    Standard_EXPORT static bool Save(const Handle(SimMesh_Model)& theModel, const char* theFileName);

    //! Loads a model from a file
    Standard_EXPORT static Handle(SimMesh_Model) Load(const char* theFileName);
};

#endif