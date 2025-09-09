#include <ViewBaseTest/ViewerDrawable.h>

#include <Draw_Display.hxx>

namespace ViewBaseTest {

// =======================================================================
// function : DrawSegments
// =======================================================================
void ViewerDrawable::DrawSegments(Draw_Display&                            theDisp,
                                  const Handle(Graphic3d_ArrayOfSegments)& theSegs,
                                  const Draw_Color&                        theColor)
{
    if (theSegs.IsNull()) {
        return;
    }

    theDisp.SetColor(theColor);
    if (theSegs->EdgeNumber() > 0) {
        const int aNbSegs = theSegs->EdgeNumber() / 2;
        for (int aSegIter = 0; aSegIter < aNbSegs; ++aSegIter) {
            int    aSeg[2]  = {theSegs->Edge(aSegIter * 2 + 1), theSegs->Edge(aSegIter * 2 + 2)};
            gp_Pnt aPnts[2] = {theSegs->Vertice(aSeg[0]), theSegs->Vertice(aSeg[1])};
            theDisp.Draw(aPnts[0], aPnts[1]);
        }
    } else {
        const int aNbSegs = theSegs->VertexNumber() / 2;
        for (int aSegIter = 0; aSegIter < aNbSegs; ++aSegIter) {
            gp_Pnt aPnts[2] = {theSegs->Vertice(aSegIter * 2 + 1), theSegs->Vertice(aSegIter * 2 + 2)};
            theDisp.Draw(aPnts[0], aPnts[1]);
        }
    }
}

}

