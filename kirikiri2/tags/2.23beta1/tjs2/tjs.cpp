//---------------------------------------------------------------------------
/*
	TJS2 Script Engine
	Copyright (C) 2000-2004  W.Dee <dee@kikyou.info>

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// "tTJS" script language API class implementation
//---------------------------------------------------------------------------
// 2004/ 5/13 W.Dee
//   Added code for context-dependent post-positioned ! operator behavior.
//---------------------------------------------------------------------------

#include "tjsCommHead.h"
#pragma hdrstop

#include <map>
#include "tjs.h"
#include "tjsScriptBlock.h"
#include "tjsArray.h"
#include "tjsDictionary.h"
#include "tjsDate.h"
#include "tjsMath.h"
#include "tjsException.h"
#include "tjsInterCodeExec.h"
#include "tjsScriptCache.h"
#include "tjsLex.h"
#include "tjsHashSearch.h"
#include "tjsRandomGenerator.h"




namespace TJS
{
#ifndef TJS_NO_REGEXP
extern iTJSDispatch2 * TJSCreateRegExpClass();
	// to avoid to include large regexp library header
#endif
//---------------------------------------------------------------------------
// TJS Version
//---------------------------------------------------------------------------
tjs_int TJSVersionMajor = 2;
tjs_int TJSVersionMinor = 4;
tjs_int TJSVersionRelease =1;
tjs_int TJSVersionHex = 0x240;

tjs_char TJSCompiledDate[] = TJS_W("" __DATE__ " " __TIME__);
	// first empty literal string is to avoid a compile error with bcc which can not
	// process directly L __DATE__ as a pre-processer wide literal string.
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// global options
//---------------------------------------------------------------------------
bool TJSEvalOperatorIsOnGlobal = false;
	// Post-! operator (evaluate expression) is to be executed on "this" context
	// since TJS2 version 2.4.1
bool TJSWarnOnNonGlobalEvalOperator = false;
	// Output warning against non-local post-! operator.
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTJSPPMap : value container for pre-processor
//---------------------------------------------------------------------------
class tTJSPPMap
{
public:
	tTJSHashTable<ttstr, tjs_int32> Values;
};
//---------------------------------------------------------------------------




#define TJS_GLOBAL_HASH_BITS 7

//---------------------------------------------------------------------------
// tTJS
//---------------------------------------------------------------------------
tTJS::tTJS()
{
	// tTJS constructor
	RefCount = 1;
	ConsoleOutput = NULL;
	PPValues = NULL;

	// ensure variant array stack for function stack
	TJSVariantArrayStackAddRef();

	// ensure hash table for reserved words
	TJSReservedWordsHashAddRef();

	// create script cache object
	Cache = new tTJSScriptCache(this);

	try
	{

		// push version value to pp value
		PPValues = new tTJSPPMap();
		PPValues->Values.Add(ttstr(TJS_W("version")), TJSVersionHex);

		// create the GLOBAL object
		Global = new tTJSCustomObject(TJS_GLOBAL_HASH_BITS);

		// register some default classes to the GLOBAL
		iTJSDispatch2 *dsp;
		tTJSVariant val;

		// Array
		dsp = new tTJSArrayClass(); //TJSCreateArrayClass();
		val = tTJSVariant(dsp, NULL);
		dsp->Release();
		Global->PropSet(TJS_MEMBERENSURE, TJS_W("Array"), NULL, &val, Global);

		// Dictionary
		dsp = new tTJSDictionaryClass();
		val = tTJSVariant(dsp, NULL);
		dsp->Release();
		Global->PropSet(TJS_MEMBERENSURE, TJS_W("Dictionary"), NULL, &val, Global);

		// Date
		dsp = new tTJSNC_Date();
		val = tTJSVariant(dsp, NULL);
		dsp->Release();
		Global->PropSet(TJS_MEMBERENSURE, TJS_W("Date"), NULL, &val, Global);

		// Math
		{
			iTJSDispatch2 * math;

			dsp = math = new tTJSNC_Math();
			val = tTJSVariant(dsp, NULL);
			dsp->Release();
			Global->PropSet(TJS_MEMBERENSURE, TJS_W("Math"), NULL, &val, Global);

			// Math.RandomGenerator
			dsp = new tTJSNC_RandomGenerator();
			val = tTJSVariant(dsp, NULL);
			dsp->Release();
			math->PropSet(TJS_MEMBERENSURE, TJS_W("RandomGenerator"), NULL, &val, math);
		}

		// Exception
		dsp = new tTJSNC_Exception();
		val = tTJSVariant(dsp, NULL);
		dsp->Release();
		Global->PropSet(TJS_MEMBERENSURE, TJS_W("Exception"), NULL, &val, Global);
#ifndef TJS_NO_REGEXP
		// RegExp
		dsp = TJSCreateRegExpClass(); // the body is implemented in tjsRegExp.cpp
		val = tTJSVariant(dsp, NULL);
		dsp->Release();
		Global->PropSet(TJS_MEMBERENSURE, TJS_W("RegExp"), NULL, &val, Global);
#endif
	}
	catch(...)
	{
		TJSVariantArrayStackRelease();
		Global->Release();
		if(PPValues) delete PPValues;
		throw;
	}
}
//---------------------------------------------------------------------------
tTJS::~tTJS()
{
	// tTJS destructor
	TJSVariantArrayStackCompactNow();
	TJSVariantArrayStackRelease();

	Global->Release();
	Global = NULL;

	delete PPValues;
	if(Cache) delete Cache;

	TJSReservedWordsHashRelease();
}
//---------------------------------------------------------------------------
void tTJS::AddRef()
{
	RefCount ++;
}
//---------------------------------------------------------------------------
void tTJS::Release()
{
	if(RefCount == 1)
	{
		delete this;
	}
	else
	{
		RefCount --;
	}
}
//---------------------------------------------------------------------------
void tTJS::Shutdown()
{
	TJSVariantArrayStackCompactNow();
	Global->Clear();
	delete Cache;
	Cache = NULL;
}
//---------------------------------------------------------------------------
iTJSDispatch2 * tTJS::GetGlobal()
{
	Global->AddRef();
	return Global;
}
//---------------------------------------------------------------------------
iTJSDispatch2 * tTJS::GetGlobalNoAddRef() const
{
	return Global;
}
//---------------------------------------------------------------------------
void tTJS::AddScriptBlock(tTJSScriptBlock *block)
{
	ScriptBlocks.push_back(block);
}
//---------------------------------------------------------------------------
void tTJS::RemoveScriptBlock(tTJSScriptBlock *block)
{
	std::vector<tTJSScriptBlock*>::iterator i = ScriptBlocks.begin();
	while(i != ScriptBlocks.end())
	{
		if(*i == block)
		{
			ScriptBlocks.erase(i);
			break;
		}
		i++;
	}
}
//---------------------------------------------------------------------------
void tTJS::SetConsoleOutput(iTJSConsoleOutput* console)
{
	// set a console output callback routine
	ConsoleOutput = console;
}
//---------------------------------------------------------------------------
void tTJS::OutputToConsole(const tjs_char *msg) const
{
	if(ConsoleOutput)
	{
		ConsoleOutput->Print(msg);
	}
}
//---------------------------------------------------------------------------
void tTJS::OutputExceptionToConsole(const tjs_char *msg) const
{
	if(ConsoleOutput)
	{
		ConsoleOutput->Print(msg);
	}
}
//---------------------------------------------------------------------------
void tTJS::OutputToConsoleWithCentering(const tjs_char *msg, tjs_uint width) const
{
	// this function does not matter whether msg includes ZENKAKU characters ...
	if(!msg) return;
	tjs_int len = TJS_strlen(msg);
	tjs_int ns = ((tjs_int)width - len)/2;
	if(ns<=0)
	{
		OutputToConsole(msg);
	}
	else
	{
		tjs_char *outbuf = new tjs_char[ns + len +1];
		tjs_char *p = outbuf;
		while(ns--) *(p++)= TJS_W(' ');
		TJS_strcpy(p, msg);
		try
		{
			OutputToConsole(outbuf);
		}
		catch(...)
		{
			delete [] outbuf;
			throw;
		}

		delete [] outbuf;
	}
}
//---------------------------------------------------------------------------
void tTJS::OutputToConsoleSeparator(const tjs_char *text, tjs_uint count) const
{
	tjs_int len = TJS_strlen(text);
	tjs_char *outbuf = new tjs_char [ len * count + 1];
	tjs_char *p = outbuf;
	while(count--)
	{
		TJS_strcpy(p, text);
		p += len;
	}

	try
	{
		OutputToConsole(outbuf);
	}
	catch(...)
	{
		delete [] outbuf;
		throw;
	}

	delete [] outbuf;
}
//---------------------------------------------------------------------------
void tTJS::Dump(tjs_uint width) const
{
	// dumps all existing script block
	tjs_char version[100];
	TJS_sprintf(version, TJS_W("TJS version %d.%d.%d (%s)"), TJSVersionMajor,
		TJSVersionMinor, TJSVersionRelease, TJSCompiledDate);

	OutputToConsoleSeparator(TJS_W("#"), width);
	OutputToConsoleWithCentering(TJS_W("TJS Context Dump"), width);
	OutputToConsoleSeparator(TJS_W("#"), width);
	OutputToConsole(version);
	OutputToConsole(TJS_W(""));

	if(ScriptBlocks.size())
	{
		std::vector<tTJSScriptBlock*>::const_iterator i;

		tjs_char buf[1024];
		TJS_sprintf(buf, TJS_W("Total %d script block(s)"), ScriptBlocks.size());
		OutputToConsole(buf);
		OutputToConsole(TJS_W(""));

		tjs_uint totalcontexts = 0;
		tjs_uint totalcodesize = 0;
		tjs_uint totaldatasize = 0;

		for(i = ScriptBlocks.begin(); i != ScriptBlocks.end(); i++)
		{
			tjs_uint n;
			const tjs_char * name = (*i)-> GetName();

			ttstr title;
			if(name)
				title = (*i)-> GetNameInfo();
			else
				title = TJS_W("(no-named script block)");

			tjs_char ptr[256];
			TJS_sprintf(ptr, TJS_W(" 0x%p"), (*i));

			title += ptr;

			OutputToConsole(title.c_str());

			n = (*i)->GetContextCount();
			totalcontexts += n;
			TJS_sprintf(buf, TJS_W("\tCount of contexts      : %d"), n);
			OutputToConsole(buf);

			n = (*i)->GetTotalVMCodeSize();
			totalcodesize += n;
			TJS_sprintf(buf, TJS_W("\tVM code area size      : %d words"), n);
			OutputToConsole(buf);

			n = (*i)->GetTotalVMDataSize();
			totaldatasize += n;
			TJS_sprintf(buf, TJS_W("\tVM constant data count : %d"), n);
			OutputToConsole(buf);

			OutputToConsole(TJS_W(""));
		}

		TJS_sprintf(buf, TJS_W("Total count of contexts      : %d"), totalcontexts);
		OutputToConsole(buf);
		TJS_sprintf(buf, TJS_W("Total VM code area size      : %d words"), totalcodesize);
		OutputToConsole(buf);
		TJS_sprintf(buf, TJS_W("Total VM constant data count : %d"), totaldatasize);
		OutputToConsole(buf);

		OutputToConsole(TJS_W(""));


		for(i = ScriptBlocks.begin(); i != ScriptBlocks.end(); i++)
		{

			OutputToConsoleSeparator(TJS_W("-"), width);
			const tjs_char * name = (*i)-> GetName();

			ttstr title;
			if(name)
				title = (*i)-> GetNameInfo();
			else
				title = TJS_W("(no-named script block)");

			tjs_char ptr[256];
			TJS_sprintf(ptr, TJS_W(" 0x%p"), (*i));

			title += ptr;

			OutputToConsoleWithCentering(title.c_str(), width);

			OutputToConsoleSeparator(TJS_W("-"), width);

			(*i)->Dump();

			OutputToConsole(TJS_W(""));
			OutputToConsole(TJS_W(""));
		}
	}
	else
	{
		OutputToConsole(TJS_W(""));
		OutputToConsole(TJS_W("There are no script blocks in the system."));
	}
}
//---------------------------------------------------------------------------
void tTJS::ExecScript(const tjs_char *script, tTJSVariant *result,
	iTJSDispatch2 *context,
	const tjs_char *name, tjs_int lineofs)
{
	TJS_F_TRACE("tTJS::ExecScript");
	TJSSetFPUE();
	if(Cache) Cache->ExecScript(script, result, context, name, lineofs);
}
//---------------------------------------------------------------------------
void tTJS::ExecScript(const ttstr &script, tTJSVariant *result,
	iTJSDispatch2 *context,
	const ttstr *name, tjs_int lineofs)
{
	TJSSetFPUE();
	if(Cache) Cache->ExecScript(script, result, context, name, lineofs);
}
//---------------------------------------------------------------------------
void tTJS::EvalExpression(const tjs_char *expression, tTJSVariant *result,
	iTJSDispatch2 *context,
	const tjs_char *name, tjs_int lineofs)
{
	TJSSetFPUE();
	if(Cache) Cache->EvalExpression(expression, result, context, name, lineofs);
}
//---------------------------------------------------------------------------
void tTJS::EvalExpression(const ttstr &expression, tTJSVariant *result,
	iTJSDispatch2 *context,
	const ttstr *name, tjs_int lineofs)
{
	TJSSetFPUE();
	if(Cache) Cache->EvalExpression(expression, result, context, name, lineofs);
}
//---------------------------------------------------------------------------
void tTJS::SetPPValue(const tjs_char *name, tjs_int32 value)
{
	PPValues->Values.Add(ttstr(name), value);
}
//---------------------------------------------------------------------------
tjs_int32 tTJS::GetPPValue(const tjs_char *name)
{
	tjs_int32 *f = PPValues->Values.Find(ttstr(name));
	if(!f) return 0;
	return *f;
}
//---------------------------------------------------------------------------
void tTJS::DoGarbageCollection()
{
	// do garbage collection
	TJSVariantArrayStackCompactNow();
	TJSCompactStringHeap();
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TextStream Creation
//---------------------------------------------------------------------------
tTJSTextReadStream * TJSDefCreateTextStreamForRead(const tTJSString &name,
	const tTJSString &mode)
{ return NULL; }
tTJSTextWriteStream * TJSDefCreateTextStreamForWrite(const tTJSString &name,
	const tTJSString &mode)
{ return NULL; }
//---------------------------------------------------------------------------
tTJSTextReadStream * (*TJSCreateTextStreamForRead)(const tTJSString &name,
	const tTJSString &mode) =
	TJSDefCreateTextStreamForRead;
tTJSTextWriteStream * (*TJSCreateTextStreamForWrite)(const tTJSString &name,
	const tTJSString &mode) =
	TJSDefCreateTextStreamForWrite;
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// tTJSBinaryStream
//---------------------------------------------------------------------------

void TJS_INTF_METHOD tTJSBinaryStream::SetEndOfStorage()
{
	TJS_eTJSError(TJSWriteError);
}
//---------------------------------------------------------------------------
tjs_uint64 TJS_INTF_METHOD tTJSBinaryStream::GetSize()
{
	tjs_uint64 orgpos = GetPosition();
	tjs_uint64 size = Seek(0, TJS_BS_SEEK_END);
	Seek(orgpos, SEEK_SET);
	return size;
}
//---------------------------------------------------------------------------
tjs_uint64 tTJSBinaryStream::GetPosition()
{
	return Seek(0, SEEK_CUR);
}
//---------------------------------------------------------------------------
void tTJSBinaryStream::SetPosition(tjs_uint64 pos)
{
	if(pos != Seek(pos, TJS_BS_SEEK_SET))
		TJS_eTJSError(TJSSeekError);
}
//---------------------------------------------------------------------------
void tTJSBinaryStream::ReadBuffer(void *buffer, tjs_uint read_size)
{
	if(Read(buffer, read_size) != read_size)
		TJS_eTJSError(TJSReadError);
}
//---------------------------------------------------------------------------
void tTJSBinaryStream::WriteBuffer(const void *buffer, tjs_uint write_size)
{
	if(Write(buffer, write_size) != write_size)
		TJS_eTJSError(TJSWriteError);
}
//---------------------------------------------------------------------------
tjs_uint64 tTJSBinaryStream::ReadI64LE()
{
#if TJS_HOST_IS_BIG_ENDIAN
	tjs_uint8 buffer[8];
	ReadBuffer(buffer, 8);
	tjs_uint64 ret = 0;
	for(tjs_int i=0; i<8; i++)
		ret += (tjs_uint64)buffer[i]<<(i*8);
	return ret;
#else
	tjs_uint64 temp;
	ReadBuffer(&temp, 8);
	return temp;
#endif
}
//---------------------------------------------------------------------------
tjs_uint32 tTJSBinaryStream::ReadI32LE()
{
#if TJS_HOST_IS_BIG_ENDIAN
	tjs_uint8 buffer[4];
	ReadBuffer(buffer, 4);
	tjs_uint32 ret = 0;
	for(tjs_int i=0; i<4; i++)
		ret += (tjs_uint32)buffer[i]<<(i*8);
	return ret;
#else
	tjs_uint32 temp;
	ReadBuffer(&temp, 4);
	return temp;
#endif
}
//---------------------------------------------------------------------------
tjs_uint16 tTJSBinaryStream::ReadI16LE()
{
#if TJS_HOST_IS_BIG_ENDIAN
	tjs_uint8 buffer[2];
	ReadBuffer(buffer, 2);
	tjs_uint16 ret = 0;
	for(tjs_int i=0; i<2; i++)
		ret += (tjs_uint16)buffer[i]<<(i*8);
	return ret;
#else
	tjs_uint16 temp;
	ReadBuffer(&temp, 2);
	return temp;
#endif
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------

} // namespace TJS
