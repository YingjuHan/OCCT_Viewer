#ifndef SIMMESH_QUADRIC_HEADERFILE
#define SIMMESH_QUADRIC_HEADERFILE

#include <SimMesh_Types.h>

//! 3D Quadric Error Metric.
//! Represented by 4x4 symmetric matrix.
class SimMesh_Quadric {
public:
    //! Empty constructor.
    SimMesh_Quadric() { Clear(); }

    //! Constructor from A B C D Area coefficients.
    SimMesh_Quadric(double theA, double theB, double theC, double theD, double theArea = 1.0)
    {
        Init(theA, theB, theC, theD, theArea);
    }

    //! Constructor from ABC D Area coefficients.
    SimMesh_Quadric(const SimMesh_Vec3d& theN, double theD, double theArea = 1.0)
    {
        Init(theN.x(), theN.y(), theN.z(), theD, theArea);
    }

    //! Initializes quadric from A B C D Area coefficients.
    void Init(double theA, double theB, double theC, double theD, double theArea)
    {
        myMat[Pos_AA] = theA * theA;
        myMat[Pos_AB] = theA * theB;
        myMat[Pos_AC] = theA * theC;
        myMat[Pos_AD] = theA * theD;
        myMat[Pos_BB] = theB * theB;
        myMat[Pos_BC] = theB * theC;
        myMat[Pos_BD] = theB * theD;
        myMat[Pos_CC] = theC * theC;
        myMat[Pos_CD] = theC * theD;
        myMat[Pos_DD] = theD * theD;
        myArea        = theArea;
    }

    //! Returns 3x3 tensor part of 4x4 matrix.
    SimMesh_Mat3d GetTensor() const
    {
        SimMesh_Mat3d aMat;
        aMat.SetRow(0, SimMesh_Vec3d(myMat[Pos_AA], myMat[Pos_AB], myMat[Pos_AC]));
        aMat.SetRow(1, SimMesh_Vec3d(myMat[Pos_BA], myMat[Pos_BB], myMat[Pos_BC]));
        aMat.SetRow(2, SimMesh_Vec3d(myMat[Pos_CA], myMat[Pos_CB], myMat[Pos_CC]));
        return aMat;
    }

    //! Returns 3d vector part of 4x4 matrix.
    SimMesh_Vec3d GetVector() const { return SimMesh_Vec3d(myMat[Pos_AD], myMat[Pos_BD], myMat[Pos_CD]); }

    //! Returns area.
    double GetArea() const { return myArea; }

    //! Sets area value.
    void SetArea(double theArea) { myArea = theArea; }

    //! Clears values.
    void Clear(double theVal = 0.0)
    {
        for (int aValIter = Pos_AA; aValIter <= Pos_DD; ++aValIter) {
            myMat[aValIter] = theVal;
        }
        myArea = theVal;
    }

    //! Adds another quadric.
    SimMesh_Quadric& operator+=(const SimMesh_Quadric& theQ)
    {
        for (int aValIter = Pos_AA; aValIter <= Pos_DD; ++aValIter) {
            myMat[aValIter] += theQ.myMat[aValIter];
        }
        myArea += theQ.myArea;
        return *this;
    }

    //! Scales coefficients.
    SimMesh_Quadric& operator*=(double theScalar)
    {
        for (int aValIter = Pos_AA; aValIter <= Pos_DD; ++aValIter) {
            myMat[aValIter] *= theScalar;
        }
        return *this;
    }

    //! Evaluates vAv + 2bv + c
    double Evaluate(double theX, double theY, double theZ) const
    {
        return theX * theX * myMat[Pos_AA] + 2 * theX * theY * myMat[Pos_AB] + 2 * theX * theZ * myMat[Pos_AC]
             + 2 * theX * myMat[Pos_AD] + theY * theY * myMat[Pos_BB] + 2 * theY * theZ * myMat[Pos_BC]
             + 2 * theY * myMat[Pos_BD] + theZ * theZ * myMat[Pos_CC] + 2 * theZ * myMat[Pos_CD] + myMat[Pos_DD];
    }

    //! Evaluates vAv + 2bv + c
    double Evaluate(const SimMesh_Vec3d& theVec3) const { return Evaluate(theVec3.x(), theVec3.y(), theVec3.z()); }

    //! Computes optimal vector.
    bool Optimize(SimMesh_Vec3d& theVec) const
    {
        SimMesh_Mat3d aMatInv;
        double        aDet = 0.0;
        if (!GetTensor().Inverted(aMatInv, aDet) || Abs(aDet) < 1e-12) {
            return false;
        }

        theVec = -(aMatInv * GetVector());
        return true;
    }

private:
    //! Store unique coefficients of symmetric 4x4 matrix
    //! | AA AB AC AD |
    //! |    BB BC BD |
    //! |       CC CD |
    //! |          DD |
    enum {
        Pos_AA,
        Pos_AB,
        Pos_AC,
        Pos_AD,
        Pos_BB,
        Pos_BC,
        Pos_BD,
        Pos_CC,
        Pos_CD,
        Pos_DD,
        //
        Pos_BA = Pos_AB,
        Pos_CA = Pos_AC,
        Pos_CB = Pos_BC
    };

private:
    double myMat[Pos_DD + 1]; //!< Packed coefficients of symmetric matrix.
    double myArea;            //!< Area.
};

#endif // SIMMESH_QUADRIC_HEADERFILE
