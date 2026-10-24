#ifndef _NATIVECUTSESSION_HEADERFILE
#define _NATIVECUTSESSION_HEADERFILE
#include <Viewer/NativeCutTimeline.h>
#include <RWVtk/VtkResourceLimits.h>
namespace Viewer {
//! Version 4 native sessions reference frozen source bundles with SHA-256 identities.
//! All resolution, validation and AIS preparation completes before live publication.
class Standard_EXPORT NativeCutSession {
public:
    NativeCutSession(NativeCutView& owner,NativeCutTimeline& timeline):myOwner(owner),myTimeline(timeline) {}
    bool Save(const std::string& file,const std::string& meshSource,const std::string& cadSource="");
    bool Load(const std::string& file);
    const std::string& Source() const { return mySource; }
    const std::string& CadSource() const { return myCadSource; }
    void SetLimits(const RWVtk::VtkResourceLimits& limits) { myLimits=limits; }
private:
    NativeCutView& myOwner;
    NativeCutTimeline& myTimeline;
    std::string mySource,myCadSource;
    RWVtk::VtkResourceLimits myLimits;
};
}
#endif
