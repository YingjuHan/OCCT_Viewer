#ifndef _SHAPEDATAEXPLORER_HEADERFILE
#define _SHAPEDATAEXPLORER_HEADERFILE

#include <Graphic3d_Vec2.hxx>
#include <NCollection_DataMap.hxx>
#include <TopExp_DfsExplorer.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_SequenceOfShape.hxx>
#include <TopTools_ShapeMapHasher.hxx>

namespace RWShape
{

    //! Auxiliary class to store shape's children indices ranges per each shape type.
    class SubShapesBounds
    {
    public:
        //! Constructor.
        SubShapesBounds()
        {
            for (Graphic3d_Vec2i &aBound : myBounds)
            {
                aBound.SetValues(-1, -1);
            }
        }

        //! Check if range for specified type is defined.
        bool IsDefined(TopAbs_ShapeEnum theType) const { return myBounds[(int)theType].x() != -1; }

        //! Check if range for specified type has intersection with other range.
        bool Contains(TopAbs_ShapeEnum theType, const Graphic3d_Vec2i &theBound) const
        {
            return theBound.x() <= myBounds[(int)theType].y() && theBound.y() >= myBounds[(int)theType].x();
        }

        //! Return number of elements in range for specified type.
        int NbElems(TopAbs_ShapeEnum theType) const
        {
            return myBounds[(int)theType].x() != -1 ? myBounds[(int)theType].y() - myBounds[(int)theType].x() + 1 : 0;
        }

        //! Init range for specified type with value if not defined;
        //! increment range's upper bound otherwise.
        void InitIncrement(TopAbs_ShapeEnum theType, int theValue)
        {
            if (myBounds[(int)theType].x() == -1)
            {
                myBounds[(int)theType].SetValues(theValue, theValue);
            }
            else
            {
                myBounds[(int)theType].y()++;
            }
        }

        //! Add range for specified type.
        void Add(TopAbs_ShapeEnum theType, const Graphic3d_Vec2i &theBound, int theOffset)
        {
            if (theBound.x() == -1)
            {
                return;
            }

            if (myBounds[(int)theType].x() == -1)
            {
                myBounds[(int)theType].SetValues(theBound.x() + theOffset, theBound.y() + theOffset);
            }
            else
            {
                myBounds[(int)theType].x() = Min(myBounds[(int)theType].x(), theBound.x() + theOffset);
                myBounds[(int)theType].y() = Max(myBounds[(int)theType].y(), theBound.y() + theOffset);
            }
        }

        //! Return range for specified type.
        const Graphic3d_Vec2i &GetBound(TopAbs_ShapeEnum theType) const { return myBounds[(int)theType]; }

        static constexpr int NbShapeTypes = TopAbs_SHAPE;

    protected:
        Graphic3d_Vec2i myBounds[NbShapeTypes];
    };

    typedef NCollection_DataMap<TopoDS_Shape, SubShapesBounds, TopTools_ShapeMapHasher> SubShapesBoundsMap;

    //! A tool to explore TopoDS_Shape subshapes of specified type limited by index range.
    class ShapeDataExplorer
    {
    public:
        //! Constructor.
        ShapeDataExplorer(const TopTools_SequenceOfShape &theShapes,
                          const SubShapesBoundsMap &theBounds,
                          const Graphic3d_Vec2i &theRange,
                          TopAbs_ShapeEnum theType)
            : myShapes(&theShapes),
              myBounds(&theBounds),
              myRange(theRange),
              myType(theType)
        {
            if (myShapes->IsEmpty())
            {
                myCurShapeIndex = 1;
                return;
            }
        }

        //! Check if there are more subshapes.
        bool More() const { return myCurShapeIndex <= myShapes->Size(); }

        //! Move to the next subshape.
        void Next()
        {
            // Iterate with TopExp_DfsExplorer(myUpperIter) over top TopoDS_Shape subshapes that are contained in myBounds
            // 1. If type of the current shape in myUpperIter is equal to reference type, return it
            // 2. Otherwise check its subshapes range
            //   2.1 If range is not defined perform subshapes iteration with TopExp_Explorer (myLowerIter)
            //   2.2 Otherwise if range is not intersected with myRange, skip corresponding subshapes
            if (myLowerIter.More())
            {
                myLowerIter.Next();
                if (myLowerIter.More())
                {
                    myCurElemIndex++;
                    return;
                }
            }

            if (!myUpperIter.More())
            {
                do
                {
                    myCurShapeIndex++;
                    if (!More())
                    {
                        return;
                    }
                    myUpperIter.Init(myShapes->Value(myCurShapeIndex));
                } while (!myUpperIter.More());
            }
            else
            {
                myUpperIter.Next();
                if (!myUpperIter.More())
                {
                    Next();
                    return;
                }
            }

            if (myUpperIter.Current().ShapeType() == myType)
            {
                myCurElemIndex++;
                myUpperIter.SetExploreCurrent(false);
                return;
            }

            const SubShapesBounds *aBounds = myBounds->Seek(myUpperIter.Current());
            if (aBounds == nullptr || !aBounds->IsDefined(myType))
            {
                myLowerIter.Init(myUpperIter.Current(), myType);
                myUpperIter.SetExploreCurrent(false);
                if (myLowerIter.More())
                {
                    myCurElemIndex++;
                    return;
                }
            }
            else if (!aBounds->Contains(myType, myRange))
            {
                myCurElemIndex += aBounds->NbElems(myType);
                myUpperIter.SetExploreCurrent(false);
            }

            Next();
        }

        //! Return current subshape.
        const TopoDS_Shape &Value() const { return myLowerIter.More() ? myLowerIter.Current() : myUpperIter.Current(); }

        //! Return index of the current subshape.
        int Index() const { return myCurElemIndex; }

    protected:
        const TopTools_SequenceOfShape *myShapes;
        const SubShapesBoundsMap *myBounds;
        Graphic3d_Vec2i myRange;
        TopAbs_ShapeEnum myType;

        TopExp_DfsExplorer myUpperIter;
        TopExp_Explorer myLowerIter;
        int myCurShapeIndex = 0;
        int myCurElemIndex = -1;
    };

} // namespace

#endif // RWShape_SHAPEDATAEXPLORER_HEADERFILE