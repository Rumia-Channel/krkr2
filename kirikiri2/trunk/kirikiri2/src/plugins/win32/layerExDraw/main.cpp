#include "ncbind/ncbind.hpp"
#include "layerExDraw.hpp"

/**
 * ���O�o�͗p
 */
void
message_log(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	char msg[1024];
	_vsnprintf_s(msg, 1024, _TRUNCATE, format, args);
	TVPAddLog(ttstr(msg));
	va_end(args);
}

/**
 * �G���[���O�o�͗p
 */
void
error_log(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	char msg[1024];
	_vsnprintf_s(msg, 1024, _TRUNCATE, format, args);
	TVPAddImportantLog(ttstr(msg));
	va_end(args);
}

extern void initGdiPlus();
extern void deInitGdiPlus();
extern Image *loadImage(const tjs_char *name);
extern RectF *getBounds(Image *image);

// ----------------------------------------------------------------
// ���̌^�̓o�^
// ���l�p�����[�^�n�͔z�񂩎������g����悤�ȓ���R���o�[�^���\�z
// ----------------------------------------------------------------

// �������O�R���o�[�^
#define NCB_SET_CONVERTOR_BOTH(type, convertor)\
NCB_TYPECONV_SRCMAP_SET(type, convertor<type>, true);\
NCB_TYPECONV_DSTMAP_SET(type, convertor<type>, true)

// SRC�������O�R���o�[�^
#define NCB_SET_CONVERTOR_SRC(type, convertor)\
NCB_TYPECONV_SRCMAP_SET(type, convertor<type>, true);\
NCB_TYPECONV_DSTMAP_SET(type, ncbNativeObjectBoxing::Unboxing, true)

// DST�������O�R���o�[�^
#define NCB_SET_CONVERTOR_DST(type, convertor)\
NCB_TYPECONV_SRCMAP_SET(type, ncbNativeObjectBoxing::Boxing,   true); \
NCB_TYPECONV_DSTMAP_SET(type, convertor<type>, true)

/**
 * �z�񂩂ǂ����̔���
 * @param var VARIANT
 * @return �z��Ȃ� true
 */
bool IsArray(const tTJSVariant &var)
{
	if (var.Type() == tvtObject) {
		iTJSDispatch2 *obj = var.AsObjectNoAddRef();
		return obj->IsInstanceOf(0, NULL, NULL, L"Array", obj) == TJS_S_TRUE;
	}
	return false;
}

// �����o�ϐ����v���p�e�B�Ƃ��ēo�^
#define NCB_MEMBER_PROPERTY(name, type, membername) \
	struct AutoProp_ ## name { \
		static void ProxySet(Class *inst, type value) { inst->membername = value; } \
		static type ProxyGet(Class *inst) {      return inst->membername; } }; \
	NCB_PROPERTY_PROXY(name,AutoProp_ ## name::ProxyGet, AutoProp_ ## name::ProxySet)

// �|�C���^�����^�� getter ��ϊ��o�^
#define NCB_ARG_PROPERTY_RO(name, type, methodname) \
	struct AutoProp_ ## name { \
		static type ProxyGet(Class *inst) { type var; inst->methodname(&var); return var; } }; \
	Property(TJS_W(# name), &AutoProp_ ## name::ProxyGet, (int)0, Proxy)

// ------------------------------------------------------
// �^�R���o�[�^�o�^
// ------------------------------------------------------

NCB_TYPECONV_CAST_INTEGER(Status);
NCB_TYPECONV_CAST_INTEGER(MatrixOrder);
NCB_TYPECONV_CAST_INTEGER(ImageType);
NCB_TYPECONV_CAST_INTEGER(RotateFlipType);

// ------------------------------------------------------- PointF
template <class T>
struct PointFConvertor {
	typedef ncbInstanceAdaptor<T> AdaptorT;
	template <typename ANYT>
	void operator ()(ANYT &adst, const tTJSVariant &src) {
		if (src.Type() == tvtObject) {
			T *obj = AdaptorT::GetNativeInstance(src.AsObjectNoAddRef());
			if (obj) {
				dst = *obj;
			} else {
				ncbPropAccessor info(src);
				if (IsArray(src)) {
					dst = PointF((REAL)info.getRealValue(0),
								 (REAL)info.getRealValue(1));
				} else {
					dst = PointF((REAL)info.getRealValue(L"x"),
								 (REAL)info.getRealValue(L"y"));
				}
			}
		} else {
			dst = T();
		}
		adst = ncbTypeConvertor::ToTarget<ANYT>::Get(&dst);
	}
private:
	T dst;
};

NCB_SET_CONVERTOR_DST(PointF, PointFConvertor);
NCB_REGISTER_SUBCLASS_DELAY(PointF) {
	NCB_CONSTRUCTOR((REAL,REAL));
	NCB_MEMBER_PROPERTY(x, REAL, X);
	NCB_MEMBER_PROPERTY(y, REAL, Y);
	NCB_METHOD(Equals);
};

PointF getPoint(const tTJSVariant &var)
{
	PointFConvertor<PointF> conv;
	PointF ret;
	conv(ret, var);
	return ret;
}

// ------------------------------------------------------- RectF
template <class T>
struct RectFConvertor {
	typedef ncbInstanceAdaptor<T> AdaptorT;
	template <typename ANYT>
	void operator ()(ANYT &adst, const tTJSVariant &src) {
		if (src.Type() == tvtObject) {
			T *obj = AdaptorT::GetNativeInstance(src.AsObjectNoAddRef());
			if (obj) {
				dst = *obj;
			} else {
				ncbPropAccessor info(src);
				if (IsArray(src)) {
					dst = RectF((REAL)info.getRealValue(0),
								(REAL)info.getRealValue(1),
								(REAL)info.getRealValue(2),
								(REAL)info.getRealValue(3));
				} else {
					dst = RectF((REAL)info.getRealValue(L"x"),
								(REAL)info.getRealValue(L"y"),
								(REAL)info.getRealValue(L"width"),
								(REAL)info.getRealValue(L"height"));
				}
			}
		} else {
			dst = T();
		}
		adst = ncbTypeConvertor::ToTarget<ANYT>::Get(&dst);
	}
private:
	T dst;
};

NCB_SET_CONVERTOR_DST(RectF, RectFConvertor);
NCB_REGISTER_SUBCLASS_DELAY(RectF) {
	NCB_CONSTRUCTOR((REAL,REAL,REAL,REAL));
	NCB_MEMBER_PROPERTY(x, REAL, X);
	NCB_MEMBER_PROPERTY(y, REAL, Y);
	NCB_MEMBER_PROPERTY(width, REAL, Width);
	NCB_MEMBER_PROPERTY(height, REAL, Height);
	NCB_PROPERTY_RO(left, GetLeft);
	NCB_PROPERTY_RO(top, GetTop);
	NCB_PROPERTY_RO(right, GetRight);
	NCB_PROPERTY_RO(bottom, GetBottom);
	NCB_ARG_PROPERTY_RO(location, PointF, GetLocation);
	NCB_ARG_PROPERTY_RO(bounds, RectF, GetBounds);
	NCB_METHOD(Clone);
// XXX	NCB_METHOD_DETAIL(Contains, Class, BOOL, Class::Contains, (REAL,REAL));
//	NCB_METHOD_DETAIL(ContainsPoint, Class, BOOL, Class::Contains, (const PointF&) const);
//	NCB_METHOD_DETAIL(ContainsRect, Class, BOOL, Class::Contains, (const RectF&));
	NCB_METHOD(Equals);
	NCB_METHOD_DETAIL(Inflate, Class, void, Class::Inflate, (REAL,REAL));
	NCB_METHOD_DETAIL(InflatePoint, Class, void, Class::Inflate, (const PointF&));
//XXX	NCB_METHOD_DETAIL(Intersect, Class, BOOL, Class::Intersect, (const Rect&));
	NCB_METHOD(IntersectsWith);
	NCB_METHOD(IsEmptyArea);
	NCB_METHOD_DETAIL(Offset, Class, void, Class::Offset, (REAL,REAL));
//XXX	NCB_METHOD_DETAIL(OffsetPoint, Class, void, Class::Offset, (const Point&));
	NCB_METHOD(Union);
};

RectF getRect(const tTJSVariant &var)
{
	RectFConvertor<RectF> conv;
	RectF ret;
	conv(ret, var);
	return ret;
}

// --------------------------------------------------------------------
// GDI+�̃f�t�H���g�R���X�g���N�^/�R�s�[�R���X�g���N�^�������Ȃ��^�̓o�^
// --------------------------------------------------------------------

/**
 * GDI+�I�u�W�F�N�g�̃��b�s���O�p�e���v���[�g�N���X
 */
template <class T>
class GdipWrapper {
	typedef T GdipClassT;
	typedef GdipWrapper<GdipClassT> WrapperT;
protected:
	GdipClassT *obj;
public:
	// �f�t�H���g�R���X�g���N�^
	GdipWrapper() : obj(NULL) {
	}

	// �֐��̋A��l�Ƃ��ẴI�u�W�F�N�g�������p�B
	// ���̂܂ܓn���ꂽ�|�C���^���g��
	GdipWrapper(GdipClassT *obj) : obj(obj) {
	}

	// �R�s�[�R���X�g���N�^
	// �����I�u�W�F�N�g�� Clone����
	GdipWrapper(const GdipWrapper &orig) : obj(NULL) {
		if (orig.obj) {
			obj = orig.obj->Clone();
		}
	}
	
	// �f�X�g���N�^
	~GdipWrapper() {
		if (obj) {
			delete obj;
		}
	}
	
	GdipClassT *getGdipObject() { return obj; }

	void setGdipObject(GdipClassT *src) {
		if (obj) {
			delete obj;
		}
		obj = src;
	}

	struct BridgeFunctor {
		GdipClassT* operator()(WrapperT *p) const {
			return p->getGdipObject();
		}
	};

	template <class CastT>
	struct CastBridgeFunctor {
		CastT* operator()(WrapperT *p) const {
			return (CastT*)p->getGdipObject();
		}
	};

};

/**
 * GDI+�I�u�W�F�N�g�����b�s���O�����N���X�p�̃R���o�[�^�i�ėp�j
 */
template <class T>
struct GdipTypeConvertor {
	typedef typename ncbTypeConvertor::Stripper<T>::Type GdipClassT;
	typedef T *GdipClassP;
	typedef GdipWrapper<GdipClassT> WrapperT;
	typedef ncbInstanceAdaptor<WrapperT> AdaptorT;
protected:
	GdipClassT *result; // ���ʂ̈ꎞ�ێ��p
public:
	GdipTypeConvertor() : result(NULL) {}
	~GdipTypeConvertor() {delete result;}
	
	void operator ()(GdipClassP &dst, const tTJSVariant &src) {
		WrapperT *obj;
		if (src.Type() == tvtObject && (obj = AdaptorT::GetNativeInstance(src.AsObjectNoAddRef()))) {
			dst = obj->getGdipObject();
		} else {
			dst = NULL;
		}
	}

	void operator ()(tTJSVariant &dst, const GdipClassP &src) {
		if (src != NULL) {
			iTJSDispatch2 *adpobj = AdaptorT::CreateAdaptor(new WrapperT(src));
			if (adpobj) {
				dst = tTJSVariant(adpobj, adpobj);
				adpobj->Release();			
			} else {
				dst = NULL;
			}
		} else {
			dst.Clear();
		}
	}
};

// �R���o�[�^�o�^�p�o�^�p�}�N��

#define NCB_GDIP_CONVERTOR(type) \
NCB_SET_CONVERTOR(type*, GdipTypeConvertor<type>);\
NCB_SET_CONVERTOR(const type*, GdipTypeConvertor<const type>)

#define NCB_GDIP_CONVERTOR2(type, convertor) \
NCB_SET_CONVERTOR(type*, convertor<type>);\
NCB_SET_CONVERTOR(const type*, convertor<const type>)

// ���b�s���O�����p
#define NCB_REGISTER_GDIP_SUBCLASS(Class) NCB_GDIP_CONVERTOR(Class);NCB_REGISTER_SUBCLASS(GdipWrapper<Class>) { typedef Class GdipClass;
#define NCB_REGISTER_GDIP_SUBCLASS2(Class, Convertor) NCB_GDIP_CONVERTOR2(Class, Convertor);NCB_REGISTER_SUBCLASS(GdipWrapper<Class>) { typedef Class GdipClass;
#define NCB_GDIP_METHOD(name)  Method(TJS_W(# name), &GdipClass::name, Bridge<GdipWrapper<GdipClass>::BridgeFunctor>())
#define NCB_GDIP_MCAST(ret, method, args) static_cast<ret (GdipClass::*) args>(&GdipClass::method)
#define NCB_GDIP_METHOD2(name, ret, method, args) Method(TJS_W(# name), NCB_GDIP_MCAST(ret, method, args), Bridge<GdipWrapper<GdipClass>::BridgeFunctor>())
#define NCB_GDIP_PROPERTY(name,get,set)  Property(TJS_W(# name), &GdipClass::get, &GdipClass::set, Bridge<GdipWrapper<GdipClass>::BridgeFunctor>())
// XXX ���܂��������Ȃ�
#define NCB_GDIP_PROPERTY_RO(name,get)  Property(TJS_W(# name), &GdipClass::get, (int)0, Bridge<GdipWrapper<GdipClass>::BridgeFunctor>())
#define NCB_GDIP_MEMBER_PROPERTY(name, type, membername) \
	struct AutoProp_ ## name { \
		static void ProxySet(GdipClass *inst, type value) { inst->membername = value; } \
		static type ProxyGet(GdipClass *inst) {      return inst->membername; } }; \
	Property(TJS_W(#name), AutoProp_ ## name::ProxyGet, AutoProp_ ## name::ProxySet, Bridge<GdipWrapper<GdipClass>::BridgeFunctor>())


// ------------------------------------------------------- Matrix

template <class T>
struct MatrixConvertor : public GdipTypeConvertor<T> {
	void operator ()(GdipClassP &dst, const tTJSVariant &src) {
		WrapperT *obj;
		if (src.Type() == tvtObject) {
			if ((obj = AdaptorT::GetNativeInstance(src.AsObjectNoAddRef()))) {
				dst = obj->getGdipObject();
			} else {
				ncbPropAccessor info(src);
				if (IsArray(src)) {
					result = new Matrix((REAL)info.getRealValue(0),
										(REAL)info.getRealValue(1),
										(REAL)info.getRealValue(2),
										(REAL)info.getRealValue(3),
										(REAL)info.getRealValue(4),
										(REAL)info.getRealValue(5));
				} else {
					result = new Matrix((REAL)info.getRealValue(L"m11"),
										(REAL)info.getRealValue(L"m12"),
										(REAL)info.getRealValue(L"m21"),
										(REAL)info.getRealValue(L"m22"),
										(REAL)info.getRealValue(L"dx"),
										(REAL)info.getRealValue(L"dy"));
				}
				dst = result;
			}
		} else {
			dst = NULL;
		}
	}
};

NCB_REGISTER_GDIP_SUBCLASS2(Matrix, MatrixConvertor)
NCB_CONSTRUCTOR(());
NCB_GDIP_METHOD(OffsetX);
NCB_GDIP_METHOD(OffsetY);
NCB_GDIP_METHOD(Equals);
// NCB_GDIP_METHOD(getElements); // �z���Ԃ�
NCB_GDIP_METHOD(SetElements);
NCB_GDIP_METHOD(GetLastStatus);
NCB_GDIP_METHOD(Invert);
NCB_GDIP_METHOD(IsIdentity);
NCB_GDIP_METHOD(IsInvertible);
NCB_GDIP_METHOD(Multiply);
NCB_GDIP_METHOD(Reset);
NCB_GDIP_METHOD(Rotate);
NCB_GDIP_METHOD(RotateAt);
NCB_GDIP_METHOD(Scale);
NCB_GDIP_METHOD(Shear);
//	NCB_GDIP_METHOD_DETAIL(TransformPoints, Class, Status, TransformPoints, (PointF*, INT)); XXX �������z��
//	NCB_GDIP_METHOD_DETAIL(TransformVectors, Class, Status, TransformVectors, (PointF*, INT)); XXX �������z��
NCB_GDIP_METHOD(Translate);
};

// ------------------------------------------------------- Image

/**
 * �C���[�W�p�R���o�[�^
 * �����񂩂���ύX�\
 */
template <class T>
struct ImageConvertor : public GdipTypeConvertor<T> {
	void operator ()(GdipClassP &dst, const tTJSVariant &src) {
		if (src.Type() == tvtObject) {
			WrapperT *obj;
			if ((obj = AdaptorT::GetNativeInstance(src.AsObjectNoAddRef()))) {
				dst = obj->getGdipObject();
			} else {
				LayerExDraw *layer = ncbInstanceAdaptor<LayerExDraw>::GetNativeInstance(src.AsObjectNoAddRef());
				if (layer) {
					dst = layer->getImage();
				} else {
					dst = NULL;
				}
			}
		} else if (src.Type() == tvtString) { // �����񂩂琶��
			dst = result = loadImage(src.GetString());
		} else {
			dst = NULL;
		}
	}
};

static void ImageLoad(GdipWrapper<Image> *obj, const tjs_char *filename)
{
	obj->setGdipObject(loadImage(filename));
}

static tTJSVariant ImageClone(GdipWrapper<Image> *obj)
{
	typedef GdipWrapper<Image> WrapperT;
	typedef ncbInstanceAdaptor<WrapperT> AdaptorT;
	tTJSVariant ret;
	Image *src = obj->getGdipObject();
	if (src) {
		Image *newimage = src->Clone();
		iTJSDispatch2 *adpobj = AdaptorT::CreateAdaptor(new WrapperT(newimage));
		if (adpobj) {
			ret = tTJSVariant(adpobj, adpobj);
			adpobj->Release();			
		} else {
			delete newimage;
		}
	}
	return ret;
}

static tTJSVariant ImageBounds(GdipWrapper<Image> *obj)
{
	typedef ncbInstanceAdaptor<RectF> AdaptorT;
	tTJSVariant ret;
	Image *src = obj->getGdipObject();
	if (src) {
		RectF *bounds = getBounds(src);
		iTJSDispatch2 *adpobj = AdaptorT::CreateAdaptor(bounds);
		if (adpobj) {
			ret = tTJSVariant(adpobj, adpobj);
			adpobj->Release();
		} else {
			delete bounds;
		}
	}
	return ret;
}

NCB_REGISTER_GDIP_SUBCLASS2(Image, ImageConvertor)
NCB_CONSTRUCTOR(());
NCB_METHOD_PROXY(load, ImageLoad);
NCB_METHOD_PROXY(Clone, ImageClone);
NCB_METHOD_PROXY(GetBounds, ImageBounds);
//GetAllPropertyItems
//NCB_GDIP_METHOD(GetBounds);
//GetEncoderParameterList
//GetEncoderParameterListSize
NCB_GDIP_METHOD(GetFlags);
//NCB_GDIP_METHOD(GetFrameCount);
//NCB_GDIP_METHOD(GetFrameDimensionCount);
//NCB_GDIP_METHOD(GetFrameDimensionList);
NCB_GDIP_METHOD(GetHeight);
NCB_GDIP_METHOD(GetHorizontalResolution);
NCB_GDIP_METHOD(GetLastStatus);
//NCB_GDIP_PROPERTY(palette, GetPalette, SetPalette);
//NCB_GDIP_METHOD(GetPaletteSize);
//GetPhysicalDimension
NCB_GDIP_METHOD(GetPixelFormat);
//NCB_GDIP_METHOD(GetPropertyCount);
//GetPropertyItemList
//GetPropertyItem
//SetPropertyItem
//GetPropertyItemSize
//GetPropertySize
//GetRawFormat
//GetThumbnailImage
NCB_GDIP_METHOD(GetType);
NCB_GDIP_METHOD(GetVerticalResolution);
NCB_GDIP_METHOD(GetWidth);
//RemovePropertyItem
NCB_GDIP_METHOD(RotateFlip);
//SelectActiveFrame
};

// ------------------------------------------------------
// ���O�L�q�N���X�o�^
// ------------------------------------------------------

NCB_REGISTER_SUBCLASS(FontInfo) {
	NCB_CONSTRUCTOR((const tjs_char *, REAL, INT));
	NCB_PROPERTY_WO(familyName, setFamilyName);
	NCB_PROPERTY(emSize, getEmSize, setEmSize);
	NCB_PROPERTY(style, getStyle, setStyle);
	NCB_PROPERTY_RO(ascent, getAscent);
	NCB_PROPERTY_RO(descent, getDescent);
	NCB_PROPERTY_RO(lineSpacing, getLineSpacing);
};

NCB_REGISTER_SUBCLASS(Appearance) {
	NCB_CONSTRUCTOR(());
	NCB_METHOD(clear);
	NCB_METHOD(addBrush);
	NCB_METHOD(addPen);
};

#define ENUM(n) Variant(#n, (int)n)
#define NCB_SUBCLASS_NAME(name) NCB_SUBCLASS(name, name)
#define NCB_GDIP_SUBCLASS(Class) NCB_SUBCLASS(Class, GdipWrapper<Class>)

NCB_REGISTER_CLASS(GdiPlus)
{
	// enums

	// Status
	ENUM(Ok);
	ENUM(GenericError);
	ENUM(InvalidParameter);
	ENUM(OutOfMemory);
	ENUM(ObjectBusy);
	ENUM(InsufficientBuffer);
	ENUM(NotImplemented);
	ENUM(Win32Error);
	ENUM(WrongState);
	ENUM(Aborted);
	ENUM(FileNotFound);
	ENUM(ValueOverflow);
	ENUM(AccessDenied);
	ENUM(UnknownImageFormat);
	ENUM(FontFamilyNotFound);
	ENUM(FontStyleNotFound);
	ENUM(NotTrueTypeFont);
	ENUM(UnsupportedGdiplusVersion);
	ENUM(GdiplusNotInitialized);
	ENUM(PropertyNotFound);
	ENUM(PropertyNotSupported);
//	ENUM(ProfileNotFound);

	ENUM(FontStyleRegular);
	ENUM(FontStyleBold);
	ENUM(FontStyleItalic);
	ENUM(FontStyleBoldItalic);
	ENUM(FontStyleUnderline);
	ENUM(FontStyleStrikeout);

	ENUM(BrushTypeSolidColor);
	ENUM(BrushTypeHatchFill);
	ENUM(BrushTypeTextureFill);
	ENUM(BrushTypePathGradient);
	ENUM(BrushTypeLinearGradient);

	ENUM(DashCapFlat);
	ENUM(DashCapRound);
	ENUM(DashCapTriangle);
	
	ENUM(DashStyleSolid);
	ENUM(DashStyleDash);
	ENUM(DashStyleDot);
	ENUM(DashStyleDashDot);
	ENUM(DashStyleDashDotDot);

	ENUM(HatchStyleHorizontal);
	ENUM(HatchStyleVertical);
	ENUM(HatchStyleForwardDiagonal);
	ENUM(HatchStyleBackwardDiagonal);
	ENUM(HatchStyleCross);
	ENUM(HatchStyleDiagonalCross);
	ENUM(HatchStyle05Percent);
	ENUM(HatchStyle10Percent);
	ENUM(HatchStyle20Percent);
	ENUM(HatchStyle25Percent);
	ENUM(HatchStyle30Percent);
	ENUM(HatchStyle40Percent);
	ENUM(HatchStyle50Percent);
	ENUM(HatchStyle60Percent);
	ENUM(HatchStyle70Percent);
	ENUM(HatchStyle75Percent);
	ENUM(HatchStyle80Percent);
	ENUM(HatchStyle90Percent);
	ENUM(HatchStyleLightDownwardDiagonal);
	ENUM(HatchStyleLightUpwardDiagonal);
	ENUM(HatchStyleDarkDownwardDiagonal);
	ENUM(HatchStyleDarkUpwardDiagonal);
	ENUM(HatchStyleWideDownwardDiagonal);
	ENUM(HatchStyleWideUpwardDiagonal);
	ENUM(HatchStyleLightVertical);
	ENUM(HatchStyleLightHorizontal);
	ENUM(HatchStyleNarrowVertical);
	ENUM(HatchStyleNarrowHorizontal);
	ENUM(HatchStyleDarkVertical);
	ENUM(HatchStyleDarkHorizontal);
	ENUM(HatchStyleDashedDownwardDiagonal);
	ENUM(HatchStyleDashedUpwardDiagonal);
	ENUM(HatchStyleDashedHorizontal);
	ENUM(HatchStyleDashedVertical);
	ENUM(HatchStyleSmallConfetti);
	ENUM(HatchStyleLargeConfetti);
	ENUM(HatchStyleZigZag);
	ENUM(HatchStyleWave);
	ENUM(HatchStyleDiagonalBrick);
	ENUM(HatchStyleHorizontalBrick);
	ENUM(HatchStyleWeave);
	ENUM(HatchStylePlaid);
	ENUM(HatchStyleDivot);
	ENUM(HatchStyleDottedGrid);
	ENUM(HatchStyleDottedDiamond);
	ENUM(HatchStyleShingle);
	ENUM(HatchStyleTrellis);
	ENUM(HatchStyleSphere);
	ENUM(HatchStyleSmallGrid);
	ENUM(HatchStyleSmallCheckerBoard);
	ENUM(HatchStyleLargeCheckerBoard);
	ENUM(HatchStyleOutlinedDiamond);
	ENUM(HatchStyleSolidDiamond);
	ENUM(HatchStyleTotal);
	ENUM(HatchStyleLargeGrid);
	ENUM(HatchStyleMin);
	ENUM(HatchStyleMax);

	ENUM(LinearGradientModeHorizontal);
	ENUM(LinearGradientModeVertical);
	ENUM(LinearGradientModeForwardDiagonal);
	ENUM(LinearGradientModeBackwardDiagonal);

	ENUM(LineCapFlat);
	ENUM(LineCapSquare);
	ENUM(LineCapRound);
	ENUM(LineCapTriangle);
	ENUM(LineCapNoAnchor);
	ENUM(LineCapSquareAnchor);
	ENUM(LineCapRoundAnchor);
	ENUM(LineCapDiamondAnchor);
	ENUM(LineCapArrowAnchor);

	ENUM(LineJoinMiter);
	ENUM(LineJoinBevel);
	ENUM(LineJoinRound);
	ENUM(LineJoinMiterClipped);
	
	ENUM(PenAlignmentCenter);
	ENUM(PenAlignmentInset);

	ENUM(WrapModeTile);
	ENUM(WrapModeTileFlipX);
	ENUM(WrapModeTileFlipY);
	ENUM(WrapModeTileFlipXY);
	ENUM(WrapModeClamp);

	ENUM(MatrixOrderPrepend);
	ENUM(MatrixOrderAppend);

	ENUM(ImageTypeUnknown);
    ENUM(ImageTypeBitmap);
	ENUM(ImageTypeMetafile);

	ENUM(RotateNoneFlipNone);
	ENUM(Rotate90FlipNone);
	ENUM(Rotate180FlipNone);
	ENUM(Rotate270FlipNone);
	ENUM(RotateNoneFlipX);
	ENUM(Rotate90FlipX);
	ENUM(Rotate180FlipX);
	ENUM(Rotate270FlipX);
	ENUM(RotateNoneFlipY);
	ENUM(Rotate90FlipY);
	ENUM(Rotate180FlipY);
	ENUM(Rotate270FlipY);
	ENUM(RotateNoneFlipXY);
	ENUM(Rotate90FlipXY);
	ENUM(Rotate180FlipXY);
	ENUM(Rotate270FlipXY);
	
// statics
	NCB_METHOD(addPrivateFont);
	NCB_METHOD(getFontList);

// classes
	NCB_SUBCLASS_NAME(PointF);
	NCB_SUBCLASS_NAME(RectF);

	NCB_GDIP_SUBCLASS(Image);
	NCB_GDIP_SUBCLASS(Matrix);
	
	NCB_SUBCLASS(Font,FontInfo);
	NCB_SUBCLASS(Appearance,Appearance);
}

NCB_GET_INSTANCE_HOOK(LayerExDraw)
{
	// �C���X�^���X�Q�b�^
	NCB_INSTANCE_GETTER(objthis) { // objthis �� iTJSDispatch2* �^�̈����Ƃ���
		ClassT* obj = GetNativeInstance(objthis);	// �l�C�e�B�u�C���X�^���X�|�C���^�擾
		if (!obj) {
			obj = new ClassT(objthis);				// �Ȃ��ꍇ�͐�������
			SetNativeInstance(objthis, obj);		// objthis �� obj ���l�C�e�B�u�C���X�^���X�Ƃ��ēo�^����
		}
		obj->reset();
		return obj;
	}
	// �f�X�g���N�^�i���ۂ̃��\�b�h���Ă΂ꂽ��ɌĂ΂��j
	~NCB_GET_INSTANCE_HOOK_CLASS () {
	}
};


// �t�b�N���A�^�b�`
NCB_ATTACH_CLASS_WITH_HOOK(LayerExDraw, Layer) {
	NCB_PROPERTY(updateWhenDraw, getUpdateWhenDraw, setUpdateWhenDraw);
	NCB_PROPERTY_WO(transform, setTransform);

	NCB_METHOD(setResolution);

	NCB_METHOD(clear);
	NCB_METHOD(drawArc);
	NCB_METHOD(drawPie);
	NCB_METHOD(drawBezier);
	NCB_METHOD(drawBeziers);
	NCB_METHOD(drawClosedCurve);
	NCB_METHOD(drawClosedCurve2);
	NCB_METHOD(drawCurve);
	NCB_METHOD(drawCurve2);
	NCB_METHOD(drawCurve3);
	NCB_METHOD(drawEllipse);
	NCB_METHOD(drawLine);
	NCB_METHOD(drawLines);
	NCB_METHOD(drawPolygon);
	NCB_METHOD(drawRectangle);
	NCB_METHOD(drawRectangles);
	NCB_METHOD(drawString);
	NCB_METHOD(measureString);

	NCB_METHOD(drawImage);
	NCB_METHOD(drawImageRect);
	NCB_METHOD(drawImageStretch);
	NCB_METHOD(drawImageAffine);

	NCB_PROPERTY(record, getRecord, setRecord);
	NCB_METHOD(redrawRecord);
	NCB_METHOD(saveRecord);
	NCB_METHOD(loadRecord);
}

// ----------------------------------- �N���E�J������

NCB_PRE_REGIST_CALLBACK(initGdiPlus);
NCB_POST_UNREGIST_CALLBACK(deInitGdiPlus);