#ifndef SCWEBVIEWER_SELECTIONMANAGER_HEADERFILE
#define SCWEBVIEWER_SELECTIONMANAGER_HEADERFILE

#include "IManager.h"
#include "Utils/ObjectId.h"
#include "Utils/SelectionMeshEntity.h"
#include "Utils/SelectionShapeElems.h"
#include "Utils/VecN.h"

namespace Viewer {
//! Represents class with selection API.
class SelectionManager : public IManager {
public:
    //! Represents constructor
    explicit SelectionManager(WasmViewer* theViewer);

    //! Return last picked point for specified object.
    Viewer::Vec3 GetLastPickedPoint(const Viewer::ObjectId& theId) const;

    //! Return the list of selected objects.
    std::string GetSelectedObjects() const;

    //! Select Objects.
    //! @param[in] theNames the names to select
    //! @param[in] theToLevelUp implicitly handle level up selection of assemblies
    void SetSelectedObjects(const std::string& theNames, bool theToLevelUp);

    //! Get multiselection mode
    bool ToAllowMultiselection() const { return myToAllowMultiselection; }

    //! Set multiselection mode
    Standard_EXPORT void SetAllowMultiselection(bool theIsMultiselection);

public:
    //! Return selection mode for specified object.
    Standard_EXPORT int getSelectionMode(const Viewer::ObjectId& theId) const;

    //! Set selection mode for objects.
    //! @param[in] theNames the objects names to change selection modes.
    //! @param[in] theMode the selection mode to be set.
    Standard_EXPORT void setSelectionMode(const std::string& theNames, const int theMode);

    //! Detected object information.
    struct DetectedObjectInfo {
        Viewer::ObjectId objectId     = Viewer::ObjectId();
        int                   partId       = -1;
        int                   subPartId    = -1;
        Vec3                  pickedPoint  = Vec3::InvalidPoint();
        Vec3                  pickedNormal = Vec3::InvalidPoint();
    };

    //! Return the last detected object.
    Standard_EXPORT DetectedObjectInfo getLastDetectedObject() const;

    //! Reset selection of all previously selected objects.
    Standard_EXPORT void resetSelection();

public:
    //! Return the detected shape element for specified shape object.
    Standard_EXPORT DetectedShapeElem getDetectedShapeElem(const Viewer::ObjectId& theId) const;

    //! Return list of selected shape elements for specified shape object.
    Standard_EXPORT std::string getSelectedShapeElems(const Viewer::ObjectId& theId, const int theElemType) const;

    //! Set list of selected shape elements for specified shape object.
    Standard_EXPORT void setSelectedShapeElems(const Viewer::ObjectId&            theId,
                                               const Viewer::SelectionShapeElems& theSelectedElems);

    //! Return list of disabled shape elements for specified shape object.
    //! @param[in] theId shape object to fetch
    //! @param[in] theElemType subshape type
    //! @param[in] theToReverse flag to reverse index map (return enabled subshapes, when TRUE)
    //! @return list of disabled shape elements
    Standard_EXPORT std::string getDisabledShapeElems(const Viewer::ObjectId& theId,
                                                      int                          theElemType,
                                                      bool                         theToReverse) const;

    //! Set list of disabled shape elements for specified shape object.
    //! @param[in] theId shape object to change
    //! @param[in] theElemType subshape type
    //! @param[in] theToReverse flag to reverse index map (return enabled subshapes, when TRUE)
    //! @param[in] theElems list of shape elements to disable
    Standard_EXPORT void setDisabledShapeElems(const Viewer::ObjectId& theId,
                                               int                          theElemType,
                                               bool                         theToReverse,
                                               const std::string&           theElems);

    //! Return subshape indexes.
    Standard_EXPORT std::string getSubshapesIndexes(const Viewer::ObjectId& theId,
                                                    const int                    theElemType,
                                                    const bool                   theIsFree,
                                                    const int                    theParentType,
                                                    const int                    theParentIndex,
                                                    const int                    theVisibility) const;

public:
    //! Return the detected mesh entity for specified mesh object.
    Standard_EXPORT SelectionMeshEntity getDetectedMeshEntity(const Viewer::ObjectId& theId) const;

    //! Return list of selected mesh entities for specified mesh object.
    Standard_EXPORT std::vector<SelectionMeshEntity> getSelectedMeshEntities(const Viewer::ObjectId& theId) const;

    //! Select sub entity of specified mesh object.
    Standard_EXPORT void setSelectedMeshEntity(const Viewer::ObjectId& theId,
                                               const SelectionMeshEntity&   theEntity);

protected:
    bool myToAllowMultiselection = true;
};

}

#endif // SCWEBVIEWER_SELECTIONMANAGER_HEADERFILE
