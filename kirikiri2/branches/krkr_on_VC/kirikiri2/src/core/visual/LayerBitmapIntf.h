//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Base Layer Bitmap implementation
//---------------------------------------------------------------------------
#ifndef LayerBitmapIntfH
#define LayerBitmapIntfH


#include "ComplexRect.h"
#include "tvpgl.h"
#include "argb.h"

/*[*/
//---------------------------------------------------------------------------
// tTVPBBBltMethod and tTVPBBStretchType
//---------------------------------------------------------------------------
enum tTVPBBBltMethod
{
	bmCopy,
	bmCopyOnAlpha,
	bmAlpha,
	bmAlphaOnAlpha,
	bmAdd,
	bmSub,
	bmMul,
	bmDodge,
	bmDarken,
	bmLighten,
	bmScreen,
	bmAddAlpha,
	bmAddAlphaOnAddAlpha,
	bmAddAlphaOnAlpha,
	bmAlphaOnAddAlpha,
	bmCopyOnAddAlpha,
	bmPsNormal,
	bmPsAdditive,
	bmPsSubtractive,
	bmPsMultiplicative,
	bmPsScreen,
	bmPsOverlay,
	bmPsHardLight,
	bmPsSoftLight,
	bmPsColorDodge,
	bmPsColorDodge5,
	bmPsColorBurn,
	bmPsLighten,
	bmPsDarken,
	bmPsDifference,
	bmPsDifference5,
	bmPsExclusion
};

enum tTVPBBStretchType
{
	stNearest = 0, // primal method; nearest neighbor method
	stFastLinear = 1, // fast linear interpolation (does not have so much precision)
	stLinear = 2,  // (strict) linear interpolation
	stCubic = 3,    // cubic interpolation

	stTypeMask = 0xf, // stretch type mask
	stFlagMask = 0xf0, // flag mask

	stRefNoClip = 0x10 // referencing source is not limited by the given rectangle
						// (may allow to see the border pixel to interpolate)
};
/*]*/



//---------------------------------------------------------------------------
// FIXME: for including order problem
//---------------------------------------------------------------------------
#include "LayerBitmapImpl.h"
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// t2DAffineMatrix
//---------------------------------------------------------------------------
struct t2DAffineMatrix
{
	/* structure for subscribing following 2D affine transformation matrix */
	/*
	|                          | a  b  0 |
	| [x', y', 1] =  [x, y, 1] | c  d  0 |
	|                          | tx ty 1 |
	|  thus,
	|
	|  x' =  ax + cy + tx
	|  y' =  bx + dy + ty
	*/

	double a;
	double b;
	double c;
	double d;
	double tx;
	double ty;
};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
#define TVP_BB_COPY_MAIN 1
#define TVP_BB_COPY_MASK 2
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
extern tTVPGLGammaAdjustData TVPIntactGammaAdjustData;
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTVPBaseBitmap
//---------------------------------------------------------------------------
class tTVPNativeBaseBitmap;
class tTVPBaseBitmap : public tTVPNativeBaseBitmap
{
public:
	tTVPBaseBitmap(tjs_uint w, tjs_uint h, tjs_uint bpp = 32);
	tTVPBaseBitmap(const tTVPBaseBitmap & r) :
		tTVPNativeBaseBitmap(r) {}
	~tTVPBaseBitmap();

public:

	void operator =(const tTVPBaseBitmap & rhs) { Assign(rhs); }

	// metrics
	void SetSizeWithFill(tjs_uint w, tjs_uint h, tjs_uint32 fillvalue);

	// point access
	tjs_uint32 GetPoint(tjs_int x, tjs_int y) const;
	bool SetPoint(tjs_int x, tjs_int y, tjs_uint32 value);
	bool SetPointMain(tjs_int x, tjs_int y, tjs_uint32 color); // for 32bpp
	bool SetPointMask(tjs_int x, tjs_int y, tjs_int mask); // for 32bpp

	// drawing stuff
	bool Fill(tTVPRect rect, tjs_uint32 value);

	bool FillColor(tTVPRect rect, tjs_uint32 color, tjs_int opa);

private:
	bool BlendColor(tTVPRect rect, tjs_uint32 color, tjs_int opa, bool additive);

public:
	bool FillColorOnAlpha(tTVPRect rect, tjs_uint32 color, tjs_int opa)
	{
		return BlendColor(rect, color, opa, false);
	}

	bool FillColorOnAddAlpha(tTVPRect rect, tjs_uint32 color, tjs_int opa)
	{
		return BlendColor(rect, color, opa, true);
	}

	bool RemoveConstOpacity(tTVPRect rect, tjs_int level);

	bool FillMask(tTVPRect rect, tjs_int value);

	bool CopyRect(tjs_int x, tjs_int y, const tTVPBaseBitmap *ref,
		tTVPRect refrect, tjs_int plane = (TVP_BB_COPY_MAIN|TVP_BB_COPY_MASK));

	bool Blt(tjs_int x, tjs_int y, const tTVPBaseBitmap *ref,
		tTVPRect refrect, tTVPBBBltMethod method, tjs_int opa,
			bool hda = true);

private:
	template <typename tFunc>
	static void TVPDoStretchLoop(tFunc func,
		tjs_int x_ref_start,
		tjs_int y_ref_start,
		tjs_int x_len, tjs_int y_len,
		tjs_uint8 * destp, tjs_int destpitch,
		tjs_int x_step, tjs_int y_step,
		const tjs_uint8 * refp, tjs_int refpitch);

	template <typename tStretchFunc>
	static void TVPDoBiLinearStretchLoop(
			tStretchFunc stretch,
			tjs_int rw, tjs_int rh,
			tjs_int dw, tjs_int dh,
			const tTVPRect & srccliprect,
			tjs_int x_ref_start,
			tjs_int y_ref_start,
			tjs_int x_len, tjs_int y_len,
			tjs_uint8 * destp, tjs_int destpitch,
			tjs_int x_step, tjs_int y_step,
			const tjs_uint8 * refp, tjs_int refpitch);

public:
	bool StretchBlt(tTVPRect cliprect, tTVPRect destrect, const tTVPBaseBitmap *ref,
		tTVPRect refrect, tTVPBBBltMethod method, tjs_int opa,
			bool hda = true, tTVPBBStretchType type = stNearest);

private:
	template <typename tFuncStretch, typename tFuncAffine>
	static void TVPDoAffineLoop(
			tFuncStretch stretch,
			tFuncAffine affine,
			tjs_int sxs,
			tjs_int sys,
			tjs_uint8 *dest,
			tjs_int l,
			tjs_int len,
			const tjs_uint8 *src,
			tjs_int srcpitch,
			tjs_int sxl,
			tjs_int syl,
			const tTVPRect & srcrect);

	template <typename tFuncStretch, typename tFuncAffine>
	static void TVPDoBilinearAffineLoop(
			tFuncStretch stretch,
			tFuncAffine affine,
			tjs_int sxs,
			tjs_int sys,
			tjs_uint8 *dest,
			tjs_int l,
			tjs_int len,
			const tjs_uint8 *src,
			tjs_int srcpitch,
			tjs_int sxl,
			tjs_int syl,
			const tTVPRect & srccliprect,
			const tTVPRect & srcrect);

public:
	bool AffineBlt(tTVPRect destrect, const tTVPBaseBitmap *ref,
		tTVPRect refrect, const tTVPPointD * points,
			tTVPBBBltMethod method, tjs_int opa,
			tTVPRect * updaterect = NULL,
			bool hda = true, tTVPBBStretchType mode = stNearest, bool clear = false,
				tjs_uint32 clearcolor = 0);

	bool AffineBlt(tTVPRect destrect, const tTVPBaseBitmap *ref,
		tTVPRect refrect, const t2DAffineMatrix & matrix,
			tTVPBBBltMethod method, tjs_int opa,
			tTVPRect * updaterect = NULL,
			bool hda = true, tTVPBBStretchType mode = stNearest, bool clear = false,
				tjs_uint32 clearcolor = 0);

private:
	template <typename tARGB>
	void DoBoxBlurLoop(const tTVPRect &rect, const tTVPRect & area);

	bool InternalDoBoxBlur(tTVPRect rect, tTVPRect area, bool hasalpha);

public:
	bool DoBoxBlur(const tTVPRect & rect, const tTVPRect & area);
	bool DoBoxBlurForAlpha(const tTVPRect & rect, const tTVPRect & area);

	void UDFlip(const tTVPRect &rect);
	void LRFlip(const tTVPRect &rect);

	void DoGrayScale(tTVPRect rect);

	void AdjustGamma(tTVPRect rect, const tTVPGLGammaAdjustData & data);
	void AdjustGammaForAdditiveAlpha(tTVPRect rect, const tTVPGLGammaAdjustData & data);


	void ConvertAddAlphaToAlpha();
	void ConvertAlphaToAddAlpha();

	// font and text related functions are implemented in each platform.

public:
};
//---------------------------------------------------------------------------




#endif
