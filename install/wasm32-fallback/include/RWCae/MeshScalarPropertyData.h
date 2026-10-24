#include <RWCae/MeshPropertyData.h>

#include <NCollection_Vector.hxx>

namespace RWCae
{
    //! Class for storing array of scalar property elements.
    class MeshScalarPropertyData : public MeshPropertyData
    {
    public:
        //! Constructor.
        explicit MeshScalarPropertyData(const int theIncrement = 256)
            : myDataDouble(theIncrement),
              myDataFloat(theIncrement)
        {
        }

        //! Return number of property values.
        inline int NbValues() const
        {
            switch (myType)
            {
            case MeshPropertyElemType::Float:
                return myDataFloat.Size();
            case MeshPropertyElemType::Double:
                return myDataDouble.Size();
            default:
                break;
            }
            return 0;
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
                init(theSize, myDataDouble);
                break;
            case MeshPropertyElemType::Float:
                init(theSize, myDataFloat);
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
            case MeshPropertyElemType::Double:
                myDataDouble.Clear();
                break;
            case MeshPropertyElemType::Float:
                myDataFloat.Clear();
                break;
            default:
                break;
            }
        }

        //! Return TRUE if value for specified index has been defined (e.g. not Nan).
        inline bool HasValue(const MeshPropertyEntityInfo &theInfo) const
        {
            switch (myType)
            {
            case MeshPropertyElemType::Double:
                return !IsNaN(myDataDouble.Value(theInfo.Index));
            case MeshPropertyElemType::Float:
                return !IsNaN(myDataFloat.Value(theInfo.Index));
            default:
                break;
            }
            return false;
        }

        //! Return value for specified index.
        inline double Value(const MeshPropertyEntityInfo &theInfo) const
        {
            switch (myType)
            {
            case MeshPropertyElemType::Double:
                return myDataDouble.Value(theInfo.Index);
            case MeshPropertyElemType::Float:
                return static_cast<double>(myDataFloat.Value(theInfo.Index));
            default:
                break;
            }
            return 0;
        }

        //! Return pointer to value for specified index.
        inline const char *RawValue(const MeshPropertyEntityInfo &theInfo) const
        {
            switch (myType)
            {
            case MeshPropertyElemType::Double:
                return reinterpret_cast<const char *>(&myDataDouble.Value(theInfo.Index));
            case MeshPropertyElemType::Float:
                return reinterpret_cast<const char *>(&myDataFloat.Value(theInfo.Index));
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
                myDataDouble.SetValue(theInfo.Index, theValue);
                break;
            case MeshPropertyElemType::Float:
                myDataFloat.SetValue(theInfo.Index, static_cast<float>(theValue));
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
                myDataDouble.SetValue(theInfo.Index, *reinterpret_cast<const double *>(theValue));
                break;
            case MeshPropertyElemType::Float:
                myDataFloat.SetValue(theInfo.Index, *reinterpret_cast<const float *>(theValue));
                break;
            default:
                break;
            }
        }

        //! Append value to array.
        inline void AppendValue(double theValue)
        {
            switch (myType)
            {
            case MeshPropertyElemType::Double:
                myDataDouble.Append(theValue);
                break;
            case MeshPropertyElemType::Float:
                myDataFloat.Append(static_cast<float>(theValue));
                break;
            default:
                break;
            }
        }

    protected:
        //! Init array of values of specified type.
        template <typename T>
        void init(int theSize, NCollection_Vector<T> &theData)
        {
            T aDefValue = std::numeric_limits<T>::quiet_NaN();
            theData.SetValue(theSize - 1, aDefValue);
            for (int i = 0; i < theSize - 1; i++)
            {
                theData.SetValue(i, aDefValue);
            }
        }

    protected:
        NCollection_Vector<double> myDataDouble;
        NCollection_Vector<float> myDataFloat;
    };

} // namespace