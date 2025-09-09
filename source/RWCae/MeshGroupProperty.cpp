#include <RWCae/MeshGroupProperty.h>

#include <Message.hxx>
#include <Standard_NotImplemented.hxx>

namespace
{
    // ================================================================
    // Function : VectorToGroupsMask
    // ================================================================
    RWCae::MeshGroupProperty::MeshGroupsMaskType VectorToGroupsMask(const std::vector<int> &theGroups)
    {
        RWCae::MeshGroupProperty::MeshGroupsMaskType aRes = 0;
        for (int aGroup : theGroups)
        {
            if (aGroup < 0 || aGroup >= RWCae::MeshGroupProperty::MaxNbGroups)
            {
                Message::SendFail() << "Error: group '" << aGroup << "' is out of range";
                continue;
            }
            aRes |= ((RWCae::MeshGroupProperty::MeshGroupsMaskType)1) << aGroup;
        }
        return aRes;
    }

    // ================================================================
    // Function : IntToGroupsMask
    // ================================================================
    RWCae::MeshGroupProperty::MeshGroupsMaskType IntToGroupsMask(const int theGroup)
    {
        if (theGroup < 0)
        {
            return 0;
        }
        if (theGroup >= RWCae::MeshGroupProperty::MaxNbGroups)
        {
            Message::SendFail() << "Error: group '" << theGroup << "' is out of range";
            return 0;
        }
        return ((RWCae::MeshGroupProperty::MeshGroupsMaskType)1) << theGroup;
    }

}

namespace RWCae
{

    // ================================================================
    // Function : Constructor
    // ================================================================
    MeshGroupProperty::MeshGroupProperty(const TCollection_AsciiString &theName,
                                         const MappingType theMapType,
                                         const int theIncrement)
        : MeshScalarPropertyBase(theName, theMapType),
          myGroups(theIncrement)
    {
    }

    // ================================================================
    // Function : Init
    // ================================================================
    void MeshGroupProperty::Init(const int theSize)
    {
        myGroups.Clear();

        for (int i = 0; i < MaxNbGroups; i++)
        {
            myGroupsInfo[i].NbEntities = 0;
            myGroupsInfo[i].Name = "";
        }

        if (theSize > 0)
        {
            myGroups.SetValue(theSize - 1, 0);
            for (MeshGroupsMaskType &aValIter : myGroups)
            {
                aValIter = 0;
            }
        }

        myIsLoaded = theSize > 0;
    }

    // ================================================================
    // Function : SetGroups
    // ================================================================
    void MeshGroupProperty::SetGroups(int theIndex, MeshGroupsMaskType theGroupMask, bool toComputeRange, bool toCalcInfo)
    {
        if (!myIsLoaded)
        {
            return;
        }

        myGroups.SetValue(theIndex, theGroupMask);

        if (theGroupMask == 0 || (!toComputeRange && !toCalcInfo))
        {
            return;
        }

        for (int i = 0; i < MaxNbGroups; i++)
        {
            if (IsInGroup(theGroupMask, i))
            {
                if (toCalcInfo)
                {
                    myGroupsInfo[i].NbEntities++;
                }
                if (toComputeRange)
                {
                    myValueRange.Add(i);
                }
            }
        }
    }

    // ================================================================
    // Function : SetGroups
    // ================================================================
    void MeshGroupProperty::SetGroups(int theIndex, const std::vector<int> &theGroups, bool toComputeRange, bool toCalcInfo)
    {
        SetGroups(theIndex, VectorToGroupsMask(theGroups), toComputeRange, toCalcInfo);
    }

    // ================================================================
    // Function : SetGroup
    // ================================================================
    void MeshGroupProperty::SetGroup(int theIndex, int theGroup, bool toComputeRange, bool toCalcInfo)
    {
        SetGroups(theIndex, IntToGroupsMask(theGroup), toComputeRange, toCalcInfo);
    }

    // ================================================================
    // Function : GetGroups
    // ================================================================
    MeshGroupProperty::MeshGroupsMaskType MeshGroupProperty::GetGroups(int theIndex) const
    {
        if (!myIsLoaded)
        {
            return 0;
        }

        return myGroups[theIndex];
    }

    // ================================================================
    // Function : GetFirstGroup
    // ================================================================
    int MeshGroupProperty::GetFirstGroup(int theIndex) const
    {
        if (!myIsLoaded)
        {
            return -1;
        }

        MeshGroupsMaskType aGroups = myGroups[theIndex];
        for (int i = 0; i < MaxNbGroups; i++)
        {
            if (IsInGroup(aGroups, i))
            {
                return i;
            }
        }
        return -1;
    }

    // ================================================================
    // Function : SetGroupName
    // ================================================================
    void MeshGroupProperty::SetGroupName(int theGroup, const TCollection_AsciiString &theName)
    {
        if (theGroup < 0 || theGroup >= MaxNbGroups)
        {
            Message::SendFail() << "Error: group '" << theGroup << "' is out of range";
            return;
        }

        myGroupsInfo[theGroup].Name = theName;
    }

    // ================================================================
    // Function : GetGroupName
    // ================================================================
    TCollection_AsciiString MeshGroupProperty::GetGroupName(int theGroup) const
    {
        if (theGroup < 0 || theGroup >= MaxNbGroups)
        {
            Message::SendFail() << "Error: group '" << theGroup << "' is out of range";
            return "";
        }

        return myGroupsInfo[theGroup].Name;
    }

    // ================================================================
    // Function : SetGroupNbEntities
    // ================================================================
    void MeshGroupProperty::SetGroupNbEntities(int theGroup, int theNbEntities)
    {
        if (theGroup < 0 || theGroup >= MaxNbGroups)
        {
            Message::SendFail() << "Error: group '" << theGroup << "' is out of range";
            return;
        }

        myGroupsInfo[theGroup].NbEntities = theNbEntities;
    }

    // ================================================================
    // Function : GetGroupNbEntities
    // ================================================================
    int MeshGroupProperty::GetGroupNbEntities(int theGroup) const
    {
        if (theGroup < 0 || theGroup >= MaxNbGroups)
        {
            Message::SendFail() << "Error: group '" << theGroup << "' is out of range";
            return 0;
        }

        return myGroupsInfo[theGroup].NbEntities;
    }

    // ================================================================
    // Function : HasValue
    // ================================================================
    bool MeshGroupProperty::HasValue(const MeshPropertyEntityInfo &theInfo) const
    {
        if (!myIsLoaded)
        {
            return false;
        }

        return myGroups[theInfo.Index] != 0;
    }

    // ================================================================
    // Function : Value
    // ================================================================
    double MeshGroupProperty::Value(const MeshPropertyEntityInfo &theInfo) const
    {
        if (!myIsLoaded)
        {
            return std::numeric_limits<double>::quiet_NaN();
        }

        int aFirstGroup = GetFirstGroup(theInfo.Index);
        return aFirstGroup != -1 ? aFirstGroup : std::numeric_limits<double>::quiet_NaN();
    }

    // ================================================================
    // Function : RawValue
    // ================================================================
    const char *MeshGroupProperty::RawValue(const MeshPropertyEntityInfo &theInfo) const
    {
        if (!myIsLoaded)
        {
            static MeshGroupsMaskType aNullValue = 0;
            return reinterpret_cast<const char *>(&aNullValue);
        }

        return reinterpret_cast<const char *>(&myGroups[theInfo.Index]);
    }

    // ================================================================
    // Function : SetValue
    // ================================================================
    void MeshGroupProperty::SetValue(const MeshPropertyEntityInfo &theInfo, double theValue, bool toComputeRange)
    {
        if (!myIsLoaded)
        {
            return;
        }

        SetGroup(theInfo.Index, (int)theValue, toComputeRange);
    }

    // ================================================================
    // Function : SetRawValue
    // ================================================================
    void MeshGroupProperty::SetRawValue(const MeshPropertyEntityInfo &theInfo, const char *theValue, bool toComputeRange)
    {
        MeshGroupsMaskType aValue = *reinterpret_cast<const MeshGroupsMaskType *>(theValue);
        SetGroups(theInfo.Index, aValue, toComputeRange);
    }

    // ================================================================
    // Function : Unload
    // ================================================================
    void MeshGroupProperty::Unload()
    {
        myGroups.Clear();
        myIsLoaded = false;
    }

    // ================================================================
    // Function : GetUsedGroups
    // ================================================================
    std::map<int, std::string> MeshGroupProperty::GetUsedGroups() const
    {
        std::map<int, std::string> aRes;
        for (int i = 0; i < MaxNbGroups; i++)
        {
            if (myGroupsInfo[i].NbEntities > 0)
            {
                aRes[i] = myGroupsInfo[i].Name.ToCString();
            }
        }
        return aRes;
    }

} // namespace