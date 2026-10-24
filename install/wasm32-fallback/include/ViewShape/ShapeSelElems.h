#ifndef SHAPESELELEMS_HEADERFILE
#define SHAPESELELEMS_HEADERFILE

#include <ViewShape/ShapeSelElemType.h>

#include <TColStd_HPackedMapOfInteger.hxx>

#include <array>

namespace ViewShape
{

    //! Structure describing selected elements of shape in `ViewShape::InteractiveShape` object.
    class ShapeSelElems
    {
    public:
        //! Empty constructor.
        ShapeSelElems() {}

        //! Reset index maps of specified element type.
        void Clear(const ShapeSelElemType theType)
        {
            if (!myIndices[(int)theType].IsNull())
            {
                myIndices[(int)theType]->ChangeMap().Clear();
                myIndices[(int)theType].Nullify();
            }
        }

        //! Reset index maps.
        void Clear()
        {
            for (int aTypeIter = 0; aTypeIter < ShapeSelElemType_NB; ++aTypeIter)
            {
                Clear((ShapeSelElemType)aTypeIter);
            }
        }

        //! Return number of selected elements of all types.
        int Extent() const
        {
            int anExtent = 0;
            for (int aTypeIter = 0; aTypeIter < ShapeSelElemType_NB; ++aTypeIter)
            {
                anExtent += Extent((ShapeSelElemType)aTypeIter);
            }
            return anExtent;
        }

        //! Return number of selected elements of specified type.
        int Extent(const ShapeSelElemType theType) const
        {
            return !myIndices[(int)theType].IsNull() ? myIndices[(int)theType]->Map().Extent() : 0;
        }

        //! Return TRUE if at least single element of specified type is set.
        bool IsValid(const ShapeSelElemType theType) const
        {
            return !myIndices[(int)theType].IsNull() && !myIndices[(int)theType]->Map().IsEmpty();
        }

        //! Return TRUE if at least one element of any type is set.
        bool IsValid() const
        {
            for (int aTypeIter = 0; aTypeIter < ShapeSelElemType_NB; ++aTypeIter)
            {
                if (IsValid((ShapeSelElemType)aTypeIter))
                {
                    return true;
                }
            }
            return false;
        }

        //! Return TRUE if index of specified element type is set.
        bool Contains(const ShapeSelElemType theType, const int theIdx) const
        {
            return IsValid(theType) && myIndices[(int)theType]->Map().Contains(theIdx);
        }

        //! Add index of specified element type.
        bool Add(const ShapeSelElemType theType, const int theIdx)
        {
            if (myIndices[(int)theType].IsNull())
            {
                myIndices[(int)theType] = new TColStd_HPackedMapOfInteger();
            }
            return myIndices[(int)theType]->ChangeMap().Add(theIdx);
        }

        //! Remove index of specified element type.
        bool Remove(const ShapeSelElemType theType, const int theIdx)
        {
            if (myIndices[(int)theType].IsNull())
            {
                return false;
            }
            return myIndices[(int)theType]->ChangeMap().Remove(theIdx);
        }

        //! Return index map of specified element type.
        const Handle(TColStd_HPackedMapOfInteger) & Indices(const ShapeSelElemType theType) const
        {
            return myIndices[(int)theType];
        }

        //! Return index map of specified element type.
        Handle(TColStd_HPackedMapOfInteger) & ChangeIndices(const ShapeSelElemType theType)
        {
            return myIndices[(int)theType];
        }

        //! Define this map as XOR with another map.
        void XOR(const ShapeSelElemType theType, const Handle(TColStd_HPackedMapOfInteger) & theMap)
        {
            if (theMap.IsNull())
            {
                return;
            }

            for (TColStd_PackedMapOfInteger::Iterator anIndexIter(theMap->Map()); anIndexIter.More(); anIndexIter.Next())
            {
                if (!Remove(theType, anIndexIter.Key()))
                {
                    Add(theType, anIndexIter.Key());
                }
            }
        }

        //! Define this map as XOR with another map.
        void XOR(const ShapeSelElems &theOther)
        {
            for (int aTypeIter = 0; aTypeIter < ShapeSelElemType_NB; ++aTypeIter)
            {
                const ShapeSelElemType aType = (ShapeSelElemType)aTypeIter;
                XOR(aType, theOther.Indices(aType));
            }
        }

    private:
        std::array<Handle(TColStd_HPackedMapOfInteger), ShapeSelElemType_NB> myIndices;
    };

} // namespace

#endif // SHAPESELELEMS_HEADERFILE
