
#ifndef DrawBase_DRAWCOMMAND_HEADERFILE
#define DrawBase_DRAWCOMMAND_HEADERFILE

#include <Draw.hxx>
#include <Draw_Interpretor.hxx>
#include <gp_Pnt.hxx>
#include <Graphic3d_Vec.hxx>
#include <Standard_Transient.hxx>

#include <memory>
#include <functional>

namespace DrawBase {

//! Draw command interface.
class DrawCommand : public Standard_Transient {
    DEFINE_STANDARD_RTTIEXT(DrawCommand, Standard_Transient)
public:
    //! Register command in Tcl interpretor.
    template<class T>
    static void Register(Draw_Interpretor& theDI, const char* theGroup)
    {
        theDI.Add(T::COMMAND_NAME, T::COMMAND_HELP, T::COMMAND_FILE, &DrawCommand::Invoke<T>, theGroup);
    }

    //! Execute command.
    template<class T>
    static inline int Invoke(Draw_Interpretor& theDI, int theNbArgs, const char** theArgVec);

public:
    //! Return TRUE if argument matches one of specified names.
    Standard_EXPORT static bool CheckArgName(const TCollection_AsciiString& theArg,
                                             const char*                    theArg1,
                                             const char*                    theArg2 = nullptr,
                                             const char*                    theArg3 = nullptr,
                                             const char*                    theArg4 = nullptr);

    //! Parse XYZ triplet.
    Standard_EXPORT static bool ParseVec3(const char** theArgVec, Graphic3d_Vec3d& thePnt);

    //! Parse XYZ triplet.
    Standard_EXPORT static bool ParseXYZ(const char** theArgVec, gp_XYZ& thePnt);

    //! Parse RGB color argument (next to the iterator position).
    static bool ParseRgbColorIterator(int theNbArgs, const char** theArgVec, int& theArgIter, Graphic3d_Vec3& theColor)
    {
        Quantity_Color aColor;
        if (ParseRgbColorIterator(theNbArgs, theArgVec, theArgIter, aColor)) {
            theColor = aColor;
            return true;
        }
        return false;
    }

    //! Parse RGB color argument (next to the iterator position).
    Standard_EXPORT static bool ParseRgbColorIterator(int             theNbArgs,
                                                      const char**    theArgVec,
                                                      int&            theArgIter,
                                                      Quantity_Color& theColor);

    //! Parse RGBA color argument (next to the iterator position).
    static bool ParseRgbaColorIterator(int theNbArgs, const char** theArgVec, int& theArgIter, Graphic3d_Vec4& theColor)
    {
        Quantity_ColorRGBA aColor;
        if (ParseRgbaColorIterator(theNbArgs, theArgVec, theArgIter, aColor)) {
            theColor = aColor;
            return true;
        }
        return false;
    }

    //! Parse RGBA or RGB color argument (next to the iterator position).
    Standard_EXPORT static bool ParseRgbaColorIterator(int                 theNbArgs,
                                                       const char**        theArgVec,
                                                       int&                theArgIter,
                                                       Quantity_ColorRGBA& theColor);

public:
    //! Destructor.
    Standard_EXPORT virtual ~DrawCommand();

    //! Empty constructor.
    Standard_EXPORT DrawCommand();

    //! Return Draw Interpretor.
    Draw_Interpretor& Interpretor() { return *myDI; }

    //! Return number of arguments.
    int NbArguments() const { return myNbArgs; }

    //! Return arguments vector.
    const char** AllArguments() const { return myArgVec; }

    //! Return argument at specific position.
    const char* Argument(int theIndex) const
    {
        if (theIndex < 0 || theIndex >= myNbArgs) {
            throw Standard_OutOfRange("DrawCommand::GetArgument(), invalid index");
        }
        return myArgVec[theIndex];
    }

public:
    //! Interface for handling argument.
    class IBinding {
    public:
        //! Destructor.
        virtual ~IBinding() {}

        //! Main interface.
        virtual bool Parse(DrawCommand* theCmd, int& theArgIter) const = 0;

    public:
        //! Argument evaluator template.
        template<class ArgT>
        static inline bool ParseValue(DrawCommand* theCmd, int& theArgIter, ArgT& theVal);

        //! Argument validator template.
        template<class ArgT>
        static inline bool ValidateValue(const ArgT& theVal, const ArgT& theMin, const ArgT& theMax);
    };

    //! Template for typed argument handles.
    template<class CmdT, class ArgT>
    class TBinding;

    //! Bind callback for a key in argument list.
    //! @code
    //!   AddKey(this, int(), "-length") = [](MyCmd* theCmd, int& theArgIter, int theVal) -> bool {
    //!     if (theVal <= 0) { theCmd->Interpretor() << "Syntax error: negative length\n"; return false; }
    //!     theCmd->myLength = theVal
    //!     return true;
    //!   };
    //! @endcode
    //! Note that lambda should have no captures.
    //! @tparam CmdT subclass of DrawCommand to use C-style callback functions
    //! @tparam ArgT argument value type
    //! @param[in] theCmd  pointer to this, a dummy argument to deduce callback function prototype
    //! @param[in] theDef  argument default value, used to deduce callback function prototype
    //! @param[in] theKey  key name
    //! @param[in] theKey2 key alias
    //! @param[in] theKey3 another  key alias
    //! @return reference to a binding interface with assign operator accepting function pointer
    template<class CmdT, typename ArgT>
    inline TBinding<CmdT, ArgT>& AddKey(CmdT*       theCmd,
                                        ArgT        theDef,
                                        const char* theKey,
                                        const char* theKey2 = nullptr,
                                        const char* theKey3 = nullptr);

    //! Bind callback for a key in argument list with range validation.
    //! @code
    //!   AddRangeKey(this, Precision::Confusion(), 100.0, "-length") = &myLength;
    //! @endcode
    //! Note that lambda should have no captures.
    //! @tparam CmdT subclass of DrawCommand to use C-style callback functions
    //! @tparam ArgT argument value type
    //! @param[in] theCmd  pointer to this, a dummy argument to deduce callback function prototype
    //! @param[in] theMin  argument minimal value, used to deduce callback function prototype
    //! @param[in] theMax  argument maximal value
    //! @param[in] theKey  key name
    //! @param[in] theKey2 key alias
    //! @param[in] theKey3 another key alias
    //! @return reference to a binding interface with assign operator accepting function pointer
    template<class CmdT, typename ArgT>
    inline TBinding<CmdT, ArgT>& AddRangeKey(CmdT*       theCmd,
                                             ArgT        theMin,
                                             ArgT        theMax,
                                             const char* theKey,
                                             const char* theKey2 = nullptr,
                                             const char* theKey3 = nullptr);

    //! Bind callback for a key with no value in argument list.
    //! @code
    //!   AddVoidKey(this, "-dump") = [](MyCmd* theCmd, int& theArgIter) -> bool {
    //!     theCmd->Dump;
    //!     return true;
    //!   };
    //! @endcode
    //! @tparam CmdT subclass of DrawCommand to use C-style callback functions
    //! @param[in] theCmd pointer to this, a dummy argument to deduce callback function prototype
    //! @param[in] theKey  key name
    //! @param[in] theKey2 key alias
    //! @param[in] theKey3 another key alias
    //! @param[in] theKey4 another key alias
    //! @return reference to a binding interface with assign operator accepting function pointer
    template<class CmdT>
    inline TBinding<CmdT, void>& AddVoidKey(CmdT*       theCmd,
                                            const char* theKey,
                                            const char* theKey2 = nullptr,
                                            const char* theKey3 = nullptr,
                                            const char* theKey4 = nullptr);

    enum DummyOnOffNo {};

    //! Bind callback for a boolean key in argument list.
    template<class CmdT>
    inline TBinding<CmdT, DummyOnOffNo>& AddOnOffNoKey(CmdT*       theCmd,
                                                       const char* theKey,
                                                       const char* theKey2 = nullptr,
                                                       const char* theKey3 = nullptr);

protected:
    //! Process arguments.
    //! Default implementation iterates over all arguments and calls ParseArgument().
    Standard_EXPORT virtual bool ParseAllArguments();

    //! Process argument at specified position.
    //! Default implementation looks for a callback bound by AddKey().
    Standard_EXPORT virtual bool ParseArgument(int& theArgIter);

    //! Execute command after (successful) processing of all arguments.
    virtual bool Perform() = 0;

protected:
    //! Command name - should be overridden in subclass.
    // @c static constexpr const char* COMMAND_NAME = "UNNAMED";
    //! Command help - should be overridden in subclass.
    // @c static constexpr const char* COMMAND_HELP = "NO HELP";
    //! Command file - should be overridden in subclass.
    // @c static constexpr const char* COMMAND_FILE = __FILE__;
protected:
    Draw_Interpretor* myDI     = nullptr;
    int               myNbArgs = 0;
    const char**      myArgVec = nullptr;

    NCollection_DataMap<TCollection_AsciiString, std::shared_ptr<IBinding>, TCollection_AsciiString> myKeyMap;
};

// =======================================================================
// function : Invoke
// =======================================================================
template<class T>
int DrawCommand::Invoke(Draw_Interpretor& theDI, int theNbArgs, const char** theArgVec)
{
    T aCmd;
    aCmd.myDI     = &theDI;
    aCmd.myNbArgs = theNbArgs;
    aCmd.myArgVec = theArgVec;
    if (!aCmd.ParseAllArguments()) {
        return 1;
    }
    if (!aCmd.Perform()) {
        return 1;
    }
    return 0;
}

//! Template for typed argument handles.
template<class CmdT, class ArgT>
class DrawCommand::TBinding : public DrawCommand::IBinding {
    //! Callback function prototypes.
    typedef bool (*FuncType)(CmdT*, int&, ArgT);
    typedef bool (*FuncType2)(CmdT*, ArgT);
    typedef bool (CmdT::*MethType)(int&, ArgT);
    typedef bool (CmdT::*MethType2)(ArgT);

public:
    //! Main constructor.
    explicit TBinding(ArgT theDef) : myMinValue(theDef), myMaxValue(theDef), myHasMinMax(false) {}

    //! Another constructor.
    TBinding(ArgT theMin, ArgT theMax) : myMinValue(theMin), myMaxValue(theMax), myHasMinMax(true) {}

    //! Assign callback function.
    TBinding& operator=(const FuncType& theFunc)
    {
        myCallback = theFunc;
        return *this;
    }

    //! Assign callback function.
    TBinding& operator=(const FuncType2& theFunc)
    {
        myCallback2 = theFunc;
        return *this;
    }

    //! Assign callback function.
    TBinding& operator=(const MethType& theFunc)
    {
        myCallback3 = theFunc;
        return *this;
    }

    //! Assign callback function.
    TBinding& operator=(const MethType2& theFunc)
    {
        myCallback4 = theFunc;
        return *this;
    }

    //! Assign callback property.
    TBinding& operator=(ArgT* theProp)
    {
        myCallback5 = theProp;
        return *this;
    }

    //! Interface implementation.
    virtual bool Parse(DrawCommand* theCmd, int& theArgIter) const override
    {
        const int aPosBack = theArgIter;

        ArgT aVal(myMinValue);
        if (!ParseValue<ArgT>(theCmd, theArgIter, aVal)) {
            return false;
        } else if (myHasMinMax && !ValidateValue<ArgT>(aVal, myMinValue, myMaxValue)) {
            theArgIter = aPosBack;
            theCmd->Interpretor() << "Syntax error: invalid range for '" << theCmd->Argument(aPosBack) << "'\n";
            return false;
        }

        CmdT* aCmd   = static_cast<CmdT*>(theCmd);
        bool  isDone = false;
        if (myCallback != nullptr) {
            isDone = myCallback(aCmd, theArgIter, aVal);
        } else if (myCallback2 != nullptr) {
            isDone = myCallback2(aCmd, aVal);
        } else if (myCallback3 != nullptr) {
            isDone = (*aCmd.*myCallback3)(theArgIter, aVal);
        } else if (myCallback4 != nullptr) {
            isDone = (*aCmd.*myCallback4)(aVal);
        } else if (myCallback5 != nullptr) {
            *myCallback5 = aVal;
            isDone       = true;
        }
        if (!isDone) {
            theArgIter = aPosBack;
            return false;
        }
        return true;
    }

private:
    FuncType  myCallback  = nullptr;
    FuncType2 myCallback2 = nullptr;
    MethType  myCallback3 = nullptr;
    MethType2 myCallback4 = nullptr;
    ArgT*     myCallback5 = nullptr;
    ArgT      myMinValue;
    ArgT      myMaxValue;
    bool      myHasMinMax = false;
};

//! Binding for string argument.
template<>
inline bool DrawCommand::IBinding::ParseValue<const char*>(DrawCommand*      theCmd,
                                                           int&              theArgIter,
                                                           Standard_CString& theVal)
{
    if (theArgIter + 1 >= theCmd->NbArguments()) {
        return false;
    }

    theVal = theCmd->Argument(++theArgIter);
    return true;
}

//! Validate string argument.
template<>
inline bool DrawCommand::IBinding::ValidateValue<const char*>(const Standard_CString&,
                                                              const Standard_CString&,
                                                              const Standard_CString&)
{
    return true;
}

//! Binding for boolean argument.
template<>
inline bool DrawCommand::IBinding::ParseValue<bool>(DrawCommand* theCmd, int& theArgIter, bool& theVal)
{
    theVal = Draw::ParseOnOffIterator(theCmd->NbArguments(), theCmd->AllArguments(), theArgIter);
    return true;
}

//! Validate bool argument.
template<>
inline bool DrawCommand::IBinding::ValidateValue<bool>(const bool&, const bool&, const bool&)
{
    return true;
}

//! Binding for integer argument.
template<>
inline bool DrawCommand::IBinding::ParseValue<int>(DrawCommand* theCmd, int& theArgIter, int& theVal)
{
    if (theArgIter + 1 >= theCmd->NbArguments() || !Draw::ParseInteger(theCmd->Argument(theArgIter + 1), theVal)) {
        return false;
    }

    ++theArgIter;
    return true;
}

//! Validate integer argument.
template<>
inline bool DrawCommand::IBinding::ValidateValue<int>(const int& theVal, const int& theMin, const int& theMax)
{
    return theVal >= theMin && theVal <= theMax;
}

//! Binding for real argument.
template<>
inline bool DrawCommand::IBinding::ParseValue<double>(DrawCommand* theCmd, int& theArgIter, double& theVal)
{
    if (theArgIter + 1 >= theCmd->NbArguments() || !Draw::ParseReal(theCmd->Argument(theArgIter + 1), theVal)) {
        return false;
    }

    ++theArgIter;
    return true;
}

//! Validate real argument.
template<>
inline bool DrawCommand::IBinding::ValidateValue<double>(const double& theVal,
                                                         const double& theMin,
                                                         const double& theMax)
{
    return theVal >= theMin && theVal <= theMax;
}

//! Binding for XY argument.
template<>
inline bool DrawCommand::IBinding::ParseValue<gp_XY>(DrawCommand* theCmd, int& theArgIter, gp_XY& theVal)
{
    static constexpr int iY2 = 2;
    if (theArgIter + iY2 >= theCmd->NbArguments()) {
        return false;
    } else if (Draw::ParseReal(theCmd->Argument(theArgIter + 1), theVal.ChangeCoord(1))
               && Draw::ParseReal(theCmd->Argument(theArgIter + iY2), theVal.ChangeCoord(iY2))) {
        theArgIter += iY2;
        return true;
    }
    return false;
}

//! Validate XY argument.
template<>
inline bool DrawCommand::IBinding::ValidateValue<gp_XY>(const gp_XY& theVal, const gp_XY& theMin, const gp_XY& theMax)
{
    return theVal.X() >= theMin.X() && theVal.X() <= theMax.X() && theVal.Y() >= theMin.Y() && theVal.Y() <= theMax.Y();
}

//! Binding for XYZ argument.
template<>
inline bool DrawCommand::IBinding::ParseValue<gp_XYZ>(DrawCommand* theCmd, int& theArgIter, gp_XYZ& theVal)
{
    static constexpr int iY2 = 2;
    static constexpr int iZ3 = 3;
    if (theArgIter + iZ3 < theCmd->NbArguments()
        && Draw::ParseReal(theCmd->Argument(theArgIter + 1), theVal.ChangeCoord(1))
        && Draw::ParseReal(theCmd->Argument(theArgIter + iY2), theVal.ChangeCoord(iY2))
        && Draw::ParseReal(theCmd->Argument(theArgIter + iZ3), theVal.ChangeCoord(iZ3))) {
        theArgIter += iZ3;
        return true;
    }
    return false;
}

//! Validate XYZ argument.
template<>
inline bool DrawCommand::IBinding::ValidateValue<gp_XYZ>(const gp_XYZ& theVal,
                                                         const gp_XYZ& theMin,
                                                         const gp_XYZ& theMax)
{
    return theVal.X() >= theMin.X() && theVal.X() <= theMax.X() && theVal.Y() >= theMin.Y() && theVal.Y() <= theMax.Y()
        && theVal.Z() >= theMin.Z() && theVal.Z() <= theMax.Z();
}

//! Binding for Vec2 argument.
template<>
inline bool DrawCommand::IBinding::ParseValue<Graphic3d_Vec2d>(DrawCommand*     theCmd,
                                                               int&             theArgIter,
                                                               Graphic3d_Vec2d& theVal)
{
    static constexpr int iY2 = 2;
    if (theArgIter + Graphic3d_Vec2d::Length() < theCmd->NbArguments()
        && Draw::ParseReal(theCmd->Argument(theArgIter + 1), theVal.x())
        && Draw::ParseReal(theCmd->Argument(theArgIter + iY2), theVal.y())) {
        theArgIter += Graphic3d_Vec2d::Length();
        return true;
    }
    return false;
}

//! Validate Vec2 argument.
template<>
inline bool DrawCommand::IBinding::ValidateValue<Graphic3d_Vec2d>(const Graphic3d_Vec2d& theVal,
                                                                  const Graphic3d_Vec2d& theMin,
                                                                  const Graphic3d_Vec2d& theMax)
{
    return theVal.x() >= theMin.x() && theVal.x() <= theMax.x() && theVal.y() >= theMin.y() && theVal.y() <= theMax.y();
}

//! Binding for Vec2 argument.
template<>
inline bool DrawCommand::IBinding::ParseValue<Graphic3d_Vec2i>(DrawCommand*     theCmd,
                                                               int&             theArgIter,
                                                               Graphic3d_Vec2i& theVal)
{
    static constexpr int iY2 = 2;
    if (theArgIter + Graphic3d_Vec2i::Length() < theCmd->NbArguments()
        && Draw::ParseInteger(theCmd->Argument(theArgIter + 1), theVal.x())
        && Draw::ParseInteger(theCmd->Argument(theArgIter + iY2), theVal.y())) {
        theArgIter += Graphic3d_Vec2i::Length();
        return true;
    }
    return false;
}

//! Validate Vec2 argument.
template<>
inline bool DrawCommand::IBinding::ValidateValue<Graphic3d_Vec2i>(const Graphic3d_Vec2i& theVal,
                                                                  const Graphic3d_Vec2i& theMin,
                                                                  const Graphic3d_Vec2i& theMax)
{
    return theVal.x() >= theMin.x() && theVal.x() <= theMax.x() && theVal.y() >= theMin.y() && theVal.y() <= theMax.y();
}

//! Binding for Vec3 argument.
template<>
inline bool DrawCommand::IBinding::ParseValue<Graphic3d_Vec3d>(DrawCommand*     theCmd,
                                                               int&             theArgIter,
                                                               Graphic3d_Vec3d& theVal)
{
    static constexpr int iY2 = 2;
    static constexpr int iZ3 = 3;
    if (theArgIter + Graphic3d_Vec3d::Length() < theCmd->NbArguments()
        && Draw::ParseReal(theCmd->Argument(theArgIter + 1), theVal.x())
        && Draw::ParseReal(theCmd->Argument(theArgIter + iY2), theVal.y())
        && Draw::ParseReal(theCmd->Argument(theArgIter + iZ3), theVal.z())) {
        theArgIter += Graphic3d_Vec3d::Length();
        return true;
    }
    return false;
}

//! Validate Vec3 argument.
template<>
inline bool DrawCommand::IBinding::ValidateValue<Graphic3d_Vec3d>(const Graphic3d_Vec3d& theVal,
                                                                  const Graphic3d_Vec3d& theMin,
                                                                  const Graphic3d_Vec3d& theMax)
{
    return theVal.x() >= theMin.x() && theVal.x() <= theMax.x() && theVal.y() >= theMin.y() && theVal.y() <= theMax.y()
        && theVal.z() >= theMin.z() && theVal.z() <= theMax.z();
}

//! Binding for Vec3 argument.
template<>
inline bool DrawCommand::IBinding::ParseValue<Graphic3d_Vec3i>(DrawCommand*     theCmd,
                                                               int&             theArgIter,
                                                               Graphic3d_Vec3i& theVal)
{
    static constexpr int iY2 = 2;
    static constexpr int iZ3 = 3;
    if (theArgIter + Graphic3d_Vec3i::Length() < theCmd->NbArguments()
        && Draw::ParseInteger(theCmd->Argument(theArgIter + 1), theVal.x())
        && Draw::ParseInteger(theCmd->Argument(theArgIter + iY2), theVal.y())
        && Draw::ParseInteger(theCmd->Argument(theArgIter + iZ3), theVal.z())) {
        theArgIter += Graphic3d_Vec3i::Length();
        return true;
    }
    return false;
}

//! Validate Vec3 argument.
template<>
inline bool DrawCommand::IBinding::ValidateValue<Graphic3d_Vec3i>(const Graphic3d_Vec3i& theVal,
                                                                  const Graphic3d_Vec3i& theMin,
                                                                  const Graphic3d_Vec3i& theMax)
{
    return theVal.x() >= theMin.x() && theVal.x() <= theMax.x() && theVal.y() >= theMin.y() && theVal.y() <= theMax.y()
        && theVal.z() >= theMin.z() && theVal.z() <= theMax.z();
}

//! Binding for Vec3 argument.
template<>
inline bool DrawCommand::IBinding::ParseValue<Graphic3d_Vec4d>(DrawCommand*     theCmd,
                                                               int&             theArgIter,
                                                               Graphic3d_Vec4d& theVal)
{
    if (theArgIter + Graphic3d_Vec4d::Length() >= theCmd->NbArguments()) {
        return false;
    }
    for (int anIter = 0; anIter < Graphic3d_Vec4d::Length(); ++anIter) {
        if (!Draw::ParseReal(theCmd->Argument(theArgIter + anIter + 1), theVal[anIter])) {
            return false;
        }
    }
    theArgIter += Graphic3d_Vec4d::Length();
    return true;
}

//! Validate Vec4 argument.
template<>
inline bool DrawCommand::IBinding::ValidateValue<Graphic3d_Vec4d>(const Graphic3d_Vec4d& theVal,
                                                                  const Graphic3d_Vec4d& theMin,
                                                                  const Graphic3d_Vec4d& theMax)
{
    return theVal.x() >= theMin.x() && theVal.x() <= theMax.x() && theVal.y() >= theMin.y() && theVal.y() <= theMax.y()
        && theVal.z() >= theMin.z() && theVal.z() <= theMax.z() && theVal.w() >= theMin.w() && theVal.w() <= theMax.w();
}

//! Binding for color argument.
template<>
inline bool DrawCommand::IBinding::ParseValue<Quantity_Color>(DrawCommand*    theCmd,
                                                              int&            theArgIter,
                                                              Quantity_Color& theVal)
{
    return DrawCommand::ParseRgbColorIterator(theCmd->NbArguments(), theCmd->AllArguments(), theArgIter, theVal);
}

//! Validate color argument.
template<>
inline bool DrawCommand::IBinding::ValidateValue<Quantity_Color>(const Quantity_Color& theVal,
                                                                 const Quantity_Color& theMin,
                                                                 const Quantity_Color& theMax)
{
    return theVal.Red() >= theMin.Red() && theVal.Red() <= theMax.Red() && theVal.Green() >= theMin.Green()
        && theVal.Green() <= theMax.Green() && theVal.Blue() >= theMin.Blue() && theVal.Blue() <= theMax.Blue();
}

//! Binding for color argument.
template<>
inline bool DrawCommand::IBinding::ParseValue<Quantity_ColorRGBA>(DrawCommand*        theCmd,
                                                                  int&                theArgIter,
                                                                  Quantity_ColorRGBA& theVal)
{
    return DrawCommand::ParseRgbaColorIterator(theCmd->NbArguments(), theCmd->AllArguments(), theArgIter, theVal);
}

//! Validate color argument.
template<>
inline bool DrawCommand::IBinding::ValidateValue<Quantity_ColorRGBA>(const Quantity_ColorRGBA& theVal,
                                                                     const Quantity_ColorRGBA& theMin,
                                                                     const Quantity_ColorRGBA& theMax)
{
    return ValidateValue<Quantity_Color>(theVal.GetRGB(), theMin.GetRGB(), theMax.GetRGB())
        && theVal.Alpha() >= theMin.Alpha() && theVal.Alpha() <= theMax.Alpha();
}

// =======================================================================
// function : AddKey
// =======================================================================
template<class CmdT, typename ArgT>
inline DrawCommand::TBinding<CmdT, ArgT>& DrawCommand::AddKey(CmdT*       theCmd,
                                                              ArgT        theDef,
                                                              const char* theKey,
                                                              const char* theKey2,
                                                              const char* theKey3)
{
    (void)theCmd;
    std::shared_ptr<TBinding<CmdT, ArgT>> aBind = std::make_shared<TBinding<CmdT, ArgT>>(theDef);
    myKeyMap.Bind(theKey, aBind);
    if (theKey2 != nullptr) {
        myKeyMap.Bind(theKey2, aBind);
    }
    if (theKey3 != nullptr) {
        myKeyMap.Bind(theKey3, aBind);
    }
    return *aBind;
}

// =======================================================================
// function : AddRangeKey
// =======================================================================
template<class CmdT, typename ArgT>
inline DrawCommand::TBinding<CmdT, ArgT>& DrawCommand::AddRangeKey(CmdT*       theCmd,
                                                                   ArgT        theMin,
                                                                   ArgT        theMax,
                                                                   const char* theKey,
                                                                   const char* theKey2,
                                                                   const char* theKey3)
{
    (void)theCmd;
    std::shared_ptr<TBinding<CmdT, ArgT>> aBind = std::make_shared<TBinding<CmdT, ArgT>>(theMin, theMax);
    myKeyMap.Bind(theKey, aBind);
    if (theKey2 != nullptr) {
        myKeyMap.Bind(theKey2, aBind);
    }
    if (theKey3 != nullptr) {
        myKeyMap.Bind(theKey3, aBind);
    }
    return *aBind;
}

//! Void argument template specialization.
template<class CmdT>
class DrawCommand::TBinding<CmdT, void> : public IBinding {
    //! Callback function prototypes.
    typedef bool (*FuncType)(CmdT*, int&);
    typedef bool (*FuncType2)(CmdT*);
    typedef bool (CmdT::*MethType)(int&);
    typedef bool (CmdT::*MethType2)(void);

public:
    //! Main constructor.
    explicit TBinding() {}

    //! Assign callback function.
    TBinding& operator=(const FuncType& theFunc)
    {
        myCallback = theFunc;
        return *this;
    }

    //! Assign callback function.
    TBinding& operator=(const FuncType2& theFunc)
    {
        myCallback2 = theFunc;
        return *this;
    }

    //! Assign callback function.
    TBinding& operator=(const MethType& theFunc)
    {
        myCallback3 = theFunc;
        return *this;
    }

    //! Assign callback function.
    TBinding& operator=(const MethType2& theFunc)
    {
        myCallback4 = theFunc;
        return *this;
    }

    //! Interface implementation.
    virtual bool Parse(DrawCommand* theCmd, int& theArgIter) const override
    {
        CmdT* aCmd   = static_cast<CmdT*>(theCmd);
        bool  isDone = false;
        if (myCallback != nullptr) {
            isDone = myCallback(aCmd, theArgIter);
        } else if (myCallback2 != nullptr) {
            isDone = myCallback2(aCmd);
        } else if (myCallback3 != nullptr) {
            isDone = (*aCmd.*myCallback3)(theArgIter);
        } else if (myCallback4 != nullptr) {
            isDone = (*aCmd.*myCallback4)();
        }
        return isDone;
    }

private:
    FuncType  myCallback  = nullptr;
    FuncType2 myCallback2 = nullptr;
    MethType  myCallback3 = nullptr;
    MethType2 myCallback4 = nullptr;
};

// =======================================================================
// function : AddVoidKey
// =======================================================================
template<class CmdT>
DrawCommand::TBinding<CmdT, void>& DrawCommand::AddVoidKey(CmdT*       theCmd,
                                                           const char* theKey,
                                                           const char* theKey2,
                                                           const char* theKey3,
                                                           const char* theKey4)
{
    (void)theCmd;
    std::shared_ptr<TBinding<CmdT, void>> aBind = std::make_shared<TBinding<CmdT, void>>();
    myKeyMap.Bind(theKey, aBind);
    if (theKey2 != nullptr) {
        myKeyMap.Bind(theKey2, aBind);
    }
    if (theKey3 != nullptr) {
        myKeyMap.Bind(theKey3, aBind);
    }
    if (theKey4 != nullptr) {
        myKeyMap.Bind(theKey4, aBind);
    }
    return *aBind;
}

//! Template for typed argument handles.
template<class CmdT>
class DrawCommand::TBinding<CmdT, DrawCommand::DummyOnOffNo> : public DrawCommand::IBinding {
    //! Callback function prototypes.
    typedef bool (*FuncType)(CmdT*, int&, bool);
    typedef bool (*FuncType2)(CmdT*, bool);
    typedef bool (CmdT::*MethType)(int&, bool);
    typedef bool (CmdT::*MethType2)(bool);

public:
    //! Main constructor.
    explicit TBinding() {}

    //! Assign callback function.
    TBinding& operator=(const FuncType& theFunc)
    {
        myCallback = theFunc;
        return *this;
    }

    //! Assign callback function.
    TBinding& operator=(const FuncType2& theFunc)
    {
        myCallback2 = theFunc;
        return *this;
    }

    //! Assign callback function.
    TBinding& operator=(const MethType& theFunc)
    {
        myCallback3 = theFunc;
        return *this;
    }

    //! Assign callback function.
    TBinding& operator=(const MethType2& theFunc)
    {
        myCallback4 = theFunc;
        return *this;
    }

    //! Assign callback property.
    TBinding& operator=(bool* theProp)
    {
        myCallback5 = theProp;
        return *this;
    }

    //! Assign callback property.
    TBinding& operator=(int* theProp)
    {
        myCallback6 = theProp;
        return *this;
    }

    //! Interface implementation.
    virtual bool Parse(DrawCommand* theCmd, int& theArgIter) const override
    {
        const int aPosBack = theArgIter;

        bool  aVal = Draw::ParseOnOffNoIterator(theCmd->NbArguments(), theCmd->AllArguments(), theArgIter);
        CmdT* aCmd = static_cast<CmdT*>(theCmd);

        bool isDone = false;
        if (myCallback != nullptr) {
            isDone = myCallback(aCmd, theArgIter, aVal);
        } else if (myCallback2 != nullptr) {
            isDone = myCallback2(aCmd, aVal);
        } else if (myCallback3 != nullptr) {
            isDone = (*aCmd.*myCallback3)(theArgIter, aVal);
        } else if (myCallback4 != nullptr) {
            isDone = (*aCmd.*myCallback4)(aVal);
        } else if (myCallback5 != nullptr) {
            *myCallback5 = aVal;
            isDone       = true;
        } else if (myCallback6 != nullptr) {
            *myCallback6 = aVal ? 1 : 0;
            isDone       = true;
        }
        if (!isDone) {
            theArgIter = aPosBack;
            return false;
        }
        return true;
    }

private:
    FuncType  myCallback  = nullptr;
    FuncType2 myCallback2 = nullptr;
    MethType  myCallback3 = nullptr;
    MethType2 myCallback4 = nullptr;
    bool*     myCallback5 = nullptr;
    int*      myCallback6 = nullptr;
};

// =======================================================================
// function : AddOnOffNoKey
// =======================================================================
template<class CmdT>
DrawCommand::TBinding<CmdT, DrawCommand::DummyOnOffNo>& DrawCommand::AddOnOffNoKey(CmdT*       theCmd,
                                                                                   const char* theKey,
                                                                                   const char* theKey2,
                                                                                   const char* theKey3)
{
    (void)theCmd;
    static constexpr int insPos = 2;

    std::shared_ptr<TBinding<CmdT, DummyOnOffNo>> aBind = std::make_shared<TBinding<CmdT, DummyOnOffNo>>();

    TCollection_AsciiString aKey(theKey);
    myKeyMap.Bind(aKey, aBind);
    aKey.Insert(insPos, "no");
    myKeyMap.Bind(aKey, aBind);
    if (theKey2 != nullptr) {
        TCollection_AsciiString aKey2(theKey2);
        myKeyMap.Bind(aKey2, aBind);
        aKey2.Insert(insPos, "no");
        myKeyMap.Bind(aKey2, aBind);
    }
    if (theKey3 != nullptr) {
        TCollection_AsciiString aKey3(theKey3);
        myKeyMap.Bind(aKey3, aBind);
        aKey3.Insert(insPos, "no");
        myKeyMap.Bind(aKey3, aBind);
    }
    return *aBind;
}

}

#endif // DrawBase_DRAWCOMMAND_HEADERFILE
