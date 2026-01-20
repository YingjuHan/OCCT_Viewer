#include <RWShapeConv/ShapeLoader.h>

#include <RWShape/GltfParser.h>

#include <RWBase/FileSystemMap.h>
#include <RWBase/TaskManager.h>

#include <BinTools.hxx>
#include <BinTools_ShapeReader.hxx>
#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#ifdef OCCTV_USE_STEP
  #include <STEPControl_Reader.hxx>
  #include <IFSelect_ReturnStatus.hxx>
#endif
#ifdef OCCTV_USE_IGES
  #include <IGESControl_Reader.hxx>
  #include <IFSelect_ReturnStatus.hxx>
#endif
#include <Message.hxx>
#include <OSD_MemInfo.hxx>
#include <OSD_Timer.hxx>
#include <RWGltf_TriangulationReader.hxx>
#include <RWStl.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Iterator.hxx>

namespace
{

    //! Function to check if specified data stream starts with specified header.
    template <unsigned M>
    static bool IsDataStartsWithHeader(const char (&theData)[2048], const char (&theHeader)[M])
    {
        return ::strncmp(theData, theHeader, strlen(theHeader)) == 0;
    }
}

namespace RWShapeConv
{

    //! Internal structure to define explode parameters.
    struct ExplodeContext
    {
        Handle(OSD_FileSystem) fileSystem;
        const RWMesh_NodeAttributeMap *rootAttribMap = nullptr;
        NCollection_List<Handle(RWShape::ShapeData)> objectList;
        NCollection_Map<TCollection_AsciiString> mapIds;

        RWShape::DataLoadState parentState = RWShape::DataLoadState_Initial;

        bool isAutoNaming = true;
        bool toGenId = true;
        bool toPropagateId = true;

    public:
        //! Return node attributes.
        static const RWMesh_NodeAttributes *FindAttribs(const RWMesh_NodeAttributeMap *theAttribMap,
                                                        const TopoDS_Shape &theShape)
        {
            if (theAttribMap == nullptr || theAttribMap->IsEmpty())
            {
                return nullptr;
            }
            const RWMesh_NodeAttributes *anAttribs = theAttribMap->Seek(theShape);
            if (anAttribs == NULL)
            {
                anAttribs = theAttribMap->Seek(theShape.Located(TopLoc_Location()));
            }
            return anAttribs;
        }

        //! Save children shape attributes to current map
        static void PropagateAttributes(const TopoDS_Shape &theShape,
                                        const RWMesh_NodeAttributeMap *theRootAttribMap,
                                        RWMesh_NodeAttributeMap &theCurrentAttribMap)

        {
            for (TopoDS_Iterator aSubShapeIter(theShape, true, false); aSubShapeIter.More(); aSubShapeIter.Next())
            {
                const TopoDS_Shape &aSubShape = aSubShapeIter.Value();
                if (const RWMesh_NodeAttributes *anAttribs = FindAttribs(theRootAttribMap, aSubShape))
                {
                    theCurrentAttribMap.Bind(aSubShape.Located(TopLoc_Location()), *anAttribs);
                }
                PropagateAttributes(aSubShape, theRootAttribMap, theCurrentAttribMap);
            }
        }

    public:
        //! Constructor.
        explicit ExplodeContext(const Handle(OSD_FileSystem) & theFileSystem) : fileSystem(theFileSystem) {}

        //! Constructor.
        ExplodeContext() {}

        //! Validate inputs.
        bool Init(const Handle(RWShape::ShapeData) & theRootShapeOwner,
                  const ShapeLoader::ExplodeParams &theExpParams,
                  const bool theIsDocRoot)
        {
            if (theExpParams.explodeDepth < -1)
            {
                Message::SendFail("Error: invalid negative explode depth");
                return false;
            }
            if (theRootShapeOwner.IsNull())
            {
                return false;
            }
            if (theRootShapeOwner->Id().IsEmpty())
            {
                if (theRootShapeOwner->AttributeMap().IsEmpty() && !theExpParams.isAutoNaming)
                {
                    Message::SendFail("Error: shape name cannot be empty in case of attributes absence and "
                                      "not auto naming");
                    return false;
                }
                if (!theIsDocRoot)
                {
                    Message::SendFail("Error: shape name cannot be empty to be exploded deeper");
                    return false;
                }
            }

            fileSystem = theRootShapeOwner->FileSystem();
            rootAttribMap = &theRootShapeOwner->AttributeMap();
            parentState = theRootShapeOwner->State();

            isAutoNaming = theExpParams.isAutoNaming;
            toGenId = theExpParams.toGenerateId;
            toPropagateId = theExpParams.toPropagateId;
            return true;
        }

        //! Explode shape to sub-shapes if it is possible.
        NCollection_List<Handle(RWShape::ShapeData)> Explode(const Handle(RWShape::ShapeData) & theRootShapeOwner,
                                                             const ShapeLoader::ExplodeParams &theExpParams,
                                                             const bool theIsDocRoot = true)
        {
            if (!Init(theRootShapeOwner, theExpParams, theIsDocRoot))
            {
                return NCollection_List<Handle(RWShape::ShapeData)>();
            }

            if (rootAttribMap->IsEmpty() && !theExpParams.isAutoNaming)
            {
                // only add root object for the root shape to result map
                NCollection_List<Handle(RWShape::ShapeData)> aResList;
                aResList.Append(theRootShapeOwner);
                return aResList;
            }

            int aSubShapeCounter = 0;
            for (TopTools_SequenceOfShape::Iterator aShapeIter(theRootShapeOwner->Shapes()); aShapeIter.More();
                 aShapeIter.Next(), ++aSubShapeCounter)
            {
                const TopoDS_Shape &aSubShape = aShapeIter.Value();
                TCollection_AsciiString aCurrentId = theRootShapeOwner->Id();
                if (theIsDocRoot)
                {
                    aCurrentId += TCollection_AsciiString(!theRootShapeOwner->Id().IsEmpty() ? ShapeLoader::THE_ID_SEPARATOR
                                                                                             : '\0');
                    if (!rootAttribMap->IsEmpty())
                    {
                        if (const RWMesh_NodeAttributes *anAttribs = FindAttribs(rootAttribMap, aSubShape))
                        {
                            aCurrentId += anAttribs->Name;
                        }
                    }
                    else if (theExpParams.isAutoNaming)
                    {
                        aCurrentId += TCollection_AsciiString(TopAbs::ShapeTypeToString(aSubShape.ShapeType())) + ShapeLoader::THE_COUNTER_SEPARATOR + aSubShapeCounter;
                    }
                }
                if (!ProcessShape(aCurrentId, aSubShape, theExpParams.explodeDepth))
                {
                    // only add root object for the root shape to result map
                    NCollection_List<Handle(RWShape::ShapeData)> aResList;
                    aResList.Append(theRootShapeOwner);
                    return aResList;
                }
            }
            return objectList;
        }

        //! Create shape interactive object.
        Handle(RWShape::ShapeData) AddShape(const TCollection_AsciiString &theObjId,
                                            const TopoDS_Shape &theShape,
                                            const RWShape::DataLoadState theState = RWShape::DataLoadState_Initial,
                                            const RWMesh_NodeAttributes *theShapeAttribs = nullptr);

        //! Create shape interactive object by given parameters.
        bool AddObject(const TCollection_AsciiString &theObjId,
                       const TopoDS_Shape &theShape,
                       const RWMesh_NodeAttributes *theShapeAttribs = nullptr);

        //! Recursive procedure to explode shape to sub-shapes.
        bool ProcessShape(const TCollection_AsciiString &theCurrentId,
                          const TopoDS_Shape &theShape,
                          const int theExplodeDepth = -1);

        //! Recursive procedure to explode shape to sub-shapes.
        bool ProcessNamedShape(const TCollection_AsciiString &theCurrentId,
                               const TopoDS_Shape &theShape,
                               const int theExplodeDepth);
    };

    //! Create shape object.
    Handle(RWShape::ShapeData) ExplodeContext::AddShape(const TCollection_AsciiString &theObjId,
                                                        const TopoDS_Shape &theShape,
                                                        const RWShape::DataLoadState theState,
                                                        const RWMesh_NodeAttributes *theShapeAttribs)

    {
        Handle(RWShape::ShapeData) aShapeOwner = new RWShape::ShapeData(theObjId, fileSystem);
        aShapeOwner->SetShape(theShape);
        aShapeOwner->SetState(theState);
        if (theShapeAttribs != nullptr)
        {
            aShapeOwner->ChangeAttributeMap().Bind(theShape, *theShapeAttribs);
        }
        if (rootAttribMap != nullptr && !rootAttribMap->IsEmpty())
        {
            // set materials for subshapes
            PropagateAttributes(theShape, rootAttribMap, aShapeOwner->ChangeAttributeMap());
        }
        return aShapeOwner;
    }

    //! Create shape interactive object by given parameters.
    bool ExplodeContext::AddObject(const TCollection_AsciiString &theObjId,
                                   const TopoDS_Shape &theShape,
                                   const RWMesh_NodeAttributes *theShapeAttribs)

    {
        TCollection_AsciiString anObjId = theObjId;
        if (toGenId)
        {
            // Guarantee unique name
            int aCounter = 0;
            while (mapIds.Contains(anObjId))
            {
                anObjId = theObjId + TCollection_AsciiString(ShapeLoader::THE_COUNTER_SEPARATOR) + (++aCounter);
            }
        }
        else if (mapIds.Contains(anObjId))
        {
            Message::SendFail() << "Error: there are several objects with the same name '" << anObjId << "'";
            return false;
        }

        // add shape to view
        mapIds.Add(anObjId);
        objectList.Append(AddShape(anObjId, theShape, parentState, theShapeAttribs));
        return true;
    }

    //! Recursive procedure to explode shape to sub-shapes.
    bool ExplodeContext::ProcessNamedShape(const TCollection_AsciiString &theCurrentId,
                                           const TopoDS_Shape &theShape,
                                           const int theExplodeDepth)
    {
        int aSubCounter = 0;
        for (TopoDS_Iterator aSubShapeIter(theShape, true, false); aSubShapeIter.More();
             aSubShapeIter.Next(), ++aSubCounter)
        {
            TopoDS_Shape aSubShape = aSubShapeIter.Value();
            TCollection_AsciiString aLastId;

            const RWMesh_NodeAttributes *anAttribs = ExplodeContext::FindAttribs(rootAttribMap, aSubShape);
            if (rootAttribMap != nullptr && !rootAttribMap->IsEmpty())
            {
                // find RWMesh_NodeAttributeMap for current sub shape
                if (anAttribs != nullptr)
                {
                    aLastId = toPropagateId ? theCurrentId + ShapeLoader::THE_ID_SEPARATOR + anAttribs->Name
                                            : anAttribs->Name;
                }
                else
                {
                    Message::SendWarning() << "Subshape of shape '" << theCurrentId << "' not found in the map";
                }
            }
            else if (isAutoNaming)
            {
                aLastId = theCurrentId + ShapeLoader::THE_ID_SEPARATOR + TopAbs::ShapeTypeToString(aSubShape.ShapeType()) + ShapeLoader::THE_COUNTER_SEPARATOR + aSubCounter;
            }
            if (aLastId.IsEmpty())
            {
                continue;
            }

            aSubShape.Move(theShape.Location(), Standard_False);
            if (aSubShape.ShapeType() == TopAbs_COMPOUND)
            {
                if (!ProcessShape(aLastId, aSubShape, theExplodeDepth >= 0 ? theExplodeDepth - 1 : theExplodeDepth))
                {
                    return false;
                }
            }
            else if (!AddObject(aLastId, aSubShape, anAttribs))
            {
                return false;
            }
        }
        return true;
    }

    //! Recursive procedure to explode shape to sub-shapes.
    bool ExplodeContext::ProcessShape(const TCollection_AsciiString &theCurrentId,
                                      const TopoDS_Shape &theShape,
                                      const int theExplodeDepth)
    {
        // check if current shape is compound
        if (theShape.ShapeType() != TopAbs_COMPOUND || theExplodeDepth == 0)
        {
            const RWMesh_NodeAttributes *anAttribs = FindAttribs(rootAttribMap, theShape);
            return AddObject(theCurrentId, theShape, anAttribs);
        }

        bool aNamedObjects = false;
        if (rootAttribMap != nullptr && !rootAttribMap->IsEmpty())
        {
            // check if subshapes are named objects
            for (TopoDS_Iterator aSubShapeIter(theShape, false, false); aSubShapeIter.More(); aSubShapeIter.Next())
            {
                const TopoDS_Shape &aSubShape = aSubShapeIter.Value();

                const RWMesh_NodeAttributes *anAttribs = FindAttribs(rootAttribMap, aSubShape);
                if (anAttribs == nullptr)
                {
                    Message::SendWarning() << "Sub shape of shape '" << theCurrentId << "' not found in the map";
                    aNamedObjects = false;
                    break;
                }
                else if (anAttribs->Name.IsEmpty())
                {
                    aNamedObjects = false;
                    break;
                }

                aNamedObjects = true;
            }
        }
        else
        {
            aNamedObjects = isAutoNaming;
        }

        // call recursively for compound shapes
        if (aNamedObjects)
        {
            return ProcessNamedShape(theCurrentId, theShape, theExplodeDepth);
        }

        const RWMesh_NodeAttributes *anAttribs = FindAttribs(rootAttribMap, theShape);
        return AddObject(theCurrentId, theShape, anAttribs);
    }

    //! Convert triangulation from double precision to single precision.
    static void ShrinkTessellationPrecision(const TopoDS_Shape &theShape)
    {
        TopLoc_Location aLoc;
        int aNbFaces = 0;
        for (const TopoDS_Face &aFaceIt : TopExp_ExplorerT<TopoDS_Face>(theShape))
        {
            const Handle(Poly_Triangulation) &aTris = BRep_Tool::Triangulation(aFaceIt, aLoc);
            if (aTris.IsNull() || !aTris->IsDoublePrecision())
            {
                continue;
            }

            Poly_ArrayOfNodes aNewNodes;
            aNewNodes.SetDoublePrecision(false);
            aNewNodes.Resize(aTris->NbNodes(), false);
            for (int aNodeIter = 0; aNodeIter < aTris->NbNodes(); ++aNodeIter)
            {
                aNewNodes.SetValue(aNodeIter, aTris->Node(aNodeIter + 1));
            }
            aTris->InternalNodes().Move(aNewNodes);
            ++aNbFaces;
        }

        int aNbEdges = 0;
        for (const TopoDS_Edge &anEdgeIt : TopExp_ExplorerT<TopoDS_Edge>(theShape))
        {
            const Handle(Poly_Polygon3D) &aPoly = BRep_Tool::Polygon3D(anEdgeIt, aLoc);
            if (aPoly.IsNull())
            {
                continue;
            }

            // conversion is not supported yet
            ++aNbEdges;
        }

        (void)aNbFaces;
        (void)aNbEdges;
    }

    // ================================================================
    // Function : OpenBrepShape
    // ================================================================
    bool ShapeLoader::OpenBrepShape(NCollection_List<Handle(RWShape::ShapeData)> &theShapeList,
                                    const TCollection_AsciiString &theName,
                                    const Handle(OSD_FileSystem) & theFileSystem,
                                    const std::string &theFilePath,
                                    const OcctBRepFormat theFormat)
    {
        OSD_Timer aReadTimer;
        aReadTimer.Start();

        std::shared_ptr<std::istream> aFile = theFileSystem->OpenIStream(theFilePath.c_str(),
                                                                         std::ios::in | std::ios::binary);
        if (aFile.get() == nullptr)
        {
            Message::SendFail() << "[ERR_SHAPE_LOAD] unable to open file '" << theFilePath << "'";
            return false;
        }

        TopoDS_Shape aShape;
        BRep_Builder aBuilder;
        switch (theFormat)
        {
        case OcctBRepFormat::Ascii:
            BRepTools::Read(aShape, *aFile, aBuilder);
            break;
        case OcctBRepFormat::Binary:
            BinTools::Read(aShape, *aFile);
            break;
        case OcctBRepFormat::SBinary:
        {
            BinTools_ShapeReader aReader;
            aReader.Read(*aFile, aShape);
            break;
        }
        }
        if (myForceSinglePrec)
        {
            ShrinkTessellationPrecision(aShape);
        }

        if (myExpParams.toExplode)
        {
            Handle(RWShape::ShapeData) aRootOwner = new RWShape::ShapeData(myExpParams.toUsePrefix ? theName : "",
                                                                           theFileSystem);
            aRootOwner->SetShape(aShape);

            ExplodeContext anExpTool;
            theShapeList = anExpTool.Explode(aRootOwner, myExpParams);
        }
        else
        {
            TCollection_AsciiString aName = theName;
            if (aName.IsEmpty())
            {
                TCollection_AsciiString aFileFolder;
                OSD_Path::FolderAndFileFromPath(theFilePath.c_str(), aFileFolder, aName);
            }

            ExplodeContext anExpTool(theFileSystem);

            Handle(RWShape::ShapeData) aShapeIO = anExpTool.AddShape(aName, aShape);
            // @c aShapeIO->MeshOwner()->SetState (RWBase::DataLoadState_Loaded); // no deferred loading
            theShapeList.Append(aShapeIO);
        }
        if (theShapeList.IsEmpty())
        {
            return false;
        }

        Standard_CString aBrepFormat = "ascii";
        aBrepFormat = theFormat == OcctBRepFormat::Binary ? "binary" : aBrepFormat;
        aBrepFormat = theFormat == OcctBRepFormat::SBinary ? "sbinary" : aBrepFormat;

        Message::SendInfo() << "Loaded " << aBrepFormat << " BREP file '" << theFilePath << "' in "
                            << aReadTimer.ElapsedTime() << " s";
        Message::DefaultMessenger()->Send(OSD_MemInfo::PrintInfo(), Message_Trace);
        return true;
    }

    // ================================================================
    // Function : OpenGltfShape
    // ================================================================
    bool ShapeLoader::OpenGltfShape(NCollection_List<Handle(RWShape::ShapeData)> &theShapeList,
                                    const TCollection_AsciiString &theName,
                                    const Handle(OSD_FileSystem) & theFileSystem,
                                    const std::string &theFilePath)
    {
        TCollection_AsciiString aName = theName;
        if (aName.IsEmpty() && !myExpParams.toExplode)
        {
            TCollection_AsciiString aFileFolder;
            OSD_Path::FolderAndFileFromPath(theFilePath.c_str(), aFileFolder, aName);
        }

        RWMesh_CoordinateSystemConverter aTrsf;
        aTrsf.SetInputLengthUnit(1.0); // meters
        aTrsf.SetInputCoordinateSystem(RWMesh_CoordinateSystem_glTF);
        // @c aTrsf.SetOutputLengthUnit(0.001); // mm
        aTrsf.SetOutputLengthUnit(mySysLenUnitScale);
        aTrsf.SetOutputCoordinateSystem(RWMesh_CoordinateSystem_Zup);

        Handle(RWShape::ShapeData) aRootOwner = new RWShape::ShapeData(myExpParams.toUsePrefix ? aName : "",
                                                                       theFileSystem);
        RWShape::GltfParser aParser(aRootOwner->ChangeShapes());
        aParser.SetFilePath(theFilePath.c_str());
        aParser.SetErrorPrefix(TCollection_AsciiString("File '") + theFilePath.c_str() + "' defines invalid glTF!\n");
        aParser.SetAttributeMap(aRootOwner->ChangeAttributeMap());
        aParser.SetMetadata(aRootOwner->ChangeMetadata());
        aParser.SetCoordinateSystemConverter(aTrsf);
        {
            std::shared_ptr<std::istream> aRootStream = theFileSystem->OpenIStream(theFilePath.c_str(),
                                                                                   std::ios::in | std::ios::binary);
            if (aRootStream.get() == nullptr || !aParser.ParseGltfFromMemory(*aRootStream, theFilePath.c_str()))
            {
                Message::SendFail() << "[ERR_SHAPE_LOAD] invalid glTF file '" << theFilePath << "'";
                return false;
            }

            Handle(RWGltf_TriangulationReader) aReader = new RWGltf_TriangulationReader();
            aReader->SetCoordinateSystemConverter(aTrsf);
            for (TopoDS_Face &aFaceIter : aParser.FaceList())
            {
                TopLoc_Location aDummyLoc;
                Handle(RWGltf_GltfLatePrimitiveArray) aLateData = Handle(RWGltf_GltfLatePrimitiveArray)::DownCast(
                    BRep_Tool::Triangulation(aFaceIter, aDummyLoc));
                aLateData->SetReader(aReader);
            }
        }

        aRootOwner->UpdateStats();
        if (myExpParams.toExplode)
        {
            ExplodeContext anExpTool;
            theShapeList = anExpTool.Explode(aRootOwner, myExpParams);
            return true;
        }

        theShapeList.Append(aRootOwner);
        return true;
    }

    // ================================================================
    // Function : OpenStlShape
    // ================================================================
    bool ShapeLoader::OpenStlShape(NCollection_List<Handle(RWShape::ShapeData)> &theShapeList,
                                   const TCollection_AsciiString &theName,
                                   const Handle(OSD_FileSystem) & theFileSystem,
                                   const std::string &theFilePath)
    {
        OSD_Timer aReadTimer;
        aReadTimer.Start();

        Handle(Poly_Triangulation) aTris = RWStl::ReadFile(theFilePath.c_str());
        if (aTris.IsNull())
        {
            Message::SendFail() << "[ERR_SHAPE_LOAD] file '" << theFilePath << "' cannot be read as STL";
            return false;
        }

        TCollection_AsciiString aName = theName;
        if (aName.IsEmpty())
        {
            TCollection_AsciiString aFileFolder;
            OSD_Path::FolderAndFileFromPath(theFilePath.c_str(), aFileFolder, aName);
        }

        TopoDS_Face aFace;
        BRep_Builder().MakeFace(aFace, aTris);

        ExplodeContext anExpTool(theFileSystem);
        Handle(RWShape::ShapeData) aShapeIO = anExpTool.AddShape(aName, aFace);
        // @c aShapeIO->SetState (RWBase::DataLoadState_Loaded); // no deferred loading
        // @c aShapeIO->SetDefaultMaterial();
        theShapeList.Append(aShapeIO);
        Message::SendInfo() << "Loaded STL file '" << theFilePath << "' in " << aReadTimer.ElapsedTime() << " s";
        Message::DefaultMessenger()->Send(OSD_MemInfo::PrintInfo(), Message_Trace);
        return true;
    }

    // ================================================================
    // Function : ProbeFileFormat
    // ================================================================
    std::string ShapeLoader::ProbeFileFormat(const std::string &theName, const char *theBuffer, int theDataLen)
    {
        TCollection_AsciiString aFileNameLower(theName.c_str());
        aFileNameLower.LowerCase();

        if (theDataLen > 0)
        {
            char aBytes[2048] = {};
            std::copy(theBuffer, theBuffer + Min(theDataLen, (int)sizeof(aBytes)), aBytes);

            if (IsDataStartsWithHeader(aBytes, "DBRep_DrawableShape") ||       //
                IsDataStartsWithHeader(aBytes, "CASCADE Topology V") ||        //
                IsDataStartsWithHeader(aBytes, "\nCASCADE Topology V") ||      //
                IsDataStartsWithHeader(aBytes, "\nOpen CASCADE Topology V") || //
                IsDataStartsWithHeader(aBytes, "StreamBRep V"))
            {
                return "BREP";
            }
            else if (IsDataStartsWithHeader(aBytes, "ISO-10303-21"))
            {
                return "STEP";
            }
            else if (IsDataStartsWithHeader(aBytes, "glTF") || IsDataStartsWithHeader(aBytes, "OCO"))
            {
                return "GLTF";
            }
            else if (IsDataStartsWithHeader(aBytes, "STL Exported by") || IsDataStartsWithHeader(aBytes, "solid"))
            {
                return "STL";
            }
        }
        else if (aFileNameLower.EndsWith(".brep") || aFileNameLower.EndsWith(".sbrep"))
        {
            return "BREP";
        }
        else if (aFileNameLower.EndsWith(".step") || aFileNameLower.EndsWith(".stp"))
        {
            return "STEP";
        }
        else if (aFileNameLower.EndsWith(".iges") || aFileNameLower.EndsWith(".igs"))
        {
            return "IGES";
        }
        else if (aFileNameLower.EndsWith(".gltf") || aFileNameLower.EndsWith(".glb"))
        {
            return "GLTF";
        }

        if (aFileNameLower.EndsWith(".stl"))
        {
            return "STL";
        }

        return "";
    }

    // ================================================================
    // Function : OpenShape
    // ================================================================
    bool ShapeLoader::OpenShape(NCollection_List<Handle(RWShape::ShapeData)> &theShapeList,
                                const TCollection_AsciiString &theName,
                                const Handle(OSD_FileSystem) & theFileSystem,
                                const std::string &theFilePath)
    {
        TCollection_AsciiString aFileNameLower(theFilePath.c_str());
        aFileNameLower.LowerCase();

        // parse header
        char aBytes[2048] = {};
        {
            std::shared_ptr<std::istream> aFile = theFileSystem->OpenIStream(theFilePath.c_str(),
                                                                             std::ios::in | std::ios::binary);
            if (aFile.get() == nullptr)
            {
                Message::SendFail() << "[ERR_SHAPE_LOAD] the file '" << theFilePath << "' cannot be opened";
                return false;
            }
            aFile->read(aBytes, sizeof(aBytes) - 1);
        }

        OSD_Timer aReadTimer;
        aReadTimer.Start();
        if (IsDataStartsWithHeader(aBytes, "DBRep_DrawableShape") ||       //
            IsDataStartsWithHeader(aBytes, "CASCADE Topology V") ||        //
            IsDataStartsWithHeader(aBytes, "\nCASCADE Topology V") ||      //
            IsDataStartsWithHeader(aBytes, "\nOpen CASCADE Topology V") || //
            IsDataStartsWithHeader(aBytes, "StreamBRep V"))
        {
            OcctBRepFormat aBrepFormat = OcctBRepFormat::Ascii;
            aBrepFormat = IsDataStartsWithHeader(aBytes, "\nOpen CASCADE Topology V") ? OcctBRepFormat::Binary
                                                                                      : aBrepFormat;
            aBrepFormat = IsDataStartsWithHeader(aBytes, "StreamBRep V") ? OcctBRepFormat::SBinary : aBrepFormat;
            return OpenBrepShape(theShapeList, theName, theFileSystem, theFilePath, aBrepFormat);
        }
        else if (IsDataStartsWithHeader(aBytes, "glTF") || IsDataStartsWithHeader(aBytes, "OCO"))
        {
            return OpenGltfShape(theShapeList, theName, theFileSystem, theFilePath);
        }
        else if (aFileNameLower.EndsWith(".stl") || IsDataStartsWithHeader(aBytes, "STL Exported by") || IsDataStartsWithHeader(aBytes, "solid"))
        {
            return OpenStlShape(theShapeList, theName, theFileSystem, theFilePath);
        }
        else if (aFileNameLower.EndsWith(".step") || aFileNameLower.EndsWith(".stp") || IsDataStartsWithHeader(aBytes, "ISO-10303-21"))
        {
#ifdef OCCTV_USE_STEP
            STEPControl_Reader aReader;
            IFSelect_ReturnStatus aStatus = aReader.ReadFile(theFilePath.c_str());
            if (aStatus != IFSelect_RetDone)
            {
                Message::SendFail() << "[ERR_SHAPE_LOAD] failed to read STEP file '" << theFilePath << "'";
                return false;
            }
            aReader.TransferRoots();
            const TopoDS_Shape aShape = aReader.OneShape();
            if (aShape.IsNull())
            {
                Message::SendFail() << "[ERR_SHAPE_LOAD] STEP file '" << theFilePath << "' has no shape";
                return false;
            }

            if (myForceSinglePrec)
            {
                ShrinkTessellationPrecision(aShape);
            }

            if (myExpParams.toExplode)
            {
                Handle(RWShape::ShapeData) aRootOwner = new RWShape::ShapeData(myExpParams.toUsePrefix ? theName : "",
                                                                               theFileSystem);
                aRootOwner->SetShape(aShape);

                ExplodeContext anExpTool;
                theShapeList = anExpTool.Explode(aRootOwner, myExpParams);
            }
            else
            {
                TCollection_AsciiString aName = theName;
                if (aName.IsEmpty())
                {
                    TCollection_AsciiString aFileFolder;
                    OSD_Path::FolderAndFileFromPath(theFilePath.c_str(), aFileFolder, aName);
                }

                ExplodeContext anExpTool(theFileSystem);
                Handle(RWShape::ShapeData) aShapeIO = anExpTool.AddShape(aName, aShape);
                theShapeList.Append(aShapeIO);
            }
            if (theShapeList.IsEmpty())
            {
                return false;
            }

            Message::SendInfo() << "Loaded STEP file '" << theFilePath << "' in " << aReadTimer.ElapsedTime() << " s";
            Message::DefaultMessenger()->Send(OSD_MemInfo::PrintInfo(), Message_Trace);
            return true;
#else
            Message::SendFail() << "[ERR_STEP_DISABLED] STEP support is disabled (OCCTV_USE_STEP=OFF)";
            return false;
#endif
        }
        else if (aFileNameLower.EndsWith(".iges") || aFileNameLower.EndsWith(".igs"))
        {
#ifdef OCCTV_USE_IGES
            IGESControl_Reader aReader;
            IFSelect_ReturnStatus aStatus = aReader.ReadFile(theFilePath.c_str());
            if (aStatus != IFSelect_RetDone)
            {
                Message::SendFail() << "[ERR_SHAPE_LOAD] failed to read IGES file '" << theFilePath << "'";
                return false;
            }
            aReader.TransferRoots();
            const TopoDS_Shape aShape = aReader.OneShape();
            if (aShape.IsNull())
            {
                Message::SendFail() << "[ERR_SHAPE_LOAD] IGES file '" << theFilePath << "' has no shape";
                return false;
            }

            if (myForceSinglePrec)
            {
                ShrinkTessellationPrecision(aShape);
            }

            if (myExpParams.toExplode)
            {
                Handle(RWShape::ShapeData) aRootOwner = new RWShape::ShapeData(myExpParams.toUsePrefix ? theName : "",
                                                                               theFileSystem);
                aRootOwner->SetShape(aShape);

                ExplodeContext anExpTool;
                theShapeList = anExpTool.Explode(aRootOwner, myExpParams);
            }
            else
            {
                TCollection_AsciiString aName = theName;
                if (aName.IsEmpty())
                {
                    TCollection_AsciiString aFileFolder;
                    OSD_Path::FolderAndFileFromPath(theFilePath.c_str(), aFileFolder, aName);
                }

                ExplodeContext anExpTool(theFileSystem);
                Handle(RWShape::ShapeData) aShapeIO = anExpTool.AddShape(aName, aShape);
                theShapeList.Append(aShapeIO);
            }
            if (theShapeList.IsEmpty())
            {
                return false;
            }

            Message::SendInfo() << "Loaded IGES file '" << theFilePath << "' in " << aReadTimer.ElapsedTime() << " s";
            Message::DefaultMessenger()->Send(OSD_MemInfo::PrintInfo(), Message_Trace);
            return true;
#else
            Message::SendFail() << "[ERR_IGES_DISABLED] IGES support is disabled (OCCTV_USE_IGES=OFF)";
            return false;
#endif
        }

        Message::SendFail() << "[ERR_SHAPE_FORMAT] file '" << theFilePath << "' has unsupported format '" << aBytes << "'";
        return false;
    }

    // ================================================================
    // Function : ExplodeShape
    // ================================================================
    bool ShapeLoader::ExplodeShape(NCollection_List<Handle(RWShape::ShapeData)> &theShapeList,
                                   const Handle(RWShape::ShapeData) & theShapeOwner,
                                   const ExplodeParams &theExpParams)
    {
        if (theShapeOwner.IsNull())
        {
            return false;
        }

        OSD_Timer aReadTimer;
        aReadTimer.Start();

        ExplodeContext anExpTool;
        theShapeList = anExpTool.Explode(theShapeOwner, theExpParams, false);
        if (theShapeList.IsEmpty())
        {
            return false;
        }

        Message::SendInfo() << "Shape '" << theShapeOwner->Id() << "' was exploded"
                            << "(depth: "
                            << (theExpParams.explodeDepth > -1 ? std::to_string(theExpParams.explodeDepth) : "max")
                            << "; parts: " << std::to_string(theShapeList.Size()) << ")"
                            << " in " << aReadTimer.ElapsedTime() << " s";
        Message::DefaultMessenger()->Send(OSD_MemInfo::PrintInfo(), Message_Trace);
        return true;
    }

    struct ShapeLoader::LoadContext
    {
        TopTools_IndexedMapOfShape map;
        Handle(RWShape::ShapeData) mesh;
        OSD_Timer timer;
        std::vector<Handle(OSD_FileSystem)> fsVec;
        Handle(RWBase::FileSystemMap) fileSystem = new RWBase::FileSystemMap();
        RWBase::IPromise *callback;
        int nbDeferred = 0;

        explicit LoadContext(RWBase::IPromise *theCallback) : callback(theCallback) {}

        void Resolve()
        {
            TopLoc_Location aLoc;

            int aNbFails = 0;
            int aNbTris = 0;
            for (TopTools_IndexedMapOfShape::Iterator aFaceIter(map); aFaceIter.More(); aFaceIter.Next())
            {
                const TopoDS_Face &aFace = TopoDS::Face(aFaceIter.Value());
                const Handle(Poly_Triangulation) &aTris = BRep_Tool::Triangulation(aFace, aLoc);
                if (!aTris.IsNull() && !aTris->HasGeometry())
                {
                    ++aNbFails;
                }
                else if (!aTris.IsNull())
                {
                    aNbTris += aTris->NbTriangles();
                }
            }
            if (aNbFails > 0 && aNbFails == map.Size())
            {
                Reject(std::string("Deferred loading of '") + mesh->Id().ToCString() + "' failed");
                return;
            }

            Message::SendInfo() << map.Size() << " deferred triangulations (" << aNbTris << " tris) of '" << mesh->Id()
                                << "' loaded in " << timer.ElapsedTime() << " s";
            if (aNbFails > 0)
            {
                Message::SendWarning() << "Warning: deferred loading of '" << mesh->Id() << "' failed for " << aNbFails
                                       << "/" << map.Size();
            }
            if (callback != nullptr)
            {
                callback->resolve();
            }
        }

        void Reject(const std::string &theErr)
        {
            if (callback != nullptr)
            {
                callback->reject(theErr);
            }
        }
    };

    // ================================================================
    // Function : LoadShape
    // ================================================================
    bool ShapeLoader::LoadShape(const Handle(RWShape::ShapeData) & theMesh, RWBase::IPromise *theCallback)
    {
        std::shared_ptr<LoadContext> aLoadCtx = std::make_shared<LoadContext>(theCallback);
        if (theMesh.IsNull())
        {
            aLoadCtx->Reject("Deferred loading of NULL object");
            return false;
        }

        aLoadCtx->mesh = theMesh;
        if (theMesh->State() != RWShape::DataLoadState_Initial)
        {
            aLoadCtx->Resolve();
            return true;
        }

        aLoadCtx->timer.Start();
        aLoadCtx->nbDeferred = theMesh->GetFaces(aLoadCtx->map);
        if (aLoadCtx->nbDeferred == 0)
        {
            theMesh->SetState(RWShape::DataLoadState_Loaded);
            aLoadCtx->Resolve();
            return true;
        }

        // for async mode
        aLoadCtx->fileSystem->RegisterProtocol(theMesh->FileSystem(), theCallback != nullptr);
        theMesh->SetState(RWShape::DataLoadState_InProcess);
        if (theCallback == nullptr)
        {
            // for sync mode
            // TODO: check if the map is resizable
            for (TopTools_IndexedMapOfShape::Iterator aFaceIter(aLoadCtx->map); aFaceIter.More(); aFaceIter.Next())
            {
                BRepTools::LoadTriangulation(aFaceIter.Value(), -1, false, aLoadCtx->fileSystem);
            }
            theMesh->SetState(RWShape::DataLoadState_Loaded);
            aLoadCtx->Resolve();
            return true;
        }

        loadShapeAsync(aLoadCtx);
        return true;
    }

    void ShapeLoader::loadShapeAsync(const std::shared_ptr<LoadContext> &theLoadCtx)
    {
        using RWBase::TaskManager;
        TaskManager::Instance()->RunAsync([theLoadCtx]
                                          {
        const int aNbThreads = Min(theLoadCtx->map.Size(), TaskManager::Instance()->NbDefaultThreadsToLaunch());
        theLoadCtx->fsVec.resize(aNbThreads);
        for (Handle(OSD_FileSystem)& aFileSysIter : theLoadCtx->fsVec) {
            aFileSysIter = new RWBase::FileSystemMap(theLoadCtx->fileSystem);
        }
        TaskManager::Instance()
            ->RunInParallel(1, theLoadCtx->map.Size() + 1, [theLoadCtx](int theThreadIndex, int theObjIndex) {
                TopoDS_Shape aShape = theLoadCtx->map.FindKey(theObjIndex);

                TopLoc_Location                   aDummyLoc;
                const Handle(Poly_Triangulation)& aTris = BRep_Tool::Triangulation(TopoDS::Face(aShape), aDummyLoc);
                if (aTris.IsNull() || !aTris->HasGeometry()) {
                    // load deferred data only once as different objects might share common faces
                    BRepTools::LoadTriangulation(aShape, -1, false, theLoadCtx->fsVec[theThreadIndex]);
                }
            });

        theLoadCtx->mesh->SetState(RWShape::DataLoadState_Loaded);
        TaskManager::Instance()->RunMain([theLoadCtx]() {
            theLoadCtx->Resolve();
        }); });
    }
}
