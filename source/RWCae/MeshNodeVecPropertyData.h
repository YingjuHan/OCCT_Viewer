#ifndef _MESHNODVECPROPERTYDATA_HEADERFILE
#define _MESHNODVECPROPERTYDATA_HEADERFILE

#include <RWCae/MeshPropertyData.h>

#include <NCollection_Vector.hxx>

#include <vector>

namespace RWCae
{
    //! Node index -> node value map.
    //! Extends @c std::vector<std::pair<int, double>> with basic "key-value" manipulation.
    //! std::vector is the most memory-efficient yet sufficiently fast container for a small amount
    //! of "key-value" pairs, though it hasn't been tested with large hexahedral meshes yet,
    //! when 8 "key-value" pairs should be stored.
    template <typename T>
    class NodeVecValueMap : public std::vector<std::pair<int, T>>
    {
    public:
        //! Empty constructor
        NodeVecValueMap() : std::vector<std::pair<int, T>>() {}

        //! Lookup for a specific "key-value" pair using the key
        bool Find(const int theKey, T &theValue) const
        {
            for (const std::pair<int, T> &anItem : *this)
            {
                if (anItem.first == theKey)
                {
                    theValue = anItem.second;
                    return true;
                }
            }
            return false;
        }

        //! Appends a new "key-value" pair or replaces the value for an existing key.
        //! This method introduces performance penalty (key search and vector resizing time),
        //! thus it should be avoided as much as possible in favor of single resize() + ChangeKey()/ChangeValue().
        void Bind(const int theKey, const T &theValue)
        {
            for (std::pair<int, T> &anItem : *this)
            {
                if (anItem.first == theKey)
                {
                    anItem.second = theValue;
                    return;
                }
            }

            this->push_back(std::pair<int, T>(theKey, theValue));
        }

        //! Convenient getter for a key at a given index in a vector
        int Key(const int theIndex) const { return (*this)[theIndex].first; }

        //! Convenient lvalue getter for a key at a given index in a vector
        int &ChangeKey(const int theIndex) { return (*this)[theIndex].first; }

        //! Convenient getter for a value at a given index in a vector
        const T &Value(const int theIndex) const { return (*this)[theIndex].second; }

        //! Convenient lvalue getter for a value at a given index in a vector
        T &ChangeValue(const int theIndex) { return (*this)[theIndex].second; }
    };

    //! Class for storing array of nodevec property elements.
    class MeshNodeVecPropertyData : public MeshPropertyData
    {
    public:
        explicit MeshNodeVecPropertyData(const int theIncrement = 256)
            : myDataDouble(theIncrement),
              myDataFloat(theIncrement)
        {
        }

        //! Init array of values of specified type.
        void Init(int theSize, MeshPropertyElemType theType)
        {
            setType(theType);

            if (theSize == 0)
            {
                return;
            }

            switch (myType)
            {
            case MeshPropertyElemType::Double:
                myDataDouble.SetValue(theSize - 1, NodeVecValueMap<double>());
                break;
            case MeshPropertyElemType::Float:
                myDataFloat.SetValue(theSize - 1, NodeVecValueMap<float>());
                break;
            default:
                break;
            }
        }

        //! Clear array of values of specified type.
        void Clear()
        {
            switch (myType)
            {
            case MeshPropertyElemType::Float:
                myDataFloat.Clear();
                break;
            case MeshPropertyElemType::Double:
                myDataDouble.Clear();
                break;
            default:
                break;
            }
        }

        //! Return number of property values.
        inline int NbValues() const
        {
            switch (myType)
            {
            case MeshPropertyElemType::Double:
                return myDataDouble.Size();
            case MeshPropertyElemType::Float:
                return myDataFloat.Size();
            default:
                break;
            }
            return 0;
        }

        //! Return TRUE if value for specified index has been defined (e.g. not Nan).
        inline bool HasValue(const MeshPropertyEntityInfo &theInfo) const
        {
            switch (myType)
            {
            case MeshPropertyElemType::Double:
                return hasValue(theInfo, myDataDouble);
            case MeshPropertyElemType::Float:
                return hasValue(theInfo, myDataFloat);
            default:
                return false;
            }
        }

        //! Return value for specified index.
        inline double Value(const MeshPropertyEntityInfo &theInfo) const
        {
            switch (myType)
            {
            case MeshPropertyElemType::Double:
                return value(theInfo, myDataDouble);
            case MeshPropertyElemType::Float:
                return static_cast<double>(value(theInfo, myDataFloat));
            default:
                return std::numeric_limits<double>::quiet_NaN();
            }
        }

        //! Return pointer to value for specified index.
        inline const char *RawValue(const MeshPropertyEntityInfo &theInfo) const
        {
            switch (myType)
            {
            case MeshPropertyElemType::Double:
                return rawValue(theInfo, myDataDouble);
            case MeshPropertyElemType::Float:
                return rawValue(theInfo, myDataFloat);
            default:
                break;
            }

            static constexpr double aNaNElem = std::numeric_limits<double>::quiet_NaN();
            return reinterpret_cast<const char *>(&aNaNElem);
        }

        //! Set value for specified index.
        inline void SetValue(const MeshPropertyEntityInfo &theInfo, double theValue)
        {
            switch (myType)
            {
            case MeshPropertyElemType::Double:
                setValue(theInfo, myDataDouble, theValue);
                break;
            case MeshPropertyElemType::Float:
                setValue(theInfo, myDataFloat, static_cast<float>(theValue));
                break;
            default:
                break;
            }
        }

        //! Set value for specified index by its pointer.
        inline void SetRawValue(const MeshPropertyEntityInfo &theInfo, const char *theValue)
        {
            switch (myType)
            {
            case MeshPropertyElemType::Double:
                setRawValue(theInfo, myDataDouble, theValue);
                break;
            case MeshPropertyElemType::Float:
                setRawValue(theInfo, myDataFloat, theValue);
                break;
            default:
                break;
            }
        }

        //! Reserve vector for specified value map.
        inline void ReserveValueMap(int theIndex, int theSize)
        {
            switch (myType)
            {
            case MeshPropertyElemType::Double:
                myDataDouble.ChangeValue(theIndex).reserve(theSize);
                break;
            case MeshPropertyElemType::Float:
                myDataFloat.ChangeValue(theIndex).reserve(theSize);
                break;
            default:
                break;
            }
        }

        //! Return indices of nodes for specified element.
        void SubIndices(int theIndex, std::vector<int> &theIndices)
        {
            theIndices.clear();

            switch (myType)
            {
            case MeshPropertyElemType::Double:
                subIndices(theIndex, myDataDouble, &theIndices);
                break;
            case MeshPropertyElemType::Float:
                subIndices(theIndex, myDataFloat, &theIndices);
                break;
            default:
                break;
            }
        }

        //! Return size of specified element.
        int NbSubIndices(int theIndex) const
        {
            switch (myType)
            {
            case MeshPropertyElemType::Double:
                return NbSubIndices(theIndex, myDataDouble);
            case MeshPropertyElemType::Float:
                return NbSubIndices(theIndex, myDataFloat);
            default:
                return 0;
            }
        }

    protected:
        //! Return TRUE if value for specified index has been defined (e.g. not Nan).
        template <typename T>
        inline bool hasValue(const MeshPropertyEntityInfo &theInfo,
                             const NCollection_Vector<NodeVecValueMap<T>> &theData) const
        {
            T aValue = std::numeric_limits<T>::quiet_NaN();
            return theData.Value(theInfo.Index).Find(theInfo.SubIndex, aValue) && !IsNaN(aValue);
        }

        //! Return value for specified index.
        template <typename T>
        inline T value(const MeshPropertyEntityInfo &theInfo, const NCollection_Vector<NodeVecValueMap<T>> &theData) const
        {
            T aValue = std::numeric_limits<T>::quiet_NaN();
            theData.Value(theInfo.Index).Find(theInfo.SubIndex, aValue);
            return aValue;
        }

        //! Return pointer to value for specified index.
        template <typename T>
        inline const char *rawValue(const MeshPropertyEntityInfo &theInfo,
                                    const NCollection_Vector<NodeVecValueMap<T>> &theData) const
        {
            const NodeVecValueMap<T> &aValueMap = theData.Value(theInfo.Index);
            for (const std::pair<int, T> &anItem : aValueMap)
            {
                if (anItem.first == theInfo.SubIndex)
                {
                    return reinterpret_cast<const char *>(&anItem.second);
                }
            }
            static constexpr T aNaNElem = std::numeric_limits<T>::quiet_NaN();
            return reinterpret_cast<const char *>(&aNaNElem);
        }

        //! Set value for specified index by its pointer.
        template <typename T>
        inline void setValue(const MeshPropertyEntityInfo &theInfo,
                             NCollection_Vector<NodeVecValueMap<T>> &theData,
                             T theValue)
        {
            if (theInfo.Index >= theData.Length())
            {
                theData.SetValue(theInfo.Index, NodeVecValueMap<T>());
            }
            if (theInfo.SubIndex != -1)
            {
                theData.ChangeValue(theInfo.Index).Bind(theInfo.SubIndex, theValue);
            }
        }

        //! Set value for specified index by its pointer.
        template <typename T>
        inline void setRawValue(const MeshPropertyEntityInfo &theInfo,
                                NCollection_Vector<NodeVecValueMap<T>> &theData,
                                const char *theValue)
        {
            if (theInfo.Index >= theData.Length())
            {
                theData.SetValue(theInfo.Index, NodeVecValueMap<T>());
            }
            if (theInfo.SubIndex != -1)
            {
                theData.ChangeValue(theInfo.Index).Bind(theInfo.SubIndex, *reinterpret_cast<const T *>(theValue));
            }
        }

        //! Return indices of nodes for specified element.
        template <typename T>
        void subIndices(int theIndex, const NCollection_Vector<NodeVecValueMap<T>> &theData, std::vector<int> *theIndices)
        {
            const NodeVecValueMap<T> &aValues = theData[theIndex];
            theIndices->reserve(aValues.size());
            for (const std::pair<int, T> &anItem : aValues)
            {
                theIndices->push_back(anItem.first);
            }
        }

        template <typename T>
        int NbSubIndices(int theIndex, const NCollection_Vector<NodeVecValueMap<T>> &theData) const
        {
            return static_cast<int>(theData[theIndex].size());
        }

    protected:
        NCollection_Vector<NodeVecValueMap<double>> myDataDouble;
        NCollection_Vector<NodeVecValueMap<float>> myDataFloat;
    };

} // namespace RWCae
#endif