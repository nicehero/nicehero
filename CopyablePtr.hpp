#ifndef ____NICE__COPYABLEPTR____
#define ____NICE__COPYABLEPTR____
#include <memory>
#include "Log.h"
namespace nicehero {

	/* 
		A copyable unique_ptr
	*/
	template <class T>
	class CopyablePtr {
	public:
		/** If value can be default-constructed, why not?
			Then we don't have to move it in */
		CopyablePtr() = default;

		/// Move a value in.
		explicit CopyablePtr(T&& t) = delete;
		explicit CopyablePtr(T* t) : value(t) {}

		/// copy is move
		CopyablePtr(const CopyablePtr& other) : value(std::move(other.value)) {}

		/// move is also move
		CopyablePtr(CopyablePtr&& other) : value(std::move(other.value)) {}

		const T& operator*() const {
			return (*value);
		}
		T& operator*() {
			return (*value);
		}
		T* get() {
			return value.get();
		}

		const T* operator->() const {
			return value.get();
		}
		T* operator->() {
			return value.get();
		}

		/// move the value out (sugar for std::move(*CopyablePtr))
		T&& move() {
			return std::move(value);
		}

		// If you want these you're probably doing it wrong, though they'd be
		// easy enough to implement
		CopyablePtr& operator=(CopyablePtr const&) = delete;
		CopyablePtr& operator=(CopyablePtr&&) = delete;

	private:
		mutable std::unique_ptr<T> value;
	};

	/// Make a CopyablePtr from the argument. Because the name "makeCopyablePtr"
	/// is already quite transparent in its intent, this will work for lvalues as
	/// if you had wrapped them in std::move.
	template <class T, class T0 = typename std::remove_reference<T>::type>
	CopyablePtr<T0> makeCopyablePtr(T&& t) {
		return CopyablePtr<T0>(std::forward<T0>(t));
	}

} // namespace folly

#endif
