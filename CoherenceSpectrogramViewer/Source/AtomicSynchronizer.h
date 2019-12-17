/*
------------------------------------------------------------------

This file is part of a plugin for the Open Ephys GUI
Copyright (C) 2018 Translational NeuroEngineering Laboratory

------------------------------------------------------------------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef ATOMIC_SYNCHRONIZER_H_INCLUDED
#define ATOMIC_SYNCHRONIZER_H_INCLUDED

#include <atomic>
#include <vector>
#include <utility>
#include <cassert>
#include <cstdlib>
#include <functional>

/*
* The purpose of AtomicSynchronizer is to allow one "writer" thread to continally
* update some arbitrary piece of information and one "reader" thread to retrieve
* the latest version of that information that has been "pushed" by the writer,
* without either thread having to wait to acquire a mutex or allocate memory
* on the heap (aside from internal allocations depending on the data structure used).
* For one reader and one writer, this requires three instances of whatever data type is
* being shared to be allocated upfront. During operation, "pushing" from the writer and
* "pulling" to the reader are accomplished by exchanging atomic indices between slots
* indicating what each instance is to be used for, rather than any actual copying or allocation.
*
* There are two interfaces to choose from:
*
*  - In most cases, the AtomicSynchronizer logic can be wrapped together with data
*    allocation and lifetime management by using the AtomicallyShared<T> class template.
*    Here, T is the type of data that needs to be exchanged.
*
*      * The constructor for an AtomicallyShared<T> simply takes whatever arguments
*        would be used to construct each T object and constructs 3 copies. (If a constructor
*        with move semantics would be used, this is called for one of the 3 copies, and the
*        other 2 use copying constructors instead.)
*
*      * Any configuration that applies to all 3 copies can by done by calling the "apply" method
*        with a function pointer or lambda, as long as there are no active readers or writers.
*
*      * To write, construct an AtomicScopedWritePtr<T> with the AtomicallyShared<T>&
*        as an argument. This can be used as a normal pointer. It can be acquired, written to,
*        and released multiple times and will keep referring to the same instance until the
*        pushUpdate() method is called, at which point this instance is released for reading
*        and a new one is acquired for writing (which might need to be cleared/reset first).
*
*      * To read, construct an AtomicScopedReadPtr<T> with the AtomicallyShared<T>& as
*        an argument. This can be used as a normal pointer, and if you want to get the
*        latest update from the writer without destroying the read ptr and constructing
*        a new one, you can call the pullUpdate() method.
*
*      * If you attempt to create two write pointers to the same AtomicallyShared object, the
*        second one will be effectively null; you can check for this with the isValid() method
*        (if isValid() ever returns false, this should be considered a bug). The same is true
*        of read pointers, except that a read pointer acquired before any writes have occurred
*        is also invalid (since there's nothing to read), so the isValid() check is necessary
*        even if you know for sure there is only ever one read pointer at once.
*
*      * The hasUpdate() method on an AtomicallyShared<T> returns true if there is new data
*        from the writer that has not been read yet. After a call to hasUpdate() returns
*        true, the next constructed read ptr (if none currently exist) or current read ptr
*        after a subsequent call to pullUpdate() is guaranteed to be valid.
*
*      * The reset() method brings you back to the state where no writes have been performed yet.
*        Must be called when no read or write pointers exist.
*
*  - Using an AtomicSynchronizer directly works similarly; the main difference is that you
*    are responsible for allocating and accessing the data, and the AtomicSynchronizer just
*    tells you which index to use (0, 1, or 2) as the reader or writer.
*
*      * To write, use an AtomiSynchronizer::ScopedWriteIndex instead of an AtomicScopedWritePtr.
*        This can be converted to int to use directly as an index, and has a pushUpdate() method
*        that works the same way as for the write pointer. The index can be -1 if you try to
*        create two write indices to the same synchronizer.
*
*      * To read, use an AtomicSynchronizer::ScopedReadIndex instead of an AtomicScopedReadPtr.
*        This works how you would expect and also has a pullUpdate() method. Remember to check
*        whether it is valid before using if you're not using hasUpdate().
*
*      * AtomicSynchronizer has hasUpdate() and reset() methods as well.
*
*      * ScopedLockout is just a try-lock for both readers and writers; it will be "valid"
*        iff no read or write indices exist at the point of construction. By constructing
*        one of these and proceeding only if it is valid, you can make changes to each data
*        instance outside of the reader/writer framework (instead of AtomicallyShared<T>::apply).
*
*/

class AtomicSynchronizer {

public:
    class ScopedWriteIndex
    {
    public:
        explicit ScopedWriteIndex(AtomicSynchronizer& o)
            : owner(&o)
            , valid(o.checkoutWriter())
        {
            if (!valid)
            {
                // just to be sure - if not valid, shouldn't be able to access the synchronizer
                owner = nullptr;
            }
        }

        ScopedWriteIndex(const ScopedWriteIndex&) = delete;
        ScopedWriteIndex& operator=(const ScopedWriteIndex&) = delete;

        ~ScopedWriteIndex()
        {
            if (valid)
            {
                owner->returnWriter();
            }
        }

        // push a write to the reader without releasing writer privileges
        void pushUpdate()
        {
            if (valid)
            {
                owner->pushWrite();
            }
        }

        operator int() const
        {
            if (valid)
            {
                return owner->writerIndex;
            }
            return -1;
        }

        bool isValid() const
        {
            return valid;
        }

    private:
        AtomicSynchronizer* owner;
        const bool valid;
    };


    class ScopedReadIndex
    {
    public:
        explicit ScopedReadIndex(AtomicSynchronizer& o)
            : owner(&o)
            , valid(o.checkoutReader())
        {
            if (valid)
            {
                owner->updateReaderIndex();
            }
            else
            {
                // just to be sure - if not valid, shouldn't be able to access the synchronizer
                owner = nullptr;
            }
        }

        ScopedReadIndex(const ScopedReadIndex&) = delete;
        ScopedReadIndex& operator=(const ScopedReadIndex&) = delete;

        ~ScopedReadIndex()
        {
            if (valid)
            {
                owner->returnReader();
            }
        }

        // update the index, if a new version is available
        void pullUpdate()
        {
            if (valid)
            {
                owner->updateReaderIndex();
            }
        }

        operator int() const
        {
            if (valid)
            {
                return owner->readerIndex;
            }
            return -1;
        }

        bool isValid() const
        {
            return valid;
        }

    private:
        AtomicSynchronizer* owner;
        const bool valid;
    };


    // Registers as both a reader and a writer, so no other reader or writer
    // can exist while it's held. Use to access all the underlying data without
    // conern for who has access to what, e.g. for updating settings, resizing, etc.
    class ScopedLockout
    {
    public:
        explicit ScopedLockout(AtomicSynchronizer& o)
            : owner(&o)
            , hasReadLock(o.checkoutReader())
            , hasWriteLock(o.checkoutWriter())
            , valid(hasReadLock && hasWriteLock)
        {}

        ~ScopedLockout()
        {
            if (hasReadLock)
            {
                owner->returnReader();
            }

            if (hasWriteLock)
            {
                owner->returnWriter();
            }
        }

        bool isValid() const
        {
            return valid;
        }

    private:
        AtomicSynchronizer* owner;
        const bool hasReadLock;
        const bool hasWriteLock;
        const bool valid;
    };


    AtomicSynchronizer()
        : nReaders(0)
        , nWriters(0)
    {
        reset();
    }

    AtomicSynchronizer(const AtomicSynchronizer&) = delete;
    AtomicSynchronizer& operator=(const AtomicSynchronizer&) = delete;

    // Reset to state with no valid object
    // No readers or writers should be active when this is called!
    // If it does fail due to existing readers or writers, returns false
    bool reset()
    {
        ScopedLockout lock(*this);
        if (!lock.isValid())
        {
            return false;
        }

        readyToReadIndex = -1;
        readyToWriteIndex = 0;
        readyToWriteIndex2 = 1;
        writerIndex = 2;
        readerIndex = -1;

        return true;
    }

    bool hasUpdate() const
    {
        return readyToReadIndex != -1;
    }

private:

    // Registers a writer and updates the writer index. If a writer already exists,
    // returns false, else returns true. returnWriter should be called to release.
    bool checkoutWriter()
    {
        // ensure there is not already a writer
        int currWriters = 0;
        if (!nWriters.compare_exchange_strong(currWriters, 1, std::memory_order_relaxed))
        {
            return false;
        }

        return true;
    }

    void returnWriter()
    {
        nWriters = 0;
    }

    // Registers a reader and updates the reader index. If a reader already exists,
    // returns false, else returns true. returnReader should be called to release.
    bool checkoutReader()
    {
        // ensure there is not already a reader
        int currReaders = 0;
        if (!nReaders.compare_exchange_strong(currReaders, 1, std::memory_order_relaxed))
        {
            return false;
        }

        return true;
    }

    void returnReader()
    {
        nReaders = 0;
    }

    // should only be called by a writer
    void pushWrite()
    {
        // It's an invariant that writerIndex != -1
        // except within this method, and this method is not reentrant.
        assert(writerIndex != -1);

        writerIndex = readyToReadIndex.exchange(writerIndex, std::memory_order_relaxed);

        if (writerIndex == -1)
        {
            // attempt to pull an index from readyToWriteIndex
            writerIndex = readyToWriteIndex.exchange(-1, std::memory_order_relaxed);

            if (writerIndex == -1)
            {
                writerIndex = readyToWriteIndex2.exchange(-1, std::memory_order_relaxed);
            }
        }

        // There are only 5 slots, so writerIndex, readyToWriteIndex, and
        // readyToWriteIndex2 cannot all be empty. There can't be a race condition
        // where one of these slots is now nonempty, because only the writer can
        // set any of them to -1 (and there's only one writer).
        assert(writerIndex != -1);
    }

    // should only be called by a reader
    void updateReaderIndex()
    {
        // Check readyToReadIndex for newly pushed update
        // It can still be updated after checking, but it cannot be emptied because the 
        // writer cannot push -1 to readyToReadIndex.
        if (readyToReadIndex != -1)
        {
            if (readerIndex != -1)
            {
                // Great, there's a new update, first have to put current
                // readerIndex somewhere though.

                // Attempt to put index into readyToWriteIndex
                int expected = -1;
                if (!readyToWriteIndex.compare_exchange_strong(expected, readerIndex,
                    std::memory_order_relaxed))
                {
                    // readyToWriteIndex is already occupied
                    // readyToWriteIndex2 must be free at this point. newIndex, readerIndex, and
                    // readyToWriteIndex all contain something.
                    readyToWriteIndex2.exchange(readerIndex, std::memory_order_relaxed);
                }
            }
            readerIndex = readyToReadIndex.exchange(-1, std::memory_order_relaxed);
        }
    }

    // shared indices
    std::atomic<int> readyToReadIndex;  // assigned by the writer; can be read by the reader
    std::atomic<int> readyToWriteIndex; // assigned by the reader; can by modified by the writer
    std::atomic<int> readyToWriteIndex2; // another slot similar to readyToWriteIndex

    int writerIndex; // index the writer may currently be writing to
    int readerIndex; // index the reader may currently be reading from

    std::atomic<int> nWriters;
    std::atomic<int> nReaders;
};


// class to actually hold data controlled by an AtomicSynchronizer
template<typename T>
class AtomicallyShared
{
public:
    template<typename... Args>
    AtomicallyShared(Args&&... args)
    {
        for (int i = 0; i < 2; ++i)
        {
            data.emplace_back(args...);
        }

        // move into the last entry, if possible
        data.emplace_back(std::forward<Args>(args)...);
    }

    bool reset()
    {
        return sync.reset();
    }

    // Call a function on each underlying data member.
    // Requires that no readers or writers exist. Returns false if
    // this condition is unmet, true otherwise.
    bool map(std::function<void(T&)> f)
    {
        AtomicSynchronizer::ScopedLockout lock(sync);
        if (!lock.isValid())
        {
            return false;
        }

        for (T& obj : data)
        {
            f(obj);
        }

        return true;
    }

    bool hasUpdate() const
    {
        return sync.hasUpdate();
    }


    class ScopedWritePtr
    {
    public:
        ScopedWritePtr(AtomicallyShared<T>& o)
            : owner(&o)
            , ind(o.sync)
            , valid(ind.isValid())
        {}

        void pushUpdate()
        {
            ind.pushUpdate();
        }

        // provide access to data

        T& operator*()
        {
            if (!valid)
            {
                // abort! abort!
                assert(false);
                std::abort();
            }
            return owner->data[ind];
        }

        T* operator->()
        {
            return &(operator*());
        }

        bool isValid() const
        {
            return valid;
        }

    private:
        AtomicallyShared<T>* owner;
        AtomicSynchronizer::ScopedWriteIndex ind;
        const bool valid;
    };

    class ScopedReadPtr
    {
    public:
        ScopedReadPtr(AtomicallyShared<T>& o)
            : owner(&o)
            , ind(o.sync)
            // if the ind is valid, but is equal to -1, this pointer is still invalid (for now)
            , valid(ind != -1)
        {}

        void pullUpdate()
        {
            ind.pullUpdate();
            // in case ind is valid but was equal to -1:
            valid = ind != -1;
        }

        // provide access to data

        const T& operator*()
        {
            if (!valid)
            {
                // abort! abort!
                assert(false);
                std::abort();
            }
            return owner->data[ind];
        }

        const T* operator->()
        {
            return &(operator*());
        }

        bool isValid() const
        {
            return valid;
        }

    private:
        AtomicallyShared<T>* owner;
        AtomicSynchronizer::ScopedReadIndex ind;
        bool valid;
    };

private:
    std::vector<T> data;
    AtomicSynchronizer sync;
};

template<typename T>
using AtomicScopedWritePtr = typename AtomicallyShared<T>::ScopedWritePtr;

template<typename T>
using AtomicScopedReadPtr = typename AtomicallyShared<T>::ScopedReadPtr;

#endif // ATOMIC_SYNCHRONIZER_H_INCLUDED
