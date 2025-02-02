/**
 * 一般的な sqobject 実装
 * 
 * Object, Thread の登録処理の実装例です。
 * 継承情報は単純リスト管理してます
 */
#include <new>
#include "sqobjectinfo.h"
#include "sqobject.h"
#include "sqthread.h"

#include <string.h>
#include <sqstdstring.h>
#include <sqstdmath.h>
#include <sqstdaux.h>

SQRESULT ERROR_CREATE(HSQUIRRELVM v) {
	return sq_throwerror(v, _SC("can't create native instance"));
}

SQRESULT ERROR_BADINSTANCE(HSQUIRRELVM v) {
	return sq_throwerror(v, _SC("bad instance"));
}

SQRESULT ERROR_BADMETHOD(HSQUIRRELVM v) {
	return sq_throwerror(v, _SC("bad method"));
}

// 基底クラスダミー用
struct BaseClass {};
inline const SQChar *GetTypeName(const BaseClass *t) { return NULL; }

// クラス名登録用
#define DECLARE_CLASSNAME0(TYPE,NAME) \
inline const SQChar * GetTypeName(const TYPE *t) { return _SC(#NAME); }
#define DECLARE_CLASSNAME(TYPE,NAME) DECLARE_CLASSNAME0(TYPE,NAME)
#define DECLARE_CLASS(TYPE) DECLARE_CLASSNAME(TYPE,TYPE)

sqobject::ObjectInfo typeTagListMap;
sqobject::ObjectInfo typeMap;

/**
 * 継承関係を登録する
 * @param typeName 型名
 * @param parentName 親の型名
 */
void
registerInherit(const SQChar *typeName, const SQChar *parentName)
{
	typeMap.create(typeName, parentName);
}

/**
 * 親の型名を返す
 * @param typeName 型名
 * @return 親の型名
 */
const SQChar *
getParentName(const SQChar *typeName)
{
	const SQChar *parent = NULL;
	typeMap.get(typeName, &parent);
	return parent;
}

/**
 * オブジェクトのタグを登録する。親オブジェクトにも再帰的に登録する。
 * @param typeName 型名
 * @parma tag タグ
 */
void
registerTypeTag(const SQChar *typeName, SQUserPointer tag)
{
	// タグを登録
	sqobject::ObjectInfo list = typeTagListMap.get(typeName);
	if (!list.isArray()) {
		list.initArray();
		typeTagListMap.create(typeName, list);
	}
	list.append(tag);

	// 親クラスにも登録
	const SQChar *pname = getParentName(typeName);
	if (pname) {
		registerTypeTag(pname, tag);
	}
}

/**
 * 該当オブジェクトのネイティブインスタンスを取得。登録されてるタグリストを使う
 * @param v squirrelVM
 * @param idx スタックインデックス
 * @param typeName 型名
 * @return 指定された型のネイティブインスタンス。みつからない場合は NULL
 */
SQUserPointer
getInstance(HSQUIRRELVM v, SQInteger idx, const SQChar *typeName)
{
	sqobject::ObjectInfo list = typeTagListMap.get(typeName);
	if (list.isArray()) {
		SQInteger max = list.len();
		for (SQInteger i=0;i<max;i++) {
			SQUserPointer tag;
			list.get(i, &tag);
			SQUserPointer up;
			if (SQ_SUCCEEDED(sq_getinstanceup(v, idx, &up, tag))) {
				return up;
			}
		}
	}
	return NULL;
}

/**
 * クラス登録用テンプレート
 * @param T 登録対象型
 * @param P 親の型
 */
template <typename T, typename P>
class SQTemplate {

private:
	HSQUIRRELVM v;
	
public:
	/**
	 * クラスを定義する
	 * @param v squirrelVM
	 * @param typetag 型識別タグ
	 */
	SQTemplate(HSQUIRRELVM v, SQUserPointer typetag) : v(v) {

		// 型名を DECLARE_CLASSNAME の登録をつかって参照
		T* typeDummy = NULL;
		P* parentDummy = NULL;
		const SQChar *typeName = GetTypeName(typeDummy);
		const SQChar *parentName = GetTypeName(parentDummy);

		sq_pushstring(v, typeName, -1);
		if (parentName) {
			// 親クラスが指定されてる場合は継承処理
			::registerInherit(typeName, parentName);
			sq_pushstring(v, parentName, -1);
			if (SQ_SUCCEEDED(sq_get(v,-3))) {
				sq_newclass(v, true);
			} else {
				sq_newclass(v, false);
			}
		} else {
			// 継承なしでクラス生成
			sq_newclass(v, false);
		}
		// タグを登録
		sq_settypetag(v, -1, typetag);
		::registerTypeTag(typeName, typetag);
		
		// コンストラクタを登録
		Register(constructor, _SC("constructor"));
	}
	
	/*
	 * ネイティブオブジェクトのリリーサ。
	 */
	static SQRESULT release(SQUserPointer up, SQInteger size) {
		if (up) {
			T* self = (T*)up;
			if (self) {
				self->destructor();
				self->~T();
				sq_free(up, size);
			}
		}
		return SQ_OK;
	}
	
	/**
	 * コンストラクタ
	 * ネイティブオブジェクトのコンストラクタに引数として HSQUIRRELVM を渡す。
	 */
	static SQRESULT constructor(HSQUIRRELVM v) {
		T *self = (T*)sq_malloc(sizeof *self);
		new (self) T(v);
		if (self) {
			SQRESULT result;
			if (SQ_SUCCEEDED(result = sq_setinstanceup(v, 1, self))) {;
				sq_setreleasehook(v, 1, release);
			} else {
				self->~T();
				sq_free(self, sizeof *self);
			}
			return result;
		} else {
			return ERROR_CREATE(v);
		}
	}

	// -------------------------------------------------
	// スタティック関数の登録
	// -------------------------------------------------

	// SQFUNCTION 登録
	void Register(SQFUNCTION func, const SQChar *name) {
		sq_pushstring(v, name, -1);
		sq_newclosure(v, func, 0);
		sq_createslot(v, -3);
	}

	// -------------------------------------------------
	// メンバ関数の登録用
	// -------------------------------------------------

	// インスタンス取得
	static T *getInstance(HSQUIRRELVM v, int idx=1) {
		const T *dummy = NULL;
		return static_cast<T*>(::getInstance(v,idx,GetTypeName(dummy)));
	}
	
	// 関数ポインタ取得
	template <typename Func>
	static void getFunc(HSQUIRRELVM v, Func **func) {
		SQUserPointer x = NULL;
		sq_getuserdata(v,sq_gettop(v),&x,NULL);
		*func = (Func*)x;
	}

	// -------------------------------------------------

	// 帰り値 int で引数無しの関数
	typedef void (T::*VoidFunc)();

	// IntFunc 呼び出し
	static SQRESULT VoidFuncCaller(HSQUIRRELVM v) {
		T *instance = getInstance(v);
		if (instance) {
			VoidFunc *func;
			getFunc(v, &func);
			if (func) {
				(instance->*(*func))();
				return 0;
			}
			return ERROR_BADMETHOD(v);
		}
		return ERROR_BADINSTANCE(v);
	}

	// VoidFunc 登録
	void Register(VoidFunc func, const SQChar *name) {
		sq_pushstring(v, name, -1);
		SQUserPointer up = sq_newuserdata(v,sizeof(func));
		memcpy(up,&func,sizeof(func));
		sq_newclosure(v,VoidFuncCaller,1);
		sq_createslot(v, -3);
	}

	// -------------------------------------------------
	
	// 帰り値 int で引数無しの関数
	typedef int (T::*IntFunc)();

	// IntFunc 呼び出し
	static SQRESULT IntFuncCaller(HSQUIRRELVM v) {
		T *instance = getInstance(v);
		if (instance) {
			IntFunc *func;
			getFunc(v, &func);
			if (func) {
				sq_pushinteger(v, (instance->*(*func))());
				return 1;
			}
			return ERROR_BADMETHOD(v);
		}
		return ERROR_BADINSTANCE(v);
	}

	// IntFunc 登録
	void Register(IntFunc func, const SQChar *name) {
		sq_pushstring(v, name, -1);
		SQUserPointer up = sq_newuserdata(v,sizeof(func));
		memcpy(up,&func,sizeof(func));
		sq_newclosure(v,IntFuncCaller,1);
		sq_createslot(v, -3);
	}

	// -------------------------------------------------

	// SQFUNCTION スタイルの関数
	typedef SQRESULT (T::*VFunc)(HSQUIRRELVM v);

	// VFunc 呼び出し
	static SQRESULT VFuncCaller(HSQUIRRELVM v) {
		T *instance = getInstance(v);
		if (instance) {
			VFunc *func = NULL;
			getFunc(v, &func);
			if (func) {
				return (instance->*(*func))(v);
			}
			return ERROR_BADMETHOD(v);
		}
		return ERROR_BADINSTANCE(v);
	}

	// VFunc 登録
	void RegisterV(VFunc func, const SQChar *name) {
		sq_pushstring(v, name, -1);
		SQUserPointer up = sq_newuserdata(v,sizeof(func));
		memcpy(up,&func,sizeof(func));
		sq_newclosure(v,VFuncCaller,1);
		sq_createslot(v, -3);
	}
};

namespace sqobject {

// typetag 全ソースでユニークなアドレスにする必要がある
const SQUserPointer OBJECTTYPETAG = (SQUserPointer)"OBJECTTYPETAG";
const SQUserPointer THREADTYPETAG = (SQUserPointer)"THREADTYPETAG";

// クラス情報定義

DECLARE_CLASSNAME(Object, SQOBJECT);
DECLARE_CLASSNAME(Thread, SQTHREAD);

// global vm
HSQUIRRELVM vm;

/// vm 初期化
HSQUIRRELVM init() {
	vm = sq_open(1024);
	sq_pushroottable(vm);
	sqstd_register_mathlib(vm);
	sqstd_register_stringlib(vm);
	sqstd_seterrorhandlers(vm);
	sq_pop(vm,1);
	typeMap.initTable();
	typeTagListMap.initTable();
	return vm;
}

/// 情報保持用グローバルVMの取得
HSQUIRRELVM getGlobalVM()
{
	return vm;
}

/// vm 終了
void done()
{
	// ルートテーブルをクリア
	sq_pushroottable(vm);
	sq_clear(vm,-1);
	sq_pop(vm,1);
	typeTagListMap.clearData();
	typeTagListMap.clear();
	typeMap.clearData();
	typeMap.clear();
	sq_close(vm);
}

// Thread のインスタンスユーザポインタを取得
Thread *
ObjectInfo::getThread()
{
	HSQUIRRELVM gv = getGlobalVM();
	push(gv);
	Thread *ret = NULL;
	ret = (Thread*)::getInstance(gv, -1, GetTypeName(ret));
	sq_pop(gv,1);
	return ret;
}

// Object のインスタンスユーザポインタを取得
Object *
ObjectInfo::getObject()
{
	HSQUIRRELVM gv = getGlobalVM();
	push(gv);
	Object *ret = NULL;
	ret = (Object*)::getInstance(gv, -1, GetTypeName(ret));
	sq_pop(gv,1);
	return ret;
}

// ------------------------------------------------------------------
// クラス登録用マクロ
// ------------------------------------------------------------------

#define SQCLASS(Class,Parent,typetag) SQTemplate<Class,Parent> cls(v,typetag)
#define SQFUNC(Class, Name)   cls.Register(&Class::Name, _SC(#Name))
#define SQVFUNC(Class, Name)  cls.RegisterV(&Class::Name, _SC(#Name))
#define SQNFUNC(Class, Name) cls.Register(Class ## _ ## Name, _SC(#Name))

#ifndef USE_SQOBJECT_TEMPLATE

static SQRESULT Object_notify(HSQUIRRELVM v)
{
	Object *instance = SQTemplate<Object,BaseClass>::getInstance(v);
	if (instance) {
		instance->notify();
		return SQ_OK;
	}
	return ERROR_BADINSTANCE(v);
}

static SQRESULT Object_notifyAll(HSQUIRRELVM v)
{
	Object *instance = SQTemplate<Object,BaseClass>::getInstance(v);
	if (instance) {
		instance->notifyAll();
		return SQ_OK;
	}
	return ERROR_BADINSTANCE(v);
}

static SQRESULT Object_hasSetProp(HSQUIRRELVM v)
{
	Object *instance = SQTemplate<Object,BaseClass>::getInstance(v);
	if (instance) {
		return instance->hasSetProp(v);
	}
	return ERROR_BADINSTANCE(v);
}

static SQRESULT Object_setDelegate(HSQUIRRELVM v)
{
	Object *instance = SQTemplate<Object,BaseClass>::getInstance(v);
	if (instance) {
		return instance->setDelegate(v);
	}
	return ERROR_BADINSTANCE(v);
}

static SQRESULT Object_getDelegate(HSQUIRRELVM v)
{
	Object *instance = SQTemplate<Object,BaseClass>::getInstance(v);
	if (instance) {
		return instance->getDelegate(v);
	}
	return ERROR_BADINSTANCE(v);
}

static SQRESULT Object_get(HSQUIRRELVM v)
{
	Object *instance = SQTemplate<Object,BaseClass>::getInstance(v);
	if (instance) {
		return instance->_get(v);
	}
	return ERROR_BADINSTANCE(v);
}

static SQRESULT Object_set(HSQUIRRELVM v)
{
	Object *instance = SQTemplate<Object,BaseClass>::getInstance(v);
	if (instance) {
		return instance->_set(v);
	}
	return ERROR_BADINSTANCE(v);
}

#endif

/**
 * クラスの登録
 * @param v squirrel VM
 */
void
Object::registerClass()
{
	HSQUIRRELVM v = getGlobalVM();
	sq_pushroottable(v); // root

	SQCLASS(Object,BaseClass,OBJECTTYPETAG);

#ifdef USE_SQOBJECT_TEMPLATE
	SQFUNC(Object,notify);
	SQFUNC(Object,notifyAll);
	SQVFUNC(Object,hasSetProp);
	SQVFUNC(Object,setDelegate);
	SQVFUNC(Object,getDelegate);
	SQVFUNC(Object,_get);
	SQVFUNC(Object,_set);
	cls.RegisterV(&Object::_get, _SC("get"));
	cls.RegisterV(&Object::_set, _SC("set"));
#else
	SQNFUNC(Object,notify);
	SQNFUNC(Object,notifyAll);
	SQNFUNC(Object,hasSetProp);
	SQNFUNC(Object,setDelegate);
	SQNFUNC(Object,getDelegate);
	cls.Register(Object_get, _SC("_get"));
	cls.Register(Object_set, _SC("_set"));
	SQNFUNC(Object,get);
	SQNFUNC(Object,set);
#endif
	sq_createslot(v, -3); // 生成したクラスを登録
	sq_pop(v,1); // root
};

#ifndef USE_SQOBJECT_TEMPLATE

static SQRESULT Thread_exec(HSQUIRRELVM v)
{
	Thread *instance = SQTemplate<Thread,Object>::getInstance(v);
	if (instance) {
		return instance->exec(v);
	}
	return ERROR_BADINSTANCE(v);
}

static SQRESULT Thread_exit(HSQUIRRELVM v)
{
	Thread *instance = SQTemplate<Thread,Object>::getInstance(v);
	if (instance) {
		return instance->exit(v);
	}
	return ERROR_BADINSTANCE(v);
}

static SQRESULT Thread_stop(HSQUIRRELVM v)
{
	Thread *instance = SQTemplate<Thread,Object>::getInstance(v);
	if (instance) {
		instance->stop();
		return SQ_OK;
	}
	return ERROR_BADINSTANCE(v);
}

static SQRESULT Thread_run(HSQUIRRELVM v)
{
	Thread *instance = SQTemplate<Thread,Object>::getInstance(v);
	if (instance) {
		instance->run();
		return SQ_OK;
	}
	return ERROR_BADINSTANCE(v);
}

static SQRESULT Thread_getCurrentTick(HSQUIRRELVM v)
{
	Thread *instance = SQTemplate<Thread,Object>::getInstance(v);
	if (instance) {
		sq_pushinteger(v,instance->getCurrentTick());
		return 1;
	}
	return ERROR_BADINSTANCE(v);
}

static SQRESULT Thread_getStatus(HSQUIRRELVM v)
{
	Thread *instance = SQTemplate<Thread,Object>::getInstance(v);
	if (instance) {
		sq_pushinteger(v,instance->getStatus());
		return 1;
	}
	return ERROR_BADINSTANCE(v);
}

static SQRESULT Thread_getExitCode(HSQUIRRELVM v)
{
	Thread *instance = SQTemplate<Thread,Object>::getInstance(v);
	if (instance) {
		return instance->getExitCode(v);
	}
	return ERROR_BADINSTANCE(v);
}

static SQRESULT Thread_wait(HSQUIRRELVM v)
{
	Thread *instance = SQTemplate<Thread,Object>::getInstance(v);
	if (instance) {
		return instance->wait(v);
	}
	return ERROR_BADINSTANCE(v);
}

static SQRESULT Thread_cancelWait(HSQUIRRELVM v)
{
	Thread *instance = SQTemplate<Thread,Object>::getInstance(v);
	if (instance) {
		instance->cancelWait();
		return SQ_OK;
	}
	return ERROR_BADINSTANCE(v);
}

#endif

/**
 * クラスの登録
 * @param v squirrel VM
 */
void
Thread::registerClass()
{
	HSQUIRRELVM v = getGlobalVM();
	sq_pushroottable(v); // root

	SQCLASS(Thread, Object, THREADTYPETAG);

#ifdef USE_SQOBJECT_TEMPLATE
	SQVFUNC(Thread,exec);
	SQVFUNC(Thread,exit);
	SQFUNC(Thread,stop);
	SQFUNC(Thread,run);
	SQFUNC(Thread,getCurrentTick);
	SQFUNC(Thread,getStatus);
	SQVFUNC(Thread,getExitCode);
	SQVFUNC(Thread,wait);
	SQFUNC(Thread,cancelWait);
#else
	SQNFUNC(Thread,exec);
	SQNFUNC(Thread,exit);
	SQNFUNC(Thread,stop);
	SQNFUNC(Thread,run);
	SQNFUNC(Thread,getCurrentTick);
	SQNFUNC(Thread,getStatus);
	SQNFUNC(Thread,getExitCode);
	SQNFUNC(Thread,wait);
	SQNFUNC(Thread,cancelWait);
#endif
	
	sq_createslot(v, -3);
	sq_pop(v, 1); // root
};

}

