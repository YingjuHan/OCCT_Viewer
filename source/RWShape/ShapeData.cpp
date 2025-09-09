#include <RWShape/ShapeData.h>

#include <RWBase/TaskManager.h>

#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <Message.hxx>
#include <NCollection_IncAllocator.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Wire.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>

#include <algorithm>

namespace
{
    constexpr TopAbs_ShapeEnum SHAPE_BOUNDS_LOWER_TYPE = TopAbs_SHELL;

    typedef NCollection_IndexedDataMap<TopoDS_Shape, RWShape::SubShapesBounds, TopTools_ShapeMapHasher>
        SubShapesBoundsIndexedMap;

    //! Apply the specified function to all subshapes.
    void IterateShapes(const TopTools_SequenceOfShape &theShapes,
                       const std::function<void(TopExp_DfsExplorer &theIter)> &theFunc)
    {
        for (const TopoDS_Shape &aRootIter : theShapes)
        {
            for (TopExp_DfsExplorer anIter(aRootIter); anIter.More(); anIter.Next())
            {
                theFunc(anIter);
            }
        }
    }

    //! Gather subshapes that have all children > SHAPE_BOUNDS_LOWER_TYPE.
    void GatherLowerShapes(const TopTools_SequenceOfShape &theShapes, SubShapesBoundsIndexedMap &theLowerShapes)
    {
        IterateShapes(theShapes, [&theLowerShapes](TopExp_DfsExplorer &theIter)
                      {
        const TopoDS_Shape& aShape = theIter.Current();
        if (aShape.ShapeType() <= SHAPE_BOUNDS_LOWER_TYPE) {
            if (theIter.Depth() > 0) {
                theLowerShapes.RemoveKey(theIter.Current(theIter.Depth() - 1));
            }
            theLowerShapes.Add(aShape, RWShape::SubShapesBounds());
        }
        if (aShape.ShapeType() >= SHAPE_BOUNDS_LOWER_TYPE) {
            theIter.SetExploreCurrent(false);
        } });
    }

    //! Gather (in parallel) subshapes indices bounds of shapes that have all children > SHAPE_BOUNDS_LOWER_TYPE.
    void GatherLowerShapesBounds(SubShapesBoundsIndexedMap &theLowerShapes)
    {
        int aNbShapes = theLowerShapes.Size();
        int aNbThreads = RWBase::TaskManager::Instance()->NbDefaultThreadsToLaunch();
        int aPartNbElems = Max(aNbShapes / aNbThreads, 1);
        int aNbParts = Max(Min(aNbShapes / aPartNbElems, aNbThreads), 1);

        std::vector<TopExp_DfsExplorer *> anExplorers(aNbThreads);
        for (TopExp_DfsExplorer *&anIter : anExplorers)
        {
            anIter = new TopExp_DfsExplorer();
        }

        auto processLowerShape = [&theLowerShapes, &anExplorers, aNbShapes, aPartNbElems, aNbParts](int theThreadId,
                                                                                                    int theIdx)
        {
            int aShapeIdxBegin = aPartNbElems * theIdx;
            int aShapeIdxEnd = theIdx == aNbParts - 1 ? aNbShapes : aShapeIdxBegin + aPartNbElems;
            TopExp_DfsExplorer &anIter = *anExplorers[theThreadId];
            for (int aShapeIdx = aShapeIdxBegin; aShapeIdx < aShapeIdxEnd; aShapeIdx++)
            {
                const TopoDS_Shape &aShape = theLowerShapes.FindKey(aShapeIdx + 1);
                RWShape::SubShapesBounds &aBounds = theLowerShapes.ChangeFromIndex(aShapeIdx + 1);

                for (anIter.Init(aShape, true, false); anIter.More(); anIter.Next())
                {
                    TopAbs_ShapeEnum aShapeType = anIter.CurrentType();
                    aBounds.InitIncrement(aShapeType, 0);
                }
            }
        };

        RWBase::TaskManager::Instance()->RunInParallel(0, aNbParts, processLowerShape);

        for (TopExp_DfsExplorer *&anIter : anExplorers)
        {
            delete anIter;
        }
    }

    //! Compute shapes subshapes indices bounds.
    void ComputeShapesBounds(const TopTools_SequenceOfShape &theShapes,
                             const SubShapesBoundsIndexedMap &theLowerShapes,
                             RWShape::SubShapesBoundsMap &theBounds)
    {
        std::vector<int> aNbProc(RWShape::SubShapesBounds::NbShapeTypes, 0);

        auto updateUpperBounds = [&theBounds, &aNbProc](const TopExp_DfsExplorer &theIter,
                                                        const RWShape::SubShapesBounds *theLowerBounds)
        {
            TopAbs_ShapeEnum aShapeType = theIter.CurrentType();
            for (int aDepth = theIter.Depth(); aDepth >= 0; aDepth--)
            {
                const TopoDS_Shape &aShape = theIter.Current(aDepth);
                RWShape::SubShapesBounds *aBounds = theBounds.ChangeSeek(aShape);
                if (aBounds == nullptr)
                {
                    aBounds = theBounds.Bound(aShape, RWShape::SubShapesBounds());
                }
                for (int aType = aShapeType + 1; aType < RWShape::SubShapesBounds::NbShapeTypes; ++aType)
                {
                    aBounds->Add((TopAbs_ShapeEnum)aType,
                                 theLowerBounds->GetBound((TopAbs_ShapeEnum)aType),
                                 aNbProc[(int)aType]);
                }
            }
            for (int aType = aShapeType + 1; aType < RWShape::SubShapesBounds::NbShapeTypes; ++aType)
            {
                aNbProc[(int)aType] += theLowerBounds->NbElems((TopAbs_ShapeEnum)aType);
            }
        };

        auto propagateBounds = [&theBounds, &aNbProc](const TopExp_DfsExplorer &theIter)
        {
            TopAbs_ShapeEnum aShapeType = theIter.CurrentType();
            for (int aDepth = theIter.Depth() - 1; aDepth >= 0; aDepth--)
            {
                const TopoDS_Shape &aPar = theIter.Current(aDepth);
                if (aPar.ShapeType() > SHAPE_BOUNDS_LOWER_TYPE)
                {
                    continue;
                }
                RWShape::SubShapesBounds *aBounds = theBounds.ChangeSeek(aPar);
                if (aBounds == nullptr)
                {
                    aBounds = theBounds.Bound(aPar, RWShape::SubShapesBounds());
                }
                aBounds->InitIncrement(aShapeType, aNbProc[(int)aShapeType]);
            }
            aNbProc[(int)aShapeType]++;
        };

        IterateShapes(theShapes, [&updateUpperBounds, &propagateBounds, &theLowerShapes](TopExp_DfsExplorer &theIter)
                      {
        const TopoDS_Shape&               aCurShape    = theIter.Current();
        const RWShape::SubShapesBounds* aLowerBounds = theLowerShapes.Seek(aCurShape);
        if (aLowerBounds != nullptr) {
            theIter.SetExploreCurrent(false);
            updateUpperBounds(theIter, aLowerBounds);
        }

        propagateBounds(theIter); });
    }
}

namespace RWShape
{

    // =======================================================================
    // function : ShapeData
    // =======================================================================
    ShapeData::ShapeData(const TCollection_AsciiString &theId, const Handle(OSD_FileSystem) & theFileSystem)
        : myId(theId),
          myFileSystem(theFileSystem)
    {
        //
    }

    // =======================================================================
    // function : ShapeData
    // =======================================================================
    ShapeData::ShapeData(const TCollection_AsciiString &theId, const TopoDS_Shape &theShape, const DataLoadState theState)
        : myId(theId),
          myState(theState)
    {
        myRootShapes.Append(theShape);
        UpdateStats();
    }

    // =======================================================================
    // function : ShapeData
    // =======================================================================
    ShapeData::ShapeData(const TCollection_AsciiString &theNewId, const Handle(ShapeData) & theOther)
        : myId(theNewId),
          myRootShapes(theOther->myRootShapes),
          myAttribMap(theOther->myAttribMap),
          myMetadata(theOther->myMetadata),
          myFileSystem(theOther->myFileSystem),
          myBndBox(theOther->myBndBox),
          myState(theOther->myState),
          myNbTris(theOther->myNbTris),
          myNbTriNodes(theOther->myNbTriNodes)
    {
        //
    }

    // =======================================================================
    // function : UpdateStats
    // =======================================================================
    void ShapeData::UpdateStats()
    {
        myBndBox = Bnd_Box();
        myNbTris = 0;
        myNbTriNodes = 0;

        TopLoc_Location aLoc;
        for (const TopoDS_Shape &aRootIter : myRootShapes)
        {
            BRepBndLib::Add(aRootIter, myBndBox);
            for (TopExp_Explorer aFaceIter(aRootIter, TopAbs_FACE); aFaceIter.More(); aFaceIter.Next())
            {
                const TopoDS_Face &aFace = TopoDS::Face(aFaceIter.Value());
                const Handle(Poly_Triangulation) &aTris = BRep_Tool::Triangulation(aFace, aLoc);
                if (aTris.IsNull())
                {
                    continue;
                }

                if (aTris->HasGeometry())
                {
                    myNbTris += aTris->NbTriangles();
                    myNbTriNodes += aTris->NbNodes();
                }
                else
                {
                    myNbTris += aTris->NbDeferredTriangles();
                    myNbTriNodes += aTris->NbDeferredNodes();
                }
            }
        }
    }

    // =======================================================================
    // function : GetSingleShape
    // =======================================================================
    TopoDS_Shape ShapeData::GetSingleShape(bool theToKeepList) const
    {
        if (myRootShapes.IsEmpty())
        {
            return TopoDS_Shape();
        }
        if (!theToKeepList && myRootShapes.Length() == 1)
        {
            return myRootShapes.First();
        }

        TopoDS_Compound aComp;
        BRep_Builder().MakeCompound(aComp);
        for (const TopoDS_Shape &aRootIter : myRootShapes)
        {
            BRep_Builder().Add(aComp, aRootIter);
        }
        return aComp;
    }

    //! Avoid filter for getting free shapes only by explorer.
    static TopAbs_ShapeEnum GetFreeSubshapeAvoidType(const TopAbs_ShapeEnum theSubshapeType)
    {
        switch (theSubshapeType)
        {
        case TopAbs_SOLID:
            return TopAbs_COMPSOLID;
        case TopAbs_SHELL:
            return TopAbs_SOLID;
        case TopAbs_FACE:
            return TopAbs_SHELL;
        case TopAbs_WIRE:
            return TopAbs_FACE;
        case TopAbs_EDGE:
            return TopAbs_WIRE;
        case TopAbs_VERTEX:
            return TopAbs_EDGE;
        default:
            return TopAbs_SHAPE;
        }
    }

    // =======================================================================
    // function : GetSubshapesIndexes
    // =======================================================================
    Handle(TColStd_HPackedMapOfInteger) ShapeData::GetSubshapesIndexes(const TopAbs_ShapeEnum theType,
                                                                       const bool theIsFree,
                                                                       const TopoDS_Shape &theParent,
                                                                       const int theVisibility) const
    {
        Handle(TColStd_HPackedMapOfInteger) aMap = new TColStd_HPackedMapOfInteger();

        Handle(NCollection_IncAllocator) anAllocator = new NCollection_IncAllocator();

        const TopAbs_ShapeEnum anIgnoreType = theIsFree ? GetFreeSubshapeAvoidType(theType) : TopAbs_SHAPE;

        TopTools_MapOfShape aFilter(0, anAllocator);
        if (!theParent.IsNull())
        {
            for (const TopoDS_Shape &anElemIter : TopExp_Explorer(theParent, theType, anIgnoreType))
            {
                aFilter.Add(anElemIter);
            }
            if (aFilter.IsEmpty())
            {
                return aMap;
            }
        }

        TopTools_MapOfShape aFreeMap(0, anAllocator);
        if (theIsFree)
        {
            for (const TopoDS_Shape &aRootIter : myRootShapes)
            {
                for (const TopoDS_Shape &anElemIter : TopExp_Explorer(aRootIter, theType, anIgnoreType))
                {
                    aFreeMap.Add(anElemIter);
                }
            }
            if (aFreeMap.IsEmpty())
            {
                return aMap;
            }
        }

        int anElemCounter = 0;
        for (const TopoDS_Shape &aRootIter : myRootShapes)
        {
            for (TopExp_Explorer anElemIter(aRootIter, theType); anElemIter.More(); anElemIter.Next(), ++anElemCounter)
            {
                if (theIsFree && !aFreeMap.Contains(anElemIter.Current()))
                {
                    // check free shapes first
                    continue;
                }
                else if (!theParent.IsNull() && !aFilter.Remove(anElemIter.Current()))
                {
                    // add only subshapes of specified parent
                    continue;
                }
                else if (theParent.IsNull() && !aFilter.Add(anElemIter.Current()))
                {
                    // add subshape once
                    continue;
                }
                else if (theVisibility != -1 && IsSubshapeHidden(anElemIter.Current()) != (theVisibility == 0))
                {
                    // visible/hidden filter
                    continue;
                }

                aMap->ChangeMap().Add(anElemCounter);
            }
        }
        return aMap;
    }

    // =======================================================================
    // function : GetSubshapes
    // =======================================================================
    bool ShapeData::GetSubshapes(std::vector<EnumeratedShape> &theShapes,
                                 const TopAbs_ShapeEnum theType,
                                 const Handle(TColStd_HPackedMapOfInteger) & theIndices) const
    {
        theShapes.clear();
        if (theIndices.IsNull() || theIndices->Map().IsEmpty())
        {
            return true;
        }

        theShapes.reserve(theIndices->Map().Extent());
        int anElemCounter = 0;
        for (const TopoDS_Shape &aRootIter : myRootShapes)
        {
            for (TopExp_Explorer anElemIter(aRootIter, theType); anElemIter.More(); anElemIter.Next(), ++anElemCounter)
            {
                if (theIndices->Map().Contains(anElemCounter))
                {
                    EnumeratedShape aShape;
                    aShape.Shape = anElemIter.Value();
                    aShape.Index = anElemCounter;
                    theShapes.push_back(aShape);
                }
            }
        }

        if ((int)theShapes.size() == theIndices->Map().Extent())
        {
            return true;
        }

        TColStd_PackedMapOfInteger aFoundMap;
        for (const EnumeratedShape &aShapeIter : theShapes)
        {
            aFoundMap.Add(aShapeIter.Index);
        }
        TColStd_PackedMapOfInteger aMap;
        aMap.Subtraction(theIndices->Map(), aFoundMap);

        std::string aStr;
        for (TColStd_PackedMapOfInteger::Iterator anIndexIter(aMap); anIndexIter.More(); anIndexIter.Next())
        {
            if (!aStr.empty())
            {
                aStr += ",";
            }
            aStr += std::to_string(anIndexIter.Key());
        }

        Message::SendFail() << "Error: ShapeData::GetSubshapes(), invalid " << TopAbs::ShapeTypeToString(theType)
                            << " indexes [" << aStr << "] have been specified";
        return false;
    }

    // =======================================================================
    // function : GetSubshape
    // =======================================================================
    TopoDS_Shape ShapeData::GetSubshape(const TopAbs_ShapeEnum theType, const int theIndex) const
    {
        Graphic3d_Vec2i aRange(theIndex, theIndex);
        for (ShapeDataExplorer anElemIter(myRootShapes, mySubshapesBounds, aRange, theType); anElemIter.More();
             anElemIter.Next())
        {
            if (anElemIter.Index() == theIndex)
            {
                return anElemIter.Value();
            }
        }
        return TopoDS_Shape();
    }

    // =======================================================================
    // function : IsSubshapeHidden
    // =======================================================================
    bool ShapeData::IsSubshapeHidden(const TopAbs_ShapeEnum theType, const int theIndex, const float theMinOpacity) const
    {
        TopoDS_Shape aSubshape = GetSubshape(theType, theIndex);
        if (aSubshape.IsNull())
        {
            Message::SendFail() << "Error: IsSubshapeHidden(" << TopAbs::ShapeTypeToString(theType) << ", " << theIndex
                                << ") called for non-existing subshape index";
            return false;
        }

        return IsSubshapeHidden(aSubshape, theMinOpacity);
    }

    // =======================================================================
    // function : IsSubshapeHidden
    // =======================================================================
    bool ShapeData::IsSubshapeHidden(const TopoDS_Shape &theShape, const float theMinOpacity) const
    {
        if (theShape.IsNull())
        {
            return false;
        }

        const RWMesh_NodeAttributes *anAttribsForHidden = myAttribMap.Seek(theShape);
        if (anAttribsForHidden == nullptr || anAttribsForHidden->Style.IsEmpty())
        {
            anAttribsForHidden = myAttribMap.Seek(theShape.Located(TopLoc_Location()));
        }
        if (anAttribsForHidden == nullptr || anAttribsForHidden->Style.IsEmpty())
        {
            return false;
        }

        if (!anAttribsForHidden->Style.IsVisible())
        {
            return true;
        }
        if (anAttribsForHidden->Style.IsSetColorSurf())
        {
            return anAttribsForHidden->Style.GetColorSurfRGBA().Alpha() < theMinOpacity;
        }
        else if (!anAttribsForHidden->Style.Material().IsNull())
        {
            return anAttribsForHidden->Style.Material()->BaseColor().Alpha() < theMinOpacity;
        }

        return false;
    }

    //! Format index map into string.
    static TCollection_AsciiString IndexMapToString(const Handle(TColStd_HPackedMapOfInteger) & theMap)
    {
        if (theMap.IsNull())
        {
            return TCollection_AsciiString();
        }

        TCollection_AsciiString aList;
        for (TColStd_PackedMapOfInteger::Iterator anIter(theMap->Map()); anIter.More(); anIter.Next())
        {
            if (!aList.IsEmpty())
            {
                aList.AssignCat(",");
            }
            aList += anIter.Key();
        }
        return aList;
    }

    // =======================================================================
    // function : SetSubshapeHidden
    // =======================================================================
    bool ShapeData::SetSubshapeHidden(const TopAbs_ShapeEnum theType,
                                      const Handle(TColStd_HPackedMapOfInteger) & theIndices,
                                      const bool theIsHidden,
                                      const bool theToInvertOthers)
    {
        bool isModified = false;
        int aNbFound = 0;
        int anElemCounter = 0;

        TopTools_MapOfShape aMap;
        for (const TopoDS_Shape &aRootIter : myRootShapes)
        {
            for (TopExp_Explorer anElemIter(aRootIter, theType); anElemIter.More(); anElemIter.Next(), ++anElemCounter)
            {
                bool toHide = !theIsHidden;
                if (theIndices.IsNull() || theIndices->Map().Contains(anElemCounter))
                {
                    toHide = !toHide;
                    ++aNbFound;
                }
                else if (!theToInvertOthers)
                {
                    continue;
                }

                if (!aMap.Add(anElemIter.Value()))
                {
                    continue; // avoid double updates of the same subshape
                }
                isModified = SetSubshapeHidden(anElemIter.Value(), toHide) || isModified;
            }
        }
        if (!theIndices.IsNull() && aNbFound != theIndices->Map().Extent())
        {
            Message::SendFail() << "Error: SetSubshapeHidden(" << TopAbs::ShapeTypeToString(theType) << ", ["
                                << IndexMapToString(theIndices) << "]"
                                << ") called for non-existing subshape indexes";
        }
        return isModified;
    }

    // =======================================================================
    // function : SetSubshapeHidden
    // =======================================================================
    bool ShapeData::SetSubshapeHidden(const TopoDS_Shape &theSubshape, const bool theIsHidden)
    {
        RWMesh_NodeAttributes *anAttribs = myAttribMap.ChangeSeek(theSubshape);
        if (anAttribs == nullptr)
        {
            anAttribs = myAttribMap.ChangeSeek(theSubshape.Located(TopLoc_Location()));
        }
        if (anAttribs == nullptr)
        {
            if (!theIsHidden)
            {
                return false;
            }
            anAttribs = myAttribMap.Bound(theSubshape, RWMesh_NodeAttributes());
        }

        bool isChanged = (anAttribs->Style.IsVisible() != !theIsHidden);
        anAttribs->Style.SetVisibility(!theIsHidden);
        if (!theIsHidden && anAttribs->Style.IsSetColorSurf() && anAttribs->Style.GetColorSurfRGBA().Alpha() < ShortRealSmall())
        {
            anAttribs->Style.SetColorSurf(Quantity_ColorRGBA(anAttribs->Style.GetColorSurf(), 1.0f));
            isChanged = true;
        }
        return isChanged;
    }

    // =======================================================================
    // function : GetSubshapeColor
    // =======================================================================
    bool ShapeData::GetSubshapeColor(Quantity_ColorRGBA &theColor, const TopAbs_ShapeEnum theType, const int theIndex) const
    {
        TopoDS_Shape aSubshape = GetSubshape(theType, theIndex);
        if (aSubshape.IsNull())
        {
            Message::SendFail() << "Error: GetSubshapeColor(" << TopAbs::ShapeTypeToString(theType) << ", " << theIndex
                                << ") called for non-existing subshape index";
            return false;
        }

        return GetSubshapeColor(theColor, aSubshape);
    }

    // =======================================================================
    // function : GetSubshapeColor
    // =======================================================================
    bool ShapeData::GetSubshapeColor(Quantity_ColorRGBA &theColor, const TopoDS_Shape &theShape) const
    {
        if (theShape.IsNull())
        {
            return false;
        }

        const RWMesh_NodeAttributes *anAttribsForColor = myAttribMap.Seek(theShape);
        if (anAttribsForColor == nullptr || anAttribsForColor->Style.IsEmpty())
        {
            anAttribsForColor = myAttribMap.Seek(theShape.Located(TopLoc_Location()));
        }
        if (anAttribsForColor == nullptr || anAttribsForColor->Style.IsEmpty())
        {
            return false;
        }

        if (anAttribsForColor->Style.IsSetColorSurf())
        {
            theColor = anAttribsForColor->Style.GetColorSurfRGBA();
            return true;
        }
        else if (!anAttribsForColor->Style.Material().IsNull())
        {
            theColor = anAttribsForColor->Style.Material()->BaseColor();
            return true;
        }

        return false;
    }

    // =======================================================================
    // function : SetSubshapeColor
    // =======================================================================
    bool ShapeData::SetSubshapeColor(const Quantity_ColorRGBA *theColor,
                                     const TopAbs_ShapeEnum theType,
                                     const Handle(TColStd_HPackedMapOfInteger) & theIndices)
    {
        if (theIndices.IsNull())
        {
            TopTools_IndexedMapOfShape aMap;
            for (const TopoDS_Shape &aRootIter : myRootShapes)
            {
                TopExp::MapShapes(aRootIter, theType, aMap);
            }

            bool isModified = false;
            for (TopTools_IndexedMapOfShape::Iterator aShapeIter(aMap); aShapeIter.More(); aShapeIter.Next())
            {
                isModified = SetSubshapeColor(aShapeIter.Value(), theColor) || isModified;
            }
            return isModified;
        }

        bool isModified = false;
        for (TColStd_PackedMapOfInteger::Iterator aShapeIter(theIndices->Map()); aShapeIter.More(); aShapeIter.Next())
        {
            TopoDS_Shape aSubshape = GetSubshape(theType, aShapeIter.Key());
            if (aSubshape.IsNull())
            {
                Message::SendFail() << "Error: SetSubshapeColor(" << TopAbs::ShapeTypeToString(theType) << ", "
                                    << aShapeIter.Key() << ") called for non-existing subshape index";
                return false;
            }
            isModified = SetSubshapeColor(aSubshape, theColor) || isModified;
        }
        return isModified;
    }

    // =======================================================================
    // function : SetSubshapeColor
    // =======================================================================
    bool ShapeData::SetSubshapeColor(const TopoDS_Shape &theSubshape, const Quantity_ColorRGBA *theColor)
    {
        RWMesh_NodeAttributes *anAttribs = myAttribMap.ChangeSeek(theSubshape);
        if (anAttribs == nullptr)
        {
            anAttribs = myAttribMap.ChangeSeek(theSubshape.Located(TopLoc_Location()));
        }
        if (anAttribs == nullptr)
        {
            if (theColor == nullptr)
            {
                return false;
            }
            anAttribs = myAttribMap.Bound(theSubshape, RWMesh_NodeAttributes());
        }

        if (theColor == nullptr)
        {
            if (!anAttribs->Style.IsSetColorSurf() && anAttribs->Style.Material().IsNull())
            {
                return false;
            }

            anAttribs->Style.UnSetColorSurf();
            anAttribs->Style.SetMaterial(Handle(XCAFDoc_VisMaterial)());
            return true;
        }

        if (anAttribs->Style.IsSetColorSurf() && anAttribs->Style.GetColorSurfRGBA().IsEqual(*theColor))
        {
            return false;
        }

        anAttribs->Style.SetColorSurf(*theColor);
        return true;
    }

    // ================================================================
    // Function : SetShape
    // ================================================================
    void ShapeData::SetShape(const TopoDS_Shape &theShape)
    {
        myRootShapes.Clear();
        TopoDS_Shape aRoot = theShape;
        // TO DO: it should be done in scope of the first transformation change for example
        // set location of root shape to presentation and reset location of shape
        // @c SetLocalTransformation(theShape.Location());
        // @c aRoot.Location(TopLoc_Location());
        myRootShapes.Append(aRoot);

        // copy attributes of original located shape
        const RWMesh_NodeAttributes *anAttribs1 = myAttribMap.Seek(theShape);
        const RWMesh_NodeAttributes *anAttribs2 = myAttribMap.Seek(aRoot);
        if (anAttribs1 != anAttribs2 && anAttribs1 != nullptr)
        {
            myAttribMap.Bind(aRoot, *anAttribs1);
        }

        mySubshapesBounds.Clear();

        UpdateStats();
    }

    // =======================================================================
    // function : GetFaces
    // =======================================================================
    int ShapeData::GetFaces(TopTools_IndexedMapOfShape &theShapeMap) const
    {
        TopLoc_Location aDummyLoc;
        TopExp_Explorer aFaceIter;
        int aNbDeferred = 0;
        for (const TopoDS_Shape &aRootIter : myRootShapes)
        {
            for (aFaceIter.Init(aRootIter, TopAbs_FACE); aFaceIter.More(); aFaceIter.Next())
            {
                TopoDS_Shape aFace = aFaceIter.Value().Located(TopLoc_Location());

                const int anExtent = theShapeMap.Extent();
                const bool wasAdded = theShapeMap.Add(aFace) != anExtent;

                const Handle(Poly_Triangulation) &aTris = BRep_Tool::Triangulation(TopoDS::Face(aFace), aDummyLoc);
                if (wasAdded && !aTris.IsNull() && !aTris->HasGeometry() && aTris->HasDeferredData())
                {
                    ++aNbDeferred;
                }
            }
        }
        return aNbDeferred;
    }

    // =======================================================================
    // function : GetSubShapesBoundsMap
    // =======================================================================
    const SubShapesBoundsMap &ShapeData::GetSubShapesBoundsMap(bool theToUpdate)
    {
        if (!theToUpdate)
        {
            return mySubshapesBounds;
        }

        mySubshapesBounds.Clear();

        // Step 1 - gather subshapes that have all children > SHAPE_BOUNDS_LOWER_TYPE
        SubShapesBoundsIndexedMap aLowerShapes;
        GatherLowerShapes(myRootShapes, aLowerShapes);

        // Step 2 - gather (in parallel) bounds of subshapes that have all children > SHAPE_BOUNDS_LOWER_TYPE
        GatherLowerShapesBounds(aLowerShapes);

        // Step 3 - gather bounds of remaining subshapes
        ComputeShapesBounds(myRootShapes, aLowerShapes, mySubshapesBounds);

        return mySubshapesBounds;
    }

} // namespace