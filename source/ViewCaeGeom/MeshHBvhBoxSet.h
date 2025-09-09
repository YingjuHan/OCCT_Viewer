#ifndef _MESHHBVHBOXSET_HEADERFILE
#define _MESHHBVHBOXSET_HEADERFILE

#include <RWBase/TaskManager.h>

#include <BVH_BoxSet.hxx>
#include <BVH_Traverse.hxx>

#include <vector>

namespace ViewCaeGeom {
//! Two-level BVH.
//! The upper level is a box set built for list of BVH_BoxSet.
//! Low-level BVHs can be updated independently.
template<class T, int N, class DT>
class MeshHBvhBoxSet : public BVH_BoxSet<T, N, int> {
public:
    typedef T                     NumType;
    static constexpr int          Dimension = N;
    typedef DT                    DataType;
    typedef BVH_BoxSet<T, N, int> BoxSet;
    typedef BVH_BoxSet<T, N, DT>  SubBoxSet;

public:
    //! Constructor.
    MeshHBvhBoxSet() : BoxSet() {}

    //! Constructor.
    MeshHBvhBoxSet(const opencascade::handle<BVH_Builder<T, N>>& theBuilder) : BoxSet(theBuilder) {}

    //! Initialize low-level BVHs.
    void Init(int theNbSubSets, int theSubSetNbElems = 0)
    {
        mySubBvhBoxSets.clear();
        mySubBvhBoxSets.reserve(theNbSubSets);
        for (int i = 0; i < theNbSubSets; i++) {
            mySubBvhBoxSets.push_back(new SubBoxSet(BoxSet::Builder()));
            mySubBvhBoxSets.back()->SetSize(theSubSetNbElems);
        }
        BVH_Object<T, N>::myIsDirty = true;
    }

    //! Return low-level BVH by index.
    const Handle(SubBoxSet)& SubSet(int theIdx) const { return mySubBvhBoxSets[theIdx]; }

    //! Add element to the specified low-level BVH.
    void AddElement(int theSubSetIdx, const DT& theElement, const BVH_Box<T, N>& theBox)
    {
        mySubBvhBoxSets[theSubSetIdx]->Add(theElement, theBox);
    }

    //! Build (if needed) low-level BVHs (in parallel if possible) and upper-level BVH.
    virtual void Update() override
    {
        if (!BVH_Object<T, N>::myIsDirty) {
            return;
        }

        BVH_BoxSet<T, N, int>::Clear();
        for (int i = 0; i < (int)mySubBvhBoxSets.size(); i++) {
            if (mySubBvhBoxSets[i]->Size() > 0) {
                BVH_BoxSet<T, N, int>::Add(i, mySubBvhBoxSets[i]->Box());
            }
        }

        BVH_PrimitiveSet<T, N>::Update();

        auto process = [&aSubSets = mySubBvhBoxSets](int, int theIndex) {
            aSubSets[theIndex]->Build();
        };

        int aNbSubSets = (int)mySubBvhBoxSets.size();

        RWBase::TaskManager::Instance()->RunInParallel(0, aNbSubSets, process);
    }

protected:
    std::vector<Handle(SubBoxSet)> mySubBvhBoxSets;
};

//! Iterator for two-level BVH.
template<class BVHSetType>
class MeshHBvhBoxSetTraverse
    : public BVH_Traverse<typename BVHSetType::NumType, BVHSetType::Dimension, BVHSetType, bool> {
    typedef BVH_Traverse<typename BVHSetType::NumType, BVHSetType::Dimension, BVHSetType, bool> BaseClass;
    typedef typename BVH::VectorType<typename BVHSetType::NumType, BVHSetType::Dimension>::Type BVH_VecNt;

public:
    //! Auxiliary iterator for low-level BVH.
    template<class BVHSubSetType>
    class MeshHBvhSubBoxSetTraverse
        : public BVH_Traverse<typename BVHSetType::NumType, BVHSetType::Dimension, BVHSubSetType, bool> {
        typedef BVH_Traverse<typename BVHSetType::NumType, BVHSetType::Dimension, BVHSubSetType, bool> SubBaseClass;

    public:
        //! Consructor.
        MeshHBvhSubBoxSetTraverse(MeshHBvhBoxSetTraverse<BVHSetType>* theParent) : SubBaseClass(), myParent(theParent)
        {
        }

        //! Defines the rules for node rejection by bounding box.
        virtual Standard_Boolean RejectNode(const BVH_VecNt&  theCornerMin,
                                            const BVH_VecNt&  theCornerMax,
                                            Standard_Boolean& theIsInside) const override
        {
            return myParent->RejectNode(theCornerMin, theCornerMax, theIsInside);
        }

        //! Defines the rules for leaf acceptance.
        virtual Standard_Boolean Accept(const Standard_Integer theIndex, const Standard_Boolean& theIsInside) override
        {
            return myParent->Accept(SubBaseClass::myBVHSet, theIndex, theIsInside);
        }

        //! Returns the flag controlling the tree descend.
        virtual Standard_Boolean Stop() const override { return myParent->Stop(); }

    protected:
        MeshHBvhBoxSetTraverse<BVHSetType>* myParent;
    };

public:
    //! Defines the rules for leaf acceptance.
    virtual bool Accept(const Standard_Integer theIndex, const bool&) override
    {
        int aSubSetIdx = BaseClass::myBVHSet->Element(theIndex);

        const Handle(typename BVHSetType::SubBoxSet)& aSubSet = BaseClass::myBVHSet->SubSet(aSubSetIdx);

        MeshHBvhSubBoxSetTraverse<typename BVHSetType::SubBoxSet> aSubTraverse(this);
        aSubTraverse.SetBVHSet(aSubSet.get());
        return aSubTraverse.Select() > 0;
    }

    //! Defines the rules for leaf acceptance.
    virtual Standard_Boolean Accept(const typename BVHSetType::SubBoxSet* theBoxSet,
                                    const Standard_Integer                theIndex,
                                    const Standard_Boolean                theIsInside) = 0;
};
} // namespace

#endif // _MESHHBVHBOXSET_HEADERFILE