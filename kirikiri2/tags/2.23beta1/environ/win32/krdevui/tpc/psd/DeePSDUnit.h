//---------------------------------------------------------------------------
/*
	fgconvert   (graphic converter for transparent images)
	Copyright (C) 2000-2004  W.Dee <dee@kikyou.info>

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
#ifndef DeePSDUnitH
#define DeePSDUnitH

#include <vcl.h>

//---------------------------------------------------------------------------
extern void __fastcall SetGrayscalePalette(Graphics::TBitmap *dib, int bit);
//---------------------------------------------------------------------------
// 例外クラス
class EDeePSD : public Exception
{
public:
	__fastcall EDeePSD(const System::AnsiString Msg) : Sysutils::Exception(
		Msg) { }
	__fastcall EDeePSD(const System::AnsiString Msg, const System::TVarRec
		* Args, const int Args_Size) : Sysutils::Exception(Msg, Args, Args_Size) { }
	__fastcall EDeePSD(int Ident, Extended Dummy) : Sysutils::Exception(
		Ident, Dummy) { }
	__fastcall EDeePSD(int Ident, const System::TVarRec * Args, const
		int Args_Size) : Sysutils::Exception(Ident, Args, Args_Size) { }
	__fastcall EDeePSD(const System::AnsiString Msg, int AHelpContext
		) : Sysutils::Exception(Msg, AHelpContext) { }
	__fastcall EDeePSD(const System::AnsiString Msg, const System::TVarRec
		* Args, const int Args_Size, int AHelpContext) : Sysutils::Exception(Msg,
		 Args, Args_Size, AHelpContext) { }
	__fastcall EDeePSD(int Ident, int AHelpContext) : Sysutils::
		Exception(Ident, AHelpContext) { }
	__fastcall EDeePSD(int Ident, const System::TVarRec * Args
		, const int Args_Size, int AHelpContext) : Sysutils::Exception(Ident,
		 Args, Args_Size, AHelpContext) { }

	__fastcall virtual ~EDeePSD(void) { }

};
//---------------------------------------------------------------------------
// TDeePSD クラス
class TDeePSD : public Graphics::TBitmap
{
private:
protected:

public:
	// コンストラクタとデストラクタ
	__fastcall TDeePSD(void);
	__fastcall ~TDeePSD(void);


	// Stream I/O  のオーバーライド
	void __fastcall LoadFromStream(Classes::TStream * Stream);
	void __fastcall SaveToStream(Classes::TStream * Stream);

public:
};
//---------------------------------------------------------------------------
#endif
