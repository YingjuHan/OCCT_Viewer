#ifndef _MESHELEMENTS_HEADERFILE
#define _MESHELEMENTS_HEADERFILE

#include <RWCae/MeshElement.h>

#include <NCollection_Vector.hxx>

namespace RWCae
{
    //! Container for mesh elements of the same type.
    //! Elements are stored as sets of indices in several global arrays:
    //! 1. Nodes
    //!    - stores indices of elements nodes
    //!    - (a1, ..., an, b1, ..., bm, ...) where ai is a node of the first element,
    //!      bj is a node of the second element etc.
    //! 2. Nodes offsets (optional)
    //!    - stores position of the first elements nodes in `Nodes` array
    //!    - is not used for mesh elements with known fixed number of nodes
    //!    - (n, m, ...) where n is a number of nodes in the first element,
    //!      m is a number of nodes in the second element etc.
    //! 3. Faces (optional)
    //!    - stores nodes indices for each element's face
    //!    - is not used for mesh elements with the only face or known fixed number of nodes
    //!    - (k, c1, ..., ck, l, d1, ..., dl, ...) where k, l - number of nodes in face,
    //!      ci, dj - face node index
    //! 4. Faces offsets (optional)
    //!    - stores position of the first elements face in `Faces` array
    //!    - is not used for mesh elements with the only face or known fixed number of nodes
    //!    - (k + 1, k + 1 + l + 1, ...) where k, l - number of nodes in face
    //! 5. Global indices
    //!    - stores indices inside MeshData indexed elements array
    //!    - one index per element
    class MeshElements
    {
    public:
        //! Empty constructor.
        MeshElements() {}

        //! Init.
        template <class Elem_T>
        void Init()
        {
            myToStoreNodesOffsets = hasNodesOffsets<Elem_T>{}.value;
            myToStoreFaces = hasFaces<Elem_T>{}.value;
            setFixedElemSize<Elem_T>(hasNodesOffsets<Elem_T>{});
        }

        //! Return true if offsets for first elements nodes are stored.
        bool ToStoreNodesOffsets() const { return myToStoreNodesOffsets; }

        //! Return true if faces are stored.
        bool ToStoreFaces() const { return myToStoreFaces; }

        //! Return number of stored elements.
        MeshEntityId NbElements() const
        {
            return myToStoreNodesOffsets ? myNodesOffsets.Size() : myNodes.Size() / myFixedElemSize;
        }

        //! Return true if no elements are stored.
        bool IsEmpty() const { return myNodes.IsEmpty(); }

    public:
        //! Clear elements.
        void Clear()
        {
            myNodesOffsets.Clear();
            myNodes.Clear();
            myFacesOffsets.Clear();
            myFaces.Clear();
            myGlobalIndices.Clear();
        }

        //! Return array of first elements nodes offsets.
        const NCollection_Vector<MeshElemEntityOffset> &NodesOffsets() const { return myNodesOffsets; }

        //! Return array of first elements nodes offsets.
        NCollection_Vector<MeshElemEntityOffset> &NodesOffsets() { return myNodesOffsets; }

        //! Return array of elements nodes.
        const NCollection_Vector<MeshEntityId> &Nodes() const { return myNodes; }

        //! Return array of elements nodes.
        NCollection_Vector<MeshEntityId> &Nodes() { return myNodes; }

        //! Return array of first elements faces offsets.
        const NCollection_Vector<MeshElemEntityOffset> &FacesOffsets() const { return myFacesOffsets; }

        //! Return array of first elements faces offsets.
        NCollection_Vector<MeshElemEntityOffset> &FacesOffsets() { return myFacesOffsets; }

        //! Return array of elements faces.
        const NCollection_Vector<MeshElemEntityId> &Faces() const { return myFaces; }

        //! Return array of elements faces.
        NCollection_Vector<MeshElemEntityId> &Faces() { return myFaces; }

        //! Return array of elements global indices.
        const NCollection_Vector<int> &GlobalIndices() const { return myGlobalIndices; }

        //! Return array of elements global indices.
        NCollection_Vector<int> &GlobalIndices() { return myGlobalIndices; }

        //! Return element's global index.
        int GlobalIndex(MeshEntityId theIdx) const { return myGlobalIndices[theIdx]; }

        //! Return number of nodes if fixed elements are stored.
        MeshElemEntityId FixedElemSize() const { return myFixedElemSize; }

    public:
        //! Return nodes indices range for specified mesh element.
        inline void GetElemNodesRange(MeshEntityId theElemIdx,
                                      MeshElemEntityOffset *theNodesStart,
                                      MeshElemEntityOffset *theNodesEnd) const
        {
            if (myToStoreNodesOffsets)
            {
                *theNodesStart = theElemIdx > 0 ? myNodesOffsets[theElemIdx - 1] : 0;
                *theNodesEnd = myNodesOffsets[theElemIdx];
            }
            else
            {
                *theNodesStart = theElemIdx * myFixedElemSize;
                *theNodesEnd = theElemIdx * myFixedElemSize + myFixedElemSize;
            }
        }

        //! Construct and return element with fixed number of nodes.
        template <class Elem_T>
        void GetElementFixed(MeshEntityId theElemIdx, Elem_T &theElem) const
        {
            theElem.SetNodes(myNodes, theElemIdx * myFixedElemSize, theElemIdx * myFixedElemSize + myFixedElemSize);
            theElem.SetGlobalIndex(myGlobalIndices[theElemIdx]);
        }

        //! Construct and return element with arbitrary number of nodes.
        template <class Elem_T>
        void GetElementArb(MeshEntityId theElemIdx, Elem_T &theElem) const
        {
            MeshElemEntityOffset aNodesStart = theElemIdx > 0 ? myNodesOffsets[theElemIdx - 1] : 0;
            MeshElemEntityOffset aNodesEnd = myNodesOffsets[theElemIdx];
            theElem.SetNodes(myNodes, aNodesStart, aNodesEnd);
            theElem.SetGlobalIndex(myGlobalIndices[theElemIdx]);
        }

        //! Construct and return element with arbitrary number of nodes and faces.
        template <class Elem_T>
        void GetElementFaced(MeshEntityId theElemIdx, Elem_T &theElem) const
        {
            MeshElemEntityOffset aNodesStart = theElemIdx > 0 ? myNodesOffsets[theElemIdx - 1] : 0;
            MeshElemEntityOffset aNodesEnd = myNodesOffsets[theElemIdx];
            theElem.SetNodes(myNodes, aNodesStart, aNodesEnd);
            theElem.SetGlobalIndex(myGlobalIndices[theElemIdx]);

            MeshElemEntityOffset aFacesStart = theElemIdx > 0 ? myFacesOffsets[theElemIdx - 1] : 0;
            MeshElemEntityOffset aFacesEnd = myFacesOffsets[theElemIdx];
            theElem.SetFaces(myFaces, aFacesStart, aFacesEnd);
        }

        //! Append a new element.
        template <class Elem_T>
        void AddElement(const Elem_T &theElem, int theGlobalIndex)
        {
            myGlobalIndices.Append(theGlobalIndex);

            MeshElemEntityOffset aPrevNodesOffset = myNodes.Size();
            for (MeshElemEntityId i = theElem.NbNodes(); i > 0; i--)
            {
                myNodes.SetValue(static_cast<int>(aPrevNodesOffset + i - 1), theElem[i - 1]);
            }

            addNodesOffsets(hasNodesOffsets<Elem_T>{});

            addFaces(theElem, hasFaces<Elem_T>{});
        }

        //! Append a new element.
        template <class Elem_T>
        void AddElement(const Elem_T &theElem)
        {
            AddElement<Elem_T>(theElem, theElem.GlobalIndex());
        }

    protected:
        //! Check if nodes offsets should be stored depending on element type.
        template <typename>
        struct hasNodesOffsets : public std::false_type
        {
        };

        //! Check if nodes offsets should be stored depending on element type.
        template <MeshElemType Elem_T>
        struct hasNodesOffsets<MeshElementArb<Elem_T>> : public std::true_type
        {
        };

        //! Check if nodes offsets should be stored depending on element type.
        template <MeshElemType Elem_T>
        struct hasNodesOffsets<MeshElementFaced<Elem_T>> : public std::true_type
        {
        };

        //! Check if faces should be stored depending on element type.
        template <typename>
        struct hasFaces : public std::false_type
        {
        };

        //! Check if faces should be stored depending on element type.
        template <MeshElemType Elem_T>
        struct hasFaces<MeshElementFaced<Elem_T>> : public std::true_type
        {
        };

        //! Add nodes offsets depending on element type.
        void addNodesOffsets(std::true_type) { myNodesOffsets.Append(myNodes.Size()); }

        //! Add nodes offsets depending on element type.
        void addNodesOffsets(std::false_type) {}

        //! Add faces depending on element type.
        template <class Elem_T>
        void addFaces(const Elem_T &theElem, std::true_type)
        {
            MeshElemEntityOffset aPrevFacesOffset = myFaces.Size();
            for (uint16_t i = theElem.FacesDataSize(); i > 0; i--)
            {
                myFaces.SetValue(static_cast<int>(aPrevFacesOffset + i - 1), theElem.FacesData(i - 1));
            }
            myFacesOffsets.Append(myFaces.Size());
        }

        //! Add faces depending on element type.
        template <class Elem_T>
        void addFaces(const Elem_T &, std::false_type)
        {
        }

        //! Set fixed number of nodes depending on element type.
        template <class Elem_T>
        void setFixedElemSize(std::true_type)
        {
        }

        //! Set fixed number of nodes depending on element type.
        template <class Elem_T>
        void setFixedElemSize(std::false_type)
        {
            myFixedElemSize = Elem_T::NbFixedNodes;
        }

    protected:
        NCollection_Vector<MeshElemEntityOffset> myNodesOffsets; //!< (n, n+m, ...)
        NCollection_Vector<MeshEntityId> myNodes;                //!< (a1...an, b1...bm, ...)

        NCollection_Vector<MeshElemEntityOffset> myFacesOffsets; //!< (k, k+l, ...)
        NCollection_Vector<MeshElemEntityId> myFaces;            //!< (k, c1...ck, l, d1...dl, ...)

        NCollection_Vector<int> myGlobalIndices; //!< (a, b, ...)

        MeshElemEntityId myFixedElemSize = 0;

        bool myToStoreNodesOffsets = false; //!< For elements with non-fixed number of nodes
        bool myToStoreFaces = false;        //!< For elements with non-fixed number of faces
    };

} // namespace

#endif // RWCae_MESHELEMENTS_HEADERFILE
