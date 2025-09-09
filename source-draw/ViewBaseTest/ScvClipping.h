#ifndef ViewBASETEST_SCVCLIPPING_HEADERFILE
#define ViewBASETEST_SCVCLIPPING_HEADERFILE

#include <ViewBaseTest/ViewerCommand.h>

#include <ViewBase/InteractiveClipping.h>

namespace ViewBaseTest {

//! Command for managing clipping box object @link ViewBase::InteractiveClipping @endlink.
class ScvClipping : public ViewerCommand {
public:
    //! Command name.
    static constexpr const char* COMMAND_NAME = "scvclipping";

    //! Command help.
    static constexpr const char* COMMAND_HELP = R"(
scvclipping Name [-config +x|+y|+z|+x-y|+x-y+z|box] [-clipping {0|1}]=1
                 [-flip {0|1}]=0 [-capping {0|1}]=0 [-objMaterial {0|1}]=1
                 [-corner X Y Z] [-x Value] [-y Value] [-z Value] [-sceneCenter]
                 [-extent DX DY DZ] [-dx Value] [-dy Value] [-dz Value]
                 [-min X Y Z] [-minX Value] [-minY Value] [-minZ Value]
                 [-max X Y Z] [-maxX Value] [-maxY Value] [-maxZ Value]
Create and display clipping box.
  -config   (or -cfg) planes configuration - signed or unsigned;
  -min/-max clipping box range,
            infinite numbers mean half-space clipping,
            invalid range means no clipping,
            add % to define value within scene AABB;
  -corner   clipping box corner
           (convenient interface to change either min or max);
  -flip     reverse clipping direction (e.g. clip
            everything outside box instead of everything inside);
  -capping  turn on/off capping;
  -objMaterial inherit object's color for capping.
)";

    //! Command source file.
    static constexpr const char* COMMAND_FILE = __FILE__;

public:
    //! Constructor.
    Standard_EXPORT ScvClipping();

    //! Process arguments.
    Standard_EXPORT virtual bool ParseAllArguments() override;

    //! Process argument.
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter) override;

    //! Execute command.
    Standard_EXPORT virtual bool Perform() override;

private:
    static constexpr int iX0 = 0;
    static constexpr int iY1 = 1;
    static constexpr int iZ2 = 2;

    static constexpr int    nbClipPlanes2 = 2;
    static constexpr double percents100   = 100.0;
    static constexpr double percents001   = 0.01;

private:
    //! Process configuration argument.
    bool ParseConfig(const char* theCfg);

    //! Parse position - absolute or relative to View.
    bool ParsePosComp(const char* theStr, const int theComp, double& theReal)
    {
        return ParseComp(theStr, theComp, false, theReal);
    }

    //! Parse vector - absolute or relative to View.
    bool ParseVecComp(const char* theStr, const int theComp, double& theReal)
    {
        return ParseComp(theStr, theComp, true, theReal);
    }

    //! Parse position - absolute or relative to View.
    bool ParseComp(const char* theStr, const int theComp, const bool theIsRange, double& theReal);

    //! Parse position - absolute or relative to View.
    bool ParsePosXYZ(const char** theArgVec, Graphic3d_Vec3d& theVec);

    //! Parse position - absolute or relative to View.
    bool ParsePosX(const char* theStr, Graphic3d_Vec3d& theVec) { return ParsePosComp(theStr, iX0, theVec.x()); }

    //! Parse position - absolute or relative to View.
    bool ParsePosY(const char* theStr, Graphic3d_Vec3d& theVec) { return ParsePosComp(theStr, iY1, theVec.y()); }

    //! Parse position - absolute or relative to View.
    bool ParsePosZ(const char* theStr, Graphic3d_Vec3d& theVec) { return ParsePosComp(theStr, iZ2, theVec.z()); }

    //! Process corner arguments.
    bool ParseCornerArguments(const TCollection_AsciiString& theArgCase,
                              int                            theNbArgs,
                              const char**                   theArgVec,
                              int&                           theArgIter);

    //! Parse vector - absolute or relative to View.
    bool ParseVecXYZ(const char** theArgVec, Graphic3d_Vec3d& theVec);

    //! Parse vector - absolute or relative to View.
    bool ParseVecX(const char* theStr, Graphic3d_Vec3d& theVec) { return ParseVecComp(theStr, iX0, theVec.x()); }

    //! Parse vector - absolute or relative to View.
    bool ParseVecY(const char* theStr, Graphic3d_Vec3d& theVec) { return ParseVecComp(theStr, iY1, theVec.y()); }

    //! Parse vector - absolute or relative to View.
    bool ParseVecZ(const char* theStr, Graphic3d_Vec3d& theVec) { return ParseVecComp(theStr, iZ2, theVec.z()); }

    //! Process extent arguments.
    bool ParseExtentArguments(const TCollection_AsciiString& theArgCase,
                              int                            theNbArgs,
                              const char**                   theArgVec,
                              int&                           theArgIter);

    //! Process min/max arguments.
    bool ParseMinMaxArguments(const TCollection_AsciiString& theArgCase,
                              int                            theNbArgs,
                              const char**                   theArgVec,
                              int&                           theArgIter);

private:
    //! Print real value.
    TCollection_AsciiString PrintReal(double theVal) const
    {
        if (Precision::IsPositiveInfinite(theVal)) {
            return "+inf";
        } else if (Precision::IsNegativeInfinite(theVal)) {
            return "-inf";
        }
        return TCollection_AsciiString(theVal);
    }

    //! Print real triplet.
    TCollection_AsciiString PrintRealXYZ(const double* theVec, const char* theSuff = "") const
    {
        return PrintReal(theVec[iX0]) + theSuff + " " + PrintReal(theVec[iY1]) + theSuff + " " + PrintReal(theVec[iZ2])
             + theSuff;
    }

    //! Print real triplet in percents.
    TCollection_AsciiString PrintRealXYZRel(const double* theVec) const;

    //! Print clipping box information.
    void PrintInfo(Draw_Interpretor& theDI, const Handle(ViewBase::InteractiveClipping)& theClipObj);

private:
    //! Set clipping corner to object.
    void SetClippingCorner(const Handle(ViewBase::InteractiveClipping)& theClipObj);

    //! Set clipping configuration to object.
    void SetClippingConfig(const Handle(ViewBase::InteractiveClipping)& theClipObj);

    //! Set clipping extent to object.
    void SetClippingExtent(const Handle(ViewBase::InteractiveClipping)& theClipObj);

    //! Set clipping box to object.
    void SetClippingBox(const Handle(ViewBase::InteractiveClipping)& theClipObj);

    //! Set clipping object flags.
    void SetClippingFlags(const Handle(ViewBase::InteractiveClipping)& theClipObj);

private:
    TCollection_AsciiString m_name;

    Graphic3d_Vec3i        m_config = Graphic3d_Vec3i(0);
    Graphic3d_Vec3d        m_corner = Graphic3d_Vec3d(RealLast());
    Graphic3d_Vec3d        m_extent = Graphic3d_Vec3d(0.0);
    NCollection_Vec3<bool> m_hasExt = NCollection_Vec3<bool>(false);

    Graphic3d_Vec3d        m_boxMin;
    Graphic3d_Vec3d        m_boxMax;
    NCollection_Vec3<bool> m_hasBoxMin = NCollection_Vec3<bool>(false);
    NCollection_Vec3<bool> m_hasBoxMax = NCollection_Vec3<bool>(false);

    int m_toFlip     = -1;
    int m_isCapping  = -1;
    int m_isClipping = -1;
    int m_isShaded   = -1;
    int m_isObjMat   = -1;
};

}

#endif // ViewBASETEST_SCVCLIPPING_HEADERFILE