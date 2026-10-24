#ifndef _MESHADAPTORSITERATOR_HEADERFILE
#define _MESHADAPTORSITERATOR_HEADERFILE

#include <RWCae/MeshAdaptor.h>

namespace RWCae
{
    //! Helper class to iterate over mesh adaptors defined by template list of mesh element types.
    //! It has two modes:
    //! - Switch - selects one adaptor of specified type and applies specified function to it;
    //! - For - applies specified function to all adaptors.
    //! For convenience, there are pre-defined iterators for all/3d/2d mesh elements.
    template <typename... Elem_T>
    class MeshAdaptorsIterator
    {
    public:
        //! Apply function to mesh adaptor of specified type.
        //! Note that template function should return bool value.
        template <typename Function>
        static bool Switch(RWCae::MeshElemType theType,
                           Function theFunc,
                           const Handle(RWCae::MeshData) & theMeshData,
                           bool theIsSurface = false)
        {
            bool aRes = false;
            ([&]
             {
            if (theType == Elem_T::Type) {
                aRes = theFunc(RWCae::MeshAdaptor<Elem_T>(*theMeshData, theIsSurface));
                return true;
            }
            return false; }() || ...);
            return aRes;
        }

        //! Apply function to all mesh adaptors.
        template <typename Function>
        static void For(Function theFunc, const Handle(RWCae::MeshData) & theMeshData, bool theIsSurface = false)
        {
            (theFunc(RWCae::MeshAdaptor<Elem_T>(*theMeshData, theIsSurface)), ...);
        }
    };

    //! Iterator for mesh adaptors of all types.
    typedef MeshAdaptorsIterator<RWCae::MeshElement0d,
                                 RWCae::MeshElementTriangle,
                                 RWCae::MeshElementTriangle2nd,
                                 RWCae::MeshElementQuad,
                                 RWCae::MeshElementQuad2nd,
                                 RWCae::MeshElementPolygon,
                                 RWCae::MeshElementTetrahedron,
                                 RWCae::MeshElementTetrahedron2nd,
                                 RWCae::MeshElementPyramid,
                                 RWCae::MeshElementPyramid2nd,
                                 RWCae::MeshElementPrism,
                                 RWCae::MeshElementPrism2nd,
                                 RWCae::MeshElementHexahedron,
                                 RWCae::MeshElementHexahedron2nd,
                                 RWCae::MeshElementPolyhedron>
        MeshAdaptorsIteratorAllElems;

    //! Iterator for mesh adaptors of 2d types.
    typedef MeshAdaptorsIterator<RWCae::MeshElementTriangle,
                                 RWCae::MeshElementTriangle2nd,
                                 RWCae::MeshElementQuad,
                                 RWCae::MeshElementQuad2nd,
                                 RWCae::MeshElementPolygon>
        MeshAdaptorsIterator2dElems;

    //! Iterator for mesh adaptors of 3d types.
    typedef MeshAdaptorsIterator<RWCae::MeshElementTetrahedron,
                                 RWCae::MeshElementTetrahedron2nd,
                                 RWCae::MeshElementPyramid,
                                 RWCae::MeshElementPyramid2nd,
                                 RWCae::MeshElementPrism,
                                 RWCae::MeshElementPrism2nd,
                                 RWCae::MeshElementHexahedron,
                                 RWCae::MeshElementHexahedron2nd,
                                 RWCae::MeshElementPolyhedron>
        MeshAdaptorsIterator3dElems;
} // namespace

#endif // _MESHADAPTORSITERATOR_HEADERFILE
