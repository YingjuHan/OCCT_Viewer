#ifndef ViewBase_STATSACTIVATOR_HEADERFILE
#define ViewBase_STATSACTIVATOR_HEADERFILE

#include <AIS_InteractiveObject.hxx>

namespace ViewBase
{
    //! Interactive object for activating on-screen statistic presentation.
    class StatsActivator : public AIS_InteractiveObject
    {
        static constexpr int m_defStatSize = 140;

    public:
        //! Empty constructor.
        Standard_EXPORT StatsActivator();

        //! Destructor.
        Standard_EXPORT virtual ~StatsActivator();

        //! Set click area size.
        void SetSize(int theSize)
        {
            if (mySize != theSize)
            {
                mySize = theSize;
                SetToUpdate();
            }
        }

        //! Return extra info.
        const Handle(TCollection_HAsciiString) & ExtraInfo() const { return myExtraInfo; }

        //! Update extra info.
        Standard_EXPORT void UpdateExtraInfo();

    private:
        //! Return TRUE for supported display modes (only mode 0 is supported).
        virtual Standard_Boolean AcceptDisplayMode(const Standard_Integer theMode) const override { return theMode == 0; }

        //! Compute presentation.
        Standard_EXPORT virtual void Compute(const Handle(PrsMgr_PresentationManager) & thePrsMgr,
                                             const Handle(Prs3d_Presentation) & thePrs,
                                             const Standard_Integer theMode) override;

        //! No selection.
        Standard_EXPORT virtual void ComputeSelection(const Handle(SelectMgr_Selection) &, const Standard_Integer) override;

    private:
        class StatsActivatorOwner;

    private:
        Handle(TCollection_HAsciiString) myExtraInfo;
        Standard_Integer mySize;
    };

} // namespace

#endif