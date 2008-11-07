#ifndef _layerExText_hpp_
#define _layerExText_hpp_

#include <windows.h>
#include <gdiplus.h>
using namespace Gdiplus;

#include <vector>
using namespace std;

#include "layerExBase.hpp"

/**
 * GDIPlus �ŗL�����p
 */
struct GdiPlus {
	/**
	 * �v���C�x�[�g�t�H���g�̒ǉ�
	 * @param fontFileName �t�H���g�t�@�C����
	 */
	static void addPrivateFont(const tjs_char *fontFileName);

	/**
	 * �t�H���g�t�@�~���[�����擾
	 * @param privateOnly true �Ȃ�v���C�x�[�g�t�H���g�̂ݎ擾
	 */
	static tTJSVariant getFontList(bool privateOnly);
};

/**
 * �t�H���g���
 */
class FontInfo {
	friend class LayerExDraw;

protected:
	FontFamily *fontFamily; //< �t�H���g�t�@�~���[
	ttstr familyName;
	REAL emSize; //< �t�H���g�T�C�Y 
	INT style; //< �t�H���g�X�^�C��

	/**
	 * �t�H���g���̃N���A
	 */
	void clear();

public:

	FontInfo();
	/**
	 * �R���X�g���N�^
	 * @param familyName �t�H���g�t�@�~���[
	 * @param emSize �t�H���g�̃T�C�Y
	 * @param style �t�H���g�X�^�C��
	 */
	FontInfo(const tjs_char *familyName, REAL emSize, INT style);
	FontInfo(const FontInfo &orig);

	/**
	 * �f�X�g���N�^
	 */
	virtual ~FontInfo();

	void setFamilyName(const tjs_char *familyName);
	const tjs_char *getFamilyName() { return familyName.c_str(); }
	void setEmSize(REAL emSize) { this->emSize = emSize; }
	REAL getEmSize() {  return emSize; }
	void setStyle(INT style) { this->style = style; }
	INT getStyle() { return style; }

	REAL getAscent() {
		if (fontFamily) {
			return fontFamily->GetCellAscent(style) * emSize / fontFamily->GetEmHeight(style);
		} else {
			return 0;
		}
	}

	REAL getDescent() {
		if (fontFamily) {
			return fontFamily->GetCellDescent(style) * emSize / fontFamily->GetEmHeight(style);
		} else {
			return 0;
		}
	}

	REAL getLineSpacing() {
		if (fontFamily) {
			return fontFamily->GetLineSpacing(style) * emSize / fontFamily->GetEmHeight(style);
		} else {
			return 0;
		}
	}
};

/**
 * �`��O�Ϗ��
 */
class Appearance {
	friend class LayerExDraw;
public:
	// �`����
	struct DrawInfo{
		int type;   // 0:�u���V 1:�y��
		void *info; // ���I�u�W�F�N�g
		REAL ox; //< �\���I�t�Z�b�g
		REAL oy; //< �\���I�t�Z�b�g
		DrawInfo() : ox(0), oy(0), type(0), info(NULL) {}
		DrawInfo(REAL ox, REAL oy, Pen *pen) : ox(ox), oy(oy), type(0), info(pen) {}
		DrawInfo(REAL ox, REAL oy, Brush *brush) : ox(ox), oy(oy), type(1), info(brush) {}
		DrawInfo(const DrawInfo &orig) {
			ox = orig.ox;
			oy = orig.oy;
			type = orig.type;
			if (orig.info) {
				switch (type) {
				case 0:
					info = (void*)((Pen*)orig.info)->Clone();
					break;
				case 1:
					info = (void*)((Brush*)orig.info)->Clone();
					break;
				}
			} else {
				info = NULL;
			}
		}
		virtual ~DrawInfo() {
			if (info) {
				switch (type) {
				case 0:
					delete (Pen*)info;
					break;
				case 1:
					delete (Brush*)info;
					break;
				}
			}
		}	
	};
	vector<DrawInfo>drawInfos;

public:
	Appearance();
	virtual ~Appearance();

	/**
	 * ���̃N���A
	 */
	void clear();
	
	/**
	 * �u���V�̒ǉ�
	 * @param colorOrBrush ARGB�F�w��܂��̓u���V���i�����j
	 * @param ox �\���I�t�Z�b�gX
	 * @param oy �\���I�t�Z�b�gY
	 */
	void addBrush(tTJSVariant colorOrBrush, REAL ox=0, REAL oy=0);
	
	/**
	 * �y���̒ǉ�
	 * @param colorOrBrush ARGB�F�w��܂��̓u���V���i�����j
	 * @param widthOrOption �y�����܂��̓y�����i�����j
	 * @param ox �\���I�t�Z�b�gX
	 * @param oy �\���I�t�Z�b�gY
	 */
	void addPen(tTJSVariant colorOrBrush, tTJSVariant widthOrOption, REAL ox=0, REAL oy=0);
};

/*
 * �A�E�g���C���x�[�X�̃e�L�X�g�`�惁�\�b�h�̒ǉ�
 */
class LayerExDraw : public layerExBase
{
protected:
	// ���ێ��p
	GeometryT width, height;
	BufferT   buffer;
	PitchT    pitch;
	
	/// ���C�����Q�Ƃ���r�b�g�}�b�v
	Bitmap *bitmap;
	/// ���C���ɑ΂��ĕ`�悷��R���e�L�X�g
	Graphics *graphics;

	bool updateWhenDraw;
	void updateRect(RectF &rect);
	
public:
	void setUpdateWhenDraw(int updateWhenDraw) {
		this->updateWhenDraw = updateWhenDraw != 0;
	}
	int getUpdateWhenDraw() { return updateWhenDraw ? 1 : 0; }

	Image *getImage() {
		return (Image*)bitmap;
	}
	
public:	
	LayerExDraw(DispatchT obj);
	~LayerExDraw();
	virtual void reset();

	// ------------------------------------------------------------------
	// �`�惁�\�b�h�Q
	// ------------------------------------------------------------------

protected:

	/**
	 * �p�X�̍X�V�̈�����擾
	 * @param app �\���\��
	 * @param path �`�悷��p�X
	 * @return �X�V�̈���
	 */
	RectF getPathExtents(const Appearance *app, const GraphicsPath *path);

	/**
	 * �p�X�̕`��
	 * @param app �A�s�A�����X
	 * @param path �`�悷��p�X
	 * @return �X�V�̈���
	 */
	RectF drawPath(const Appearance *app, const GraphicsPath *path);


public:
	/**
	 * ��ʂ̏���
	 * @param argb �����F
	 */
	void clear(ARGB argb);

	/**
	 * �~�ʂ̕`��
	 * @param app �A�s�A�����X
	 * @param x ������W
	 * @param y ������W
	 * @param width ����
	 * @param height �c��
	 * @param startAngle ���v�����~�ʊJ�n�ʒu
	 * @param sweepAngle �`��p�x
	 * @return �X�V�̈���
	 */
	RectF drawArc(const Appearance *app, REAL x, REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle);

	/**
	 * �~���̕`��
	 * @param app �A�s�A�����X
	 * @param x ������W
	 * @param y ������W
	 * @param width ����
	 * @param height �c��
	 * @param startAngle ���v�����~�ʊJ�n�ʒu
	 * @param sweepAngle �`��p�x
	 * @return �X�V�̈���
	 */
	RectF drawPie(const Appearance *app, REAL x, REAL y, REAL width, REAL height, REAL startAngle, REAL sweepAngle);
	
	/**
	 * �x�W�F�Ȑ��̕`��
	 * @param app �A�s�A�����X
	 * @param x1
	 * @param y1
	 * @param x2
	 * @param y2
	 * @param x3
	 * @param y3
	 * @param x4
	 * @param y4
	 * @return �X�V�̈���
	 */
	RectF drawBezier(const Appearance *app, REAL x1, REAL y1, REAL x2, REAL y2, REAL x3, REAL y3, REAL x4, REAL y4);

	/**
	 * �A���x�W�F�Ȑ��̕`��
	 * @param app �A�s�A�����X
	 * @param points �_�̔z��
	 * @return �X�V�̈���
	 */
	RectF drawBeziers(const Appearance *app, tTJSVariant points);

	/**
	 * Closed cardinal spline �̕`��
	 * @param app �A�s�A�����X
	 * @param points �_�̔z��
	 * @return �X�V�̈���
	 */
	RectF drawClosedCurve(const Appearance *app, tTJSVariant points);

	/**
	 * Closed cardinal spline �̕`��
	 * @param app �A�s�A�����X
	 * @param points �_�̔z��
	 * @pram tension tension
	 * @return �X�V�̈���
	 */
	RectF drawClosedCurve2(const Appearance *app, tTJSVariant points, REAL tension);

	/**
	 * cardinal spline �̕`��
	 * @param app �A�s�A�����X
	 * @param points �_�̔z��
	 * @return �X�V�̈���
	 */
	RectF drawCurve(const Appearance *app, tTJSVariant points);

	/**
	 * cardinal spline �̕`��
	 * @param app �A�s�A�����X
	 * @param points �_�̔z��
	 * @parma tension tension
	 * @return �X�V�̈���
	 */
	RectF drawCurve2(const Appearance *app, tTJSVariant points, REAL tension);

	/**
	 * cardinal spline �̕`��
	 * @param app �A�s�A�����X
	 * @param points �_�̔z��
	 * @param offset
	 * @param numberOfSegment
	 * @param tension tension
	 * @return �X�V�̈���
	 */
	RectF drawCurve3(const Appearance *app, tTJSVariant points, int offset, int numberOfSegments, REAL tension);
	
	/**
	 * �ȉ~�̕`��
	 * @param app �A�s�A�����X
	 * @param x
	 * @param y
	 * @param width
	 * @param height
	 * @return �X�V�̈���
	 */
	RectF drawEllipse(const Appearance *app, REAL x, REAL y, REAL width, REAL height);

	/**
	 * �����̕`��
	 * @param app �A�s�A�����X
	 * @param x1 �n�_X���W
	 * @param y1 �n�_Y���W
	 * @param x2 �I�_X���W
	 * @param y2 �I�_Y���W
	 * @return �X�V�̈���
	 */
	RectF drawLine(const Appearance *app, REAL x1, REAL y1, REAL x2, REAL y2);

	/**
	 * �A�������̕`��
	 * @param app �A�s�A�����X
	 * @param points �_�̔z��
	 * @return �X�V�̈���
	 */
	RectF drawLines(const Appearance *app, tTJSVariant points);

	/**
	 * ���p�`�̕`��
	 * @param app �A�s�A�����X
	 * @param points �_�̔z��
	 * @return �X�V�̈���
	 */
	RectF drawPolygon(const Appearance *app, tTJSVariant points);
	
	/**
	 * ��`�̕`��
	 * @param app �A�s�A�����X
	 * @param x
	 * @param y
	 * @param width
	 * @param height
	 * @return �X�V�̈���
	 */
	RectF drawRectangle(const Appearance *app, REAL x, REAL y, REAL width, REAL height);

	/**
	 * ������`�̕`��
	 * @param app �A�s�A�����X
	 * @param rects ��`���̔z��
	 * @return �X�V�̈���
	 */
	RectF drawRectangles(const Appearance *app, tTJSVariant rects);
	
	/**
	 * ������̕`��
	 * @param font �t�H���g
	 * @param app �A�s�A�����X
	 * @param x �`��ʒuX
	 * @param y �`��ʒuY
	 * @param text �`��e�L�X�g
	 * @return �X�V�̈���
	 */
	RectF drawString(const FontInfo *font, const Appearance *app, REAL x, REAL y, const tjs_char *text);

	/**
	 * ������̕`��X�V�̈���̎擾
	 * @param font �t�H���g
	 * @param text �`��e�L�X�g
	 * @return �X�V�̈���̎��� left, top, width, height
	 */
	RectF measureString(const FontInfo *font, const tjs_char *text);

	// -----------------------------------------------------------------------------
	
	/**
	 * �摜�̕`��B�R�s�[��͌��摜�� Bounds ��z�������ʒu�A�T�C�Y�� Pixel �w��ɂȂ�܂��B
	 * @param x �R�s�[�挴�_X
	 * @param y �R�s�[�挴�_Y
	 * @param image �R�s�[���摜
	 * @return �X�V�̈���
	 */
	RectF drawImage(REAL x, REAL y, Image *src);

	/**
	 * �摜�̋�`�R�s�[
	 * @param dleft �R�s�[�捶�[
	 * @param dtop  �R�s�[���[
	 * @param src �R�s�[���摜
	 * @param sleft ����`�̍��[
	 * @param stop  ����`�̏�[
	 * @param swidth ����`�̉���
	 * @param sheight  ����`�̏c��
	 * @return �X�V�̈���
	 */
	RectF drawImageRect(REAL dleft, REAL dtop, Image *src, REAL sleft, REAL stop, REAL swidth, REAL sheight);

	/**
	 * �摜�̊g��k���R�s�[
	 * @param dleft �R�s�[�捶�[
	 * @param dtop  �R�s�[���[
	 * @param dwidth �R�s�[��̉���
	 * @param dheight  �R�s�[��̏c��
	 * @param src �R�s�[���摜
	 * @param sleft ����`�̍��[
	 * @param stop  ����`�̏�[
	 * @param swidth ����`�̉���
	 * @param sheight  ����`�̏c��
	 * @return �X�V�̈���
	 */
	RectF drawImageStretch(REAL dleft, REAL dtop, REAL dwidth, REAL dheight, Image *src, REAL sleft, REAL stop, REAL swidth, REAL sheight);

	/**
	 * �摜�̃A�t�B���ϊ��R�s�[
	 * @param src �R�s�[���摜
	 * @param sleft ����`�̍��[
	 * @param stop  ����`�̏�[
	 * @param swidth ����`�̉���
	 * @param sheight  ����`�̏c��
	 * @param affine �A�t�B���p�����[�^�̎��(true:�ϊ��s��, false:���W�w��), 
	 * @return �X�V�̈���
	 */
	RectF drawImageAffine(Image *src, REAL sleft, REAL stop, REAL swidth, REAL sheight, bool affine, REAL A, REAL B, REAL C, REAL D, REAL E, REAL F);
};

#endif