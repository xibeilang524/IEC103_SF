/*
 * Mutex.h
 *
 *  Created on:
 *      Author: Administrator
 */

#ifndef MUTEX_H_
#define MUTEX_H_
#include <pthread.h>
#include <stdlib.h>

typedef pthread_mutex_t MutexType;

class Mutex {
public:
    // Create a Mutex that is not held by anybody.  This constructor is
    // typically used for Mutexes allocated on the heap or the stack.
    // See below for a recommendation for constructing global Mutex
    // objects.
    inline Mutex();

    // Destructor
    inline ~Mutex();

    inline void Lock();    // Block if needed until free then acquire exclusively
    inline void Unlock();  // Release a lock acquired via Lock()
    inline bool TryLock(); // If free, Lock() and return true, else return false
    // Note that on systems that don't support read-write locks, these may
    // be implemented as synonyms to Lock() and Unlock().  So you can use
    // these for efficiency, but don't use them anyplace where being able
    // to do shared reads is necessary to avoid deadlock.
    inline void ReaderLock();   // Block until free or shared then acquire a share
    inline void ReaderUnlock(); // Release a read share of this Mutex
    inline void WriterLock() { Lock(); }     // Acquire an exclusive lock
    inline void WriterUnlock() { Unlock(); } // Release a lock from WriterLock()

    // TODO(hamaji): Do nothing, implement correctly.
    inline void AssertHeld() {}
private:
    MutexType mutex_;
    // We want to make sure that the compiler sets is_safe_ to true only
    // when we tell it to, and never makes assumptions is_safe_ is
    // always true.  volatile is the most reliable way to do that.
    volatile bool is_safe_;

    inline void SetIsSafe() { is_safe_ = true; }

    // Catch the error of writing Mutex when intending MutexLock.
    Mutex(Mutex* /*ignored*/) {}
    // Disallow "evil" constructors
    Mutex(const Mutex&);
    void operator=(const Mutex&);
};
#define SAFE_PTHREAD(fncall)  do {   /* run fncall if is_safe_ is true */  \
    if (is_safe_ && fncall(&mutex_) != 0) abort();                           \
    } while (0)

    Mutex::Mutex()             {
        SetIsSafe();
        if (is_safe_ && pthread_mutex_init(&mutex_, NULL) != 0) abort();
    }
    Mutex::~Mutex()            { SAFE_PTHREAD(pthread_mutex_destroy); }
    void Mutex::Lock()         { SAFE_PTHREAD(pthread_mutex_lock); }
    void Mutex::Unlock()       { SAFE_PTHREAD(pthread_mutex_unlock); }
    bool Mutex::TryLock()      { return is_safe_ ?
        pthread_mutex_trylock(&mutex_) == 0 : true; }
    void Mutex::ReaderLock()   { Lock(); }
    void Mutex::ReaderUnlock() { Unlock(); }
    class MutexLock {
    public:
        explicit MutexLock(Mutex *mu) : mu_(mu) { mu_->Lock(); }
        ~MutexLock() { mu_->Unlock(); }
    private:
        Mutex * const mu_;
        // Disallow "evil" constructors
        MutexLock(const MutexLock&);
        void operator=(const MutexLock&);
    };

    // ReaderMutexLock and WriterMutexLock do the same, for rwlocks
    class ReaderMutexLock {
    public:
        explicit ReaderMutexLock(Mutex *mu) : mu_(mu) { mu_->ReaderLock(); }
        ~ReaderMutexLock() { mu_->ReaderUnlock(); }
    private:
        Mutex * const mu_;
        // Disallow "evil" constructors
        ReaderMutexLock(const ReaderMutexLock&);
        void operator=(const ReaderMutexLock&);
    };

    class WriterMutexLock {
    public:
        explicit WriterMutexLock(Mutex *mu) : mu_(mu) { mu_->WriterLock(); }
        ~WriterMutexLock() { mu_->WriterUnlock(); }
    private:
        Mutex * const mu_;
        // Disallow "evil" constructors
        WriterMutexLock(const WriterMutexLock&);
        void operator=(const WriterMutexLock&);
    };

#endif /* MUTEX_H_ */
