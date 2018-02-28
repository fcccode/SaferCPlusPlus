
// Copyright (c) 2015 Noah Lopez
// Use, modification, and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef MSEMSEARRAY_H
#define MSEMSEARRAY_H

/*compiler specific defines*/
#ifdef _MSC_VER
#if (1700 > _MSC_VER)
#define MSVC2010_COMPATIBLE 1
#endif /*(1700 > _MSC_VER)*/
#if (1900 > _MSC_VER)
#define MSVC2013_COMPATIBLE 1
#endif /*(1900 > _MSC_VER)*/
#else /*_MSC_VER*/
#if (defined(__GNUC__) || defined(__GNUG__))
#define GPP_COMPATIBLE 1
#if ((5 > __GNUC__) && (!defined(__clang__)))
#define GPP4P8_COMPATIBLE 1
#endif /*((5 > __GNUC__) && (!defined(__clang__)))*/
#endif
#endif /*_MSC_VER*/

//define MSE_MSEARRAY_USE_MSE_PRIMITIVES 1
#ifdef MSE_MSEARRAY_USE_MSE_PRIMITIVES
#include "mseprimitives.h"
#endif // MSE_MSEARRAY_USE_MSE_PRIMITIVES

#include "msescope.h"
#include "mseoptional.h"
#include <array>
#include <assert.h>
#include <memory>
#include <unordered_map>
#include <functional>
#include <tuple>
#include <climits>       // ULONG_MAX
#include <stdexcept>
#include <type_traits>
#include <shared_mutex>
#ifdef MSE_SELF_TESTS
#include <iostream>
#include <string>
#include <algorithm>
#include <iterator>
#endif // MSE_SELF_TESTS

#ifdef _MSC_VER
#pragma warning( push )  
#pragma warning( disable : 4100 4456 4189 )
#endif /*_MSC_VER*/

#ifdef MSE_CUSTOM_THROW_DEFINITION
#include <iostream>
#define MSE_THROW(x) MSE_CUSTOM_THROW_DEFINITION(x)
#else // MSE_CUSTOM_THROW_DEFINITION
#define MSE_THROW(x) throw(x)
#endif // MSE_CUSTOM_THROW_DEFINITION

namespace mse {

#ifndef _STD
#define _STD std::
#endif /*_STD*/

#ifndef _NOEXCEPT
#define _NOEXCEPT
#endif /*_NOEXCEPT*/

#ifndef _NOEXCEPT_OP
#define _NOEXCEPT_OP(x)	noexcept(x)
#endif /*_NOEXCEPT_OP*/

#ifndef _THROW_NCEE
#define _THROW_NCEE(x, y)	MSE_THROW(x(y))
#endif /*_THROW_NCEE*/

#ifdef MSE_MSEARRAY_USE_MSE_PRIMITIVES
	typedef mse::CSize_t msear_size_t;
	typedef mse::CInt msear_int;
	typedef bool msear_bool; // no added safety benefit to using mse::CBool in this case
	#define msear_as_a_size_t as_a_size_t
#else // MSE_MSEARRAY_USE_MSE_PRIMITIVES
#if SIZE_MAX <= ULONG_MAX
#define MSE_MSEARRAY_BASE_INTEGER_TYPE long int
#else // SIZE_MAX <= ULONG_MAX
#define MSE_MSEARRAY_BASE_INTEGER_TYPE long long int
#endif // SIZE_MAX <= ULONG_MAX

	typedef size_t msear_size_t;
	typedef MSE_MSEARRAY_BASE_INTEGER_TYPE msear_int;
	typedef bool msear_bool;
	typedef size_t msear_as_a_size_t;
#endif // MSE_MSEARRAY_USE_MSE_PRIMITIVES

	class nii_array_range_error : public std::range_error {
	public:
		using std::range_error::range_error;
	};
	class nii_array_null_dereference_error : public std::logic_error {
	public:
		using std::logic_error::logic_error;
	};
	class msearray_range_error : public std::range_error { public:
		using std::range_error::range_error;
	};
	class msearray_null_dereference_error : public std::logic_error { public:
		using std::logic_error::logic_error;
	};

	/* msear_pointer behaves similar to native pointers. It's a bit safer in that it initializes to
	nullptr by default and checks for attempted dereference of null pointers. */
	template<typename _Ty>
	class msear_pointer {
	public:
		msear_pointer() : m_ptr(nullptr) {}
		msear_pointer(_Ty* ptr) : m_ptr(ptr) {}
		//msear_pointer(const msear_pointer<_Ty>& src) : m_ptr(src.m_ptr) {}
		template<class _Ty2, class = typename std::enable_if<
			std::is_same<_Ty2, _Ty>::value || ((!std::is_const<_Ty2>::value) && std::is_same<const _Ty2, _Ty>::value)
			, void>::type>
		msear_pointer(const msear_pointer<_Ty2>& src) : m_ptr(src.m_ptr) {}

		_Ty& operator*() const {
#ifndef MSE_DISABLE_MSEAR_POINTER_CHECKS
			if (nullptr == m_ptr) { MSE_THROW(msearray_null_dereference_error("attempt to dereference null pointer - mse::msear_pointer")); }
#endif /*MSE_DISABLE_MSEAR_POINTER_CHECKS*/
			return (*m_ptr);
		}
		_Ty* operator->() const {
#ifndef MSE_DISABLE_MSEAR_POINTER_CHECKS
			if (nullptr == m_ptr) { MSE_THROW(msearray_null_dereference_error("attempt to dereference null pointer - mse::msear_pointer")); }
#endif /*MSE_DISABLE_MSEAR_POINTER_CHECKS*/
			return m_ptr;
		}

		msear_pointer<_Ty>& operator=(_Ty* ptr) {
			m_ptr = ptr;
			return (*this);
		}
		template<class _Ty2, class = typename std::enable_if<
			std::is_same<_Ty2, _Ty>::value
			|| ((!std::is_const<_Ty2>::value) && std::is_same<const _Ty2, _Ty>::value)
			, void>::type>
		msear_pointer<_Ty>& operator=(const msear_pointer<_Ty2>& src) {
			m_ptr = src.m_ptr;
			return (*this);
		}

		bool operator==(const msear_pointer _Right_cref) const { return (_Right_cref.m_ptr == m_ptr); }
		bool operator!=(const msear_pointer _Right_cref) const { return (!((*this) == _Right_cref)); }
		//bool operator==(const _Ty* _Right_cref) const { return (_Right_cref == m_ptr); }
		//bool operator!=(const _Ty* _Right_cref) const { return (!((*this) == _Right_cref)); }

		bool operator!() const { return (!m_ptr); }
		operator bool() const { return (m_ptr != nullptr); }

		explicit operator _Ty*() const { return m_ptr; }

		void not_async_shareable_tag() const {} /* Indication that this type is not eligible to be shared between threads. */

		_Ty* m_ptr;
	};

	class non_thread_safe_mutex {
	public:
		void lock() {	// lock exclusive
			if (m_is_locked) {
				MSE_THROW(std::system_error(std::make_error_code(std::errc::resource_deadlock_would_occur)));
			}
			m_is_locked = true;
		}
		bool try_lock() {	// try to lock exclusive
			if (m_is_locked) {
				return false;
			}
			else
			{
				m_is_locked = true;
				return true;
			}
		}
		template<class _Rep, class _Period>
		bool try_lock_for(const std::chrono::duration<_Rep, _Period>& _Rel_time) {	// try to lock for duration
			return (try_lock_until(std::chrono::steady_clock::now() + _Rel_time));
		}
		template<class _Clock, class _Duration>
		bool try_lock_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time) {	// try to lock until time point
			return try_lock();
		}
		void unlock() {	// unlock exclusive
			assert(m_is_locked);
			m_is_locked = false;
		}

		bool m_is_locked = false;
	};

	class non_thread_safe_shared_mutex {
	public:
		void lock() {	// lock exclusive
			if (m_is_exclusive_locked || (1 <= m_shared_lock_count)) {
				MSE_THROW(std::system_error(std::make_error_code(std::errc::resource_deadlock_would_occur)));
			}
			m_is_exclusive_locked = true;
		}
		bool try_lock() {	// try to lock exclusive
			bool retval = true;
			if (m_is_exclusive_locked || (1 <= m_shared_lock_count)) {
				retval = false;
			}
			else {
				m_is_exclusive_locked = true;
			}
			return retval;
		}
		template<class _Rep, class _Period>
		bool try_lock_for(const std::chrono::duration<_Rep, _Period>& _Rel_time) {	// try to lock for duration
			return (try_lock_until(std::chrono::steady_clock::now() + _Rel_time));
		}
		template<class _Clock, class _Duration>
		bool try_lock_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time) {	// try to lock until time point
			return try_lock();
		}
		void unlock() {	// unlock exclusive
			assert(m_is_exclusive_locked && (0 == m_shared_lock_count));
			m_is_exclusive_locked = false;
		}
		void lock_shared() {	// lock non-exclusive
			if (m_is_exclusive_locked) {
				MSE_THROW(std::system_error(std::make_error_code(std::errc::resource_deadlock_would_occur)));
			}
			m_shared_lock_count += 1;
		}
		bool try_lock_shared() {	// try to lock non-exclusive
			bool retval = true;
			if (m_is_exclusive_locked) {
				retval = false;
			}
			else {
				m_shared_lock_count += 1;
			}
			return retval;
		}
		template<class _Rep, class _Period>
		bool try_lock_shared_for(const std::chrono::duration<_Rep, _Period>& _Rel_time) {	// try to lock non-exclusive for relative time
			return (try_lock_shared_until(_Rel_time + std::chrono::steady_clock::now()));
		}
		template<class _Clock, class _Duration>
		bool try_lock_shared_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time) {	// try to lock non-exclusive until absolute time
			return try_lock_shared();
		}
		void unlock_shared() {	// unlock non-exclusive
			assert((1 <= m_shared_lock_count) && (!m_is_exclusive_locked));
			m_shared_lock_count -= 1;
		}

		bool m_is_exclusive_locked = false;
		size_t m_shared_lock_count = 0;
	};

	class non_thread_safe_recursive_shared_timed_mutex : public non_thread_safe_shared_mutex {
	public:
		typedef non_thread_safe_shared_mutex base_class;

		void lock() {
			base_class::lock_shared();
		}
		bool try_lock() {
			return base_class::try_lock_shared();
		}
		template<class _Rep, class _Period>
		bool try_lock_for(const std::chrono::duration<_Rep, _Period>& _Rel_time) {
			return (try_lock_until(std::chrono::steady_clock::now() + _Rel_time));
		}
		template<class _Clock, class _Duration>
		bool try_lock_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time) {
			return base_class::try_lock_shared_until(_Abs_time);
		}
		void unlock() {
			base_class::unlock_shared();
		}

		void nonrecursive_lock() {
			base_class::lock();
		}
		bool try_nonrecursive_lock() {
			return base_class::try_lock();
		}
		template<class _Rep, class _Period>
		bool try_nonrecursive_lock_for(const std::chrono::duration<_Rep, _Period>& _Rel_time) {
			return (try_nonrecursive_lock_until(std::chrono::steady_clock::now() + _Rel_time));
		}
		template<class _Clock, class _Duration>
		bool try_nonrecursive_lock_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time) {
			return base_class::try_lock_until(_Abs_time);
		}
		void nonrecursive_unlock() {
			base_class::unlock();
		}
	};

	class dummy_recursive_shared_timed_mutex {
	public:
		void lock() {
		}
		bool try_lock() {
			return true;
		}
		template<class _Rep, class _Period>
		bool try_lock_for(const std::chrono::duration<_Rep, _Period>& _Rel_time) {	// try to lock for duration
			return (try_lock_until(std::chrono::steady_clock::now() + _Rel_time));
		}
		template<class _Clock, class _Duration>
		bool try_lock_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time) {	// try to lock until time point
			return try_lock();
		}
		void unlock() {
		}
		void nonrecursive_lock() {
			lock();
		}
		bool try_nonrecursive_lock() {	// try to lock nonrecursive
			return try_lock();
		}
		template<class _Rep, class _Period>
		bool try_nonrecursive_lock_for(const std::chrono::duration<_Rep, _Period>& _Rel_time) {	// try to nonrecursive lock for duration
			return (try_nonrecursive_lock_until(std::chrono::steady_clock::now() + _Rel_time));
		}
		template<class _Clock, class _Duration>
		bool try_nonrecursive_lock_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time) {	// try to nonrecursive lock until time point
			return try_lock_until(_Abs_time);
		}
		void nonrecursive_unlock() {
			unlock();
		}
		void lock_shared() {	// lock non-exclusive
		}
		bool try_lock_shared() {	// try to lock non-exclusive
			return true;
		}
		template<class _Rep, class _Period>
		bool try_lock_shared_for(const std::chrono::duration<_Rep, _Period>& _Rel_time) {	// try to lock non-exclusive for relative time
			return (try_lock_shared_until(_Rel_time + std::chrono::steady_clock::now()));
		}
		template<class _Clock, class _Duration>
		bool try_lock_shared_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time) {	// try to lock non-exclusive until absolute time
			return try_lock_shared();
		}
		void unlock_shared() {	// unlock non-exclusive
		}
	};


	template<typename T>
	struct HasNonrecursiveUnlockMethod_msemsearray {
		template<typename U, void(U::*)() const> struct SFINAE {};
		template<typename U> static char Test(SFINAE<U, &U::nonrecursive_unlock>*);
		template<typename U> static int Test(...);
		static const bool Has = (sizeof(Test<T>(0)) == sizeof(char));
	};

	template<typename T>
	struct HasUnlockSharedMethod_msemsearray {
		template<typename U, void(U::*)() const> struct SFINAE {};
		template<typename U> static char Test(SFINAE<U, &U::unlock_shared>*);
		template<typename U> static int Test(...);
		static const bool Has = (sizeof(Test<T>(0)) == sizeof(char));
	};

	template<class _Mutex>
	class recursive_shared_mutex_wrapped : public _Mutex {
	public:
		typedef _Mutex base_class;

		void nonrecursive_lock() {
			nonrecursive_lock_helper(typename std::conditional<HasNonrecursiveUnlockMethod_msemsearray<_Mutex>::Has, std::true_type, std::false_type>::type());
		}
		bool try_nonrecursive_lock() {	// try to lock nonrecursive
			return nonrecursive_try_lock_helper(typename std::conditional<HasNonrecursiveUnlockMethod_msemsearray<_Mutex>::Has, std::true_type, std::false_type>::type());
		}
		void nonrecursive_unlock() {
			nonrecursive_unlock_helper(typename std::conditional<HasNonrecursiveUnlockMethod_msemsearray<_Mutex>::Has, std::true_type, std::false_type>::type());
		}

		void lock_shared() {	// lock non-exclusive
			lock_shared_helper(typename std::conditional<HasUnlockSharedMethod_msemsearray<_Mutex>::Has, std::true_type, std::false_type>::type());
		}
		bool try_lock_shared() {	// try to lock non-exclusive
			return try_lock_shared_helper(typename std::conditional<HasUnlockSharedMethod_msemsearray<_Mutex>::Has, std::true_type, std::false_type>::type());
		}
		void unlock_shared() {	// unlock non-exclusive
			unlock_shared_helper(typename std::conditional<HasUnlockSharedMethod_msemsearray<_Mutex>::Has, std::true_type, std::false_type>::type());
		}
	private:
		void nonrecursive_lock_helper(std::true_type) {
			base_class::nonrecursive_lock();
		}
		void nonrecursive_lock_helper(std::false_type) {
			base_class::lock();
		}
		bool nonrecursive_try_lock_helper(std::true_type) {
			return base_class::nonrecursive_try_lock();
		}
		bool nonrecursive_try_lock_helper(std::false_type) {
			return base_class::try_lock();
		}
		void nonrecursive_unlock_helper(std::true_type) {
			base_class::nonrecursive_unlock();
		}
		void nonrecursive_unlock_helper(std::false_type) {
			base_class::unlock();
		}

		void lock_shared_helper(std::true_type) {
			base_class::lock_shared();
		}
		void lock_shared_helper(std::false_type) {
			base_class::lock();
		}
		bool try_lock_shared_helper(std::true_type) {
			return base_class::try_lock_shared();
		}
		bool try_lock_shared_helper(std::false_type) {
			return base_class::try_lock();
		}
		void unlock_shared_helper(std::true_type) {
			base_class::unlock_shared();
		}
		void unlock_shared_helper(std::false_type) {
			base_class::unlock();
		}

	};


	template<class _Mutex>
	class unique_nonrecursive_lock
	{	// a version of std::unique_lock that calls "nonrecursive_lock()" instead of "lock()"
	public:
		typedef unique_nonrecursive_lock<_Mutex> _Myt;
		typedef _Mutex mutex_type;

		// CONSTRUCT, ASSIGN, AND DESTROY
		unique_nonrecursive_lock() _NOEXCEPT
			: _Pmtx(0), _Owns(false)
		{	// default construct
		}

		explicit unique_nonrecursive_lock(_Mutex& _Mtx)
			: _Pmtx(&_Mtx), _Owns(false)
		{	// construct and lock
			_Pmtx->nonrecursive_lock();
			_Owns = true;
		}

		unique_nonrecursive_lock(_Mutex& _Mtx, std::adopt_lock_t)
			: _Pmtx(&_Mtx), _Owns(true)
		{	// construct and assume already locked
		}

		unique_nonrecursive_lock(_Mutex& _Mtx, std::defer_lock_t) _NOEXCEPT
			: _Pmtx(&_Mtx), _Owns(false)
		{	// construct but don't lock
		}

		unique_nonrecursive_lock(_Mutex& _Mtx, std::try_to_lock_t)
			: _Pmtx(&_Mtx), _Owns(_Pmtx->try_nonrecursive_lock())
		{	// construct and try to lock
		}

		template<class _Rep,
			class _Period>
			unique_nonrecursive_lock(_Mutex& _Mtx,
				const std::chrono::duration<_Rep, _Period>& _Rel_time)
			: _Pmtx(&_Mtx), _Owns(_Pmtx->try_nonrecursive_lock_for(_Rel_time))
		{	// construct and lock with timeout
		}

		template<class _Clock,
			class _Duration>
			unique_nonrecursive_lock(_Mutex& _Mtx,
				const std::chrono::time_point<_Clock, _Duration>& _Abs_time)
			: _Pmtx(&_Mtx), _Owns(_Pmtx->try_nonrecursive_lock_until(_Abs_time))
		{	// construct and lock with timeout
		}

		unique_nonrecursive_lock(unique_nonrecursive_lock&& _Other) _NOEXCEPT
			: _Pmtx(_Other._Pmtx), _Owns(_Other._Owns)
		{	// destructive copy
			_Other._Pmtx = 0;
			_Other._Owns = false;
		}

		unique_nonrecursive_lock& operator=(unique_nonrecursive_lock&& _Other)
		{	// destructive copy
			if (this != &_Other)
			{	// different, move contents
				if (_Owns)
					_Pmtx->nonrecursive_unlock();
				_Pmtx = _Other._Pmtx;
				_Owns = _Other._Owns;
				_Other._Pmtx = 0;
				_Other._Owns = false;
			}
			return (*this);
		}

		~unique_nonrecursive_lock() _NOEXCEPT
		{	// clean up
			if (_Owns)
				_Pmtx->nonrecursive_unlock();
		}

		unique_nonrecursive_lock(const unique_nonrecursive_lock&) = delete;
		unique_nonrecursive_lock& operator=(const unique_nonrecursive_lock&) = delete;

		// LOCK AND UNLOCK
		void lock()
		{	// lock the mutex
			_Validate();
			_Pmtx->nonrecursive_lock();
			_Owns = true;
		}

		bool try_lock()
		{	// try to lock the mutex
			_Validate();
			_Owns = _Pmtx->try_nonrecursive_lock();
			return (_Owns);
		}

		template<class _Rep,
			class _Period>
			bool try_lock_for(const std::chrono::duration<_Rep, _Period>& _Rel_time)
		{	// try to lock mutex for _Rel_time
			_Validate();
			_Owns = _Pmtx->try_nonrecursive_lock_for(_Rel_time);
			return (_Owns);
		}

		template<class _Clock,
			class _Duration>
			bool try_lock_until(
				const std::chrono::time_point<_Clock, _Duration>& _Abs_time)
		{	// try to lock mutex until _Abs_time
			_Validate();
			_Owns = _Pmtx->try_nonrecursive_lock_until(_Abs_time);
			return (_Owns);
		}

		void unlock()
		{	// try to unlock the mutex
			if (!_Pmtx || !_Owns)
				_THROW_NCEE(std::system_error,
					_STD make_error_code(std::errc::operation_not_permitted));

			_Pmtx->nonrecursive_unlock();
			_Owns = false;
		}

		// MUTATE
		void swap(unique_nonrecursive_lock& _Other) _NOEXCEPT
		{	// swap with _Other
			_STD swap(_Pmtx, _Other._Pmtx);
			_STD swap(_Owns, _Other._Owns);
		}

		_Mutex *release() _NOEXCEPT
		{	// disconnect
			_Mutex *_Res = _Pmtx;
			_Pmtx = 0;
			_Owns = false;
			return (_Res);
		}

		// OBSERVE
		bool owns_lock() const _NOEXCEPT
		{	// return true if this object owns the lock
			return (_Owns);
		}

		explicit operator bool() const _NOEXCEPT
		{	// return true if this object owns the lock
			return (_Owns);
		}

		_Mutex *mutex() const _NOEXCEPT
		{	// return pointer to managed mutex
			return (_Pmtx);
		}

	private:
		_Mutex *_Pmtx;
		bool _Owns;

		void _Validate() const
		{	// check if the mutex can be locked
			if (!_Pmtx)
				_THROW_NCEE(std::system_error,
					_STD make_error_code(std::errc::operation_not_permitted));

			if (_Owns)
				_THROW_NCEE(std::system_error,
					_STD make_error_code(std::errc::resource_deadlock_would_occur));
		}
	};

#ifndef _XSTD
#define _XSTD ::std::
#endif /*_XSTD*/

#ifndef _STD
#define _STD std::
#endif /*_STD*/

#ifndef _NOEXCEPT
#define _NOEXCEPT
#endif /*_NOEXCEPT*/

#ifndef _CONST_FUN
#define _CONST_FUN constexpr
#endif /*_CONST_FUN*/

	/* Some iterators are prone to having their target container prematurely deallocated out from under them. If you have a safe pointer
	to the target container, you can use TSyncWeakFixedIterator<> as a safe iterator that welds a copy of the safe pointer (aka "lease")
	to the iterator. */
	template <class _TIterator, class _TLeasePointer>
	class TSyncWeakFixedIterator : public _TIterator {
	public:
		typedef _TIterator base_class;
		TSyncWeakFixedIterator(const TSyncWeakFixedIterator&) = default;
		template<class _TLeasePointer2, class = typename std::enable_if<std::is_convertible<_TLeasePointer2, _TLeasePointer>::value, void>::type>
		TSyncWeakFixedIterator(const TSyncWeakFixedIterator<_TIterator, _TLeasePointer2>&src) : base_class(src), m_lease_pointer(src.lease_pointer()) {}

		auto operator*() const -> typename std::add_lvalue_reference<decltype(*((*this).m_target_pointer))>::type {
			/*const auto &test_cref =*/ *m_lease_pointer; // this should throw if m_lease_pointer is no longer valid
			return (*((*this).m_target_pointer));
		}
		auto operator->() const -> decltype(std::addressof(*((*this).m_target_pointer))) {
			/*const auto &test_cref =*/ *m_lease_pointer; // this should throw if m_lease_pointer is no longer valid
			return std::addressof(*((*this).m_target_pointer));
		}

		_TLeasePointer lease_pointer() const { return (*this).m_lease_pointer; }

		template <class _TIterator2, class _TLeasePointer2>
		static TSyncWeakFixedIterator make(const _TIterator2& src_iterator, const _TLeasePointer2& lease_pointer) {
			return TSyncWeakFixedIterator(src_iterator, lease_pointer);
		}
		void not_async_shareable_tag() const {} /* Indication that this type is not eligible to be shared between threads. */

	protected:
		TSyncWeakFixedIterator(const _TIterator& src_iterator, const _TLeasePointer& lease_pointer/* often a registered pointer */)
			: base_class(src_iterator), m_lease_pointer(lease_pointer) {}
	private:
		TSyncWeakFixedIterator& operator=(const TSyncWeakFixedIterator& _Right_cref) = delete;

		_TLeasePointer m_lease_pointer;

		//friend class TSyncWeakFixedConstIterator<_TIterator, _TLeasePointer>;
	};

	template <class _Ty, class _TLeasePointer>
	class TSyncWeakFixedIterator<_Ty*, _TLeasePointer> : public TSaferPtrForLegacy<_Ty> {
	public:
		typedef TSaferPtrForLegacy<_Ty> _TIterator;
		typedef _TIterator base_class;
		TSyncWeakFixedIterator(const TSyncWeakFixedIterator&) = default;
		template<class _TLeasePointer2, class = typename std::enable_if<std::is_convertible<_TLeasePointer2, _TLeasePointer>::value, void>::type>
		TSyncWeakFixedIterator(const TSyncWeakFixedIterator<_TIterator, _TLeasePointer2>&src) : base_class(src), m_lease_pointer(src.lease_pointer()) {}

		auto operator*() const -> typename std::add_lvalue_reference<decltype(*((*this).m_target_pointer))>::type {
			/*const auto &test_cref =*/ *m_lease_pointer; // this should throw if m_lease_pointer is no longer valid
			return (*((*this).m_target_pointer));
		}
		auto operator->() const -> decltype(std::addressof(*((*this).m_target_pointer))) {
			/*const auto &test_cref =*/ *m_lease_pointer; // this should throw if m_lease_pointer is no longer valid
			return std::addressof(*((*this).m_target_pointer));
		}

		_TLeasePointer lease_pointer() const { return (*this).m_lease_pointer; }

		template <class _TIterator2, class _TLeasePointer2>
		static TSyncWeakFixedIterator make(const _TIterator2& src_iterator, const _TLeasePointer2& lease_pointer) {
			return TSyncWeakFixedIterator(src_iterator, lease_pointer);
		}
		void not_async_shareable_tag() const {} /* Indication that this type is not eligible to be shared between threads. */

	protected:
		TSyncWeakFixedIterator(const _TIterator& src_iterator, const _TLeasePointer& lease_pointer/* often a registered pointer */)
			: base_class(src_iterator), m_lease_pointer(lease_pointer) {}
		TSyncWeakFixedIterator(const _Ty* & src_iterator, const _TLeasePointer& lease_pointer/* often a registered pointer */)
			: base_class(TSaferPtrForLegacy<_Ty>(src_iterator)), m_lease_pointer(lease_pointer) {}
	private:
		TSyncWeakFixedIterator& operator=(const TSyncWeakFixedIterator& _Right_cref) = delete;

		_TLeasePointer m_lease_pointer;

		//friend class TSyncWeakFixedConstIterator<_TIterator, _TLeasePointer>;
	};

	template <class _TIterator, class _TLeasePointer>
	TSyncWeakFixedIterator<_TIterator, _TLeasePointer> make_syncweak_iterator(const _TIterator& src_iterator, const _TLeasePointer& lease_pointer) {
		return TSyncWeakFixedIterator<_TIterator, _TLeasePointer>::make(src_iterator, lease_pointer);
	}


	/* Some data structures to determine, at compile time, if a given type has certain features. */

	template<typename T>
	struct HasNotAsyncShareableTagMethod_msemsearray
	{
		template<typename U, void(U::*)() const> struct SFINAE {};
		template<typename U> static char Test(SFINAE<U, &U::not_async_shareable_tag>*);
		template<typename U> static int Test(...);
		static const bool Has = (sizeof(Test<T>(0)) == sizeof(char));
	};

	template<typename T>
	struct HasAsyncShareableTagMethod_msemsearray
	{
		template<typename U, void(U::*)() const> struct SFINAE {};
		template<typename U> static char Test(SFINAE<U, &U::async_shareable_tag>*);
		template<typename U> static int Test(...);
		static const bool Has = (sizeof(Test<T>(0)) == sizeof(char));
	};

	template<class T, class EqualTo>
	struct HasOrInheritsAssignmentOperator_msemsearray_impl
	{
		template<class U, class V>
		static auto test(U*) -> decltype(std::declval<U>() = std::declval<V>(), bool(true));
		template<typename, typename>
		static auto test(...)->std::false_type;

		using type = typename std::is_same<bool, decltype(test<T, EqualTo>(0))>::type;
	};
	template<class T, class EqualTo = T>
	struct HasOrInheritsAssignmentOperator_msemsearray : HasOrInheritsAssignmentOperator_msemsearray_impl<
		typename std::remove_reference<T>::type, typename std::remove_reference<EqualTo>::type>::type {};

	template<class T, class EqualTo>
	struct HasOrInheritsEqualityOperator_msemsearray_impl
	{
		template<class U, class V>
		static auto test(U*) -> decltype(std::declval<U>() == std::declval<V>(), bool(true));
		template<typename, typename>
		static auto test(...)->std::false_type;

		using type = typename std::is_same<bool, decltype(test<T, EqualTo>(0))>::type;
	};
	template<class T, class EqualTo = T>
	struct HasOrInheritsEqualityOperator_msemsearray : HasOrInheritsEqualityOperator_msemsearray_impl<
		typename std::remove_reference<T>::type, typename std::remove_reference<EqualTo>::type>::type {};


	/* array_helper_type is a template class used to help our (non-aggregate) array types support initializer lists
	of non-default constructible elements. This array_helper_type template has many template specializations. */

	template<class _Ty, size_t _Size>
	class array_helper_type {
	public:
		static typename std::array<_Ty, _Size> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			/* Template specializations of this function construct mse::msearrays of non-default constructible
			elements. This (non-specialized) implementation here should cause a compile error when invoked. */
			if (0 < _Ilist.size()) { MSE_THROW(msearray_range_error("sorry, arrays of this size are not supported when the elements are non-default constructible - mse::mstd::array")); }
			typename std::array<_Ty, _Size> retval{};
			return retval;
		}
	};
	template<class _Ty> class array_helper_type<_Ty, 1> {
	public:
		static typename std::array<_Ty, 1> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			/* This template specialization constructs an mse::msearray of size 1 and supports non-default
			constructible elements. */
			if (1 != _Ilist.size()) { MSE_THROW(msearray_range_error("the size of the initializer list does not match the size of the array - mse::mstd::array")); }
			typename std::array<_Ty, 1> retval{ *(_Ilist.begin()) }; return retval;
		}
	};
	/* Template specializations that construct mse::msearrays of different sizes are located later in the file. */

	template<class _StateMutex>
	class destructor_lock_guard1 {
	public:
		explicit destructor_lock_guard1(_StateMutex& _Mtx) : _MyStateMutex(_Mtx) {
			try {
				_Mtx.lock();
			}
			catch (...) {
				/* It may not be safe to continue if the object is destroyed while the object state is locked (and presumably
				in use) by another part of the code. */
				std::cerr << "\n\nFatal Error: mse::destructor_lock_guard1() failed \n\n";
				std::terminate();
			}
		}
		~destructor_lock_guard1() _NOEXCEPT {
			_MyStateMutex.unlock();
		}
		destructor_lock_guard1(const destructor_lock_guard1&) = delete;
		destructor_lock_guard1& operator=(const destructor_lock_guard1&) = delete;
	private:
		_StateMutex& _MyStateMutex;
	};

	typedef 
#if !defined(NDEBUG) || defined(MSE_ENABLE_REENTRANCY_CHECKS_BY_DEFAULT)
		non_thread_safe_mutex
#else // !defined(NDEBUG) || defined(MSE_ENABLE_REENTRANCY_CHECKS_BY_DEFAULT)
		dummy_recursive_shared_timed_mutex
#endif // !defined(NDEBUG) || defined(MSE_ENABLE_REENTRANCY_CHECKS_BY_DEFAULT)
		default_state_mutex;

	namespace us {
		template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
		class msearray;
	}

	/* nii_array<> is essentially a memory-safe array that does not expose (unprotected) non-static member functions
	like begin() or end() which return (memory) unsafe iterators. It does provide static member function templates
	like ss_begin<>(...) and ss_end<>(...) which take a pointer parameter and return a (bounds-checked) iterator that
	inherits the safety of the given pointer. nii_array<> also supports "scope" iterators which are safe without any
	run-time overhead. nii_array<> is a data type that is eligible to be shared between asynchronous threads. */
	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
	class nii_array {
	public:
		typedef std::array<_Ty, _Size> std_array;
		typedef std_array _MA;
		typedef nii_array _Myt;

		nii_array() {}
		nii_array(_MA&& _X) : m_array(std::forward<decltype(_X)>(_X)) {}
		nii_array(const _MA& _X) : m_array(_X) {}
		nii_array(_Myt&& _X) : m_array(std::forward<decltype(_X.contained_array())>(_X.contained_array())) {}
		nii_array(const _Myt& _X) : m_array(_X.contained_array()) {}
		//nii_array(_XSTD initializer_list<typename _MA::base_class::value_type> _Ilist) : m_array(_Ilist) {}
		static std::array<_Ty, _Size> std_array_initial_value(std::true_type, _XSTD initializer_list<_Ty> _Ilist) {
			/* _Ty is default constructible. */
			std::array<_Ty, _Size> retval;
			assert(_Size >= _Ilist.size());
			auto stop_size = _Size;
			if (_Size > _Ilist.size()) {
				stop_size = _Ilist.size();
				/* just to make sure that all the retval elements are initialized as if by aggregate initialization. */
				retval = std::array<_Ty, _Size>{};
			}
			msear_size_t count = 0;
			auto Il_it = _Ilist.begin();
			auto target_it = retval.begin();
			for (; (count < stop_size); Il_it++, count += 1, target_it++) {
				(*target_it) = (*Il_it);
			}
			return retval;
		}
		static std::array<_Ty, _Size> std_array_initial_value(std::false_type, _XSTD initializer_list<_Ty> _Ilist) {
			/* _Ty is not default constructible. */
			return array_helper_type<_Ty, _Size>::std_array_initial_value2(_Ilist);
		}
		nii_array(_XSTD initializer_list<_Ty> _Ilist) : m_array(std_array_initial_value(std::is_default_constructible<_Ty>(), _Ilist)) {
			/* std::array<> is an "aggregate type" (basically a POD struct with no base class, constructors or private
			data members (details here: http://en.cppreference.com/w/cpp/language/aggregate_initialization)). As such,
			support for construction from initializer list is automatically generated by the compiler. Specifically,
			aggregate types support "aggregate initialization". But since mstd::array has a member with an explicitly
			defined constructor (or at least I think that's why), it is not an aggregate type and therefore doesn't
			qualify to have support for "aggregate initialization" automatically generated by the compiler. It doesn't
			seem possible to emulate full aggregate initialization compatibility, so we'll just have to do the best we
			can. */
		}

		~nii_array() {
			mse::destructor_lock_guard1<_TStateMutex> lock1(m_mutex1);

			/* This is just a no-op function that will cause a compile error when _Ty is not an eligible type. */
			valid_if_Ty_is_not_an_xscope_type();
		}

		operator _MA() const { return contained_array(); }

		_CONST_FUN typename std_array::const_reference operator[](msear_size_t _P) const {
			return (*this).at(msear_as_a_size_t(_P));
		}
		typename std_array::reference operator[](msear_size_t _P) {
			return (*this).at(msear_as_a_size_t(_P));
		}
		typename std_array::reference front() {	// return first element of mutable sequence
			if (0 == (*this).size()) { MSE_THROW(nii_array_range_error("front() on empty - typename std_array::reference front() - nii_array")); }
			return m_array.front();
		}
		_CONST_FUN typename std_array::const_reference front() const {	// return first element of nonmutable sequence
			if (0 == (*this).size()) { MSE_THROW(nii_array_range_error("front() on empty - typename std_array::const_reference front() - nii_array")); }
			return m_array.front();
		}
		typename std_array::reference back() {	// return last element of mutable sequence
			if (0 == (*this).size()) { MSE_THROW(nii_array_range_error("back() on empty - typename std_array::reference back() - nii_array")); }
			return m_array.back();
		}
		_CONST_FUN typename std_array::const_reference back() const {	// return last element of nonmutable sequence
			if (0 == (*this).size()) { MSE_THROW(nii_array_range_error("back() on empty - typename std_array::const_reference back() - nii_array")); }
			return m_array.back();
		}

		typedef typename std_array::value_type value_type;
		//typedef typename std_array::size_type size_type;
		typedef msear_size_t size_type;
		//typedef typename std_array::difference_type difference_type;
		typedef msear_int difference_type;
		typedef typename std_array::pointer pointer;
		typedef typename std_array::const_pointer const_pointer;
		typedef typename std_array::reference reference;
		typedef typename std_array::const_reference const_reference;

		typedef typename std_array::iterator iterator;
		typedef typename std_array::const_iterator const_iterator;

		typedef typename std_array::reverse_iterator reverse_iterator;
		typedef typename std_array::const_reverse_iterator const_reverse_iterator;

		void assign(const _Ty& _Value)
		{	// assign value to all elements
			std::lock_guard<_TStateMutex> lock1(m_mutex1);
			m_array.assign(_Value);
		}

		void fill(const _Ty& _Value)
		{	// assign value to all elements
			std::lock_guard<_TStateMutex> lock1(m_mutex1);
			m_array.fill(_Value);
		}

		void swap(_Myt& _Other) {	// swap contents with _Other
			std::lock_guard<_TStateMutex> lock1(m_mutex1);
			m_array.swap(_Other.m_array);
		}

		void swap(_MA& _Other) {	// swap contents with _Other
			std::lock_guard<_TStateMutex> lock1(m_mutex1);
			m_array.swap(_Other);
		}

		_CONST_FUN size_type size() const _NOEXCEPT
		{	// return length of sequence
			return m_array.size();
		}

		_CONST_FUN size_type max_size() const _NOEXCEPT
		{	// return maximum possible length of sequence
			return m_array.max_size();
		}

		_CONST_FUN bool empty() const _NOEXCEPT
		{	// test if sequence is empty
			return m_array.empty();
		}

		reference at(msear_size_t _Pos)
		{	// subscript mutable sequence with checking
			return m_array.at(msear_as_a_size_t(_Pos));
		}

		_CONST_FUN const_reference at(msear_size_t _Pos) const
		{	// subscript nonmutable sequence with checking
			return m_array.at(msear_as_a_size_t(_Pos));
		}

		value_type *data() _NOEXCEPT
		{	// return pointer to mutable data array
			return m_array.data();
		}

		const value_type *data() const _NOEXCEPT
		{	// return pointer to nonmutable data array
			return m_array.data();
		}

		nii_array& operator=(const nii_array& _Right_cref) {
			std::lock_guard<_TStateMutex> lock1(m_mutex1);
			m_array = _Right_cref.m_array;
			return (*this);
		}
		/*
		nii_array& operator=(const std_array& _Right_cref) {
		std::lock_guard<_TStateMutex> lock1(m_mutex1);
		m_array = _Right_cref;
		return (*this);
		}
		*/

		//class random_access_const_iterator_base : public std::iterator<std::random_access_iterator_tag, value_type, difference_type, const_pointer, const_reference> {};
		//class random_access_iterator_base : public std::iterator<std::random_access_iterator_tag, value_type, difference_type, pointer, reference> {};
		class random_access_const_iterator_base {
		public:
			using iterator_category = std::random_access_iterator_tag;
			using value_type = typename _Myt::value_type;
			using difference_type = typename _Myt::difference_type;
			using pointer = typename _Myt::const_pointer;
			using reference = typename _Myt::const_reference;
		};
		class random_access_iterator_base {
		public:
			using iterator_category = std::random_access_iterator_tag;
			using value_type = typename _Myt::value_type;
			using difference_type = typename _Myt::difference_type;
			using pointer = typename _Myt::pointer;
			using reference = typename _Myt::reference;
		};

		class xscope_ss_const_iterator_type;
		class xscope_ss_iterator_type;

		/* The reason we specify the default parameter in the definition instead of this forward declaration is that there seems to be a
		bug in clang (3.8.0) such that if we don't specify the default parameter in the definition it seems to subsequently behave as if
		one were never specified. g++ and msvc don't seem to have the same issue. */
		template<typename _TArrayPointer, class/* = typename std::enable_if<(!std::is_base_of<XScopeTagBase, _TArrayPointer>::value), void>::type*/>
		class Tss_iterator_type;

		/* Tss_const_iterator_type is a bounds checked const_iterator. */
		template<typename _TArrayConstPointer, class = typename std::enable_if<(!std::is_base_of<XScopeTagBase, _TArrayConstPointer>::value), void>::type>
		class Tss_const_iterator_type : public random_access_const_iterator_base {
		public:
			typedef typename std::iterator_traits<typename std_array::const_iterator>::iterator_category iterator_category;
			typedef typename std::iterator_traits<typename std_array::const_iterator>::value_type value_type;
			//typedef typename std::iterator_traits<typename std_array::const_iterator>::difference_type difference_type;
			typedef msear_int difference_type;
			typedef typename std::iterator_traits<typename std_array::const_iterator>::pointer const_pointer;
			typedef typename std::iterator_traits<typename std_array::const_iterator>::reference const_reference;

			typedef typename std::iterator_traits<typename std_array::const_iterator>::pointer pointer;
			typedef typename std::iterator_traits<typename std_array::const_iterator>::reference reference;

			template<class = typename std::enable_if<std::is_default_constructible<_TArrayConstPointer>::value, void>::type>
			Tss_const_iterator_type() {}

			Tss_const_iterator_type(const _TArrayConstPointer& owner_cptr) : m_owner_cptr(owner_cptr) {}

			Tss_const_iterator_type(const Tss_const_iterator_type& src) = default;
			template<class _Ty2, class = typename std::enable_if<std::is_convertible<_Ty2, _TArrayConstPointer>::value, void>::type>
			Tss_const_iterator_type(const Tss_iterator_type<_Ty2, void>& src) : m_owner_cptr(src.target_container_ptr()), m_index(src.position()) {}

			void reset() { set_to_end_marker(); }
			bool points_to_an_item() const {
				if (m_owner_cptr->size() > m_index) { return true; }
				else {
					assert(m_index == m_owner_cptr->size());
					return false;
				}
			}
			bool points_to_end_marker() const {
				if (false == points_to_an_item()) {
					assert(m_index == m_owner_cptr->size());
					return true;
				}
				else { return false; }
			}
			bool points_to_beginning() const {
				if (0 == m_index) { return true; }
				else { return false; }
			}
			/* has_next_item_or_end_marker() is just an alias for points_to_an_item(). */
			bool has_next_item_or_end_marker() const { return points_to_an_item(); } //his is
																						/* has_next() is just an alias for points_to_an_item() that's familiar to java programmers. */
			bool has_next() const { return has_next_item_or_end_marker(); }
			bool has_previous() const {
				return ((1 <= m_owner_cptr->size()) && (!points_to_beginning()));
			}
			void set_to_beginning() {
				m_index = 0;
			}
			void set_to_end_marker() {
				m_index = m_owner_cptr->size();
			}
			void set_to_next() {
				if (points_to_an_item()) {
					m_index += 1;
					assert(m_owner_cptr->size() >= m_index);
				}
				else {
					MSE_THROW(nii_array_range_error("attempt to use invalid const_item_pointer - void set_to_next() - Tss_const_iterator_type - nii_array"));
				}
			}
			void set_to_previous() {
				if (has_previous()) {
					m_index -= 1;
				}
				else {
					MSE_THROW(nii_array_range_error("attempt to use invalid const_item_pointer - void set_to_previous() - Tss_const_iterator_type - nii_array"));
				}
			}
			Tss_const_iterator_type& operator ++() { (*this).set_to_next(); return (*this); }
			Tss_const_iterator_type operator++(int) { Tss_const_iterator_type _Tmp = *this; (*this).set_to_next(); return (_Tmp); }
			Tss_const_iterator_type& operator --() { (*this).set_to_previous(); return (*this); }
			Tss_const_iterator_type operator--(int) { Tss_const_iterator_type _Tmp = *this; (*this).set_to_previous(); return (_Tmp); }
			void advance(difference_type n) {
				auto new_index = msear_int(m_index) + n;
				if ((0 > new_index) || (m_owner_cptr->size() < msear_size_t(new_index))) {
					MSE_THROW(nii_array_range_error("index out of range - void advance(difference_type n) - Tss_const_iterator_type - nii_array"));
				}
				else {
					m_index = msear_size_t(new_index);
				}
			}
			void regress(difference_type n) { advance(-n); }
			Tss_const_iterator_type& operator +=(difference_type n) { (*this).advance(n); return (*this); }
			Tss_const_iterator_type& operator -=(difference_type n) { (*this).regress(n); return (*this); }
			Tss_const_iterator_type operator+(difference_type n) const {
				Tss_const_iterator_type retval(*this);
				retval.advance(n);
				return retval;
			}
			Tss_const_iterator_type operator-(difference_type n) const { return ((*this) + (-n)); }
			difference_type operator-(const Tss_const_iterator_type &rhs) const {
				if (rhs.m_owner_cptr != (*this).m_owner_cptr) { MSE_THROW(nii_array_range_error("invalid argument - difference_type operator-(const Tss_const_iterator_type &rhs) const - nii_array::Tss_const_iterator_type")); }
				auto retval = difference_type((*this).m_index) - difference_type(rhs.m_index);
				assert(difference_type((*m_owner_cptr).size()) >= retval);
				return retval;
			}
			const_reference operator*() const {
				return (*m_owner_cptr).at(msear_as_a_size_t((*this).m_index));
			}
			const_reference item() const { return operator*(); }
			const_reference previous_item() const {
				return (*m_owner_cptr).at(msear_as_a_size_t((*this).m_index - 1));
			}
			const_pointer operator->() const {
				return std::addressof((*m_owner_cptr).at(msear_as_a_size_t((*this).m_index)));
			}
			const_reference operator[](difference_type _Off) const { return (*m_owner_cptr).at(msear_as_a_size_t(difference_type(m_index) + _Off)); }
			/*
			Tss_const_iterator_type& operator=(const typename std_array::const_iterator& _Right_cref)
			{
			msear_int d = std::distance<typename std_array::iterator>(m_owner_cptr->cbegin(), _Right_cref);
			if ((0 <= d) && (m_owner_cptr->size() >= d)) {
			if (m_owner_cptr->size() == d) {
			assert(m_owner_cptr->cend() == _Right_cref);
			}
			m_index = msear_size_t(d);
			std_array::const_iterator::operator=(_Right_cref);
			}
			else {
			MSE_THROW(nii_array_range_error("doesn't seem to be a valid assignment value - Tss_const_iterator_type& operator=(const typename std_array::const_iterator& _Right_cref) - Tss_const_iterator_type - nii_array"));
			}
			return (*this);
			}
			*/
			/*
			Tss_const_iterator_type& operator=(const Tss_const_iterator_type& _Right_cref) {
				((*this).m_owner_cptr) = _Right_cref.m_owner_cptr;
				(*this).m_index = _Right_cref.m_index;
				return (*this);
			}
			*/
			bool operator==(const Tss_const_iterator_type& _Right_cref) const {
				if (this->m_owner_cptr != _Right_cref.m_owner_cptr) { MSE_THROW(nii_array_range_error("invalid argument - Tss_const_iterator_type& operator==(const Tss_const_iterator_type& _Right) - Tss_const_iterator_type - nii_array")); }
				return (_Right_cref.m_index == m_index);
			}
			bool operator!=(const Tss_const_iterator_type& _Right_cref) const { return (!(_Right_cref == (*this))); }
			bool operator<(const Tss_const_iterator_type& _Right) const {
				if (this->m_owner_cptr != _Right.m_owner_cptr) { MSE_THROW(nii_array_range_error("invalid argument - Tss_const_iterator_type& operator<(const Tss_const_iterator_type& _Right) - Tss_const_iterator_type - nii_array")); }
				return (m_index < _Right.m_index);
			}
			bool operator<=(const Tss_const_iterator_type& _Right) const { return (((*this) < _Right) || (_Right == (*this))); }
			bool operator>(const Tss_const_iterator_type& _Right) const { return (!((*this) <= _Right)); }
			bool operator>=(const Tss_const_iterator_type& _Right) const { return (!((*this) < _Right)); }
			void set_to_const_item_pointer(const Tss_const_iterator_type& _Right_cref) {
				(*this) = _Right_cref;
			}
			msear_size_t position() const {
				return m_index;
			}
			_TArrayConstPointer target_container_ptr() const {
				return m_owner_cptr;
			}
		private:
			_TArrayConstPointer m_owner_cptr;
			msear_size_t m_index = 0;

			friend class /*_Myt*/nii_array<_Ty, _Size>;
		};
		/* Tss_iterator_type is a bounds checked iterator. */
		template<typename _TArrayPointer, class = typename std::enable_if<(!std::is_base_of<XScopeTagBase, _TArrayPointer>::value), void>::type>
		class Tss_iterator_type : public random_access_iterator_base {
		public:
			typedef typename std::iterator_traits<typename std_array::iterator>::iterator_category iterator_category;
			typedef typename std::iterator_traits<typename std_array::iterator>::value_type value_type;
			//typedef typename std::iterator_traits<typename std_array::iterator>::difference_type difference_type;
			typedef msear_int difference_type;
			typedef typename std::iterator_traits<typename std_array::iterator>::pointer pointer;
			typedef typename std::iterator_traits<typename std_array::iterator>::reference reference;
			typedef difference_type distance_type;	// retained

			template<class = typename std::enable_if<std::is_default_constructible<_TArrayPointer>::value, void>::type>
			Tss_iterator_type() {}

			Tss_iterator_type(const _TArrayPointer& owner_ptr) : m_owner_ptr(owner_ptr) {}

			void reset() { set_to_end_marker(); }
			bool points_to_an_item() const {
				if (m_owner_ptr->size() > m_index) { return true; }
				else {
					assert(m_index == m_owner_ptr->size());
					return false;
				}
			}
			bool points_to_end_marker() const {
				if (false == points_to_an_item()) {
					assert(m_index == m_owner_ptr->size());
					return true;
				}
				else { return false; }
			}
			bool points_to_beginning() const {
				if (0 == m_index) { return true; }
				else { return false; }
			}
			/* has_next_item_or_end_marker() is just an alias for points_to_an_item(). */
			bool has_next_item_or_end_marker() const { return points_to_an_item(); }
			/* has_next() is just an alias for points_to_an_item() that's familiar to java programmers. */
			bool has_next() const { return has_next_item_or_end_marker(); }
			bool has_previous() const {
				return ((1 <= m_owner_ptr->size()) && (!points_to_beginning()));
			}
			void set_to_beginning() {
				m_index = 0;
			}
			void set_to_end_marker() {
				m_index = m_owner_ptr->size();
			}
			void set_to_next() {
				if (points_to_an_item()) {
					m_index += 1;
					assert(m_owner_ptr->size() >= m_index);
				}
				else {
					MSE_THROW(nii_array_range_error("attempt to use invalid item_pointer - void set_to_next() - Tss_const_iterator_type - nii_array"));
				}
			}
			void set_to_previous() {
				if (has_previous()) {
					m_index -= 1;
				}
				else {
					MSE_THROW(nii_array_range_error("attempt to use invalid item_pointer - void set_to_previous() - Tss_iterator_type - nii_array"));
				}
			}
			Tss_iterator_type& operator ++() { (*this).set_to_next(); return (*this); }
			Tss_iterator_type operator++(int) { Tss_iterator_type _Tmp = *this; (*this).set_to_next(); return (_Tmp); }
			Tss_iterator_type& operator --() { (*this).set_to_previous(); return (*this); }
			Tss_iterator_type operator--(int) { Tss_iterator_type _Tmp = *this; (*this).set_to_previous(); return (_Tmp); }
			void advance(difference_type n) {
				auto new_index = msear_int(m_index) + n;
				if ((0 > new_index) || (m_owner_ptr->size() < msear_size_t(new_index))) {
					MSE_THROW(nii_array_range_error("index out of range - void advance(difference_type n) - Tss_iterator_type - nii_array"));
				}
				else {
					m_index = msear_size_t(new_index);
				}
			}
			void regress(difference_type n) { advance(-n); }
			Tss_iterator_type& operator +=(difference_type n) { (*this).advance(n); return (*this); }
			Tss_iterator_type& operator -=(difference_type n) { (*this).regress(n); return (*this); }
			Tss_iterator_type operator+(difference_type n) const {
				Tss_iterator_type retval(*this);
				retval.advance(n);
				return retval;
			}
			Tss_iterator_type operator-(difference_type n) const { return ((*this) + (-n)); }
			difference_type operator-(const Tss_iterator_type& rhs) const {
				if (rhs.m_owner_ptr != (*this).m_owner_ptr) { MSE_THROW(nii_array_range_error("invalid argument - difference_type operator-(const Tss_iterator_type& rhs) const - nii_array::Tss_iterator_type")); }
				auto retval = difference_type((*this).m_index) - difference_type(rhs.m_index);
				assert(difference_type((*m_owner_ptr).size()) >= retval);
				return retval;
			}
			reference operator*() const {
				return (*m_owner_ptr).at(msear_as_a_size_t((*this).m_index));
			}
			reference item() const { return operator*(); }
			reference previous_item() const {
				if ((*this).has_previous()) {
					return (*m_owner_ptr)[m_index - 1];
				}
				else {
					MSE_THROW(nii_array_range_error("attempt to use invalid item_pointer - reference previous_item() - Tss_const_iterator_type - nii_array"));
				}
			}
			pointer operator->() const {
				return std::addressof((*m_owner_ptr).at(msear_as_a_size_t((*this).m_index)));
			}
			reference operator[](difference_type _Off) const { return (*m_owner_ptr).at(msear_as_a_size_t(difference_type(m_index) + _Off)); }
			/*
			Tss_iterator_type& operator=(const typename std_array::iterator& _Right_cref)
			{
			msear_int d = std::distance<typename std_array::iterator>(m_owner_ptr->begin(), _Right_cref);
			if ((0 <= d) && (m_owner_ptr->size() >= d)) {
			if (m_owner_ptr->size() == d) {
			assert(m_owner_ptr->end() == _Right_cref);
			}
			m_index = msear_size_t(d);
			(*this).m_base_iterator.operator=(_Right_cref);
			}
			else {
			MSE_THROW(nii_array_range_error("doesn't seem to be a valid assignment value - Tss_iterator_type& operator=(const typename std_array::iterator& _Right_cref) - Tss_const_iterator_type - nii_array"));
			}
			return (*this);
			}
			*/
			/*
			Tss_iterator_type& operator=(const Tss_iterator_type& _Right_cref) {
				((*this).m_owner_ptr) = _Right_cref.m_owner_ptr;
				(*this).m_index = _Right_cref.m_index;
				return (*this);
			}
			*/
			bool operator==(const Tss_iterator_type& _Right_cref) const {
				if (this->m_owner_ptr != _Right_cref.m_owner_ptr) { MSE_THROW(nii_array_range_error("invalid argument - Tss_iterator_type& operator==(const Tss_iterator_type& _Right) - Tss_iterator_type - nii_array")); }
				return (_Right_cref.m_index == m_index);
			}
			bool operator!=(const Tss_iterator_type& _Right_cref) const { return (!(_Right_cref == (*this))); }
			bool operator<(const Tss_iterator_type& _Right) const {
				if (this->m_owner_ptr != _Right.m_owner_ptr) { MSE_THROW(nii_array_range_error("invalid argument - Tss_iterator_type& operator<(const Tss_iterator_type& _Right) - Tss_iterator_type - nii_array")); }
				return (m_index < _Right.m_index);
			}
			bool operator<=(const Tss_iterator_type& _Right) const { return (((*this) < _Right) || (_Right == (*this))); }
			bool operator>(const Tss_iterator_type& _Right) const { return (!((*this) <= _Right)); }
			bool operator>=(const Tss_iterator_type& _Right) const { return (!((*this) < _Right)); }
			void set_to_item_pointer(const Tss_iterator_type& _Right_cref) {
				(*this) = _Right_cref;
			}
			msear_size_t position() const {
				return m_index;
			}
			_TArrayPointer target_container_ptr() const {
				return m_owner_ptr;
			}
			/*
			operator Tss_const_iterator_type<_TArrayPointer>() const {
			Tss_const_iterator_type<_TArrayPointer> retval;
			if (nullptr != m_owner_ptr) {
			retval = m_owner_ptr->ss_cbegin<_TArrayPointer>(m_owner_ptr);
			retval.advance(msear_int(m_index));
			}
			return retval;
			}
			*/
		private:
			msear_size_t m_index = 0;
			//msear_pointer<_Myt> m_owner_ptr = nullptr;
			_TArrayPointer m_owner_ptr;

			friend class /*_Myt*/nii_array<_Ty, _Size>;
			template<typename _TArrayConstPointer, class/* = typename std::enable_if<(!std::is_base_of<XScopeTagBase, _TArrayConstPointer>::value), void>::type*/>
			friend class Tss_const_iterator_type;
		};

		template<typename _TArrayPointer, class = typename std::enable_if<(!std::is_base_of<XScopeTagBase, _TArrayPointer>::value), void>::type>
		using Tss_reverse_iterator_type = std::reverse_iterator<Tss_iterator_type<_TArrayPointer>>;
		template<typename _TArrayConstPointer, class = typename std::enable_if<(!std::is_base_of<XScopeTagBase, _TArrayConstPointer>::value), void>::type>
		using Tss_const_reverse_iterator_type = std::reverse_iterator<Tss_const_iterator_type<_TArrayConstPointer>>;

		typedef Tss_iterator_type<msear_pointer<_Myt>> ss_iterator_type;
		typedef Tss_const_iterator_type<msear_pointer<const _Myt>> ss_const_iterator_type;
		typedef Tss_reverse_iterator_type<msear_pointer<_Myt>> ss_reverse_iterator_type;
		typedef Tss_const_reverse_iterator_type<msear_pointer<const _Myt>> ss_const_reverse_iterator_type;

		template<typename _TArrayPointer>
		static auto ss_begin(const _TArrayPointer& owner_ptr) {
			mse::T_valid_if_not_an_xscope_type<_TArrayPointer>();
			typedef typename std::conditional<std::is_const<typename std::remove_reference<decltype(*owner_ptr)>::type>::value
				, Tss_const_iterator_type<_TArrayPointer>, Tss_iterator_type<_TArrayPointer> >::type return_type;
			return_type retval(owner_ptr);
			retval.set_to_beginning();
			return retval;
		}

		template<typename _TArrayPointer>
		static auto ss_end(const _TArrayPointer& owner_ptr) {
			mse::T_valid_if_not_an_xscope_type<_TArrayPointer>();
			typedef typename std::conditional<std::is_const<typename std::remove_reference<decltype(*owner_ptr)>::type>::value
				, Tss_const_iterator_type<_TArrayPointer>, Tss_iterator_type<_TArrayPointer> >::type return_type;
			return_type retval(owner_ptr);
			retval.set_to_end_marker();
			return retval;
		}

		template<typename _TArrayPointer>
		static auto ss_cbegin(const _TArrayPointer& owner_ptr) {
			mse::T_valid_if_not_an_xscope_type<_TArrayPointer>();
			Tss_const_iterator_type<_TArrayPointer> retval(owner_ptr);
			retval.set_to_beginning();
			return retval;
		}

		template<typename _TArrayPointer>
		static auto ss_cend(const _TArrayPointer& owner_ptr) {
			mse::T_valid_if_not_an_xscope_type<_TArrayPointer>();
			Tss_const_iterator_type<_TArrayPointer> retval(owner_ptr);
			retval.set_to_end_marker();
			return retval;
		}

		template<typename _TArrayPointer>
		static auto ss_rbegin(const _TArrayPointer& owner_ptr) {
			mse::T_valid_if_not_an_xscope_type<_TArrayPointer>();
			typedef typename std::conditional<std::is_const<typename std::remove_reference<decltype(*owner_ptr)>::type>::value
				, Tss_const_reverse_iterator_type<_TArrayPointer>, Tss_reverse_iterator_type<_TArrayPointer> >::type return_type;
			return return_type(ss_end<_TArrayPointer>(owner_ptr));
		}

		template<typename _TArrayPointer>
		static auto ss_rend(const _TArrayPointer& owner_ptr) {
			mse::T_valid_if_not_an_xscope_type<_TArrayPointer>();
			typedef typename std::conditional<std::is_const<typename std::remove_reference<decltype(*owner_ptr)>::type>::value
				, Tss_const_reverse_iterator_type<_TArrayPointer>, Tss_reverse_iterator_type<_TArrayPointer> >::type return_type;
			return return_type(ss_begin<_TArrayPointer>(owner_ptr));
		}

		template<typename _TArrayPointer>
		static auto ss_crbegin(const _TArrayPointer& owner_ptr) {
			mse::T_valid_if_not_an_xscope_type<_TArrayPointer>();
			return (Tss_const_reverse_iterator_type<_TArrayPointer>(ss_cend<_TArrayPointer>(owner_ptr)));
		}

		template<typename _TArrayPointer>
		static auto ss_crend(const _TArrayPointer& owner_ptr) {
			mse::T_valid_if_not_an_xscope_type<_TArrayPointer>();
			return (Tss_const_reverse_iterator_type<_TArrayPointer>(ss_crbegin<_TArrayPointer>(owner_ptr)));
		}

		class xscope_ss_const_iterator_type : public ss_const_iterator_type, public XScopeContainsNonOwningScopeReferenceTagBase, public StrongPointerNotAsyncShareableTagBase {
		public:
			template <typename _TXScopePointer, class = typename std::enable_if<
				std::is_convertible<_TXScopePointer, mse::TXScopeItemFixedConstPointer<nii_array> >::value
				|| std::is_convertible<_TXScopePointer, mse::TXScopeItemFixedPointer<nii_array> >::value
				|| std::is_convertible<_TXScopePointer, mse::TXScopeFixedConstPointer<nii_array> >::value
				|| std::is_convertible<_TXScopePointer, mse::TXScopeFixedPointer<nii_array> >::value
				, void>::type>
			xscope_ss_const_iterator_type(const _TXScopePointer& owner_ptr) : ss_const_iterator_type((*owner_ptr).ss_cbegin()) {}

			xscope_ss_const_iterator_type(const xscope_ss_const_iterator_type& src_cref) : ss_const_iterator_type(src_cref) {}
			xscope_ss_const_iterator_type(const xscope_ss_iterator_type& src_cref) : ss_const_iterator_type(src_cref) {}
			~xscope_ss_const_iterator_type() {}
			const ss_const_iterator_type& nii_array_ss_const_iterator_type() const {
				return (*this);
			}
			ss_const_iterator_type& nii_array_ss_const_iterator_type() {
				return (*this);
			}
			const ss_const_iterator_type& mvssci() const { return nii_array_ss_const_iterator_type(); }
			ss_const_iterator_type& mvssci() { return nii_array_ss_const_iterator_type(); }

			void reset() { ss_const_iterator_type::reset(); }
			bool points_to_an_item() const { return ss_const_iterator_type::points_to_an_item(); }
			bool points_to_end_marker() const { return ss_const_iterator_type::points_to_end_marker(); }
			bool points_to_beginning() const { return ss_const_iterator_type::points_to_beginning(); }
			/* has_next_item_or_end_marker() is just an alias for points_to_an_item(). */
			bool has_next_item_or_end_marker() const { return ss_const_iterator_type::has_next_item_or_end_marker(); }
			/* has_next() is just an alias for points_to_an_item() that's familiar to java programmers. */
			bool has_next() const { return ss_const_iterator_type::has_next(); }
			bool has_previous() const { return ss_const_iterator_type::has_previous(); }
			void set_to_beginning() { ss_const_iterator_type::set_to_beginning(); }
			void set_to_end_marker() { ss_const_iterator_type::set_to_end_marker(); }
			void set_to_next() { ss_const_iterator_type::set_to_next(); }
			void set_to_previous() { ss_const_iterator_type::set_to_previous(); }
			xscope_ss_const_iterator_type& operator ++() { ss_const_iterator_type::operator ++(); return (*this); }
			xscope_ss_const_iterator_type operator++(int) { xscope_ss_const_iterator_type _Tmp = *this; ss_const_iterator_type::operator++(); return (_Tmp); }
			xscope_ss_const_iterator_type& operator --() { ss_const_iterator_type::operator --(); return (*this); }
			xscope_ss_const_iterator_type operator--(int) { xscope_ss_const_iterator_type _Tmp = *this; ss_const_iterator_type::operator--(); return (_Tmp); }
			void advance(difference_type n) { ss_const_iterator_type::advance(n); }
			void regress(difference_type n) { ss_const_iterator_type::regress(n); }
			xscope_ss_const_iterator_type& operator +=(difference_type n) { ss_const_iterator_type::operator +=(n); return (*this); }
			xscope_ss_const_iterator_type& operator -=(difference_type n) { ss_const_iterator_type::operator -=(n); return (*this); }
			xscope_ss_const_iterator_type operator+(difference_type n) const { auto retval = (*this); retval += n; return retval; }
			xscope_ss_const_iterator_type operator-(difference_type n) const { return ((*this) + (-n)); }
			difference_type operator-(const xscope_ss_const_iterator_type& _Right_cref) const { return ss_const_iterator_type::operator-(_Right_cref); }
			const_reference operator*() const { return ss_const_iterator_type::operator*(); }
			const_reference item() const { return operator*(); }
			const_reference previous_item() const { return ss_const_iterator_type::previous_item(); }
			const_pointer operator->() const { return ss_const_iterator_type::operator->(); }
			const_reference operator[](difference_type _Off) const { return ss_const_iterator_type::operator[](_Off); }
			xscope_ss_const_iterator_type& operator=(const ss_const_iterator_type& _Right_cref) {
				if ((&(*_Right_cref.target_container_ptr())) != (&(*(*this).target_container_ptr()))) { MSE_THROW(nii_array_range_error("invalid argument - xscope_ss_const_iterator_type& operator=(const xscope_ss_const_iterator_type& _Right_cref) - nii_array::xscope_ss_const_iterator_type")); }
				ss_const_iterator_type::operator=(_Right_cref);
				return (*this);
			}
			xscope_ss_const_iterator_type& operator=(const ss_iterator_type& _Right_cref) {
				if ((&(*_Right_cref.target_container_ptr())) != (&(*(*this).target_container_ptr()))) { MSE_THROW(nii_array_range_error("invalid argument - xscope_ss_const_iterator_type& operator=(const ss_iterator_type& _Right_cref) - nii_array::xscope_ss_const_iterator_type")); }
				return operator=(ss_const_iterator_type(_Right_cref));
			}
			bool operator==(const xscope_ss_const_iterator_type& _Right_cref) const { return ss_const_iterator_type::operator==(_Right_cref); }
			bool operator!=(const xscope_ss_const_iterator_type& _Right_cref) const { return (!(_Right_cref == (*this))); }
			bool operator<(const xscope_ss_const_iterator_type& _Right) const { return ss_const_iterator_type::operator<(_Right); }
			bool operator<=(const xscope_ss_const_iterator_type& _Right) const { return ss_const_iterator_type::operator<=(_Right); }
			bool operator>(const xscope_ss_const_iterator_type& _Right) const { return ss_const_iterator_type::operator>(_Right); }
			bool operator>=(const xscope_ss_const_iterator_type& _Right) const { return ss_const_iterator_type::operator>=(_Right); }
			void set_to_const_item_pointer(const xscope_ss_const_iterator_type& _Right_cref) { ss_const_iterator_type::set_to_item_pointer(_Right_cref); }
			msear_size_t position() const { return ss_const_iterator_type::position(); }
			auto target_container_ptr() const {
				return mse::us::unsafe_make_xscope_const_pointer_to(*(ss_const_iterator_type::target_container_ptr()));
			}
			void xscope_ss_iterator_type_tag() const {}
			void not_async_shareable_tag() const {} /* Indication that this type is not eligible to be shared between threads. */
		private:
			void* operator new(size_t size) { return ::operator new(size); }

			//typename ss_const_iterator_type (*this);
			friend class /*_Myt*/nii_array<_Ty, _Size>;
			friend class xscope_ss_iterator_type;
		};
		class xscope_ss_iterator_type : public ss_iterator_type, public XScopeContainsNonOwningScopeReferenceTagBase, public StrongPointerNotAsyncShareableTagBase {
		public:
			template <typename _TXScopePointer, class = typename std::enable_if<
				std::is_convertible<_TXScopePointer, mse::TXScopeItemFixedPointer<nii_array> >::value
				|| std::is_convertible<_TXScopePointer, mse::TXScopeFixedPointer<nii_array> >::value
				, void>::type>
			xscope_ss_iterator_type(const _TXScopePointer& owner_ptr) : ss_iterator_type((*owner_ptr).ss_begin()) {}

			xscope_ss_iterator_type(const xscope_ss_iterator_type& src_cref) : ss_iterator_type(src_cref) {}
			~xscope_ss_iterator_type() {}
			const ss_iterator_type& nii_array_ss_iterator_type() const {
				return (*this);
			}
			ss_iterator_type& nii_array_ss_iterator_type() {
				return (*this);
			}
			const ss_iterator_type& mvssi() const { return nii_array_ss_iterator_type(); }
			ss_iterator_type& mvssi() { return nii_array_ss_iterator_type(); }

			void reset() { ss_iterator_type::reset(); }
			bool points_to_an_item() const { return ss_iterator_type::points_to_an_item(); }
			bool points_to_end_marker() const { return ss_iterator_type::points_to_end_marker(); }
			bool points_to_beginning() const { return ss_iterator_type::points_to_beginning(); }
			/* has_next_item_or_end_marker() is just an alias for points_to_an_item(). */
			bool has_next_item_or_end_marker() const { return ss_iterator_type::has_next_item_or_end_marker(); }
			/* has_next() is just an alias for points_to_an_item() that's familiar to java programmers. */
			bool has_next() const { return ss_iterator_type::has_next(); }
			bool has_previous() const { return ss_iterator_type::has_previous(); }
			void set_to_beginning() { ss_iterator_type::set_to_beginning(); }
			void set_to_end_marker() { ss_iterator_type::set_to_end_marker(); }
			void set_to_next() { ss_iterator_type::set_to_next(); }
			void set_to_previous() { ss_iterator_type::set_to_previous(); }
			xscope_ss_iterator_type& operator ++() { ss_iterator_type::operator ++(); return (*this); }
			xscope_ss_iterator_type operator++(int) { xscope_ss_iterator_type _Tmp = *this; ss_iterator_type::operator++(); return (_Tmp); }
			xscope_ss_iterator_type& operator --() { ss_iterator_type::operator --(); return (*this); }
			xscope_ss_iterator_type operator--(int) { xscope_ss_iterator_type _Tmp = *this; ss_iterator_type::operator--(); return (_Tmp); }
			void advance(difference_type n) { ss_iterator_type::advance(n); }
			void regress(difference_type n) { ss_iterator_type::regress(n); }
			xscope_ss_iterator_type& operator +=(difference_type n) { ss_iterator_type::operator +=(n); return (*this); }
			xscope_ss_iterator_type& operator -=(difference_type n) { ss_iterator_type::operator -=(n); return (*this); }
			xscope_ss_iterator_type operator+(difference_type n) const { auto retval = (*this); retval += n; return retval; }
			xscope_ss_iterator_type operator-(difference_type n) const { return ((*this) + (-n)); }
			difference_type operator-(const xscope_ss_iterator_type& _Right_cref) const { return ss_iterator_type::operator-(_Right_cref); }
			reference operator*() const { return ss_iterator_type::operator*(); }
			reference item() const { return operator*(); }
			reference previous_item() const { return ss_iterator_type::previous_item(); }
			pointer operator->() const { return ss_iterator_type::operator->(); }
			reference operator[](difference_type _Off) const { return ss_iterator_type::operator[](_Off); }
			xscope_ss_iterator_type& operator=(const ss_iterator_type& _Right_cref) {
				if ((&(*_Right_cref.target_container_ptr())) != (&(*(*this).target_container_ptr()))) { MSE_THROW(nii_array_range_error("invalid argument - xscope_ss_iterator_type& operator=(const xscope_ss_iterator_type& _Right_cref) - nii_array::xscope_ss_iterator_type")); }
				ss_iterator_type::operator=(_Right_cref);
				return (*this);
			}
			bool operator==(const xscope_ss_iterator_type& _Right_cref) const { return ss_iterator_type::operator==(_Right_cref); }
			bool operator!=(const xscope_ss_iterator_type& _Right_cref) const { return (!(_Right_cref == (*this))); }
			bool operator<(const xscope_ss_iterator_type& _Right) const { return ss_iterator_type::operator<(_Right); }
			bool operator<=(const xscope_ss_iterator_type& _Right) const { return ss_iterator_type::operator<=(_Right); }
			bool operator>(const xscope_ss_iterator_type& _Right) const { return ss_iterator_type::operator>(_Right); }
			bool operator>=(const xscope_ss_iterator_type& _Right) const { return ss_iterator_type::operator>=(_Right); }
			void set_to_item_pointer(const xscope_ss_iterator_type& _Right_cref) { ss_iterator_type::set_to_item_pointer(_Right_cref); }
			msear_size_t position() const { return ss_iterator_type::position(); }
			auto target_container_ptr() const {
				return mse::us::unsafe_make_xscope_pointer_to(*(ss_iterator_type::target_container_ptr()));
			}
			void xscope_ss_iterator_type_tag() const {}
			void not_async_shareable_tag() const {} /* Indication that this type is not eligible to be shared between threads. */
		private:
			void* operator new(size_t size) { return ::operator new(size); }

			//typename ss_iterator_type (*this);
			friend class /*_Myt*/nii_array<_Ty, _Size>;
		};

		typedef xscope_ss_const_iterator_type xscope_const_iterator;
		typedef xscope_ss_iterator_type xscope_iterator;

		bool operator==(const _Myt& _Right) const {	// test for array equality
			return (_Right.m_array == m_array);
		}
		bool operator<(const _Myt& _Right) const {	// test if _Left < _Right for arrays
			return (m_array < _Right.m_array);
		}

		/* This array is safely "async shareable" if the elements it contains are also "async shareable". */
		template<class _Ty2 = _Ty, class = typename std::enable_if<(std::is_same<_Ty2, _Ty>::value) && (
			(std::integral_constant<bool, HasAsyncShareableTagMethod_msemsearray<_Ty2>::Has>()) || (std::is_arithmetic<_Ty2>::value)
			), void>::type>
		void async_shareable_tag() const {} /* Indication that this type is eligible to be shared between threads. */

	private:
		/* If _Ty is an xscope type, then the following member function will not instantiate, causing an
		(intended) compile error. */
		template<class _Ty2 = _Ty, class = typename std::enable_if<(std::is_same<_Ty2, _Ty>::value) && (!std::is_base_of<XScopeTagBase, _Ty2>::value), void>::type>
		void valid_if_Ty_is_not_an_xscope_type() const {}

		ss_iterator_type ss_begin() {	// return std_array::iterator for beginning of mutable sequence
			ss_iterator_type retval(this);
			retval.set_to_beginning();
			return retval;
		}
		ss_const_iterator_type ss_begin() const {	// return std_array::iterator for beginning of nonmutable sequence
			ss_const_iterator_type retval(this);
			retval.set_to_beginning();
			return retval;
		}
		ss_iterator_type ss_end() {	// return std_array::iterator for end of mutable sequence
			ss_iterator_type retval(this);
			retval.set_to_end_marker();
			return retval;
		}
		ss_const_iterator_type ss_end() const {	// return std_array::iterator for end of nonmutable sequence
			ss_const_iterator_type retval(this);
			retval.set_to_end_marker();
			return retval;
		}
		ss_const_iterator_type ss_cbegin() const {	// return std_array::iterator for beginning of nonmutable sequence
			ss_const_iterator_type retval(this);
			retval.set_to_beginning();
			return retval;
		}
		ss_const_iterator_type ss_cend() const {	// return std_array::iterator for end of nonmutable sequence
			ss_const_iterator_type retval(this);
			retval.set_to_end_marker();
			return retval;
		}

		ss_const_reverse_iterator_type ss_crbegin() const {	// return std_array::iterator for beginning of reversed nonmutable sequence
			return (ss_rbegin());
		}
		ss_const_reverse_iterator_type ss_crend() const {	// return std_array::iterator for end of reversed nonmutable sequence
			return (ss_rend());
		}
		ss_reverse_iterator_type ss_rbegin() {	// return std_array::iterator for beginning of reversed mutable sequence
			return (reverse_iterator(ss_end()));
		}
		ss_const_reverse_iterator_type ss_rbegin() const {	// return std_array::iterator for beginning of reversed nonmutable sequence
			return (const_reverse_iterator(ss_end()));
		}
		ss_reverse_iterator_type ss_rend() {	// return std_array::iterator for end of reversed mutable sequence
			return (reverse_iterator(ss_begin()));
		}
		ss_const_reverse_iterator_type ss_rend() const {	// return std_array::iterator for end of reversed nonmutable sequence
			return (const_reverse_iterator(ss_begin()));
		}

		const _MA& contained_array() const { return m_array; }
		_MA& contained_array() { return m_array; }

		std_array m_array;
		_TStateMutex m_mutex1;

		friend class xscope_ss_const_iterator_type;
		friend class xscope_ss_iterator_type;
		friend class us::msearray<_Ty, _Size, _TStateMutex>;

		template<size_t _Idx, class _Tz, size_t _Size2>
		friend _CONST_FUN _Tz& std::get(mse::nii_array<_Tz, _Size2>& _Arr) _NOEXCEPT;
		template<size_t _Idx, class _Tz, size_t _Size2>
		friend _CONST_FUN const _Tz& std::get(const mse::nii_array<_Tz, _Size2>& _Arr) _NOEXCEPT;
		template<size_t _Idx, class _Tz, size_t _Size2>
		friend _CONST_FUN _Tz&& std::get(mse::nii_array<_Tz, _Size2>&& _Arr) _NOEXCEPT;
	};

	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex> inline bool operator!=(const nii_array<_Ty, _Size, _TStateMutex>& _Left,
		const nii_array<_Ty, _Size, _TStateMutex>& _Right) {	// test for array inequality
		return (!(_Left == _Right));
	}

	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex> inline bool operator>(const nii_array<_Ty, _Size, _TStateMutex>& _Left,
		const nii_array<_Ty, _Size, _TStateMutex>& _Right) {	// test if _Left > _Right for arrays
		return (_Right < _Left);
	}

	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex> inline bool operator<=(const nii_array<_Ty, _Size, _TStateMutex>& _Left,
		const nii_array<_Ty, _Size, _TStateMutex>& _Right) {	// test if _Left <= _Right for arrays
		return (!(_Right < _Left));
	}

	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex> inline bool operator>=(const nii_array<_Ty, _Size, _TStateMutex>& _Left,
		const nii_array<_Ty, _Size, _TStateMutex>& _Right) {	// test if _Left >= _Right for arrays
		return (!(_Left < _Right));
	}

	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
	TXScopeItemFixedPointer<_Ty> xscope_pointer_to_array_element(const typename nii_array<_Ty, _Size, _TStateMutex>::xscope_ss_iterator_type& iter_cref) {
		return mse::us::unsafe_make_xscope_pointer_to(*iter_cref);
	}
	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
	TXScopeItemFixedPointer<_Ty> xscope_pointer_to_array_element(const mse::TXScopeItemFixedPointer<nii_array<_Ty, _Size, _TStateMutex> >& ptr, typename nii_array<_Ty, _Size, _TStateMutex>::size_type _P) {
		return mse::us::unsafe_make_xscope_pointer_to((*ptr)[_P]);
	}
#if !defined(MSE_SCOPEPOINTER_DISABLED)
	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
	TXScopeItemFixedPointer<_Ty> xscope_pointer_to_array_element(const mse::TXScopeFixedPointer<nii_array<_Ty, _Size, _TStateMutex> >& ptr, typename nii_array<_Ty, _Size, _TStateMutex>::size_type _P) {
		return mse::us::unsafe_make_xscope_pointer_to((*ptr)[_P]);
	}
#endif // !defined(MSE_SCOPEPOINTER_DISABLED)

	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
	TXScopeItemFixedConstPointer<_Ty> xscope_const_pointer_to_array_element(const typename nii_array<_Ty, _Size, _TStateMutex>::xscope_ss_const_iterator_type& iter_cref) {
		return mse::us::unsafe_make_xscope_const_pointer_to(*iter_cref);
	}
	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
	TXScopeItemFixedConstPointer<_Ty> xscope_const_pointer_to_array_element(const mse::TXScopeItemFixedConstPointer<nii_array<_Ty, _Size, _TStateMutex> >& ptr, typename nii_array<_Ty, _Size, _TStateMutex>::size_type _P) {
		return mse::us::unsafe_make_xscope_const_pointer_to((*ptr)[_P]);
	}
	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
	TXScopeItemFixedConstPointer<_Ty> xscope_const_pointer_to_array_element(const mse::TXScopeItemFixedPointer<nii_array<_Ty, _Size, _TStateMutex> >& ptr, typename nii_array<_Ty, _Size, _TStateMutex>::size_type _P) {
		return mse::us::unsafe_make_xscope_const_pointer_to((*ptr)[_P]);
	}
#if !defined(MSE_SCOPEPOINTER_DISABLED)
	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
	TXScopeItemFixedConstPointer<_Ty> xscope_const_pointer_to_array_element(const mse::TXScopeFixedConstPointer<nii_array<_Ty, _Size, _TStateMutex> >& ptr, typename nii_array<_Ty, _Size, _TStateMutex>::size_type _P) {
		return mse::us::unsafe_make_xscope_const_pointer_to((*ptr)[_P]);
	}
	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
	TXScopeItemFixedConstPointer<_Ty> xscope_const_pointer_to_array_element(const mse::TXScopeFixedPointer<nii_array<_Ty, _Size, _TStateMutex> >& ptr, typename nii_array<_Ty, _Size, _TStateMutex>::size_type _P) {
		return mse::us::unsafe_make_xscope_const_pointer_to((*ptr)[_P]);
	}
#endif // !defined(MSE_SCOPEPOINTER_DISABLED)

}

namespace std {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmismatched-tags"
#endif /*__clang__*/

	template<class _Ty, size_t _Size>
	struct tuple_size<mse::nii_array<_Ty, _Size> >
		: integral_constant<size_t, _Size>
	{	// struct to determine number of elements in array
	};

	template<size_t _Idx, class _Ty, size_t _Size>
	struct tuple_element<_Idx, mse::nii_array<_Ty, _Size> >
	{	// struct to determine type of element _Idx in array
		static_assert(_Idx < _Size, "array index out of bounds");

		typedef _Ty type;
	};

#ifdef __clang__
#pragma clang diagnostic pop
#endif /*__clang__*/

	// TUPLE INTERFACE TO array
	template<size_t _Idx, class _Ty, size_t _Size>
	_CONST_FUN _Ty& get(mse::nii_array<_Ty, _Size>& _Arr) _NOEXCEPT
	{	// return element at _Idx in array _Arr
		static_assert(_Idx < _Size, "array index out of bounds");
		return (std::get<_Idx>(_Arr.contained_array()));
	}

	template<size_t _Idx, class _Ty, size_t _Size>
	_CONST_FUN const _Ty& get(const mse::nii_array<_Ty, _Size>& _Arr) _NOEXCEPT
	{	// return element at _Idx in array _Arr
		static_assert(_Idx < _Size, "array index out of bounds");
		return (std::get<_Idx>(_Arr.contained_array()));
	}

	template<size_t _Idx, class _Ty, size_t _Size>
	_CONST_FUN _Ty&& get(mse::nii_array<_Ty, _Size>&& _Arr) _NOEXCEPT
	{	// return element at _Idx in array _Arr
		static_assert(_Idx < _Size, "array index out of bounds");
		return (_STD move(std::get<_Idx>(_Arr.contained_array())));
	}

	template<class _Ty, size_t _Size, class _TStateMutex = mse::default_state_mutex/*, class = enable_if_t<_Size == 0 || _Is_swappable<_Ty>::value>*/>
	void swap(mse::nii_array<_Ty, _Size, _TStateMutex>& _Left, mse::nii_array<_Ty, _Size, _TStateMutex>& _Right) _NOEXCEPT
	{
		_Left.swap(_Right);
	}
	template<class _Ty, size_t _Size, class _TStateMutex = mse::default_state_mutex/*, class = enable_if_t<_Size == 0 || _Is_swappable<_Ty>::value>*/>
	void swap(array<_Ty, _Size>& _Left, mse::nii_array<_Ty, _Size, _TStateMutex>& _Right) _NOEXCEPT_OP(_NOEXCEPT_OP(_Right.swap(_Left)))
	{	// swap arrays
		return (_Right.swap(_Left));
	}
	template<class _Ty, size_t _Size, class _TStateMutex = mse::default_state_mutex/*, class = enable_if_t<_Size == 0 || _Is_swappable<_Ty>::value>*/>
	void swap(mse::nii_array<_Ty, _Size, _TStateMutex>& _Left, array<_Ty, _Size>& _Right) _NOEXCEPT
	{
		_Left.swap(_Right);
	}
}

namespace mse {

	namespace us {
		/* msearray<> is an unsafe extension of nii_array<> that provides the traditional begin() and end() (non-static)
		member functions that return unsafe iterators. It also provides ss_begin() and ss_end() (non-static) member
		functions which return bounds-checked, but still technically unsafe iterators. */
		template<class _Ty, size_t _Size, class _TStateMutex>
		class msearray : public nii_array<_Ty, _Size, _TStateMutex> {
		public:
			typedef nii_array<_Ty, _Size, _TStateMutex> base_class;
			typedef std::array<_Ty, _Size> std_array;
			typedef msearray _Myt;

			MSE_USING(msearray, base_class);
			msearray(_XSTD initializer_list<typename base_class::_MA::value_type> _Ilist) : base_class(_Ilist) {}

			typedef typename base_class::value_type value_type;
			typedef typename base_class::size_type size_type;
			//typedef msear_size_t size_type;
			typedef typename base_class::difference_type difference_type;
			//typedef msear_int difference_type;
			typedef typename base_class::pointer pointer;
			typedef typename base_class::const_pointer const_pointer;
			typedef typename base_class::reference reference;
			typedef typename base_class::const_reference const_reference;

			typedef typename base_class::_MA::iterator iterator;
			typedef typename base_class::_MA::const_iterator const_iterator;
			typedef typename base_class::_MA::reverse_iterator reverse_iterator;
			typedef typename base_class::_MA::const_reverse_iterator const_reverse_iterator;

			iterator begin() _NOEXCEPT {	// return iterator for beginning of mutable sequence
				return base_class::contained_array().begin();
			}
			const_iterator begin() const _NOEXCEPT {	// return iterator for beginning of nonmutable sequence
				return base_class::contained_array().begin();
			}
			iterator end() _NOEXCEPT {	// return iterator for end of mutable sequence
				return base_class::contained_array().end();
			}
			const_iterator end() const _NOEXCEPT {	// return iterator for beginning of nonmutable sequence
				return base_class::contained_array().end();
			}
			reverse_iterator rbegin() _NOEXCEPT {	// return iterator for beginning of reversed mutable sequence
				return base_class::contained_array().rbegin();
			}
			const_reverse_iterator rbegin() const _NOEXCEPT {	// return iterator for beginning of reversed nonmutable sequence
				return base_class::contained_array().rbegin();
			}
			reverse_iterator rend() _NOEXCEPT {	// return iterator for end of reversed mutable sequence
				return base_class::contained_array().rend();
			}
			const_reverse_iterator rend() const _NOEXCEPT {	// return iterator for end of reversed nonmutable sequence
				return base_class::contained_array().rend();
			}
			const_iterator cbegin() const _NOEXCEPT {	// return iterator for beginning of nonmutable sequence
				return base_class::contained_array().cbegin();
			}
			const_iterator cend() const _NOEXCEPT {	// return iterator for end of nonmutable sequence
				return base_class::contained_array().cend();
			}
			const_reverse_iterator crbegin() const _NOEXCEPT {	// return iterator for beginning of reversed nonmutable sequence
				return base_class::contained_array().crbegin();
			}
			const_reverse_iterator crend() const _NOEXCEPT {	// return iterator for end of reversed nonmutable sequence
				return base_class::contained_array().crend();
			}

			typedef typename base_class::ss_iterator_type ss_iterator_type;
			typedef typename base_class::ss_const_iterator_type ss_const_iterator_type;
			typedef typename base_class::ss_reverse_iterator_type ss_reverse_iterator_type;
			typedef typename base_class::ss_const_reverse_iterator_type ss_const_reverse_iterator_type;

			ss_iterator_type ss_begin() {	// return std_array::iterator for beginning of mutable sequence
				return base_class::ss_begin();
			}
			ss_const_iterator_type ss_begin() const {	// return std_array::iterator for beginning of nonmutable sequence
				return base_class::ss_begin();
			}
			ss_iterator_type ss_end() {	// return std_array::iterator for end of mutable sequence
				return base_class::ss_end();
			}
			ss_const_iterator_type ss_end() const {	// return std_array::iterator for end of nonmutable sequence
				return base_class::ss_end();
			}
			ss_const_iterator_type ss_cbegin() const {	// return std_array::iterator for beginning of nonmutable sequence
				return base_class::ss_cbegin();
			}
			ss_const_iterator_type ss_cend() const {	// return std_array::iterator for end of nonmutable sequence
				return base_class::ss_cend();
			}

			ss_const_reverse_iterator_type ss_crbegin() const {	// return std_array::iterator for beginning of reversed nonmutable sequence
				return base_class::ss_crbegin();
			}
			ss_const_reverse_iterator_type ss_crend() const {	// return std_array::iterator for end of reversed nonmutable sequence
				return base_class::ss_crend();
			}
			ss_reverse_iterator_type ss_rbegin() {	// return std_array::iterator for beginning of reversed mutable sequence
				return base_class::ss_rbegin();
			}
			ss_const_reverse_iterator_type ss_rbegin() const {	// return std_array::iterator for beginning of reversed nonmutable sequence
				return base_class::ss_rbegin();
			}
			ss_reverse_iterator_type ss_rend() {	// return std_array::iterator for end of reversed mutable sequence
				return base_class::ss_rend();
			}
			ss_const_reverse_iterator_type ss_rend() const {	// return std_array::iterator for end of reversed nonmutable sequence
				return base_class::ss_rend();
			}

			template<typename _TMseArrayPointer> using Tss_iterator_type = typename base_class::template Tss_iterator_type<_TMseArrayPointer>;
			template<typename _TMseArrayPointer> using Tss_const_iterator_type = typename base_class::template Tss_const_iterator_type<_TMseArrayPointer>;
			template<typename _TMseArrayPointer> using Tss_reverse_iterator_type = typename base_class::template Tss_reverse_iterator_type<_TMseArrayPointer>;
			template<typename _TMseArrayPointer> using Tss_const_reverse_iterator_type = typename base_class::template Tss_const_reverse_iterator_type<_TMseArrayPointer>;

			//template <class X> using ss_begin = typename base_class::template ss_begin<X>;
			template<typename _TMseArrayPointer>
			static Tss_iterator_type<_TMseArrayPointer> ss_begin(const _TMseArrayPointer& owner_ptr) {	// return iterator for beginning of mutable sequence
				return base_class::template ss_begin<_TMseArrayPointer>(owner_ptr);
			}
			template<typename _TMseArrayPointer>
			static Tss_iterator_type<_TMseArrayPointer> ss_end(_TMseArrayPointer owner_ptr) {	// return iterator for end of mutable sequence
				return base_class::template ss_end<_TMseArrayPointer>(owner_ptr);
			}
			template<typename _TMseArrayPointer>
			static Tss_const_iterator_type<_TMseArrayPointer> ss_cbegin(_TMseArrayPointer owner_ptr) {	// return iterator for beginning of nonmutable sequence
				return base_class::template ss_cbegin<_TMseArrayPointer>(owner_ptr);
			}
			template<typename _TMseArrayPointer>
			static Tss_const_iterator_type<_TMseArrayPointer> ss_cend(_TMseArrayPointer owner_ptr) {	// return iterator for end of nonmutable sequence
				return base_class::template ss_cend<_TMseArrayPointer>(owner_ptr);
			}
			template<typename _TMseArrayPointer>
			static Tss_reverse_iterator_type<_TMseArrayPointer> ss_rbegin(_TMseArrayPointer owner_ptr) {	// return iterator for beginning of reversed mutable sequence
				return (Tss_reverse_iterator_type<_TMseArrayPointer>(ss_end<_TMseArrayPointer>(owner_ptr)));
			}
			template<typename _TMseArrayPointer>
			static Tss_reverse_iterator_type<_TMseArrayPointer> ss_rend(_TMseArrayPointer owner_ptr) {	// return iterator for end of reversed mutable sequence
				return (Tss_reverse_iterator_type<_TMseArrayPointer>(ss_cbegin<_TMseArrayPointer>(owner_ptr)));
			}
			template<typename _TMseArrayPointer>
			static Tss_const_reverse_iterator_type<_TMseArrayPointer> ss_crbegin(_TMseArrayPointer owner_ptr) {	// return iterator for beginning of reversed nonmutable sequence
				return (Tss_const_reverse_iterator_type<_TMseArrayPointer>(ss_end<_TMseArrayPointer>(owner_ptr)));
			}

			class xscope_ss_iterator_type;

			class xscope_ss_const_iterator_type : public ss_const_iterator_type, public XScopeContainsNonOwningScopeReferenceTagBase, public StrongPointerNotAsyncShareableTagBase {
			public:
				template <typename _TXScopePointer, class = typename std::enable_if<
					std::is_convertible<_TXScopePointer, mse::TXScopeItemFixedConstPointer<msearray> >::value
					|| std::is_convertible<_TXScopePointer, mse::TXScopeItemFixedPointer<msearray> >::value
					|| std::is_convertible<_TXScopePointer, mse::TXScopeFixedConstPointer<msearray> >::value
					|| std::is_convertible<_TXScopePointer, mse::TXScopeFixedPointer<msearray> >::value
					, void>::type>
				xscope_ss_const_iterator_type(const _TXScopePointer& owner_ptr) : ss_const_iterator_type((*owner_ptr).ss_cbegin()) {}

				xscope_ss_const_iterator_type(const xscope_ss_const_iterator_type& src_cref) : ss_const_iterator_type(src_cref) {}
				xscope_ss_const_iterator_type(const xscope_ss_iterator_type& src_cref) : ss_const_iterator_type(src_cref) {}
				~xscope_ss_const_iterator_type() {}
				const ss_const_iterator_type& msearray_ss_const_iterator_type() const {
					return (*this);
				}
				ss_const_iterator_type& msearray_ss_const_iterator_type() {
					return (*this);
				}
				const ss_const_iterator_type& mvssci() const { return msearray_ss_const_iterator_type(); }
				ss_const_iterator_type& mvssci() { return msearray_ss_const_iterator_type(); }

				void reset() { ss_const_iterator_type::reset(); }
				bool points_to_an_item() const { return ss_const_iterator_type::points_to_an_item(); }
				bool points_to_end_marker() const { return ss_const_iterator_type::points_to_end_marker(); }
				bool points_to_beginning() const { return ss_const_iterator_type::points_to_beginning(); }
				/* has_next_item_or_end_marker() is just an alias for points_to_an_item(). */
				bool has_next_item_or_end_marker() const { return ss_const_iterator_type::has_next_item_or_end_marker(); }
				/* has_next() is just an alias for points_to_an_item() that's familiar to java programmers. */
				bool has_next() const { return ss_const_iterator_type::has_next(); }
				bool has_previous() const { return ss_const_iterator_type::has_previous(); }
				void set_to_beginning() { ss_const_iterator_type::set_to_beginning(); }
				void set_to_end_marker() { ss_const_iterator_type::set_to_end_marker(); }
				void set_to_next() { ss_const_iterator_type::set_to_next(); }
				void set_to_previous() { ss_const_iterator_type::set_to_previous(); }
				xscope_ss_const_iterator_type& operator ++() { ss_const_iterator_type::operator ++(); return (*this); }
				xscope_ss_const_iterator_type operator++(int) { xscope_ss_const_iterator_type _Tmp = *this; ss_const_iterator_type::operator++(); return (_Tmp); }
				xscope_ss_const_iterator_type& operator --() { ss_const_iterator_type::operator --(); return (*this); }
				xscope_ss_const_iterator_type operator--(int) { xscope_ss_const_iterator_type _Tmp = *this; ss_const_iterator_type::operator--(); return (_Tmp); }
				void advance(difference_type n) { ss_const_iterator_type::advance(n); }
				void regress(difference_type n) { ss_const_iterator_type::regress(n); }
				xscope_ss_const_iterator_type& operator +=(difference_type n) { ss_const_iterator_type::operator +=(n); return (*this); }
				xscope_ss_const_iterator_type& operator -=(difference_type n) { ss_const_iterator_type::operator -=(n); return (*this); }
				xscope_ss_const_iterator_type operator+(difference_type n) const { auto retval = (*this); retval += n; return retval; }
				xscope_ss_const_iterator_type operator-(difference_type n) const { return ((*this) + (-n)); }
				difference_type operator-(const xscope_ss_const_iterator_type& _Right_cref) const { return ss_const_iterator_type::operator-(_Right_cref); }
				const_reference operator*() const { return ss_const_iterator_type::operator*(); }
				const_reference item() const { return operator*(); }
				const_reference previous_item() const { return ss_const_iterator_type::previous_item(); }
				const_pointer operator->() const { return ss_const_iterator_type::operator->(); }
				const_reference operator[](difference_type _Off) const { return ss_const_iterator_type::operator[](_Off); }
				xscope_ss_const_iterator_type& operator=(const ss_const_iterator_type& _Right_cref) {
					if ((&(*_Right_cref.target_container_ptr())) != (&(*(*this).target_container_ptr()))) { MSE_THROW(msearray_range_error("invalid argument - xscope_ss_const_iterator_type& operator=(const xscope_ss_const_iterator_type& _Right_cref) - msearray::xscope_ss_const_iterator_type")); }
					ss_const_iterator_type::operator=(_Right_cref);
					return (*this);
				}
				xscope_ss_const_iterator_type& operator=(const ss_iterator_type& _Right_cref) {
					if ((&(*_Right_cref.target_container_ptr())) != (&(*(*this).target_container_ptr()))) { MSE_THROW(msearray_range_error("invalid argument - xscope_ss_const_iterator_type& operator=(const ss_iterator_type& _Right_cref) - msearray::xscope_ss_const_iterator_type")); }
					return operator=(ss_const_iterator_type(_Right_cref));
				}
				bool operator==(const xscope_ss_const_iterator_type& _Right_cref) const { return ss_const_iterator_type::operator==(_Right_cref); }
				bool operator!=(const xscope_ss_const_iterator_type& _Right_cref) const { return (!(_Right_cref == (*this))); }
				bool operator<(const xscope_ss_const_iterator_type& _Right) const { return ss_const_iterator_type::operator<(_Right); }
				bool operator<=(const xscope_ss_const_iterator_type& _Right) const { return ss_const_iterator_type::operator<=(_Right); }
				bool operator>(const xscope_ss_const_iterator_type& _Right) const { return ss_const_iterator_type::operator>(_Right); }
				bool operator>=(const xscope_ss_const_iterator_type& _Right) const { return ss_const_iterator_type::operator>=(_Right); }
				void set_to_const_item_pointer(const xscope_ss_const_iterator_type& _Right_cref) { ss_const_iterator_type::set_to_item_pointer(_Right_cref); }
				msear_size_t position() const { return ss_const_iterator_type::position(); }
				auto target_container_ptr() const {
					return mse::us::unsafe_make_xscope_const_pointer_to(*(ss_const_iterator_type::target_container_ptr()));
				}
				void xscope_ss_iterator_type_tag() const {}
				void not_async_shareable_tag() const {} /* Indication that this type is not eligible to be shared between threads. */
			private:
				void* operator new(size_t size) { return ::operator new(size); }

				//typename ss_const_iterator_type (*this);
				friend class /*_Myt*/msearray<_Ty, _Size>;
				friend class xscope_ss_iterator_type;
			};
			class xscope_ss_iterator_type : public ss_iterator_type, public XScopeContainsNonOwningScopeReferenceTagBase, public StrongPointerNotAsyncShareableTagBase {
			public:
				template <typename _TXScopePointer, class = typename std::enable_if<
					std::is_convertible<_TXScopePointer, mse::TXScopeItemFixedPointer<msearray> >::value
					|| std::is_convertible<_TXScopePointer, mse::TXScopeFixedPointer<msearray> >::value
					, void>::type>
				xscope_ss_iterator_type(const _TXScopePointer& owner_ptr) : ss_iterator_type((*owner_ptr).ss_begin()) {}

				xscope_ss_iterator_type(const xscope_ss_iterator_type& src_cref) : ss_iterator_type(src_cref) {}
				~xscope_ss_iterator_type() {}
				const ss_iterator_type& msearray_ss_iterator_type() const {
					return (*this);
				}
				ss_iterator_type& msearray_ss_iterator_type() {
					return (*this);
				}
				const ss_iterator_type& mvssi() const { return msearray_ss_iterator_type(); }
				ss_iterator_type& mvssi() { return msearray_ss_iterator_type(); }

				void reset() { ss_iterator_type::reset(); }
				bool points_to_an_item() const { return ss_iterator_type::points_to_an_item(); }
				bool points_to_end_marker() const { return ss_iterator_type::points_to_end_marker(); }
				bool points_to_beginning() const { return ss_iterator_type::points_to_beginning(); }
				/* has_next_item_or_end_marker() is just an alias for points_to_an_item(). */
				bool has_next_item_or_end_marker() const { return ss_iterator_type::has_next_item_or_end_marker(); }
				/* has_next() is just an alias for points_to_an_item() that's familiar to java programmers. */
				bool has_next() const { return ss_iterator_type::has_next(); }
				bool has_previous() const { return ss_iterator_type::has_previous(); }
				void set_to_beginning() { ss_iterator_type::set_to_beginning(); }
				void set_to_end_marker() { ss_iterator_type::set_to_end_marker(); }
				void set_to_next() { ss_iterator_type::set_to_next(); }
				void set_to_previous() { ss_iterator_type::set_to_previous(); }
				xscope_ss_iterator_type& operator ++() { ss_iterator_type::operator ++(); return (*this); }
				xscope_ss_iterator_type operator++(int) { xscope_ss_iterator_type _Tmp = *this; ss_iterator_type::operator++(); return (_Tmp); }
				xscope_ss_iterator_type& operator --() { ss_iterator_type::operator --(); return (*this); }
				xscope_ss_iterator_type operator--(int) { xscope_ss_iterator_type _Tmp = *this; ss_iterator_type::operator--(); return (_Tmp); }
				void advance(difference_type n) { ss_iterator_type::advance(n); }
				void regress(difference_type n) { ss_iterator_type::regress(n); }
				xscope_ss_iterator_type& operator +=(difference_type n) { ss_iterator_type::operator +=(n); return (*this); }
				xscope_ss_iterator_type& operator -=(difference_type n) { ss_iterator_type::operator -=(n); return (*this); }
				xscope_ss_iterator_type operator+(difference_type n) const { auto retval = (*this); retval += n; return retval; }
				xscope_ss_iterator_type operator-(difference_type n) const { return ((*this) + (-n)); }
				difference_type operator-(const xscope_ss_iterator_type& _Right_cref) const { return ss_iterator_type::operator-(_Right_cref); }
				reference operator*() const { return ss_iterator_type::operator*(); }
				reference item() const { return operator*(); }
				reference previous_item() const { return ss_iterator_type::previous_item(); }
				pointer operator->() const { return ss_iterator_type::operator->(); }
				reference operator[](difference_type _Off) const { return ss_iterator_type::operator[](_Off); }
				xscope_ss_iterator_type& operator=(const ss_iterator_type& _Right_cref) {
					if ((&(*_Right_cref.target_container_ptr())) != (&(*(*this).target_container_ptr()))) { MSE_THROW(msearray_range_error("invalid argument - xscope_ss_iterator_type& operator=(const xscope_ss_iterator_type& _Right_cref) - msearray::xscope_ss_iterator_type")); }
					ss_iterator_type::operator=(_Right_cref);
					return (*this);
				}
				bool operator==(const xscope_ss_iterator_type& _Right_cref) const { return ss_iterator_type::operator==(_Right_cref); }
				bool operator!=(const xscope_ss_iterator_type& _Right_cref) const { return (!(_Right_cref == (*this))); }
				bool operator<(const xscope_ss_iterator_type& _Right) const { return ss_iterator_type::operator<(_Right); }
				bool operator<=(const xscope_ss_iterator_type& _Right) const { return ss_iterator_type::operator<=(_Right); }
				bool operator>(const xscope_ss_iterator_type& _Right) const { return ss_iterator_type::operator>(_Right); }
				bool operator>=(const xscope_ss_iterator_type& _Right) const { return ss_iterator_type::operator>=(_Right); }
				void set_to_item_pointer(const xscope_ss_iterator_type& _Right_cref) { ss_iterator_type::set_to_item_pointer(_Right_cref); }
				msear_size_t position() const { return ss_iterator_type::position(); }
				auto target_container_ptr() const {
					return mse::us::unsafe_make_xscope_pointer_to(*(ss_iterator_type::target_container_ptr()));
				}
				void xscope_ss_iterator_type_tag() const {}
				void not_async_shareable_tag() const {} /* Indication that this type is not eligible to be shared between threads. */
			private:
				void* operator new(size_t size) { return ::operator new(size); }

				//typename ss_iterator_type (*this);
				friend class /*_Myt*/msearray<_Ty, _Size>;
			};

			typedef xscope_ss_const_iterator_type xscope_const_iterator;
			typedef xscope_ss_iterator_type xscope_iterator;

			void not_async_shareable_tag() const {} /* Indication that this type is not eligible to be shared between threads. */

		private:

			auto contained_array() const -> decltype(base_class::contained_array()) { return base_class::contained_array(); }
			auto contained_array() -> decltype(base_class::contained_array()) { return base_class::contained_array(); }

			template<size_t _Idx, class _Tz, size_t _Size2>
			friend _CONST_FUN _Tz& std::get(mse::us::msearray<_Tz, _Size2>& _Arr) _NOEXCEPT;
			template<size_t _Idx, class _Tz, size_t _Size2>
			friend _CONST_FUN const _Tz& std::get(const mse::us::msearray<_Tz, _Size2>& _Arr) _NOEXCEPT;
			template<size_t _Idx, class _Tz, size_t _Size2>
			friend _CONST_FUN _Tz&& std::get(mse::us::msearray<_Tz, _Size2>&& _Arr) _NOEXCEPT;
		};

		template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex> inline bool operator!=(const msearray<_Ty, _Size, _TStateMutex>& _Left,
			const msearray<_Ty, _Size, _TStateMutex>& _Right) {	// test for array inequality
			return (!(_Left == _Right));
		}

		template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex> inline bool operator>(const msearray<_Ty, _Size, _TStateMutex>& _Left,
			const msearray<_Ty, _Size, _TStateMutex>& _Right) {	// test if _Left > _Right for arrays
			return (_Right < _Left);
		}

		template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex> inline bool operator<=(const msearray<_Ty, _Size, _TStateMutex>& _Left,
			const msearray<_Ty, _Size, _TStateMutex>& _Right) {	// test if _Left <= _Right for arrays
			return (!(_Right < _Left));
		}

		template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex> inline bool operator>=(const msearray<_Ty, _Size, _TStateMutex>& _Left,
			const msearray<_Ty, _Size, _TStateMutex>& _Right) {	// test if _Left >= _Right for arrays
			return (!(_Left < _Right));
		}

		template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
		TXScopeItemFixedPointer<_Ty> xscope_pointer_to_array_element(const typename msearray<_Ty, _Size, _TStateMutex>::xscope_ss_iterator_type& iter_cref) {
			return mse::us::unsafe_make_xscope_pointer_to(*iter_cref);
		}
		template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
		TXScopeItemFixedPointer<_Ty> xscope_pointer_to_array_element(const mse::TXScopeItemFixedPointer<msearray<_Ty, _Size, _TStateMutex> >& ptr, typename msearray<_Ty, _Size, _TStateMutex>::size_type _P) {
			return mse::us::unsafe_make_xscope_pointer_to((*ptr)[_P]);
		}
#if !defined(MSE_SCOPEPOINTER_DISABLED)
		template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
		TXScopeItemFixedPointer<_Ty> xscope_pointer_to_array_element(const mse::TXScopeFixedPointer<msearray<_Ty, _Size, _TStateMutex> >& ptr, typename msearray<_Ty, _Size, _TStateMutex>::size_type _P) {
			return mse::us::unsafe_make_xscope_pointer_to((*ptr)[_P]);
		}
#endif // !defined(MSE_SCOPEPOINTER_DISABLED)

		template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
		TXScopeItemFixedConstPointer<_Ty> xscope_const_pointer_to_array_element(const typename msearray<_Ty, _Size, _TStateMutex>::xscope_ss_const_iterator_type& iter_cref) {
			return mse::us::unsafe_make_xscope_const_pointer_to(*iter_cref);
		}
		template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
		TXScopeItemFixedConstPointer<_Ty> xscope_const_pointer_to_array_element(const mse::TXScopeItemFixedConstPointer<msearray<_Ty, _Size, _TStateMutex> >& ptr, typename msearray<_Ty, _Size, _TStateMutex>::size_type _P) {
			return mse::us::unsafe_make_xscope_const_pointer_to((*ptr)[_P]);
		}
		template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
		TXScopeItemFixedConstPointer<_Ty> xscope_const_pointer_to_array_element(const mse::TXScopeItemFixedPointer<msearray<_Ty, _Size, _TStateMutex> >& ptr, typename msearray<_Ty, _Size, _TStateMutex>::size_type _P) {
			return mse::us::unsafe_make_xscope_const_pointer_to((*ptr)[_P]);
		}
#if !defined(MSE_SCOPEPOINTER_DISABLED)
		template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
		TXScopeItemFixedConstPointer<_Ty> xscope_const_pointer_to_array_element(const mse::TXScopeFixedConstPointer<msearray<_Ty, _Size, _TStateMutex> >& ptr, typename msearray<_Ty, _Size, _TStateMutex>::size_type _P) {
			return mse::us::unsafe_make_xscope_const_pointer_to((*ptr)[_P]);
		}
		template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
		TXScopeItemFixedConstPointer<_Ty> xscope_const_pointer_to_array_element(const mse::TXScopeFixedPointer<msearray<_Ty, _Size, _TStateMutex> >& ptr, typename msearray<_Ty, _Size, _TStateMutex>::size_type _P) {
			return mse::us::unsafe_make_xscope_const_pointer_to((*ptr)[_P]);
		}
#endif // !defined(MSE_SCOPEPOINTER_DISABLED)
	}

	/* Using the mse::msearray<> alias of mse::us::msearray<> is deprecated. */
	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
	using msearray = us::msearray< _Ty, _Size, _TStateMutex>;


	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
	TXScopeItemFixedPointer<_Ty> xscope_pointer_to_array_element(const typename msearray<_Ty, _Size, _TStateMutex>::xscope_ss_iterator_type& iter_cref) {
		return mse::us::unsafe_make_xscope_pointer_to(*iter_cref);
	}
	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
	TXScopeItemFixedPointer<_Ty> xscope_pointer_to_array_element(const mse::TXScopeItemFixedPointer<msearray<_Ty, _Size, _TStateMutex> >& ptr, typename msearray<_Ty, _Size, _TStateMutex>::size_type _P) {
		return mse::us::unsafe_make_xscope_pointer_to((*ptr)[_P]);
	}
#if !defined(MSE_SCOPEPOINTER_DISABLED)
	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
	TXScopeItemFixedPointer<_Ty> xscope_pointer_to_array_element(const mse::TXScopeFixedPointer<msearray<_Ty, _Size, _TStateMutex> >& ptr, typename msearray<_Ty, _Size, _TStateMutex>::size_type _P) {
		return mse::us::unsafe_make_xscope_pointer_to((*ptr)[_P]);
	}
#endif // !defined(MSE_SCOPEPOINTER_DISABLED)

	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
	TXScopeItemFixedConstPointer<_Ty> xscope_const_pointer_to_array_element(const typename msearray<_Ty, _Size, _TStateMutex>::xscope_ss_const_iterator_type& iter_cref) {
		return mse::us::unsafe_make_xscope_const_pointer_to(*iter_cref);
	}
	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
	TXScopeItemFixedConstPointer<_Ty> xscope_const_pointer_to_array_element(const mse::TXScopeItemFixedConstPointer<msearray<_Ty, _Size, _TStateMutex> >& ptr, typename msearray<_Ty, _Size, _TStateMutex>::size_type _P) {
		return mse::us::unsafe_make_xscope_const_pointer_to((*ptr)[_P]);
	}
	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
	TXScopeItemFixedConstPointer<_Ty> xscope_const_pointer_to_array_element(const mse::TXScopeItemFixedPointer<msearray<_Ty, _Size, _TStateMutex> >& ptr, typename msearray<_Ty, _Size, _TStateMutex>::size_type _P) {
		return mse::us::unsafe_make_xscope_const_pointer_to((*ptr)[_P]);
	}
#if !defined(MSE_SCOPEPOINTER_DISABLED)
	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
	TXScopeItemFixedConstPointer<_Ty> xscope_const_pointer_to_array_element(const mse::TXScopeFixedConstPointer<msearray<_Ty, _Size, _TStateMutex> >& ptr, typename msearray<_Ty, _Size, _TStateMutex>::size_type _P) {
		return mse::us::unsafe_make_xscope_const_pointer_to((*ptr)[_P]);
	}
	template<class _Ty, size_t _Size, class _TStateMutex = default_state_mutex>
	TXScopeItemFixedConstPointer<_Ty> xscope_const_pointer_to_array_element(const mse::TXScopeFixedPointer<msearray<_Ty, _Size, _TStateMutex> >& ptr, typename msearray<_Ty, _Size, _TStateMutex>::size_type _P) {
		return mse::us::unsafe_make_xscope_const_pointer_to((*ptr)[_P]);
	}
#endif // !defined(MSE_SCOPEPOINTER_DISABLED)


	template<class _TArray>
	auto make_xscope_const_iterator(const mse::TXScopeFixedConstPointer<_TArray>& owner_ptr) {
		return typename _TArray::xscope_const_iterator(owner_ptr);
	}
	template<class _TArray>
	auto make_xscope_const_iterator(const mse::TXScopeFixedPointer<_TArray>& owner_ptr) {
		return typename _TArray::xscope_const_iterator(owner_ptr);
	}
#if !defined(MSE_SCOPEPOINTER_DISABLED)
	template<class _TArray>
	auto make_xscope_const_iterator(const mse::TXScopeItemFixedConstPointer<_TArray>& owner_ptr) {
		return typename _TArray::xscope_const_iterator(owner_ptr);
	}
	template<class _TArray>
	auto make_xscope_const_iterator(const mse::TXScopeItemFixedPointer<_TArray>& owner_ptr) {
		return typename _TArray::xscope_const_iterator(owner_ptr);
	}
#endif // !defined(MSE_SCOPEPOINTER_DISABLED)

	template<class _TArray>
	auto make_xscope_iterator(const mse::TXScopeFixedPointer<_TArray>& owner_ptr) {
		return typename _TArray::xscope_iterator(owner_ptr);
	}
#if !defined(MSE_SCOPEPOINTER_DISABLED)
	template<class _TArray>
	auto make_xscope_iterator(const mse::TXScopeItemFixedPointer<_TArray>& owner_ptr) {
		return typename _TArray::xscope_iterator(owner_ptr);
	}
#endif // !defined(MSE_REGISTEREDPOINTER_DISABLED)


	/* deprecated */
	template<class _TArray>
	typename _TArray::xscope_ss_const_iterator_type make_xscope_ss_const_iterator_type(const mse::TXScopeFixedConstPointer<_TArray>& owner_ptr) {
		return typename _TArray::xscope_ss_const_iterator_type(owner_ptr);
	}
	template<class _TArray>
	typename _TArray::xscope_ss_const_iterator_type make_xscope_ss_const_iterator_type(const mse::TXScopeFixedPointer<_TArray>& owner_ptr) {
		return typename _TArray::xscope_ss_const_iterator_type(owner_ptr);
	}
#if !defined(MSE_SCOPEPOINTER_DISABLED)
	template<class _TArray>
	typename _TArray::xscope_ss_const_iterator_type make_xscope_ss_const_iterator_type(const mse::TXScopeItemFixedConstPointer<_TArray>& owner_ptr) {
		return typename _TArray::xscope_ss_const_iterator_type(owner_ptr);
	}
	template<class _TArray>
	typename _TArray::xscope_ss_const_iterator_type make_xscope_ss_const_iterator_type(const mse::TXScopeItemFixedPointer<_TArray>& owner_ptr) {
		return typename _TArray::xscope_ss_const_iterator_type(owner_ptr);
	}
#endif // !defined(MSE_SCOPEPOINTER_DISABLED)

	template<class _TArray>
	typename _TArray::xscope_ss_iterator_type make_xscope_ss_iterator_type(const mse::TXScopeFixedPointer<_TArray>& owner_ptr) {
		return typename _TArray::xscope_ss_iterator_type(owner_ptr);
	}
#if !defined(MSE_SCOPEPOINTER_DISABLED)
	template<class _TArray>
	typename _TArray::xscope_ss_iterator_type make_xscope_ss_iterator_type(const mse::TXScopeItemFixedPointer<_TArray>& owner_ptr) {
		return typename _TArray::xscope_ss_iterator_type(owner_ptr);
	}
#endif // !defined(MSE_SCOPEPOINTER_DISABLED)

}

namespace std {

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmismatched-tags"
#endif /*__clang__*/

	template<class _Ty, size_t _Size>
	struct tuple_size<mse::us::msearray<_Ty, _Size> >
		: integral_constant<size_t, _Size>
	{	// struct to determine number of elements in array
	};

	template<size_t _Idx, class _Ty, size_t _Size>
	struct tuple_element<_Idx, mse::us::msearray<_Ty, _Size> >
	{	// struct to determine type of element _Idx in array
		static_assert(_Idx < _Size, "array index out of bounds");

		typedef _Ty type;
	};

#ifdef __clang__
#pragma clang diagnostic pop
#endif /*__clang__*/

	// TUPLE INTERFACE TO array
	template<size_t _Idx, class _Ty, size_t _Size>
	_CONST_FUN _Ty& get(mse::us::msearray<_Ty, _Size>& _Arr) _NOEXCEPT
	{	// return element at _Idx in array _Arr
		static_assert(_Idx < _Size, "array index out of bounds");
		return (std::get<_Idx>(_Arr.contained_array()));
	}

	template<size_t _Idx, class _Ty, size_t _Size>
	_CONST_FUN const _Ty& get(const mse::us::msearray<_Ty, _Size>& _Arr) _NOEXCEPT
	{	// return element at _Idx in array _Arr
		static_assert(_Idx < _Size, "array index out of bounds");
		return (std::get<_Idx>(_Arr.contained_array()));
	}

	template<size_t _Idx, class _Ty, size_t _Size>
	_CONST_FUN _Ty&& get(mse::us::msearray<_Ty, _Size>&& _Arr) _NOEXCEPT
	{	// return element at _Idx in array _Arr
		static_assert(_Idx < _Size, "array index out of bounds");
		return (_STD move(std::get<_Idx>(_Arr.contained_array())));
	}

	template<class _Ty, size_t _Size, class _TStateMutex = mse::default_state_mutex/*, class = enable_if_t<_Size == 0 || _Is_swappable<_Ty>::value>*/>
	void swap(mse::us::msearray<_Ty, _Size, _TStateMutex>& _Left, mse::us::msearray<_Ty, _Size, _TStateMutex>& _Right) _NOEXCEPT
	{
		_Left.swap(_Right);
	}
	template<class _Ty, size_t _Size, class _TStateMutex = mse::default_state_mutex/*, class = enable_if_t<_Size == 0 || _Is_swappable<_Ty>::value>*/>
	void swap(array<_Ty, _Size>& _Left, mse::us::msearray<_Ty, _Size, _TStateMutex>& _Right) _NOEXCEPT_OP(_NOEXCEPT_OP(_Right.swap(_Left)))
	{	// swap arrays
		return (_Right.swap(_Left));
	}
	template<class _Ty, size_t _Size, class _TStateMutex = mse::default_state_mutex/*, class = enable_if_t<_Size == 0 || _Is_swappable<_Ty>::value>*/>
	void swap(mse::us::msearray<_Ty, _Size, _TStateMutex>& _Left, array<_Ty, _Size>& _Right) _NOEXCEPT
	{
		_Left.swap(_Right);
	}
}

namespace mse {

	template <typename _TRAIterator>
	class random_access_const_iterator_base_from_ra_iterator {
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = typename std::remove_reference<decltype(*std::declval<_TRAIterator>())>::type;
		using difference_type = decltype(std::declval<_TRAIterator>() - std::declval<_TRAIterator>());
		using const_pointer = typename std::add_pointer<typename std::add_const<decltype(*std::declval<_TRAIterator>())>::type>::type;
		using const_reference = typename std::add_const<decltype(*std::declval<_TRAIterator>())>::type;
	};
	template <typename _TRAIterator>
	class random_access_iterator_base_from_ra_iterator {
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = typename std::remove_reference<decltype(*std::declval<_TRAIterator>())>::type;
		using difference_type = decltype(std::declval<_TRAIterator>() - std::declval<_TRAIterator>());
		using pointer = typename std::add_pointer<decltype(*std::declval<_TRAIterator>())>::type;
		using reference = decltype(*std::declval<_TRAIterator>());
	};

	template <typename _TRAContainer>
	class random_access_const_iterator_base_from_ra_container {
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = typename std::remove_reference<decltype(std::declval<_TRAContainer>()[0])>::type;
		using difference_type = typename mse::us::msearray<int, 0>::difference_type;
		using const_pointer = typename std::add_pointer<typename std::add_const<decltype(std::declval<_TRAContainer>()[0])>::type>::type;
		using const_reference = typename std::add_const<decltype(std::declval<_TRAContainer>()[0])>::type;
	};
	template <typename _TRAContainer>
	class random_access_iterator_base_from_ra_container {
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = typename std::remove_reference<decltype(std::declval<_TRAContainer>()[0])>::type;
		using difference_type = typename mse::us::msearray<int, 0>::difference_type;
		using pointer = typename std::add_pointer<decltype(std::declval<_TRAContainer>()[0])>::type;
		using reference = decltype(std::declval<_TRAContainer>()[0]);
	};

	template <typename _TRAContainerPointer> class TRAConstIteratorBase;

	template <typename _TRAContainerPointer>
	class TRAIteratorBase : public random_access_iterator_base_from_ra_container<decltype(*std::declval<_TRAContainerPointer>())>
		, public std::conditional<std::is_base_of<ContainsNonOwningScopeReferenceTagBase, _TRAContainerPointer>::value, ContainsNonOwningScopeReferenceTagBase, TPlaceHolder_msescope<TRAIteratorBase<_TRAContainerPointer> > >::type
	{
	public:
		typedef typename std::remove_reference<typename std::remove_const<decltype((*std::declval<_TRAContainerPointer>())[0])>::type>::type value_type;
		typedef decltype((*std::declval<_TRAContainerPointer>())[0]) reference;
		typedef typename std::add_lvalue_reference<typename std::add_const<value_type>::type>::type const_reference;
		typedef typename mse::us::msearray<int, 0>::size_type size_type;
		typedef typename mse::us::msearray<int, 0>::difference_type difference_type;

	private:
		const _TRAContainerPointer m_ra_container_pointer;
		difference_type m_index = 0;

	public:
		TRAIteratorBase(const TRAIteratorBase& src)
			: m_ra_container_pointer(src.m_ra_container_pointer), m_index(src.m_index) {}
		TRAIteratorBase(_TRAContainerPointer ra_container_pointer, size_type index = 0)
			: m_ra_container_pointer(ra_container_pointer), m_index(difference_type(index)) {}

		auto operator*() const -> reference {
			return (*m_ra_container_pointer)[m_index];
		}
		auto operator->() const -> typename std::add_pointer<value_type>::type {
			return std::addressof((*m_ra_container_pointer)[m_index]);
		}
		reference operator[](difference_type _Off) const { return (*m_ra_container_pointer)[size_type(m_index + _Off)]; }
		TRAIteratorBase& operator +=(difference_type x) {
			m_index += (x);
			return (*this);
		}
		TRAIteratorBase& operator -=(difference_type x) { operator +=(-x); return (*this); }
		TRAIteratorBase& operator ++() { operator +=(1); return (*this); }
		TRAIteratorBase& operator ++(int) { auto _Tmp = *this; operator +=(1); return (_Tmp); }
		TRAIteratorBase& operator --() { operator -=(1); return (*this); }
		TRAIteratorBase& operator --(int) { auto _Tmp = *this; operator -=(1); return (_Tmp); }

		TRAIteratorBase operator+(difference_type n) const { auto retval = (*this); retval += n; return retval; }
		TRAIteratorBase operator-(difference_type n) const { return ((*this) + (-n)); }
		difference_type operator-(const TRAIteratorBase& _Right_cref) const {
			if (!(_Right_cref.m_ra_container_pointer == m_ra_container_pointer)) { MSE_THROW(msearray_range_error("invalid argument - difference_type operator-() - TRAIteratorBase")); }
			return m_index - _Right_cref.m_index;
		}
		bool operator ==(const TRAIteratorBase& _Right_cref) const {
			return ((_Right_cref.m_index == m_index) && (_Right_cref.m_ra_container_pointer == m_ra_container_pointer));
		}
		bool operator !=(const TRAIteratorBase& _Right_cref) const { return !((*this) == _Right_cref); }
		bool operator<(const TRAIteratorBase& _Right_cref) const { return (0 > operator-(_Right_cref)); }
		bool operator>(const TRAIteratorBase& _Right_cref) const { return (0 > operator-(_Right_cref)); }
		bool operator<=(const TRAIteratorBase& _Right_cref) const { return (0 >= operator-(_Right_cref)); }
		bool operator>=(const TRAIteratorBase& _Right_cref) const { return (0 >= operator-(_Right_cref)); }
		TRAIteratorBase& operator=(const TRAIteratorBase& _Right_cref) {
			if (!(_Right_cref.m_ra_container_pointer == m_ra_container_pointer)) { MSE_THROW(msearray_range_error("invalid argument - TRAIteratorBase& operator=() - TRAIteratorBase")); }
			m_index = _Right_cref.m_index;
			return (*this);
		}
		friend class TRAConstIteratorBase<_TRAContainerPointer>;
	};

	template <typename _TRAContainerPointer>
	class TXScopeRAIterator : public TRAIteratorBase<_TRAContainerPointer>, public XScopeTagBase {
	public:
		typedef TRAIteratorBase<_TRAContainerPointer> base_class;
		typedef typename base_class::value_type value_type;
		typedef typename base_class::reference reference;
		typedef typename base_class::const_reference const_reference;
		typedef typename base_class::size_type size_type;
		typedef typename base_class::difference_type difference_type;

		TXScopeRAIterator(const TRAIteratorBase<_TRAContainerPointer>& src)
			: base_class(src) {}
		TXScopeRAIterator(_TRAContainerPointer ra_container_pointer, size_type index = 0)
			: base_class(ra_container_pointer, index) {}

		TXScopeRAIterator& operator +=(difference_type x) {
			base_class::operator +=(x);
			return (*this);
		}
		TXScopeRAIterator& operator -=(difference_type x) { operator +=(-x); return (*this); }
		TXScopeRAIterator& operator ++() { operator +=(1); return (*this); }
		TXScopeRAIterator& operator ++(int) { auto _Tmp = *this; operator +=(1); return (_Tmp); }
		TXScopeRAIterator& operator --() { operator -=(1); return (*this); }
		TXScopeRAIterator& operator --(int) { auto _Tmp = *this; operator -=(1); return (_Tmp); }

		TXScopeRAIterator operator+(difference_type n) const { auto retval = (*this); retval += n; return retval; }
		TXScopeRAIterator operator-(difference_type n) const { return ((*this) + (-n)); }
		difference_type operator-(const TRAIteratorBase<_TRAContainerPointer>& _Right_cref) const {
			return base_class::operator-(_Right_cref);
		}

		TXScopeRAIterator& operator=(const TRAIteratorBase<_TRAContainerPointer>& _Right_cref) {
			base_class::operator=(_Right_cref);
			return (*this);
		}

		void xscope_tag() const {}
		void xscope_iterator_tag() const {}
		void not_async_shareable_tag() const {} /* Indication that this type is not eligible to be shared between threads. */
	private:
		void* operator new(size_t size) { return ::operator new(size); }

		TXScopeRAIterator* operator&() { return this; }
		const TXScopeRAIterator* operator&() const { return this; }
	};

	template <typename _TRAContainerPointer>
	class TRAIterator : public TRAIteratorBase<_TRAContainerPointer> {
	public:
		typedef TRAIteratorBase<_TRAContainerPointer> base_class;
		typedef typename base_class::value_type value_type;
		typedef typename base_class::reference reference;
		typedef typename base_class::const_reference const_reference;
		typedef typename base_class::size_type size_type;
		typedef typename base_class::difference_type difference_type;

		TRAIterator(const TRAIterator& src) : base_class(src) {}

		template <typename _TRAContainerPointer1>
		TRAIterator(_TRAContainerPointer1 ra_container_pointer, size_type index = 0) : base_class(ra_container_pointer, index) {}

		virtual ~TRAIterator() {
			mse::T_valid_if_not_an_xscope_type<_TRAContainerPointer>();
		}

		TRAIterator& operator +=(difference_type x) {
			base_class::operator +=(x);
			return (*this);
		}
		TRAIterator& operator -=(difference_type x) { operator +=(-x); return (*this); }
		TRAIterator& operator ++() { operator +=(1); return (*this); }
		TRAIterator& operator ++(int) { auto _Tmp = *this; operator +=(1); return (_Tmp); }
		TRAIterator& operator --() { operator -=(1); return (*this); }
		TRAIterator& operator --(int) { auto _Tmp = *this; operator -=(1); return (_Tmp); }

		TRAIterator operator+(difference_type n) const { auto retval = (*this); retval += n; return retval; }
		TRAIterator operator-(difference_type n) const { return ((*this) + (-n)); }
		difference_type operator-(const TRAIteratorBase<_TRAContainerPointer>& _Right_cref) const {
			return base_class::operator-(_Right_cref);
		}

		TRAIterator& operator=(const TRAIteratorBase<_TRAContainerPointer>& _Right_cref) {
			base_class::operator=(_Right_cref);
			return (*this);
		}
	};

	template <typename _TRAContainerPointer>
	class TRAConstIteratorBase : public random_access_const_iterator_base_from_ra_container<decltype(*std::declval<_TRAContainerPointer>())>
		, public std::conditional<std::is_base_of<ContainsNonOwningScopeReferenceTagBase, _TRAContainerPointer>::value, ContainsNonOwningScopeReferenceTagBase, TPlaceHolder_msescope<TRAConstIteratorBase<_TRAContainerPointer> > >::type
	{
	public:
		typedef typename std::remove_reference<typename std::remove_const<decltype((*std::declval<_TRAContainerPointer>())[0])>::type>::type value_type;
		typedef decltype((*std::declval<_TRAContainerPointer>())[0]) reference;
		typedef typename std::add_lvalue_reference<typename std::add_const<value_type>::type>::type const_reference;
		typedef typename mse::us::msearray<int, 0>::size_type size_type;
		typedef typename mse::us::msearray<int, 0>::difference_type difference_type;

	private:
		const _TRAContainerPointer m_ra_container_pointer;
		difference_type m_index = 0;

	public:
		TRAConstIteratorBase(const TRAConstIteratorBase& src)
			: m_ra_container_pointer(src.m_ra_container_pointer), m_index(src.m_index) {}
		TRAConstIteratorBase(const TRAIteratorBase<_TRAContainerPointer>& src)
			: m_ra_container_pointer(src.m_ra_container_pointer), m_index(src.m_index) {}
		TRAConstIteratorBase(_TRAContainerPointer ra_container_pointer, size_type index = 0)
			: m_ra_container_pointer(ra_container_pointer), m_index(difference_type(index)) {}

		auto operator*() const -> const_reference {
			return (*m_ra_container_pointer)[m_index];
		}
		auto operator->() const -> typename std::add_pointer<typename std::add_const<value_type>::type>::type {
			return std::addressof((*m_ra_container_pointer)[m_index]);
		}
		const_reference operator[](difference_type _Off) const { return (*m_ra_container_pointer)[size_type(m_index + _Off)]; }
		TRAConstIteratorBase& operator +=(difference_type x) {
			m_index += (x);
			return (*this);
		}
		TRAConstIteratorBase& operator -=(difference_type x) { operator +=(-x); return (*this); }
		TRAConstIteratorBase& operator ++() { operator +=(1); return (*this); }
		TRAConstIteratorBase& operator ++(int) { auto _Tmp = *this; operator +=(1); return (_Tmp); }
		TRAConstIteratorBase& operator --() { operator -=(1); return (*this); }
		TRAConstIteratorBase& operator --(int) { auto _Tmp = *this; operator -=(1); return (_Tmp); }

		TRAConstIteratorBase operator+(difference_type n) const { auto retval = (*this); retval += n; return retval; }
		TRAConstIteratorBase operator-(difference_type n) const { return ((*this) + (-n)); }
		difference_type operator-(const TRAConstIteratorBase& _Right_cref) const {
			if (!(_Right_cref.m_ra_container_pointer == m_ra_container_pointer)) { MSE_THROW(msearray_range_error("invalid argument - difference_type operator-() - TRAConstIteratorBase")); }
			return m_index - _Right_cref.m_index;
		}
		bool operator ==(const TRAConstIteratorBase& _Right_cref) const {
			return ((_Right_cref.m_index == m_index) && (_Right_cref.m_ra_container_pointer == m_ra_container_pointer));
		}
		bool operator !=(const TRAConstIteratorBase& _Right_cref) const { return !((*this) == _Right_cref); }
		bool operator<(const TRAConstIteratorBase& _Right_cref) const { return (0 > operator-(_Right_cref)); }
		bool operator>(const TRAConstIteratorBase& _Right_cref) const { return (0 > operator-(_Right_cref)); }
		bool operator<=(const TRAConstIteratorBase& _Right_cref) const { return (0 >= operator-(_Right_cref)); }
		bool operator>=(const TRAConstIteratorBase& _Right_cref) const { return (0 >= operator-(_Right_cref)); }
		TRAConstIteratorBase& operator=(const TRAConstIteratorBase& _Right_cref) {
			if (!(_Right_cref.m_ra_container_pointer == m_ra_container_pointer)) { MSE_THROW(msearray_range_error("invalid argument - TRAConstIteratorBase& operator=() - TRAConstIteratorBase")); }
			m_index = _Right_cref.m_index;
			return (*this);
		}
	};

	template <typename _TRAContainerPointer>
	class TXScopeRAConstIterator : public TRAConstIteratorBase<_TRAContainerPointer>, public XScopeTagBase {
	public:
		typedef TRAConstIteratorBase<_TRAContainerPointer> base_class;
		typedef typename base_class::value_type value_type;
		typedef typename base_class::const_reference const_reference;
		typedef typename base_class::size_type size_type;
		typedef typename base_class::difference_type difference_type;

		TXScopeRAConstIterator(const TRAConstIteratorBase<_TRAContainerPointer>& src)
			: base_class(src) {}
		TXScopeRAConstIterator(_TRAContainerPointer ra_container_pointer, size_type index = 0)
			: base_class(ra_container_pointer, index) {}

		TXScopeRAConstIterator& operator +=(difference_type x) {
			base_class::operator +=(x);
			return (*this);
		}
		TXScopeRAConstIterator& operator -=(difference_type x) { operator +=(-x); return (*this); }
		TXScopeRAConstIterator& operator ++() { operator +=(1); return (*this); }
		TXScopeRAConstIterator& operator ++(int) { auto _Tmp = *this; operator +=(1); return (_Tmp); }
		TXScopeRAConstIterator& operator --() { operator -=(1); return (*this); }
		TXScopeRAConstIterator& operator --(int) { auto _Tmp = *this; operator -=(1); return (_Tmp); }

		TXScopeRAConstIterator operator+(difference_type n) const { auto retval = (*this); retval += n; return retval; }
		TXScopeRAConstIterator operator-(difference_type n) const { return ((*this) + (-n)); }
		difference_type operator-(const TRAConstIteratorBase<_TRAContainerPointer>& _Right_cref) const {
			return base_class::operator-(_Right_cref);
		}

		TXScopeRAConstIterator& operator=(const TRAConstIteratorBase<_TRAContainerPointer>& _Right_cref) {
			base_class::operator=(_Right_cref);
			return (*this);
		}

		void xscope_tag() const {}
		void xscope_iterator_tag() const {}
		void not_async_shareable_tag() const {} /* Indication that this type is not eligible to be shared between threads. */
	private:
		void* operator new(size_t size) { return ::operator new(size); }

		TXScopeRAConstIterator* operator&() { return this; }
		const TXScopeRAConstIterator* operator&() const { return this; }
	};

	template <typename _TRAContainerPointer>
	class TRAConstIterator : public TRAConstIteratorBase<_TRAContainerPointer> {
	public:
		typedef TRAConstIteratorBase<_TRAContainerPointer> base_class;
		typedef typename base_class::value_type value_type;
		typedef typename base_class::const_reference const_reference;
		typedef typename base_class::size_type size_type;
		typedef typename base_class::difference_type difference_type;

		TRAConstIterator(const TRAConstIterator& src) : base_class(src) {}

		template <typename _TRAContainerPointer1>
		TRAConstIterator(_TRAContainerPointer1 ra_container_pointer, size_type index = 0) : base_class(ra_container_pointer, index) {}

		virtual ~TRAConstIterator() {
			mse::T_valid_if_not_an_xscope_type<_TRAContainerPointer>();
		}

		TRAConstIterator& operator +=(difference_type x) {
			base_class::operator +=(x);
			return (*this);
		}
		TRAConstIterator& operator -=(difference_type x) { operator +=(-x); return (*this); }
		TRAConstIterator& operator ++() { operator +=(1); return (*this); }
		TRAConstIterator& operator ++(int) { auto _Tmp = *this; operator +=(1); return (_Tmp); }
		TRAConstIterator& operator --() { operator -=(1); return (*this); }
		TRAConstIterator& operator --(int) { auto _Tmp = *this; operator -=(1); return (_Tmp); }

		TRAConstIterator operator+(difference_type n) const { auto retval = (*this); retval += n; return retval; }
		TRAConstIterator operator-(difference_type n) const { return ((*this) + (-n)); }
		difference_type operator-(const TRAConstIteratorBase<_TRAContainerPointer>& _Right_cref) const {
			return base_class::operator-(_Right_cref);
		}

		TRAConstIterator& operator=(const TRAConstIteratorBase<_TRAContainerPointer>& _Right_cref) {
			base_class::operator=(_Right_cref);
			return (*this);
		}
	};

	template <typename _TRAContainerPointer>
	using TXScopeRandomAccessIterator = TXScopeRAIterator<_TRAContainerPointer>;
	template <typename _TRAContainerPointer>
	using TRandomAccessIterator = TRAIterator<_TRAContainerPointer>;
	template <typename _TRAContainerPointer>
	using TXScopeRandomAccessConstIterator = TXScopeRAConstIterator<_TRAContainerPointer>;
	template <typename _TRAContainerPointer>
	using TRandomAccessConstIterator = TRAConstIterator<_TRAContainerPointer>;

	template <typename _TRAContainerPointer>
	auto make_xscope_random_access_iterator(const _TRAContainerPointer& ra_container_pointer, typename TXScopeRandomAccessIterator<_TRAContainerPointer>::size_type index = 0) {
		return TXScopeRandomAccessIterator<_TRAContainerPointer>(ra_container_pointer, index);
	}
	template <typename _TRAContainerPointer>
	auto make_random_access_iterator(const _TRAContainerPointer& ra_container_pointer, typename TRandomAccessIterator<_TRAContainerPointer>::size_type index = 0) {
		return TRandomAccessIterator<_TRAContainerPointer>(ra_container_pointer, index);
	}
	template <typename _TRAContainerPointer>
	auto make_xscope_random_access_const_iterator(const _TRAContainerPointer& ra_container_pointer, typename TXScopeRandomAccessConstIterator<_TRAContainerPointer>::size_type index = 0) {
		return TXScopeRandomAccessConstIterator<_TRAContainerPointer>(ra_container_pointer, index);
	}
	template <typename _TRAContainerPointer>
	auto make_random_access_const_iterator(const _TRAContainerPointer& ra_container_pointer, typename TRandomAccessConstIterator<_TRAContainerPointer>::size_type index = 0) {
		return TRandomAccessConstIterator<_TRAContainerPointer>(ra_container_pointer, index);
	}


	template <typename _TRAIterator> class TRASectionConstIteratorBase;

	template <typename _TRAIterator>
	class TRASectionIteratorBase : public random_access_iterator_base_from_ra_iterator<_TRAIterator>
		, public std::conditional<std::is_base_of<ContainsNonOwningScopeReferenceTagBase, _TRAIterator>::value, ContainsNonOwningScopeReferenceTagBase, TPlaceHolder_msescope<TRASectionIteratorBase<_TRAIterator> > >::type
	{
	public:
		typedef typename std::remove_reference<decltype(std::declval<_TRAIterator>()[0])>::type value_type;
		typedef decltype(std::declval<_TRAIterator>()[0]) reference;
		typedef typename std::add_lvalue_reference<typename std::add_const<value_type>::type>::type const_reference;
		typedef decltype(std::declval<_TRAIterator>() - std::declval<_TRAIterator>()) difference_type;
		typedef typename mse::nii_array<int, 0>::size_type size_type;

	private:
		const _TRAIterator m_ra_iterator;
		const size_type m_count = 0;
		difference_type m_index = 0;

	public:
		TRASectionIteratorBase(const TRASectionIteratorBase& src)
			: m_ra_iterator(src.m_ra_iterator), m_count(src.m_count), m_index(src.m_index) {}
		TRASectionIteratorBase(_TRAIterator ra_iterator, size_type count, size_type index = 0)
			: m_ra_iterator(ra_iterator), m_count(count), m_index(difference_type(index)) {}

		void bounds_check(difference_type index) const {
			if ((0 > index) || (difference_type(m_count) <= index)) {
				MSE_THROW(msearray_range_error("out of bounds index - void bounds_check() - TRASectionIteratorBase"));
			}
		}
		void dereference_bounds_check() const {
			bounds_check(m_index);
		}
		auto operator*() const -> reference {
			dereference_bounds_check();
			return m_ra_iterator[m_index];
		}
		auto operator->() const -> typename std::add_pointer<value_type>::type {
			dereference_bounds_check();
			return std::addressof(m_ra_iterator[m_index]);
		}
		reference operator[](difference_type _Off) const {
			bounds_check(_Off);
			return m_ra_iterator[_Off];
		}
		TRASectionIteratorBase& operator +=(difference_type x) {
			m_index += (x);
			return (*this);
		}
		TRASectionIteratorBase& operator -=(difference_type x) { operator +=(-x); return (*this); }
		TRASectionIteratorBase& operator ++() { operator +=(1); return (*this); }
		TRASectionIteratorBase& operator ++(int) { auto _Tmp = *this; operator +=(1); return (_Tmp); }
		TRASectionIteratorBase& operator --() { operator -=(1); return (*this); }
		TRASectionIteratorBase& operator --(int) { auto _Tmp = *this; operator -=(1); return (_Tmp); }

		TRASectionIteratorBase operator+(difference_type n) const { auto retval = (*this); retval += n; return retval; }
		TRASectionIteratorBase operator-(difference_type n) const { return ((*this) + (-n)); }
		difference_type operator-(const TRASectionIteratorBase& _Right_cref) const {
			if (!(_Right_cref.m_ra_iterator == m_ra_iterator)) { MSE_THROW(msearray_range_error("invalid argument - difference_type operator-() - TRASectionIteratorBase")); }
			return m_index - _Right_cref.m_index;
		}
		bool operator ==(const TRASectionIteratorBase& _Right_cref) const {
			return ((_Right_cref.m_index == m_index) && (_Right_cref.m_count == m_count) && (_Right_cref.m_ra_iterator == m_ra_iterator));
		}
		bool operator !=(const TRASectionIteratorBase& _Right_cref) const { return !((*this) == _Right_cref); }
		bool operator<(const TRASectionIteratorBase& _Right_cref) const { return (0 > operator-(_Right_cref)); }
		bool operator>(const TRASectionIteratorBase& _Right_cref) const { return (0 > operator-(_Right_cref)); }
		bool operator<=(const TRASectionIteratorBase& _Right_cref) const { return (0 >= operator-(_Right_cref)); }
		bool operator>=(const TRASectionIteratorBase& _Right_cref) const { return (0 >= operator-(_Right_cref)); }
		TRASectionIteratorBase& operator=(const TRASectionIteratorBase& _Right_cref) {
			if (!(_Right_cref.m_ra_iterator == m_ra_iterator)) { MSE_THROW(msearray_range_error("invalid argument - TRASectionIteratorBase& operator=() - TRASectionIteratorBase")); }
			m_index = _Right_cref.m_index;
			return (*this);
		}
		friend class TRASectionConstIteratorBase<_TRAIterator>;
	};

	template <typename _TRAIterator>
	class TXScopeRASectionIterator : public TRASectionIteratorBase<_TRAIterator>, public XScopeTagBase {
	public:
		typedef TRASectionIteratorBase<_TRAIterator> base_class;
		typedef typename base_class::value_type value_type;
		typedef typename base_class::reference reference;
		typedef typename base_class::const_reference const_reference;
		typedef typename base_class::size_type size_type;
		typedef typename base_class::difference_type difference_type;

		TXScopeRASectionIterator(const TRASectionIteratorBase<_TRAIterator>& src)
			: base_class(src) {}
		TXScopeRASectionIterator(_TRAIterator ra_iterator, size_type count, size_type index = 0)
			: base_class(ra_iterator, count, index) {}

		TXScopeRASectionIterator& operator +=(difference_type x) {
			base_class::operator +=(x);
			return (*this);
		}
		TXScopeRASectionIterator& operator -=(difference_type x) { operator +=(-x); return (*this); }
		TXScopeRASectionIterator& operator ++() { operator +=(1); return (*this); }
		TXScopeRASectionIterator& operator ++(int) { auto _Tmp = *this; operator +=(1); return (_Tmp); }
		TXScopeRASectionIterator& operator --() { operator -=(1); return (*this); }
		TXScopeRASectionIterator& operator --(int) { auto _Tmp = *this; operator -=(1); return (_Tmp); }

		TXScopeRASectionIterator operator+(difference_type n) const { auto retval = (*this); retval += n; return retval; }
		TXScopeRASectionIterator operator-(difference_type n) const { return ((*this) + (-n)); }
		difference_type operator-(const TRASectionIteratorBase<_TRAIterator>& _Right_cref) const {
			return base_class::operator-(_Right_cref);
		}

		TXScopeRASectionIterator& operator=(const TRASectionIteratorBase<_TRAIterator>& _Right_cref) {
			base_class::operator=(_Right_cref);
			return (*this);
		}

		void xscope_tag() const {}
		void xscope_iterator_tag() const {}
		void not_async_shareable_tag() const {} /* Indication that this type is not eligible to be shared between threads. */
	private:
		void* operator new(size_t size) { return ::operator new(size); }

		TXScopeRASectionIterator* operator&() { return this; }
		const TXScopeRASectionIterator* operator&() const { return this; }
	};

	template <typename _TRAIterator>
	class TRASectionIterator : public TRASectionIteratorBase<_TRAIterator> {
	public:
		typedef TRASectionIteratorBase<_TRAIterator> base_class;
		typedef typename base_class::value_type value_type;
		typedef typename base_class::reference reference;
		typedef typename base_class::const_reference const_reference;
		typedef typename base_class::size_type size_type;
		typedef typename base_class::difference_type difference_type;

		TRASectionIterator(const TRASectionIterator& src)
			: base_class(src) {}
		template <typename _TRAIterator1>
		TRASectionIterator(_TRAIterator1 ra_iterator, size_type count, size_type index = 0) : base_class(ra_iterator, count, index) {}
		virtual ~TRASectionIterator() {
			mse::T_valid_if_not_an_xscope_type<_TRAIterator>();
		}

		TRASectionIterator& operator +=(difference_type x) {
			base_class::operator +=(x);
			return (*this);
		}
		TRASectionIterator& operator -=(difference_type x) { operator +=(-x); return (*this); }
		TRASectionIterator& operator ++() { operator +=(1); return (*this); }
		TRASectionIterator& operator ++(int) { auto _Tmp = *this; operator +=(1); return (_Tmp); }
		TRASectionIterator& operator --() { operator -=(1); return (*this); }
		TRASectionIterator& operator --(int) { auto _Tmp = *this; operator -=(1); return (_Tmp); }

		TRASectionIterator operator+(difference_type n) const { auto retval = (*this); retval += n; return retval; }
		TRASectionIterator operator-(difference_type n) const { return ((*this) + (-n)); }
		difference_type operator-(const TRASectionIteratorBase<_TRAIterator>& _Right_cref) const {
			return base_class::operator-(_Right_cref);
		}

		TRASectionIterator& operator=(const TRASectionIteratorBase<_TRAIterator>& _Right_cref) {
			base_class::operator=(_Right_cref);
			return (*this);
		}
	};

	template <typename _TRAIterator>
	class TRASectionConstIteratorBase : public random_access_const_iterator_base_from_ra_iterator<_TRAIterator>
		, public std::conditional<std::is_base_of<ContainsNonOwningScopeReferenceTagBase, _TRAIterator>::value, ContainsNonOwningScopeReferenceTagBase, TPlaceHolder_msescope<TRASectionConstIteratorBase<_TRAIterator> > >::type
	{
	public:
		typedef typename std::remove_reference<decltype(std::declval<_TRAIterator>()[0])>::type value_type;
		typedef decltype(std::declval<_TRAIterator>()[0]) reference;
		typedef typename std::add_lvalue_reference<typename std::add_const<value_type>::type>::type const_reference;
		typedef decltype(std::declval<_TRAIterator>() - std::declval<_TRAIterator>()) difference_type;
		typedef typename mse::nii_array<int, 0>::size_type size_type;

	private:
		const _TRAIterator m_ra_iterator;
		const size_type m_count = 0;
		difference_type m_index = 0;

	public:
		TRASectionConstIteratorBase(const TRASectionConstIteratorBase& src)
			: m_ra_iterator(src.m_ra_iterator), m_count(src.m_count), m_index(src.m_index) {}
		TRASectionConstIteratorBase(const TRASectionIteratorBase<_TRAIterator>& src)
			: m_ra_iterator(src.m_ra_iterator), m_count(src.m_count), m_index(src.m_index) {}
		TRASectionConstIteratorBase(_TRAIterator ra_iterator, size_type count, size_type index = 0)
			: m_ra_iterator(ra_iterator), m_count(count), m_index(difference_type(index)) {}

		void bounds_check(difference_type index) const {
			if ((0 > index) || (difference_type(m_count) <= index)) {
				MSE_THROW(msearray_range_error("out of bounds index - void bounds_check() - TRASectionConstIteratorBase"));
			}
		}
		void dereference_bounds_check() const {
			bounds_check(m_index);
		}
		auto operator*() const -> const_reference {
			dereference_bounds_check();
			return m_ra_iterator[m_index];
		}
		auto operator->() const -> typename std::add_pointer<typename std::add_const<value_type>::type>::type {
			dereference_bounds_check();
			return std::addressof(m_ra_iterator[m_index]);
		}
		const_reference operator[](difference_type _Off) const {
			bounds_check(_Off);
			return m_ra_iterator[_Off];
		}
		TRASectionConstIteratorBase& operator +=(difference_type x) {
			m_index += (x);
			return (*this);
		}
		TRASectionConstIteratorBase& operator -=(difference_type x) { operator +=(-x); return (*this); }
		TRASectionConstIteratorBase& operator ++() { operator +=(1); return (*this); }
		TRASectionConstIteratorBase& operator ++(int) { auto _Tmp = *this; operator +=(1); return (_Tmp); }
		TRASectionConstIteratorBase& operator --() { operator -=(1); return (*this); }
		TRASectionConstIteratorBase& operator --(int) { auto _Tmp = *this; operator -=(1); return (_Tmp); }

		TRASectionConstIteratorBase operator+(difference_type n) const { auto retval = (*this); retval += n; return retval; }
		TRASectionConstIteratorBase operator-(difference_type n) const { return ((*this) + (-n)); }
		difference_type operator-(const TRASectionConstIteratorBase& _Right_cref) const {
			if (!(_Right_cref.m_ra_iterator == m_ra_iterator)) { MSE_THROW(msearray_range_error("invalid argument - difference_type operator-() - TRASectionConstIteratorBase")); }
			return m_index - _Right_cref.m_index;
		}
		bool operator ==(const TRASectionConstIteratorBase& _Right_cref) const {
			return ((_Right_cref.m_index == m_index) && (_Right_cref.m_count == m_count) && (_Right_cref.m_ra_iterator == m_ra_iterator));
		}
		bool operator !=(const TRASectionConstIteratorBase& _Right_cref) const { return !((*this) == _Right_cref); }
		bool operator<(const TRASectionConstIteratorBase& _Right_cref) const { return (0 > operator-(_Right_cref)); }
		bool operator>(const TRASectionConstIteratorBase& _Right_cref) const { return (0 > operator-(_Right_cref)); }
		bool operator<=(const TRASectionConstIteratorBase& _Right_cref) const { return (0 >= operator-(_Right_cref)); }
		bool operator>=(const TRASectionConstIteratorBase& _Right_cref) const { return (0 >= operator-(_Right_cref)); }
		TRASectionConstIteratorBase& operator=(const TRASectionConstIteratorBase& _Right_cref) {
			if (!(_Right_cref.m_ra_iterator == m_ra_iterator)) { MSE_THROW(msearray_range_error("invalid argument - TRASectionConstIteratorBase& operator=() - TRASectionConstIteratorBase")); }
			m_index = _Right_cref.m_index;
			return (*this);
		}
	};

	template <typename _TRAIterator>
	class TXScopeRASectionConstIterator : public TRASectionConstIteratorBase<_TRAIterator>, public XScopeTagBase {
	public:
		typedef TRASectionConstIteratorBase<_TRAIterator> base_class;
		typedef typename base_class::value_type value_type;
		typedef typename base_class::const_reference const_reference;
		typedef typename base_class::size_type size_type;
		typedef typename base_class::difference_type difference_type;

		TXScopeRASectionConstIterator(const TRASectionConstIteratorBase<_TRAIterator>& src)
			: base_class(src) {}
		TXScopeRASectionConstIterator(_TRAIterator ra_iterator, size_type count, size_type index = 0)
			: base_class(ra_iterator, count, index) {}

		TXScopeRASectionConstIterator& operator +=(difference_type x) {
			base_class::operator +=(x);
			return (*this);
		}
		TXScopeRASectionConstIterator& operator -=(difference_type x) { operator +=(-x); return (*this); }
		TXScopeRASectionConstIterator& operator ++() { operator +=(1); return (*this); }
		TXScopeRASectionConstIterator& operator ++(int) { auto _Tmp = *this; operator +=(1); return (_Tmp); }
		TXScopeRASectionConstIterator& operator --() { operator -=(1); return (*this); }
		TXScopeRASectionConstIterator& operator --(int) { auto _Tmp = *this; operator -=(1); return (_Tmp); }

		TXScopeRASectionConstIterator operator+(difference_type n) const { auto retval = (*this); retval += n; return retval; }
		TXScopeRASectionConstIterator operator-(difference_type n) const { return ((*this) + (-n)); }
		difference_type operator-(const TRASectionConstIteratorBase<_TRAIterator>& _Right_cref) const {
			return base_class::operator-(_Right_cref);
		}

		TXScopeRASectionConstIterator& operator=(const TRASectionConstIteratorBase<_TRAIterator>& _Right_cref) {
			base_class::operator=(_Right_cref);
			return (*this);
		}

		void xscope_tag() const {}
		void xscope_iterator_tag() const {}
		void not_async_shareable_tag() const {} /* Indication that this type is not eligible to be shared between threads. */
	private:
		void* operator new(size_t size) { return ::operator new(size); }

		TXScopeRASectionConstIterator* operator&() { return this; }
		const TXScopeRASectionConstIterator* operator&() const { return this; }
	};

	template <typename _TRAIterator>
	class TRASectionConstIterator : public TRASectionConstIteratorBase<_TRAIterator> {
	public:
		typedef TRASectionConstIteratorBase<_TRAIterator> base_class;
		typedef typename base_class::value_type value_type;
		typedef typename base_class::const_reference const_reference;
		typedef typename base_class::size_type size_type;
		typedef typename base_class::difference_type difference_type;

		TRASectionConstIterator(const TRASectionConstIterator& src) : base_class(src) {}
		template <typename _TRAIterator1>
		TRASectionConstIterator(_TRAIterator1 ra_iterator, size_type count, size_type index = 0)
			: base_class(ra_iterator, count, index) {}
		virtual ~TRASectionConstIterator() {
			mse::T_valid_if_not_an_xscope_type<_TRAIterator>();
		}

		TRASectionConstIterator& operator +=(difference_type x) {
			base_class::operator +=(x);
			return (*this);
		}
		TRASectionConstIterator& operator -=(difference_type x) { operator +=(-x); return (*this); }
		TRASectionConstIterator& operator ++() { operator +=(1); return (*this); }
		TRASectionConstIterator& operator ++(int) { auto _Tmp = *this; operator +=(1); return (_Tmp); }
		TRASectionConstIterator& operator --() { operator -=(1); return (*this); }
		TRASectionConstIterator& operator --(int) { auto _Tmp = *this; operator -=(1); return (_Tmp); }

		TRASectionConstIterator operator+(difference_type n) const { auto retval = (*this); retval += n; return retval; }
		TRASectionConstIterator operator-(difference_type n) const { return ((*this) + (-n)); }
		difference_type operator-(const TRASectionConstIteratorBase<_TRAIterator>& _Right_cref) const {
			return base_class::operator-(_Right_cref);
		}

		TRASectionConstIterator& operator=(const TRASectionConstIteratorBase<_TRAIterator>& _Right_cref) {
			base_class::operator=(_Right_cref);
			return (*this);
		}
	};


	template <typename _TRAIterator>
	class TRandomAccessConstSectionBase;
	template <typename _TRAIterator>
	class TXScopeRandomAccessSection;
	template <typename _TRAIterator>
	class TXScopeRandomAccessConstSection;
	template <typename _TRAIterator>
	class TRandomAccessSection;
	template <typename _TRAIterator>
	class TRandomAccessConstSection;

	class RandomAccessSectionTag {};
	class RandomAccessConstSectionTag {};

	template<class T, class EqualTo>
	struct HasOrInheritsBeginMethod_msemsearray_impl
	{
		template<class U, class V>
		static auto test(U*) -> decltype(std::declval<U>().begin() == std::declval<V>().begin(), bool(true));
		template<typename, typename>
		static auto test(...)->std::false_type;

		using type = typename std::is_same<bool, decltype(test<T, EqualTo>(0))>::type;
	};
	template<class T, class EqualTo = T>
	struct HasOrInheritsBeginMethod_msemsearray : HasOrInheritsBeginMethod_msemsearray_impl<
		typename std::remove_reference<T>::type, typename std::remove_reference<EqualTo>::type>::type {};

	template<class T, class EqualTo>
	struct HasOrInheritsStaticSSBeginMethod_msemsearray_impl
	{
		template<class U, class V>
		static auto test(U*) -> decltype(U::ss_begin(std::declval<U*>()) == V::ss_begin(std::declval<V*>()), bool(true));
		template<typename, typename>
		static auto test(...)->std::false_type;

		using type = typename std::is_same<bool, decltype(test<T, EqualTo>(0))>::type;
	};
	template<class T, class EqualTo = T>
	struct HasOrInheritsStaticSSBeginMethod_msemsearray : HasOrInheritsStaticSSBeginMethod_msemsearray_impl<
		typename std::remove_reference<T>::type, typename std::remove_reference<EqualTo>::type>::type {};

	template<typename T, size_t n>
	size_t native_array_size_msemsearray(const T(&)[n]) {
		return n;
	}
	template<class T, class EqualTo>
	struct IsNativeArray_msemsearray_impl
	{
		template<class U, class V>
		static auto test(U*) -> decltype(native_array_size_msemsearray(std::declval<U>()) == native_array_size_msemsearray(std::declval<V>()), bool(true));
		template<typename, typename>
		static auto test(...)->std::false_type;

		using type = typename std::is_same<bool, decltype(test<T, EqualTo>(0))>::type;
	};
	template<class T, class EqualTo = T>
	struct IsNativeArray_msemsearray : IsNativeArray_msemsearray_impl<
		typename std::remove_reference<T>::type, typename std::remove_reference<EqualTo>::type>::type {};

	template<class T, class EqualTo>
	struct IsDereferenceable_msemsearray_impl
	{
		template<class U, class V>
		static auto test(U*) -> decltype((*std::declval<U>()) == (*std::declval<V>()), bool(true));
		template<typename, typename>
		static auto test(...)->std::false_type;

		using type = typename std::is_same<bool, decltype(test<T, EqualTo>(0))>::type;
	};
	template<class T, class EqualTo = T>
	struct IsDereferenceable_msemsearray : IsDereferenceable_msemsearray_impl<
		typename std::remove_reference<T>::type, typename std::remove_reference<EqualTo>::type>::type {};

	template<typename _Ty, class = typename std::enable_if<(IsDereferenceable_msemsearray<_Ty>::value), void>::type>
	void T_valid_if_is_dereferenceable() {}

	template <typename _TRAIterator>
	class TRandomAccessSectionBase : public RandomAccessSectionTag
		, public std::conditional<std::is_base_of<ContainsNonOwningScopeReferenceTagBase, _TRAIterator>::value, ContainsNonOwningScopeReferenceTagBase, TPlaceHolder_msescope<TRandomAccessSectionBase<_TRAIterator> > >::type
	{
	public:
		typedef typename std::remove_reference<decltype(std::declval<_TRAIterator>()[0])>::type value_type;
		typedef decltype(std::declval<_TRAIterator>()[0]) reference;
		typedef typename std::add_lvalue_reference<typename std::add_const<value_type>::type>::type const_reference;
		typedef typename mse::us::msearray<value_type, 0>::size_type size_type;
		typedef decltype(std::declval<_TRAIterator>() - std::declval<_TRAIterator>()) difference_type;
		static const size_type npos = size_type(-1);
		typedef _TRAIterator ra_iterator_type;

		//TRandomAccessSectionBase(const TRandomAccessSectionBase& src) = default;
		TRandomAccessSectionBase(const TRandomAccessSectionBase& src) : m_start_iter(src.m_start_iter), m_count(src.m_count) {}
		TRandomAccessSectionBase(const _TRAIterator& start_iter, size_type count) : m_start_iter(start_iter), m_count(count) {}
		template <typename _TRALoneParam>
		TRandomAccessSectionBase(const _TRALoneParam& param)
			: m_start_iter(s_iter_from_lone_param(param)), m_count(s_count_from_lone_param(param)) {}
		/* The presence of this constructor for native arrays should not be construed as condoning the use of native arrays. */
		template<size_t Tn>
		TRandomAccessSectionBase(value_type(&native_array)[Tn]) : m_start_iter(native_array), m_count(Tn) {}

		reference operator[](size_type _P) const {
			if (m_count <= _P) { MSE_THROW(msearray_range_error("out of bounds index - reference operator[](size_type _P) - TRandomAccessSectionBase")); }
			return m_start_iter[difference_type(_P)];
		}
		reference at(size_type _P) const {
			return (*this)[_P];
		}
		reference front() const {
			if (0 == (*this).size()) { MSE_THROW(msearray_range_error("front() on empty - reference front() const - TRandomAccessSectionBase")); }
			return (*this)[0];
		}
		reference back() const {
			if (0 == (*this).size()) { MSE_THROW(msearray_range_error("back() on empty - reference back() const - TRandomAccessSectionBase")); }
			return (*this)[(*this).size() - 1];
		}
		size_type size() const _NOEXCEPT {
			return m_count;
		}
		size_type length() const _NOEXCEPT {
			return (*this).size();
		}
		size_type max_size() const _NOEXCEPT {	// return maximum possible length of sequence
			return static_cast<size_type>((std::numeric_limits<difference_type>::max)());
		}
		bool empty() const _NOEXCEPT {
			return (0 == (*this).size());
		}

		bool equal(const TRandomAccessConstSectionBase<_TRAIterator>& sv) const _NOEXCEPT {
			if (size() != sv.size()) {
				return false;
			}
			//return std::equal(xscope_cbegin(), xscope_cend(), sv.xscope_cbegin());
			auto first1 = xscope_cbegin();
			auto last1 = xscope_cend();
			auto first2 = sv.xscope_cbegin();
			while (first1 != last1) {
				if (!(*first1 == *first2)) {
					return false;
				}
				++first1; ++first2;
			}
			return true;
		}
		bool equal(size_type pos1, size_type n1, TRandomAccessConstSectionBase<_TRAIterator> sv) const {
			return subsection(pos1, n1).equal(sv);
		}
		bool equal(size_type pos1, size_type n1, TRandomAccessConstSectionBase<_TRAIterator> sv, size_type pos2, size_type n2) const {
			return subsection(pos1, n1).equal(sv.subsection(pos2, n2));
		}
		template <typename _TRAIterator2>
		bool equal(size_type pos1, size_type n1, const _TRAIterator2& s, size_type n2) const {
			return subsection(pos1, n1).equal(TRandomAccessConstSectionBase<_TRAIterator>(s, n2));
		}
		bool operator==(const TRandomAccessConstSectionBase<_TRAIterator>& sv) const {
			return equal(sv);
		}
		bool operator!=(const TRandomAccessConstSectionBase<_TRAIterator>& sv) const {
			return !((*this) == sv);
		}

		bool lexicographical_compare(const TRandomAccessConstSectionBase<_TRAIterator>& sv) const _NOEXCEPT {
			return std::lexicographical_compare(xscope_cbegin(), xscope_cend(), sv.xscope_cbegin(), sv.xscope_cend());
		}
		bool lexicographical_compare(size_type pos1, size_type n1, TRandomAccessConstSectionBase<_TRAIterator> sv) const {
			return subsection(pos1, n1).lexicographical_compare(sv);
		}
		bool lexicographical_compare(size_type pos1, size_type n1, TRandomAccessConstSectionBase<_TRAIterator> sv, size_type pos2, size_type n2) const {
			return subsection(pos1, n1).lexicographical_compare(sv.subsection(pos2, n2));
		}
		template <typename _TRAIterator2>
		bool lexicographical_compare(size_type pos1, size_type n1, const _TRAIterator2& s, size_type n2) const {
			return subsection(pos1, n1).lexicographical_compare(TRandomAccessConstSectionBase<_TRAIterator>(s, n2));
		}
		bool operator<(const TRandomAccessConstSectionBase<_TRAIterator>& sv) const {
			return lexicographical_compare(sv);
		}
		bool operator>(const TRandomAccessConstSectionBase<_TRAIterator>& sv) const {
			return sv.lexicographical_compare(*this);
		}
		bool operator<=(const TRandomAccessConstSectionBase<_TRAIterator>& sv) const { return !((*this) > sv); }
		bool operator>=(const TRandomAccessConstSectionBase<_TRAIterator>& sv) const { return !((*this) < sv); }

		int compare(const TRandomAccessConstSectionBase<_TRAIterator>& sv) const _NOEXCEPT {
			size_type rlen = std::min(size(), sv.size());

			int retval = 0;
			auto _First1 = (*this).xscope_cbegin();
			auto _First2 = sv.xscope_cbegin();
			for (; 0 < rlen; --rlen, ++_First1, ++_First2)
				if (!((*_First1) == (*_First2)))
					return (((*_First1) < (*_First2)) ? -1 : +1);

			if (retval == 0) // first rlen chars matched
				retval = size() == sv.size() ? 0 : (size() < sv.size() ? -1 : 1);
			return retval;
		}
		int compare(size_type pos1, size_type n1, TRandomAccessConstSectionBase<_TRAIterator> sv) const {
			return subsection(pos1, n1).compare(sv);
		}
		int compare(size_type pos1, size_type n1, TRandomAccessConstSectionBase<_TRAIterator> sv, size_type pos2, size_type n2) const {
			return subsection(pos1, n1).compare(sv.subsection(pos2, n2));
		}
		template <typename _TRAIterator2>
		int compare(size_type pos1, size_type n1, const _TRAIterator2& s, size_type n2) const {
			return subsection(pos1, n1).compare(TRandomAccessConstSectionBase<_TRAIterator>(s, n2));
		}

		template <typename _TRAIterator2>
		size_type copy(_TRAIterator2 target_iter, size_type n, size_type pos = 0) const {
			if (pos + n > (*this).size()) {
				if (pos >= (*this).size()) {
					return 0;
				}
				else {
					n = (*this).size() - pos;
				}
			}
			for (size_type i = 0; i < n; i += 1) {
				(*target_iter) = (*this)[i];
				++target_iter;
			}
			return n;
		}

		void remove_prefix(size_type n) /*_NOEXCEPT*/ {
			if (n > (*this).size()) { MSE_THROW(msearray_range_error("out of bounds index - void remove_prefix() - TRandomAccessSectionBase")); }
			m_count -= n;
			m_start_iter += n;
		}
		void remove_suffix(size_type n) /*_NOEXCEPT*/ {
			if (n > (*this).size()) { MSE_THROW(msearray_range_error("out of bounds index - void remove_suffix() - TRandomAccessSectionBase")); }
			m_count -= n;
		}

		template<typename _Ty2, class = typename std::enable_if<std::is_base_of<TRandomAccessSectionBase, _Ty2>::value && HasOrInheritsAssignmentOperator_msemsearray<_TRAIterator>::value, void>::type>
		void swap(_Ty2& _Other) _NOEXCEPT_OP(_NOEXCEPT_OP(TRandomAccessSectionBase(_Other)) && _NOEXCEPT_OP(std::declval<_TRAIterator>().operator=(std::declval<_TRAIterator>()))) {
			TRandomAccessSectionBase& _Other2 = _Other;
			std::swap((*this), _Other2);
		}

		size_type find(const TRandomAccessConstSectionBase<_TRAIterator>& s, size_type pos = 0) const _NOEXCEPT {
			if ((1 > s.size()) || (1 > (*this).size())) {
				return npos;
			}
			auto cit = std::search((*this).xscope_cbegin(), (*this).xscope_cend(), s.xscope_cbegin(), s.xscope_cend());
			if ((*this).xscope_cend() == cit) {
				return npos;
			}
			return (cit - (*this).xscope_cbegin());
		}
		size_type find(const value_type& c, size_type pos = 0) const _NOEXCEPT {
			if ((*this).size() <= 1) {
				return npos;
			}
			auto cit1 = std::find(xscope_cbegin(), xscope_cend(), c);
			return (xscope_cend() == cit1) ? npos : (cit1 - xscope_cbegin());
		}
		size_type rfind(const TRandomAccessConstSectionBase<_TRAIterator>& s, size_type pos = npos) const _NOEXCEPT {
			if ((1 > s.size()) || (1 > (*this).size())) {
				return npos;
			}
			auto cit = std::find_end((*this).xscope_cbegin(), (*this).xscope_cend(), s.xscope_cbegin(), s.xscope_cend());
			if ((*this).xscope_cend() == cit) {
				return npos;
			}
			return (cit - (*this).xscope_cbegin());
		}
		size_type rfind(const value_type& c, size_type pos = npos) const _NOEXCEPT {
			if ((*this).size() <= 1) {
				return npos;
			}
			if (pos < (*this).size()) {
				++pos;
			}
			else {
				pos = (*this).size();
			}
			for (size_type i = pos; 0 != i;) {
				--i;
				if ((*this)[i] == c) {
					return i;
				}
			}
			return npos;
		}
		size_type find_first_of(const TRandomAccessConstSectionBase<_TRAIterator>& s, size_type pos = 0) const _NOEXCEPT {
			if ((1 > s.size()) || (1 > (*this).size())) {
				return npos;
			}
			auto cit = std::find_first_of((*this).xscope_cbegin(), (*this).xscope_cend(), s.xscope_cbegin(), s.xscope_cend());
			if ((*this).xscope_cend() == cit) {
				return npos;
			}
			return (cit - (*this).xscope_cbegin());
		}
		size_type find_first_of(const value_type& c, size_type pos = 0) const _NOEXCEPT {
			return find(c, pos);
		}
		size_type find_last_of(const TRandomAccessConstSectionBase<_TRAIterator>& s, size_type pos = npos) const _NOEXCEPT {
			if ((1 > s.size()) || (1 > (*this).size())) {
				return npos;
			}
			if (pos < (*this).size()) {
				++pos;
			}
			else {
				pos = (*this).size();
			}
			for (size_type i = pos; 0 != i;) {
				--i;
				const auto& domain_element_cref((*this)[i]);
				for (const auto& search_element_cref : s) {
					if (domain_element_cref == search_element_cref) {
						return i;
					}
				}
			}
			return npos;
		}
		size_type find_last_of(const value_type& c, size_type pos = npos) const _NOEXCEPT {
			return rfind(c, pos);
		}
		size_type find_first_not_of(const TRandomAccessConstSectionBase<_TRAIterator>& s, size_type pos = 0) const _NOEXCEPT {
			if ((1 > s.size()) || (1 > (*this).size())) {
				return npos;
			}
			auto cit = std::find_if_not((*this).xscope_cbegin(), (*this).xscope_cend(), [&s](const value_type& domain_element_cref) {
				for (const auto& search_element_cref : s) {
					if (domain_element_cref == search_element_cref) {
						return true;
					}
				}
				return false;
			}
			);
			if ((*this).xscope_cend() == cit) {
				return npos;
			}
			return (cit - (*this).xscope_cbegin());
		}
		size_type find_first_not_of(const value_type& c, size_type pos = 0) const _NOEXCEPT {
			if (1 > (*this).size()) {
				return npos;
			}
			auto cit = std::find_if_not((*this).xscope_cbegin(), (*this).xscope_cend(), [&c](const value_type& domain_element_cref) {
				if (domain_element_cref == c) {
					return true;
				}
				return false;
			}
			);
			if ((*this).xscope_cend() == cit) {
				return npos;
			}
			return (cit - (*this).xscope_cbegin());
		}
		size_type find_last_not_of(const TRandomAccessConstSectionBase<_TRAIterator>& s, size_type pos = npos) const _NOEXCEPT {
			if ((1 > s.size()) || (1 > (*this).size())) {
				return npos;
			}
			if (pos < (*this).size()) {
				++pos;
			}
			else {
				pos = (*this).size();
			}
			for (size_type i = pos; 0 != i;) {
				--i;
				const auto& domain_element_cref((*this)[i]);
				for (const auto& search_element_cref : s) {
					if (domain_element_cref != search_element_cref) {
						return i;
					}
				}
			}
			return npos;
		}
		size_type find_last_not_of(const value_type& c, size_type pos = npos) const _NOEXCEPT {
			if (1 > (*this).size()) {
				return npos;
			}
			if (pos < (*this).size()) {
				++pos;
			}
			else {
				pos = (*this).size();
			}
			for (size_type i = pos; 0 != i;) {
				--i;
				const auto& domain_element_cref((*this)[i]);
				if (domain_element_cref != c) {
					return i;
				}
			}
			return npos;
		}

		bool starts_with(const TRandomAccessConstSectionBase<_TRAIterator>& s) const _NOEXCEPT {
			return (size() >= s.size()) && equal(0, s.size(), s);
		}
		bool starts_with(const value_type& c) const _NOEXCEPT {
			return (!empty()) && (front() == c);
		}
		bool ends_with(const TRandomAccessConstSectionBase<_TRAIterator>& s) const _NOEXCEPT {
			return (size() >= s.size()) && equal(size() - s.size(), npos, s);
		}
		bool ends_with(const value_type& c) const _NOEXCEPT {
			return (!empty()) && (back() == c);
		}

		//typedef TXScopeRASectionIterator<_TRAIterator> xscope_iterator;
		//typedef TXScopeRASectionConstIterator<_TRAIterator> xscope_const_iterator;
		class xscope_iterator : public TXScopeRASectionIterator<_TRAIterator> {
		public:
			typedef TXScopeRASectionIterator<_TRAIterator> base_class;

			MSE_USING(xscope_iterator, base_class);
			template<class _TRASectionPointer>
			xscope_iterator(const _TRASectionPointer& ptr) : base_class((*ptr).m_start_iter, (*ptr).m_count) {}
		};
		class xscope_const_iterator : public TXScopeRASectionConstIterator<_TRAIterator> {
		public:
			typedef TXScopeRASectionConstIterator<_TRAIterator> base_class;

			MSE_USING(xscope_const_iterator, base_class);
			template<class _TRASectionPointer>
			xscope_const_iterator(const _TRASectionPointer& ptr) : base_class((*ptr).m_start_iter, (*ptr).m_count) {}
		};
		xscope_iterator xscope_begin() const { return xscope_iterator((*this).m_start_iter, (*this).m_count); }
		xscope_const_iterator xscope_cbegin() const { return xscope_const_iterator((*this).m_start_iter, (*this).m_count); }
		xscope_iterator xscope_end() const {
			auto retval(xscope_iterator((*this).m_start_iter, (*this).m_count));
			retval += (*this).m_count;
			return retval;
		}
		xscope_const_iterator xscope_cend() const {
			auto retval(xscope_const_iterator((*this).m_start_iter, (*this).m_count));
			retval += (*this).m_count;
			return retval;
		}

		template <typename _TRALoneParam>
		static auto s_iter_from_lone_param(const _TRALoneParam& param) {
			/* _TRALoneParam being either another TRandomAccessSectionBase<>, a "random access" container, or a pointer to "random
			access" container is supported. Different initialization implementations are required for each case. */
			return s_iter_from_lone_param1(typename std::is_base_of<RandomAccessSectionTag, _TRALoneParam>::type(), param);
		}
		template <typename _TRALoneParam>
		static auto s_count_from_lone_param(const _TRALoneParam& param) {
			return s_count_from_lone_param1(typename std::is_base_of<RandomAccessSectionTag, _TRALoneParam>::type(), param);
		}

	protected:
		TRandomAccessSectionBase subsection(size_type pos = 0, size_type n = npos) const {
			return pos > (*this).size()
				? (MSE_THROW(msearray_range_error("out of bounds index - TRandomAccessSectionBase subsection() const - TRandomAccessSectionBase")))
				: TRandomAccessSectionBase((*this).m_start_iter + pos, std::min(n, (*this).size() - pos));
		}

		typedef TRASectionIterator<_TRAIterator> iterator;
		typedef TRASectionConstIterator<_TRAIterator> const_iterator;
		iterator begin() { return iterator((*this).m_start_iter, (*this).m_count); }
		const_iterator begin() const { return cbegin(); }
		const_iterator cbegin() const { return const_iterator((*this).m_start_iter, (*this).m_count); }
		iterator end() {
			auto retval(iterator((*this).m_start_iter, (*this).m_count));
			retval += (*this).m_count;
			return retval;
		}
		const_iterator end() const { return cend(); }
		const_iterator cend() const {
			auto retval(const_iterator((*this).m_start_iter, (*this).m_count));
			retval += (*this).m_count;
			return retval;
		}

	private:
		/* construction helper functions */
		template <typename _TRAPointer>
		static auto s_iter_from_ptr_helper2(std::true_type, const _TRAPointer& ptr) {
			/* ptr seems to be an xscope pointer.*/
			return mse::make_xscope_random_access_iterator(ptr, 0);
		}
		template <typename _TRAPointer>
		static auto s_iter_from_ptr_helper2(std::false_type, const _TRAPointer& ptr) {
			return mse::make_random_access_iterator(ptr, 0);
		}
		template <typename _TRALoneParam>
		static auto s_iter_from_lone_param2(std::true_type, const _TRALoneParam& ra_container) {
			/* The parameter seems to be a container with a "begin()" member function. So we'll use that function to obtain the
			iterator we need. */
			return ra_container.begin();
		}
		template <typename _TRALoneParam>
		static auto s_iter_from_lone_param2(std::false_type, const _TRALoneParam& ptr) {
			/* The parameter doesn't seem to be a container with a "begin()" member function. Here we'll assume that it is a pointer to
			a supported container. If you get a compile error here, then construction from the given parameter type isn't supported. */
			mse::T_valid_if_is_dereferenceable<_TRALoneParam>();
			return s_iter_from_ptr_helper2(typename std::is_base_of<mse::XScopeTagBase, _TRALoneParam>::type(), ptr);
		}
		template <typename _TRALoneParam>
		static auto s_iter_from_lone_param1(std::true_type, const _TRALoneParam& ra_section) {
			/* The parameter is another "random access section". */
			return ra_section.m_start_iter;
		}
		template <typename _TRALoneParam>
		static auto s_iter_from_lone_param1(std::false_type, const _TRALoneParam& param) {
			/* The parameter is not a "random access section". */
			return s_iter_from_lone_param2(typename HasOrInheritsBeginMethod_msemsearray<_TRALoneParam>::type(), param);
		}
		template <typename _TRALoneParam>
		static auto s_count_from_lone_param3(std::true_type, const _TRALoneParam& native_array) {
			/* The parameter seems to be a native array. */
			return mse::native_array_size_msemsearray(native_array);
		}
		template <typename _TRALoneParam>
		static auto s_count_from_lone_param3(std::false_type, const _TRALoneParam& ptr) {
			/* The parameter doesn't seem to be a container with a "begin()" member function or a native array. Here we'll
			assume that it is a pointer to a container with a size() member function. If you get a compile error here, then
			construction from the given parameter type isn't supported. In particular, "char *" pointers to null terminated
			strings are not supported as a lone parameter. */
			return (*ptr).size();
		}
		template <typename _TRALoneParam>
		static auto s_count_from_lone_param2(std::true_type, const _TRALoneParam& ra_container) {
			/* The parameter seems to be a container with a "begin()" member function. We'll assume it has a "size()" member function too. */
			return ra_container.size();
		}
		template <typename _TRALoneParam>
		static auto s_count_from_lone_param2(std::false_type, const _TRALoneParam& param) {
			/* The parameter doesn't seem to be a container with a "begin()" member function. */
			return s_count_from_lone_param3(typename mse::IsNativeArray_msemsearray<_TRALoneParam>::type(), param);
		}
		template <typename _TRALoneParam>
		static auto s_count_from_lone_param1(std::true_type, const _TRALoneParam& ra_section) {
			/* The parameter is another "random access section". */
			return ra_section.m_count;
		}
		template <typename _TRALoneParam>
		static auto s_count_from_lone_param1(std::false_type, const _TRALoneParam& param) {
			/* The parameter is not a "random access section". */
			return s_count_from_lone_param2(typename HasOrInheritsBeginMethod_msemsearray<_TRALoneParam>::type(), param);
		}

		TRandomAccessSectionBase<_TRAIterator>* operator&() { return this; }
		const TRandomAccessSectionBase<_TRAIterator>* operator&() const { return this; }

		_TRAIterator m_start_iter;
		size_type m_count = 0;

		friend class TXScopeRandomAccessSection<_TRAIterator>;
		friend class TRandomAccessSection<_TRAIterator>;
		template<typename _TRAIterator1> friend class TXScopeTRandomAccessConstSectionBase;
		template<typename _TRAIterator1> friend class TXScopeTRandomAccessSectionBase;
		template<typename _TRAIterator1> friend class TRandomAccessConstSectionBase;
		template<typename _TRAIterator1> friend class TRandomAccessSectionBase;
	};

	template <typename _TRAIterator>
	class TXScopeRandomAccessSection : public TRandomAccessSectionBase<_TRAIterator>, public XScopeTagBase {
	public:
		typedef TRandomAccessSectionBase<_TRAIterator> base_class;
		typedef typename base_class::value_type value_type;
		typedef typename base_class::reference reference;
		typedef typename base_class::const_reference const_reference;
		typedef typename base_class::size_type size_type;
		typedef typename base_class::difference_type difference_type;
		static const size_type npos = size_type(-1);

		//TXScopeRandomAccessSection(const _TRAIterator& start_iter, size_type count) : base_class(start_iter,count) {}
		//TXScopeRandomAccessSection(const TXScopeRandomAccessSection& src) = default;
		//template<class _Ty2 = _TRAIterator, class = typename std::enable_if<(std::is_same<_Ty2, _TRAIterator>::value) && (!std::is_base_of<XScopeTagBase, _TRAIterator>::value), void>::type>
		//TXScopeRandomAccessSection(const TRandomAccessSection<_TRAIterator>& src) : base_class(src) {}
		//TXScopeRandomAccessSection(const base_class& src) : base_class(src) {}

		MSE_USING(TXScopeRandomAccessSection, base_class);

		TXScopeRandomAccessSection xscope_subsection(size_type pos = 0, size_type n = npos) const {
			return pos > (*this).size()
				? (MSE_THROW(msearray_range_error("out of bounds index - TXScopeRandomAccessSection xscope_subsection() const - TXScopeRandomAccessSection")))
				: TXScopeRandomAccessSection((*this).m_start_iter + pos, std::min(n, (*this).size() - pos));
		}
		TRandomAccessSection<_TRAIterator> subsection(size_type pos = 0, size_type n = npos) const {
			return pos > (*this).size()
				? (MSE_THROW(msearray_range_error("out of bounds index - TRandomAccessSection<_TRAIterator> subsection() const - TXScopeRandomAccessSection")))
				: TRandomAccessSection<_TRAIterator>((*this).m_start_iter + pos, std::min(n, (*this).size() - pos));
		}

		typedef typename base_class::xscope_iterator xscope_iterator;
		typedef typename base_class::xscope_const_iterator xscope_const_iterator;

		/* These are here because some standard algorithms require them. Prefer the "xscope_" prefixed versions to
		acknowledge that scope iterators are returned. */
		auto begin() { return (*this).xscope_begin(); }
		auto begin() const { return cbegin(); }
		auto cbegin() const { return (*this).xscope_cbegin(); }
		auto end() { return (*this).xscope_end(); }
		auto end() const { return cend(); }
		auto cend() const { return (*this).xscope_cend(); }

	private:
		TXScopeRandomAccessSection<_TRAIterator>& operator=(const TXScopeRandomAccessSection<_TRAIterator>& _Right_cref) = delete;
		void* operator new(size_t size) { return ::operator new(size); }

		TXScopeRandomAccessSection<_TRAIterator>* operator&() { return this; }
		const TXScopeRandomAccessSection<_TRAIterator>* operator&() const { return this; }
	};

	template <typename _TRAIterator>
	auto make_xscope_random_access_section(const _TRAIterator& start_iter, typename TXScopeRandomAccessSection<_TRAIterator>::size_type count) {
		return TXScopeRandomAccessSection<_TRAIterator>(start_iter, count);
	}
	template <typename _TRALoneParam>
	auto make_xscope_random_access_section(const _TRALoneParam& param) {
		typedef typename std::remove_reference<decltype(mse::TRandomAccessSectionBase<char *>::s_iter_from_lone_param(param))>::type _TRAIterator;
		return TXScopeRandomAccessSection<_TRAIterator>(param);
	}

	template <typename _TRAIterator>
	class TRandomAccessSection : public TRandomAccessSectionBase<_TRAIterator> {
	public:
		typedef TRandomAccessSectionBase<_TRAIterator> base_class;
		typedef typename base_class::value_type value_type;
		typedef typename base_class::reference reference;
		typedef typename base_class::const_reference const_reference;
		typedef typename base_class::size_type size_type;
		typedef typename base_class::difference_type difference_type;
		static const size_type npos = size_type(-1);

		TRandomAccessSection(const TRandomAccessSection& src) : base_class(static_cast<const base_class&>(src)) {}
		TRandomAccessSection(const _TRAIterator& start_iter, size_type count) : base_class(start_iter, count) {}
		template <typename _TRALoneParam>
		TRandomAccessSection(const _TRALoneParam& param) : base_class(param) {}
		/* The presence of this constructor for native arrays should not be construed as condoning the use of native arrays. */
		template<size_t Tn>
		TRandomAccessSection(value_type(&native_array)[Tn]) : base_class(native_array) {}
		virtual ~TRandomAccessSection() {
			mse::T_valid_if_not_an_xscope_type<_TRAIterator>();
		}

		TRandomAccessSection subsection(size_type pos = 0, size_type n = npos) const {
			return pos > (*this).size()
				? (MSE_THROW(msearray_range_error("out of bounds index - TRandomAccessSection subsection() const - TRandomAccessSection")))
				: TRandomAccessSection((*this).m_start_iter + pos, std::min(n, (*this).size() - pos));
		}

		typedef typename base_class::iterator iterator;
		typedef typename base_class::const_iterator const_iterator;
		iterator begin() { return base_class::begin(); }
		const_iterator begin() const { return cbegin(); }
		const_iterator cbegin() const { return base_class::cbegin(); }
		iterator end() { return base_class::end(); }
		const_iterator end() const { return cend(); }
		const_iterator cend() const { return base_class::cend(); }
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		reverse_iterator rbegin() {	// return iterator for beginning of reversed mutable sequence
			return (reverse_iterator(end()));
		}
		const_reverse_iterator rbegin() const {	// return iterator for beginning of reversed nonmutable sequence
			return (const_reverse_iterator(end()));
		}
		reverse_iterator rend() {	// return iterator for end of reversed mutable sequence
			return (reverse_iterator(begin()));
		}
		const_reverse_iterator rend() const {	// return iterator for end of reversed nonmutable sequence
			return (const_reverse_iterator(begin()));
		}
		const_reverse_iterator crbegin() const {	// return iterator for beginning of reversed nonmutable sequence
			return (rbegin());
		}
		const_reverse_iterator crend() const {	// return iterator for end of reversed nonmutable sequence
			return (rend());
		}

		friend class TXScopeRandomAccessSection<_TRAIterator>;
		friend class TXScopeRandomAccessConstSection<_TRAIterator>;
		friend class TRandomAccessConstSection<_TRAIterator>;
	};

	template <typename _TRAIterator>
	auto make_random_access_section(const _TRAIterator& start_iter, typename TRandomAccessSection<_TRAIterator>::size_type count) {
		return TRandomAccessSection<_TRAIterator>(start_iter, count);
	}
	template <typename _TRALoneParam>
	auto make_random_access_section(const _TRALoneParam& param) {
		typedef typename std::remove_reference<decltype(mse::TRandomAccessSectionBase<char *>::s_iter_from_lone_param(param))>::type _TRAIterator;
		return TRandomAccessSection<_TRAIterator>(param);
	}

	template <typename _TRAIterator>
	class TRandomAccessConstSectionBase : public RandomAccessConstSectionTag
		, public std::conditional<std::is_base_of<ContainsNonOwningScopeReferenceTagBase, _TRAIterator>::value, ContainsNonOwningScopeReferenceTagBase, TPlaceHolder_msescope<TRandomAccessConstSectionBase<_TRAIterator> > >::type
	{
	public:
		typedef typename std::remove_reference<decltype(std::declval<_TRAIterator>()[0])>::type value_type;
		//typedef decltype(std::declval<_TRAIterator>()[0]) reference;
		typedef typename std::add_lvalue_reference<typename std::add_const<value_type>::type>::type const_reference;
		typedef typename mse::us::msearray<value_type, 0>::size_type size_type;
		typedef decltype(std::declval<_TRAIterator>() - std::declval<_TRAIterator>()) difference_type;
		static const size_type npos = size_type(-1);
		typedef _TRAIterator ra_iterator_type;

		//TRandomAccessConstSectionBase(const TRandomAccessConstSectionBase& src) = default;
		TRandomAccessConstSectionBase(const TRandomAccessConstSectionBase& src) : m_start_iter(src.m_start_iter), m_count(src.m_count) {}
		TRandomAccessConstSectionBase(const TRandomAccessSectionBase<_TRAIterator>& src) : m_start_iter(src.m_start_iter), m_count(src.m_count) {}
		TRandomAccessConstSectionBase(const _TRAIterator& start_iter, size_type count) : m_start_iter(start_iter), m_count(count) {}
		template <typename _TRALoneParam>
		TRandomAccessConstSectionBase(const _TRALoneParam& param)
			/* _TRALoneParam being either another TRandomAccess(Const)SectionBase<> or a pointer to "random access" container is
			supported. Different initialization implementations are required for each of the two cases. */
			: m_start_iter(s_iter_from_lone_param(param))
			, m_count(s_count_from_lone_param(param)) {}

		const_reference operator[](size_type _P) const {
			if (m_count <= _P) { MSE_THROW(msearray_range_error("out of bounds index - reference operator[](size_type _P) - TRandomAccessConstSectionBase")); }
			return m_start_iter[difference_type(_P)];
		}
		const_reference at(size_type _P) const {
			return (*this)[_P];
		}
		const_reference front() const {
			if (0 == (*this).size()) { MSE_THROW(msearray_range_error("front() on empty - const_reference front() const - TRandomAccessConstSectionBase")); }
			return (*this)[0];
		}
		const_reference back() const {
			if (0 == (*this).size()) { MSE_THROW(msearray_range_error("back() on empty - const_reference back() const - TRandomAccessConstSectionBase")); }
			return (*this)[(*this).size() - 1];
		}
		size_type size() const _NOEXCEPT {
			return m_count;
		}
		size_type length() const _NOEXCEPT {
			return (*this).size();
		}
		size_type max_size() const _NOEXCEPT {	// return maximum possible length of sequence
			return static_cast<size_type>((std::numeric_limits<difference_type>::max)());
		}
		bool empty() const _NOEXCEPT {
			return (0 == (*this).size());
		}

		bool equal(const TRandomAccessConstSectionBase<_TRAIterator>& sv) const _NOEXCEPT {
			if (size() != sv.size()) {
				return false;
			}
			//return std::equal(xscope_cbegin(), xscope_cend(), sv.xscope_cbegin());
			auto first1 = xscope_cbegin();
			auto last1 = xscope_cend();
			auto first2 = sv.xscope_cbegin();
			while (first1 != last1) {
				if (!(*first1 == *first2)) {
					return false;
				}
				++first1; ++first2;
			}
			return true;
		}
		bool equal(size_type pos1, size_type n1, TRandomAccessConstSectionBase<_TRAIterator> sv) const {
			return subsection(pos1, n1).equal(sv);
		}
		bool equal(size_type pos1, size_type n1, TRandomAccessConstSectionBase<_TRAIterator> sv, size_type pos2, size_type n2) const {
			return subsection(pos1, n1).equal(sv.subsection(pos2, n2));
		}
		template <typename _TRAIterator2>
		bool equal(size_type pos1, size_type n1, const _TRAIterator2& s, size_type n2) const {
			return subsection(pos1, n1).equal(TRandomAccessConstSectionBase<_TRAIterator>(s, n2));
		}
		bool operator==(const TRandomAccessConstSectionBase& sv) const {
			return equal(sv);
		}
		bool operator!=(const TRandomAccessConstSectionBase& sv) const {
			return !((*this) == sv);
		}

		bool lexicographical_compare(const TRandomAccessConstSectionBase<_TRAIterator>& sv) const _NOEXCEPT {
			return std::lexicographical_compare(xscope_cbegin(), xscope_cend(), sv.xscope_cbegin(), sv.xscope_cend());
		}
		bool lexicographical_compare(size_type pos1, size_type n1, TRandomAccessConstSectionBase<_TRAIterator> sv) const {
			return subsection(pos1, n1).lexicographical_compare(sv);
		}
		bool lexicographical_compare(size_type pos1, size_type n1, TRandomAccessConstSectionBase<_TRAIterator> sv, size_type pos2, size_type n2) const {
			return subsection(pos1, n1).lexicographical_compare(sv.subsection(pos2, n2));
		}
		template <typename _TRAIterator2>
		bool lexicographical_compare(size_type pos1, size_type n1, const _TRAIterator2& s, size_type n2) const {
			return subsection(pos1, n1).lexicographical_compare(TRandomAccessConstSectionBase<_TRAIterator>(s, n2));
		}
		bool operator<(const TRandomAccessConstSectionBase<_TRAIterator>& sv) const {
			return lexicographical_compare(sv);
		}
		bool operator>(const TRandomAccessConstSectionBase<_TRAIterator>& sv) const {
			return sv.lexicographical_compare(*this);
		}
		bool operator<=(const TRandomAccessConstSectionBase<_TRAIterator>& sv) const { return !((*this) > sv); }
		bool operator>=(const TRandomAccessConstSectionBase<_TRAIterator>& sv) const { return !((*this) < sv); }

		template <typename _TRAIterator2>
		size_type copy(_TRAIterator2 target_iter, size_type n, size_type pos = 0) const {
			if (pos + n > (*this).size()) {
				if (pos >= (*this).size()) {
					return 0;
				}
				else {
					n = (*this).size() - pos;
				}
			}
			for (size_type i = 0; i < n; i += 1) {
				(*target_iter) = (*this)[i];
				++target_iter;
			}
			return n;
		}

		void remove_prefix(size_type n) /*_NOEXCEPT*/ {
			if (n > (*this).size()) { MSE_THROW(msearray_range_error("out of bounds index - void remove_prefix() - TRandomAccessConstSectionBase")); }
			m_count -= n;
			m_start_iter += n;
		}
		void remove_suffix(size_type n) /*_NOEXCEPT*/ {
			if (n > (*this).size()) { MSE_THROW(msearray_range_error("out of bounds index - void remove_suffix() - TRandomAccessConstSectionBase")); }
			m_count -= n;
		}

		template<typename _Ty2, class = typename std::enable_if<std::is_base_of<TRandomAccessConstSectionBase, _Ty2>::value && HasOrInheritsAssignmentOperator_msemsearray<_TRAIterator>::value, void>::type>
		void swap(_Ty2& _Other) _NOEXCEPT_OP(_NOEXCEPT_OP(TRandomAccessConstSectionBase(_Other)) && _NOEXCEPT_OP(std::declval<_TRAIterator>().operator=(std::declval<_TRAIterator>()))) {
			TRandomAccessConstSectionBase& _Other2 = _Other;
			std::swap((*this), _Other2);
		}

		size_type find(const TRandomAccessConstSectionBase<_TRAIterator>& s, size_type pos = 0) const _NOEXCEPT {
			if ((1 > s.size()) || (1 > (*this).size())) {
				return npos;
			}
			auto cit = std::search((*this).xscope_cbegin(), (*this).xscope_cend(), s.xscope_cbegin(), s.xscope_cend());
			if ((*this).xscope_cend() == cit) {
				return npos;
			}
			return (cit - (*this).xscope_cbegin());
		}
		size_type find(const value_type& c, size_type pos = 0) const _NOEXCEPT {
			if ((*this).size() <= 1) {
				return npos;
			}
			auto cit1 = std::find(xscope_cbegin(), xscope_cend(), c);
			return (xscope_cend() == cit1) ? npos : (cit1 - xscope_cbegin());
		}
		size_type rfind(const TRandomAccessConstSectionBase<_TRAIterator>& s, size_type pos = npos) const _NOEXCEPT {
			if ((1 > s.size()) || (1 > (*this).size())) {
				return npos;
			}
			auto cit = std::find_end((*this).xscope_cbegin(), (*this).xscope_cend(), s.xscope_cbegin(), s.xscope_cend());
			if ((*this).xscope_cend() == cit) {
				return npos;
			}
			return (cit - (*this).xscope_cbegin());
		}
		size_type rfind(const value_type& c, size_type pos = npos) const _NOEXCEPT {
			if ((*this).size() <= 1) {
				return npos;
			}
			if (pos < (*this).size()) {
				++pos;
			}
			else {
				pos = (*this).size();
			}
			for (size_type i = pos; 0 != i;) {
				--i;
				if ((*this)[i] == c) {
					return i;
				}
			}
			return npos;
		}
		size_type find_first_of(const TRandomAccessConstSectionBase<_TRAIterator>& s, size_type pos = 0) const _NOEXCEPT {
			if ((1 > s.size()) || (1 > (*this).size())) {
				return npos;
			}
			auto cit = std::find_first_of((*this).xscope_cbegin(), (*this).xscope_cend(), s.xscope_cbegin(), s.xscope_cend());
			if ((*this).xscope_cend() == cit) {
				return npos;
			}
			return (cit - (*this).xscope_cbegin());
		}
		size_type find_first_of(const value_type& c, size_type pos = 0) const _NOEXCEPT {
			return find(c, pos);
		}
		size_type find_last_of(const TRandomAccessConstSectionBase<_TRAIterator>& s, size_type pos = npos) const _NOEXCEPT {
			if ((1 > s.size()) || (1 > (*this).size())) {
				return npos;
			}
			if (pos < (*this).size()) {
				++pos;
			}
			else {
				pos = (*this).size();
			}
			for (size_type i = pos; 0 != i;) {
				--i;
				const auto& domain_element_cref((*this)[i]);
				for (const auto& search_element_cref : s) {
					if (domain_element_cref == search_element_cref) {
						return i;
					}
				}
			}
			return npos;
		}
		size_type find_last_of(const value_type& c, size_type pos = npos) const _NOEXCEPT {
			return rfind(c, pos);
		}
		size_type find_first_not_of(const TRandomAccessConstSectionBase<_TRAIterator>& s, size_type pos = 0) const _NOEXCEPT {
			if ((1 > s.size()) || (1 > (*this).size())) {
				return npos;
			}
			auto cit = std::find_if_not((*this).xscope_cbegin(), (*this).xscope_cend(), [&s](const value_type& domain_element_cref) {
				for (const auto& search_element_cref : s) {
					if (domain_element_cref == search_element_cref) {
						return true;
					}
				}
				return false;
			}
			);
			if ((*this).xscope_cend() == cit) {
				return npos;
			}
			return (cit - (*this).xscope_cbegin());
		}
		size_type find_first_not_of(const value_type& c, size_type pos = 0) const _NOEXCEPT {
			if (1 > (*this).size()) {
				return npos;
			}
			auto cit = std::find_if_not((*this).xscope_cbegin(), (*this).xscope_cend(), [&c](const value_type& domain_element_cref) {
				if (domain_element_cref == c) {
					return true;
				}
				return false;
			}
			);
			if ((*this).xscope_cend() == cit) {
				return npos;
			}
			return (cit - (*this).xscope_cbegin());
		}
		size_type find_last_not_of(const TRandomAccessConstSectionBase<_TRAIterator>& s, size_type pos = npos) const _NOEXCEPT {
			if ((1 > s.size()) || (1 > (*this).size())) {
				return npos;
			}
			if (pos < (*this).size()) {
				++pos;
			}
			else {
				pos = (*this).size();
			}
			for (size_type i = pos; 0 != i;) {
				--i;
				const auto& domain_element_cref((*this)[i]);
				for (const auto& search_element_cref : s) {
					if (domain_element_cref != search_element_cref) {
						return i;
					}
				}
			}
			return npos;
		}
		size_type find_last_not_of(const value_type& c, size_type pos = npos) const _NOEXCEPT {
			if (1 > (*this).size()) {
				return npos;
			}
			if (pos < (*this).size()) {
				++pos;
			}
			else {
				pos = (*this).size();
			}
			for (size_type i = pos; 0 != i;) {
				--i;
				const auto& domain_element_cref((*this)[i]);
				if (domain_element_cref != c) {
					return i;
				}
			}
			return npos;
		}

		bool starts_with(const TRandomAccessConstSectionBase<_TRAIterator>& s) const _NOEXCEPT {
			return (size() >= s.size()) && equal(0, s.size(), s);
		}
		bool starts_with(const value_type& c) const _NOEXCEPT {
			return (!empty()) && (front() == c);
		}
		bool ends_with(const TRandomAccessConstSectionBase<_TRAIterator>& s) const _NOEXCEPT {
			return (size() >= s.size()) && equal(size() - s.size(), npos, s);
		}
		bool ends_with(const value_type& c) const _NOEXCEPT {
			return (!empty()) && (back() == c);
		}

		//typedef TXScopeRASectionConstIterator<_TRAIterator> xscope_const_iterator;
		class xscope_const_iterator : public TXScopeRASectionConstIterator<_TRAIterator> {
		public:
			typedef TXScopeRASectionConstIterator<_TRAIterator> base_class;

			MSE_USING(xscope_const_iterator, base_class);
			template<class _TRASectionPointer>
			xscope_const_iterator(const _TRASectionPointer& ptr) : base_class((*ptr).m_start_iter, (*ptr).m_count) {}
		};
		xscope_const_iterator xscope_begin() const { return (*this).xscope_cbegin(); }
		xscope_const_iterator xscope_cbegin() const { return xscope_const_iterator((*this).m_start_iter, (*this).m_count); }
		xscope_const_iterator xscope_end() const { return (*this).xscope_cend(); }
		xscope_const_iterator xscope_cend() const {
			auto retval(xscope_const_iterator((*this).m_start_iter, (*this).m_count));
			retval += (*this).m_count;
			return retval;
		}

		template <typename _TRALoneParam>
		static auto s_iter_from_lone_param(const _TRALoneParam& param) {
			/* _TRALoneParam being either another TRandomAccess(Const)SectionBase<> or a pointer to "random access" container is
			supported. Different initialization implementations are required for each of the two cases. */
			return s_iter_from_lone_param1(typename std::conditional<
				std::is_base_of<RandomAccessConstSectionTag, _TRALoneParam>::value || std::is_base_of<RandomAccessSectionTag, _TRALoneParam>::value
				, std::true_type, std::false_type>::type(), param);
		}
		template <typename _TRALoneParam>
		static auto s_count_from_lone_param(const _TRALoneParam& param) {
			return TRandomAccessSectionBase<_TRAIterator>::s_count_from_lone_param1(typename std::conditional<
				std::is_base_of<RandomAccessConstSectionTag, _TRALoneParam>::value || std::is_base_of<RandomAccessSectionTag, _TRALoneParam>::value
				, std::true_type, std::false_type>::type(), param);
		}

	protected:
		TRandomAccessConstSectionBase subsection(size_type pos = 0, size_type n = npos) const {
			return pos > (*this).size()
				? (MSE_THROW(msearray_range_error("out of bounds index - TRandomAccessConstSectionBase subsection() const - TRandomAccessConstSectionBase")))
					: TRandomAccessConstSectionBase((*this).m_start_iter + pos, std::min(n, (*this).size() - pos));
		}

		typedef TRASectionConstIterator<_TRAIterator> const_iterator;
		const_iterator cbegin() const { return const_iterator((*this).m_start_iter, (*this).m_count); }
		const_iterator begin() const { return cbegin(); }
		const_iterator cend() const {
			auto retval(const_iterator((*this).m_start_iter, (*this).m_count));
			retval += (*this).m_count;
			return retval;
		}
		const_iterator end() const { return cend(); }

	private:
		/* construction helper functions */
		template <typename _TRAPointer>
		static auto s_iter_from_ptr_helper2(std::true_type, const _TRAPointer& ptr) {
			/* ptr seems to be an xscope pointer.*/
			return mse::make_xscope_random_access_const_iterator(ptr, 0);
		}
		template <typename _TRAPointer>
		static auto s_iter_from_ptr_helper2(std::false_type, const _TRAPointer& ptr) {
			return mse::make_random_access_const_iterator(ptr, 0);
		}
		template <typename _TRALoneParam>
		static auto s_iter_from_lone_param2(std::true_type, const _TRALoneParam& ra_container) {
			/* The parameter seems to be a container with a "begin()" member function. So we'll use that function to obtain the
			iterator we need. */
			return ra_container.cbegin();
		}
		template <typename _TRALoneParam>
		static auto s_iter_from_lone_param2(std::false_type, const _TRALoneParam& ptr) {
			/* The parameter doesn't seem to be a container with a "begin()" member function. Here we'll assume that it is a pointer to
			a supported container. If you get a compile error here, then construction from the given parameter type isn't supported. */
			mse::T_valid_if_is_dereferenceable<_TRALoneParam>();
			return s_iter_from_ptr_helper2(typename std::is_base_of<mse::XScopeTagBase, _TRALoneParam>::type(), ptr);
		}
		template <typename _TRALoneParam>
		static auto s_iter_from_lone_param1(std::true_type, const _TRALoneParam& ra_section) {
			/* The parameter is another "random access section". */
			return ra_section.m_start_iter;
		}
		template <typename _TRALoneParam>
		static auto s_iter_from_lone_param1(std::false_type, const _TRALoneParam& param) {
			/* The parameter is not a "random access section". */
			return s_iter_from_lone_param2(typename HasOrInheritsBeginMethod_msemsearray<_TRALoneParam>::type(), param);
		}

		TRandomAccessConstSectionBase<_TRAIterator>* operator&() { return this; }
		const TRandomAccessConstSectionBase<_TRAIterator>* operator&() const { return this; }

		_TRAIterator m_start_iter;
		size_type m_count = 0;

		friend class TXScopeRandomAccessConstSection<_TRAIterator>;
		friend class TRandomAccessConstSection<_TRAIterator>;
		template<typename _TRAIterator1> friend class TRandomAccessConstSectionBase;
		/* We're friending TRandomAccessSectionBase<> because at the moment we're using its "constructor
		helper" (static) member functions, instead of duplicating them here, and those functions will need access to
		the private data members of this class. */
		template<typename _TRAIterator1> friend class TRandomAccessSectionBase;
	};

	template <typename _TRAIterator>
	class TXScopeRandomAccessConstSection : public TRandomAccessConstSectionBase<_TRAIterator>, public XScopeTagBase {
	public:
		typedef TRandomAccessConstSectionBase<_TRAIterator> base_class;
		typedef typename base_class::value_type value_type;
		typedef typename base_class::const_reference const_reference;
		typedef typename base_class::size_type size_type;
		typedef typename base_class::difference_type difference_type;
		static const size_type npos = size_type(-1);

		//TXScopeRandomAccessConstSection(const _TRAIterator& start_iter, size_type count) : base_class(start_iter, count) {}
		//TXScopeRandomAccessConstSection(const TXScopeRandomAccessConstSection& src) = default;
		//template<class _Ty2 = _TRAIterator, class = typename std::enable_if<(std::is_same<_Ty2, _TRAIterator>::value) && (!std::is_base_of<XScopeTagBase, _TRAIterator>::value), void>::type>
		//TXScopeRandomAccessConstSection(const TRandomAccessConstSectionBase<_TRAIterator>& src) : base_class(src) {}
		//template<class _Ty2 = _TRAIterator, class = typename std::enable_if<(std::is_same<_Ty2, _TRAIterator>::value) && (!std::is_base_of<XScopeTagBase, _TRAIterator>::value), void>::type>
		//TXScopeRandomAccessConstSection(const TRandomAccessSectionBase<_TRAIterator>& src) : base_class(src) {}

		MSE_USING(TXScopeRandomAccessConstSection, base_class);

		TXScopeRandomAccessConstSection xscope_subsection(size_type pos = 0, size_type n = npos) const {
			return pos > (*this).size()
				? (MSE_THROW(msearray_range_error("out of bounds index - TXScopeRandomAccessConstSection xscope_subsection() const - TXScopeRandomAccessConstSection")))
				: TXScopeRandomAccessConstSection((*this).m_start_iter + pos, std::min(n, (*this).size() - pos));
		}
		TRandomAccessConstSection<_TRAIterator> subsection(size_type pos = 0, size_type n = npos) const {
			return pos > (*this).size()
				? (MSE_THROW(msearray_range_error("out of bounds index - TRandomAccessConstSection<_TRAIterator> subsection() const - TXScopeRandomAccessConstSection")))
				: TRandomAccessConstSection<_TRAIterator>((*this).m_start_iter + pos, std::min(n, (*this).size() - pos));
		}

		//typedef typename base_class::xscope_iterator xscope_iterator;
		typedef typename base_class::xscope_const_iterator xscope_const_iterator;

		/* These are here because some standard algorithms require them. Prefer the "xscope_" prefixed versions to
		acknowledge that scope iterators are returned. */
		auto begin() const { return (*this).xscope_cbegin(); }
		auto cbegin() const { return (*this).xscope_cbegin(); }
		auto end() const { return (*this).xscope_cend(); }
		auto cend() const { return (*this).xscope_cend(); }

	private:
		TXScopeRandomAccessConstSection<_TRAIterator>& operator=(const TXScopeRandomAccessConstSection<_TRAIterator>& _Right_cref) = delete;
		void* operator new(size_t size) { return ::operator new(size); }

		TXScopeRandomAccessConstSection<_TRAIterator>* operator&() { return this; }
		const TXScopeRandomAccessConstSection<_TRAIterator>* operator&() const { return this; }
	};

	template <typename _TRAIterator>
	auto make_xscope_random_access_const_section(const _TRAIterator& start_iter, typename TXScopeRandomAccessConstSection<_TRAIterator>::size_type count) {
		return TXScopeRandomAccessConstSection<_TRAIterator>(start_iter, count);
	}
	template <typename _TRALoneParam>
	auto make_xscope_random_access_const_section(const _TRALoneParam& param) {
		typedef typename std::remove_reference<decltype(mse::TRandomAccessConstSectionBase<char *>::s_iter_from_lone_param(param))>::type _TRAIterator;
		return TXScopeRandomAccessConstSection<_TRAIterator>(param);
	}

	template <typename _TRAIterator>
	class TRandomAccessConstSection : public TRandomAccessConstSectionBase<_TRAIterator> {
	public:
		typedef TRandomAccessConstSectionBase<_TRAIterator> base_class;
		typedef typename base_class::value_type value_type;
		typedef typename base_class::const_reference const_reference;
		typedef typename base_class::size_type size_type;
		typedef typename base_class::difference_type difference_type;
		static const size_type npos = size_type(-1);

		TRandomAccessConstSection(const TRandomAccessConstSection& src) : base_class(static_cast<const base_class&>(src)) {}
		TRandomAccessConstSection(const TRandomAccessSection<_TRAIterator>& src) : base_class(static_cast<const base_class&>(src)) {}
		TRandomAccessConstSection(const _TRAIterator& start_iter, size_type count) : base_class(start_iter, count) {}
		template <typename _TRALoneParam>
		TRandomAccessConstSection(const _TRALoneParam& param) : base_class(param) {}
		virtual ~TRandomAccessConstSection() {
			mse::T_valid_if_not_an_xscope_type<_TRAIterator>();
		}

		TRandomAccessConstSection subsection(size_type pos = 0, size_type n = npos) const {
			return pos > (*this).size()
				? (MSE_THROW(msearray_range_error("out of bounds index - TRandomAccessConstSection subsection() const - TRandomAccessConstSection")))
				: TRandomAccessConstSection((*this).m_start_iter + pos, std::min(n, (*this).size() - pos));
		}

		typedef TRASectionConstIterator<_TRAIterator> const_iterator;
		const_iterator begin() const { return cbegin(); }
		const_iterator cbegin() const { return base_class::cbegin(); }
		const_iterator end() const { return cend(); }
		const_iterator cend() const { return base_class::cend(); }
		typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
		const_reverse_iterator rbegin() const {	// return iterator for beginning of reversed nonmutable sequence
			return (const_reverse_iterator(end()));
		}
		const_reverse_iterator rend() const {	// return iterator for end of reversed nonmutable sequence
			return (const_reverse_iterator(begin()));
		}
		const_reverse_iterator crbegin() const {	// return iterator for beginning of reversed nonmutable sequence
			return (rbegin());
		}
		const_reverse_iterator crend() const {	// return iterator for end of reversed nonmutable sequence
			return (rend());
		}

		friend class TXScopeRandomAccessConstSection<_TRAIterator>;
	};

	template <typename _TRAIterator>
	auto make_random_access_const_section(const _TRAIterator& start_iter, typename TRandomAccessConstSection<_TRAIterator>::size_type count) {
		return TRandomAccessConstSection<_TRAIterator>(start_iter, count);
	}
	template <typename _TRALoneParam>
	auto make_random_access_const_section(const _TRALoneParam& param) {
		typedef typename std::remove_reference<decltype(mse::TRandomAccessConstSectionBase<char *>::s_iter_from_lone_param(param))>::type _TRAIterator;
		return TRandomAccessConstSection<_TRAIterator>(param);
	}


	template <class N>
	struct is_std_array { static const int value = 0; };
	template <class N, int _Size>
	struct is_std_array<std::array<N, _Size> > { static const int value = 1; };

	/* TAsyncShareableObj is intended as a transparent wrapper for other classes/objects. */
	template<typename _TROy>
	class TAsyncShareableObj : public _TROy {
	public:
		MSE_USING(TAsyncShareableObj, _TROy);
		TAsyncShareableObj(const TAsyncShareableObj& _X) : _TROy(_X) {}
		TAsyncShareableObj(TAsyncShareableObj&& _X) : _TROy(std::forward<decltype(_X)>(_X)) {}
		virtual ~TAsyncShareableObj() {
			/* This is just a no-op function that will cause a compile error when _TROy is a prohibited type. */
			valid_if_TROy_is_not_marked_as_unshareable();
		}

		template<class _Ty2>
		TAsyncShareableObj& operator=(_Ty2&& _X) { _TROy::operator=(std::forward<decltype(_X)>(_X)); return (*this); }
		template<class _Ty2>
		TAsyncShareableObj& operator=(const _Ty2& _X) { _TROy::operator=(_X); return (*this); }

		void async_shareable_tag() const {} /* Indication that this type is eligible to be shared between threads. */

	private:

		/* If _TROy is "marked" as not safe to share among threads, then the following member function will not
		instantiate, causing an (intended) compile error. */
		template<class = typename std::enable_if<(std::integral_constant<bool, HasAsyncShareableTagMethod_msemsearray<_TROy>::Has>()) || (
			(!std::is_convertible<_TROy*, NotAsyncShareableTagBase*>::value)
			/*(!std::integral_constant<bool, HasNotAsyncShareableTagMethod_msemsearray<_TROy>::Has>())*/
				), void>::type>
		void valid_if_TROy_is_not_marked_as_unshareable() const {}

		TAsyncShareableObj* operator&() {
			return this;
		}
		const TAsyncShareableObj* operator&() const {
			return this;
		}
	};

	namespace us {
		template<typename _TROy> using TUserDeclaredAsyncShareableObj = mse::TAsyncShareableObj<_TROy>;
	}


	template<class _Ty, class _TAccessMutex = non_thread_safe_recursive_shared_timed_mutex> class TAccessControlledReadWriteObj;
	template<typename _Ty, class _TAccessMutex = non_thread_safe_recursive_shared_timed_mutex> class TAccessControlledReadWriteConstPointer;

	template<typename _Ty, class _TAccessMutex = non_thread_safe_recursive_shared_timed_mutex>
	class TAccessControlledReadWritePointer {
	public:
		TAccessControlledReadWritePointer(const TAccessControlledReadWriteObj<_Ty, _TAccessMutex>& src);
		TAccessControlledReadWritePointer(const TAccessControlledReadWritePointer& src) : m_obj_ptr(src.m_obj_ptr), m_unique_lock(src.m_obj_ptr->m_mutex1) {}
		TAccessControlledReadWritePointer(TAccessControlledReadWritePointer&& src) = default; /* Note, the move constructor is only safe when std::move() is prohibited. */
		virtual ~TAccessControlledReadWritePointer() {}

		operator bool() const {
			//assert(is_valid()); //{ MSE_THROW(asyncshared_use_of_invalid_pointer_error("attempt to use invalid pointer - mse::TAccessControlledReadWritePointer")); }
			return bool(m_obj_ptr);
		}
		typename std::conditional<std::is_const<_Ty>::value
			, const TAccessControlledReadWriteObj<_Ty, _TAccessMutex>&, TAccessControlledReadWriteObj<_Ty, _TAccessMutex>&>::type operator*() const {
			assert(is_valid()); //{ MSE_THROW(asyncshared_use_of_invalid_pointer_error("attempt to use invalid pointer - mse::TAccessControlledReadWritePointer")); }
			return (*m_obj_ptr);
		}
		typename std::conditional<std::is_const<_Ty>::value
			, const TAccessControlledReadWriteObj<_Ty, _TAccessMutex>*, TAccessControlledReadWriteObj<_Ty, _TAccessMutex>*>::type operator->() const {
			assert(is_valid()); //{ MSE_THROW(asyncshared_use_of_invalid_pointer_error("attempt to use invalid pointer - mse::TAccessControlledReadWritePointer")); }
			return std::addressof(*m_obj_ptr);
		}
	private:
		TAccessControlledReadWritePointer(TAccessControlledReadWriteObj<_Ty, _TAccessMutex>* obj_ptr) : m_obj_ptr(obj_ptr), m_unique_lock(obj_ptr->m_mutex1) {}
		TAccessControlledReadWritePointer(TAccessControlledReadWriteObj<_Ty, _TAccessMutex>* obj_ptr, std::try_to_lock_t) : m_obj_ptr(obj_ptr), m_unique_lock(obj_ptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock()) {
				m_obj_ptr = nullptr;
			}
		}
		template<class _Rep, class _Period>
		TAccessControlledReadWritePointer(TAccessControlledReadWriteObj<_Ty, _TAccessMutex>* obj_ptr, std::try_to_lock_t, const std::chrono::duration<_Rep, _Period>& _Rel_time) : m_obj_ptr(obj_ptr), m_unique_lock(obj_ptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock_for(_Rel_time)) {
				m_obj_ptr = nullptr;
			}
		}
		template<class _Clock, class _Duration>
		TAccessControlledReadWritePointer(TAccessControlledReadWriteObj<_Ty, _TAccessMutex>* obj_ptr, std::try_to_lock_t, const std::chrono::time_point<_Clock, _Duration>& _Abs_time) : m_obj_ptr(obj_ptr), m_unique_lock(obj_ptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock_until(_Abs_time)) {
				m_obj_ptr = nullptr;
			}
		}
		TAccessControlledReadWritePointer<_Ty, _TAccessMutex>& operator=(const TAccessControlledReadWritePointer<_Ty, _TAccessMutex>& _Right_cref) = delete;
		TAccessControlledReadWritePointer<_Ty, _TAccessMutex>& operator=(TAccessControlledReadWritePointer<_Ty, _TAccessMutex>&& _Right) = delete;

		TAccessControlledReadWritePointer<_Ty, _TAccessMutex>* operator&() { return this; }
		const TAccessControlledReadWritePointer<_Ty, _TAccessMutex>* operator&() const { return this; }
		bool is_valid() const {
			bool retval = bool(m_obj_ptr);
			return retval;
		}

		_Ty* m_obj_ptr = nullptr;
		std::unique_lock<_TAccessMutex> m_unique_lock;

		friend class TAccessControlledReadWriteObj<_Ty, _TAccessMutex>;
		friend class TAccessControlledReadWriteConstPointer<_Ty, _TAccessMutex>;
	};

	template<typename _Ty, class _TAccessMutex>
	class TAccessControlledReadWriteConstPointer {
	public:
		TAccessControlledReadWriteConstPointer(const TAccessControlledReadWriteConstPointer& src) : m_obj_ptr(src.m_obj_ptr), m_unique_lock(src.m_obj_ptr->m_mutex1) {}
		TAccessControlledReadWriteConstPointer(TAccessControlledReadWriteConstPointer&& src) = default;
		TAccessControlledReadWriteConstPointer(const TAccessControlledReadWritePointer<_Ty, _TAccessMutex>& src) : m_obj_ptr(src.m_obj_ptr), m_unique_lock(src.m_obj_ptr->m_mutex1) {}
		virtual ~TAccessControlledReadWriteConstPointer() {}

		operator bool() const {
			//assert(is_valid()); //{ MSE_THROW(asyncshared_use_of_invalid_pointer_error("attempt to use invalid pointer - mse::TAccessControlledReadWriteConstPointer")); }
			return bool(m_obj_ptr);
		}
		const TAccessControlledReadWriteObj<const _Ty>& operator*() const {
			assert(is_valid()); //{ MSE_THROW(asyncshared_use_of_invalid_pointer_error("attempt to use invalid pointer - mse::TAccessControlledReadWriteConstPointer")); }
			const TAccessControlledReadWriteObj<const _Ty>* extra_const_ptr = reinterpret_cast<const TAccessControlledReadWriteObj<const _Ty>*>(std::addressof(*m_obj_ptr));
			return (*extra_const_ptr);
		}
		const TAccessControlledReadWriteObj<const _Ty>* operator->() const {
			assert(is_valid()); //{ MSE_THROW(asyncshared_use_of_invalid_pointer_error("attempt to use invalid pointer - mse::TAccessControlledReadWriteConstPointer")); }
			const TAccessControlledReadWriteObj<const _Ty>* extra_const_ptr = reinterpret_cast<const TAccessControlledReadWriteObj<const _Ty>*>(std::addressof(*m_obj_ptr));
			return extra_const_ptr;
		}
	private:
		TAccessControlledReadWriteConstPointer(TAccessControlledReadWriteObj<_Ty, _TAccessMutex>* obj_ptr) : m_obj_ptr(obj_ptr), m_unique_lock(obj_ptr->m_mutex1) {}
		TAccessControlledReadWriteConstPointer(TAccessControlledReadWriteObj<_Ty, _TAccessMutex>* obj_ptr, std::try_to_lock_t) : m_obj_ptr(obj_ptr), m_unique_lock(obj_ptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock()) {
				m_obj_ptr = nullptr;
			}
		}
		template<class _Rep, class _Period>
		TAccessControlledReadWriteConstPointer(TAccessControlledReadWriteObj<_Ty, _TAccessMutex>* obj_ptr, std::try_to_lock_t, const std::chrono::duration<_Rep, _Period>& _Rel_time) : m_obj_ptr(obj_ptr), m_unique_lock(obj_ptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock_for(_Rel_time)) {
				m_obj_ptr = nullptr;
			}
		}
		template<class _Clock, class _Duration>
		TAccessControlledReadWriteConstPointer(TAccessControlledReadWriteObj<_Ty, _TAccessMutex>* obj_ptr, std::try_to_lock_t, const std::chrono::time_point<_Clock, _Duration>& _Abs_time) : m_obj_ptr(obj_ptr), m_unique_lock(obj_ptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock_until(_Abs_time)) {
				m_obj_ptr = nullptr;
			}
		}
		TAccessControlledReadWriteConstPointer<_Ty, _TAccessMutex>& operator=(const TAccessControlledReadWriteConstPointer<_Ty, _TAccessMutex>& _Right_cref) = delete;
		TAccessControlledReadWriteConstPointer<_Ty, _TAccessMutex>& operator=(TAccessControlledReadWriteConstPointer<_Ty, _TAccessMutex>&& _Right) = delete;

		TAccessControlledReadWriteConstPointer<_Ty, _TAccessMutex>* operator&() { return this; }
		const TAccessControlledReadWriteConstPointer<_Ty, _TAccessMutex>* operator&() const { return this; }
		bool is_valid() const {
			bool retval = bool(m_obj_ptr);
			return retval;
		}

		_Ty* m_obj_ptr = nullptr;
		typedef recursive_shared_mutex_wrapped<_TAccessMutex> _TWrappedAccessMutex;
		std::shared_lock<_TWrappedAccessMutex> m_unique_lock;

		friend class TAccessControlledReadWriteObj<_Ty, _TAccessMutex>;
	};

	template<typename _Ty, class _TAccessMutex = non_thread_safe_recursive_shared_timed_mutex>
	class TAccessControlledExclusiveReadWritePointer {
	public:
		TAccessControlledExclusiveReadWritePointer(const TAccessControlledExclusiveReadWritePointer& src) = delete;
		TAccessControlledExclusiveReadWritePointer(TAccessControlledExclusiveReadWritePointer&& src) = default;
		virtual ~TAccessControlledExclusiveReadWritePointer() {}

		operator bool() const {
			//assert(is_valid()); //{ MSE_THROW(asyncshared_use_of_invalid_pointer_error("attempt to use invalid pointer - mse::TAccessControlledExclusiveReadWritePointer")); }
			return bool(m_obj_ptr);
		}
		typename std::conditional<std::is_const<_Ty>::value
			, const TAccessControlledReadWriteObj<_Ty, _TAccessMutex>&, TAccessControlledReadWriteObj<_Ty, _TAccessMutex>&>::type operator*() const {
			assert(is_valid()); //{ MSE_THROW(asyncshared_use_of_invalid_pointer_error("attempt to use invalid pointer - mse::TAccessControlledExclusiveReadWritePointer")); }
			return (*m_obj_ptr);
		}
		typename std::conditional<std::is_const<_Ty>::value
			, const TAccessControlledReadWriteObj<_Ty, _TAccessMutex>*, TAccessControlledReadWriteObj<_Ty, _TAccessMutex>*>::type operator->() const {
			assert(is_valid()); //{ MSE_THROW(asyncshared_use_of_invalid_pointer_error("attempt to use invalid pointer - mse::TAccessControlledExclusiveReadWritePointer")); }
			return std::addressof(*m_obj_ptr);
		}
	private:
		TAccessControlledExclusiveReadWritePointer(TAccessControlledReadWriteObj<_Ty, _TAccessMutex>* obj_ptr) : m_obj_ptr(obj_ptr), m_unique_lock(obj_ptr->m_mutex1) {}
		TAccessControlledExclusiveReadWritePointer(TAccessControlledReadWriteObj<_Ty, _TAccessMutex>* obj_ptr, std::try_to_lock_t) : m_obj_ptr(obj_ptr), m_unique_lock(obj_ptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock()) {
				m_obj_ptr = nullptr;
			}
		}
		template<class _Rep, class _Period>
		TAccessControlledExclusiveReadWritePointer(TAccessControlledReadWriteObj<_Ty, _TAccessMutex>* obj_ptr, std::try_to_lock_t, const std::chrono::duration<_Rep, _Period>& _Rel_time) : m_obj_ptr(obj_ptr), m_unique_lock(obj_ptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock_for(_Rel_time)) {
				m_obj_ptr = nullptr;
			}
		}
		template<class _Clock, class _Duration>
		TAccessControlledExclusiveReadWritePointer(TAccessControlledReadWriteObj<_Ty, _TAccessMutex>* obj_ptr, std::try_to_lock_t, const std::chrono::time_point<_Clock, _Duration>& _Abs_time) : m_obj_ptr(obj_ptr), m_unique_lock(obj_ptr->m_mutex1, std::defer_lock) {
			if (!m_unique_lock.try_lock_until(_Abs_time)) {
				m_obj_ptr = nullptr;
			}
		}
		TAccessControlledExclusiveReadWritePointer<_Ty, _TAccessMutex>& operator=(const TAccessControlledExclusiveReadWritePointer<_Ty, _TAccessMutex>& _Right_cref) = delete;
		TAccessControlledExclusiveReadWritePointer<_Ty, _TAccessMutex>& operator=(TAccessControlledExclusiveReadWritePointer<_Ty, _TAccessMutex>&& _Right) = delete;

		TAccessControlledExclusiveReadWritePointer<_Ty, _TAccessMutex>* operator&() { return this; }
		const TAccessControlledExclusiveReadWritePointer<_Ty, _TAccessMutex>* operator&() const { return this; }
		bool is_valid() const {
			bool retval = bool(m_obj_ptr);
			return retval;
		}

		TAccessControlledReadWriteObj<_Ty, _TAccessMutex>* m_obj_ptr;
		typedef recursive_shared_mutex_wrapped<_TAccessMutex> _TWrappedAccessMutex;
		unique_nonrecursive_lock<_TWrappedAccessMutex> m_unique_lock;

		friend class TAccessControlledReadWriteObj<_Ty, _TAccessMutex>;
		//friend class TAccessControlledReadWriteExclusiveConstPointer<_Ty, _TAccessMutex>;
	};

	template<class _Ty, class _TAccessMutex/* = non_thread_safe_recursive_shared_timed_mutex*/>
	class TAccessControlledReadWriteObj {
	public:
		TAccessControlledReadWriteObj(const TAccessControlledReadWriteObj& src_cref) : m_obj(src_cref.m_obj) {}

		template <class... Args>
		TAccessControlledReadWriteObj(Args&&... args) : m_obj(std::forward<Args>(args)...) {}
		~TAccessControlledReadWriteObj() {
			try {
				m_mutex1.nonrecursive_lock();
				//m_mutex1.nonrecursive_unlock();
			}
			catch (...) {
				/* It would be unsafe to allow this object to be destroyed as there are outstanding references to this object (in
				this thread). */
				std::cerr << "\n\nFatal Error: mse::TAccessControlledReadWriteObj<> destructed with outstanding references in the same thread \n\n";
				std::terminate();
			}

			/* This is just a no-op function that will cause a compile error when _Ty is not an eligible type. */
			valid_if_Ty_is_marked_as_shareable();
		}

		TAccessControlledReadWritePointer<_Ty, _TAccessMutex> writelock_ptr() {
			return TAccessControlledReadWritePointer<_Ty, _TAccessMutex>(*this);
		}
		mse::mstd::optional<TAccessControlledReadWritePointer<_Ty, _TAccessMutex>> try_writelock_ptr() {
			mse::mstd::optional<TAccessControlledReadWritePointer<_Ty, _TAccessMutex>> retval(TAccessControlledReadWritePointer<_Ty, _TAccessMutex>(*this, std::try_to_lock));
			if (!((*retval).is_valid())) {
				return{};
			}
			return retval;
		}
		template<class _Rep, class _Period>
		mse::mstd::optional<TAccessControlledReadWritePointer<_Ty, _TAccessMutex>> try_writelock_ptr_for(const std::chrono::duration<_Rep, _Period>& _Rel_time) {
			mse::mstd::optional<TAccessControlledReadWritePointer<_Ty, _TAccessMutex>> retval(TAccessControlledReadWritePointer<_Ty, _TAccessMutex>(*this, std::try_to_lock, _Rel_time));
			if (!((*retval).is_valid())) {
				return{};
			}
			return retval;
		}
		template<class _Clock, class _Duration>
		mse::mstd::optional<TAccessControlledReadWritePointer<_Ty, _TAccessMutex>> try_writelock_ptr_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time) {
			mse::mstd::optional<TAccessControlledReadWritePointer<_Ty, _TAccessMutex>> retval(TAccessControlledReadWritePointer<_Ty, _TAccessMutex>(*this, std::try_to_lock, _Abs_time));
			if (!((*retval).is_valid())) {
				return{};
			}
			return retval;
		}
		TAccessControlledReadWriteConstPointer<_Ty, _TAccessMutex> readlock_ptr() {
			return TAccessControlledReadWriteConstPointer<_Ty, _TAccessMutex>(*this);
		}
		mse::mstd::optional<TAccessControlledReadWriteConstPointer<_Ty, _TAccessMutex>> try_readlock_ptr() {
			mse::mstd::optional<TAccessControlledReadWriteConstPointer<_Ty, _TAccessMutex>> retval(TAccessControlledReadWriteConstPointer<_Ty, _TAccessMutex>(*this, std::try_to_lock));
			if (!((*retval).is_valid())) {
				return{};
			}
			return retval;
		}
		template<class _Rep, class _Period>
		mse::mstd::optional<TAccessControlledReadWriteConstPointer<_Ty, _TAccessMutex>> try_readlock_ptr_for(const std::chrono::duration<_Rep, _Period>& _Rel_time) {
			mse::mstd::optional<TAccessControlledReadWriteConstPointer<_Ty, _TAccessMutex>> retval(TAccessControlledReadWriteConstPointer<_Ty, _TAccessMutex>(*this, std::try_to_lock, _Rel_time));
			if (!((*retval).is_valid())) {
				return{};
			}
			return retval;
		}
		template<class _Clock, class _Duration>
		mse::mstd::optional<TAccessControlledReadWriteConstPointer<_Ty, _TAccessMutex>> try_readlock_ptr_until(const std::chrono::time_point<_Clock, _Duration>& _Abs_time) {
			mse::mstd::optional<TAccessControlledReadWriteConstPointer<_Ty, _TAccessMutex>> retval(TAccessControlledReadWriteConstPointer<_Ty, _TAccessMutex>(*this, std::try_to_lock, _Abs_time));
			if (!((*retval).is_valid())) {
				return{};
			}
			return retval;
		}
		/* Note that an exclusive_writelock_ptr cannot coexist with any other lock_ptrs (targeting the same object), including ones in
		the same thread. Thus, using exclusive_writelock_ptrs without sufficient care introduces the potential for exceptions (in a way
		that sticking to (regular) writelock_ptrs doesn't). */
		TAccessControlledExclusiveReadWritePointer<_Ty, _TAccessMutex> exclusive_writelock_ptr() {
			return TAccessControlledExclusiveReadWritePointer<_Ty, _TAccessMutex>(this);
		}

		typedef recursive_shared_mutex_wrapped<_TAccessMutex> _TWrappedAccessMutex;
		_TWrappedAccessMutex m_mutex1;

	private:
		/* If _Ty is not "marked" as safe to share among threads (via the presence of the "async_shareable_tag()" member
		function), then the following member function will not instantiate, causing an (intended) compile error. User-defined
		objects can be marked safe to share by wrapping them with us::TUserDeclaredAsyncShareableObj<>. */
		template<class _Ty2 = _Ty, class = typename std::enable_if<(std::is_same<_Ty2, _Ty>::value) && (std::integral_constant<bool, HasAsyncShareableTagMethod_msemsearray<_Ty2>::Has>()), void>::type>
		void valid_if_Ty_is_marked_as_shareable() const {}

		TAccessControlledReadWriteObj* operator&() { return this; }
		const TAccessControlledReadWriteObj* operator&() const { return this; }

		_Ty m_obj;

		//friend class TAccessControlledReadOnlyObj<_Ty, _TAccessMutex>;
		friend class TAccessControlledReadWritePointer<_Ty, _TAccessMutex>;
	};

	template<typename _Ty, class _TAccessMutex/* = non_thread_safe_recursive_shared_timed_mutex*/>
	TAccessControlledReadWritePointer<_Ty, _TAccessMutex>::TAccessControlledReadWritePointer(const TAccessControlledReadWriteObj<_Ty, _TAccessMutex>& src) : m_obj_ptr(&src), m_unique_lock(src.m_mutex1) {}


#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif /*__clang__*/
	
	/* These are specializations of the array_helper_type template class defined earlier in this file. */
	template<class _Ty> class array_helper_type<_Ty, 2> {
	public:
		static typename std::array<_Ty, 2> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			if (2 != _Ilist.size()) { MSE_THROW(msearray_range_error("the size of the initializer list does not match the size of the array - mse::mstd::array")); }
			typename std::array<_Ty, 2> retval{ *(_Ilist.begin()), *(_Ilist.begin() + 1) }; return retval;
		}
	};
	template<class _Ty> class array_helper_type<_Ty, 3> {
	public:
		static typename std::array<_Ty, 3> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			if (3 != _Ilist.size()) { MSE_THROW(msearray_range_error("the size of the initializer list does not match the size of the array - mse::mstd::array")); }
			typename std::array<_Ty, 3> retval{ *(_Ilist.begin()), *(_Ilist.begin() + 1), *(_Ilist.begin() + 2) }; return retval;
		}
	};
	template<class _Ty> class array_helper_type<_Ty, 4> {
	public:
		static typename std::array<_Ty, 4> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			if (4 != _Ilist.size()) { MSE_THROW(msearray_range_error("the size of the initializer list does not match the size of the array - mse::mstd::array")); }
			typename std::array<_Ty, 4> retval{ *(_Ilist.begin()), *(_Ilist.begin() + 1), *(_Ilist.begin() + 2), *(_Ilist.begin() + 3) }; return retval;
		}
	};
	template<class _Ty> class array_helper_type<_Ty, 5> {
	public:
		static typename std::array<_Ty, 5> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			if (5 != _Ilist.size()) { MSE_THROW(msearray_range_error("the size of the initializer list does not match the size of the array - mse::mstd::array")); }
			typename std::array<_Ty, 5> retval{ *(_Ilist.begin()), *(_Ilist.begin() + 1), *(_Ilist.begin() + 2), *(_Ilist.begin() + 3), *(_Ilist.begin() + 4) }; return retval;
		}
	};
	template<class _Ty> class array_helper_type<_Ty, 6> {
	public:
		static typename std::array<_Ty, 6> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			if (6 != _Ilist.size()) { MSE_THROW(msearray_range_error("the size of the initializer list does not match the size of the array - mse::mstd::array")); }
			typename std::array<_Ty, 6> retval{ *(_Ilist.begin()), *(_Ilist.begin() + 1), *(_Ilist.begin() + 2), *(_Ilist.begin() + 3), *(_Ilist.begin() + 4), *(_Ilist.begin() + 5) }; return retval;
		}
	};
	template<class _Ty> class array_helper_type<_Ty, 7> {
	public:
		static typename std::array<_Ty, 7> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			if (7 != _Ilist.size()) { MSE_THROW(msearray_range_error("the size of the initializer list does not match the size of the array - mse::mstd::array")); }
			typename std::array<_Ty, 7> retval{ *(_Ilist.begin()), *(_Ilist.begin() + 1), *(_Ilist.begin() + 2), *(_Ilist.begin() + 3), *(_Ilist.begin() + 4), *(_Ilist.begin() + 5), *(_Ilist.begin() + 6) }; return retval;
		}
	};
	template<class _Ty> class array_helper_type<_Ty, 8> {
	public:
		static typename std::array<_Ty, 8> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			if (8 != _Ilist.size()) { MSE_THROW(msearray_range_error("the size of the initializer list does not match the size of the array - mse::mstd::array")); }
			typename std::array<_Ty, 8> retval{ *(_Ilist.begin()), *(_Ilist.begin() + 1), *(_Ilist.begin() + 2), *(_Ilist.begin() + 3), *(_Ilist.begin() + 4), *(_Ilist.begin() + 5), *(_Ilist.begin() + 6), *(_Ilist.begin() + 7) }; return retval;
		}
	};
	template<class _Ty> class array_helper_type<_Ty, 9> {
	public:
		static typename std::array<_Ty, 9> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			if (9 != _Ilist.size()) { MSE_THROW(msearray_range_error("the size of the initializer list does not match the size of the array - mse::mstd::array")); }
			typename std::array<_Ty, 9> retval{ *(_Ilist.begin()), *(_Ilist.begin() + 1), *(_Ilist.begin() + 2), *(_Ilist.begin() + 3), *(_Ilist.begin() + 4), *(_Ilist.begin() + 5), *(_Ilist.begin() + 6), *(_Ilist.begin() + 7), *(_Ilist.begin() + 8) }; return retval;
		}
	};
	template<class _Ty> class array_helper_type<_Ty, 10> {
	public:
		static typename std::array<_Ty, 10> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			if (10 != _Ilist.size()) { MSE_THROW(msearray_range_error("the size of the initializer list does not match the size of the array - mse::mstd::array")); }
			typename std::array<_Ty, 10> retval{ *(_Ilist.begin()), *(_Ilist.begin() + 1), *(_Ilist.begin() + 2), *(_Ilist.begin() + 3), *(_Ilist.begin() + 4), *(_Ilist.begin() + 5), *(_Ilist.begin() + 6), *(_Ilist.begin() + 7), *(_Ilist.begin() + 8), *(_Ilist.begin() + 9) }; return retval;
		}
	};
	template<class _Ty> class array_helper_type<_Ty, 11> {
	public:
		static typename std::array<_Ty, 11> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			if (11 != _Ilist.size()) { MSE_THROW(msearray_range_error("the size of the initializer list does not match the size of the array - mse::mstd::array")); }
			typename std::array<_Ty, 11> retval{ *(_Ilist.begin()), *(_Ilist.begin() + 1), *(_Ilist.begin() + 2), *(_Ilist.begin() + 3), *(_Ilist.begin() + 4), *(_Ilist.begin() + 5), *(_Ilist.begin() + 6), *(_Ilist.begin() + 7), *(_Ilist.begin() + 8), *(_Ilist.begin() + 9), *(_Ilist.begin() + 10) }; return retval;
		}
	};
	template<class _Ty> class array_helper_type<_Ty, 12> {
	public:
		static typename std::array<_Ty, 12> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			if (12 != _Ilist.size()) { MSE_THROW(msearray_range_error("the size of the initializer list does not match the size of the array - mse::mstd::array")); }
			typename std::array<_Ty, 12> retval{ *(_Ilist.begin()), *(_Ilist.begin() + 1), *(_Ilist.begin() + 2), *(_Ilist.begin() + 3), *(_Ilist.begin() + 4), *(_Ilist.begin() + 5), *(_Ilist.begin() + 6), *(_Ilist.begin() + 7), *(_Ilist.begin() + 8), *(_Ilist.begin() + 9), *(_Ilist.begin() + 10), *(_Ilist.begin() + 11) }; return retval;
		}
	};
	template<class _Ty> class array_helper_type<_Ty, 13> {
	public:
		static typename std::array<_Ty, 13> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			if (13 != _Ilist.size()) { MSE_THROW(msearray_range_error("the size of the initializer list does not match the size of the array - mse::mstd::array")); }
			typename std::array<_Ty, 13> retval{ *(_Ilist.begin()), *(_Ilist.begin() + 1), *(_Ilist.begin() + 2), *(_Ilist.begin() + 3), *(_Ilist.begin() + 4), *(_Ilist.begin() + 5), *(_Ilist.begin() + 6), *(_Ilist.begin() + 7), *(_Ilist.begin() + 8), *(_Ilist.begin() + 9), *(_Ilist.begin() + 10), *(_Ilist.begin() + 11), *(_Ilist.begin() + 12) }; return retval;
		}
	};
	template<class _Ty> class array_helper_type<_Ty, 14> {
	public:
		static typename std::array<_Ty, 14> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			if (14 != _Ilist.size()) { MSE_THROW(msearray_range_error("the size of the initializer list does not match the size of the array - mse::mstd::array")); }
			typename std::array<_Ty, 14> retval{ *(_Ilist.begin()), *(_Ilist.begin() + 1), *(_Ilist.begin() + 2), *(_Ilist.begin() + 3), *(_Ilist.begin() + 4), *(_Ilist.begin() + 5), *(_Ilist.begin() + 6), *(_Ilist.begin() + 7), *(_Ilist.begin() + 8), *(_Ilist.begin() + 9), *(_Ilist.begin() + 10), *(_Ilist.begin() + 11), *(_Ilist.begin() + 12), *(_Ilist.begin() + 13) }; return retval;
		}
	};
	template<class _Ty> class array_helper_type<_Ty, 15> {
	public:
		static typename std::array<_Ty, 15> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			if (15 != _Ilist.size()) { MSE_THROW(msearray_range_error("the size of the initializer list does not match the size of the array - mse::mstd::array")); }
			typename std::array<_Ty, 15> retval{ *(_Ilist.begin()), *(_Ilist.begin() + 1), *(_Ilist.begin() + 2), *(_Ilist.begin() + 3), *(_Ilist.begin() + 4), *(_Ilist.begin() + 5), *(_Ilist.begin() + 6), *(_Ilist.begin() + 7), *(_Ilist.begin() + 8), *(_Ilist.begin() + 9), *(_Ilist.begin() + 10), *(_Ilist.begin() + 11), *(_Ilist.begin() + 12), *(_Ilist.begin() + 13), *(_Ilist.begin() + 14) }; return retval;
		}
	};
	template<class _Ty> class array_helper_type<_Ty, 16> {
	public:
		static typename std::array<_Ty, 16> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			if (16 != _Ilist.size()) { MSE_THROW(msearray_range_error("the size of the initializer list does not match the size of the array - mse::mstd::array")); }
			typename std::array<_Ty, 16> retval{ *(_Ilist.begin()), *(_Ilist.begin() + 1), *(_Ilist.begin() + 2), *(_Ilist.begin() + 3), *(_Ilist.begin() + 4), *(_Ilist.begin() + 5), *(_Ilist.begin() + 6), *(_Ilist.begin() + 7), *(_Ilist.begin() + 8), *(_Ilist.begin() + 9), *(_Ilist.begin() + 10), *(_Ilist.begin() + 11), *(_Ilist.begin() + 12), *(_Ilist.begin() + 13), *(_Ilist.begin() + 14), *(_Ilist.begin() + 15) }; return retval;
		}
	};
	template<class _Ty> class array_helper_type<_Ty, 17> {
	public:
		static typename std::array<_Ty, 17> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			if (17 != _Ilist.size()) { MSE_THROW(msearray_range_error("the size of the initializer list does not match the size of the array - mse::mstd::array")); }
			typename std::array<_Ty, 17> retval{ *(_Ilist.begin()), *(_Ilist.begin() + 1), *(_Ilist.begin() + 2), *(_Ilist.begin() + 3), *(_Ilist.begin() + 4), *(_Ilist.begin() + 5), *(_Ilist.begin() + 6), *(_Ilist.begin() + 7), *(_Ilist.begin() + 8), *(_Ilist.begin() + 9), *(_Ilist.begin() + 10), *(_Ilist.begin() + 11), *(_Ilist.begin() + 12), *(_Ilist.begin() + 13), *(_Ilist.begin() + 14), *(_Ilist.begin() + 15), *(_Ilist.begin() + 16) }; return retval;
		}
	};
	template<class _Ty> class array_helper_type<_Ty, 18> {
	public:
		static typename std::array<_Ty, 18> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			if (18 != _Ilist.size()) { MSE_THROW(msearray_range_error("the size of the initializer list does not match the size of the array - mse::mstd::array")); }
			typename std::array<_Ty, 18> retval{ *(_Ilist.begin()), *(_Ilist.begin() + 1), *(_Ilist.begin() + 2), *(_Ilist.begin() + 3), *(_Ilist.begin() + 4), *(_Ilist.begin() + 5), *(_Ilist.begin() + 6), *(_Ilist.begin() + 7), *(_Ilist.begin() + 8), *(_Ilist.begin() + 9), *(_Ilist.begin() + 10), *(_Ilist.begin() + 11), *(_Ilist.begin() + 12), *(_Ilist.begin() + 13), *(_Ilist.begin() + 14), *(_Ilist.begin() + 15), *(_Ilist.begin() + 16), *(_Ilist.begin() + 17) }; return retval;
		}
	};
	template<class _Ty> class array_helper_type<_Ty, 19> {
	public:
		static typename std::array<_Ty, 19> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			if (19 != _Ilist.size()) { MSE_THROW(msearray_range_error("the size of the initializer list does not match the size of the array - mse::mstd::array")); }
			typename std::array<_Ty, 19> retval{ *(_Ilist.begin()), *(_Ilist.begin() + 1), *(_Ilist.begin() + 2), *(_Ilist.begin() + 3), *(_Ilist.begin() + 4), *(_Ilist.begin() + 5), *(_Ilist.begin() + 6), *(_Ilist.begin() + 7), *(_Ilist.begin() + 8), *(_Ilist.begin() + 9), *(_Ilist.begin() + 10), *(_Ilist.begin() + 11), *(_Ilist.begin() + 12), *(_Ilist.begin() + 13), *(_Ilist.begin() + 14), *(_Ilist.begin() + 15), *(_Ilist.begin() + 16), *(_Ilist.begin() + 17), *(_Ilist.begin() + 18) }; return retval;
		}
	};
	template<class _Ty> class array_helper_type<_Ty, 20> {
	public:
		static typename std::array<_Ty, 20> std_array_initial_value2(_XSTD initializer_list<_Ty> _Ilist) {
			if (20 != _Ilist.size()) { MSE_THROW(msearray_range_error("the size of the initializer list does not match the size of the array - mse::mstd::array")); }
			typename std::array<_Ty, 20> retval{ *(_Ilist.begin()), *(_Ilist.begin() + 1), *(_Ilist.begin() + 2), *(_Ilist.begin() + 3), *(_Ilist.begin() + 4), *(_Ilist.begin() + 5), *(_Ilist.begin() + 6), *(_Ilist.begin() + 7), *(_Ilist.begin() + 8), *(_Ilist.begin() + 9), *(_Ilist.begin() + 10), *(_Ilist.begin() + 11), *(_Ilist.begin() + 12), *(_Ilist.begin() + 13), *(_Ilist.begin() + 14), *(_Ilist.begin() + 15), *(_Ilist.begin() + 16), *(_Ilist.begin() + 17), *(_Ilist.begin() + 18), *(_Ilist.begin() + 19) }; return retval;
		}
	};

#ifdef __clang__
#pragma clang diagnostic pop
#endif /*__clang__*/
	

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-function"
#else /*__clang__*/
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif /*__GNUC__*/
#endif /*__clang__*/

	class msearray_test {
	public:
		void test1() {
#ifdef MSE_SELF_TESTS
			// construction uses aggregate initialization
			mse::us::msearray<int, 3> a1{ { 1, 2, 3 } }; // double-braces required in C++11 (not in C++14)
			mse::us::msearray<int, 3> a2 = { 11, 12, 13 };  // never required after =
			mse::us::msearray<std::string, 2> a3 = { std::string("a"), "b" };

			// container operations are supported
			std::sort(a1.begin(), a1.end());
			std::reverse_copy(a2.begin(), a2.end(),
				std::ostream_iterator<int>(std::cout, " "));

			std::cout << '\n';

			// ranged for loop is supported
			for (const auto& s : a3)
				std::cout << s << ' ';

			a1.swap(a2);
			assert(13 == a1[2]);
			assert(3 == a2[2]);

			std::swap(a1, a2);
			assert(3 == a1[2]);
			assert(13 == a2[2]);

			std::get<0>(a1) = 21;
			std::get<1>(a1) = 22;
			std::get<2>(a1) = 23;

			auto l_tuple_size = std::tuple_size<mse::us::msearray<int, 3>>::value;
			std::tuple_element<1, mse::us::msearray<int, 3>>::type b1 = 5;

			a1 = a2;

			{
				mse::us::msearray<int, 5> a = { 10, 20, 30, 40, 50 };
				mse::us::msearray<int, 5> b = { 10, 20, 30, 40, 50 };
				mse::us::msearray<int, 5> c = { 50, 40, 30, 20, 10 };

				if (a == b) std::cout << "a and b are equal\n";
				if (b != c) std::cout << "b and c are not equal\n";
				if (b<c) std::cout << "b is less than c\n";
				if (c>b) std::cout << "c is greater than b\n";
				if (a <= b) std::cout << "a is less than or equal to b\n";
				if (a >= b) std::cout << "a is greater than or equal to b\n";
			}
			{
				mse::us::msearray<int, 6> myarray;

				myarray.fill(5);

				std::cout << "myarray contains:";
				for (int& x : myarray) { std::cout << ' ' << x; }

				std::cout << '\n';
			}
#endif // MSE_SELF_TESTS
		}
	};

#ifdef __clang__
#pragma clang diagnostic pop
#else /*__clang__*/
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif /*__GNUC__*/
#endif /*__clang__*/

}

#undef MSE_THROW

#ifdef _MSC_VER
#pragma warning( pop )  
#endif /*_MSC_VER*/

#endif /*ndef MSEMSEARRAY_H*/
