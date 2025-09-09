#ifndef SENSITIVECOMPOUND_HEADERFILE
#define SENSITIVECOMPOUND_HEADERFILE

#include <ViewShape/SensitiveShape.h>

#include <Select3D_SensitiveSet.hxx>
#include <SelectMgr_Selection.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <TColStd_PackedMapOfInteger.hxx>
#include <TopoDS_Shape.hxx>

namespace ViewShape
{

    //! Sensitive for picking TopoDS_Shape (as compound of subshapes).
    class SensitiveCompound : public Select3D_SensitiveSet, public SensitiveShape
    {
    public:
        //! Main constructor.
        Standard_EXPORT SensitiveCompound(const TopoDS_Shape &theShape,
                                          const Handle(SelectMgr_EntityOwner) & theOwnerId,
                                          const std::vector<Handle(Select3D_SensitiveEntity)> &theEntities);

        //! Destructor.
        Standard_EXPORT virtual ~SensitiveCompound();

        //! Returns the sensitive.
        const std::vector<Handle(Select3D_SensitiveEntity)> &GetEntities() const { return myEntities; }

        //! Returns last detected (topmost) entity.
        Standard_EXPORT Handle(Select3D_SensitiveEntity) GetLastDetected() const;

        //! Returns indices of last detected elements of rubber-band test.
        const TColStd_PackedMapOfInteger &GetLastDetectedMap() const { return myDetectedMap; }

        //! Return shape.
        virtual TopoDS_Shape GetShape() const Standard_OVERRIDE { return myShape; }

        //! Return shape index.
        virtual int GetShapeIndex() const Standard_OVERRIDE { return mySubshapeIndex; }

        //! Set shape index.
        void SetShapeIndex(int theIndex) { mySubshapeIndex = theIndex; }

    public:
        //! Returns the amount of sub-entities.
        virtual Standard_Integer NbSubElements() const Standard_OVERRIDE { return (int)myEntities.size(); }

        //! Returns connected entity.
        Standard_EXPORT virtual Handle(Select3D_SensitiveEntity) GetConnected() Standard_OVERRIDE;

        //! Sets the owner for all entities in shape.
        Standard_EXPORT virtual void Set(const Handle(SelectMgr_EntityOwner) & theOwnerId) Standard_OVERRIDE;

        //! Returns bounding box of the shape.
        //! If location transformation is set, it will be applied.
        Standard_EXPORT virtual Select3D_BndBox3d BoundingBox() Standard_OVERRIDE;

        //! Returns center of the shape.
        //! If location transformation is set, it will be applied
        virtual gp_Pnt CenterOfGeometry() const Standard_OVERRIDE { return myCenter; }

        //! Returns the length of vector of sensitive entities.
        virtual Standard_Integer Size() const Standard_OVERRIDE { return (int)myEntities.size(); }

        //! Returns bounding box of sensitive entity with index theIdx.
        virtual Select3D_BndBox3d Box(const Standard_Integer theIdx) const Standard_OVERRIDE
        {
            const int aSensIdx = myEntityIndexes[theIdx];
            return myEntities[aSensIdx]->BoundingBox();
        }

        //! Returns geometry center of sensitive entity index theIdx in
        //! the vector along the given axis theAxis.
        Standard_EXPORT virtual Standard_Real Center(const Standard_Integer theIdx,
                                                     const Standard_Integer theAxis) const Standard_OVERRIDE;

        //! Swaps items with indexes theIdx1 and theIdx2 in the vector.
        virtual void Swap(const Standard_Integer theIdx1, const Standard_Integer theIdx2) Standard_OVERRIDE
        {
            std::swap(myEntityIndexes[theIdx1], myEntityIndexes[theIdx2]);
        }

        //! Checks whether the sensitive entity is overlapped by current selecting volume.
        Standard_EXPORT virtual Standard_Boolean Matches(SelectBasics_SelectingVolumeManager &theMgr,
                                                         SelectBasics_PickResult &thePickResult) Standard_OVERRIDE;

    protected:
        //! Checks whether the entity with index theIdx overlaps the current selecting volume.
        Standard_EXPORT virtual Standard_Boolean overlapsElement(SelectBasics_PickResult &thePickResult,
                                                                 SelectBasics_SelectingVolumeManager &theMgr,
                                                                 Standard_Integer theElemIdx,
                                                                 Standard_Boolean theIsFullInside) Standard_OVERRIDE;

        //! Checks whether the entity with index theIdx is inside the current selecting volume.
        Standard_EXPORT virtual Standard_Boolean elementIsInside(SelectBasics_SelectingVolumeManager &theMgr,
                                                                 Standard_Integer theElemIdx,
                                                                 Standard_Boolean theIsFullInside) Standard_OVERRIDE;

        //! Calculates distance from the 3d projection of used-picked screen point to center of the geometry.
        virtual Standard_Real distanceToCOG(SelectBasics_SelectingVolumeManager &theMgr) Standard_OVERRIDE
        {
            return theMgr.DistToGeometryCenter(myCenter);
        }

        //! Return TRUE if shape type selection mode is enabled.
        Standard_EXPORT bool isActiveShapeType(int theSelMode) const;

        //! Return TRUE if subshape is disabled by filter.
        Standard_EXPORT bool isDisabledSubshape(const Handle(Select3D_SensitiveEntity) & theSens) const;

        //! Calculate picked point (similar to SelectMgr_ViewerSelector::updatePoint3d()).
        Standard_EXPORT void updatePickedPoint(SelectBasics_PickResult &thePickResult,
                                               const Handle(Select3D_SensitiveEntity) & theEntity);

        //! Check detected element.
        Standard_EXPORT bool checkDetectedElem(const Handle(Select3D_SensitiveEntity) & theSens,
                                               const SelectBasics_PickResult &thePickResult) const;

    private:
        TopoDS_Shape myShape;                                     //!< shape
        std::vector<Handle(Select3D_SensitiveEntity)> myEntities; //!< Vector of sub-entities
        std::vector<Standard_Integer> myEntityIndexes;            //!< Indexes of entities for BVH build
        gp_Pnt myCenter;                                          //!< Center of the whole wire
        mutable Select3D_BndBox3d myBndBox;                       //!< Bounding box of the whole wire
        TColStd_PackedMapOfInteger myDetectedMap;                 //!< indices of last detected elements of rubber-band test

        SelectBasics_PickResult myTmpPick;
        int myTmpDetectedIdx = -1;
        int myTmpSens = 0;
        int mySubshapeIndex = -1;
    };

} // namespace

#endif // RWSHAPE_SENSITIVECOMPOUND_HEADERFILE
