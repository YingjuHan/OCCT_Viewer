#ifndef SIMMESH_ATTRIBUTES_HEADERFILE
#define SIMMESH_ATTRIBUTES_HEADERFILE

#include <SimMesh_Color.h>
#include <SimMesh_Types.h>

#include <Standard_Assert.hxx>

#include <vector>

//! Contains the attributes that can be associated with a vertex.
class SimMesh_Attributes {
public:
    //! Constructor takes the storage requirements.
    Standard_EXPORT SimMesh_Attributes(const bool theIsNormal, const bool theIsColor, const bool theIsTexCoord);

    //! Returns true if normal vector is stored.
    bool IsNormal() const
    {
        return myData.size() == sizeof(AttrN) || myData.size() == sizeof(AttrNC) || myData.size() == sizeof(AttrNT)
            || myData.size() == sizeof(AttrNTC);
    }

    //! Returns true if texture coords are stored.
    bool IsTexCoord() const
    {
        return myData.size() == sizeof(AttrT) || myData.size() == sizeof(AttrTC) || myData.size() == sizeof(AttrNT)
            || myData.size() == sizeof(AttrNTC);
    }

    //! Returns true if color is stored.
    bool IsColor() const
    {
        return myData.size() == sizeof(AttrC) || myData.size() == sizeof(AttrTC) || myData.size() == sizeof(AttrNC)
            || myData.size() == sizeof(AttrNTC);
    }

    //! Get access to normal vector.
    const SimMesh_Vec3f& GetNormal() const
    {
        Standard_ASSERT_RAISE(IsNormal(), "Normal is not defined");
        return *reinterpret_cast<const AttrN*>(myData.data());
    }

    //! Get non-const access to normal vector.
    SimMesh_Vec3f& ChangeNormal()
    {
        Standard_ASSERT_RAISE(IsNormal(), "Normal is not defined");
        return *reinterpret_cast<AttrN*>(myData.data());
    }

    //! Get access to texture coords.
    const SimMesh_Vec2f& GetTexCoord() const
    {
        Standard_ASSERT_RAISE(IsTexCoord(), "Texture Coord is not defined");
        if (myData.size() == sizeof(AttrT)) {
            return *reinterpret_cast<const AttrT*>(myData.data());
        } else if (myData.size() == sizeof(AttrTC)) {
            return reinterpret_cast<const AttrTC*>(myData.data())->texCoord;
        } else if (myData.size() == sizeof(AttrNT)) {
            return reinterpret_cast<const AttrNT*>(myData.data())->texCoord;
        } else { // if (myData.size() == sizeof(AttrNTC)) {
            return reinterpret_cast<const AttrNTC*>(myData.data())->texCoord;
        }
    }

    //! Get non-const access to texture coords.
    SimMesh_Vec2f& ChangeTexCoord()
    {
        Standard_ASSERT_RAISE(IsTexCoord(), "Texture Coord is not defined");
        if (myData.size() == sizeof(AttrT)) {
            return *reinterpret_cast<AttrT*>(myData.data());
        } else if (myData.size() == sizeof(AttrTC)) {
            return reinterpret_cast<AttrTC*>(myData.data())->texCoord;
        } else if (myData.size() == sizeof(AttrNT)) {
            return reinterpret_cast<AttrNT*>(myData.data())->texCoord;
        } else { // if (myData.size() == sizeof(AttrNTC)) {
            return reinterpret_cast<AttrNTC*>(myData.data())->texCoord;
        }
    }

    //! Get access to color.
    const SimMesh_Color& GetColor() const
    {
        Standard_ASSERT_RAISE(IsColor(), "Color is not defined");
        if (myData.size() == sizeof(AttrC)) {
            return *reinterpret_cast<const AttrC*>(myData.data());
        } else if (myData.size() == sizeof(AttrTC)) {
            return reinterpret_cast<const AttrTC*>(myData.data())->color;
        } else if (myData.size() == sizeof(AttrNC)) {
            return reinterpret_cast<const AttrNC*>(myData.data())->color;
        } else { // if (myData.size() == sizeof(AttrNTC)) {
            return reinterpret_cast<const AttrNTC*>(myData.data())->color;
        }
    }

    //! Get non-const access to color.
    SimMesh_Color& ChangeColor()
    {
        Standard_ASSERT_RAISE(IsColor(), "Color is not defined");
        if (myData.size() == sizeof(AttrC)) {
            return *reinterpret_cast<AttrC*>(myData.data());
        } else if (myData.size() == sizeof(AttrTC)) {
            return reinterpret_cast<AttrTC*>(myData.data())->color;
        } else if (myData.size() == sizeof(AttrNC)) {
            return reinterpret_cast<AttrNC*>(myData.data())->color;
        } else { // if (myData.size() == sizeof(AttrNTC)) {
            return reinterpret_cast<AttrNTC*>(myData.data())->color;
        }
    }

    //! Modifies this attributes to values interpolated with other attributes.
    Standard_EXPORT void Interpolate(const SimMesh_Attributes& theOther, const double theInterParam);

private:
    //! Define storage class for normal.
    using AttrN = SimMesh_Vec3f;
    //! Define storage class for texture coords.
    using AttrT = SimMesh_Vec2f;
    //! Define storage class for color.
    using AttrC = SimMesh_Color;
    //! Define storage class for normal and texture coords.
    struct AttrNT {
        AttrN normal;
        AttrT texCoord;
    };
    //! Define storage class for normal and color.
    struct AttrNC {
        AttrN normal;
        AttrC color;
    };
    //! Define storage class for texture coords and color.
    struct AttrTC {
        AttrT texCoord;
        AttrC color;
    };
    //! Define storage class for normal, texture coords and color.
    struct AttrNTC {
        AttrN normal;
        AttrT texCoord;
        AttrC color;
    };

private:
    //! Buffer for storage.
    std::vector<char> myData;
};

#endif