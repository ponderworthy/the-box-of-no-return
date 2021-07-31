/*
 * Copyright (c) 2014 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef LS_REF_H
#define LS_REF_H

#include <set>
#include <stdio.h>

// You may enable this while developing or at least when you encounter any kind
// of crashes or other misbehaviors in conjunction with Ref class guarded code.
// Enabling the following macro will add a bunch of sanity checks for easing
// debugging of such issues, however it comes with the cost that everything will
// be much slower.
#define LS_REF_ASSERT_MODE 0

#if LS_REF_ASSERT_MODE
# warning LS_REF_ASSERT_MODE is enabled which will decrease runtime efficiency!
#endif

// Enable this for VERY verbose debug messages for debbugging deep issues with
// Ref class.
#define LS_REF_VERBOSE_DEBUG_MSG 0

#if LS_REF_ASSERT_MODE
# include <assert.h>
#endif

namespace LinuxSampler {

    //TODO: make reference count increment/decrement thread safe

    template<typename T, typename T_BASE> class Ref;

    extern std::set<void*> _allRefPtrs;

    /**
     * Exists just for implementation detail purpose, you cannot use it
     * directly. Use its derived template class Ref instead.
     *
     * @see Ref
     */
    template<typename T_BASE>
    class RefBase {
    public:
        template<typename T_BASE1>
        class _RefCounter {
        public:
            _RefCounter(T_BASE1* p, int refs) :
                references(refs), ptr(p)
            {
                #if LS_REF_VERBOSE_DEBUG_MSG
                printf("Ref 0x%lx: new counter (refs=%d)\n", (long long)ptr, references);
                #endif
                #if LS_REF_ASSERT_MODE
                assert(p);
                assert(refs > 0);
                assert(!_allRefPtrs.count(p));
                _allRefPtrs.insert(p);
                #endif
            }

            virtual ~_RefCounter() {
                #if LS_REF_VERBOSE_DEBUG_MSG
                printf("Ref 0x%lx: counter destructor (refs=%d)\n", (long long)ptr, references);
                #endif
                fflush(stdout);
            }

            void retain() {
                references++;
                #if LS_REF_VERBOSE_DEBUG_MSG
                printf("Ref 0x%lx: retain (refs=%d)\n", (long long)ptr, references);
                #endif
            }

            void release() {
                if (!references) return;
                references--;
                #if LS_REF_VERBOSE_DEBUG_MSG
                printf("Ref 0x%lx: release (refs=%d)\n", (long long)ptr, references);
                #endif
                if (!references) deletePtr();
            }
        //protected:
            void deletePtr() {
                #if LS_REF_VERBOSE_DEBUG_MSG
                printf("RefCounter 0x%lx: deletePtr() (refs=%d)\n", (long long)ptr, references);
                #endif
                #if LS_REF_ASSERT_MODE
                assert(!references);
                _allRefPtrs.erase(ptr);
                #endif
                delete ptr;
                delete this;
            }

            int references;
            T_BASE1* ptr;
            //friend class ... todo
        };
        typedef _RefCounter<T_BASE> RefCounter;

        virtual ~RefBase() {
            if (refCounter) refCounter->release();
            refCounter = NULL;
        }

    //protected:
        RefCounter* refCounter;
        //friend class Ref<T_BASE, T_BASE>;

    protected:
        RefBase() : refCounter(NULL) {
            #if LS_REF_VERBOSE_DEBUG_MSG
            printf("(RefBase empty ctor)\n");
            #endif
        }
/*
        RefBase(RefCounter* rc) {
            refCounter = rc;
        }

        RefBase(const RefBase& r) {
            refCounter = r.refCounter;
            if (refCounter) refCounter->retain();
        }
*/
    };

    /**
     * Replicates a std::shared_ptr template class, to avoid a build requirement
     * of having a C++11 compliant compiler (std::shared_ptr was not part of the
     * C++03 standard).
     *
     * In contrast to the STL implementation though this implementation here
     * also supports copying references of derived, different types (in a type
     * safe manner). You can achieve that by providing a second template
     * argument (which is optional), for declaring a common subtype. For example
     * the following code would not compile:
     * @code
     * void example(UILabel* pLabel) {
     *     Ref<UILabel> lbl = pLabel;
     *     Ref<UIWidget> w = lbl; // compile error, incompatible Ref types
     *     w->resize(16,300);
     * }
     * @endcode
     * Whereas the following would work:
     * @code
     * void example(UILabel* pLabel) {
     *     Ref<UILabel,UIWidget> lbl = pLabel;
     *     Ref<UIWidget> w = lbl; // works (assuming that UILabel is a subclass of UIWidget)
     *     w->resize(16,300);
     * }
     * @endcode
     * Like the STL's std::shared_ptr, this class also emulates raw pointer
     * access and operators. With one addition: if used in the derived common
     * subtype manner as shown above, access to the actual data and boolean
     * operator will also check whether the underlying pointer (of the common
     * subclass) can actually be casted safely to the objects main type (first
     * template argument of this class). For example:
     * @code
     * void example(UILabel* pLabel) { // assuming pLabel is not NULL ...
     *     Ref<UILabel,UIWidget> lbl = pLabel;
     *     Ref<UIDialog,UIWidget> dlg = lbl;
     *     bool b1 = lbl; // will be true (assuming pLabel was not NULL)
     *     bool b2 = dlg; // will be false (assuming that UIDialog is not derived from UILabel)
     *     lbl->setText("foo"); // works
     *     dlg->showModal(); // would crash with -> operator providing a NULL pointer
     * }
     * @endcode
     * Like with std::shared_ptr you must be @b very cautious that you
     * initialize only one Ref class object directly with the same raw pointer.
     * If you forget this fundamental rule somewhere, your code will crash!
     * @code
     * UIWidget* ptr = new UIWidget();
     * Ref<UIWidget> w1 = ptr;
     * Ref<UIWidget> w2 = w1;  // this is OK, copy from a Ref object
     * Ref<UIWidget> w3 = ptr; // illegal! 2nd direct init from same raw pointer. This will crash!
     * @endcode
     * It would be possible to write an implementation of the Ref class that
     * could handle the case above as well without crashing, however it would be
     * too slow for practice. Because it would require a global lookup table
     * maintaining all memory pointers which are currently already guarded by
     * this class. Plus it would need an expensive synchronization to prevent
     * concurrent access on that global lookup table.
     */
    template<typename T, typename T_BASE = T>
    class Ref : public RefBase<T_BASE> {
    public:
        typedef RefBase<T_BASE> RefBaseT;
        typedef typename RefBase<T_BASE>::RefCounter RefCounter;
        
        Ref() : RefBaseT() {
            #if LS_REF_VERBOSE_DEBUG_MSG
            printf("Ref empty ctor Ref:0x%lx\n", (long long)this);
            #endif
        }

        Ref(const T_BASE* p) : RefBaseT() {
            #if LS_REF_VERBOSE_DEBUG_MSG
            printf("Ref base ptr ctor Ref:0x%lx <- p:0x%lx\n", (long long)this, (long long)p);
            #endif
            RefBaseT::refCounter = p ? new RefCounter((T_BASE*)p, 1) : NULL;
        }

        Ref(const T* p) : RefBaseT() {
            #if LS_REF_VERBOSE_DEBUG_MSG
            printf("Ref main ptr ctor Ref:0x%lx <- p:0x%lx\n", (long long)this, (long long)p);
            #endif
            RefBaseT::refCounter = p ? new RefCounter((T*)p, 1) : NULL;
        }

        Ref(const RefBaseT& r) : RefBaseT() {
            #if LS_REF_VERBOSE_DEBUG_MSG
            printf("Ref base ref ctor Ref:0x%lx <- Ref:0x%lx\n", (long long)this, (long long)&r);
            #endif
            RefBaseT::refCounter = r.refCounter;
            if (RefBaseT::refCounter)
                RefBaseT::refCounter->retain();
        }

        Ref(const Ref& r) : RefBaseT() {
            #if LS_REF_VERBOSE_DEBUG_MSG
            printf("Ref main ref ctor Ref:0x%lx <- Ref:0x%lx\n", (long long)this, (long long)&r);
            #endif
            RefBaseT::refCounter = r.refCounter;
            if (RefBaseT::refCounter)
                RefBaseT::refCounter->retain();
        }

        inline T* operator->() {
            return dynamic_cast<T*>( RefBaseT::refCounter->ptr );
        }

        inline const T* operator->() const {
            return dynamic_cast<const T*>( RefBaseT::refCounter->ptr );
        }

        inline T& operator*() {
            return *dynamic_cast<T*>( RefBaseT::refCounter->ptr );
        }

        inline const T& operator*() const {
            return *dynamic_cast<const T*>( RefBaseT::refCounter->ptr );
        }

        inline bool operator==(const RefBaseT& other) const {
            return RefBaseT::refCounter == other.refCounter;
        }

        inline bool operator!=(const RefBaseT& other) const {
            return RefBaseT::refCounter != other.refCounter;
        }

        inline operator bool() const {
            return RefBaseT::refCounter && RefBaseT::refCounter->ptr &&
                   dynamic_cast<const T*>( RefBaseT::refCounter->ptr );
        }

        inline bool operator!() const {
            return !( RefBaseT::refCounter && RefBaseT::refCounter->ptr &&
                      dynamic_cast<const T*>( RefBaseT::refCounter->ptr ) );
        }

/*
        inline operator RefBaseT&() {
            return *this;
        }
        
        inline operator const RefBaseT&() const {
            return *this;
        }
*/
        inline bool isEquivalent(const RefBaseT& other) const {
            if (static_cast<const RefBaseT*>(this) == &other)
                return true;
            return (RefBaseT::refCounter == other.refCounter);
        }

        inline bool isEquivalent(const T_BASE* const other) const {
            if (!other) return !RefBaseT::refCounter;
            if (!RefBaseT::refCounter) return false;
            return other == RefBaseT::refCounter->ptr;
        }

        Ref<T,T_BASE>& operator=(const RefBaseT& other) {
            #if LS_REF_VERBOSE_DEBUG_MSG
            printf("Ref base ref assignment Ref:0x%lx <- Ref:0x%lx\n", (long long)this, (long long)&other);
            #endif
            if (isEquivalent(other)) {
                #if LS_REF_VERBOSE_DEBUG_MSG
                printf("Ref 0x%lx WRN: equivalent ref assignment ignored.\n", (long long)this);
                #endif
                return *this;
            }
            if (RefBaseT::refCounter) {
                RefBaseT::refCounter->release();
                RefBaseT::refCounter = NULL;
            }
            RefBaseT::refCounter = other.refCounter;
            if (RefBaseT::refCounter)
                RefBaseT::refCounter->retain();
            return *this;
        }

        Ref<T,T_BASE>& operator=(const Ref<T,T_BASE>& other) {
            #if LS_REF_VERBOSE_DEBUG_MSG
            printf("Ref main ref assignment Ref:0x%lx <- Ref:0x%lx\n", (long long)this, (long long)&other);
            #endif
            if (isEquivalent(other)) {
                #if LS_REF_VERBOSE_DEBUG_MSG
                printf("Ref 0x%lx WRN: equivalent ref assignment ignored.\n", (long long)this);
                #endif
                return *this;
            }
            if (RefBaseT::refCounter) {
                RefBaseT::refCounter->release();
                RefBaseT::refCounter = NULL;
            }
            RefBaseT::refCounter = other.refCounter;
            if (RefBaseT::refCounter)
                RefBaseT::refCounter->retain();
            return *this;
        }

        Ref<T,T_BASE>& operator=(const T* p) {
            #if LS_REF_VERBOSE_DEBUG_MSG
            printf("Ref main ptr assignment Ref:0x%lx <- p:0x%lx\n", (long long)this, p);
            #endif
            if (isEquivalent(p)) {
                #if LS_REF_VERBOSE_DEBUG_MSG
                printf("Ref 0x%lx WRN: equivalent ptr assignment ignored.\n", (long long)this);
                #endif
                return *this;
            }
            if (RefBaseT::refCounter) {
                RefBaseT::refCounter->release();
                RefBaseT::refCounter = NULL;
            }
            RefBaseT::refCounter = p ? new RefCounter((T*)p, 1) : NULL;
            #if LS_REF_VERBOSE_DEBUG_MSG
            printf("Ref main ptr assignment done\n");
            #endif
            return *this;
        }

        Ref<T,T_BASE>& operator=(const T_BASE* p) {
            #if LS_REF_VERBOSE_DEBUG_MSG
            printf("Ref base ptr assignment Ref:0x%lx <- p:0x%lx\n", (long long)this, p);
            #endif
            if (isEquivalent(p)) {
                #if LS_REF_VERBOSE_DEBUG_MSG
                printf("Ref 0x%lx WRN: equivalent ptr assignment ignored.\n", (long long)this);
                #endif
                return *this;
            }
            if (RefBaseT::refCounter) {
                RefBaseT::refCounter->release();
                RefBaseT::refCounter = NULL;
            }
            RefBaseT::refCounter = p ? new RefCounter((T*)p, 1) : NULL;
            return *this;
        }
    };

} // namespace LinuxSampler

#endif // LS_REF_H
