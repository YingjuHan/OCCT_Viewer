#ifndef _IMANAGER_HEADERFILE
#define _IMANAGER_HEADERFILE

#include "Utils/ColorTools.h"
#include "Utils/ObjectId.h"

#include <ViewBase/InteractiveOwner.h>
#include <ViewBase/IInteractiveObject.h>

#include <TColStd_HPackedMapOfInteger.hxx>

namespace Viewer {

DEFINE_STANDARD_HANDLE(WasmViewer, Dummy); // forward declared class due to cyclic dependency

//! Class provides public interface for managing some properties of WasmViewer.
class IManager : public Standard_Transient {
public:
    static constexpr char THE_LIST_SEPARATOR = '\n';

    //! Format ids from map into string.
    static void IdsMapToString(std::string& theListStr, const TColStd_PackedMapOfInteger& theMap);

    //! Format ids from map into string.
    static void IdsMapToString(std::string& theListStr, const Handle(TColStd_HPackedMapOfInteger)& theMap)
    {
        if (!theMap.IsNull()) {
            IdsMapToString(theListStr, theMap->Map());
        }
    }

    //! Parse ids list into integer map.
    static Handle(TColStd_HPackedMapOfInteger) IdsMapFromString(const std::string& theListStr);

public:
    //! Main constructor
    explicit IManager(WasmViewer* theViewer);

    //! Find owner by id.
    Handle(ViewBase::InteractiveOwner) FindPrsOwner(const ObjectId& theObjId) const;

    //! Find owner by id.
    template<class T>
    bool FindPrsOwner(opencascade::handle<T>& theObject, const ObjectId& theObjId) const
    {
        Handle(ViewBase::InteractiveOwner) anObj = FindPrsOwner(theObjId);
        if (!anObj.IsNull()) {
            theObject = opencascade::handle<T>::DownCast(anObj);
            return !theObject.IsNull();
        }
        return false;
    }

    //! Find owner by id.
    template<class T1, class T2>
    bool FindPrsOwnerAndObject(opencascade::handle<T1>& theOwner,
                               opencascade::handle<T2>& theObject,
                               const ObjectId&          theObjId) const
    {
        Handle(ViewBase::InteractiveOwner) anOwner = FindPrsOwner(theObjId);
        if (!anOwner.IsNull()) {
            theOwner = opencascade::handle<T1>::DownCast(anOwner);
            if (theOwner.IsNull()) {
                return false;
            }

            theObject = opencascade::handle<T2>::DownCast(anOwner->Selectable());
            return !theObject.IsNull();
        }
        return false;
    }

    //! Find AIS object by id.
    Handle(AIS_InteractiveObject) FindAisObject(const ObjectId& theObjId) const;

    //! Find owner by id.
    template<class T>
    bool FindAisObject(opencascade::handle<T>& theObject, const ObjectId& theObjId) const
    {
        Handle(AIS_InteractiveObject) anObj = FindAisObject(theObjId);
        if (!anObj.IsNull()) {
            theObject = opencascade::handle<T>::DownCast(anObj);
            return !theObject.IsNull();
        }
        return false;
    }

public:
    //! Find view by id.
    Standard_EXPORT Handle(V3d_View) findView(int theViewId) const;

    //! Invalidate viewer content.
    Standard_EXPORT void invalidateViewer(const Handle(AIS_InteractiveObject)& theObj);

protected:
    WasmViewer* myWebViewer; //!< back link to the viewer object
};

} // namespace

#endif // _IMANAGER_HEADERFILE