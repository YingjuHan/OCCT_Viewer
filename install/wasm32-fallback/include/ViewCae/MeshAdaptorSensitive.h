#ifndef MESHADAPTORSENSITIVE_HEADERFILE
#define MESHADAPTORSENSITIVE_HEADERFILE

#include <ViewBase/InteractiveOwner.h>

#include <ViewCae/MeshPresentation.h>

#include <RWCae/MeshAdaptor.h>

#include <Select3D_SensitiveBox.hxx>
#include <Select3D_SensitiveSet.hxx>
#include <TColStd_DataMapOfIntegerInteger.hxx>
#include <TColStd_HArray1OfInteger.hxx>

namespace ViewCae {
//! Sensitive for picking CAE mesh defined by @link RWCae::MeshData @endlink
//! within @link ViewCae::InteractiveCaeMesh @endlink interactive object.
class MeshAdaptorSensitive : public Select3D_SensitiveSet {
public:
    //! Sensitive factor.
    enum {
        DefaultSensiveFactor0D = 12,
        DefaultSensiveFactor1D = 3,
        DefaultSensiveFactor2D = 0,
    };

    //! Return default sensitive factor.
    template<class Elem_T>
    static int DefaultSensitiveFactor()
    {
        if (Elem_T::Type == RWCae::MeshElemType::Elem0d) {
            return DefaultSensiveFactor0D;
        }
        return DefaultSensiveFactor2D;
    }

public:
    //! Constructor.
    Standard_EXPORT MeshAdaptorSensitive(const Handle(SelectMgr_EntityOwner)& theOwnerId,
                                         ViewCaeGeom::MeshGeometry*          theMeshGeom,
                                         MeshPresentation*                    theMeshPrs);

public:
    //! Returns the amount of nodes in triangulation
    virtual Standard_Integer NbSubElements() const Standard_OVERRIDE { return 1; }

    //! Returns the amount of sub-entities of the complex entity
    virtual Standard_Integer Size() const Standard_OVERRIDE
    {
        return !myBVHPrimIndexes.IsNull() ? myBVHPrimIndexes->Size() : 0;
    }

    //! Returns geometry center of sensitive entity index theIdx along the given axis theAxis
    virtual Standard_Real Center(const Standard_Integer theIdx, const Standard_Integer theAxis) const Standard_OVERRIDE
    {
        const Select3D_BndBox3d& aBox    = Box(theIdx);
        SelectMgr_Vec3           aCenter = (aBox.CornerMin() + aBox.CornerMax()) * 0.5;
        return theAxis == 0 ? aCenter.x() : (theAxis == 1 ? aCenter.y() : aCenter.z());
    }

    //! Swaps items with indexes theIdx1 and theIdx2
    virtual void Swap(const Standard_Integer theIdx1, const Standard_Integer theIdx2) Standard_OVERRIDE
    {
        Standard_Integer anElemIdx1            = myBVHPrimIndexes->Value(theIdx1);
        Standard_Integer anElemIdx2            = myBVHPrimIndexes->Value(theIdx2);
        myBVHPrimIndexes->ChangeValue(theIdx1) = anElemIdx2;
        myBVHPrimIndexes->ChangeValue(theIdx2) = anElemIdx1;
    }

    //! Returns bounding box of the whole set.
    //! This method should be redefined in Select3D_SensitiveSet descendants
    virtual Select3D_BndBox3d BoundingBox() Standard_OVERRIDE
    {
        if (!myBndBox.IsValid()) {
            computeBoundingBox();
        }
        return myBndBox;
    }

    //! Returns center of the whole set.
    //! This method should be redefined in Select3D_SensitiveSet descendants
    virtual gp_Pnt CenterOfGeometry() const Standard_OVERRIDE { return myCDG3D; }

    //! Checks whether one or more entities of the set overlap current selecting volume.
    //! Implements the traverse of BVH tree built for the set
    Standard_EXPORT virtual Standard_Boolean Matches(SelectBasics_SelectingVolumeManager& theMgr,
                                                     SelectBasics_PickResult& thePickResult) Standard_OVERRIDE;

    //! Return indices of elements detected with Box/Polyline selectors.
    const TColStd_PackedMapOfInteger& GetDetectedElements() const { return myDetectedIndices; }

    //! Return corresponding mesh geometry.
    const ViewCaeGeom::MeshGeometry* MeshGeom() const { return myMeshGeom; }

    //! Return corresponding mesh presentation.
    const MeshPresentation* MeshPrs() const { return myMeshPrs; }

    //! Compute bounding box.
    virtual void computeBoundingBox() {}

protected:
    //! Calculates distance from the 3d projection of used-picked screen point to center of the geometry
    virtual Standard_Real distanceToCOG(SelectBasics_SelectingVolumeManager& theMgr) Standard_OVERRIDE
    {
        return theMgr.DistToGeometryCenter(myCDG3D);
    }

protected:
    ViewCaeGeom::MeshGeometry*      myMeshGeom;
    MeshPresentation*                myMeshPrs;
    gp_Pnt                           myCDG3D;
    Handle(TColStd_HArray1OfInteger) myBVHPrimIndexes;
    Select3D_BndBox3d                myBndBox;
    TColStd_PackedMapOfInteger       myDetectedIndices;
};

//! Sensitive for elements of CAE mesh.
template<class Elem_T>
class MeshAdaptorSensitiveElement : public MeshAdaptorSensitive {
    // element node indices
    static constexpr int m_n0 = 0, m_n1 = 1, m_n2 = 2, m_n3 = 3;

public:
    //! Constructor.
    MeshAdaptorSensitiveElement(const Handle(SelectMgr_EntityOwner)& theOwnerId,
                                const RWCae::MeshAdaptor<Elem_T>&  theMeshAdaptor,
                                ViewCaeGeom::MeshGeometry*          theMeshGeom,
                                MeshPresentation*                    theMeshPrs)
        : MeshAdaptorSensitive(theOwnerId, theMeshGeom, theMeshPrs)
    {
        Init(theMeshAdaptor);
    }

    //! Constructs a sensitive mesh object.
    void Init(const RWCae::MeshAdaptor<Elem_T>& theMeshAdaptor);

public:
    virtual Handle(Select3D_SensitiveEntity) GetConnected() Standard_OVERRIDE
    {
        Handle(Select3D_SensitiveEntity) aNewEntity =
            new MeshAdaptorSensitiveElement<Elem_T>(myOwnerId, myMeshAdaptor, myMeshGeom, myMeshPrs);
        return aNewEntity;
    }

    //! Returns bounding box of sub-entity with index theIdx in sub-entity list
    virtual Select3D_BndBox3d Box(const Standard_Integer theIdx) const Standard_OVERRIDE;

    //! Return index of the last detected element (if it was selected with Point selector).
    int GetDetectedElement() const
    {
        return myDetectedIdx != -1 ? myBVHPrimIndexes->Value(myDetectedIdx) : myDetectedIdx;
    }

    //! Return corresponding mesh adaptor.
    const RWCae::MeshAdaptor<Elem_T>& Adaptor() const { return myMeshAdaptor; }

protected:
    //! Checks whether the entity with index theIdx is (fully) inside the current selecting volume
    //! @param[in] theMgr selection manager
    //! @param[in] theElemIdx element index within BVH tree to check
    //! @param[in] theIsFullInside when TRUE indicates that entire BVH node
    //!                            is already inside selection volume (in case of rectangle selection);
    //!                            in this case algorithm might skip
    //!                            checking the element and just register it as detected
    virtual Standard_Boolean elementIsInside(SelectBasics_SelectingVolumeManager& theMgr,
                                             Standard_Integer                     theElemIdx,
                                             Standard_Boolean                     theIsFullInside) Standard_OVERRIDE;

    //! Checks whether the entity with index theIdx (partially) overlaps the current selecting volume.
    //! @param[out] thePickResult picking result, should update minimum depth
    //! @param[in] theMgr selection manager
    //! @param[in] theElemIdx element index within BVH tree to check
    //! @param[in] theIsFullInside when TRUE indicates that entire BVH node
    //!                            is already inside selection volume (in case of rectangle selection);
    //!                            in this case algorithm might skip
    //!                             checking the element and just register it as detected
    virtual Standard_Boolean overlapsElement(SelectBasics_PickResult&             thePickResult,
                                             SelectBasics_SelectingVolumeManager& theMgr,
                                             Standard_Integer                     theElemIdx,
                                             Standard_Boolean                     theIsFullInside) Standard_OVERRIDE;

    //! Compute bounding box.
    virtual void computeBoundingBox() Standard_OVERRIDE;

protected:
    //! Return node position by its index and apply offset (if is specified) to it.
    void getNodePosition(Graphic3d_Vec3& theNode, RWCae::MeshEntityId theNodeIndex) const
    {
        myMeshAdaptor.NodePosition(theNode, theNodeIndex);
        myMeshGeom->ModifyNode(theNodeIndex, theNode);
    }

protected:
    RWCae::MeshAdaptor<Elem_T> myMeshAdaptor;
};

//! Sensitive for nodes of CAE mesh.
class MeshAdaptorSensitiveNode : public MeshAdaptorSensitive {
public:
    //! Constructor.
    MeshAdaptorSensitiveNode(const Handle(SelectMgr_EntityOwner)& theOwnerId,
                             ViewCaeGeom::MeshGeometry*          theMeshGeom,
                             MeshPresentation*                    theMeshPrs)
        : MeshAdaptorSensitive(theOwnerId, theMeshGeom, theMeshPrs)
    {
        mySFactor = DefaultSensiveFactor0D;
        Init();
    }

    //! Constructs a sensitive mesh object.
    void Init();

public:
    virtual Handle(Select3D_SensitiveEntity) GetConnected() Standard_OVERRIDE
    {
        Handle(Select3D_SensitiveEntity) aNewEntity = new MeshAdaptorSensitiveNode(myOwnerId, myMeshGeom, myMeshPrs);
        return aNewEntity;
    }

    //! Returns bounding box of sub-entity with index theIdx in sub-entity list
    virtual Select3D_BndBox3d Box(const Standard_Integer theIdx) const Standard_OVERRIDE;

    //! Return index of the last detected element (if it was selected with Point selector).
    int GetDetectedNode() const { return myDetectedIdx != -1 ? myBVHPrimIndexes->Value(myDetectedIdx) : myDetectedIdx; }

protected:
    //! Checks whether the entity with index theIdx (partially) overlaps the current selecting volume.
    //! @param[out] thePickResult picking result, should update minimum depth
    //! @param[in] theMgr selection manager
    //! @param[in] theElemIdx element index within BVH tree to check
    //! @param[in] theIsFullInside when TRUE indicates that entire BVH node
    //!                            is already inside selection volume (in case of rectangle selection);
    //!                            in this case algorithm might skip
    //!                             checking the element and just register it as detected
    virtual Standard_Boolean overlapsElement(SelectBasics_PickResult&             thePickResult,
                                             SelectBasics_SelectingVolumeManager& theMgr,
                                             Standard_Integer                     theElemIdx,
                                             Standard_Boolean                     theIsFullInside) Standard_OVERRIDE;

    //! Checks whether the entity with index theIdx is (fully) inside the current selecting volume
    //! @param[in] theMgr selection manager
    //! @param[in] theElemIdx element index within BVH tree to check
    //! @param[in] theIsFullInside when TRUE indicates that entire BVH node
    //!                            is already inside selection volume (in case of rectangle selection);
    //!                            in this case algorithm might skip
    //!                            checking the element and just register it as detected
    virtual Standard_Boolean elementIsInside(SelectBasics_SelectingVolumeManager& theMgr,
                                             Standard_Integer                     theElemIdx,
                                             Standard_Boolean                     theIsFullInside) Standard_OVERRIDE;

    //! Compute bounding box.
    virtual void computeBoundingBox() Standard_OVERRIDE;

protected:
    //! Return node position by its index and apply offset (if is specified) to it.
    void getNodePosition(Graphic3d_Vec3& theNode, RWCae::MeshEntityId theNodeIndex) const
    {
        theNode = myMeshGeom->GetMeshData()->Nodes()[theNodeIndex];
        myMeshGeom->ModifyNode(theNodeIndex, theNode);
    }
};

//! Sensitive box with detected point in its center.
class MeshSensitiveBox : public Select3D_SensitiveBox {
public:
    //! Constructor.
    MeshSensitiveBox(const Handle(SelectMgr_EntityOwner)& theOwnerId, const Bnd_Box& theBox)
        : Select3D_SensitiveBox(theOwnerId, theBox)
    {
    }

    //! Checks whether the box overlaps current selecting volume
    virtual Standard_Boolean Matches(SelectBasics_SelectingVolumeManager& theMgr,
                                     SelectBasics_PickResult&             thePickResult) override
    {
        if (!Select3D_SensitiveBox::Matches(theMgr, thePickResult)) {
            return false;
        }

        thePickResult.SetDepth(RealLast());
        thePickResult.SetPickedPoint(CenterOfGeometry());
        return true;
    }
};

#ifdef Standard_UNUSED
    // suppress CodeCheck warnings considering template exports as 'forward declarations'
    #define ExportTemplate
#endif

typedef MeshAdaptorSensitiveElement<RWCae::MeshElement0d>             MeshAdaptorSens0d;
typedef MeshAdaptorSensitiveElement<RWCae::MeshElementTriangle>       MeshAdaptorSensTriangle;
typedef MeshAdaptorSensitiveElement<RWCae::MeshElementTriangle2nd>    MeshAdaptorSensTriangle2nd;
typedef MeshAdaptorSensitiveElement<RWCae::MeshElementQuad>           MeshAdaptorSensQuad;
typedef MeshAdaptorSensitiveElement<RWCae::MeshElementQuad2nd>        MeshAdaptorSensQuad2nd;
typedef MeshAdaptorSensitiveElement<RWCae::MeshElementPolygon>        MeshAdaptorSensPolygon;
typedef MeshAdaptorSensitiveElement<RWCae::MeshElementTetrahedron>    MeshAdaptorSensTetrahedron;
typedef MeshAdaptorSensitiveElement<RWCae::MeshElementTetrahedron2nd> MeshAdaptorSensTetrahedron2nd;
typedef MeshAdaptorSensitiveElement<RWCae::MeshElementPyramid>        MeshAdaptorSensPyramid;
typedef MeshAdaptorSensitiveElement<RWCae::MeshElementPyramid2nd>     MeshAdaptorSensPyramid2nd;
typedef MeshAdaptorSensitiveElement<RWCae::MeshElementPrism>          MeshAdaptorSensPrism;
typedef MeshAdaptorSensitiveElement<RWCae::MeshElementPrism2nd>       MeshAdaptorSensPrism2nd;
typedef MeshAdaptorSensitiveElement<RWCae::MeshElementHexahedron>     MeshAdaptorSensHexahedron;
typedef MeshAdaptorSensitiveElement<RWCae::MeshElementHexahedron2nd>  MeshAdaptorSensHexahedron2nd;
typedef MeshAdaptorSensitiveElement<RWCae::MeshElementPolyhedron>     MeshAdaptorSensPolyhedron;

} // namespace

#endif // MESHADAPTORSENSITIVE_HEADERFILE