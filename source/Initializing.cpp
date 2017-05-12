#include <vcl.h>
#pragma hdrstop
#include "TMainForm.h"
#include "mtkVCLUtils.h"
#include "mtkLogger.h"
#include "atRenderClient.h"
#include "atROIHistory.h"

using namespace mtk;
extern string gAppName;
extern string gAppDataLocation;
//---------------------------------------------------------------------------
void __fastcall TMainForm::FormCreate(TObject *Sender)
{
	this->Caption = vclstr(createWindowTitle("VolumeCreator", Application));
    this->DoubleBuffered = true;

	gLogger.setLogLevel(mLogLevel);
	mLogFileReader.start(true);
    mCurrentRB.setX1(XCoord->getValue());
    mCurrentRB.setY1(YCoord->getValue());
    mCurrentRB.setWidth(Width->getValue());
    mCurrentRB.setHeight(Height->getValue());
    mROIHistory.add(mOriginalRB);

    //Setup renderclient
    mRC.setBaseURL(mBaseUrlE->getValue());
    mRC.getProject().setupForStack(mCurrentOwner.getValue(), mCurrentProject.getValue(), mCurrentStack.getValue());


    //Populate owners
    StringList o = mRC.getOwners();
    if(o.size())
    {
		populateDropDown(o, OwnerCB);
    }

	//Select owner
    int index = OwnerCB->Items->IndexOf(mCurrentOwner.c_str());

    if(index > -1)
    {
		OwnerCB->ItemIndex = index;
        OwnerCB->OnChange(NULL);

        //Select last project
        index = ProjectCB->Items->IndexOf(mCurrentProject.c_str());
        if(index > -1)
        {
            ProjectCB->ItemIndex = index;
            ProjectCB->OnChange(NULL);

            //Then select last stack
            index = StackCB->Items->IndexOf(mCurrentStack.c_str());
            if(index > -1)
            {
                StackCB->ItemIndex = index;
                StackCB->OnChange(NULL);
            }
        }
    }


	enableDisableGroupBox(TestSSHGB, 		false);
	enableDisableGroupBox(StackGenerationGB,false);
//	enableDisableGroupBox(PostProcessingGB, false);

    TabSheet5->TabVisible = false;

    //Setup path for ssh
	TSSHFrame1->ScFileStorage->Path = vclstr(gAppDataLocation);
}

void TMainForm::setupIniFile()
{
	string fldr = getSpecialFolder(CSIDL_LOCAL_APPDATA);
	fldr =  joinPath(fldr, gAppName);

	if(!folderExists(fldr))
	{
		CreateDir(fldr.c_str());
	}

	mIniFileC->init(fldr);

	//For convenience and for option form, populate appProperties container
	mAppProperties.append(&mGeneralProperties);
}

bool TMainForm::setupAndReadIniParameters()
{
	mIniFileC->load();
	mGeneralProperties.setIniFile(mIniFileC->getIniFile());

	//Setup parameters
	mGeneralProperties.setSection("GENERAL");
	mGeneralProperties.add((BaseProperty*)  &mBottomPanelHeight.setup( 	            	"HEIGHT_OF_BOTTOM_PANEL",    	    205));
	mGeneralProperties.add((BaseProperty*)  &mLogLevel.setup( 	                    	"LOG_LEVEL",    	                lAny));

    mGeneralProperties.add((BaseProperty*)  &mBaseUrlE->getProperty()->setup(	        "BASE_URL", 	                    "http://ibs-forrestc-ux1.corp.alleninstitute.org:8081/render-ws/v1"));
    mGeneralProperties.add((BaseProperty*)  &mCurrentOwner.setup(		        		"OWNER", 		                    "Sharmishtaas"));
    mGeneralProperties.add((BaseProperty*)  &mCurrentProject.setup(	    			    "PROJECT", 		                    "M270907_Scnn1aTg2Tdt_13"));
    mGeneralProperties.add((BaseProperty*)  &mCurrentStack.setup(	        			"STACK_NAME", 	                    "ALIGNEDSTACK_DEC12"));

    mGeneralProperties.add((BaseProperty*)  &mScaleE->getProperty()->setup(		        "SCALE", 			                0.02));
    mGeneralProperties.add((BaseProperty*)  &XCoord->getProperty()->setup(	        	"VIEW_X_COORD",    	                0));
    mGeneralProperties.add((BaseProperty*)  &YCoord->getProperty()->setup(	        	"VIEW_Y_COORD",    	                0));
    mGeneralProperties.add((BaseProperty*)  &Width->getProperty()->setup(		        "VIEW_WIDTH", 		                0));
    mGeneralProperties.add((BaseProperty*)  &Height->getProperty()->setup(	        	"VIEW_HEIGHT", 		                0));
    mGeneralProperties.add((BaseProperty*)  &MinIntensity->getProperty()->setup(	    "MIN_INTENSITY", 		            0));
    mGeneralProperties.add((BaseProperty*)  &MaxIntensity->getProperty()->setup(	    "MAX_INTENSITY", 		            65535));

	mGeneralProperties.add((BaseProperty*)  &mImageCacheFolderE->getProperty()->setup(	"IMAGE_CACHE_FOLDER",  				"C:\\ImageCache"));

    //Stack Generation
	mGeneralProperties.add((BaseProperty*)  &VolumesFolder->getProperty()->setup(		"VOLUMES_ROOT_FOLDER",  	  		"/nas1/temp"));
	mGeneralProperties.add((BaseProperty*)  &SubFolder1->getProperty()->setup(			"VOLUMES_SUB_FOLDER_1",  	  		"temp"));
	mGeneralProperties.add((BaseProperty*)  &VolumesScaleE->getProperty()->setup(	   	"VOLUMES_SCALE",  	 		 		0.01));


	//Read from file. Create if file do not exist
	mGeneralProperties.read();

	//Update UI
    mBaseUrlE->update();
    mScaleE->update();
    XCoord->update();
    YCoord->update();
    Width->update();
    Height->update();
	MinIntensity->update();
	MaxIntensity->update();
	mImageCacheFolderE->update();
    VolumesFolder->update();
    SubFolder1->update();
    VolumesScaleE->update();
	mBottomPanel->Height = mBottomPanelHeight;
    return true;
}

