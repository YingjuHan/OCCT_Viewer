#ifndef _SHAPELOADER_HEADERFILE
#define _SHAPELOADER_HEADERFILE

#include <RWShape/ShapeData.h>

#include <RWBase/IPromise.h>

#include <OSD_FileSystem.hxx>

namespace RWShapeConv
{
    //! Class provides public interface for loading B-Rep geometry and assembly structure
    //! from file in default visualization formats.
    class ShapeLoader
    {
    public:
        DEFINE_STANDARD_ALLOC
    public:
        static constexpr char THE_ID_SEPARATOR = ':';
        static constexpr char THE_COUNTER_SEPARATOR = '_';

        //! Structure defining explode parameters.
        struct ExplodeParams
        {
            bool toExplode = false;    //!< explode model or display as a single part
            int explodeDepth = -1;     //!< depth of explode process (default value is -1 - explode to maximal possible depth)
            bool toUsePrefix = true;   //!< use name parameter as prefix of result interactive objects names or not
            bool isAutoNaming = true;  //!< generate id by shape type ("compound", "solid", etc)
                                       //!< depending on parents in case of attributes absence
            bool toGenerateId = true;  //!< generate unique ids or use initial ones in case of existing attributes
            bool toPropagateId = true; //!< generate id depending on parents in case of existing attributes
        };

        //! Probe format from file header.
        Standard_EXPORT static std::string ProbeFileFormat(const std::string &theName,
                                                           const char *theBuffer,
                                                           int theDataLen);

    public:
        //! Return explode params.
        const ExplodeParams &GetExplodeParams() const { return myExpParams; }

        //! Return explode params.
        ExplodeParams &ChangeExplodeParams() { return myExpParams; }

        //! Set explode params.
        void SetExplodeParams(const ExplodeParams &theParams) { myExpParams = theParams; }

        //! Return destination length units as scale factor to m (meters).
        double GetSystemLengthUnitScale() const { return mySysLenUnitScale; }

        //! Set destination length units as scale factor to m (meters).
        void SetSystemLengthUnitScale(double theScale) { mySysLenUnitScale = theScale; }

        //! Return if single-precision conversion should be forced after reading triangulation from BREP; FALSE by default.
        bool ToForceSinglePrecision() const { return myForceSinglePrec; }

        //! Set if single-precision conversion should be forced after reading triangulation from BREP.
        void SetForceSinglePrecision(bool theToForce) { myForceSinglePrec = theToForce; }

    public:
        //! Open object from file system.
        //! @param[out] theShapeList  list of result objects
        //! @param[in]  theName       object name
        //! @param[in]  theFileSystem file system
        //! @param[in]  theFilePath   file path
        //! @return FALSE on error
        Standard_EXPORT bool OpenShape(NCollection_List<Handle(RWShape::ShapeData)> &theShapeList,
                                       const TCollection_AsciiString &theName,
                                       const Handle(OSD_FileSystem) & theFileSystem,
                                       const std::string &theFilePath);

        //! Explode shape by its interactive owner.
        //! @param[out] theShapeList   list of result objects
        //! @param[in] theShapeOwner interactive owner, its shapes will ve exploded
        //! @param[in] theExpParams  explosion options
        //! @return FALSE on error
        Standard_EXPORT static bool ExplodeShape(NCollection_List<Handle(RWShape::ShapeData)> &theShapeList,
                                                 const Handle(RWShape::ShapeData) & theShapeOwner,
                                                 const ExplodeParams &theExpParams);

        //! Load deferred data for specified model.
        Standard_EXPORT static bool LoadShape(const Handle(RWShape::ShapeData) & theMesh, RWBase::IPromise *theCallback);

    private:
        struct LoadContext;

    private:
        //! Load shape async
        static void loadShapeAsync(const std::shared_ptr<LoadContext> &theLoadCtx);

    private:
        //! OCCT B-Rep format options.
        enum class OcctBRepFormat
        {
            Ascii,
            Binary,
            SBinary
        };

        //! Read shape from BREP file.
        Standard_EXPORT bool OpenBrepShape(NCollection_List<Handle(RWShape::ShapeData)> &theShapeList,
                                           const TCollection_AsciiString &theName,
                                           const Handle(OSD_FileSystem) & theFileSystem,
                                           const std::string &theFilePath,
                                           const OcctBRepFormat theFormat);

        //! Read shape from STL file.
        Standard_EXPORT bool OpenStlShape(NCollection_List<Handle(RWShape::ShapeData)> &theShapeList,
                                          const TCollection_AsciiString &theName,
                                          const Handle(OSD_FileSystem) & theFileSystem,
                                          const std::string &theFilePath);

        //! Read shape from glTF file.
        Standard_EXPORT bool OpenGltfShape(NCollection_List<Handle(RWShape::ShapeData)> &theShapeList,
                                           const TCollection_AsciiString &theName,
                                           const Handle(OSD_FileSystem) & theFileSystem,
                                           const std::string &theFilePath);

    private:
        ExplodeParams myExpParams;
        double mySysLenUnitScale = 1.0;
        bool myForceSinglePrec = false;
    };

}

#endif // _SHAPELOADER_HEADERFILE