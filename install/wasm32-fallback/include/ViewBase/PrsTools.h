#ifndef _PRSTOOLS_HXX_HEADERFILE
#define _PRSTOOLS_HXX_HEADERFILE

#include <Graphic3d_Vec2.hxx>

namespace ViewBase
{

    //! Common tools for interactive presentations.
    class PrsTools
    {
    public:
        //! Hasher for segment defined by two integer values.
        struct SegmentsHasher
        {
            static int HashCode(const Graphic3d_Vec2i &theVec, const int theUpper)
            {
#ifndef Standard_UNUSED // suppress dummy 'theUpper might be 0' static analyzer warning
                if (theUpper == 0)
                {
                    return 1;
                }
#endif
                return int(static_cast<unsigned int>(theVec[0] + theVec[1]) & 0x7FFFFFFF) % theUpper + 1;
            }

            static bool IsEqual(const Graphic3d_Vec2i &theVec1, const Graphic3d_Vec2i &theVec2)
            {
                return !((static_cast<unsigned int>(theVec1[0] - theVec2[0]) | static_cast<unsigned int>(theVec1[1] - theVec2[1])) && (static_cast<unsigned int>(theVec1[0] - theVec2[1]) | static_cast<unsigned int>(theVec1[1] - theVec2[0])));
            }
        };
    };

}

#endif