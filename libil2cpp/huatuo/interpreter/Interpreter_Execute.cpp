﻿
#include "Interpreter.h"

#include <cmath>

#include "codegen/il2cpp-codegen-il2cpp.h"
#include "vm/Object.h"
#include "vm/Class.h"
#include "vm/ClassInlines.h"
#include "vm/Array.h"
#include "vm/Image.h"
#include "vm/Exception.h"
#include "vm/Thread.h"
#include "metadata/GenericMetadata.h"

#include "Instruction.h"
#include "MethodBridge.h"
#include "InstrinctDef.h"
#include "MemoryUtil.h"
#include "../metadata/MetadataModule.h"
#include "InterpreterModule.h"

using namespace huatuo::metadata;

namespace huatuo
{
namespace interpreter
{

#pragma region memory

#define LOCAL_ALLOC(size) interpFrameGroup.AllocLoc(size)

#pragma endregion

#pragma region arith

#define CHECK_ADD_OVERFLOW(a,b) \
(int32_t)(b) >= 0 ? (int32_t)(INT32_MAX) - (int32_t)(b) < (int32_t)(a) ? -1 : 0	\
: (int32_t)(INT32_MIN) - (int32_t)(b) > (int32_t)(a) ? +1 : 0

#define CHECK_SUB_OVERFLOW(a,b) \
(int32_t)(b) < 0 ? (int32_t)(INT32_MAX) + (int32_t)(b) < (int32_t)(a) ? -1 : 0	\
: (int32_t)(INT32_MIN) + (int32_t)(b) > (int32_t)(a) ? +1 : 0

#define CHECK_ADD_OVERFLOW_UN(a,b) \
(uint32_t)(UINT32_MAX) - (uint32_t)(b) < (uint32_t)(a) ? -1 : 0

#define CHECK_SUB_OVERFLOW_UN(a,b) \
(uint32_t)(a) < (uint32_t)(b) ? -1 : 0

#define CHECK_ADD_OVERFLOW64(a,b) \
(int64_t)(b) >= 0 ? (int64_t)(INT64_MAX) - (int64_t)(b) < (int64_t)(a) ? -1 : 0	\
: (int64_t)(INT64_MIN) - (int64_t)(b) > (int64_t)(a) ? +1 : 0

#define CHECK_SUB_OVERFLOW64(a,b) \
(int64_t)(b) < 0 ? (int64_t)(INT64_MAX) + (int64_t)(b) < (int64_t)(a) ? -1 : 0	\
: (int64_t)(INT64_MIN) + (int64_t)(b) > (int64_t)(a) ? +1 : 0

#define CHECK_ADD_OVERFLOW64_UN(a,b) \
(uint64_t)(UINT64_MAX) - (uint64_t)(b) < (uint64_t)(a) ? -1 : 0

#define CHECK_SUB_OVERFLOW64_UN(a,b) \
(uint64_t)(a) < (uint64_t)(b) ? -1 : 0

#define CHECK_ADD_OVERFLOW_NAT(a,b) CHECK_ADD_OVERFLOW64(a,b)
#define CHECK_ADD_OVERFLOW_NAT_UN(a,b) CHECK_ADD_OVERFLOW64_UN(a,b)

	/* Resolves to TRUE if the operands would overflow */
#define CHECK_MUL_OVERFLOW(a,b) \
((int32_t)(a) == 0) || ((int32_t)(b) == 0) ? 0 : \
(((int32_t)(a) > 0) && ((int32_t)(b) == -1)) ? 0 : \
(((int32_t)(a) < 0) && ((int32_t)(b) == -1)) ? (a == INT32_MIN) : \
(((int32_t)(a) > 0) && ((int32_t)(b) > 0)) ? (int32_t)(a) > ((INT32_MAX) / (int32_t)(b)) : \
(((int32_t)(a) > 0) && ((int32_t)(b) < 0)) ? (int32_t)(a) > ((INT32_MIN) / (int32_t)(b)) : \
(((int32_t)(a) < 0) && ((int32_t)(b) > 0)) ? (int32_t)(a) < ((INT32_MIN) / (int32_t)(b)) : \
(int32_t)(a) < ((INT32_MAX) / (int32_t)(b))

#define CHECK_MUL_OVERFLOW_UN(a,b) \
((uint32_t)(a) == 0) || ((uint32_t)(b) == 0) ? 0 : \
(uint32_t)(b) > ((UINT32_MAX) / (uint32_t)(a))

#define CHECK_MUL_OVERFLOW64(a,b) \
((int64_t)(a) == 0) || ((int64_t)(b) == 0) ? 0 : \
(((int64_t)(a) > 0) && ((int64_t)(b) == -1)) ? 0 : \
(((int64_t)(a) < 0) && ((int64_t)(b) == -1)) ? (a == INT64_MIN) : \
(((int64_t)(a) > 0) && ((int64_t)(b) > 0)) ? (int64_t)(a) > ((INT64_MAX) / (int64_t)(b)) : \
(((int64_t)(a) > 0) && ((int64_t)(b) < 0)) ? (int64_t)(a) > ((INT64_MIN) / (int64_t)(b)) : \
(((int64_t)(a) < 0) && ((int64_t)(b) > 0)) ? (int64_t)(a) < ((INT64_MIN) / (int64_t)(b)) : \
(int64_t)(a) < ((INT64_MAX) / (int64_t)(b))

#define CHECK_MUL_OVERFLOW64_UN(a,b) \
((uint64_t)(a) == 0) || ((uint64_t)(b) == 0) ? 0 : \
(uint64_t)(b) > ((UINT64_MAX) / (uint64_t)(a))



	inline int32_t HiDiv(int32_t a, int32_t b)
	{
		if (b == 0)
		{
			il2cpp::vm::Exception::RaiseDivideByZeroException();
		}
		return a / b;
	}

	inline int64_t HiDiv(int64_t a, int64_t b)
	{
		if (b == 0)
		{
			il2cpp::vm::Exception::RaiseDivideByZeroException();
		}
		return a / b;
	}

	inline float HiDiv(float a, float b)
	{
		return a / b;
	}

	inline double HiDiv(double a, double b)
	{
		return a / b;
	}

	inline int32_t HiMulUn(int32_t a, int32_t b)
	{
		return (uint32_t)a * (uint32_t)b;
	}

	inline int64_t HiMulUn(int64_t a, int64_t b)
	{
		return (uint64_t)a * (uint64_t)b;
	}

	inline int32_t HiDivUn(int32_t a, int32_t b)
	{
		return (uint32_t)a / (uint32_t)b;
	}

	inline int64_t HiDivUn(int64_t a, int64_t b)
	{
		return (uint64_t)a / (uint64_t)b;
	}

	inline float HiRem(float a, float b)
	{
		return std::fmodf(a, b);
	}

	inline double HiRem(double a, double b)
	{
		return std::fmod(a, b);
	}

	inline int32_t HiRem(int32_t a, int32_t b)
	{
		return a % b;
	}

	inline int64_t HiRem(int64_t a, int64_t b)
	{
		return a % b;
	}

	inline uint32_t HiRemUn(int32_t a, int32_t b)
	{
		return (uint32_t)a % (uint32_t)b;
	}

	inline uint64_t HiRemUn(int64_t a, int64_t b)
	{
		return (uint64_t)a % (uint64_t)b;
	}

	inline uint32_t HiShrUn(int32_t a, int64_t b)
	{
		return (uint32_t)a >> b;
	}

	inline uint32_t HiShrUn(int32_t a, int32_t b)
	{
		return (uint32_t)a >> b;
	}

	inline uint64_t HiShrUn(int64_t a, int32_t b)
	{
		return (uint64_t)a >> b;
	}

	inline uint64_t HiShrUn(int64_t a, int64_t b)
	{
		return (uint64_t)a >> b;
	}


	inline void HiCheckFinite(float x)
	{
		if (std::isinf(x) || std::isnan(x))
		{
			il2cpp::vm::Exception::RaiseOverflowException();
		}
	}

	inline void HiCheckFinite(double x)
	{
		if (std::isinf(x) || std::isnan(x))
		{
			il2cpp::vm::Exception::RaiseOverflowException();
		}
	}



#define DefCompare(cmp, op) template<typename T> bool Compare##cmp(T a, T b) { return a op b; }
#define DefCompareUn(cmp, op) inline bool Compare##cmp(int32_t a, int32_t b) { return (uint32_t)a op (uint32_t)b; } \
inline bool Compare##cmp(int64_t a, int64_t b) { return (uint64_t)a op (uint64_t)b; } \
inline bool Compare##cmp(float a, float b) { return a op b; } \
inline bool Compare##cmp(double a, double b) { return a op b; }

	DefCompare(Ceq, == );
	DefCompare(Cne, != );
	DefCompare(Cgt, > );
	DefCompare(Cge, >= );
	DefCompare(Clt, < );
	DefCompare(Cle, <= );

	DefCompareUn(CneUn, != );
	DefCompareUn(CgtUn, > );
	DefCompareUn(CgeUn, >= );
	DefCompareUn(CltUn, < );
	DefCompareUn(CleUn, <= );

#pragma endregion

#pragma region object

	inline void INIT_CLASS(Il2CppClass* klass)
	{
		il2cpp::vm::ClassInlines::InitFromCodegen(klass);
	}

	inline int32_t GetTypeValueSize(Il2CppClass* klass)
	{
		return klass->instance_size - sizeof(Il2CppObject);
	}

	inline void CHECK_NOT_NULL_THROW(const void* ptr)
	{
		if (!ptr)
		{
			il2cpp::vm::Exception::RaiseNullReferenceException();
		}
	}

#define CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(ARR) CHECK_NOT_NULL_THROW(ARR); \
if (ARR->max_length <= (*(uint32_t*)(localVarBase + __index))) { \
	il2cpp::vm::Exception::Raise(il2cpp::vm::Exception::GetIndexOutOfRangeException()); \
}

	inline void CHECK_TYPE_MATCH_ELSE_THROW(Il2CppClass* klass1, Il2CppClass* klass2)
	{
		if (klass1 != klass2)
		{
			il2cpp::vm::Exception::Raise(il2cpp::vm::Exception::GetArrayTypeMismatchException());
		}
	}

	inline void CheckArrayElementTypeCompatible(Il2CppClass* arrKlass, Il2CppClass* eleKlass)
	{
		if (il2cpp::vm::Class::GetElementClass(arrKlass) != eleKlass)
		{
			il2cpp::vm::Exception::Raise(il2cpp::vm::Exception::GetArrayTypeMismatchException());
		}
	}

#define CHECK_ARRAY_INDEX_IN_RANGE_ELSE_THROW(obj, length) if (length >= il2cpp::vm::Array::GetLength(obj)) { il2cpp::vm::Exception::Raise(il2cpp::vm::Exception::GetIndexOutOfRangeException()); }
#define CHECK_ARRAY_TYPE_COMPATIBLE(arr, klazz) if (!il2cpp::vm::Class::IsAssignableFrom((arr)->klass->element_class, klazz)) { il2cpp::vm::Exception::Raise(il2cpp::vm::Exception::GetArrayTypeMismatchException()); }

#define GET_RUNTIMEHANDLE_FROM_TOKEN(token) ((void*)nullptr)

	inline MethodInfo* GET_OBJECT_VIRTUAL_METHOD(Il2CppObject* obj, const MethodInfo* method)
	{
		const MethodInfo* result;
		if (huatuo::metadata::IsVirtualMethod(method->flags))
		{
			if (huatuo::metadata::IsInterface(method->klass->flags))
			{
				result = il2cpp_codegen_get_interface_invoke_data(method->slot, obj, method->klass).method;
				/*if (method->is_inflated)
				{
					result = il2cpp_codegen_get_generic_interface_method(method, obj);
				}
				else
				{
					result = il2cpp_codegen_get_interface_invoke_data(method->slot, obj, method->klass).method;
				}*/
			}
			else
			{
				result = il2cpp_codegen_get_virtual_invoke_data(method->slot, obj).method;
				/*if (method->is_inflated)
				{
					result = il2cpp_codegen_get_generic_virtual_method(method, obj);
				}
				else
				{
					result = il2cpp_codegen_get_virtual_invoke_data(method->slot, obj).method;
				}*/
			}
			if (result->genericMethod && method->genericMethod) // means it's genericInstance method 或generic method
			{
				//IL2CPP_ASSERT(method->genericMethod);
				result = il2cpp::metadata::GenericMetadata::Inflate(GetUnderlyingMethodInfo(result), &method->genericMethod->context);
			}
		}
		else
		{
			result = method;
		}
		return const_cast<MethodInfo*>(result);
	}

#define GET_OBJECT_INTERFACE_METHOD(obj, intfKlass, slot) (MethodInfo*)nullptr

	inline void* HiUnbox(Il2CppObject* obj, Il2CppClass* klass)
	{
		if (il2cpp::vm::Class::IsNullable(klass))
		{
			if (!obj)
			{
				return nullptr;
			}
			klass = il2cpp::vm::Class::GetNullableArgument(klass);
		}
		else
		{
			if (obj == nullptr)
			{
				il2cpp::vm::Exception::RaiseNullReferenceException();
			}
		}

		if (klass != obj->klass)
		{
			il2cpp::vm::Exception::Raise(il2cpp::vm::Exception::GetInvalidCastException("cast fail"), nullptr);
		}
		return il2cpp::vm::Object::Unbox(obj);
	}

	inline void HiUnboxAny(Il2CppObject* obj, Il2CppClass* klass, void* data)
	{
		if (il2cpp::vm::Class::IsNullable(klass))
		{
			UnBoxNullable(obj, klass, data);
		}
		else
		{
			CHECK_NOT_NULL_THROW(obj);
			if (klass != obj->klass)
			{
				il2cpp::vm::Exception::Raise(il2cpp::vm::Exception::GetInvalidCastException("cast fail"), nullptr);
			}
			std::memcpy(data, il2cpp::vm::Object::Unbox(obj), klass->instance_size - sizeof(Il2CppObject));
		}
	}

	inline void HiCastClass(Il2CppObject* obj, Il2CppClass* klass)
	{
		if (obj != nullptr && !il2cpp::vm::Class::IsAssignableFrom(klass, obj->klass))
		{
			il2cpp::vm::Exception::Raise(il2cpp::vm::Exception::GetInvalidCastException("cast fail"), nullptr);
		}
	}

	inline Il2CppTypedRef MAKE_TYPEDREFERENCE(Il2CppClass* klazz, void* ptr)
	{
		return Il2CppTypedRef{ &klazz->byval_arg, ptr, klazz };
	}

	inline void* RefAnyType(Il2CppTypedRef ref)
	{
		return (void*)ref.type;
	}

	inline void* RefAnyValue(Il2CppTypedRef ref, Il2CppClass* klass)
	{
		if (klass != ref.klass)
		{
			il2cpp::vm::Exception::Raise(il2cpp::vm::Exception::GetInvalidCastException(klass->name));
		}
		return ref.value;
	}

	// not boxed data
	inline void InitNullableValueType(void* nullableValueTypeObj, void* data, uint32_t size)
	{
		*((uint8_t*)nullableValueTypeObj + size) = 1;
		memcpy(nullableValueTypeObj, data, size);
	}

	inline void NewNullableValueType(void* nullableValueTypeObj, void* data, uint32_t size)
	{
		*((uint8_t*)nullableValueTypeObj + size) = 1;
		memcpy(nullableValueTypeObj, data, size);
	}

	static_assert(sizeof(il2cpp_array_size_t) == 8, "array size type == 8");

	inline Il2CppArray* NewMdArray(Il2CppClass* fullArrKlass, il2cpp_array_size_t* lengths, il2cpp_array_size_t* lowerBounds)
	{
		switch (fullArrKlass->rank)
		{
		case 1:
		{
			if (lengths)
			{
				lengths[0] = (int32_t)lengths[0];
			}
			if (lowerBounds)
			{
				lowerBounds[0] = (int32_t)lowerBounds[0];
			}
			break;
		}
		case 2:
		{
			if (lengths)
			{
				lengths[0] = (int32_t)lengths[0];
				lengths[1] = (int32_t)lengths[1];
			}
			if (lowerBounds)
			{
				lowerBounds[0] = (int32_t)lowerBounds[0];
				lowerBounds[1] = (int32_t)lowerBounds[1];
			}
			break;
		}
		default:
		{
			for (uint8_t i = 0; i < fullArrKlass->rank; i++)
			{
				if (lengths)
				{
					lengths[i] = (int32_t)lengths[i];
				}
				if (lowerBounds)
				{
					lowerBounds[i] = (int32_t)lowerBounds[i];
				}
			}
			break;
		}
		}
		return il2cpp::vm::Array::NewFull(fullArrKlass, lengths, lowerBounds);
	}

	inline void* GetMdArrayElementAddress(Il2CppArray* arr, il2cpp_array_size_t* indexs)
	{
		Il2CppClass* klass = arr->klass;
		uint32_t eleSize = klass->element_size;
		Il2CppArrayBounds* bounds = arr->bounds;
		char* arrayStart = il2cpp::vm::Array::GetFirstElementAddress(arr);
		switch (klass->rank)
		{
		case 1:
		{
			Il2CppArrayBounds& bound = bounds[0];
			int32_t idx = (int32_t)indexs[0] - bound.lower_bound;
			if (idx >= 0 && idx < bound.length)
			{
				return arrayStart + (idx * eleSize);
			}
			else
			{
				il2cpp::vm::Exception::Raise(il2cpp::vm::Exception::GetIndexOutOfRangeException());
			}
			break;
		}
		case 2:
		{
			Il2CppArrayBounds& bound0 = bounds[0];
			int32_t idx0 = (int32_t)indexs[0] - bound0.lower_bound;
			Il2CppArrayBounds& bound1 = bounds[1];
			int32_t idx1 = (int32_t)indexs[1] - bound1.lower_bound;
			if (idx0 >= 0 && idx0 < bound0.length && idx1 >= 0 && idx1 < bound1.length)
			{
				return arrayStart + ((idx0 * bound1.length) + idx1) * eleSize;
			}
			else
			{
				il2cpp::vm::Exception::Raise(il2cpp::vm::Exception::GetIndexOutOfRangeException());
			}
			break;
		}
		case 3:
		{
			Il2CppArrayBounds& bound0 = bounds[0];
			int32_t idx0 = (int32_t)indexs[0] - bound0.lower_bound;
			Il2CppArrayBounds& bound1 = bounds[1];
			int32_t idx1 = (int32_t)indexs[1] - bound1.lower_bound;
			Il2CppArrayBounds& bound2 = bounds[2];
			int32_t idx2 = (int32_t)indexs[2] - bound2.lower_bound;
			if (idx0 >= 0 && idx0 < bound0.length && idx1 >= 0 && idx1 < bound1.length && idx2 >= 0 && idx2 < bound2.length)
			{
				return arrayStart + ((idx0 * bound1.length + idx1) * bound2.length + idx2) * eleSize;
			}
			else
			{
				il2cpp::vm::Exception::Raise(il2cpp::vm::Exception::GetIndexOutOfRangeException());
			}
			break;
		}
		default:
		{
			IL2CPP_ASSERT(klass->rank > 0);
			il2cpp_array_size_t totalIdx = 0;
			for (uint8_t i = 0; i < klass->rank; i++)
			{
				Il2CppArrayBounds& bound = bounds[i];
				int32_t idx = (int32_t)indexs[i] - bound.lower_bound;
				if (idx >= 0 && idx < bound.length)
				{
					totalIdx = totalIdx * bound.length + idx;
				}
			}
			return arrayStart + totalIdx * eleSize;
		}
		}
	}

	inline void GetMdArrayElement(Il2CppArray* arr, il2cpp_array_size_t* indexs, void* value)
	{
		CopyBySize(value, GetMdArrayElementAddress(arr, indexs), arr->klass->element_size);
	}

	inline void SetMdArrayElement(Il2CppArray* arr, il2cpp_array_size_t* indexs, void* value)
	{
		CopyBySize(GetMdArrayElementAddress(arr, indexs), value, arr->klass->element_size);
	}

#pragma endregion


#pragma region misc

	inline bool IsNullableHasValue(void* nullableValueObj, uint32_t offset)
	{
		return *((uint8_t*)nullableValueObj + offset);
	}

	inline void GetNullableValueOrDefault(void* dst, void* nullableValueObj, uint32_t size)
	{
		if (*((uint8_t*)nullableValueObj + size))
		{
			std::memcpy(dst, nullableValueObj, size);
		}
		else
		{
			std::memset(dst, 0, size);
		}
	}

	inline void GetNullableValueOrDefault(void* dst, void* nullableValueObj, void* defaultData, uint32_t size)
	{
		std::memcpy(dst, *((uint8_t*)nullableValueObj + size) ? nullableValueObj : defaultData, size);
	}

	inline void GetNullableValue(void* dst, void* nullableValueObj, uint32_t size)
	{
		if (*((uint8_t*)nullableValueObj + size))
		{
			std::memcpy(dst, nullableValueObj, size);
		}
		else
		{
			il2cpp::vm::Exception::Raise(il2cpp::vm::Exception::GetInvalidOperationException("Nullable object must have a value."));
		}
	}

	inline int32_t HiInterlockedCompareExchange(int32_t* location, int32_t newValue, int32_t oldValue)
	{
		return il2cpp::os::Atomic::CompareExchange(location, newValue, oldValue);
	}


	inline int64_t HiInterlockedCompareExchange(int64_t* location, int64_t newValue, int64_t oldValue)
	{
		return il2cpp::os::Atomic::CompareExchange64(location, newValue, oldValue);
	}


	inline void* HiInterlockedCompareExchange(void** location, void* newValue, void* oldValue)
	{
		return il2cpp::os::Atomic::CompareExchangePointer(location, newValue, oldValue);
	}

	inline int32_t HiInterlockedExchange(int32_t* location, int32_t newValue)
	{
		return il2cpp::os::Atomic::Exchange(location, newValue);
	}

	inline int64_t HiInterlockedExchange(int64_t* location, int64_t newValue)
	{
		return il2cpp::os::Atomic::Exchange64(location, newValue);
	}

	inline void* HiInterlockedExchange(void** location, void* newValue)
	{
		return il2cpp::os::Atomic::ExchangePointer(location, newValue);
	}

#define MEMORY_BARRIER() il2cpp::os::Atomic::FullMemoryBarrier()

#pragma endregion

#pragma region function

	inline void CallDelegateMethod(const MethodInfo* method, Il2CppObject* obj, Managed2NativeCallMethod staticM2NMethod, Managed2NativeCallMethod instanceM2NMethod, uint16_t* argIdxs, StackObject* localVarBase, void* ret)
	{
		if (huatuo::metadata::IsInstanceMethod(method))
		{
			(localVarBase + argIdxs[0])->obj = obj;
			method = GET_OBJECT_VIRTUAL_METHOD(obj, method);
			instanceM2NMethod(method, argIdxs, localVarBase, ret);
		}
		else
		{
			staticM2NMethod(method, argIdxs + 1, localVarBase, ret);
		}
	}

	inline void HiCallDelegate(Il2CppMulticastDelegate* del, Managed2NativeCallMethod staticM2NMethod, Managed2NativeCallMethod instanceM2NMethod, uint16_t* argIdxs, StackObject* localVarBase, void* ret)
	{
		if (del->delegates == nullptr)
		{
			CallDelegateMethod(del->delegate.method, del->delegate.target, staticM2NMethod, instanceM2NMethod, argIdxs, localVarBase, ret);
		}
		else
		{
			Il2CppArray* dels = del->delegates;
			for (il2cpp_array_size_t i = 0; i < dels->max_length; i++)
			{
				Il2CppMulticastDelegate* subDel = il2cpp_array_get(dels, Il2CppMulticastDelegate*, i);
				IL2CPP_ASSERT(subDel);
				//IL2CPP_ASSERT(subDel->delegate.method->klass->parent == il2cpp_defaults.multicastdelegate_class);
				IL2CPP_ASSERT(subDel->delegates == nullptr);
				CallDelegateMethod(subDel->delegate.method, subDel->delegate.target, staticM2NMethod, instanceM2NMethod, argIdxs, localVarBase, ret);
			}
		}
	}



#define SAVE_CUR_FRAME(nextIp) { \
	frame->ip = nextIp; \
}

#define LOAD_PREV_FRAME() { \
	imi = frame->method; \
	ip = frame->ip; \
	ipBase = imi->codes; \
	localVarBase = frame->stackBasePtr; \
}



	// maxStackSize包含 arg + local + eval,对于解释器栈来说，可能多余
#define PREPARE_NEW_FRAME(newMethodInfo, argBasePtr, retPtr, withArgStack) { \
	imi = newMethodInfo->huatuoData ? (InterpMethodInfo*)newMethodInfo->huatuoData : InterpreterModule::GetInterpMethodInfo(huatuo::metadata::MetadataModule::GetImage(newMethodInfo->klass), newMethodInfo); \
	frame = interpFrameGroup.EnterFrame(imi, argBasePtr, withArgStack); \
	RuntimeClassCCtorInit(newMethodInfo); \
	frame->ret = retPtr; \
	ip = ipBase = imi->codes; \
	localVarBase = frame->stackBasePtr; \
}

#define LEAVE_FRAME() { \
	frame = interpFrameGroup.LeaveFrame(); \
	if (frame) \
	{ \
		LOAD_PREV_FRAME(); \
	}\
	else \
	{ \
		goto ExitEvalLoop; \
	} \
}

#define CALL_INTERP(nextIp, methodInfo, argBasePtr, retPtr) { \
	SAVE_CUR_FRAME(nextIp) \
	PREPARE_NEW_FRAME(methodInfo, argBasePtr, retPtr, true); \
}


#define CALL_DELEGATE(_resolvedArgIdxs, _del, _managed2NativeStaticCall, _managed2NativeInstanceCall, _ret) \
if (_del->delegates == nullptr) \
{\
    const MethodInfo* method = _del->delegate.method; \
    if (huatuo::metadata::IsInterpreterMethod(method))\
    {\
        if (huatuo::metadata::IsInstanceMethod(method))\
        {\
            Il2CppObject* obj = _del->delegate.target; \
            (localVarBase + _resolvedArgIdxs[0])->obj = obj; \
            method = GET_OBJECT_VIRTUAL_METHOD(obj, method); \
            _managed2NativeInstanceCall(method, _resolvedArgIdxs, localVarBase, _ret); \
        }\
        else\
        {\
            _managed2NativeStaticCall(method, _resolvedArgIdxs + 1, localVarBase, _ret); \
        }\
    }\
    else\
    {\
        CallDelegateMethod(_del->delegate.method, _del->delegate.target, _managed2NativeStaticCall, _managed2NativeInstanceCall, _resolvedArgIdxs, localVarBase, _ret); \
    }\
}\
else\
{\
    Il2CppArray* _dels = _del->delegates; \
    for (il2cpp_array_size_t i = 0; i < _dels->max_length; i++)\
    {\
        Il2CppMulticastDelegate* subDel = il2cpp_array_get(_dels, Il2CppMulticastDelegate*, i); \
        IL2CPP_ASSERT(subDel); \
        IL2CPP_ASSERT(subDel->delegates == nullptr); \
        CallDelegateMethod(subDel->delegate.method, subDel->delegate.target, _managed2NativeStaticCall, _managed2NativeInstanceCall, _resolvedArgIdxs, localVarBase, _ret); \
    }\
}

#pragma endregion

#pragma region exception

#define IN_EXCEPTION_HANDE() (frame->exFlowType == ExceptionFlowType::Exception)

#define PREPARE_EXCEPTION()         frame->throwOffset = (int32_t)(ip - ipBase); \
frame->nextExClauseIndex = 0; \
frame->exFlowType = ExceptionFlowType::Exception; \

#define FIND_NEXT_EX_HANDLER_OR_UNWIND() \
while (true) \
{ \
	IL2CPP_ASSERT(IN_EXCEPTION_HANDE()); \
	IL2CPP_ASSERT(curException); \
	int32_t exClauseNum = (int32_t)imi->exClauses.size(); \
	for (; frame->nextExClauseIndex < exClauseNum; ) \
	{ \
		InterpExceptionClause* iec = imi->exClauses[frame->nextExClauseIndex++]; \
		if (frame->throwOffset < iec->tryBeginOffset) \
		{ \
			break; \
		} \
		if (frame->throwOffset < iec->tryEndOffset) \
		{ \
			switch (iec->flags) \
			{ \
			case CorILExceptionClauseType::Exception: \
			{ \
				if (il2cpp::vm::Class::IsAssignableFrom(iec->exKlass, curException->klass)) \
				{ \
					ip = ipBase + iec->handlerBeginOffset; \
					StackObject* exObj = localVarBase + imi->evalStackBaseOffset; \
					exObj->obj = curException; \
					frame->exFlowType = ExceptionFlowType::None; \
					goto LoopStart; \
				} \
				break; \
			} \
			case CorILExceptionClauseType::Filter: \
			{ \
				ip = ipBase + iec->filterBeginOffset; \
				StackObject* exObj = localVarBase + imi->evalStackBaseOffset; \
				exObj->obj = curException; \
				goto LoopStart; \
			} \
			case CorILExceptionClauseType::Finally: \
			{ \
				ip = ipBase + iec->handlerBeginOffset; \
				goto LoopStart; \
			} \
			case CorILExceptionClauseType::Fault: \
			{ \
				ip = ipBase + iec->handlerBeginOffset; \
				goto LoopStart; \
			} \
			default: \
			{ \
				IL2CPP_ASSERT(false); \
			} \
			} \
		} \
	} \
	frame = interpFrameGroup.LeaveFrame(); \
	if (frame) \
	{ \
		LOAD_PREV_FRAME(); \
		PREPARE_EXCEPTION(); \
	}\
	else \
	{ \
		goto UnWindFail; \
	} \
}


#define CONTINUE_NEXT_FINALLY() \
IL2CPP_ASSERT(frame->exFlowType == ExceptionFlowType::Leave); \
int32_t exClauseNum = (int32_t)imi->exClauses.size(); \
for (; frame->nextExClauseIndex < exClauseNum; ) \
{ \
	InterpExceptionClause* iec = imi->exClauses[frame->nextExClauseIndex++]; \
	if (frame->leaveTarget < iec->tryBeginOffset) \
	{ \
		break; \
	} \
	if (frame->leaveTarget < iec->tryEndOffset) \
	{ \
		switch (iec->flags) \
		{ \
		case CorILExceptionClauseType::Finally: \
		{ \
			ip = ipBase + iec->handlerBeginOffset; \
			goto LoopStart; \
		} \
		case CorILExceptionClauseType::Exception: \
		case CorILExceptionClauseType::Filter: \
		case CorILExceptionClauseType::Fault: \
		{ \
			break; \
		} \
		default: \
		{ \
			IL2CPP_ASSERT(false); \
		} \
		} \
	} \
} \
ip = ipBase + frame->leaveTarget; \
frame->leaveTarget = 0;

#define THROW_EX(ex) { CHECK_NOT_NULL_THROW(ex);   il2cpp::vm::Exception::Raise(ex, const_cast<MethodInfo*>(imi->method));}
#define RETHROW_EX() { il2cpp::vm::Exception::Raise(curException, const_cast<MethodInfo*>(imi->method)); }

#define LEAVE_EX(target)  { \
	frame->leaveTarget = target; \
	curException = nullptr; \
	frame->exFlowType = ExceptionFlowType::Leave;\
	frame->nextExClauseIndex = 0; \
	CONTINUE_NEXT_FINALLY(); \
}

#define ENDFILTER_EX(value) \
if(value) \
{ \
    frame->exFlowType = ExceptionFlowType::None; \
} \
else \
{\
    FIND_NEXT_EX_HANDLER_OR_UNWIND();\
}

#define ENDFINALLY_EX() if (IN_EXCEPTION_HANDE()) \
{ \
    FIND_NEXT_EX_HANDLER_OR_UNWIND(); \
} \
else \
{ \
    CONTINUE_NEXT_FINALLY();\
}

#pragma endregion 


	void Interpreter::Execute(const MethodInfo* methodInfo, StackObject* args, StackObject* ret)
	{
		INIT_CLASS(methodInfo->klass);
		MachineState& machine = InterpreterModule::GetCurrentThreadMachineState();
		InterpFrameGroup interpFrameGroup(machine);

		const InterpMethodInfo* imi;
		InterpFrame* frame;
		StackObject* localVarBase;
		byte* ipBase;
		byte* ip;

		PREPARE_NEW_FRAME(methodInfo, args, ret, false);

		// exception handler
		Il2CppException* curException = nullptr;

	LoopStart:
		try
		{
			for (;;)
			{
				switch (*(HiOpcodeEnum*)ip)
				{
#pragma region memory
					//!!!{{MEMORY
				case HiOpcodeEnum::InitLocals_n_2:
				{
					uint16_t __size = *(uint16_t*)(ip + 2);
					InitDefaultN(localVarBase + imi->localVarBaseOffset, __size);
				    ip += 4;
				    continue;
				}
				case HiOpcodeEnum::InitLocals_n_4:
				{
					uint32_t __size = *(uint32_t*)(ip + 2);
					InitDefaultN(localVarBase + imi->localVarBaseOffset, __size);
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdlocVarVar:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(uint64_t*)(localVarBase + __dst)) = (*(uint64_t*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdlocVarVarSize:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					uint16_t __size = *(uint16_t*)(ip + 6);
					std::memcpy((void*)(localVarBase + __dst), (void*)(localVarBase + __src), __size);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdlocVarAddress:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(void**)(localVarBase + __dst)) = (void*)(localVarBase + __src);
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdcVarConst_1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint8_t __src = *(uint8_t*)(ip + 4);
					uint8_t ____pad__ = *(uint8_t*)(ip + 0);
					(*(int32_t*)(localVarBase + __dst)) = __src;
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdcVarConst_2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = __src;
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdcVarConst_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint32_t __src = *(uint32_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = __src;
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdcVarConst_8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint64_t __src = *(uint64_t*)(ip + 4);
					(*(uint64_t*)(localVarBase + __dst)) = __src;
				    ip += 12;
				    continue;
				}
				case HiOpcodeEnum::LdnullVar:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					(*(void**)(localVarBase + __dst)) = nullptr;
				    ip += 4;
				    continue;
				}
				case HiOpcodeEnum::LdindVarVar_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (*(int8_t*)*(void**)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdindVarVar_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (*(uint8_t*)*(void**)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdindVarVar_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (*(int16_t*)*(void**)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdindVarVar_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (*(uint16_t*)*(void**)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdindVarVar_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (*(int32_t*)*(void**)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdindVarVar_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (*(uint32_t*)*(void**)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdindVarVar_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (*(int64_t*)*(void**)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdindVarVar_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(float*)(localVarBase + __dst)) = (*(float*)*(void**)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdindVarVar_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(double*)(localVarBase + __dst)) = (*(double*)*(void**)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::StindVarVar_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int8_t*)*(void**)(localVarBase + __dst)) = (*(int8_t*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::StindVarVar_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int16_t*)*(void**)(localVarBase + __dst)) = (*(int16_t*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::StindVarVar_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)*(void**)(localVarBase + __dst)) = (*(int32_t*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::StindVarVar_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)*(void**)(localVarBase + __dst)) = (*(int64_t*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::StindVarVar_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(float*)*(void**)(localVarBase + __dst)) = (*(float*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::StindVarVar_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(double*)*(void**)(localVarBase + __dst)) = (*(double*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LocalAllocVarVar_n_2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __size = *(uint16_t*)(ip + 4);
					(*(void**)(localVarBase + __dst)) = LOCAL_ALLOC((*(uint16_t*)(localVarBase + __size)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LocalAllocVarVar_n_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __size = *(uint16_t*)(ip + 4);
					(*(void**)(localVarBase + __dst)) = LOCAL_ALLOC((*(uint32_t*)(localVarBase + __size)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::InitblkVarVarVar:
				{
					uint16_t __addr = *(uint16_t*)(ip + 2);
					uint16_t __value = *(uint16_t*)(ip + 4);
					uint16_t __size = *(uint16_t*)(ip + 6);
					std::memset((*(void**)(localVarBase + __addr)), (*(uint8_t*)(localVarBase + __value)), (*(uint32_t*)(localVarBase + __size)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CpblkVarVar:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					uint16_t __size = *(uint16_t*)(ip + 6);
					std::memcpy((*(void**)(localVarBase + __dst)), (*(void**)(localVarBase + __src)), (*(uint32_t*)(localVarBase + __size)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::MemoryBarrier:
				{
					MEMORY_BARRIER();
				    ip += 2;
				    continue;
				}

				//!!!}}MEMORY


#pragma endregion

#pragma region CONVERT
		//!!!{{CONVERT
				case HiOpcodeEnum::ConvertVarVar_i1_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int8_t)((*(int8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i1_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint8_t)((*(int8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i1_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int16_t)((*(int8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i1_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint16_t)((*(int8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i1_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int32_t)((*(int8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i1_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint32_t)((*(int8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i1_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (int64_t)((*(int8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i1_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (uint64_t)((*(int8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i1_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(float*)(localVarBase + __dst)) = (float)((*(int8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i1_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(double*)(localVarBase + __dst)) = (double)((*(int8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u1_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int8_t)((*(uint8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u1_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint8_t)((*(uint8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u1_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int16_t)((*(uint8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u1_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint16_t)((*(uint8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u1_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int32_t)((*(uint8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u1_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint32_t)((*(uint8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u1_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (int64_t)((*(uint8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u1_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (uint64_t)((*(uint8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u1_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(float*)(localVarBase + __dst)) = (float)((*(uint8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u1_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(double*)(localVarBase + __dst)) = (double)((*(uint8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i2_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int8_t)((*(int16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i2_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint8_t)((*(int16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i2_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int16_t)((*(int16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i2_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint16_t)((*(int16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i2_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int32_t)((*(int16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i2_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint32_t)((*(int16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i2_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (int64_t)((*(int16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i2_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (uint64_t)((*(int16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i2_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(float*)(localVarBase + __dst)) = (float)((*(int16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i2_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(double*)(localVarBase + __dst)) = (double)((*(int16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u2_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int8_t)((*(uint16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u2_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint8_t)((*(uint16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u2_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int16_t)((*(uint16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u2_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint16_t)((*(uint16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u2_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int32_t)((*(uint16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u2_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint32_t)((*(uint16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u2_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (int64_t)((*(uint16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u2_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (uint64_t)((*(uint16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u2_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(float*)(localVarBase + __dst)) = (float)((*(uint16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u2_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(double*)(localVarBase + __dst)) = (double)((*(uint16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i4_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int8_t)((*(int32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i4_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint8_t)((*(int32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i4_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int16_t)((*(int32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i4_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint16_t)((*(int32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i4_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int32_t)((*(int32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i4_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint32_t)((*(int32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i4_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (int64_t)((*(int32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i4_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (uint64_t)((*(int32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i4_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(float*)(localVarBase + __dst)) = (float)((*(int32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i4_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(double*)(localVarBase + __dst)) = (double)((*(int32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u4_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int8_t)((*(uint32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u4_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint8_t)((*(uint32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u4_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int16_t)((*(uint32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u4_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint16_t)((*(uint32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u4_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int32_t)((*(uint32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u4_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint32_t)((*(uint32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u4_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (int64_t)((*(uint32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u4_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (uint64_t)((*(uint32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u4_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(float*)(localVarBase + __dst)) = (float)((*(uint32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u4_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(double*)(localVarBase + __dst)) = (double)((*(uint32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i8_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int8_t)((*(int64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i8_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint8_t)((*(int64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i8_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int16_t)((*(int64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i8_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint16_t)((*(int64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i8_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int32_t)((*(int64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i8_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint32_t)((*(int64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i8_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (int64_t)((*(int64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i8_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (uint64_t)((*(int64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i8_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(float*)(localVarBase + __dst)) = (float)((*(int64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_i8_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(double*)(localVarBase + __dst)) = (double)((*(int64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u8_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int8_t)((*(uint64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u8_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint8_t)((*(uint64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u8_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int16_t)((*(uint64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u8_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint16_t)((*(uint64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u8_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int32_t)((*(uint64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u8_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint32_t)((*(uint64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u8_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (int64_t)((*(uint64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u8_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (uint64_t)((*(uint64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u8_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(float*)(localVarBase + __dst)) = (float)((*(uint64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_u8_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(double*)(localVarBase + __dst)) = (double)((*(uint64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_f4_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int8_t)((*(float*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_f4_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint8_t)((*(float*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_f4_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int16_t)((*(float*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_f4_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint16_t)((*(float*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_f4_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int32_t)((*(float*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_f4_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint32_t)((*(float*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_f4_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (int64_t)((*(float*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_f4_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (uint64_t)((*(float*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_f4_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(float*)(localVarBase + __dst)) = (float)((*(float*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_f4_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(double*)(localVarBase + __dst)) = (double)((*(float*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_f8_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int8_t)((*(double*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_f8_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint8_t)((*(double*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_f8_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int16_t)((*(double*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_f8_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint16_t)((*(double*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_f8_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (int32_t)((*(double*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_f8_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = (uint32_t)((*(double*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_f8_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (int64_t)((*(double*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_f8_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = (uint64_t)((*(double*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_f8_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(float*)(localVarBase + __dst)) = (float)((*(double*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertVarVar_f8_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(double*)(localVarBase + __dst)) = (double)((*(double*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i1_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int8_t)((*(int8_t*)(localVarBase + __src))) < 0)
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int8_t*)(localVarBase + __dst)) = (int8_t)((*(int8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i1_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int8_t)((*(int8_t*)(localVarBase + __src))) < 0)
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint8_t*)(localVarBase + __dst)) = (uint8_t)((*(int8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i1_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int8_t)(int16_t)((*(int8_t*)(localVarBase + __src))) != (*(int8_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int16_t*)(localVarBase + __dst)) = (int16_t)((*(int8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i1_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int8_t)(uint16_t)((*(int8_t*)(localVarBase + __src))) != (*(int8_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint16_t*)(localVarBase + __dst)) = (uint16_t)((*(int8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i1_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int8_t)(int32_t)((*(int8_t*)(localVarBase + __src))) != (*(int8_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int32_t*)(localVarBase + __dst)) = (int32_t)((*(int8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i1_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int8_t)(uint32_t)((*(int8_t*)(localVarBase + __src))) != (*(int8_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint32_t*)(localVarBase + __dst)) = (uint32_t)((*(int8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i1_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int8_t)(int64_t)((*(int8_t*)(localVarBase + __src))) != (*(int8_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int64_t*)(localVarBase + __dst)) = (int64_t)((*(int8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i1_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int8_t)(uint64_t)((*(int8_t*)(localVarBase + __src))) != (*(int8_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint64_t*)(localVarBase + __dst)) = (uint64_t)((*(int8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i1_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int8_t)(float)((*(int8_t*)(localVarBase + __src))) != (*(int8_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(float*)(localVarBase + __dst)) = (float)((*(int8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i1_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int8_t)(double)((*(int8_t*)(localVarBase + __src))) != (*(int8_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(double*)(localVarBase + __dst)) = (double)((*(int8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u1_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int8_t)((*(uint8_t*)(localVarBase + __src))) < 0)
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int8_t*)(localVarBase + __dst)) = (int8_t)((*(uint8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u1_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int8_t)((*(uint8_t*)(localVarBase + __src))) < 0)
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint8_t*)(localVarBase + __dst)) = (uint8_t)((*(uint8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u1_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint8_t)(int16_t)((*(uint8_t*)(localVarBase + __src))) != (*(uint8_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int16_t*)(localVarBase + __dst)) = (int16_t)((*(uint8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u1_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint8_t)(uint16_t)((*(uint8_t*)(localVarBase + __src))) != (*(uint8_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint16_t*)(localVarBase + __dst)) = (uint16_t)((*(uint8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u1_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint8_t)(int32_t)((*(uint8_t*)(localVarBase + __src))) != (*(uint8_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int32_t*)(localVarBase + __dst)) = (int32_t)((*(uint8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u1_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint8_t)(uint32_t)((*(uint8_t*)(localVarBase + __src))) != (*(uint8_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint32_t*)(localVarBase + __dst)) = (uint32_t)((*(uint8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u1_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint8_t)(int64_t)((*(uint8_t*)(localVarBase + __src))) != (*(uint8_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int64_t*)(localVarBase + __dst)) = (int64_t)((*(uint8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u1_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint8_t)(uint64_t)((*(uint8_t*)(localVarBase + __src))) != (*(uint8_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint64_t*)(localVarBase + __dst)) = (uint64_t)((*(uint8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u1_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint8_t)(float)((*(uint8_t*)(localVarBase + __src))) != (*(uint8_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(float*)(localVarBase + __dst)) = (float)((*(uint8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u1_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint8_t)(double)((*(uint8_t*)(localVarBase + __src))) != (*(uint8_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(double*)(localVarBase + __dst)) = (double)((*(uint8_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i2_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int16_t)(int8_t)((*(int16_t*)(localVarBase + __src))) != (*(int16_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int8_t*)(localVarBase + __dst)) = (int8_t)((*(int16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i2_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int16_t)(uint8_t)((*(int16_t*)(localVarBase + __src))) != (*(int16_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint8_t*)(localVarBase + __dst)) = (uint8_t)((*(int16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i2_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int16_t)((*(int16_t*)(localVarBase + __src))) < 0)
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int16_t*)(localVarBase + __dst)) = (int16_t)((*(int16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i2_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int16_t)((*(int16_t*)(localVarBase + __src))) < 0)
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint16_t*)(localVarBase + __dst)) = (uint16_t)((*(int16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i2_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int16_t)(int32_t)((*(int16_t*)(localVarBase + __src))) != (*(int16_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int32_t*)(localVarBase + __dst)) = (int32_t)((*(int16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i2_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int16_t)(uint32_t)((*(int16_t*)(localVarBase + __src))) != (*(int16_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint32_t*)(localVarBase + __dst)) = (uint32_t)((*(int16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i2_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int16_t)(int64_t)((*(int16_t*)(localVarBase + __src))) != (*(int16_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int64_t*)(localVarBase + __dst)) = (int64_t)((*(int16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i2_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int16_t)(uint64_t)((*(int16_t*)(localVarBase + __src))) != (*(int16_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint64_t*)(localVarBase + __dst)) = (uint64_t)((*(int16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i2_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int16_t)(float)((*(int16_t*)(localVarBase + __src))) != (*(int16_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(float*)(localVarBase + __dst)) = (float)((*(int16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i2_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int16_t)(double)((*(int16_t*)(localVarBase + __src))) != (*(int16_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(double*)(localVarBase + __dst)) = (double)((*(int16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u2_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint16_t)(int8_t)((*(uint16_t*)(localVarBase + __src))) != (*(uint16_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int8_t*)(localVarBase + __dst)) = (int8_t)((*(uint16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u2_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint16_t)(uint8_t)((*(uint16_t*)(localVarBase + __src))) != (*(uint16_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint8_t*)(localVarBase + __dst)) = (uint8_t)((*(uint16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u2_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int16_t)((*(uint16_t*)(localVarBase + __src))) < 0)
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int16_t*)(localVarBase + __dst)) = (int16_t)((*(uint16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u2_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int16_t)((*(uint16_t*)(localVarBase + __src))) < 0)
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint16_t*)(localVarBase + __dst)) = (uint16_t)((*(uint16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u2_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint16_t)(int32_t)((*(uint16_t*)(localVarBase + __src))) != (*(uint16_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int32_t*)(localVarBase + __dst)) = (int32_t)((*(uint16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u2_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint16_t)(uint32_t)((*(uint16_t*)(localVarBase + __src))) != (*(uint16_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint32_t*)(localVarBase + __dst)) = (uint32_t)((*(uint16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u2_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint16_t)(int64_t)((*(uint16_t*)(localVarBase + __src))) != (*(uint16_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int64_t*)(localVarBase + __dst)) = (int64_t)((*(uint16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u2_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint16_t)(uint64_t)((*(uint16_t*)(localVarBase + __src))) != (*(uint16_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint64_t*)(localVarBase + __dst)) = (uint64_t)((*(uint16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u2_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint16_t)(float)((*(uint16_t*)(localVarBase + __src))) != (*(uint16_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(float*)(localVarBase + __dst)) = (float)((*(uint16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u2_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint16_t)(double)((*(uint16_t*)(localVarBase + __src))) != (*(uint16_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(double*)(localVarBase + __dst)) = (double)((*(uint16_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i4_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int32_t)(int8_t)((*(int32_t*)(localVarBase + __src))) != (*(int32_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int8_t*)(localVarBase + __dst)) = (int8_t)((*(int32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i4_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int32_t)(uint8_t)((*(int32_t*)(localVarBase + __src))) != (*(int32_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint8_t*)(localVarBase + __dst)) = (uint8_t)((*(int32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i4_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int32_t)(int16_t)((*(int32_t*)(localVarBase + __src))) != (*(int32_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int16_t*)(localVarBase + __dst)) = (int16_t)((*(int32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i4_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int32_t)(uint16_t)((*(int32_t*)(localVarBase + __src))) != (*(int32_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint16_t*)(localVarBase + __dst)) = (uint16_t)((*(int32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i4_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int32_t)((*(int32_t*)(localVarBase + __src))) < 0)
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int32_t*)(localVarBase + __dst)) = (int32_t)((*(int32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i4_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int32_t)((*(int32_t*)(localVarBase + __src))) < 0)
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint32_t*)(localVarBase + __dst)) = (uint32_t)((*(int32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i4_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int32_t)(int64_t)((*(int32_t*)(localVarBase + __src))) != (*(int32_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int64_t*)(localVarBase + __dst)) = (int64_t)((*(int32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i4_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int32_t)(uint64_t)((*(int32_t*)(localVarBase + __src))) != (*(int32_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint64_t*)(localVarBase + __dst)) = (uint64_t)((*(int32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i4_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int32_t)(float)((*(int32_t*)(localVarBase + __src))) != (*(int32_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(float*)(localVarBase + __dst)) = (float)((*(int32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i4_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int32_t)(double)((*(int32_t*)(localVarBase + __src))) != (*(int32_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(double*)(localVarBase + __dst)) = (double)((*(int32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u4_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint32_t)(int8_t)((*(uint32_t*)(localVarBase + __src))) != (*(uint32_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int8_t*)(localVarBase + __dst)) = (int8_t)((*(uint32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u4_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint32_t)(uint8_t)((*(uint32_t*)(localVarBase + __src))) != (*(uint32_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint8_t*)(localVarBase + __dst)) = (uint8_t)((*(uint32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u4_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint32_t)(int16_t)((*(uint32_t*)(localVarBase + __src))) != (*(uint32_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int16_t*)(localVarBase + __dst)) = (int16_t)((*(uint32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u4_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint32_t)(uint16_t)((*(uint32_t*)(localVarBase + __src))) != (*(uint32_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint16_t*)(localVarBase + __dst)) = (uint16_t)((*(uint32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u4_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int32_t)((*(uint32_t*)(localVarBase + __src))) < 0)
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int32_t*)(localVarBase + __dst)) = (int32_t)((*(uint32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u4_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int32_t)((*(uint32_t*)(localVarBase + __src))) < 0)
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint32_t*)(localVarBase + __dst)) = (uint32_t)((*(uint32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u4_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint32_t)(int64_t)((*(uint32_t*)(localVarBase + __src))) != (*(uint32_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int64_t*)(localVarBase + __dst)) = (int64_t)((*(uint32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u4_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint32_t)(uint64_t)((*(uint32_t*)(localVarBase + __src))) != (*(uint32_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint64_t*)(localVarBase + __dst)) = (uint64_t)((*(uint32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u4_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint32_t)(float)((*(uint32_t*)(localVarBase + __src))) != (*(uint32_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(float*)(localVarBase + __dst)) = (float)((*(uint32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u4_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint32_t)(double)((*(uint32_t*)(localVarBase + __src))) != (*(uint32_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(double*)(localVarBase + __dst)) = (double)((*(uint32_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i8_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int64_t)(int8_t)((*(int64_t*)(localVarBase + __src))) != (*(int64_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int8_t*)(localVarBase + __dst)) = (int8_t)((*(int64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i8_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int64_t)(uint8_t)((*(int64_t*)(localVarBase + __src))) != (*(int64_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint8_t*)(localVarBase + __dst)) = (uint8_t)((*(int64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i8_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int64_t)(int16_t)((*(int64_t*)(localVarBase + __src))) != (*(int64_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int16_t*)(localVarBase + __dst)) = (int16_t)((*(int64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i8_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int64_t)(uint16_t)((*(int64_t*)(localVarBase + __src))) != (*(int64_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint16_t*)(localVarBase + __dst)) = (uint16_t)((*(int64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i8_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int64_t)(int32_t)((*(int64_t*)(localVarBase + __src))) != (*(int64_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int32_t*)(localVarBase + __dst)) = (int32_t)((*(int64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i8_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int64_t)(uint32_t)((*(int64_t*)(localVarBase + __src))) != (*(int64_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint32_t*)(localVarBase + __dst)) = (uint32_t)((*(int64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i8_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int64_t)((*(int64_t*)(localVarBase + __src))) < 0)
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int64_t*)(localVarBase + __dst)) = (int64_t)((*(int64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i8_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int64_t)((*(int64_t*)(localVarBase + __src))) < 0)
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint64_t*)(localVarBase + __dst)) = (uint64_t)((*(int64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i8_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int64_t)(float)((*(int64_t*)(localVarBase + __src))) != (*(int64_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(float*)(localVarBase + __dst)) = (float)((*(int64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_i8_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int64_t)(double)((*(int64_t*)(localVarBase + __src))) != (*(int64_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(double*)(localVarBase + __dst)) = (double)((*(int64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u8_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint64_t)(int8_t)((*(uint64_t*)(localVarBase + __src))) != (*(uint64_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int8_t*)(localVarBase + __dst)) = (int8_t)((*(uint64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u8_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint64_t)(uint8_t)((*(uint64_t*)(localVarBase + __src))) != (*(uint64_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint8_t*)(localVarBase + __dst)) = (uint8_t)((*(uint64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u8_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint64_t)(int16_t)((*(uint64_t*)(localVarBase + __src))) != (*(uint64_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int16_t*)(localVarBase + __dst)) = (int16_t)((*(uint64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u8_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint64_t)(uint16_t)((*(uint64_t*)(localVarBase + __src))) != (*(uint64_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint16_t*)(localVarBase + __dst)) = (uint16_t)((*(uint64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u8_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint64_t)(int32_t)((*(uint64_t*)(localVarBase + __src))) != (*(uint64_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int32_t*)(localVarBase + __dst)) = (int32_t)((*(uint64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u8_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint64_t)(uint32_t)((*(uint64_t*)(localVarBase + __src))) != (*(uint64_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint32_t*)(localVarBase + __dst)) = (uint32_t)((*(uint64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u8_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int64_t)((*(uint64_t*)(localVarBase + __src))) < 0)
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int64_t*)(localVarBase + __dst)) = (int64_t)((*(uint64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u8_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int64_t)((*(uint64_t*)(localVarBase + __src))) < 0)
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint64_t*)(localVarBase + __dst)) = (uint64_t)((*(uint64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u8_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint64_t)(float)((*(uint64_t*)(localVarBase + __src))) != (*(uint64_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(float*)(localVarBase + __dst)) = (float)((*(uint64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_u8_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((uint64_t)(double)((*(uint64_t*)(localVarBase + __src))) != (*(uint64_t*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(double*)(localVarBase + __dst)) = (double)((*(uint64_t*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_f4_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((float)(int8_t)((*(float*)(localVarBase + __src))) != (*(float*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int8_t*)(localVarBase + __dst)) = (int8_t)((*(float*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_f4_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((float)(uint8_t)((*(float*)(localVarBase + __src))) != (*(float*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint8_t*)(localVarBase + __dst)) = (uint8_t)((*(float*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_f4_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((float)(int16_t)((*(float*)(localVarBase + __src))) != (*(float*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int16_t*)(localVarBase + __dst)) = (int16_t)((*(float*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_f4_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((float)(uint16_t)((*(float*)(localVarBase + __src))) != (*(float*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint16_t*)(localVarBase + __dst)) = (uint16_t)((*(float*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_f4_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((float)(int32_t)((*(float*)(localVarBase + __src))) != (*(float*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int32_t*)(localVarBase + __dst)) = (int32_t)((*(float*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_f4_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((float)(uint32_t)((*(float*)(localVarBase + __src))) != (*(float*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint32_t*)(localVarBase + __dst)) = (uint32_t)((*(float*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_f4_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((float)(int64_t)((*(float*)(localVarBase + __src))) != (*(float*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int64_t*)(localVarBase + __dst)) = (int64_t)((*(float*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_f4_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((float)(uint64_t)((*(float*)(localVarBase + __src))) != (*(float*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint64_t*)(localVarBase + __dst)) = (uint64_t)((*(float*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_f4_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int32_t)((*(float*)(localVarBase + __src))) < 0)
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(float*)(localVarBase + __dst)) = (float)((*(float*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_f4_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((float)(double)((*(float*)(localVarBase + __src))) != (*(float*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(double*)(localVarBase + __dst)) = (double)((*(float*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_f8_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((double)(int8_t)((*(double*)(localVarBase + __src))) != (*(double*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int8_t*)(localVarBase + __dst)) = (int8_t)((*(double*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_f8_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((double)(uint8_t)((*(double*)(localVarBase + __src))) != (*(double*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint8_t*)(localVarBase + __dst)) = (uint8_t)((*(double*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_f8_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((double)(int16_t)((*(double*)(localVarBase + __src))) != (*(double*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int16_t*)(localVarBase + __dst)) = (int16_t)((*(double*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_f8_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((double)(uint16_t)((*(double*)(localVarBase + __src))) != (*(double*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint16_t*)(localVarBase + __dst)) = (uint16_t)((*(double*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_f8_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((double)(int32_t)((*(double*)(localVarBase + __src))) != (*(double*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int32_t*)(localVarBase + __dst)) = (int32_t)((*(double*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_f8_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((double)(uint32_t)((*(double*)(localVarBase + __src))) != (*(double*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint32_t*)(localVarBase + __dst)) = (uint32_t)((*(double*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_f8_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((double)(int64_t)((*(double*)(localVarBase + __src))) != (*(double*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(int64_t*)(localVarBase + __dst)) = (int64_t)((*(double*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_f8_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((double)(uint64_t)((*(double*)(localVarBase + __src))) != (*(double*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(uint64_t*)(localVarBase + __dst)) = (uint64_t)((*(double*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_f8_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((double)(float)((*(double*)(localVarBase + __src))) != (*(double*)(localVarBase + __src)))
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(float*)(localVarBase + __dst)) = (float)((*(double*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::ConvertOverflowVarVar_f8_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
				    if ((int64_t)((*(double*)(localVarBase + __src))) < 0)
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    (*(double*)(localVarBase + __dst)) = (double)((*(double*)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}

				//!!!}}CONVERT
#pragma endregion

#pragma region ARITH
		//!!!{{ARITH
				case HiOpcodeEnum::BinOpVarVarVar_Add_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = (*(int32_t*)(localVarBase + __op1)) + (*(int32_t*)(localVarBase + __op2));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Sub_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = (*(int32_t*)(localVarBase + __op1)) - (*(int32_t*)(localVarBase + __op2));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Mul_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = (*(int32_t*)(localVarBase + __op1)) * (*(int32_t*)(localVarBase + __op2));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_MulUn_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = HiMulUn((*(int32_t*)(localVarBase + __op1)), (*(int32_t*)(localVarBase + __op2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Div_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = HiDiv((*(int32_t*)(localVarBase + __op1)), (*(int32_t*)(localVarBase + __op2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_DivUn_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = HiDivUn((*(int32_t*)(localVarBase + __op1)), (*(int32_t*)(localVarBase + __op2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Rem_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = HiRem((*(int32_t*)(localVarBase + __op1)), (*(int32_t*)(localVarBase + __op2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_RemUn_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = HiRemUn((*(int32_t*)(localVarBase + __op1)), (*(int32_t*)(localVarBase + __op2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_And_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = (*(int32_t*)(localVarBase + __op1)) & (*(int32_t*)(localVarBase + __op2));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Or_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = (*(int32_t*)(localVarBase + __op1)) | (*(int32_t*)(localVarBase + __op2));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Xor_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = (*(int32_t*)(localVarBase + __op1)) ^ (*(int32_t*)(localVarBase + __op2));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Add_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int64_t*)(localVarBase + __ret)) = (*(int64_t*)(localVarBase + __op1)) + (*(int64_t*)(localVarBase + __op2));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Sub_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int64_t*)(localVarBase + __ret)) = (*(int64_t*)(localVarBase + __op1)) - (*(int64_t*)(localVarBase + __op2));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Mul_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int64_t*)(localVarBase + __ret)) = (*(int64_t*)(localVarBase + __op1)) * (*(int64_t*)(localVarBase + __op2));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_MulUn_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int64_t*)(localVarBase + __ret)) = HiMulUn((*(int64_t*)(localVarBase + __op1)), (*(int64_t*)(localVarBase + __op2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Div_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int64_t*)(localVarBase + __ret)) = HiDiv((*(int64_t*)(localVarBase + __op1)), (*(int64_t*)(localVarBase + __op2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_DivUn_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int64_t*)(localVarBase + __ret)) = HiDivUn((*(int64_t*)(localVarBase + __op1)), (*(int64_t*)(localVarBase + __op2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Rem_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int64_t*)(localVarBase + __ret)) = HiRem((*(int64_t*)(localVarBase + __op1)), (*(int64_t*)(localVarBase + __op2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_RemUn_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int64_t*)(localVarBase + __ret)) = HiRemUn((*(int64_t*)(localVarBase + __op1)), (*(int64_t*)(localVarBase + __op2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_And_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int64_t*)(localVarBase + __ret)) = (*(int64_t*)(localVarBase + __op1)) & (*(int64_t*)(localVarBase + __op2));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Or_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int64_t*)(localVarBase + __ret)) = (*(int64_t*)(localVarBase + __op1)) | (*(int64_t*)(localVarBase + __op2));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Xor_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(int64_t*)(localVarBase + __ret)) = (*(int64_t*)(localVarBase + __op1)) ^ (*(int64_t*)(localVarBase + __op2));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Add_f4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(float*)(localVarBase + __ret)) = (*(float*)(localVarBase + __op1)) + (*(float*)(localVarBase + __op2));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Sub_f4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(float*)(localVarBase + __ret)) = (*(float*)(localVarBase + __op1)) - (*(float*)(localVarBase + __op2));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Mul_f4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(float*)(localVarBase + __ret)) = (*(float*)(localVarBase + __op1)) * (*(float*)(localVarBase + __op2));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Div_f4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(float*)(localVarBase + __ret)) = HiDiv((*(float*)(localVarBase + __op1)), (*(float*)(localVarBase + __op2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Rem_f4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(float*)(localVarBase + __ret)) = HiRem((*(float*)(localVarBase + __op1)), (*(float*)(localVarBase + __op2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Add_f8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(double*)(localVarBase + __ret)) = (*(double*)(localVarBase + __op1)) + (*(double*)(localVarBase + __op2));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Sub_f8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(double*)(localVarBase + __ret)) = (*(double*)(localVarBase + __op1)) - (*(double*)(localVarBase + __op2));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Mul_f8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(double*)(localVarBase + __ret)) = (*(double*)(localVarBase + __op1)) * (*(double*)(localVarBase + __op2));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Div_f8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(double*)(localVarBase + __ret)) = HiDiv((*(double*)(localVarBase + __op1)), (*(double*)(localVarBase + __op2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpVarVarVar_Rem_f8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
					(*(double*)(localVarBase + __ret)) = HiRem((*(double*)(localVarBase + __op1)), (*(double*)(localVarBase + __op2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpOverflowVarVarVar_Add_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
				    int32_t op1 = (*(int32_t*)(localVarBase + __op1));
				    int32_t op2 = (*(int32_t*)(localVarBase + __op2));
				    if ((CHECK_ADD_OVERFLOW(op1, op2)) == 0)
				    {
				        (*(int32_t*)(localVarBase + __ret)) = __op1 + __op2;
				    }
				    else
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpOverflowVarVarVar_Sub_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
				    int32_t op1 = (*(int32_t*)(localVarBase + __op1));
				    int32_t op2 = (*(int32_t*)(localVarBase + __op2));
				    if ((CHECK_SUB_OVERFLOW(op1, op2)) == 0)
				    {
				        (*(int32_t*)(localVarBase + __ret)) = __op1 - __op2;
				    }
				    else
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpOverflowVarVarVar_Mul_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
				    int32_t op1 = (*(int32_t*)(localVarBase + __op1));
				    int32_t op2 = (*(int32_t*)(localVarBase + __op2));
				    if ((CHECK_MUL_OVERFLOW(op1, op2)) == 0)
				    {
				        (*(int32_t*)(localVarBase + __ret)) = __op1 * __op2;
				    }
				    else
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpOverflowVarVarVar_Add_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
				    int64_t op1 = (*(int64_t*)(localVarBase + __op1));
				    int64_t op2 = (*(int64_t*)(localVarBase + __op2));
				    if ((CHECK_ADD_OVERFLOW64(op1, op2)) == 0)
				    {
				        (*(int64_t*)(localVarBase + __ret)) = __op1 + __op2;
				    }
				    else
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpOverflowVarVarVar_Sub_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
				    int64_t op1 = (*(int64_t*)(localVarBase + __op1));
				    int64_t op2 = (*(int64_t*)(localVarBase + __op2));
				    if ((CHECK_SUB_OVERFLOW64(op1, op2)) == 0)
				    {
				        (*(int64_t*)(localVarBase + __ret)) = __op1 - __op2;
				    }
				    else
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpOverflowVarVarVar_Mul_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
				    int64_t op1 = (*(int64_t*)(localVarBase + __op1));
				    int64_t op2 = (*(int64_t*)(localVarBase + __op2));
				    if ((CHECK_MUL_OVERFLOW64(op1, op2)) == 0)
				    {
				        (*(int64_t*)(localVarBase + __ret)) = __op1 * __op2;
				    }
				    else
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpOverflowVarVarVar_Add_u4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
				    uint32_t op1 = (*(uint32_t*)(localVarBase + __op1));
				    uint32_t op2 = (*(uint32_t*)(localVarBase + __op2));
				    if ((CHECK_ADD_OVERFLOW_UN(op1, op2)) == 0)
				    {
				        (*(uint32_t*)(localVarBase + __ret)) = __op1 + __op2;
				    }
				    else
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpOverflowVarVarVar_Sub_u4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
				    uint32_t op1 = (*(uint32_t*)(localVarBase + __op1));
				    uint32_t op2 = (*(uint32_t*)(localVarBase + __op2));
				    if ((CHECK_SUB_OVERFLOW_UN(op1, op2)) == 0)
				    {
				        (*(uint32_t*)(localVarBase + __ret)) = __op1 - __op2;
				    }
				    else
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpOverflowVarVarVar_Mul_u4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
				    uint32_t op1 = (*(uint32_t*)(localVarBase + __op1));
				    uint32_t op2 = (*(uint32_t*)(localVarBase + __op2));
				    if ((CHECK_MUL_OVERFLOW_UN(op1, op2)) == 0)
				    {
				        (*(uint32_t*)(localVarBase + __ret)) = __op1 * __op2;
				    }
				    else
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpOverflowVarVarVar_Add_u8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
				    uint64_t op1 = (*(uint64_t*)(localVarBase + __op1));
				    uint64_t op2 = (*(uint64_t*)(localVarBase + __op2));
				    if ((CHECK_ADD_OVERFLOW64_UN(op1, op2)) == 0)
				    {
				        (*(uint64_t*)(localVarBase + __ret)) = __op1 + __op2;
				    }
				    else
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpOverflowVarVarVar_Sub_u8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
				    uint64_t op1 = (*(uint64_t*)(localVarBase + __op1));
				    uint64_t op2 = (*(uint64_t*)(localVarBase + __op2));
				    if ((CHECK_SUB_OVERFLOW64_UN(op1, op2)) == 0)
				    {
				        (*(uint64_t*)(localVarBase + __ret)) = __op1 - __op2;
				    }
				    else
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BinOpOverflowVarVarVar_Mul_u8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __op1 = *(uint16_t*)(ip + 4);
					uint16_t __op2 = *(uint16_t*)(ip + 6);
				    uint64_t op1 = (*(uint64_t*)(localVarBase + __op1));
				    uint64_t op2 = (*(uint64_t*)(localVarBase + __op2));
				    if ((CHECK_MUL_OVERFLOW64_UN(op1, op2)) == 0)
				    {
				        (*(uint64_t*)(localVarBase + __ret)) = __op1 * __op2;
				    }
				    else
				    {
				        il2cpp::vm::Exception::RaiseOverflowException();
				    }
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BitShiftBinOpVarVarVar_Shl_i4_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __value = *(uint16_t*)(ip + 4);
					uint16_t __shiftAmount = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = (*(int32_t*)(localVarBase + __value)) << (*(int32_t*)(localVarBase + __shiftAmount));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BitShiftBinOpVarVarVar_Shr_i4_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __value = *(uint16_t*)(ip + 4);
					uint16_t __shiftAmount = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = (*(int32_t*)(localVarBase + __value)) >> (*(int32_t*)(localVarBase + __shiftAmount));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BitShiftBinOpVarVarVar_ShrUn_i4_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __value = *(uint16_t*)(ip + 4);
					uint16_t __shiftAmount = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = HiShrUn((*(int32_t*)(localVarBase + __value)), (*(int32_t*)(localVarBase + __shiftAmount)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BitShiftBinOpVarVarVar_Shl_i4_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __value = *(uint16_t*)(ip + 4);
					uint16_t __shiftAmount = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = (*(int32_t*)(localVarBase + __value)) << (*(int64_t*)(localVarBase + __shiftAmount));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BitShiftBinOpVarVarVar_Shr_i4_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __value = *(uint16_t*)(ip + 4);
					uint16_t __shiftAmount = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = (*(int32_t*)(localVarBase + __value)) >> (*(int64_t*)(localVarBase + __shiftAmount));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BitShiftBinOpVarVarVar_ShrUn_i4_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __value = *(uint16_t*)(ip + 4);
					uint16_t __shiftAmount = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = HiShrUn((*(int32_t*)(localVarBase + __value)), (*(int64_t*)(localVarBase + __shiftAmount)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BitShiftBinOpVarVarVar_Shl_i8_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __value = *(uint16_t*)(ip + 4);
					uint16_t __shiftAmount = *(uint16_t*)(ip + 6);
					(*(int64_t*)(localVarBase + __ret)) = (*(int64_t*)(localVarBase + __value)) << (*(int32_t*)(localVarBase + __shiftAmount));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BitShiftBinOpVarVarVar_Shr_i8_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __value = *(uint16_t*)(ip + 4);
					uint16_t __shiftAmount = *(uint16_t*)(ip + 6);
					(*(int64_t*)(localVarBase + __ret)) = (*(int64_t*)(localVarBase + __value)) >> (*(int32_t*)(localVarBase + __shiftAmount));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BitShiftBinOpVarVarVar_ShrUn_i8_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __value = *(uint16_t*)(ip + 4);
					uint16_t __shiftAmount = *(uint16_t*)(ip + 6);
					(*(int64_t*)(localVarBase + __ret)) = HiShrUn((*(int64_t*)(localVarBase + __value)), (*(int32_t*)(localVarBase + __shiftAmount)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BitShiftBinOpVarVarVar_Shl_i8_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __value = *(uint16_t*)(ip + 4);
					uint16_t __shiftAmount = *(uint16_t*)(ip + 6);
					(*(int64_t*)(localVarBase + __ret)) = (*(int64_t*)(localVarBase + __value)) << (*(int64_t*)(localVarBase + __shiftAmount));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BitShiftBinOpVarVarVar_Shr_i8_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __value = *(uint16_t*)(ip + 4);
					uint16_t __shiftAmount = *(uint16_t*)(ip + 6);
					(*(int64_t*)(localVarBase + __ret)) = (*(int64_t*)(localVarBase + __value)) >> (*(int64_t*)(localVarBase + __shiftAmount));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::BitShiftBinOpVarVarVar_ShrUn_i8_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __value = *(uint16_t*)(ip + 4);
					uint16_t __shiftAmount = *(uint16_t*)(ip + 6);
					(*(int64_t*)(localVarBase + __ret)) = HiShrUn((*(int64_t*)(localVarBase + __value)), (*(int64_t*)(localVarBase + __shiftAmount)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::UnaryOpVarVar_Neg_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = - (*(int32_t*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::UnaryOpVarVar_Not_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int32_t*)(localVarBase + __dst)) = ~ (*(int32_t*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::UnaryOpVarVar_Neg_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = - (*(int64_t*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::UnaryOpVarVar_Not_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(int64_t*)(localVarBase + __dst)) = ~ (*(int64_t*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::UnaryOpVarVar_Neg_f4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(float*)(localVarBase + __dst)) = - (*(float*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::UnaryOpVarVar_Neg_f8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					(*(double*)(localVarBase + __dst)) = - (*(double*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::CheckFiniteVar_f4:
				{
					uint16_t __src = *(uint16_t*)(ip + 2);
					HiCheckFinite((*(float*)(localVarBase + __src)));
				    ip += 4;
				    continue;
				}
				case HiOpcodeEnum::CheckFiniteVar_f8:
				{
					uint16_t __src = *(uint16_t*)(ip + 2);
					HiCheckFinite((*(double*)(localVarBase + __src)));
				    ip += 4;
				    continue;
				}

				//!!!}}ARITH
#pragma endregion

#pragma region COMPARE
		//!!!{{COMPARE
				case HiOpcodeEnum::CompOpVarVarVar_Ceq_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __c1 = *(uint16_t*)(ip + 4);
					uint16_t __c2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = CompareCeq((*(int32_t*)(localVarBase + __c1)), (*(int32_t*)(localVarBase + __c2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CompOpVarVarVar_Ceq_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __c1 = *(uint16_t*)(ip + 4);
					uint16_t __c2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = CompareCeq((*(int64_t*)(localVarBase + __c1)), (*(int64_t*)(localVarBase + __c2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CompOpVarVarVar_Ceq_f4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __c1 = *(uint16_t*)(ip + 4);
					uint16_t __c2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = CompareCeq((*(float*)(localVarBase + __c1)), (*(float*)(localVarBase + __c2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CompOpVarVarVar_Ceq_f8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __c1 = *(uint16_t*)(ip + 4);
					uint16_t __c2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = CompareCeq((*(double*)(localVarBase + __c1)), (*(double*)(localVarBase + __c2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CompOpVarVarVar_Cgt_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __c1 = *(uint16_t*)(ip + 4);
					uint16_t __c2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = CompareCgt((*(int32_t*)(localVarBase + __c1)), (*(int32_t*)(localVarBase + __c2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CompOpVarVarVar_Cgt_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __c1 = *(uint16_t*)(ip + 4);
					uint16_t __c2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = CompareCgt((*(int64_t*)(localVarBase + __c1)), (*(int64_t*)(localVarBase + __c2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CompOpVarVarVar_Cgt_f4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __c1 = *(uint16_t*)(ip + 4);
					uint16_t __c2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = CompareCgt((*(float*)(localVarBase + __c1)), (*(float*)(localVarBase + __c2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CompOpVarVarVar_Cgt_f8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __c1 = *(uint16_t*)(ip + 4);
					uint16_t __c2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = CompareCgt((*(double*)(localVarBase + __c1)), (*(double*)(localVarBase + __c2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CompOpVarVarVar_CgtUn_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __c1 = *(uint16_t*)(ip + 4);
					uint16_t __c2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = CompareCgtUn((*(int32_t*)(localVarBase + __c1)), (*(int32_t*)(localVarBase + __c2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CompOpVarVarVar_CgtUn_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __c1 = *(uint16_t*)(ip + 4);
					uint16_t __c2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = CompareCgtUn((*(int64_t*)(localVarBase + __c1)), (*(int64_t*)(localVarBase + __c2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CompOpVarVarVar_CgtUn_f4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __c1 = *(uint16_t*)(ip + 4);
					uint16_t __c2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = CompareCgtUn((*(float*)(localVarBase + __c1)), (*(float*)(localVarBase + __c2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CompOpVarVarVar_CgtUn_f8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __c1 = *(uint16_t*)(ip + 4);
					uint16_t __c2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = CompareCgtUn((*(double*)(localVarBase + __c1)), (*(double*)(localVarBase + __c2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CompOpVarVarVar_Clt_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __c1 = *(uint16_t*)(ip + 4);
					uint16_t __c2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = CompareClt((*(int32_t*)(localVarBase + __c1)), (*(int32_t*)(localVarBase + __c2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CompOpVarVarVar_Clt_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __c1 = *(uint16_t*)(ip + 4);
					uint16_t __c2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = CompareClt((*(int64_t*)(localVarBase + __c1)), (*(int64_t*)(localVarBase + __c2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CompOpVarVarVar_Clt_f4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __c1 = *(uint16_t*)(ip + 4);
					uint16_t __c2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = CompareClt((*(float*)(localVarBase + __c1)), (*(float*)(localVarBase + __c2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CompOpVarVarVar_Clt_f8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __c1 = *(uint16_t*)(ip + 4);
					uint16_t __c2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = CompareClt((*(double*)(localVarBase + __c1)), (*(double*)(localVarBase + __c2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CompOpVarVarVar_CltUn_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __c1 = *(uint16_t*)(ip + 4);
					uint16_t __c2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = CompareCltUn((*(int32_t*)(localVarBase + __c1)), (*(int32_t*)(localVarBase + __c2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CompOpVarVarVar_CltUn_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __c1 = *(uint16_t*)(ip + 4);
					uint16_t __c2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = CompareCltUn((*(int64_t*)(localVarBase + __c1)), (*(int64_t*)(localVarBase + __c2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CompOpVarVarVar_CltUn_f4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __c1 = *(uint16_t*)(ip + 4);
					uint16_t __c2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = CompareCltUn((*(float*)(localVarBase + __c1)), (*(float*)(localVarBase + __c2)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CompOpVarVarVar_CltUn_f8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __c1 = *(uint16_t*)(ip + 4);
					uint16_t __c2 = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __ret)) = CompareCltUn((*(double*)(localVarBase + __c1)), (*(double*)(localVarBase + __c2)));
				    ip += 8;
				    continue;
				}

				//!!!}}COMPARE
#pragma endregion

#pragma region BRANCH
		//!!!{{BRANCH
				case HiOpcodeEnum::BranchUncondition_4:
				{
					int32_t __offset = *(int32_t*)(ip + 2);
					ip = ipBase + __offset;
				    continue;
				}
				case HiOpcodeEnum::BranchTrueVar_i4:
				{
					uint16_t __op = *(uint16_t*)(ip + 2);
					int32_t __offset = *(int32_t*)(ip + 4);
				    if ((*(int32_t*)(localVarBase + __op)))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 8;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchTrueVar_i8:
				{
					uint16_t __op = *(uint16_t*)(ip + 2);
					int32_t __offset = *(int32_t*)(ip + 4);
				    if ((*(int64_t*)(localVarBase + __op)))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 8;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchFalseVar_i4:
				{
					uint16_t __op = *(uint16_t*)(ip + 2);
					int32_t __offset = *(int32_t*)(ip + 4);
				    if (!(*(int32_t*)(localVarBase + __op)))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 8;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchFalseVar_i8:
				{
					uint16_t __op = *(uint16_t*)(ip + 2);
					int32_t __offset = *(int32_t*)(ip + 4);
				    if (!(*(int64_t*)(localVarBase + __op)))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 8;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_Ceq_i4:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCeq((*(int32_t*)(localVarBase + __op1)), (*(int32_t*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_Ceq_i8:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCeq((*(int64_t*)(localVarBase + __op1)), (*(int64_t*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_Ceq_f4:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCeq((*(float*)(localVarBase + __op1)), (*(float*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_Ceq_f8:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCeq((*(double*)(localVarBase + __op1)), (*(double*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_CneUn_i4:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCneUn((*(int32_t*)(localVarBase + __op1)), (*(int32_t*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_CneUn_i8:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCneUn((*(int64_t*)(localVarBase + __op1)), (*(int64_t*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_CneUn_f4:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCneUn((*(float*)(localVarBase + __op1)), (*(float*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_CneUn_f8:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCneUn((*(double*)(localVarBase + __op1)), (*(double*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_Cgt_i4:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCgt((*(int32_t*)(localVarBase + __op1)), (*(int32_t*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_Cgt_i8:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCgt((*(int64_t*)(localVarBase + __op1)), (*(int64_t*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_Cgt_f4:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCgt((*(float*)(localVarBase + __op1)), (*(float*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_Cgt_f8:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCgt((*(double*)(localVarBase + __op1)), (*(double*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_CgtUn_i4:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCgtUn((*(int32_t*)(localVarBase + __op1)), (*(int32_t*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_CgtUn_i8:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCgtUn((*(int64_t*)(localVarBase + __op1)), (*(int64_t*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_CgtUn_f4:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCgtUn((*(float*)(localVarBase + __op1)), (*(float*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_CgtUn_f8:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCgtUn((*(double*)(localVarBase + __op1)), (*(double*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_Cge_i4:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCge((*(int32_t*)(localVarBase + __op1)), (*(int32_t*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_Cge_i8:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCge((*(int64_t*)(localVarBase + __op1)), (*(int64_t*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_Cge_f4:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCge((*(float*)(localVarBase + __op1)), (*(float*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_Cge_f8:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCge((*(double*)(localVarBase + __op1)), (*(double*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_CgeUn_i4:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCgeUn((*(int32_t*)(localVarBase + __op1)), (*(int32_t*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_CgeUn_i8:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCgeUn((*(int64_t*)(localVarBase + __op1)), (*(int64_t*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_CgeUn_f4:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCgeUn((*(float*)(localVarBase + __op1)), (*(float*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_CgeUn_f8:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCgeUn((*(double*)(localVarBase + __op1)), (*(double*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_Clt_i4:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareClt((*(int32_t*)(localVarBase + __op1)), (*(int32_t*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_Clt_i8:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareClt((*(int64_t*)(localVarBase + __op1)), (*(int64_t*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_Clt_f4:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareClt((*(float*)(localVarBase + __op1)), (*(float*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_Clt_f8:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareClt((*(double*)(localVarBase + __op1)), (*(double*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_CltUn_i4:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCltUn((*(int32_t*)(localVarBase + __op1)), (*(int32_t*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_CltUn_i8:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCltUn((*(int64_t*)(localVarBase + __op1)), (*(int64_t*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_CltUn_f4:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCltUn((*(float*)(localVarBase + __op1)), (*(float*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_CltUn_f8:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCltUn((*(double*)(localVarBase + __op1)), (*(double*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_Cle_i4:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCle((*(int32_t*)(localVarBase + __op1)), (*(int32_t*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_Cle_i8:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCle((*(int64_t*)(localVarBase + __op1)), (*(int64_t*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_Cle_f4:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCle((*(float*)(localVarBase + __op1)), (*(float*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_Cle_f8:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCle((*(double*)(localVarBase + __op1)), (*(double*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_CleUn_i4:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCleUn((*(int32_t*)(localVarBase + __op1)), (*(int32_t*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_CleUn_i8:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCleUn((*(int64_t*)(localVarBase + __op1)), (*(int64_t*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_CleUn_f4:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCleUn((*(float*)(localVarBase + __op1)), (*(float*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchVarVar_CleUn_f8:
				{
					uint16_t __op1 = *(uint16_t*)(ip + 2);
					uint16_t __op2 = *(uint16_t*)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 6);
				    if (CompareCleUn((*(double*)(localVarBase + __op1)), (*(double*)(localVarBase + __op2))))
				    {
				        ip = ipBase + __offset;
				    }
				    else
				    {
				        ip += 10;
				    }
				    continue;
				}
				case HiOpcodeEnum::BranchJump:
				{
					uint32_t __token = *(uint32_t*)(ip + 2);
					IL2CPP_ASSERT(false);
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::BranchSwitch:
				{
				    uint16_t __value = *(uint16_t*)(ip + 2);
				    uint32_t __caseNum = *(uint32_t*)(ip + 4);
				    uint32_t __caseOffsets = *(uint32_t*)(ip + 8);
				    uint32_t __idx = (*(uint32_t*)(localVarBase + __value));
				    if (__idx < __caseNum)
				    {
				        ip = ipBase + ((uint32_t*)&imi->resolveDatas[__caseOffsets])[__idx];
				    }
				    else 
				    {
				        ip += 12;
				    }
				    continue;
				}

				//!!!}}BRANCH
#pragma endregion

#pragma region FUNCTION
		//!!!{{FUNCTION
				case HiOpcodeEnum::NewClassVar:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					void* __managed2NativeMethod = *(void**)(ip + 4);
					MethodInfo* __method = *(MethodInfo**)(ip + 12);
					uint32_t __argIdxs = *(uint32_t*)(ip + 20);
				    uint16_t* _argIdxs = ((uint16_t*)&imi->resolveDatas[__argIdxs]);
				    Il2CppObject* _obj = il2cpp::vm::Object::New(__method->klass);
				    *(Il2CppObject**)(localVarBase + _argIdxs[0]) = _obj;
				    ((Managed2NativeCallMethod)__managed2NativeMethod)(__method, _argIdxs, localVarBase, nullptr);
				    (*(Il2CppObject**)(localVarBase + __obj)) = _obj;
				    ip += 24;
				    continue;
				}
				case HiOpcodeEnum::NewClassVar_Ctor_0:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					MethodInfo* __method = *(MethodInfo**)(ip + 4);
				    Il2CppObject* _obj = il2cpp::vm::Object::New(__method->klass);
				    ((NativeClassCtor0)(__method->methodPointer))(_obj, __method);
				    (*(Il2CppObject**)(localVarBase + __obj)) = _obj;
				    ip += 12;
				    continue;
				}
				case HiOpcodeEnum::NewClassVar_NotCtor:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
				    (*(Il2CppObject**)(localVarBase + __obj)) = il2cpp::vm::Object::New(__klass);
				    ip += 12;
				    continue;
				}
				case HiOpcodeEnum::NewValueTypeVar:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					void* __managed2NativeMethod = *(void**)(ip + 4);
					MethodInfo* __method = *(MethodInfo**)(ip + 12);
					uint32_t __argIdxs = *(uint32_t*)(ip + 20);
				    uint16_t* _argIdxs = ((uint16_t*)&imi->resolveDatas[__argIdxs]);
				    int32_t _typeSize = GetTypeValueSize(__method->klass);
				    // arg1, arg2, ..., argN, value type, __this
				    StackObject* _frameBasePtr = localVarBase + _argIdxs[0];
				    void* _this = _frameBasePtr - GetStackSizeByByteSize(_typeSize);
				    _frameBasePtr->ptr = (Il2CppObject*)_this - 1; // adjust pointer
				    InitDefaultN(_this, _typeSize);
				    ((Managed2NativeCallMethod)__managed2NativeMethod)(__method, _argIdxs, localVarBase, nullptr);
				    std::memcpy((void*)(localVarBase + __obj), _this, _typeSize);
				    ip += 24;
				    continue;
				}
				case HiOpcodeEnum::NewClassInterpVar:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					MethodInfo* __method = *(MethodInfo**)(ip + 4);
					uint16_t __argBase = *(uint16_t*)(ip + 12);
					uint16_t __argStackObjectNum = *(uint16_t*)(ip + 14);
					uint16_t __ctorFrameBase = *(uint16_t*)(ip + 16);
				    IL2CPP_ASSERT(__obj < __ctorFrameBase);
				    Il2CppObject* _newObj = il2cpp::vm::Object::New(__method->klass);
				    StackObject* _frameBasePtr = (StackObject*)(void*)(localVarBase + __ctorFrameBase);
				    std::memcpy(_frameBasePtr + 1, (void*)(localVarBase + __argBase), __argStackObjectNum * sizeof(StackObject)); // move arg
				    _frameBasePtr->obj = _newObj; // prepare this 
				    (*(Il2CppObject**)(localVarBase + __obj)) = _newObj; // set must after move
				    CALL_INTERP((ip + 18), __method, _frameBasePtr, nullptr);
				    continue;
				}
				case HiOpcodeEnum::NewClassInterpVar_Ctor_0:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					MethodInfo* __method = *(MethodInfo**)(ip + 4);
					uint16_t __ctorFrameBase = *(uint16_t*)(ip + 12);
				    IL2CPP_ASSERT(__obj < __ctorFrameBase);
				    Il2CppObject* _newObj = il2cpp::vm::Object::New(__method->klass);
				    StackObject* _frameBasePtr = (StackObject*)(void*)(localVarBase + __ctorFrameBase);
				    _frameBasePtr->obj = _newObj; // prepare this 
				    (*(Il2CppObject**)(localVarBase + __obj)) = _newObj;
				    CALL_INTERP((ip + 14), __method, _frameBasePtr, nullptr);
				    continue;
				}
				case HiOpcodeEnum::NewValueTypeInterpVar:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					MethodInfo* __method = *(MethodInfo**)(ip + 4);
					uint16_t __argBase = *(uint16_t*)(ip + 12);
					uint16_t __argStackObjectNum = *(uint16_t*)(ip + 14);
					uint16_t __ctorFrameBase = *(uint16_t*)(ip + 16);
				    IL2CPP_ASSERT(__obj < __ctorFrameBase);
				    StackObject* _frameBasePtr = (StackObject*)(void*)(localVarBase + __ctorFrameBase);
				    std::memcpy(_frameBasePtr + 1, (void*)(localVarBase + __argBase), __argStackObjectNum * sizeof(StackObject)); // move arg
				    _frameBasePtr->ptr = (StackObject*)(void*)(localVarBase + __obj); // prepare this, adjust pointer
				    int32_t _typeSize = GetTypeValueSize(__method->klass);
				    InitDefaultN((void*)(localVarBase + __obj), _typeSize); // init after move
				    CALL_INTERP((ip + 18), __method, _frameBasePtr, nullptr);
				    continue;
				}
				case HiOpcodeEnum::AdjustValueTypeRefVar:
				{
					uint16_t __data = *(uint16_t*)(ip + 2);
				    // ref => fake value type boxed object value. // fake obj = ref(value_type) - sizeof(Il2CppObject)
				    ((StackObject*)((void*)(localVarBase + __data)))->obj -= 1;
				    ip += 4;
				    continue;
				}
				case HiOpcodeEnum::BoxRefVarVar:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 6);
				    (*(Il2CppObject**)(localVarBase + __dst)) = il2cpp::vm::Object::Box(__klass, (*(void**)(localVarBase + __src)));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::LdvirftnVarVar:
				{
					uint16_t __resultMethod = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					MethodInfo* __virtualMethod = *(MethodInfo**)(ip + 6);
				    (*(MethodInfo**)(localVarBase + __resultMethod)) = GET_OBJECT_VIRTUAL_METHOD((*(Il2CppObject**)(localVarBase + __obj)), __virtualMethod);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::RetVar_ret_8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
				    Copy8(frame->ret, (void*)(localVarBase + __ret));
				    LEAVE_FRAME();
				    continue;
				}
				case HiOpcodeEnum::RetVar_ret_12:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
				    Copy12(frame->ret, (void*)(localVarBase + __ret));
				    LEAVE_FRAME();
				    continue;
				}
				case HiOpcodeEnum::RetVar_ret_16:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
				    Copy16(frame->ret, (void*)(localVarBase + __ret));
				    LEAVE_FRAME();
				    continue;
				}
				case HiOpcodeEnum::RetVar_ret_20:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
				    Copy20(frame->ret, (void*)(localVarBase + __ret));
				    LEAVE_FRAME();
				    continue;
				}
				case HiOpcodeEnum::RetVar_ret_24:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
				    Copy24(frame->ret, (void*)(localVarBase + __ret));
				    LEAVE_FRAME();
				    continue;
				}
				case HiOpcodeEnum::RetVar_ret_28:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
				    Copy28(frame->ret, (void*)(localVarBase + __ret));
				    LEAVE_FRAME();
				    continue;
				}
				case HiOpcodeEnum::RetVar_ret_32:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
				    Copy32(frame->ret, (void*)(localVarBase + __ret));
				    LEAVE_FRAME();
				    continue;
				}
				case HiOpcodeEnum::RetVar_ret_n:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __size = *(uint16_t*)(ip + 4);
				    std::memcpy(frame->ret, (void*)(localVarBase + __ret), __size);
				    LEAVE_FRAME();
				    continue;
				}
				case HiOpcodeEnum::RetVar_void:
				{
				    LEAVE_FRAME();
				    continue;
				}
				case HiOpcodeEnum::CallNative_void:
				{
					uint32_t __managed2NativeMethod = *(uint32_t*)(ip + 2);
					uint32_t __methodInfo = *(uint32_t*)(ip + 6);
					uint32_t __argIdxs = *(uint32_t*)(ip + 10);
				    ((Managed2NativeCallMethod)imi->resolveDatas[__managed2NativeMethod])(((MethodInfo*)imi->resolveDatas[__methodInfo]), ((uint16_t*)&imi->resolveDatas[__argIdxs]), localVarBase, nullptr);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::CallNative_ret:
				{
					uint32_t __managed2NativeMethod = *(uint32_t*)(ip + 2);
					uint32_t __methodInfo = *(uint32_t*)(ip + 6);
					uint32_t __argIdxs = *(uint32_t*)(ip + 10);
					uint16_t __ret = *(uint16_t*)(ip + 14);
				    ((Managed2NativeCallMethod)imi->resolveDatas[__managed2NativeMethod])(((MethodInfo*)imi->resolveDatas[__methodInfo]), ((uint16_t*)&imi->resolveDatas[__argIdxs]), localVarBase, (void*)(localVarBase + __ret));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::CallInterp_void:
				{
					MethodInfo* __methodInfo = *(MethodInfo**)(ip + 2);
					uint16_t __argBase = *(uint16_t*)(ip + 10);
				    CALL_INTERP((ip + 12), __methodInfo, (StackObject*)(void*)(localVarBase + __argBase), nullptr);
				    continue;
				}
				case HiOpcodeEnum::CallInterp_ret:
				{
					MethodInfo* __methodInfo = *(MethodInfo**)(ip + 2);
					uint16_t __argBase = *(uint16_t*)(ip + 10);
					uint16_t __ret = *(uint16_t*)(ip + 12);
				    CALL_INTERP((ip + 14), __methodInfo, (StackObject*)(void*)(localVarBase + __argBase), (void*)(localVarBase + __ret));
				    continue;
				}
				case HiOpcodeEnum::CallVirtual_void:
				{
					uint32_t __managed2NativeMethod = *(uint32_t*)(ip + 2);
					uint32_t __methodInfo = *(uint32_t*)(ip + 6);
					uint32_t __argIdxs = *(uint32_t*)(ip + 10);
				    uint16_t* _argIdxData = ((uint16_t*)&imi->resolveDatas[__argIdxs]);
					StackObject* _objPtr = localVarBase + _argIdxData[0];
				    Il2CppObject* _obj = _objPtr->obj;
				    MethodInfo* _actualMethod = GET_OBJECT_VIRTUAL_METHOD(_obj, ((MethodInfo*)imi->resolveDatas[__methodInfo]));
				    if (huatuo::metadata::IsInterpreterMethod(_actualMethod))
				    {
						if (_actualMethod->klass->valuetype)
						{
				            _objPtr->obj += 1;
						}
				        CALL_INTERP((ip + 14), _actualMethod, _objPtr, nullptr);
				    }
				    else 
				    {
				        ((Managed2NativeCallMethod)imi->resolveDatas[__managed2NativeMethod])(_actualMethod, _argIdxData, localVarBase, nullptr);
				        ip += 14;
				    }
				    continue;
				}
				case HiOpcodeEnum::CallVirtual_ret:
				{
					uint32_t __managed2NativeMethod = *(uint32_t*)(ip + 2);
					uint32_t __methodInfo = *(uint32_t*)(ip + 6);
					uint32_t __argIdxs = *(uint32_t*)(ip + 10);
					uint16_t __ret = *(uint16_t*)(ip + 14);
				    uint16_t* _argIdxData = ((uint16_t*)&imi->resolveDatas[__argIdxs]);
					StackObject* _objPtr = localVarBase + _argIdxData[0];
				    Il2CppObject* _obj = _objPtr->obj;
				    MethodInfo* _actualMethod = GET_OBJECT_VIRTUAL_METHOD(_obj, ((MethodInfo*)imi->resolveDatas[__methodInfo]));
				    void* ret = (void*)(localVarBase + __ret);
				    if (huatuo::metadata::IsInterpreterMethod(_actualMethod))
				    {
						if (_actualMethod->klass->valuetype)
						{
				            _objPtr->obj += 1;
						}
				        CALL_INTERP((ip + 16), _actualMethod, _objPtr, ret);
				    }
				    else 
				    {
				        ((Managed2NativeCallMethod)imi->resolveDatas[__managed2NativeMethod])(_actualMethod, _argIdxData, localVarBase, ret);
				        ip += 16;
				    }
				    continue;
				}
				case HiOpcodeEnum::CallInterpVirtual_void:
				{
					MethodInfo* __method = *(MethodInfo**)(ip + 2);
					uint16_t __argBase = *(uint16_t*)(ip + 10);
				    StackObject* _argBasePtr = (StackObject*)(void*)(localVarBase + __argBase);
				    MethodInfo* _actualMethod = GET_OBJECT_VIRTUAL_METHOD(_argBasePtr->obj, __method);
				    if (_actualMethod->klass->valuetype)
				    {
				        _argBasePtr->obj += 1; // adjust pointer
				    }
				    CALL_INTERP((ip + 12), _actualMethod, _argBasePtr, nullptr);
				    continue;
				}
				case HiOpcodeEnum::CallInterpVirtual_ret:
				{
					MethodInfo* __method = *(MethodInfo**)(ip + 2);
					uint16_t __argBase = *(uint16_t*)(ip + 10);
					uint16_t __ret = *(uint16_t*)(ip + 12);
				    StackObject* _argBasePtr = (StackObject*)(void*)(localVarBase + __argBase);
				    MethodInfo* _actualMethod = GET_OBJECT_VIRTUAL_METHOD(_argBasePtr->obj, __method);
				    if (_actualMethod->klass->valuetype)
				    {
				        _argBasePtr->obj += 1; // adjust pointer
				    }
				    CALL_INTERP((ip + 14), _actualMethod, _argBasePtr, (void*)(localVarBase + __ret));
				    continue;
				}
				case HiOpcodeEnum::CallInd_void:
				{
					uint32_t __managed2NativeMethod = *(uint32_t*)(ip + 2);
					uint32_t __methodInfo = *(uint32_t*)(ip + 6);
					uint32_t __argIdxs = *(uint32_t*)(ip + 10);
				    ((Managed2NativeCallMethod)imi->resolveDatas[__managed2NativeMethod])(((MethodInfo*)imi->resolveDatas[__methodInfo]), ((uint16_t*)&imi->resolveDatas[__argIdxs]), localVarBase, nullptr);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::CallInd_ret:
				{
					uint32_t __managed2NativeMethod = *(uint32_t*)(ip + 2);
					uint32_t __methodInfo = *(uint32_t*)(ip + 6);
					uint32_t __argIdxs = *(uint32_t*)(ip + 10);
					uint16_t __ret = *(uint16_t*)(ip + 14);
				    ((Managed2NativeCallMethod)imi->resolveDatas[__managed2NativeMethod])(((MethodInfo*)imi->resolveDatas[__methodInfo]), ((uint16_t*)&imi->resolveDatas[__argIdxs]), localVarBase, (void*)(localVarBase + __argIdxs));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::CallDelegate_void:
				{
					uint32_t __managed2NativeStaticMethod = *(uint32_t*)(ip + 2);
					uint32_t __managed2NativeInstanceMethod = *(uint32_t*)(ip + 6);
					uint32_t __argIdxs = *(uint32_t*)(ip + 10);
				    uint16_t* resolvedArgIdxs = ((uint16_t*)&imi->resolveDatas[__argIdxs]);
				    Il2CppObject* __obj = localVarBase[resolvedArgIdxs[0]].obj;
				    HiCallDelegate((Il2CppMulticastDelegate*)__obj, ((Managed2NativeCallMethod)imi->resolveDatas[__managed2NativeStaticMethod]), ((Managed2NativeCallMethod)imi->resolveDatas[__managed2NativeInstanceMethod]), resolvedArgIdxs, localVarBase, nullptr);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::CallDelegate_ret:
				{
					uint32_t __managed2NativeStaticMethod = *(uint32_t*)(ip + 2);
					uint32_t __managed2NativeInstanceMethod = *(uint32_t*)(ip + 6);
					uint32_t __argIdxs = *(uint32_t*)(ip + 10);
					uint16_t __ret = *(uint16_t*)(ip + 14);
				    uint16_t* resolvedArgIdxs = ((uint16_t*)&imi->resolveDatas[__argIdxs]);
				    Il2CppObject* __obj = localVarBase[resolvedArgIdxs[0]].obj;
				    HiCallDelegate((Il2CppMulticastDelegate*)__obj, ((Managed2NativeCallMethod)imi->resolveDatas[__managed2NativeStaticMethod]), ((Managed2NativeCallMethod)imi->resolveDatas[__managed2NativeInstanceMethod]), resolvedArgIdxs, localVarBase, (void*)(localVarBase + __ret));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::NewDelegate:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					uint16_t __obj = *(uint16_t*)(ip + 12);
					uint16_t __method = *(uint16_t*)(ip + 14);
				    Il2CppDelegate* del = (Il2CppDelegate*)il2cpp::vm::Object::New(__klass);
				    il2cpp::vm::Type::ConstructDelegate(del, (*(Il2CppObject**)(localVarBase + __obj)), (*(MethodInfo**)(localVarBase + __method))->methodPointer, (*(MethodInfo**)(localVarBase + __method)));
				    (*(Il2CppObject**)(localVarBase + __dst)) = (Il2CppObject*)del;
				    ip += 16;
				    continue;
				}

				//!!!}}FUNCTION
#pragma endregion

#pragma region OBJECT
		//!!!{{OBJECT
				case HiOpcodeEnum::BoxVarVar:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __data = *(uint16_t*)(ip + 4);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 6);
				    (*(Il2CppObject**)(localVarBase + __dst)) = il2cpp::vm::Object::Box(__klass, (void*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::UnBoxVarVar:
				{
					uint16_t __addr = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 6);
				    (*(void**)(localVarBase + __addr)) = HiUnbox((*(Il2CppObject**)(localVarBase + __obj)), __klass);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::UnBoxAnyVarVar:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 6);
				    HiUnboxAny((*(Il2CppObject**)(localVarBase + __obj)), __klass, (void*)(localVarBase + __dst));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::CastclassVar:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint32_t __klass = *(uint32_t*)(ip + 4);
				    HiCastClass((*(Il2CppObject**)(localVarBase + __obj)), ((Il2CppClass*)imi->resolveDatas[__klass]));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::IsInstVar:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint32_t __klass = *(uint32_t*)(ip + 4);
				    (*(Il2CppObject**)(localVarBase + __obj)) = il2cpp::vm::Object::IsInst((*(Il2CppObject**)(localVarBase + __obj)), ((Il2CppClass*)imi->resolveDatas[__klass]));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdtokenVar:
				{
					uint16_t __runtimeHandle = *(uint16_t*)(ip + 2);
					void* __token = *(void**)(ip + 4);
				    (*(void**)(localVarBase + __runtimeHandle)) = __token;
				    ip += 12;
				    continue;
				}
				case HiOpcodeEnum::MakeRefVarVar:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __data = *(uint16_t*)(ip + 4);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 6);
				    (*(Il2CppTypedRef*)(localVarBase + __dst)) = MAKE_TYPEDREFERENCE(__klass, (*(void**)(localVarBase + __data)));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::RefAnyTypeVarVar:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __typedRef = *(uint16_t*)(ip + 4);
				    (*(void**)(localVarBase + __dst)) = RefAnyType((*(Il2CppTypedRef*)(localVarBase + __typedRef)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::RefAnyValueVarVar:
				{
					uint16_t __addr = *(uint16_t*)(ip + 2);
					uint16_t __typedRef = *(uint16_t*)(ip + 4);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 6);
				    (*(void**)(localVarBase + __addr)) = RefAnyValue((*(Il2CppTypedRef*)(localVarBase + __typedRef)), __klass);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::CpobjVarVar_1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy1((*(void**)(localVarBase + __dst)), (*(void**)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::CpobjVarVar_2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy2((*(void**)(localVarBase + __dst)), (*(void**)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::CpobjVarVar_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy4((*(void**)(localVarBase + __dst)), (*(void**)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::CpobjVarVar_8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy8((*(void**)(localVarBase + __dst)), (*(void**)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::CpobjVarVar_12:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy12((*(void**)(localVarBase + __dst)), (*(void**)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::CpobjVarVar_16:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy16((*(void**)(localVarBase + __dst)), (*(void**)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::CpobjVarVar_20:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy20((*(void**)(localVarBase + __dst)), (*(void**)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::CpobjVarVar_24:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy24((*(void**)(localVarBase + __dst)), (*(void**)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::CpobjVarVar_28:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy28((*(void**)(localVarBase + __dst)), (*(void**)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::CpobjVarVar_32:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy32((*(void**)(localVarBase + __dst)), (*(void**)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::CpobjVarVar_n_2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					uint16_t __size = *(uint16_t*)(ip + 6);
					std::memcpy((*(void**)(localVarBase + __dst)), (*(void**)(localVarBase + __src)), (*(uint16_t*)(localVarBase + __size)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::CpobjVarVar_n_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					uint16_t __size = *(uint16_t*)(ip + 6);
					std::memcpy((*(void**)(localVarBase + __dst)), (*(void**)(localVarBase + __src)), (*(uint32_t*)(localVarBase + __size)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdobjVarVar_1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy1((void*)(localVarBase + __dst), (*(void**)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdobjVarVar_2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy2((void*)(localVarBase + __dst), (*(void**)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdobjVarVar_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy4((void*)(localVarBase + __dst), (*(void**)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdobjVarVar_8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy8((void*)(localVarBase + __dst), (*(void**)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdobjVarVar_12:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy12((void*)(localVarBase + __dst), (*(void**)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdobjVarVar_16:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy16((void*)(localVarBase + __dst), (*(void**)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdobjVarVar_20:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy20((void*)(localVarBase + __dst), (*(void**)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdobjVarVar_24:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy24((void*)(localVarBase + __dst), (*(void**)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdobjVarVar_28:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy28((void*)(localVarBase + __dst), (*(void**)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdobjVarVar_32:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy32((void*)(localVarBase + __dst), (*(void**)(localVarBase + __src)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::LdobjVarVar_n_2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					uint16_t __size = *(uint16_t*)(ip + 6);
					std::memcpy((void*)(localVarBase + __dst), (*(void**)(localVarBase + __src)), (*(uint16_t*)(localVarBase + __size)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdobjVarVar_n_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					uint16_t __size = *(uint16_t*)(ip + 6);
					std::memcpy((void*)(localVarBase + __dst), (*(void**)(localVarBase + __src)), (*(uint32_t*)(localVarBase + __size)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::StobjVarVar_1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy1((*(void**)(localVarBase + __dst)), (void*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::StobjVarVar_2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy2((*(void**)(localVarBase + __dst)), (void*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::StobjVarVar_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy4((*(void**)(localVarBase + __dst)), (void*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::StobjVarVar_8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy8((*(void**)(localVarBase + __dst)), (void*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::StobjVarVar_12:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy12((*(void**)(localVarBase + __dst)), (void*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::StobjVarVar_16:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy16((*(void**)(localVarBase + __dst)), (void*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::StobjVarVar_20:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy20((*(void**)(localVarBase + __dst)), (void*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::StobjVarVar_24:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy24((*(void**)(localVarBase + __dst)), (void*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::StobjVarVar_28:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy28((*(void**)(localVarBase + __dst)), (void*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::StobjVarVar_32:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					Copy32((*(void**)(localVarBase + __dst)), (void*)(localVarBase + __src));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::StobjVarVar_n_2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					uint16_t __size = *(uint16_t*)(ip + 6);
					std::memcpy((*(void**)(localVarBase + __dst)), (void*)(localVarBase + __src), (*(uint16_t*)(localVarBase + __size)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::StobjVarVar_n_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __src = *(uint16_t*)(ip + 4);
					uint16_t __size = *(uint16_t*)(ip + 6);
					std::memcpy((*(void**)(localVarBase + __dst)), (void*)(localVarBase + __src), (*(uint32_t*)(localVarBase + __size)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::InitobjVar_1:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					InitDefault1((*(void**)(localVarBase + __obj)));
				    ip += 4;
				    continue;
				}
				case HiOpcodeEnum::InitobjVar_2:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					InitDefault2((*(void**)(localVarBase + __obj)));
				    ip += 4;
				    continue;
				}
				case HiOpcodeEnum::InitobjVar_4:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					InitDefault4((*(void**)(localVarBase + __obj)));
				    ip += 4;
				    continue;
				}
				case HiOpcodeEnum::InitobjVar_8:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					InitDefault8((*(void**)(localVarBase + __obj)));
				    ip += 4;
				    continue;
				}
				case HiOpcodeEnum::InitobjVar_12:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					InitDefault12((*(void**)(localVarBase + __obj)));
				    ip += 4;
				    continue;
				}
				case HiOpcodeEnum::InitobjVar_16:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					InitDefault16((*(void**)(localVarBase + __obj)));
				    ip += 4;
				    continue;
				}
				case HiOpcodeEnum::InitobjVar_20:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					InitDefault20((*(void**)(localVarBase + __obj)));
				    ip += 4;
				    continue;
				}
				case HiOpcodeEnum::InitobjVar_24:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					InitDefault24((*(void**)(localVarBase + __obj)));
				    ip += 4;
				    continue;
				}
				case HiOpcodeEnum::InitobjVar_28:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					InitDefault28((*(void**)(localVarBase + __obj)));
				    ip += 4;
				    continue;
				}
				case HiOpcodeEnum::InitobjVar_32:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					InitDefault32((*(void**)(localVarBase + __obj)));
				    ip += 4;
				    continue;
				}
				case HiOpcodeEnum::InitobjVar_n_2:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __size = *(uint16_t*)(ip + 4);
					InitDefaultN((*(void**)(localVarBase + __obj)), __size);
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::InitobjVar_n_4:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint32_t __size = *(uint32_t*)(ip + 4);
					InitDefaultN((*(void**)(localVarBase + __obj)), __size);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdstrVar:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint32_t __str = *(uint32_t*)(ip + 4);
				    (*(Il2CppString**)(localVarBase + __dst)) = ((Il2CppString*)imi->resolveDatas[__str]);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldVarVar_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    (*(int32_t*)(localVarBase + __dst)) = *(int8_t*)((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldVarVar_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    (*(int32_t*)(localVarBase + __dst)) = *(uint8_t*)((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldVarVar_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    (*(int32_t*)(localVarBase + __dst)) = *(int16_t*)((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldVarVar_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    (*(int32_t*)(localVarBase + __dst)) = *(uint16_t*)((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldVarVar_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    (*(int32_t*)(localVarBase + __dst)) = *(int32_t*)((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldVarVar_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    (*(int32_t*)(localVarBase + __dst)) = *(uint32_t*)((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldVarVar_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    (*(int64_t*)(localVarBase + __dst)) = *(int64_t*)((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldVarVar_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    (*(int64_t*)(localVarBase + __dst)) = *(uint64_t*)((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldVarVar_size_8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    Copy8((void*)(localVarBase + __dst), (uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldVarVar_size_12:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    Copy12((void*)(localVarBase + __dst), (uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldVarVar_size_16:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    Copy16((void*)(localVarBase + __dst), (uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldVarVar_size_20:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    Copy20((void*)(localVarBase + __dst), (uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldVarVar_size_24:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    Copy24((void*)(localVarBase + __dst), (uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldVarVar_size_28:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    Copy28((void*)(localVarBase + __dst), (uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldVarVar_size_32:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    Copy32((void*)(localVarBase + __dst), (uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldVarVar_n_2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
					uint16_t __size = *(uint16_t*)(ip + 8);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    std::memcpy((void*)(localVarBase + __dst), (uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset, __size);
				    ip += 10;
				    continue;
				}
				case HiOpcodeEnum::LdfldVarVar_n_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
					uint32_t __size = *(uint32_t*)(ip + 8);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    std::memcpy((void*)(localVarBase + __dst), (uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset, __size);
				    ip += 12;
				    continue;
				}
				case HiOpcodeEnum::LdfldValueTypeVarVar_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __dst)) = *(int8_t*)((byte*)(void*)(localVarBase + __obj) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldValueTypeVarVar_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __dst)) = *(uint8_t*)((byte*)(void*)(localVarBase + __obj) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldValueTypeVarVar_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __dst)) = *(int16_t*)((byte*)(void*)(localVarBase + __obj) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldValueTypeVarVar_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __dst)) = *(uint16_t*)((byte*)(void*)(localVarBase + __obj) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldValueTypeVarVar_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __dst)) = *(int32_t*)((byte*)(void*)(localVarBase + __obj) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldValueTypeVarVar_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
					(*(int32_t*)(localVarBase + __dst)) = *(uint32_t*)((byte*)(void*)(localVarBase + __obj) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldValueTypeVarVar_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
					(*(int64_t*)(localVarBase + __dst)) = *(int64_t*)((byte*)(void*)(localVarBase + __obj) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldValueTypeVarVar_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
					(*(int64_t*)(localVarBase + __dst)) = *(uint64_t*)((byte*)(void*)(localVarBase + __obj) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldValueTypeVarVar_size_8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
					Copy8((void*)(localVarBase + __dst), (byte*)(void*)(localVarBase + __obj) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldValueTypeVarVar_size_12:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
					Copy12((void*)(localVarBase + __dst), (byte*)(void*)(localVarBase + __obj) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldValueTypeVarVar_size_16:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
					Copy16((void*)(localVarBase + __dst), (byte*)(void*)(localVarBase + __obj) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldValueTypeVarVar_size_20:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
					Copy20((void*)(localVarBase + __dst), (byte*)(void*)(localVarBase + __obj) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldValueTypeVarVar_size_24:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
					Copy24((void*)(localVarBase + __dst), (byte*)(void*)(localVarBase + __obj) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldValueTypeVarVar_size_28:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
					Copy28((void*)(localVarBase + __dst), (byte*)(void*)(localVarBase + __obj) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldValueTypeVarVar_size_32:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
					Copy32((void*)(localVarBase + __dst), (byte*)(void*)(localVarBase + __obj) + __offset);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::LdfldValueTypeVarVar_n_2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
					uint16_t __size = *(uint16_t*)(ip + 8);
					std::memcpy((void*)(localVarBase + __dst), (byte*)(void*)(localVarBase + __obj) + __offset, __size);
				    ip += 10;
				    continue;
				}
				case HiOpcodeEnum::LdfldValueTypeVarVar_n_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
					uint32_t __size = *(uint32_t*)(ip + 8);
					std::memcpy((void*)(localVarBase + __dst), (byte*)(void*)(localVarBase + __obj) + __offset, __size);
				    ip += 12;
				    continue;
				}
				case HiOpcodeEnum::LdfldaVarVar:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    (*(void**)(localVarBase + __dst)) = (uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset;
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::StfldVarVar_i1:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 4);
					uint16_t __data = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    *(int8_t*)((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset) = (*(int8_t*)(localVarBase + __data));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::StfldVarVar_u1:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 4);
					uint16_t __data = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    *(uint8_t*)((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset) = (*(uint8_t*)(localVarBase + __data));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::StfldVarVar_i2:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 4);
					uint16_t __data = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    *(int16_t*)((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset) = (*(int16_t*)(localVarBase + __data));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::StfldVarVar_u2:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 4);
					uint16_t __data = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    *(uint16_t*)((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset) = (*(uint16_t*)(localVarBase + __data));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::StfldVarVar_i4:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 4);
					uint16_t __data = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    *(int32_t*)((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset) = (*(int32_t*)(localVarBase + __data));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::StfldVarVar_u4:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 4);
					uint16_t __data = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    *(uint32_t*)((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset) = (*(uint32_t*)(localVarBase + __data));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::StfldVarVar_i8:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 4);
					uint16_t __data = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    *(int64_t*)((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset) = (*(int64_t*)(localVarBase + __data));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::StfldVarVar_u8:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 4);
					uint16_t __data = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    *(uint64_t*)((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset) = (*(uint64_t*)(localVarBase + __data));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::StfldVarVar_size_8:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 4);
					uint16_t __data = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    Copy8((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset, (void*)(localVarBase + __data));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::StfldVarVar_size_12:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 4);
					uint16_t __data = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    Copy12((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset, (void*)(localVarBase + __data));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::StfldVarVar_size_16:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 4);
					uint16_t __data = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    Copy16((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset, (void*)(localVarBase + __data));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::StfldVarVar_size_20:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 4);
					uint16_t __data = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    Copy20((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset, (void*)(localVarBase + __data));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::StfldVarVar_size_24:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 4);
					uint16_t __data = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    Copy24((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset, (void*)(localVarBase + __data));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::StfldVarVar_size_28:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 4);
					uint16_t __data = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    Copy28((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset, (void*)(localVarBase + __data));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::StfldVarVar_size_32:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 4);
					uint16_t __data = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    Copy32((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset, (void*)(localVarBase + __data));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::StfldVarVar_n_2:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 4);
					uint16_t __data = *(uint16_t*)(ip + 6);
					uint16_t __size = *(uint16_t*)(ip + 8);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    std::memcpy((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset, (void*)(localVarBase + __data), __size);
				    ip += 10;
				    continue;
				}
				case HiOpcodeEnum::StfldVarVar_n_4:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 4);
					uint16_t __data = *(uint16_t*)(ip + 6);
					uint32_t __size = *(uint32_t*)(ip + 8);
				    CHECK_NOT_NULL_THROW((*(Il2CppObject**)(localVarBase + __obj)));
				    std::memcpy((uint8_t*)(*(Il2CppObject**)(localVarBase + __obj)) + __offset, (void*)(localVarBase + __data), __size);
				    ip += 12;
				    continue;
				}
				case HiOpcodeEnum::LdsfldVarVar_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    (*(int32_t*)(localVarBase + __dst)) = *(int8_t*)(((byte*)__klass->static_fields) + __offset);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::LdsfldVarVar_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    (*(int32_t*)(localVarBase + __dst)) = *(uint8_t*)(((byte*)__klass->static_fields) + __offset);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::LdsfldVarVar_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    (*(int32_t*)(localVarBase + __dst)) = *(int16_t*)(((byte*)__klass->static_fields) + __offset);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::LdsfldVarVar_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    (*(int32_t*)(localVarBase + __dst)) = *(uint16_t*)(((byte*)__klass->static_fields) + __offset);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::LdsfldVarVar_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    (*(int32_t*)(localVarBase + __dst)) = *(int32_t*)(((byte*)__klass->static_fields) + __offset);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::LdsfldVarVar_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    (*(int32_t*)(localVarBase + __dst)) = *(uint32_t*)(((byte*)__klass->static_fields) + __offset);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::LdsfldVarVar_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    (*(int64_t*)(localVarBase + __dst)) = *(int64_t*)(((byte*)__klass->static_fields) + __offset);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::LdsfldVarVar_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    (*(int64_t*)(localVarBase + __dst)) = *(uint64_t*)(((byte*)__klass->static_fields) + __offset);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::LdsfldVarVar_size_8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy8((void*)(localVarBase + __dst), ((byte*)__klass->static_fields) + __offset);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::LdsfldVarVar_size_12:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy12((void*)(localVarBase + __dst), ((byte*)__klass->static_fields) + __offset);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::LdsfldVarVar_size_16:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy16((void*)(localVarBase + __dst), ((byte*)__klass->static_fields) + __offset);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::LdsfldVarVar_size_20:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy20((void*)(localVarBase + __dst), ((byte*)__klass->static_fields) + __offset);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::LdsfldVarVar_size_24:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy24((void*)(localVarBase + __dst), ((byte*)__klass->static_fields) + __offset);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::LdsfldVarVar_size_28:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy28((void*)(localVarBase + __dst), ((byte*)__klass->static_fields) + __offset);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::LdsfldVarVar_size_32:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy32((void*)(localVarBase + __dst), ((byte*)__klass->static_fields) + __offset);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::LdsfldVarVar_n_2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 12);
					uint16_t __size = *(uint16_t*)(ip + 14);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    std::memcpy((void*)(localVarBase + __dst), (((byte*)__klass->static_fields) + __offset), __size);
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::LdsfldVarVar_n_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 12);
					uint32_t __size = *(uint32_t*)(ip + 14);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    std::memcpy((void*)(localVarBase + __dst), (((byte*)__klass->static_fields) + __offset), __size);
				    ip += 18;
				    continue;
				}
				case HiOpcodeEnum::StsfldVarVar_i1:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    *(int8_t*)(((byte*)__klass->static_fields) + __offset) = (*(int8_t*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StsfldVarVar_u1:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    *(uint8_t*)(((byte*)__klass->static_fields) + __offset) = (*(uint8_t*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StsfldVarVar_i2:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    *(int16_t*)(((byte*)__klass->static_fields) + __offset) = (*(int16_t*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StsfldVarVar_u2:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    *(uint16_t*)(((byte*)__klass->static_fields) + __offset) = (*(uint16_t*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StsfldVarVar_i4:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    *(int32_t*)(((byte*)__klass->static_fields) + __offset) = (*(int32_t*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StsfldVarVar_u4:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    *(uint32_t*)(((byte*)__klass->static_fields) + __offset) = (*(uint32_t*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StsfldVarVar_i8:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    *(int64_t*)(((byte*)__klass->static_fields) + __offset) = (*(int64_t*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StsfldVarVar_u8:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    *(uint64_t*)(((byte*)__klass->static_fields) + __offset) = (*(uint64_t*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StsfldVarVar_size_8:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy8(((byte*)__klass->static_fields) + __offset, (void*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StsfldVarVar_size_12:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy12(((byte*)__klass->static_fields) + __offset, (void*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StsfldVarVar_size_16:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy16(((byte*)__klass->static_fields) + __offset, (void*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StsfldVarVar_size_20:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy20(((byte*)__klass->static_fields) + __offset, (void*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StsfldVarVar_size_24:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy24(((byte*)__klass->static_fields) + __offset, (void*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StsfldVarVar_size_28:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy28(((byte*)__klass->static_fields) + __offset, (void*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StsfldVarVar_size_32:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy32(((byte*)__klass->static_fields) + __offset, (void*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StsfldVarVar_n_2:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
					uint16_t __size = *(uint16_t*)(ip + 14);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    std::memcpy(((byte*)__klass->static_fields) + __offset, (void*)(localVarBase + __data), __size);
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::StsfldVarVar_n_4:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
					uint32_t __size = *(uint32_t*)(ip + 14);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    std::memcpy(((byte*)__klass->static_fields) + __offset, (void*)(localVarBase + __data), __size);
				    ip += 18;
				    continue;
				}
				case HiOpcodeEnum::LdsfldaVarVar:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					uint16_t __offset = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    (*(void**)(localVarBase + __dst)) = ((byte*)__klass->static_fields) + __offset;
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::LdthreadlocalaVarVar:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    (*(void**)(localVarBase + __dst)) = il2cpp::vm::Thread::GetThreadStaticData(__offset);
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::LdthreadlocalVarVar_i1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    (*(int32_t*)(localVarBase + __dst)) = *(int8_t*)(il2cpp::vm::Thread::GetThreadStaticData(__offset));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::LdthreadlocalVarVar_u1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    (*(int32_t*)(localVarBase + __dst)) = *(uint8_t*)(il2cpp::vm::Thread::GetThreadStaticData(__offset));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::LdthreadlocalVarVar_i2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    (*(int32_t*)(localVarBase + __dst)) = *(int16_t*)(il2cpp::vm::Thread::GetThreadStaticData(__offset));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::LdthreadlocalVarVar_u2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    (*(int32_t*)(localVarBase + __dst)) = *(uint16_t*)(il2cpp::vm::Thread::GetThreadStaticData(__offset));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::LdthreadlocalVarVar_i4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    (*(int32_t*)(localVarBase + __dst)) = *(int32_t*)(il2cpp::vm::Thread::GetThreadStaticData(__offset));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::LdthreadlocalVarVar_u4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    (*(int32_t*)(localVarBase + __dst)) = *(uint32_t*)(il2cpp::vm::Thread::GetThreadStaticData(__offset));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::LdthreadlocalVarVar_i8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    (*(int64_t*)(localVarBase + __dst)) = *(int64_t*)(il2cpp::vm::Thread::GetThreadStaticData(__offset));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::LdthreadlocalVarVar_u8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    (*(int64_t*)(localVarBase + __dst)) = *(uint64_t*)(il2cpp::vm::Thread::GetThreadStaticData(__offset));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::LdthreadlocalVarVar_size_8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy8((void*)(localVarBase + __dst), il2cpp::vm::Thread::GetThreadStaticData(__offset));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::LdthreadlocalVarVar_size_12:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy12((void*)(localVarBase + __dst), il2cpp::vm::Thread::GetThreadStaticData(__offset));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::LdthreadlocalVarVar_size_16:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy16((void*)(localVarBase + __dst), il2cpp::vm::Thread::GetThreadStaticData(__offset));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::LdthreadlocalVarVar_size_20:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy20((void*)(localVarBase + __dst), il2cpp::vm::Thread::GetThreadStaticData(__offset));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::LdthreadlocalVarVar_size_24:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy24((void*)(localVarBase + __dst), il2cpp::vm::Thread::GetThreadStaticData(__offset));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::LdthreadlocalVarVar_size_28:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy28((void*)(localVarBase + __dst), il2cpp::vm::Thread::GetThreadStaticData(__offset));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::LdthreadlocalVarVar_size_32:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy32((void*)(localVarBase + __dst), il2cpp::vm::Thread::GetThreadStaticData(__offset));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::LdthreadlocalVarVar_n_2:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 12);
					uint16_t __size = *(uint16_t*)(ip + 16);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    std::memcpy((void*)(localVarBase + __dst), il2cpp::vm::Thread::GetThreadStaticData(__offset), __size);
				    ip += 18;
				    continue;
				}
				case HiOpcodeEnum::LdthreadlocalVarVar_n_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 4);
					int32_t __offset = *(int32_t*)(ip + 12);
					uint32_t __size = *(uint32_t*)(ip + 16);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    std::memcpy((void*)(localVarBase + __dst), il2cpp::vm::Thread::GetThreadStaticData(__offset), __size);
				    ip += 20;
				    continue;
				}
				case HiOpcodeEnum::StthreadlocalVarVar_i1:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    *(int8_t*)(il2cpp::vm::Thread::GetThreadStaticData(__offset)) = (*(int8_t*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StthreadlocalVarVar_u1:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    *(uint8_t*)(il2cpp::vm::Thread::GetThreadStaticData(__offset)) = (*(uint8_t*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StthreadlocalVarVar_i2:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    *(int16_t*)(il2cpp::vm::Thread::GetThreadStaticData(__offset)) = (*(int16_t*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StthreadlocalVarVar_u2:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    *(uint16_t*)(il2cpp::vm::Thread::GetThreadStaticData(__offset)) = (*(uint16_t*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StthreadlocalVarVar_i4:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    *(int32_t*)(il2cpp::vm::Thread::GetThreadStaticData(__offset)) = (*(int32_t*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StthreadlocalVarVar_u4:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    *(uint32_t*)(il2cpp::vm::Thread::GetThreadStaticData(__offset)) = (*(uint32_t*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StthreadlocalVarVar_i8:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    *(int64_t*)(il2cpp::vm::Thread::GetThreadStaticData(__offset)) = (*(int64_t*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StthreadlocalVarVar_u8:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    *(uint64_t*)(il2cpp::vm::Thread::GetThreadStaticData(__offset)) = (*(uint64_t*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StthreadlocalVarVar_size_8:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy8(il2cpp::vm::Thread::GetThreadStaticData(__offset), (void*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StthreadlocalVarVar_size_12:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy12(il2cpp::vm::Thread::GetThreadStaticData(__offset), (void*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StthreadlocalVarVar_size_16:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy16(il2cpp::vm::Thread::GetThreadStaticData(__offset), (void*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StthreadlocalVarVar_size_20:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy20(il2cpp::vm::Thread::GetThreadStaticData(__offset), (void*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StthreadlocalVarVar_size_24:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy24(il2cpp::vm::Thread::GetThreadStaticData(__offset), (void*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StthreadlocalVarVar_size_28:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy28(il2cpp::vm::Thread::GetThreadStaticData(__offset), (void*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StthreadlocalVarVar_size_32:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    Copy32(il2cpp::vm::Thread::GetThreadStaticData(__offset), (void*)(localVarBase + __data));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::StthreadlocalVarVar_n_2:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
					uint16_t __size = *(uint16_t*)(ip + 14);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    std::memcpy(il2cpp::vm::Thread::GetThreadStaticData(__offset), (void*)(localVarBase + __data), __size);
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::StthreadlocalVarVar_n_4:
				{
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 2);
					uint16_t __offset = *(uint16_t*)(ip + 10);
					uint16_t __data = *(uint16_t*)(ip + 12);
					uint32_t __size = *(uint32_t*)(ip + 14);
				    Interpreter::RuntimeClassCCtorInit(__klass);
				    std::memcpy(il2cpp::vm::Thread::GetThreadStaticData(__offset), (void*)(localVarBase + __data), __size);
				    ip += 18;
				    continue;
				}

				//!!!}}OBJECT
#pragma endregion

#pragma region ARRAY
		//!!!{{ARRAY
				case HiOpcodeEnum::NewArrVarVar_4:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __size = *(uint16_t*)(ip + 4);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 6);
				    (*(Il2CppArray**)(localVarBase + __arr)) =  il2cpp::vm::Array::NewSpecific(__klass, (*(int32_t*)(localVarBase + __size)));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::NewArrVarVar_8:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __size = *(uint16_t*)(ip + 4);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 6);
				    (*(Il2CppArray**)(localVarBase + __arr)) =  il2cpp::vm::Array::NewSpecific(__klass, (*(int64_t*)(localVarBase + __size)));
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::GetArrayLengthVarVar_4:
				{
					uint16_t __len = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    (*(uint32_t*)(localVarBase + __len)) = (uint32_t)il2cpp::vm::Array::GetLength((*(Il2CppArray**)(localVarBase + __arr)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::GetArrayLengthVarVar_8:
				{
					uint16_t __len = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    (*(uint64_t*)(localVarBase + __len)) = (uint64_t)il2cpp::vm::Array::GetLength((*(Il2CppArray**)(localVarBase + __arr)));
				    ip += 6;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementAddressAddrVarVar_i4:
				{
					uint16_t __addr = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    (*(void**)(localVarBase + __addr)) = load_array_elema(arr, (*(int32_t*)(localVarBase + __index)), il2cpp::vm::Array::GetElementSize(arr->klass));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementAddressAddrVarVar_i8:
				{
					uint16_t __addr = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    (*(void**)(localVarBase + __addr)) = load_array_elema(arr, (*(int64_t*)(localVarBase + __index)), il2cpp::vm::Array::GetElementSize(arr->klass));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementAddressCheckAddrVarVar_i4:
				{
					uint16_t __addr = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
					Il2CppClass* __eleKlass = *(Il2CppClass**)(ip + 8);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    CheckArrayElementTypeCompatible(arr->klass, __eleKlass);
				    (*(void**)(localVarBase + __addr)) = load_array_elema(arr, (*(int32_t*)(localVarBase + __index)), il2cpp::vm::Array::GetElementSize(arr->klass));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementAddressCheckAddrVarVar_i8:
				{
					uint16_t __addr = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
					Il2CppClass* __eleKlass = *(Il2CppClass**)(ip + 8);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    CheckArrayElementTypeCompatible(arr->klass, __eleKlass);
				    (*(void**)(localVarBase + __addr)) = load_array_elema(arr, (*(int64_t*)(localVarBase + __index)), il2cpp::vm::Array::GetElementSize(arr->klass));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_i1_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    (*(int32_t*)(localVarBase + __dst)) = il2cpp_array_get(arr, int8_t, (*(int32_t*)(localVarBase + __index)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_u1_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    (*(int32_t*)(localVarBase + __dst)) = il2cpp_array_get(arr, uint8_t, (*(int32_t*)(localVarBase + __index)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_i2_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    (*(int32_t*)(localVarBase + __dst)) = il2cpp_array_get(arr, int16_t, (*(int32_t*)(localVarBase + __index)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_u2_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    (*(int32_t*)(localVarBase + __dst)) = il2cpp_array_get(arr, uint16_t, (*(int32_t*)(localVarBase + __index)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_i4_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    (*(int32_t*)(localVarBase + __dst)) = il2cpp_array_get(arr, int32_t, (*(int32_t*)(localVarBase + __index)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_u4_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    (*(int32_t*)(localVarBase + __dst)) = il2cpp_array_get(arr, uint32_t, (*(int32_t*)(localVarBase + __index)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_i8_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    (*(int64_t*)(localVarBase + __dst)) = il2cpp_array_get(arr, int64_t, (*(int32_t*)(localVarBase + __index)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_u8_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    (*(int64_t*)(localVarBase + __dst)) = il2cpp_array_get(arr, uint64_t, (*(int32_t*)(localVarBase + __index)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_size_12_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    Copy12((void*)(localVarBase + __dst), load_array_elema(arr, (*(int32_t*)(localVarBase + __index)), 12));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_size_16_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    Copy16((void*)(localVarBase + __dst), load_array_elema(arr, (*(int32_t*)(localVarBase + __index)), 16));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_n_4:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    int32_t eleSize = il2cpp::vm::Array::GetElementSize(arr->klass);
				    std::memcpy((void*)(localVarBase + __dst), load_array_elema(arr, (*(int32_t*)(localVarBase + __index)), eleSize), eleSize);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_i1_8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    (*(int32_t*)(localVarBase + __dst)) = il2cpp_array_get(arr, int8_t, (*(int64_t*)(localVarBase + __index)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_u1_8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    (*(int32_t*)(localVarBase + __dst)) = il2cpp_array_get(arr, uint8_t, (*(int64_t*)(localVarBase + __index)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_i2_8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    (*(int32_t*)(localVarBase + __dst)) = il2cpp_array_get(arr, int16_t, (*(int64_t*)(localVarBase + __index)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_u2_8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    (*(int32_t*)(localVarBase + __dst)) = il2cpp_array_get(arr, uint16_t, (*(int64_t*)(localVarBase + __index)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_i4_8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    (*(int32_t*)(localVarBase + __dst)) = il2cpp_array_get(arr, int32_t, (*(int64_t*)(localVarBase + __index)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_u4_8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    (*(int32_t*)(localVarBase + __dst)) = il2cpp_array_get(arr, uint32_t, (*(int64_t*)(localVarBase + __index)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_i8_8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    (*(int64_t*)(localVarBase + __dst)) = il2cpp_array_get(arr, int64_t, (*(int64_t*)(localVarBase + __index)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_u8_8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    (*(int64_t*)(localVarBase + __dst)) = il2cpp_array_get(arr, uint64_t, (*(int64_t*)(localVarBase + __index)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_size_12_8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    Copy12((void*)(localVarBase + __dst), load_array_elema(arr, (*(int64_t*)(localVarBase + __index)), 12));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_size_16_8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    Copy16((void*)(localVarBase + __dst), load_array_elema(arr, (*(int64_t*)(localVarBase + __index)), 16));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetArrayElementVarVar_n_8:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __index = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(arr)
				    int32_t eleSize = il2cpp::vm::Array::GetElementSize(arr->klass);
				    std::memcpy((void*)(localVarBase + __dst), load_array_elema(arr, (*(int64_t*)(localVarBase + __index)), eleSize), eleSize);
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_i1_4:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    il2cpp_array_set((*(Il2CppArray**)(localVarBase + __arr)), int8_t, (*(int32_t*)(localVarBase + __index)), (*(int8_t*)(localVarBase + __ele)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_u1_4:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    il2cpp_array_set((*(Il2CppArray**)(localVarBase + __arr)), uint8_t, (*(int32_t*)(localVarBase + __index)), (*(uint8_t*)(localVarBase + __ele)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_i2_4:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    il2cpp_array_set((*(Il2CppArray**)(localVarBase + __arr)), int16_t, (*(int32_t*)(localVarBase + __index)), (*(int16_t*)(localVarBase + __ele)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_u2_4:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    il2cpp_array_set((*(Il2CppArray**)(localVarBase + __arr)), uint16_t, (*(int32_t*)(localVarBase + __index)), (*(uint16_t*)(localVarBase + __ele)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_i4_4:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    il2cpp_array_set((*(Il2CppArray**)(localVarBase + __arr)), int32_t, (*(int32_t*)(localVarBase + __index)), (*(int32_t*)(localVarBase + __ele)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_u4_4:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    il2cpp_array_set((*(Il2CppArray**)(localVarBase + __arr)), uint32_t, (*(int32_t*)(localVarBase + __index)), (*(uint32_t*)(localVarBase + __ele)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_i8_4:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    il2cpp_array_set((*(Il2CppArray**)(localVarBase + __arr)), int64_t, (*(int32_t*)(localVarBase + __index)), (*(int64_t*)(localVarBase + __ele)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_u8_4:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    il2cpp_array_set((*(Il2CppArray**)(localVarBase + __arr)), uint64_t, (*(int32_t*)(localVarBase + __index)), (*(uint64_t*)(localVarBase + __ele)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_ref_4:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    il2cpp_array_setref((*(Il2CppArray**)(localVarBase + __arr)), (*(int32_t*)(localVarBase + __index)), (*(Il2CppObject**)(localVarBase + __ele)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_size_12_4:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    Il2CppArray* _arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(_arr)
				    Copy12(load_array_elema(_arr, (*(int32_t*)(localVarBase + __index)), 12), (void*)(localVarBase + __ele));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_size_16_4:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    Il2CppArray* _arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(_arr)
				    Copy16(load_array_elema(_arr, (*(int32_t*)(localVarBase + __index)), 16), (void*)(localVarBase + __ele));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_n_4:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_THROW(arr);
				    int32_t eleSize = il2cpp::vm::Array::GetElementSize(arr->klass);
				    il2cpp_array_setrefwithsize(arr, eleSize, __index, (void*)(localVarBase + __ele));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_i1_8:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    il2cpp_array_set((*(Il2CppArray**)(localVarBase + __arr)), int8_t, (*(int64_t*)(localVarBase + __index)), (*(int8_t*)(localVarBase + __ele)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_u1_8:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    il2cpp_array_set((*(Il2CppArray**)(localVarBase + __arr)), uint8_t, (*(int64_t*)(localVarBase + __index)), (*(uint8_t*)(localVarBase + __ele)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_i2_8:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    il2cpp_array_set((*(Il2CppArray**)(localVarBase + __arr)), int16_t, (*(int64_t*)(localVarBase + __index)), (*(int16_t*)(localVarBase + __ele)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_u2_8:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    il2cpp_array_set((*(Il2CppArray**)(localVarBase + __arr)), uint16_t, (*(int64_t*)(localVarBase + __index)), (*(uint16_t*)(localVarBase + __ele)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_i4_8:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    il2cpp_array_set((*(Il2CppArray**)(localVarBase + __arr)), int32_t, (*(int64_t*)(localVarBase + __index)), (*(int32_t*)(localVarBase + __ele)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_u4_8:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    il2cpp_array_set((*(Il2CppArray**)(localVarBase + __arr)), uint32_t, (*(int64_t*)(localVarBase + __index)), (*(uint32_t*)(localVarBase + __ele)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_i8_8:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    il2cpp_array_set((*(Il2CppArray**)(localVarBase + __arr)), int64_t, (*(int64_t*)(localVarBase + __index)), (*(int64_t*)(localVarBase + __ele)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_u8_8:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    il2cpp_array_set((*(Il2CppArray**)(localVarBase + __arr)), uint64_t, (*(int64_t*)(localVarBase + __index)), (*(uint64_t*)(localVarBase + __ele)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_ref_8:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    il2cpp_array_setref((*(Il2CppArray**)(localVarBase + __arr)), (*(int64_t*)(localVarBase + __index)), (*(Il2CppObject**)(localVarBase + __ele)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_size_12_8:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    Il2CppArray* _arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(_arr)
				    Copy12(load_array_elema(_arr, (*(int64_t*)(localVarBase + __index)), 12), (void*)(localVarBase + __ele));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_size_16_8:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    Il2CppArray* _arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_AND_ARRAY_BOUNDARY(_arr)
				    Copy16(load_array_elema(_arr, (*(int64_t*)(localVarBase + __index)), 16), (void*)(localVarBase + __ele));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementVarVar_n_8:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
				    Il2CppArray* arr = (*(Il2CppArray**)(localVarBase + __arr));
				    CHECK_NOT_NULL_THROW(arr);
				    int32_t eleSize = il2cpp::vm::Array::GetElementSize(arr->klass);
				    il2cpp_array_setrefwithsize(arr, eleSize, __index, (void*)(localVarBase + __ele));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementObjectCheckVarVar_4:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 8);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    CHECK_ARRAY_TYPE_COMPATIBLE((*(Il2CppArray**)(localVarBase + __arr)), __klass);
				    il2cpp_array_set((*(Il2CppArray**)(localVarBase + __arr)), Il2CppObject*, (*(int32_t*)(localVarBase + __index)), (*(Il2CppObject**)(localVarBase + __ele)));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::SetArrayElementObjectCheckVarVar_8:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __index = *(uint16_t*)(ip + 4);
					uint16_t __ele = *(uint16_t*)(ip + 6);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 8);
				    CHECK_NOT_NULL_THROW((*(Il2CppArray**)(localVarBase + __arr)));
				    CHECK_ARRAY_TYPE_COMPATIBLE((*(Il2CppArray**)(localVarBase + __arr)), __klass);
				    il2cpp_array_set((*(Il2CppArray**)(localVarBase + __arr)), Il2CppObject*, (*(int64_t*)(localVarBase + __index)), (*(Il2CppObject**)(localVarBase + __ele)));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::NewMdArrVarVar_length:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __lengthIdxs = *(uint16_t*)(ip + 4);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 6);
				(*(Il2CppArray**)(localVarBase + __arr)) =  NewMdArray(__klass, (il2cpp_array_size_t*)(void*)(localVarBase + __lengthIdxs), nullptr);
				    ip += 14;
				    continue;
				}
				case HiOpcodeEnum::NewMdArrVarVar_length_bound:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __lengthIdxs = *(uint16_t*)(ip + 4);
					uint16_t __lowerBoundIdxs = *(uint16_t*)(ip + 6);
					Il2CppClass* __klass = *(Il2CppClass**)(ip + 8);
				(*(Il2CppArray**)(localVarBase + __arr)) =  NewMdArray(__klass, (il2cpp_array_size_t*)(void*)(localVarBase + __lengthIdxs), (il2cpp_array_size_t*)(void*)(localVarBase + __lowerBoundIdxs));
				    ip += 16;
				    continue;
				}
				case HiOpcodeEnum::GetMdArrElementVarVar:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __lengthIdxs = *(uint16_t*)(ip + 4);
					uint16_t __value = *(uint16_t*)(ip + 6);
				    GetMdArrayElement((*(Il2CppArray**)(localVarBase + __arr)), (il2cpp_array_size_t*)(void*)(localVarBase + __lengthIdxs), (void*)(localVarBase + __value));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::GetMdArrElementAddressVarVar:
				{
					uint16_t __addr = *(uint16_t*)(ip + 2);
					uint16_t __arr = *(uint16_t*)(ip + 4);
					uint16_t __lengthIdxs = *(uint16_t*)(ip + 6);
				    (*(void**)(localVarBase + __addr)) = GetMdArrayElementAddress((*(Il2CppArray**)(localVarBase + __arr)), (il2cpp_array_size_t*)(void*)(localVarBase + __lengthIdxs));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::SetMdArrElementVarVar:
				{
					uint16_t __arr = *(uint16_t*)(ip + 2);
					uint16_t __lengthIdxs = *(uint16_t*)(ip + 4);
					uint16_t __value = *(uint16_t*)(ip + 6);
				    SetMdArrayElement((*(Il2CppArray**)(localVarBase + __arr)), (il2cpp_array_size_t*)(void*)(localVarBase + __lengthIdxs), (void*)(localVarBase + __value));
				    ip += 8;
				    continue;
				}

				//!!!}}ARRAY
#pragma endregion

#pragma region EXCEPTION
		//!!!{{EXCEPTION
				case HiOpcodeEnum::ThrowEx:
				{
					uint16_t __exceptionObj = *(uint16_t*)(ip + 2);
					THROW_EX((Il2CppException*)(*(Il2CppObject**)(localVarBase + __exceptionObj)));
				    continue;
				}
				case HiOpcodeEnum::RethrowEx:
				{
					RETHROW_EX();
				    continue;
				}
				case HiOpcodeEnum::LeaveEx:
				{
					int32_t __offset = *(int32_t*)(ip + 2);
					LEAVE_EX(__offset);
				    continue;
				}
				case HiOpcodeEnum::EndFilterEx:
				{
					uint16_t __value = *(uint16_t*)(ip + 2);
					ENDFILTER_EX((*(bool*)(localVarBase + __value)));
				    ip += 4;
				    continue;
				}
				case HiOpcodeEnum::EndFinallyEx:
				{
					ENDFINALLY_EX();
				    continue;
				}

				//!!!}}EXCEPTION
#pragma endregion

#pragma region instrinct
		//!!!{{INSTRINCT
				case HiOpcodeEnum::NullableNewVarVar:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __data = *(uint16_t*)(ip + 4);
					uint32_t __size = *(uint32_t*)(ip + 6);
				    NewNullableValueType((void*)(localVarBase + __dst), (void*)(localVarBase + __data), __size);
				    ip += 10;
				    continue;
				}
				case HiOpcodeEnum::NullableCtorVarVar:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __data = *(uint16_t*)(ip + 4);
					uint32_t __size = *(uint32_t*)(ip + 6);
				    InitNullableValueType((*(void**)(localVarBase + __dst)), (void*)(localVarBase + __data), __size);
				    ip += 10;
				    continue;
				}
				case HiOpcodeEnum::NullableHasValueVar:
				{
					uint16_t __result = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint32_t __offset = *(uint32_t*)(ip + 6);
				    (*(uint32_t*)(localVarBase + __result)) = IsNullableHasValue((*(void**)(localVarBase + __obj)), __offset);
				    ip += 10;
				    continue;
				}
				case HiOpcodeEnum::NullableGetValueOrDefaultVarVar:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint32_t __size = *(uint32_t*)(ip + 6);
				    GetNullableValueOrDefault((void*)(localVarBase + __dst), (*(void**)(localVarBase + __obj)), __size);
				    ip += 10;
				    continue;
				}
				case HiOpcodeEnum::NullableGetValueOrDefaultVarVar_1:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint16_t __defaultValue = *(uint16_t*)(ip + 6);
					uint32_t __size = *(uint32_t*)(ip + 8);
				    GetNullableValueOrDefault((void*)(localVarBase + __dst), (*(void**)(localVarBase + __obj)), (void*)(localVarBase + __defaultValue), __size);
				    ip += 12;
				    continue;
				}
				case HiOpcodeEnum::NullableGetValueVarVar:
				{
					uint16_t __dst = *(uint16_t*)(ip + 2);
					uint16_t __obj = *(uint16_t*)(ip + 4);
					uint32_t __size = *(uint32_t*)(ip + 6);
				    GetNullableValue((void*)(localVarBase + __dst), (*(void**)(localVarBase + __obj)), __size);
				    ip += 10;
				    continue;
				}
				case HiOpcodeEnum::InterlockedCompareExchangeVarVarVarVar_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __localtion = *(uint16_t*)(ip + 4);
					uint16_t __value = *(uint16_t*)(ip + 6);
					uint16_t __comparand = *(uint16_t*)(ip + 8);
				    (*(int32_t*)(localVarBase + __ret)) = HiInterlockedCompareExchange((int32_t*)(*(void**)(localVarBase + __localtion)), (*(int32_t*)(localVarBase + __value)), (*(int32_t*)(localVarBase + __comparand)));
				    ip += 10;
				    continue;
				}
				case HiOpcodeEnum::InterlockedCompareExchangeVarVarVarVar_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __localtion = *(uint16_t*)(ip + 4);
					uint16_t __value = *(uint16_t*)(ip + 6);
					uint16_t __comparand = *(uint16_t*)(ip + 8);
				    (*(int64_t*)(localVarBase + __ret)) = HiInterlockedCompareExchange((int64_t*)(*(void**)(localVarBase + __localtion)), (*(int64_t*)(localVarBase + __value)), (*(int64_t*)(localVarBase + __comparand)));
				    ip += 10;
				    continue;
				}
				case HiOpcodeEnum::InterlockedCompareExchangeVarVarVarVar_pointer:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __localtion = *(uint16_t*)(ip + 4);
					uint16_t __value = *(uint16_t*)(ip + 6);
					uint16_t __comparand = *(uint16_t*)(ip + 8);
				    (*(void**)(localVarBase + __ret)) = HiInterlockedCompareExchange((void**)(*(void**)(localVarBase + __localtion)), (*(void**)(localVarBase + __value)), (*(void**)(localVarBase + __comparand)));
				    ip += 10;
				    continue;
				}
				case HiOpcodeEnum::InterlockedExchangeVarVarVar_i4:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __localtion = *(uint16_t*)(ip + 4);
					uint16_t __value = *(uint16_t*)(ip + 6);
				    (*(int32_t*)(localVarBase + __ret)) = HiInterlockedExchange((int32_t*)(*(void**)(localVarBase + __localtion)), (*(int32_t*)(localVarBase + __value)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::InterlockedExchangeVarVarVar_i8:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __localtion = *(uint16_t*)(ip + 4);
					uint16_t __value = *(uint16_t*)(ip + 6);
				    (*(int64_t*)(localVarBase + __ret)) = HiInterlockedExchange((int64_t*)(*(void**)(localVarBase + __localtion)), (*(int64_t*)(localVarBase + __value)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::InterlockedExchangeVarVarVar_pointer:
				{
					uint16_t __ret = *(uint16_t*)(ip + 2);
					uint16_t __localtion = *(uint16_t*)(ip + 4);
					uint16_t __value = *(uint16_t*)(ip + 6);
				    (*(void**)(localVarBase + __ret)) = HiInterlockedExchange((void**)(*(void**)(localVarBase + __localtion)), (*(void**)(localVarBase + __value)));
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::NewSystemObjectVar:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
				(*(Il2CppObject**)(localVarBase + __obj)) = il2cpp::vm::Object::New(il2cpp_defaults.object_class);
				    ip += 4;
				    continue;
				}
				case HiOpcodeEnum::NewVector2VarVarVar:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __x = *(uint16_t*)(ip + 4);
					uint16_t __y = *(uint16_t*)(ip + 6);
				    *(HtVector2*)(*(void**)(localVarBase + __obj)) = {(*(float*)(localVarBase + __x)), (*(float*)(localVarBase + __y))};
				    ip += 8;
				    continue;
				}
				case HiOpcodeEnum::NewVector3VarVarVarVar:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __x = *(uint16_t*)(ip + 4);
					uint16_t __y = *(uint16_t*)(ip + 6);
					uint16_t __z = *(uint16_t*)(ip + 8);
				    *(HtVector3*)(*(void**)(localVarBase + __obj)) = {(*(float*)(localVarBase + __x)), (*(float*)(localVarBase + __y)), (*(float*)(localVarBase + __z))};
				    ip += 10;
				    continue;
				}
				case HiOpcodeEnum::NewVector4VarVarVarVarVar:
				{
					uint16_t __obj = *(uint16_t*)(ip + 2);
					uint16_t __x = *(uint16_t*)(ip + 4);
					uint16_t __y = *(uint16_t*)(ip + 6);
					uint16_t __z = *(uint16_t*)(ip + 8);
					uint16_t __w = *(uint16_t*)(ip + 10);
				    *(HtVector4*)(*(void**)(localVarBase + __obj)) = {(*(float*)(localVarBase + __x)), (*(float*)(localVarBase + __y)), (*(float*)(localVarBase + __z)), (*(float*)(localVarBase + __w))};
				    ip += 12;
				    continue;
				}

				//!!!}}INSTRINCT
#pragma endregion
				default:
					break;
				}
			}
		ExitEvalLoop:;
		}
		catch (Il2CppExceptionWrapper ex)
		{
			curException = ex.ex;

			PREPARE_EXCEPTION();
			FIND_NEXT_EX_HANDLER_OR_UNWIND();
		}
		catch (Il2CppException* ex)
		{
			curException = ex;

			PREPARE_EXCEPTION();
			FIND_NEXT_EX_HANDLER_OR_UNWIND();
		}
		return;
	UnWindFail:
		IL2CPP_ASSERT(curException);
		interpFrameGroup.CleanUpFrames();
		il2cpp::vm::Exception::Raise(curException);
	}


}
}

