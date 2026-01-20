#ifndef MESHGROUPPROPERTY_HEADERFILE
#define MESHGROUPPROPERTY_HEADERFILE

#include <RWCae/MeshScalarPropertyBase.h>

#include <NCollection_Vector.hxx>

#include <limits>
#include <map>
#include <vector>

namespace RWCae
{
    //! Class for storing groups properties assigned to mesh elements/nodes.
    class MeshGroupProperty : public MeshScalarPropertyBase
    {
    public:
        typedef uint32_t MeshGroupsMaskType;
        static constexpr int MaxNbGroups = std::numeric_limits<MeshGroupsMaskType>::digits;

    public:
        //! Default constructor.
        Standard_EXPORT MeshGroupProperty(const TCollection_AsciiString &theName,
                                          const MappingType theMapType,
                                          const int theIncrement = 256);

    public:
        //! Return number of property values.
        virtual int NbValues() const override { return myGroups.Size(); }

        //! Return TRUE if value for specified index has been defined (e.g. not Nan).
        Standard_EXPORT virtual bool HasValue(const MeshPropertyEntityInfo &theInfo) const override;

        //! Return value for specified index.
        Standard_EXPORT virtual double Value(const MeshPropertyEntityInfo &theInfo) const override;

        //! Return pointer to value for specified index.
        Standard_EXPORT virtual const char *RawValue(const MeshPropertyEntityInfo &theInfo) const override;

        //! Set value for specified index.
        Standard_EXPORT virtual void SetValue(const MeshPropertyEntityInfo &theInfo,
                                              double theValue,
                                              bool toComputeRange = true) override;

        //! Set value for specified index by its pointer.
        Standard_EXPORT virtual void SetRawValue(const MeshPropertyEntityInfo &theInfo,
                                                 const char *theValue,
                                                 bool toComputeRange = true) override;

        //! Return size of elements in bytes.
        virtual size_t ElemSize() const override { return sizeof(MeshGroupsMaskType); }

        //! Return type of elements.
        virtual MeshPropertyElemType ElemType() const override { return MeshPropertyElemType::Float; }

        //! Return property base type.
        virtual MeshScalarPropertyBaseType BaseType() const override { return MeshScalarPropertyBaseType::Group; }

        //! Clear values data and mark as not loaded.
        Standard_EXPORT virtual void Unload() override;

    public:
        //! Initialize the array of required size with specified value.
        Standard_EXPORT void Init(const int theSize);

        //! Set groups for specified entity.
        Standard_EXPORT void SetGroups(int theIndex,
                                       MeshGroupsMaskType theGroupsMask,
                                       bool toComputeRange = true,
                                       bool toCalcInfo = true);

        //! Set groups for specified entity.
        Standard_EXPORT void SetGroups(int theIndex,
                                       const std::vector<int> &theGroups,
                                       bool toComputeRange = true,
                                       bool toCalcInfo = true);

        //! Set group for specified entity.
        Standard_EXPORT void SetGroup(int theIndex, int theGroup, bool toComputeRange = true, bool toCalcInfo = true);

        //! Return all assigned groups for specified entity.
        Standard_EXPORT MeshGroupsMaskType GetGroups(int theIndex) const;

        //! Return first assigned group for specified entity.
        Standard_EXPORT int GetFirstGroup(int theIndex) const;

        //! Set name for specified group.
        Standard_EXPORT void SetGroupName(int theGroup, const TCollection_AsciiString &theName);

        //! Return name for specified group.
        Standard_EXPORT TCollection_AsciiString GetGroupName(int theGroup) const;

        //! Set number of entities for specified group.
        Standard_EXPORT void SetGroupNbEntities(int theGroup, int theNbEntities);

        //! Return number of entities for specified group.
        Standard_EXPORT int GetGroupNbEntities(int theGroup) const;

        //! Return map of groups with non-zero number of entities.
        Standard_EXPORT std::map<int, std::string> GetUsedGroups() const;

        //! Create a new instance of this class.
        static Handle(MeshScalarPropertyBase) NewInstance(MeshPropertyBase::MappingType theType)
        {
            return new MeshGroupProperty("", theType);
        }

        //! Check theGroupsMask contains theGroup.
        static bool IsInGroup(RWCae::MeshGroupProperty::MeshGroupsMaskType theGroupsMask, int theGroup)
        {
            return (theGroupsMask & (((RWCae::MeshGroupProperty::MeshGroupsMaskType)1) << theGroup)) != 0;
        }

    protected:
        //! Struct for storing group information.
        struct GroupInfo
        {
            TCollection_AsciiString Name;
            int NbEntities = 0;
        };

    protected:
        NCollection_Vector<MeshGroupsMaskType> myGroups;
        GroupInfo myGroupsInfo[MaxNbGroups];
    };

} // namespace

#endif // MESHGROUPPROPERTY_HEADERFILE
