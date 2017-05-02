#include <vcl.h>
#pragma hdrstop
#include "TMainForm.h"
#include "mtkVCLUtils.h"
#include "mtkLogger.h"
#include <vector>
#include "atRenderClient.h"
#include "MagickWand/MagickWand.h"
#include "mtkExeFile.h"
#include "mtkMathUtils.h"
#include "atImageForm.h"
#include "TMemoLogger.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "TFloatLabeledEdit"
#pragma link "TIntegerLabeledEdit"
#pragma link "TSTDStringLabeledEdit"
#pragma link "TIntLabel"
#pragma link "TPropertyCheckBox"
#pragma link "mtkIniFileC"
#pragma link "mtkIntEdit"
#pragma link "ScBridge"
#pragma link "ScSSHChannel"
#pragma link "ScSSHClient"
#pragma link "TSSHFrame"
#pragma resource "*.dfm"
TMainForm *MainForm;
using namespace mtk;
using namespace std;
extern string gLogFileName;

void sendToClipBoard(const string& str);
//---------------------------------------------------------------------------
__fastcall TMainForm::TMainForm(TComponent* Owner)
	: TForm(Owner),
    mLogLevel(lAny),
    mLogFileReader(joinPath(getSpecialFolder(CSIDL_LOCAL_APPDATA), "volumeCreator", gLogFileName), logMsg),
//    mTiffCP("C:\\cygwin\\bin\\tiffcp.exe"),
    mBottomPanelHeight(205),
	mCreateCacheThread(),
    mRC(IdHTTP1),
    mImageForm(NULL)
{
//	mTiffCP.FOnStateEvent = processEvent;
    setupIniFile();
    setupAndReadIniParameters();

    mCreateCacheThread.setCacheRoot(mImageCacheFolderE->getValue());
  	TMemoLogger::mMemoIsEnabled = true;
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::ClickZ(TObject *Sender)
{
	int ii = mZs->ItemIndex;
    if(ii == -1)
    {
    	return;
    }

    int z = toInt(stdstr(mZs->Items->Strings[ii]));

    //Fetch data using URL
    RenderClient rs(IdHTTP1, mBaseUrlE->getValue(), mOwnerE->getValue(), mProjectE->getValue(),
                        mStackNameE->getValue(), "jpeg-image", z, mCurrentRB, mScaleE->getValue(), mImageCacheFolderE->getValue());

    //First check if we already is having this data
	try
    {
        try
        {
            Log(lDebug) << "Loading z = "<<z;
            Log(lDebug) << "URL = "<< rs.getURL();

            TMemoryStream* imageMem = rs.getImage(z);
            if(imageMem)
            {
                Image1->Picture->Graphic->LoadFromStream(imageMem);
                Image1->Invalidate();
            }

            Log(lInfo) << "WxH = " <<Image1->Picture->Width << "x" << Image1->Picture->Height;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            Log(lError) << "There was a memory problem..";
        }
    }
    __finally
    {
    	rs.clearImageMemory();
    }
}

void __fastcall TMainForm::mZMaxEKeyDown(TObject *Sender, WORD &Key, TShiftState Shift)
{
	if(Key == VK_RETURN)
    {
    	UpdateZList();
    }
}

//---------------------------------------------------------------------------
void  TMainForm::UpdateZList()
{
}

void __fastcall TMainForm::mScaleEKeyDown(TObject *Sender, WORD &Key, TShiftState Shift)
{
	if(Key == VK_RETURN)
    {
        mCurrentRB = RenderBox(mXCoordE->getValue(), mYCoordE->getValue(), mWidthE->getValue(), mHeightE->getValue());
		ClickZ(Sender);
    }
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::IdHTTP1Work(TObject *ASender, TWorkMode AWorkMode,
          __int64 AWorkCount)
{
//	Log(lInfo) << "Pos: " << AWorkCount;
//	ProgressBar1->Position = AWorkCount;
//	this->Update();
}

void __fastcall TMainForm::IdHTTP1WorkBegin(TObject *ASender, TWorkMode AWorkMode, __int64 AWorkCountMax)
{
//	ProgressBar1->Max = AWorkCountMax;
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::IdHTTP1Status(TObject *ASender, const TIdStatus AStatus,
          const UnicodeString AStatusText)
{
	Log(lInfo) << "HTTPStatus: " << stdstr(AStatusText);
}

TPoint controlToImage(const TPoint& p, double scale, double stretchFactor)
{
	TPoint pt;
    pt.X = (p.X / scale) / stretchFactor;
    pt.Y = (p.Y / scale) / stretchFactor;
	return pt;
}

double TMainForm::getImageStretchFactor()
{
	if((mScaleE->getValue() * mHeightE->getValue() * mWidthE->getValue()) == 0)
    {
    	Log(lError) << "Tried to divide by zero!";
    	return 1;
    }

    if(Image1->Height < Image1->Width)
    {
    	return Image1->Height / (mScaleE->getValue() * mHeightE->getValue());
    }
    else
    {
		return Image1->Width / (mScaleE->getValue() * mWidthE->getValue());
    }
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::mStretchCBClick(TObject *Sender)
{
//	Image1->Stretch = mStretchCB->Checked;
}

TCanvas* TMainForm::getCanvas()
{
	return PaintBox1->Canvas;
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::FormMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y)
{
	if(Button == TMouseButton::mbRight)
    {
		mResetButton->Click();
        return;
    }

    Drawing = true;
    getCanvas()->MoveTo(X , Y);
    Origin = Point(X, Y);
    MovePt = Origin;

    //For selection
	mTopLeftSelCorner = Mouse->CursorPos;
	mTopLeftSelCorner = this->Image1->ScreenToClient(mTopLeftSelCorner);

	//Convert to world image coords (minus offset)
    double stretchFactor = getImageStretchFactor();
    mTopLeftSelCorner = controlToImage(mTopLeftSelCorner, mScaleE->getValue(), stretchFactor);
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::Image1MouseMove(TObject *Sender, TShiftState Shift, int X, int Y)
{
	TPoint p = this->Image1->ScreenToClient(Mouse->CursorPos);
	mXC->setValue(p.X);
	mYC->setValue(p.Y);

	//Convert to world image coords (minus offset)
    double stretchFactor = getImageStretchFactor();
    if(stretchFactor)
    {
	    p = controlToImage(p, mScaleE->getValue(), stretchFactor);
	  	mX->setValue(p.X);
		mY->setValue(p.Y);
    }
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::FormMouseUp(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y)
{
	if(!Drawing ||  (Button == TMouseButton::mbRight))
    {
    	return;
    }

	Drawing = false;


    //For selection
	mBottomRightSelCorner = this->Image1->ScreenToClient(Mouse->CursorPos);

	//Convert to world image coords (minus offset)
    double stretchFactor = getImageStretchFactor();
    mBottomRightSelCorner = controlToImage(mBottomRightSelCorner, mScaleE->getValue(), stretchFactor);

	//Check if selection indicate a 'reset'
	if(mBottomRightSelCorner.X - mTopLeftSelCorner.X <= 0 || mBottomRightSelCorner.Y - mTopLeftSelCorner.Y <=0)
    {
    	resetButtonClick(NULL);
		return;
    }

	mXCoordE->setValue(mXCoordE->getValue() + mTopLeftSelCorner.X);
	mYCoordE->setValue(mYCoordE->getValue() + mTopLeftSelCorner.Y);

    mWidthE->setValue(mBottomRightSelCorner.X - mTopLeftSelCorner.X);
    mHeightE->setValue(mBottomRightSelCorner.Y - mTopLeftSelCorner.Y);

    //Add to render history
    mCurrentRB = RenderBox(mXCoordE->getValue(), mYCoordE->getValue(), mWidthE->getValue(), mHeightE->getValue());
    mROIHistory.add(mCurrentRB);
	ClickZ(Sender);
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::FormMouseMove(TObject *Sender, TShiftState Shift,
          int X, int Y)
{
	if(Drawing)
  	{
		DrawShape(Origin, MovePt, pmNotXor);
		MovePt = Point(X, Y);
		DrawShape(Origin, MovePt, pmNotXor);
  	}

  Image1MouseMove(Sender, Shift, X, Y);
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::DrawShape(TPoint TopLeft, TPoint BottomRight, TPenMode AMode)
{
  	getCanvas()->Pen->Mode = AMode;
	getCanvas()->Rectangle(TopLeft.x, TopLeft.y, BottomRight.x, BottomRight.y);
}

int	TMainForm::getCurrentZ()
{
	int ii = mZs->ItemIndex;
    if(ii == -1)
    {
    	return -1;
    }

    return toInt(stdstr(mZs->Items->Strings[ii]));
}

void __fastcall TMainForm::resetButtonClick(TObject *Sender)
{
    //Get saved renderbox for current slice
	try
    {
	    mScaleE->setValue(0.05);
        mCurrentRB = mRC.getBoxForZ(getCurrentZ());
        render(&mCurrentRB);
        mROIHistory.clear();
        mROIHistory.add(mCurrentRB);
    }
    catch(...)
    {}
}

void TMainForm::render(RenderBox* box)
{
	if(box)
    {
        mCurrentRB = *(box);
        mXCoordE->setValue(mCurrentRB.getX1());
        mYCoordE->setValue(mCurrentRB.getY1());
        mWidthE->setValue(mCurrentRB.getWidth());
        mHeightE->setValue(mCurrentRB.getHeight());
    }

	ClickZ(NULL);
}

void __fastcall TMainForm::historyBtnClick(TObject *Sender)
{
    TButton* b = dynamic_cast<TButton*>(Sender);
    if(b == mHistoryFFW)
    {
        RenderBox* rb = mROIHistory.next();
        if(rb)
        {
            render(rb);
        }
    }
    else if(b== mHistoryBackBtn)
    {
        RenderBox* rb = mROIHistory.previous();
        if(rb)
        {
            render(rb);
        }
    }
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::TraverseZClick(TObject *Sender)
{
	TButton* b = dynamic_cast<TButton*>(Sender);
    if(mZs->ItemIndex > -1 && mZs->ItemIndex < mZs->Count)
    {
    	mZs->Selected[mZs->ItemIndex] = false;
    }
    mZs->Selected[mZs->ItemIndex] = true;
    render();
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::mFetchSelectedZsBtnClick(TObject *Sender)
{
    int z = toInt(stdstr(mZs->Items->Strings[0]));
	RenderClient rs(IdHTTP1, mBaseUrlE->getValue(), mOwnerE->getValue(), mProjectE->getValue(),
	    mStackNameE->getValue(), "jpeg-image", z, mCurrentRB, mScaleE->getValue(), mImageCacheFolderE->getValue());

    //Create image URLs
    StringList urls;
    for(int i = 0; i < mZs->Count; i++)
    {
        int	z = toInt(stdstr(mZs->Items->Strings[i]));
    	urls.append(rs.getURLForZ(z));
    }

	mCreateCacheThread.setup(urls, mImageCacheFolderE->getValue());
    if(!mCreateCacheThread.isRunning())
    {
		mCreateCacheThread.start();
    }
    else
    {
    	Log(lInfo) << "Creating cache thread is already running..";
    }
}


//---------------------------------------------------------------------------
void __fastcall TMainForm::mMoveOutSelectedBtnClick(TObject *Sender)
{
//	if(mZs->SelCount == 0)
//    {
//    	Log(lWarning) << "There are no items selected...";
//        return;
//    }
//
//	int selectedAfter;
//    for(int i = 0; i < mZs->Count; i++)
//    {
//    	if(mZs->Selected[i])
//        {
// 			String item = mZs->Items->Strings[i];
//        	mDeSelectedZs->AddItem(item, NULL);
//            selectedAfter = i;
//        }
//    }
//
//    int selCount = mZs->SelCount;
//    mZs->DeleteSelected();
//
//    int newlySelected = selectedAfter - (selCount -1);
//    if(newlySelected > -1 && newlySelected < mZs->Count)
//    {
//    	mZs->Selected[newlySelected] = true;
//        mZs->ItemIndex = newlySelected;
//        render();
//    }
//
//    sortTListBoxNumerically(mDeSelectedZs);
}


void __fastcall TMainForm::mRestoreUnselectedBtnClick(TObject *Sender)
{
//    for(int i = 0; i < mDeSelectedZs->Count; i++)
//    {
//    	if(mDeSelectedZs->Selected[i])
//        {
// 			String item = mDeSelectedZs->Items->Strings[i];
//        	mZs->AddItem(item, NULL);
//        }
//    }
//    mDeSelectedZs->DeleteSelected();
//    sortTListBoxNumerically(mZs);
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::mBrowseForCacheFolderClick(TObject *Sender)
{
	//Browse for folder
	string res = browseForFolder(mImageCacheFolderE->getValue());
    if(folderExists(res))
    {
		mImageCacheFolderE->setValue(res);
    }
    else
    {
    	Log(lWarning) << "Cache folder was not set..";
    }
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::mGetValidZsBtnClick(TObject *Sender)
{
	//Fetch valid zs for current project
   	RenderClient rs(IdHTTP1, mBaseUrlE->getValue(), mOwnerE->getValue(), mProjectE->getValue(),	mStackNameE->getValue());
    StringList zs = rs.getValidZs();

	Log(lInfo) << "Fetched "<<zs.count()<<" valid z's";

    //Populate list box
	populateListBox(zs, mZs);
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::mCLearMemoClick(TObject *Sender)
{
	infoMemo->Clear();
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::mUpdateZsBtnClick(TObject *Sender)
{
	//Fetch valid zs for current project
   	RenderClient rs(IdHTTP1, mBaseUrlE->getValue(), mOwnerE->getValue(), mProjectE->getValue(),	mStackNameE->getValue());
    StringList zs = rs.getZs();

    if(zs.size() > 1)
    {
    	//Populate list boxes
	    Log(lInfo) << "Valid Z's: "<<zs[0];
    	Log(lInfo) << "Missing Z's: "<<zs[1];
    }

//    //Populate list boxes
//    mValidZsLB->Clear();
//    if(zs.size())
//    {
//        StringList validZ(zs[0], ',');
//        for(int i = 0; i < validZ.size(); i++)
//        {
//	        mValidZsLB->Items->Add(vclstr(validZ[i]));
//        }
//    }

//    mMissingZsLB->Clear();
//    if(zs.size() > 1)
//    {
//        StringList missingZ(zs[1], ',');
//        for(int i = 0; i < missingZ.size(); i++)
//        {
//	        mMissingZsLB->Items->Add(vclstr(missingZ[i]));
//        }
//    }
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::CopyValidZs1Click(TObject *Sender)
{
	//Figure out wich listbox called

	TListBox* lb = dynamic_cast<TListBox*>(PopupMenu1->PopupComponent);

    if(!lb)
    {
    	return;
    }
    stringstream zs;
    for(int i = 0; i < lb->Count; i++)
    {
    	zs << stdstr(lb->Items->Strings[i]);
        if(i < (lb->Count -1))
        {
        	zs <<",";
        }
    }
	sendToClipBoard(zs.str());
}

void sendToClipBoard(const string& str)
{
    const char* output = str.c_str();
    const size_t len = strlen(output) + 1;
    HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE, len);
    memcpy(GlobalLock(hMem), output, len);
    GlobalUnlock(hMem);
    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::GetOptimalBoundsBtnClick(TObject *Sender)
{
	RenderClient rs(IdHTTP1, mBaseUrlE->getValue(), mOwnerE->getValue(), mProjectE->getValue(),	mStackNameE->getValue());

    vector<int> zs = rs.getValidZs();
    RenderBox box = rs.getOptimalXYBoxForZs(zs);
    Log(lInfo) << "XMin = " << box.getX1();
    Log(lInfo) << "XMax = " << box.getX2();
    Log(lInfo) << "YMin = " << box.getY1();
    Log(lInfo) << "YMax = " << box.getY2();

   	vector<RenderBox> bounds = rs.getBounds();
    for(int i = 0; i < bounds.size(); i++)
    {
	    Log(lInfo) <<bounds[i].getZ()<<","<<bounds[i].getX1()<<","<<bounds[i].getX2()<<","<<bounds[i].getY1()<<","<<bounds[i].getY2();
    }
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::mValidZsLBClick(TObject *Sender)
{
	//Fetch section info
	//Get selected z
//    int index = mValidZsLB->ItemIndex;
//    if(index != -1)
//    {
//        RenderClient rs(IdHTTP1, mBaseUrlE->getValue(), mOwnerE->getValue(), mProjectE->getValue(),	mStackNameE->getValue());
//
//        vector<int> zs;
//        int test = mValidZsLB->Items->Strings[index].ToInt();
//        zs.push_back(test);
//
//        RenderBox box = rs.getOptimalXYBoxForZs(zs);
//
//        Log(lInfo) << "XMin = " << box.getX1();
//        Log(lInfo) << "XMax = " << box.getX2();
//        Log(lInfo) << "YMin = " << box.getY1();
//        Log(lInfo) << "YMax = " << box.getY2();
//    }
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::mZoomBtnClick(TObject *Sender)
{

	TButton* b = dynamic_cast<TButton*>(Sender);

	double zoomFactor = mZoomFactor->getValue();
    if(b == mZoomOutBtn)
    {
		zoomFactor *= (-1.0);
    }

	//Modify bounding box with x%
    mCurrentRB = RenderBox(mXCoordE->getValue(), mYCoordE->getValue(), mWidthE->getValue(), mHeightE->getValue());
    mCurrentRB.zoom(zoomFactor);

	mXCoordE->setValue(mCurrentRB.getX1());
    mYCoordE->setValue(mCurrentRB.getY1());
    mWidthE->setValue( mCurrentRB.getWidth());
    mHeightE->setValue(mCurrentRB.getHeight());

    //Scale the scaling
    double scale  = (double) Image1->Height / (double) mCurrentRB.getHeight();
    Log(lInfo) << "Scaling is: " << scale;
	if(scale < 0.005)
    {
    	scale = 0.009;
    }
    else if(scale > 1.0)
    {
    	scale = 1.0;
    }
	mScaleE->setValue(scale);
	ClickZ(Sender);
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::mOwnersCBChange(TObject *Sender)
{
	//Update Projects CB
    //Get selected owner
    if(mOwnersCB->ItemIndex == -1)
    {
		return;
    }

    string owner = stdstr(mOwnersCB->Items->Strings[mOwnersCB->ItemIndex]);
    mOwnerE->setValue(owner);

    //Populate projects
    StringList p = mRC.getProjectsForOwner(mOwnerE->getValue());
    if(p.size())
    {
		populateDropDown(p, mProjectsCB);
    }
}

void __fastcall TMainForm::mProjectsCBChange(TObject *Sender)
{
	//Update Stacks CB
    //Get selected owner
    if(mProjectsCB->ItemIndex == -1)
    {
		return;
    }

    string owner = stdstr(mOwnersCB->Items->Strings[mOwnersCB->ItemIndex]);
    string project = stdstr(mProjectsCB->Items->Strings[mProjectsCB->ItemIndex]);
    mProjectE->setValue(project);

    //Populate stacks
    StringList s = mRC.getStacksForProject(owner, mProjectE->getValue());
    if(s.size())
    {
		populateDropDown(s, mStacksCB);
    }
}

void __fastcall TMainForm::mStacksCBChange(TObject *Sender)
{

    if(mStacksCB->ItemIndex == -1)
    {
		return;
    }

    string stack = stdstr(mStacksCB->Items->Strings[mStacksCB->ItemIndex]);
	mStackNameE->setValue(stack);

	mRC.getProject().setupForStack(mOwnerE->getValue(), mProjectE->getValue(), mStackNameE->getValue());
   	mGetValidZsBtnClick(NULL);
    resetButtonClick(NULL);
	ClickZ(NULL);
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::mDetachBtnClick(TObject *Sender)
{
	//Open image form
//    if(!mImageForm)
    {
    	mImageForm = new TImageForm(gApplicationRegistryRoot, "ImageForm", this);

    }
	mImageForm->Show();
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::mCloseBottomPanelBtnClick(TObject *Sender)
{
	mBottomPanel->Visible = false;
    mShowBottomPanelBtn->Visible = true;
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::mShowBottomPanelBtnClick(TObject *Sender)
{
	mBottomPanel->Visible = true;
    mShowBottomPanelBtn->Visible = false;
    Splitter2->Top = mBottomPanel->Top - 1;
}


void __fastcall TMainForm::TSSHFrame1ScSSHShell1AsyncReceive(TObject *Sender)
{
	MLog() << stdstr(TSSHFrame1->ScSSHShell1->ReadString());
}


void __fastcall TMainForm::CMDButtonClick(TObject *Sender)
{
	stringstream cmd;
    cmd << stdstr(mCMD->Text) << endl;
    TSSHFrame1->ScSSHShell1->WriteString(vclstr(cmd.str()));
}

string escape(const string& before)
{
    string escaped(before);
    //Pretty bisarre syntax.. see http://stackoverflow.com/questions/1250079/how-to-escape-single-quotes-within-single-quoted-strings
    escaped = replaceSubstring("'", "'\"'\"'", escaped);
    return "'" + escaped + "'";
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::RunClick(TObject *Sender)
{
	string scriptName("getVolume.sh");
    stringstream cmd;
	//First copy our script to host
    string remoteScript(joinPath(stdstr(mVolumesFolder->Text), scriptName, '/'));
	cmd << "touch "<< remoteScript << endl;
    TSSHFrame1->ScSSHShell1->WriteString(vclstr(cmd.str()));
	cmd.str("");

	cmd << "chmod +x "<< remoteScript << endl;
    TSSHFrame1->ScSSHShell1->WriteString(vclstr(cmd.str()));
	cmd.str("");

    StringList lines(getStrings(BashScriptMemo));
    if(lines.size() <= 1)
    {
    	Log(lError) << "Can't populate remote script. Script Memo is Empty??";
        return;
    }

	cmd << "echo \"\" > " <<remoteScript << endl;
    //Copy content of memo
    for(int i = 0 ; i < lines.size(); i++)
    {
		Log(lInfo) << "echo "<<escape(lines[i])<< " >> " << remoteScript << endl;
		cmd << "echo "<< escape(lines[i])<< " >> " << remoteScript << endl;
    }

    TSSHFrame1->ScSSHShell1->WriteString(vclstr(cmd.str()));
    cmd.str("");

	//Create commandline for remote bash script
	cmd<< remoteScript;

    //First argument is number of sections
    cmd<<" "<<mZs->Count;

    //Second argument is section numbers
    cmd << " '";
	for(int i = 0; i < mZs->Count; i++)
//	for(int i = 0; i < 2; i++)
    {
    	cmd << mZs->Items->Strings[i].ToInt();
        if(i < mZs->Count -1)
        {
        	cmd << " ";
        }
    }
    cmd <<"'";

    //Third argument is root outputfolder
    cmd <<" "<<stdstr(mVolumesFolder->Text);

    //Fourth arg is custom outputfolder
	cmd <<" "<<stdstr(mCustomOutputFolder->Text);

    //Fifth arg is custom outputfolder
	cmd <<" "<<stdstr(mChannelNameE->Text);

	//Sixth is owner
    cmd <<" "<<stdstr(mOwnerE->Text);

    //7th - project
    cmd <<" "<<stdstr(mProjectE->Text);

	//8th - stack
	cmd <<" "<<stdstr(mStackNameE->Text);

    //9th - scale
	cmd <<" "<<stdstr(ScaleE->Text);

	//10th - static bounds?
    cmd <<" "<<mtk::toString(BoundsCB->Checked);

    if(BoundsCB->Checked)
    {
    	//Pass bounds, xmin, xmax, ymin,ymax
        cmd <<" '"
        	<<mXCoordE->getValue()<<","
        	<<mXCoordE->getValue() + mWidthE->getValue()<<","
            <<mYCoordE->getValue()<<","
            <<mYCoordE->getValue() + mHeightE->getValue()<<"'";
    }

    cmd << endl;

    //Now execute..
    TSSHFrame1->ScSSHShell1->WriteString(vclstr(cmd.str()));
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::TSSHFrame1ScSSHClientAfterConnect(TObject *Sender)
{
	enableDisableGroupBox(StackGenerationGB, true);
	enableDisableGroupBox(TestSSHGB, true);

  	TSSHFrame1->ScSSHClientAfterConnect(Sender);
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::TSSHFrame1ScSSHClientAfterDisconnect(TObject *Sender)
{
	TSSHFrame1->ScSSHClientAfterDisconnect(Sender);
	enableDisableGroupBox(StackGenerationGB, false);
	enableDisableGroupBox(TestSSHGB, false);
}

//---------------------------------------------------------------------------
void __fastcall TMainForm::FormKeyDown(TObject *Sender, WORD &Key, TShiftState Shift)
{
	if(Key == VK_ESCAPE)
    {
        Close();
    }
}


