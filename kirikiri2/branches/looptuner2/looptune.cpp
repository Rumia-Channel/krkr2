//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <inifiles.hpp>
#include "LoopTunerMainUnit.h"
#include "looptune.h"
USERES("looptune.res");
USEUNIT("WaveReader.cpp");
USEUNIT("RIFFWaveContext.cpp");
USEUNIT("TSSWaveContext.cpp");
USEUNIT("tvpsnd.c");
USEUNIT("WaveView.cpp");
USEUNIT("WaveLoopManager.cpp");
USEUNIT("DSound.cpp");
USEFORM("LoopTunerMainUnit.cpp", TSSLoopTuner2MainForm);
USEUNIT("CharacterSet.cpp");
USEFORM("EditLabelAttribUnit.cpp", EditLabelAttribFrame); /* TFrame: File Type */
USEFORM("EditLinkAttribUnit.cpp", EditLinkAttribFrame); /* TFrame: File Type */
USEFORM("LinkDetailUnit.cpp", LinkDetailForm);
USEFORM("LabelDetailUnit.cpp", LabelDetailForm);
//---------------------------------------------------------------------------
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	HWND wnd = FindWindow("TTSSLoopTuner2MainForm", NULL);
	if(wnd)
	{
		char buf[256];
		GetWindowText(wnd, buf, 256-1);
		if(strstr(buf, "(//designing//)")==NULL)
		{
			PostMessage(wnd, WM_SHOWFRONT, 0, 0);
			int i;
			for(i=1; i<_argc; i++)
			{
				if(FileExists(_argv[i]))
				{
					AnsiString data = "open:" + AnsiString(_argv[i]);
					COPYDATASTRUCT cds;
					cds.dwData = 0x746f8ab3;
					cds.cbData = data.Length() + 1;
					cds.lpData = data.c_str();
					SendMessage(wnd, WM_COPYDATA, NULL, (LPARAM)&cds);
					break;
				}
			}
			return 0;
		}
	}

	try
	{
		Application->Initialize();
		Application->Title = "Loop Tuner";
		Application->CreateForm(__classid(TTSSLoopTuner2MainForm), &TSSLoopTuner2MainForm);
		Application->CreateForm(__classid(TLabelDetailForm), &LabelDetailForm);
		Application->Run();
	}
	catch (Exception &exception)
	{
		Application->ShowException(&exception);
	}

	CommitAndCloseIniFile();
	return 0;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// global supporting functions
//---------------------------------------------------------------------------
static TMemIniFile * MemIniFile = NULL;
//---------------------------------------------------------------------------
TMemIniFile *  __fastcall GetIniFile()
{
	if(!MemIniFile) MemIniFile = new TMemIniFile(ParamStr(0) + ".ini");
	return MemIniFile;
}
//---------------------------------------------------------------------------
void __fastcall CommitIniFile()
{
	if(MemIniFile) MemIniFile->UpdateFile();
}
//---------------------------------------------------------------------------
void __fastcall CommitAndCloseIniFile()
{
	if(MemIniFile)
	{
		MemIniFile->UpdateFile();
		delete MemIniFile;
		MemIniFile = NULL;
	}
}
//---------------------------------------------------------------------------
void __fastcall ReadWindowBasicInformationFromIniFile(const char * section, TForm * form)
{
	// window position and size
	int w, h, l, t;
	w = GetIniFile()->ReadInteger(section, "Width", -1);
	if(w == -1) w = form->Width;
	h = GetIniFile()->ReadInteger(section, "Height", -1);
	if(h == -1) h = form->Height;
	l = GetIniFile()->ReadInteger(section, "Left", -1);
	if(l == -1) l = form->Left;
	t = GetIniFile()->ReadInteger(section, "Top", -1);
	if(t == -1) t = form->Top;

	if(l > Screen->DesktopWidth + Screen->DesktopLeft) l = 0;
	if(t > Screen->DesktopHeight + Screen->DesktopTop) t = 0;

	form->SetBounds(l, t, w, h);
}
//---------------------------------------------------------------------------
void __fastcall WriteWindowBasicInformationToIniFile(const char * section, TForm * form)
{
	// window position and size
	GetIniFile()->WriteInteger(section, "Width", form->Width);
	GetIniFile()->WriteInteger(section, "Height", form->Height);
	GetIniFile()->WriteInteger(section, "Left", form->Left);
	GetIniFile()->WriteInteger(section, "Top", form->Top);
}
//---------------------------------------------------------------------------

