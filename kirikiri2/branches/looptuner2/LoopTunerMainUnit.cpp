//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <mbstring.h>

#include "DSound.h"
#include "LoopTunerMainUnit.h"
#include "LinkDetailUnit.h"
#include "LabelDetailUnit.h"
#include "looptune.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "EditLabelAttribUnit"
#pragma link "EditLinkAttribUnit"
#pragma resource "*.dfm"
TTSSLoopTuner2MainForm *TSSLoopTuner2MainForm;
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
AnsiString __fastcall GetLongFileName(AnsiString fn)
{
	// fn をロングファイル名に変換して返す
	char buf[MAX_PATH];
	char *fnp=0;
	GetFullPathName(fn.c_str(),MAX_PATH,buf,&fnp);
	return AnsiString(buf);
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
static AnsiString OrgCaption;
__fastcall TTSSLoopTuner2MainForm::TTSSLoopTuner2MainForm(TComponent* Owner)
	: TForm(Owner)
{
	Reader = new TWaveReader();
	Reader->OnReadProgress = OnReaderProgress;
	InitDirectSound(Application->Handle);
	Manager = NULL;
	ActiveControl = WaveView;
	CreateWaveView();
	ResettingFlags = false;

	Application->HintHidePause = 24*60*60*1000;
		// not to hide tool tip hint immediately

	ReadFromIni();
	Caption = OrgCaption = Application->Title;

	// open a file specified in command line argument
	int i;
	int paramcount = ParamCount();
	for(i=1; i<=paramcount; i++)
	{
		if(FileExists(ParamStr(i)))
		{
			OpenDialog->FileName = GetLongFileName(ParamStr(i));
			Open();
			break;
		}
	}

}
//---------------------------------------------------------------------------
__fastcall TTSSLoopTuner2MainForm::~TTSSLoopTuner2MainForm()
{
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::ReadFromIni()
{
	// read information from ini file
	const char * section = "Main";

	// window position and size
	ReadWindowBasicInformationFromIniFile(section, this);

	bool b = GetIniFile()->ReadBool(section, "ShowEditFlags", false);
	if(b) ShowEditFlagsAction->Execute();

	OpenDialog->InitialDir = GetIniFile()->ReadString("Folder", "InitialDir", "");
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::WriteToIni()
{
	// write information to ini file
	const char * section = "Main";
	WriteWindowBasicInformationToIniFile(section, this);
	GetIniFile()->WriteBool(section, "ShowEditFlags", ShowEditFlagsAction->Checked);

	if(OpenDialog->FileName != "")
	{
		GetIniFile()->WriteString("Folder", "InitialDir",ExtractFilePath(
			OpenDialog->FileName));
	}
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::FormDestroy(TObject *Sender)
{
	StopPlay();
	FreeDirectSound();
	if(WaveView) delete WaveView;
	if(Manager) delete Manager, Manager = NULL;
	delete Reader;
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::FormShow(TObject *Sender)
{
	DragAcceptFiles(Handle,true);
}
//---------------------------------------------------------------------------
bool __fastcall TTSSLoopTuner2MainForm::GetCanClose()
{
	// get whether the document can be closed
	if(!Manager) return true;
	if(WaveView->Modified)
	{
		int r = MessageDlg("ファイル " + FileName + ".sli は変更されています。\n"
			"変更を保存しますか？", mtWarning, TMsgDlgButtons() << mbYes << mbNo << mbCancel, 0);
		if(r == mrYes)
		{
			SaveAction->Execute();
		}
		else if(r == mrCancel)
		{
			return false;
		}
	}
	return true;
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::FormClose(TObject *Sender,
	  TCloseAction &Action)
{
	WriteToIni();
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::FormCloseQuery(TObject *Sender,
	  bool &CanClose)
{
	CanClose =  GetCanClose();
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::CreateWaveView()
{
	if(!WaveView)
	{
		WaveView = new TWaveView(this);
		WaveView->Parent = this;
		WaveView->Align = alClient;
	}

	WaveView->ClearAll();
	WaveView->Reader = NULL;
	WaveView->OnStopFollowingMarker = WaveViewStopFollowingMarker;
	WaveView->OnWaveDoubleClick		= WaveViewWaveDoubleClick;
	WaveView->OnLinkDoubleClick		= WaveViewLinkDoubleClick;
	WaveView->OnLabelDoubleClick	= WaveViewLabelDoubleClick;
	WaveView->OnNotifyPopup			= WaveViewNotifyPopup;
	WaveView->OnShowCaret			= WaveViewShowCaret;
	WaveView->OnLinkSelected		= WaveViewLinkSelected;
	WaveView->OnLabelSelected		= WaveViewLabelSelected;
	WaveView->OnSelectionLost		= WaveViewSelectionLost;
	WaveView->OnLinkModified		= WaveViewLinkModified;
	WaveView->OnLabelModified		= WaveViewLabelModified;

	WaveView->PopupMenu = WaveViewPopupMenu;

	ActiveControl  = WaveView;
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::ApplicationEventsHint(TObject *Sender)
{
	StatusBar->Panels->Items[4]->Text =   GetLongHint(Application->Hint);
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::OnReaderProgress(TObject *sender)
{
	int pct = Reader->SamplesRead / (Reader->NumSamples / 100);
	if(pct > 100) pct = 100;
	Caption = ExtractFileName(FileName) +  " - 読み込み中 " + AnsiString(pct) + "%";
	if(Reader->ReadDone)
	{
		WaveView->Reader = Reader; // reset the reader
		Manager->SetDecoder(Reader);

		// note that below two wiil call OnLinkModified/OnLabelModified event.
		// but it's ok because the handler will write back the same data to the waveview.
		WaveView->SetLinks(Manager->GetLinks());
		WaveView->SetLabels(Manager->GetLabels());
		WaveView->PushFirstUndoState();

		WaveView->SetInitialMagnify();
		StatusBar->Panels->Items[0]->Text = WaveView->GetMagnifyString();

		Caption = ExtractFileName(FileName) + " - " + OrgCaption;
		Application->Title = Caption;
	}
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::Open()
{
	// check the extension ".sli"
	if(OpenDialog->FileName.Length() >= 4)
	{
		if(!_mbsicmp(OpenDialog->FileName.c_str() +
			(OpenDialog->FileName.Length() - 4), ".sli"))
		{
			AnsiString n = OpenDialog->FileName;
			n.SetLength(n.Length() - 4);
			OpenDialog->FileName = n;
		}
	}

	FileName = OpenDialog->FileName;

	// stop playing
	StopPlay();

	// reset all
	Caption = OrgCaption;
	Application->Title = Caption;
	CreateWaveView();
	FollowMarkerAction->Checked = true;

	// create manager
	if(Manager) delete Manager, Manager = NULL;
	Manager = new tTVPWaveLoopManager();
	IgnoreLinksAction->Checked = false;
	FlagsClearSpeedButtonClick(this);

	// load link and label information
	if(FileExists(FileName + ".sli"))
	{
		char *mem = NULL;
		TFileStream *fs = new TFileStream(FileName + ".sli", fmShareDenyWrite | fmOpenRead);
		try
		{
			try
			{
				// load into memory
				int size = fs->Size;
				mem = new char [size + 1];
				fs->Read(mem, size);
				mem[size] = '\0';
			}
			catch(...)
			{
				delete fs;
				throw;
			}
			delete fs;

			// read from the memory
			if(!Manager->ReadInformation(mem))// note that this method can destory the buffer
			{
				throw Exception(FileName + ".sli は文法が間違っているか、"
					"不正な形式のため、読み込むことができません");
			}
		}
		catch(...)
		{
			if(mem) delete [] mem;
			throw;
		}
		if(mem) delete [] mem;
	}

	// load wave
	Reader->LoadWave(FileName);
}
//---------------------------------------------------------------------------

void __fastcall TTSSLoopTuner2MainForm::OpenActionExecute(TObject *Sender)
{
	if(!GetCanClose()) return;
	OpenDialog->Filter = Reader->FilterString;
	if(OpenDialog->Execute())
	{
		Open();
	}
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::SaveActionExecute(TObject *Sender)
{
	// save current loop information
	TFileStream * fs = new TFileStream(FileName  + ".sli" ,  fmShareDenyWrite | fmCreate);
	try
	{
		Manager->SetLinks(WaveView->GetLinks());
		Manager->SetLabels(WaveView->GetLabels());
		AnsiString str;
		Manager->WriteInformation(str);
		fs->Write(str.c_str(), str.Length());
		WaveView->Modified = false;
	}
	catch(...)
	{
		delete fs;
		throw;
	}
	delete fs;
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::WMDoOpen(TMessage &msg)
{
	if(!GetCanClose()) return;
	Open();
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::WMShowFront(TMessage &msg)
{
	Application->Restore();
	Application->BringToFront();
	this->BringToFront();
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::WMCopyData(TWMCopyData &msg)
{
	if(msg.CopyDataStruct->dwData == 0x746f8ab3)
	{
		if(msg.CopyDataStruct->lpData && msg.CopyDataStruct->cbData >= 6 &&
			!memcmp((const char*)msg.CopyDataStruct->lpData, "open:", 5))
		{
			// open action
			const char *filename = (const char*)(msg.CopyDataStruct->lpData) + 5;

			OpenDialog->FileName = GetLongFileName(filename);
			::PostMessage(Handle, WM_DOOPEN, 0, 0);
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::WMDropFiles(TMessage &msg)
{
	HDROP hd=(HDROP)msg.WParam;

	char FileName[ MAX_PATH ];
	int FileCount=
		DragQueryFile(hd,0xFFFFFFFF,NULL,MAX_PATH);
			//いくつ落とされたか

	for(int i=FileCount-1;i>=0;i--)
	{
		DragQueryFile(hd, i, FileName, MAX_PATH );
		WIN32_FIND_DATA fd;
		HANDLE h;
		if((h = FindFirstFile(FileName,&fd))!=INVALID_HANDLE_VALUE)
		{
			FindClose(h);

			DragFinish(hd);

			OpenDialog->FileName = GetLongFileName(FileName);
			::PostMessage(Handle, WM_DOOPEN, 0, 0);

			return;
		}

	}
	DragFinish(hd);
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::ZoomInActionExecute(TObject *Sender)
{
	WaveView->Magnify ++;
	StatusBar->Panels->Items[0]->Text = WaveView->GetMagnifyString();
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::ZoomOutActionExecute(TObject *Sender)
{
	WaveView->Magnify --;
	StatusBar->Panels->Items[0]->Text = WaveView->GetMagnifyString();
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::UndoActionExecute(TObject *Sender)
{
	WaveView->Undo();
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::RedoActionExecute(TObject *Sender)
{
	WaveView->Redo();
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::DeleteActionExecute(TObject *Sender)
{
	WaveView->DeleteItem();
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::NewLinkActionExecute(TObject *Sender)
{
	WaveView->CreateNewLink();
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::NewLabelActionExecute(TObject *Sender)
{
	WaveView->CreateNewLabel();
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::EditLinkDetailActionExecute(
	  TObject *Sender)
{
	WaveView->PerformLinkDoubleClick();
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::EditLabelDetailActionExecute(
	  TObject *Sender)
{
	WaveView->PerformLabelDoubleClick();
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::PlayFromStartActionExecute(TObject *Sender)
{
	PlayFrom(0);
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::PlayFromCaretActionExecute(
	  TObject *Sender)
{
	PlayFrom(WaveView->CaretPos);
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::StopPlayActionExecute(TObject *Sender)
{
	StopPlay();
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::ShowEditFlagsActionExecute(
	  TObject *Sender)
{
	ShowEditFlagsAction->Checked = !ShowEditFlagsAction->Checked;
	FlagsPanel->Visible = ShowEditFlagsAction->Checked;
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::IgnoreLinksActionExecute(
	  TObject *Sender)
{
	IgnoreLinksAction->Checked = !IgnoreLinksAction->Checked;
 	if(Manager) Manager->SetIgnoreLinks(IgnoreLinksAction->Checked);
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::PlayFrom(int pos)
{
	if(Reader->ReadDone)
	{
		StopPlay();
		const WAVEFORMATEXTENSIBLE * wfx;
		wfx = Reader->GetWindowsFormat();
		Manager->SetPosition(pos);
		WaveView->ResetMarkerFollow();
		StartPlay(wfx, Manager);
	}
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::FollowMarkerActionExecute(
	  TObject *Sender)
{
	FollowMarkerAction->Checked = !FollowMarkerAction->Checked;
	WaveView->FollowingMarker = FollowMarkerAction->Checked;
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::WaveViewStopFollowingMarker(TObject *Sender)
{
	FollowMarkerAction->Checked = false;
	WaveView->FollowingMarker = FollowMarkerAction->Checked;
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::GotoLinkFromActionExecute(
	  TObject *Sender)
{
	if(WaveView->FocusedLink != -1)
	{
		WaveView->CaretPos = WaveView->GetLinks()[WaveView->FocusedLink].From;
		WaveView->EnsureView(WaveView->CaretPos);
		WaveView->ShowCaret = true;
	}
}
//---------------------------------------------------------------------------

void __fastcall TTSSLoopTuner2MainForm::GotoLinkToActionExecute(
	  TObject *Sender)
{
	if(WaveView->FocusedLink != -1)
	{
		WaveView->CaretPos = WaveView->GetLinks()[WaveView->FocusedLink].To;
		WaveView->EnsureView(WaveView->CaretPos);
		WaveView->ShowCaret = true;
	}
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::ApplicationEventsIdle(TObject *Sender,
	  bool &Done)
{
	int pos = (int)GetCurrentPlayingPos();

	if(pos != -1)
	{
		Sleep(1); // this will make the cpu usage low
		Done = false;

		WaveView->MarkerPos = pos;

		StatusBar->Panels->Items[3]->Text = "再生中 " + Reader->SamplePosToTimeString(pos);
		StopPlayAction->Enabled = true;
	}
	else
	{
		Done = true;
		WaveView->MarkerPos = -1;

		StatusBar->Panels->Items[3]->Text = "停止";
		StopPlayAction->Enabled = false;
	}

	UndoAction->Enabled = WaveView->CanUndo();
	RedoAction->Enabled = WaveView->CanRedo();
	DeleteAction->Enabled = WaveView->CanDeleteItem();
	EditLinkDetailAction->Enabled = WaveView->FocusedLink != -1;
	EditLabelDetailAction->Enabled = WaveView->FocusedLabel != -1;
	GotoLinkFromAction->Enabled = WaveView->FocusedLink != -1;
	GotoLinkToAction->Enabled = WaveView->FocusedLink != -1;

	bool en = Reader && Reader->ReadDone;
	SaveAction            ->Enabled  = en;
	PlayFromCaretAction   ->Enabled  = en;
	PlayFromStartAction   ->Enabled  = en;
	ZoomInAction          ->Enabled  = en;
	ZoomOutAction         ->Enabled  = en;
	NewLinkAction         ->Enabled  = en;
	NewLabelAction        ->Enabled  = en;

	if(Manager && Manager->GetFlagsModifiedByLabelExpression()) ResetFlagsEditFromLoopManager();

	POINT mouse_pt;
	GetCursorPos(&mouse_pt);
	mouse_pt = WaveView->ScreenToClient(*(TPoint*)&mouse_pt);
	AnsiString cursor_pos, cursor_ch;
	WaveView->GetPositionStringAt(mouse_pt.x, mouse_pt.y, cursor_pos, cursor_ch);

	StatusBar->Panels->Items[1]->Text = cursor_ch;
	StatusBar->Panels->Items[2]->Text = cursor_pos;
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::WaveViewWaveDoubleClick(TObject *Sender, int pos)
{
	PlayFrom(pos);
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::WaveViewLinkDoubleClick(TObject *Sender, int num, tTVPWaveLoopLink &link)
{
	bool org_waveview_followingmarker =  WaveView->FollowingMarker;
	WaveView->FollowingMarker = true; // temporarily enable the marker

	TLinkDetailForm * ldf = new TLinkDetailForm(this);
	try
	{
		ldf->SetReaderAndLink(Reader, link, num);
		ldf->ShowModal();
	}
	catch(...)
	{
		WaveView->FollowingMarker = org_waveview_followingmarker;
		delete ldf;
		throw;
	}
	WaveView->FollowingMarker = org_waveview_followingmarker;
	delete ldf;
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::WaveViewLabelDoubleClick(TObject *Sender,
	int num, tTVPWaveLabel &label)
{
	TLabelDetailForm * ldf = new TLabelDetailForm(this);
	try
	{
		ldf->SetLabel(label);
		if(ldf->ShowModal() == mrOk)
		{
			WaveView->InvalidateLabel(num);
			label = ldf->GetLabel();
			WaveView->NotifyLabelChanged();
			WaveView->InvalidateLabel(num);
			WaveView->PushUndo(); //==== push undo
		}
	}
	catch(...)
	{
		delete ldf;
		throw;
	}
	delete ldf;
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::WaveViewNotifyPopup(TObject *Sender, AnsiString type,
	const TPoint &point)
{
	if(type == "Link")
	{
		ForLinkPopupMenu->Popup(point.x, point.y);
	}
	else if(type == "Label")
	{
		ForLabelPopupMenu->Popup(point.x, point.y);
	}
	else if(type == "Wave")
	{
		ForWavePopupMenu->Popup(point.x, point.y);
	}
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::WaveViewShowCaret(TObject *Sender, int pos)
{
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::WaveViewLinkSelected(TObject *Sender, int num, tTVPWaveLoopLink &link)
{
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::WaveViewLabelSelected(TObject *Sender, int num, tTVPWaveLabel &label)
{
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::WaveViewSelectionLost(TObject *Sender)
{
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::WaveViewLinkModified(TObject *Sender)
{
	Manager->SetLinks(WaveView->GetLinks());
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::WaveViewLabelModified(TObject *Sender)
{
	Manager->SetLabels(WaveView->GetLabels());
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::FlagsEditToggleMenuItemClick(
	  TObject *Sender)
{
	TEdit *edit = dynamic_cast<TEdit*>(ActiveControl);
	if(edit)
	{
		if(edit->Text.ToInt())
			edit->Text = "0";
		else
			edit->Text = "1";
		edit->SelStart = 1;
	}
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::FlagEdit0DblClick(TObject *Sender)
{
	TEdit *edit = dynamic_cast<TEdit*>(ActiveControl);
	if(edit)
	{
		if(edit->Text.ToInt())
			edit->Text = "0";
		else
			edit->Text = "1";
		edit->SelStart = 1;
	}
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::FlagsEditZeroMenuItemClick(
	  TObject *Sender)
{
	TEdit *edit = dynamic_cast<TEdit*>(ActiveControl);
	if(edit)
		edit->Text = "0";
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::FlasgEditOneMenuItemClick(TObject *Sender)
{
	TEdit *edit = dynamic_cast<TEdit*>(ActiveControl);
	if(edit)
		edit->Text = "1";
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::FlagEdit0MouseDown(TObject *Sender,
	  TMouseButton Button, TShiftState Shift, int X, int Y)
{
	TWinControl *control = dynamic_cast<TWinControl*>(Sender);
	if(control) control->SetFocus();
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::FlagEdit0Change(TObject *Sender)
{
	if(ResettingFlags) return;

	TEdit *edit = dynamic_cast<TEdit*>(Sender);
	if(edit)
	{
		if(edit->Text == "")
		{
			edit->Text = "0";
			edit->SelStart = 1;
		}

		if(edit->Text != "0" && edit->Text[1] == '0')
		{
			edit->Text = AnsiString(edit->Text.ToInt());
			edit->SelStart = edit->Text.Length();
		}

		if(edit->Text.ToInt() > TVP_WL_MAX_FLAG_VALUE)
			edit->Text = AnsiString(TVP_WL_MAX_FLAG_VALUE);
	}

	// refrect all controls
	if(Manager)
	{
		int count = FlagsPanel->ControlCount;
		for(int i = 0; i < count ; i++)
		{
			TControl * comp = FlagsPanel->Controls[i];
			TEdit * edit = dynamic_cast<TEdit *>(comp);
			if(edit)
			{
				if(edit->Name.AnsiPos("FlagEdit") == 1)
				{
					int num = atoi(edit->Name.c_str()  + 8);
					Manager->SetFlag(num, edit->Text.ToInt());
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::FlagEdit0KeyPress(TObject *Sender,
	  char &Key)
{
	if(!(Key >= '0' && Key <= '9' || Key < 0x20)) Key = 0;
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::ResetFlagsEditFromLoopManager()
{
	// display flags from waveloopmanager
	if(!Manager) return;

	ResettingFlags = true;
	tjs_int flags[TVP_WL_MAX_FLAGS];
	Manager->CopyFlags(flags);

	int count = FlagsPanel->ControlCount;
	for(int i = 0; i < count ; i++)
	{
		TControl * comp = FlagsPanel->Controls[i];
		TEdit * edit = dynamic_cast<TEdit *>(comp);
		if(edit)
		{
			if(edit->Name.AnsiPos("FlagEdit") == 1)
			{
				int num = atoi(edit->Name.c_str()  + 8);
				edit->Text = AnsiString(flags[num]);
			}
		}
	}
	ResettingFlags = false;
}
//---------------------------------------------------------------------------
void __fastcall TTSSLoopTuner2MainForm::FlagsClearSpeedButtonClick(
	  TObject *Sender)
{
	ResettingFlags = true;

	int count = FlagsPanel->ControlCount;
	for(int i = 0; i < count ; i++)
	{
		TControl * comp = FlagsPanel->Controls[i];
		TEdit * edit = dynamic_cast<TEdit *>(comp);
		if(edit)
		{
			if(edit->Name.AnsiPos("FlagEdit") == 1)
			{
				edit->Text = "0";
			}
		}
	}

	if(Manager) Manager->ClearFlags();

	ResettingFlags = false;
}
//---------------------------------------------------------------------------

































