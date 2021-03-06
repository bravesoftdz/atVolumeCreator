#ifndef atVolumeCreatorProjectH
#define atVolumeCreatorProjectH
#include "arraybot/atABExporter.h"
#include "mtkProject.h"
#include "mtkConstants.h"
#include "core/atATObject.h"
#include <vector>

//---------------------------------------------------------------------------
extern const string gVolumeCreatorProjectFileVersion;

namespace tinyxml2
{
	class XMLElement;
}

using namespace std;
using mtk::gEmptyString;
//!A render project is a project as exposed by Render
class RenderProject;

//!We are using an enum for process type in order to save/retrieve different processes from XML
enum VCObjectType
{
	vcoBaseType = 0,
    vcoRenderProject,
    vcoVolume,
    vcoUnknown
};

string 			toString(VCObjectType tp);
VCObjectType 	toVCObjectType(const string& str);


//!A VolumeCreatorProject captures (is a container) for renderprojects and other possible objects related to a project
//!in volume creator
//!A VolumeCreatorProject is the baseclass for such objects.

class VolumeCreatorProject : public mtk::Project, public ATObject
{

    public:
                                                VolumeCreatorProject(const string& projectName = gEmptyString);
                                                ~VolumeCreatorProject();

		virtual bool 							isModified();
        virtual bool                            save(const string& fName = mtk::gEmptyString);
        virtual bool                            open();

        virtual bool							addChild(VolumeCreatorProject* child);

        string                                  getPresentXMLModelVersion();
        virtual mtk::XMLElement*           		addToXMLDocument(mtk::XMLDocument& doc, mtk::XMLNode* docRoot);
		virtual mtk::XMLElement*                addToXMLDocumentAsChild(mtk::XMLDocument& doc, mtk::XMLNode* node);

        virtual bool   							loadFromXML(mtk::XMLNode* node);

		string 									getVCObjectTypeAsString();

        										//!Info text is used if the user want to document the purpose of
                                                //a particular process
        string									mInfoText;

        int										getNumberOfChilds();
        VolumeCreatorProject*					getChild(int i);

    protected:
        bool                                    resetXML();
        int                                     loadVCObjects();

                					            //!The VCObject type help us construct
                                                //a new VC object from a file
        VCObjectType		  		            mVCObjectType;

        VolumeCreatorProject*		            createVCObject(tinyxml2::XMLElement* element);
		RenderProject*							createRenderProject(tinyxml2::XMLElement* element);

        										//!Childs can be various tuypes of objecs, e.g. renderprojects and volumes
        vector<VolumeCreatorProject*>			mChilds;

};

#endif
