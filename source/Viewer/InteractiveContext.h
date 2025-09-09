#ifndef _INTERACTIVECONTEXT_HEADERFILE
#define _INTERACTIVECONTEXT_HEADERFILE

#include "IManager.h"
#include "Utils/ObjectId.h"
#include "Utils/VecN.h"

#include <RWBase/IPromise.h>

#include <ViewBase/InteractiveOwner.h>

#include <AIS_DataMapOfIOStatus.hxx>

namespace Viewer {
//! Class provides public interface for managing Interactive Objects.
class InteractiveContext : public IManager {
public:
    // id of undefined view / all views
    static constexpr int UNDEFINED_VIEW_ID = -2;

public:
    //! Remove object with specified Id.
    Standard_EXPORT static bool RemoveObject(const Handle(AIS_InteractiveContext)& theCtx, const ObjectId& theObjId);

    //! Load deferred data.
    Standard_EXPORT static bool LoadData(const Handle(AIS_InteractiveContext)& theCtx,
                                         const ObjectId&                       theObjId,
                                         RWBase::IPromise*                   theCallback);

    //! Generate unique id.
    Standard_EXPORT static ObjectId GenerateId(const Handle(AIS_InteractiveContext)& theCtx, const ObjectId& theBase);

public:
    //! Main constructor
    Standard_EXPORT explicit InteractiveContext(WasmViewer* theViewer);

    //! Return TRUE if object with specified Id exists.
    Standard_EXPORT bool IsAlive(const ObjectId& theObjId) const;

    //! Return object type.
    Standard_EXPORT std::string GetType(const ObjectId& theObjId) const;

    //! Return TRUE if deferred geometry data of object has been already loaded.
    Standard_EXPORT bool IsLoaded(const ObjectId& theObjId) const;

    //! Return TRUE if object with specified Id is in enabled state.
    Standard_EXPORT bool IsEnabled(const ObjectId& theObjId) const;

    //! Set enabled state.
    Standard_EXPORT void SetEnabled(const ObjectId& theObjId, const bool theToEnable);

    //! Return TRUE if object with specified Id is in visible state.
    Standard_EXPORT bool IsVisible(const ObjectId& theObjId) const;

    //! Set visible state.
    Standard_EXPORT void SetVisible(const ObjectId& theObjId, const bool theToShow);

    //! Return visible state within specific View.
    Standard_EXPORT bool IsVisibleInView(const ObjectId& theObjId, const int theViewId) const;

    //! Return View id where object has been set to be visible or -2 if undefined or displayed in multiple views.
    Standard_EXPORT int GetVisibleInView(const ObjectId& theObjId) const;

    //! Set object visible only within specific View or visible all views (-2).
    Standard_EXPORT void SetVisibleInView(const ObjectId& theObjId, const int theViewId);

    //! Return object corner (Viewer::ViewCorner); has no meaning for 3D objects.
    Standard_EXPORT int GetViewCorner(const ObjectId& theObjIdOld) const;

    //! Set view corner (Viewer::ViewCorner).
    Standard_EXPORT void SetViewCorner(const ObjectId& theObjIdOld, int theCorner);

    //! Return object corner offset; has no meaning for 3D objects.
    Standard_EXPORT Vec2 GetViewCornerOffset(const ObjectId& theObjIdOld) const;

    //! Set view corner offset.
    Standard_EXPORT void SetViewCornerOffset(const ObjectId& theObjIdOld, const Vec2& theOffset);

    //! Return object AABB.
    Standard_EXPORT BndBox3 GetMinMax(const ObjectId& theObjIdOld) const;

    //! Return a value that is added to the default displaying/selection object's priority.
    Standard_EXPORT int GetPriorityModifier(const ObjectId& theObjId) const;

    //! Set a value that is added to the default displaying/selection object's priority.
    Standard_EXPORT void SetPriorityModifier(const ObjectId& theObjId, const int theModifier);

    //! Infinite presentation flag.
    Standard_EXPORT bool IsInfiniteState(const ObjectId& theObjId) const;

    //! Set infinite presentation flag.
    Standard_EXPORT void SetInfiniteState(const ObjectId& theObjId, const bool theIsInf);

    //! Remove object with specified Id.
    Standard_EXPORT bool removeObject(const ObjectId& theObjId);

    //! Remove all named objects.
    Standard_EXPORT bool RemoveAll();

    //! Return ids of all named objects.
    //! @param[in] theIdPrefix optional id prefix that should present in returned named object
    Standard_EXPORT std::string GetObjectsInside(const std::string& theIdPrefix = "") const;

    //! Return id of last dragged object.
    Standard_EXPORT std::string GetLastDraggedObject() const;

    //! Make a copy of existing object.
    //! @param[in] theObjIdOld id of existing object
    //! @param[in] theObjIdNew id of object to create or "" if id should be generated
    //! @return id of created object or "" on failure
    Standard_EXPORT ObjectId Clone(const ObjectId& theObjIdOld, const ObjectId& theObjIdNew);

    //! Load deferred data.
    Standard_EXPORT bool loadData(const ObjectId& theObjId, RWBase::IPromise* theCallback);

    //! Generate unique id of not yet existing object.
    Standard_EXPORT ObjectId generateId(const ObjectId& theBase) const;

public:
    //! Remove object with specified Id.
    Standard_EXPORT static bool RemoveObject(const Handle(AIS_InteractiveContext)& theCtx,
                                             const Handle(AIS_InteractiveObject)&  theObject);
};

} // namespace

#endif // _INTERACTIVECONTEXT_HEADERFILE

