#include <SimMesh_Attributes.h>

// =======================================================================
// function : SimMesh_Attributes
// =======================================================================
SimMesh_Attributes::SimMesh_Attributes(const bool theIsNormal, const bool theIsColor, const bool theIsTexCoord)
{
    constexpr size_t aSize[] = {0,
                                sizeof(AttrT),
                                sizeof(AttrC),
                                sizeof(AttrTC),
                                sizeof(AttrN),
                                sizeof(AttrNT),
                                sizeof(AttrNC),
                                sizeof(AttrNTC)};

    unsigned int aNCTBits = ((theIsNormal ? 1 : 0) << 2) | ((theIsColor ? 1 : 0) << 1) | (theIsTexCoord ? 1 : 0);
    myData.resize(aSize[aNCTBits], '\0');
}

// =======================================================================
// function : Interpolate
// =======================================================================
void SimMesh_Attributes::Interpolate(const SimMesh_Attributes& theOther, const double theInterParam)
{
    const float aW1 = float(1. - theInterParam);
    const float aW2 = float(theInterParam);
    if (IsNormal() && theOther.IsNormal()) {
        SimMesh_Vec3f& aNorm = ChangeNormal();
        aNorm                = aNorm * aW1 + theOther.GetNormal() * aW2;
        aNorm.Normalize();
    }
    if (IsColor() && theOther.IsColor()) {
        SimMesh_Color&       aC1 = ChangeColor();
        const SimMesh_Color& aC2 = theOther.GetColor();
        aC1.Set(aC1.R() * aW1 + aC2.R() * aW2,
                aC1.G() * aW1 + aC2.G() * aW2,
                aC1.B() * aW1 + aC2.B() * aW2,
                aC1.A() * aW1 + aC2.A() * aW2);
    }
    if (IsTexCoord() && theOther.IsTexCoord()) {
        SimMesh_Vec2f&       aT1 = ChangeTexCoord();
        const SimMesh_Vec2f& aT2 = theOther.GetTexCoord();
        aT1.SetValues(aT1.x() * aW1 + aT2.x() * aW2, aT1.y() * aW1 + aT2.y() * aW2);
    }
}

