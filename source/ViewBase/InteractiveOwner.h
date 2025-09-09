#ifndef INTERACTIVEOWNER_HEADERFILE
#define INTERACTIVEOWNER_HEADERFILE

#include <AIS_InteractiveContext.hxx>
#include <SelectMgr_EntityOwner.hxx>

//! @namespace ViewBase
//! `ViewBase` is a C++ library defining OCCT-based 3D Viewer (expanding AIS package)
//! and common presentation interfaces like `ViewBase::InteractiveOwner`.
namespace ViewBase
{
    //! Min value that can be passed to SelectMgr_EntityOwner::SetPriority method.
    constexpr int MinOwnerSelectionPriority = 0;
    //! Average value that can be passed to SelectMgr_EntityOwner::SetPriority method.
    constexpr int AverageOwnerSelectionPriority5 = 5;
    //! Max value that can be passed to SelectMgr_EntityOwner::SetPriority method.
    constexpr int MaxOwnerSelectionPriority = 9;

    //! Basic owner object for selecting / defining interactive object or it's part.
    //! Each global interactive object in the Viewer is defined by unique (string) identifier.
    class InteractiveOwner : public SelectMgr_EntityOwner
    {
    public:
        //! List of owners.
        typedef NCollection_List<Handle(InteractiveOwner)> OwnerList;

        //! Auxiliary list wrapper for range-based iteration.
        struct OwnerRange
        {
            explicit OwnerRange(const OwnerList::Iterator &theIter) : myIter(theIter) {}
            OwnerList::iterator begin() const { return OwnerList::iterator(myIter); }
            OwnerList::iterator end() const { return OwnerList::iterator(); }

        private:
            OwnerList::Iterator myIter;
        };

        //! Auxiliary tool for iteration over individual (child) owners within Interactive Context.
        class Iterator
        {
        public:
            //! Main constructor.
            explicit Iterator(const AIS_InteractiveContext &theCtx) : myObjIter(theCtx.ObjectIterator()) { Next(); }

            //! Return TRUE if current iterator position is valid.
            bool More() const { return myCurrent != nullptr; }

            //! Return current iterator position.
            ViewBase::InteractiveOwner *Current() const { return myOwnIter.More() ? myOwnIter.Value().get() : myCurrent; }

            //! Return current parent owner.
            ViewBase::InteractiveOwner *CurrentParentOwner() const { return myCurrent; }

            //! Return current Interactive Object.
            const Handle(AIS_InteractiveObject) & CurrentObject() const { return myObjIter.Key(); }

            //! Move to the next owner in context.
            Standard_EXPORT void Next();

        private:
            AIS_DataMapOfIOStatus::Iterator myObjIter;
            ViewBase::InteractiveOwner *myCurrent = nullptr;
            OwnerList::Iterator myOwnIter;
        };

    public:
        //! Find interactive owner by id.
        Standard_EXPORT static Handle(InteractiveOwner) FindPrsOwnerById(const AIS_InteractiveContext &theCtx,
                                                                         const TCollection_AsciiString &theObjId);

        //! Find interactive owner by it's id.
        template <class T>
        static bool FindPrsOwnerById(opencascade::handle<T> &theObject,
                                     const AIS_InteractiveContext &theCtx,
                                     const TCollection_AsciiString &theObjId)
        {
            Handle(InteractiveOwner) anObj = FindPrsOwnerById(theCtx, theObjId);
            if (!anObj.IsNull())
            {
                theObject = opencascade::handle<T>::DownCast(anObj);
                return !theObject.IsNull();
            }
            return false;
        }

        //! Calculate picking tolerance at specified 3D point position and camera orientation.
        Standard_EXPORT static double GetPickTolerance(const Handle(StdSelect_ViewerSelector3d) & theSelector,
                                                       const gp_Pnt &thePoint);

        //! Subparts information.
        struct SelectionPart
        {
            Standard_Integer Id;
            Standard_Integer SubId;

            explicit SelectionPart(Standard_Integer theId = -1, Standard_Integer theSubId = -1) : Id(theId), SubId(theSubId)
            {
            }
        };

    public:
        //! Custom subclass to access protected fields (to be improved in OCCT?).
        class TolViewerSelector : public SelectMgr_ViewerSelector
        {
        public:
            const gp_Pnt &GetCamEye() const { return myCameraEye; }
            const gp_Dir &GetCamDir() const { return myCameraDir; }
            double GetCamScale() const { return myCameraScale; }
        };

    public:
        //! Creates the entity owner.
        Standard_EXPORT explicit InteractiveOwner(const TCollection_AsciiString &theId,
                                                  const SelectionPart &thePart = SelectionPart(),
                                                  const Standard_Integer thePriority = 0);

        //! Destructor.
        Standard_EXPORT virtual ~InteractiveOwner();

        //! Return owner's id.
        const TCollection_AsciiString &Id() const { return myId; }

        //! Return parent.
        virtual const InteractiveOwner *Parent() const { return nullptr; }

        //! Return children iterator.
        virtual OwnerList::Iterator ChildIterator() const { return OwnerList::Iterator(); }

        //! Return children range for iteration.
        OwnerRange Children() const { return OwnerRange(ChildIterator()); }

        //! Find child by id.
        Standard_EXPORT const InteractiveOwner *FindChildById(const TCollection_AsciiString &theObjId) const;

        //! Return selection part.
        const SelectionPart &GetSelectionPart() const { return myPart; }

        //! Set selection part.
        void SetSelectionPart(const SelectionPart &thePart) { myPart = thePart; }

        //! Return suitable Z-Layer for highlighting presentation.
        Standard_EXPORT Graphic3d_ZLayerId GetHighlightZLayer(const Handle(Prs3d_Drawer) & theStyle,
                                                              bool theIsSelection) const;

        //! Return Prs3d_Presentation for selection or highlighting.
        Standard_EXPORT virtual Handle(Prs3d_Presentation) GetHighlightPresentation(
            const Handle(PrsMgr_PresentationManager) & thePrsMgr,
            const Handle(Prs3d_Drawer) & theStyle,
            const bool theIsSelection);

        //! Prepare presentation for selection or highlighting.
        Standard_EXPORT void PrepareHighlightPresentation(const Handle(Prs3d_Presentation) & thePrs,
                                                          const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                                          const Handle(Prs3d_Drawer) & theStyle,
                                                          const bool theIsSelection);

        //! Return presentation for displaying elements using Z Layer different from main presentation.
        Standard_EXPORT Handle(PrsMgr_Presentation) GetLayeredPresentation(
            const Handle(PrsMgr_PresentationManager) & thePrsMgr,
            const int theDispMode,
            const bool theToCreate,
            const Graphic3d_ZLayerId theLayer);

        //! Return presentation for displaying topmost elements on top of main presentation.
        Handle(PrsMgr_Presentation) GetTopmostPresentation(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                                           const int theDispMode,
                                                           const bool theToCreate)
        {
            return GetLayeredPresentation(thePrsMgr, theDispMode, theToCreate, Graphic3d_ZLayerId_Topmost);
        }

        //! Return used selection scheme.
        Standard_EXPORT AIS_SelectionScheme GetCurrentSelectionScheme() const;

        //! Return active selection mode; 0 by default.
        int GetSelectionMode() const { return mySelMode; }

        //! Set new selection mode.
        Standard_EXPORT virtual void SetSelectionMode(const int theMode);

        //! Check if selection mode is acceptable; default implementation accepts only 0th selection mode.
        virtual bool AcceptSelectionMode(const int theMode) const { return theMode == 0; }

        //! Check if marked selected.
        bool IsMarkedSelected() const { return myMarkedSelected; }

        //! Mark selected.
        void MarkSelected(bool theIsSelected) { myMarkedSelected = theIsSelected; }

        //! Return a value that is added to the default displaying/selection object's priority, 0 by default.
        int PriorityModifier() const { return myPriorityModifier; }

        //! Set a value that is added to the default displaying/selection object's priority.
        void SetPriorityModifier(int theValue) { myPriorityModifier = theValue; }

        //! Infinite presentation flag, FALSE by default.
        //! Used to distinguish auxiliary presentations (construction elements, manipulators) from main scene objects.
        //! Such objects are not considered within FitAll operation.
        bool IsInfiniteState() const { return myIsInfinite; }

        //! Set infinite presentation flag.
        void SetInfiniteState(int theValue) { myIsInfinite = theValue; }

    public:
        //! Highlights selectable object's presentation.
        Standard_EXPORT virtual void HilightWithColor(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                                      const Handle(Prs3d_Drawer) & theStyle,
                                                      const Standard_Integer theMode) override;

        //! Removes highlighting from the owner.
        Standard_EXPORT virtual void Unhilight(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                               const Standard_Integer theMode) override;

        //! Handle mouse button click event.
        Standard_EXPORT virtual Standard_Boolean HandleMouseClick(const Graphic3d_Vec2i &thePoint,
                                                                  Aspect_VKeyMouse theButton,
                                                                  Aspect_VKeyFlags theModifiers,
                                                                  bool theIsDoubleClick) override;

    public:
        //! Check if object's deferred data is loaded.
        virtual bool IsLoaded() const
        {
            for (const Handle(InteractiveOwner) &anOwnerIter : Children())
            {
                if (!anOwnerIter->IsLoaded())
                {
                    return false;
                }
            }
            return true;
        }

        //! Return copy of this presentation; default implementation returns NULL.
        virtual Handle(ViewBase::InteractiveOwner) Clone(const TCollection_AsciiString &theNewId)
        {
            (void)theNewId;
            return Handle(ViewBase::InteractiveOwner)();
        }

        //! Create interactive object.
        virtual Handle(AIS_InteractiveObject) CreatePresentation() const { return Handle(AIS_InteractiveObject)(); }

    protected:
        class LayeredAisObject;

    protected:
        TCollection_AsciiString myId;
        SelectionPart myPart;

        int mySelMode = 0;
        int myPriorityModifier = 0;
        bool myIsInfinite = false;

        bool myMarkedSelected = false;
    };
}

#endif // INTERACTIVEOWNER_HEADERFILE
