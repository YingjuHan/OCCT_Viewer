#ifndef RWCae_MESHPROPERTYDATA_HEADERFILE
#define RWCae_MESHPROPERTYDATA_HEADERFILE

#include <cmath>
#include <limits>

namespace RWCae
{
    //! Return true for NaN.
    template <typename T>
    inline bool IsNaN(const T theValue)
    {
#if defined(_MSC_VER)
        return ::_isnan(theValue) != 0;
#else
        return ::std::isnan(theValue);
#endif
    }

    //! Type of elements in array of mesh property values.
    enum class MeshPropertyElemType
    {
        Double,
        Float
    };

    //! Indices of mesh entity.
    struct MeshPropertyEntityInfo
    {
        int Index;
        int SubIndex;
        explicit MeshPropertyEntityInfo(int theIndex, int theSubIndex = -1) : Index(theIndex), SubIndex(theSubIndex) {}
    };

    //! Base class for storing array of mesh property values.
    class MeshPropertyData
    {
    public:
        //! Return type of elements.
        MeshPropertyElemType ElemType() const { return myType; }

        //! Return size of elements in bytes.
        size_t ElemSize() const { return myElemSize; }

        //! Return NaN value of specified type.
        inline double NaNValue() const
        {
            switch (myType)
            {
            case MeshPropertyElemType::Double:
                return myDoubleNaNValue;
            case MeshPropertyElemType::Float:
                return static_cast<double>(myFloatNaNValue);
            default:
                break;
            }
            return myDoubleNaNValue;
        }

        //! Return pointer to NaN value of specified type.
        inline const char *RawNaNValue() const
        {
            switch (myType)
            {
            case MeshPropertyElemType::Double:
                return reinterpret_cast<const char *>(&myDoubleNaNValue);
            case MeshPropertyElemType::Float:
                return reinterpret_cast<const char *>(&myFloatNaNValue);
            default:
                break;
            }

            return reinterpret_cast<const char *>(&myDoubleNaNValue);
        }

    protected:
        //! Set type of elements.
        void setType(MeshPropertyElemType theType)
        {
            myType = theType;
            switch (myType)
            {
            case MeshPropertyElemType::Double:
                myElemSize = sizeof(double);
                break;
            case MeshPropertyElemType::Float:
                myElemSize = sizeof(float);
                break;
            default:
                break;
            }
        }

    protected:
        MeshPropertyElemType myType = MeshPropertyElemType::Double;
        size_t myElemSize = sizeof(double);

        double myDoubleNaNValue = std::numeric_limits<double>::quiet_NaN();
        float myFloatNaNValue = std::numeric_limits<float>::quiet_NaN();
    };

} // namespace

#endif // RWCae_MESHPROPERTYDATA_HEADERFILE
