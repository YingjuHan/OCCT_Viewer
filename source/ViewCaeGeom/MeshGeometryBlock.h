#ifndef _MESHGEOMETRYBLOCK_HEADERFILE
#define _MESHGEOMETRYBLOCK_HEADERFILE

#include <ViewCaeGeom/MeshGeometry.h>

#include <Graphic3d_ClipPlane.hxx>

namespace ViewCaeGeom {
//! Status for MeshData block node.
enum class MeshDataGeomBlockStatus { Enabled, Disabled, Partial };

//! Class that defines MeshData block node with enabling status.
class MeshDataGeomBlock : public RWCae::MeshDataBlock {
public:
    //! Constructor.
    Standard_EXPORT explicit MeshDataGeomBlock(const TCollection_AsciiString& theName);

    //! Constructor.
    Standard_EXPORT explicit MeshDataGeomBlock(const Handle(RWCae::MeshDataBlock)& theBlock);

    //! Destructor.
    Standard_EXPORT virtual ~MeshDataGeomBlock();

    //! Return status of the block node.
    MeshDataGeomBlockStatus Status() const { return myStatus; }

    //! Set status of the block node.
    void SetStatus(MeshDataGeomBlockStatus theStatus) { myStatus = theStatus; }

protected:
    MeshDataGeomBlockStatus myStatus = MeshDataGeomBlockStatus::Enabled;
};

//! Mesh geometry for filtering mesh nodes/elements by MeshData blocks.
class MeshGeometryBlock : public MeshGeometry {
public:
    //! Constructor.
    Standard_EXPORT MeshGeometryBlock(const Handle(MeshBvhData)&            theBvhData,
                                      const Handle(RWCae::MeshDataBlock)& theBlocks);

    //! Destructor.
    Standard_EXPORT virtual ~MeshGeometryBlock();

    //! Return geometry type.
    virtual MeshGeometryType Type() const override { return MeshGeometryType::Block; }

    //! Return tree of MeshData block nodes with enabled status.
    Standard_EXPORT Handle(RWCae::MeshDataBlock) GetEnabled() const;

    //! Check if all nodes in the tree have enabled status.
    bool IsAllEnabled() const { return myBlocks->Status() == MeshDataGeomBlockStatus::Enabled; }

    //! Set enabling status for specified nodes and update status of other nodes.
    Standard_EXPORT bool SetEnabled(const Handle(RWCae::MeshDataBlock)& theBlocks, bool theEnabled);

    //! Set enabling status only for specified nodes and update status of other nodes.
    Standard_EXPORT bool SetOnlyEnabled(const Handle(RWCae::MeshDataBlock)& theBlocks);

    //! Generate a new property from base property for mesh data for this mesh geometry.
    Standard_EXPORT virtual Handle(RWCae::MeshNVecProperty) CalculateProperty(
        const Handle(RWCae::MeshNVecProperty)&  theProp,
        const MeshGeomElem*                       theElem  = nullptr,
        const std::vector<RWCae::MeshEntityId>* theNodes = nullptr) const override
    {
        return genPropValues(theProp, theElem, theNodes);
    }

    //! Dump geometry parameters to Json.
    Standard_EXPORT virtual void ToJson(RWBase::JsonHelper::Writer& theJson) override;

    //! Restore geometry parameters from Json.
    Standard_EXPORT virtual bool FromJson(const RWBase::JsonHelper::Value& theJson,
                                          const Handle(RWCae::MeshData)&   theMeshData,
                                          int                                theTime,
                                          bool&                              theIsChanged) override;

    //! Return true if this mesh geometry can modify mesh data of parent mesh geometries.
    virtual bool IsModifier() const override { return true; }

protected:
    //! Set status of node and its children.
    Standard_EXPORT void updateChildrenStatus(MeshDataGeomBlock* theNode, MeshDataGeomBlockStatus theStatus);

    //! Update status of node's parents.
    Standard_EXPORT void updateParentsStatus(const MeshDataGeomBlock* theNode);

    //! Calculate mesh data based on parent mesh geometries.
    Standard_EXPORT virtual void generateMeshData() override {}

    //! Apply accept element modifier.
    Standard_EXPORT virtual bool acceptElement(const Handle(RWCae::MeshData)& theMeshData,
                                               const RWCae::MeshEntityId      theElemIdx,
                                               const MeshGeometry*              theMeshGeom) const override;

    //! Apply accept node modifier.
    Standard_EXPORT virtual bool acceptNode(const RWCae::MeshEntityId theNodeIdx,
                                            const MeshGeometry*         theMeshGeom,
                                            const Graphic3d_Vec3&       theNode) const override;

protected:
    Handle(MeshDataGeomBlock) myBlocks;
};

} // namespace

#endif // _MESHGEOMETRYBLOCK_HEADERFILE
