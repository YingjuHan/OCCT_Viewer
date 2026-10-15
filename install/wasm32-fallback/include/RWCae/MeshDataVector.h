#ifndef _MESHDATAVECTOR_HEADERFILE
#define _MESHDATAVECTOR_HEADERFILE

#include <NCollection_Vector.hxx>

namespace RWCae
{
    //! Vector with extra useful methods.
    template <class TheItemType>
    class MeshDataVector : public NCollection_Vector<TheItemType>
    {
    public:
        //! Constructor
        explicit MeshDataVector(const Standard_Integer theIncrement = 256,
                                const Handle(NCollection_BaseAllocator) &theAlloc = NULL)
            : NCollection_Vector<TheItemType>(theIncrement, theAlloc)
        {
        }

        //! Copy constructor
        explicit MeshDataVector(const NCollection_Vector<TheItemType> &theOther) : NCollection_Vector<TheItemType>(theOther)
        {
        }

        //! Return TRUE if vector is allocated using single block of continuous memory
        //! (e.g. the pointer to the first element can be used to access vector as Array).
        bool IsContinuous() const { return NCollection_Vector<TheItemType>::myNBlocks == 1; }

        //! Return the vector increment.
        Standard_Integer Increment() const { return NCollection_Vector<TheItemType>::myIncrement; }

        //! Return number of blocks.
        Standard_Integer NbBlocks() const { return NCollection_Vector<TheItemType>::myNBlocks; }
    };

} // namespace

#endif // _MESHDATAVECTOR_HEADERFILE