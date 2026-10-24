
#ifndef _MESHBVHDATA_HEADERFILE
#define _MESHBVHDATA_HEADERFILE

#include <ViewCaeGeom/MeshHBvhBoxSet.h>

#include <RWCae/MeshAdaptor.h>
#include <RWCae/MeshData.h>

#include <Bnd_Tools.hxx>
#include <BVH_BoxSet.hxx>
#include <Graphic3d_ClipPlane.hxx>
#include <memory>
#include <vector>

namespace ViewCaeGeom {
class MeshGeometry; // forward declared class due to cyclic dependency
//! Class for storing mesh BVH box set computed on mesh volume elements.
class MeshBvhData : public Standard_Transient {
public:
    typedef MeshHBvhBoxSet<float, 3, RWCae::MeshEntityId>  MeshBvhBoxSet;
    typedef MeshHBvhBoxSet<double, 1, RWCae::MeshEntityId> MeshBvhValueSet;

public:
    //! Empty constructor.
    Standard_EXPORT MeshBvhData();

    //! Return computed BVH box set for specified mesh geometry.
    Standard_EXPORT const Handle(MeshBvhBoxSet)& BvhBoxSet(const Handle(MeshGeometry)& theMeshGeom);
    //! Immutable coordinates prepared with a modified source BVH. Invalidated
    //! by exactly the same geometry/deformation lifetime as that BVH.
    Standard_EXPORT std::shared_ptr<const std::vector<Graphic3d_Vec3>> PreparedNodePositions(
        const Handle(MeshGeometry)& theMeshGeom) const;

    //! Remove BVH box set for specified mesh geometry (if it isn't used anymore).
    Standard_EXPORT void RemoveBvhBoxSet(const Handle(MeshGeometry)& theMeshGeom);

    //! Return BVH box set computed on mesh property values for specified mesh geometry.
    Standard_EXPORT const Handle(MeshBvhValueSet)& BvhValueSet(const Handle(MeshGeometry)& theMeshGeom,
                                                               const Handle(RWCae::MeshScalarPropertyBase)& theProp);

    //! Remove BVH box set computed on mesh property values for specified mesh geometry (if it isn't used anymore).
    void RemoveBvhValueSet(const Handle(MeshGeometry)&                    theMeshGeom,
                           const Handle(RWCae::MeshScalarPropertyBase)& theProp);

    //! Mark BVH value set to update for specified mesh geometry.
    void SetToUpdateBvhValueSet(const Handle(MeshGeometry)&                    theMeshGeom,
                                const Handle(RWCae::MeshScalarPropertyBase)& theProp);

    //! Remove all computed BVH.
    void Clear()
    {
        myBvhBoxSets.Clear();
        myNodePositions.Clear();
        myBvhValueSets.Clear();
    }

protected:
    //! Calculate BVH box set for mesh elements.
    Standard_EXPORT void fillBvhBoxSet(const Handle(MeshGeometry)& theMeshGeom, Handle(MeshBvhBoxSet)& theBvhBoxSet);

    //! Find computed Bvh box set for mesh geometry with the same mesh data.
    Standard_EXPORT bool getBvhBoxSet(const Handle(MeshGeometry)& theMeshGeom, Handle(MeshBvhBoxSet)& theBvhBoxSet);

    //! Calculate BVH value set for mesh elements.
    Standard_EXPORT void fillBvhValueSet(const Handle(MeshGeometry)&                    theMeshGeom,
                                         const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                         Handle(MeshBvhValueSet)&                       theBvhValueSet);

    //! Calculate element property value range.
    Standard_EXPORT void fillBvhValueElementRange(const Handle(RWCae::MeshData)&               theMeshData,
                                                  RWCae::MeshEntityId                          theElemIdx,
                                                  const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                                  Bnd_Range&                                     theRange);

    //! Calculate element property value range for nodevec property.
    Standard_EXPORT Bnd_Range getNodeVecValueRange(const RWCae::MeshElements&                   theElems,
                                                   RWCae::MeshEntityId                          theElemIdx,
                                                   RWCae::MeshElemEntityOffset                  theNodeBegin,
                                                   RWCae::MeshElemEntityOffset                  theNodeEnd,
                                                   const Handle(RWCae::MeshScalarPropertyBase)& theProp);

    //! Calculate element property value range for scalar elemental property.
    Standard_EXPORT Bnd_Range getElementalValueRange(RWCae::MeshEntityId                          theElemIdx,
                                                     const Handle(RWCae::MeshScalarPropertyBase)& theProp);

    //! Calculate element property value range for scalar nodal property.
    Standard_EXPORT Bnd_Range getNodalValueRange(const RWCae::MeshElements&                   theElems,
                                                 RWCae::MeshElemEntityOffset                  theNodeBegin,
                                                 RWCae::MeshElemEntityOffset                  theNodeEnd,
                                                 const Handle(RWCae::MeshScalarPropertyBase)& theProp);

    //! Find computed Bvh value set for mesh geometry with the same mesh data and property.
    Standard_EXPORT bool getBvhValueSet(const Handle(MeshGeometry)&                    theMeshGeom,
                                        const Handle(RWCae::MeshScalarPropertyBase)& theProp,
                                        Handle(MeshBvhValueSet)&                       theBvhValueSet);

protected:
    //! Struct for storing mesh geometry and mesh scalar property.
    struct ValueData {
        Handle(MeshGeometry)                    MeshGeom;
        Handle(RWCae::MeshScalarPropertyBase) Prop;

        ValueData() {}

        ValueData(const Handle(MeshGeometry)& theMeshGeom, const Handle(RWCae::MeshScalarPropertyBase)& theProp)
            : MeshGeom(theMeshGeom),
              Prop(theProp)
        {
        }

        //! Hash value, for Map interface.
        static int HashCode(const ViewCaeGeom::MeshBvhData::ValueData& theElem, const int theUpper)
        {
            unsigned int aHashCode = 0;

            aHashCode = aHashCode ^ (unsigned int)::HashCode(theElem.MeshGeom, theUpper);
            aHashCode = aHashCode ^ (unsigned int)::HashCode(theElem.Prop, theUpper);
            return (static_cast<int>(aHashCode & 0x7fffffff) % theUpper) + 1;
        }

        //! Matching two instances, for Map interface.
        static bool IsEqual(const ViewCaeGeom::MeshBvhData::ValueData& theElem1,
                            const ViewCaeGeom::MeshBvhData::ValueData& theElem2)
        {
            return theElem1.MeshGeom == theElem2.MeshGeom && theElem1.Prop == theElem2.Prop;
        }
    };

    NCollection_DataMap<Handle(MeshGeometry), Handle(MeshBvhBoxSet)>   myBvhBoxSets;
    NCollection_DataMap<Handle(MeshGeometry), std::shared_ptr<const std::vector<Graphic3d_Vec3>>> myNodePositions;
    NCollection_DataMap<ValueData, Handle(MeshBvhValueSet), ValueData> myBvhValueSets;
};

} // namespace

#endif // _MESHBVHDATA_HEADERFILE
