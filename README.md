## Motivation

pthread_mutex_t and std::mutex take up 40 bytes of memory. You could imagine a situation, where there are lots of mutexes and, hence, every byte counts.


"Turnstiles" are an example of how one can lower that memory footprint. This idea is used internally in NetBSD and Solaris among others. It hinges on the fact that a single thread cannot sleep on more than one mutex at a time. This fact allows all mutexes to be just placeholders. These placeholders are allocated a state (a "turnstile" in this terminology) on demand - when there is contention and one of the threads has to be suspended. The turnstile is held by the mutex until no more threads need to wait on it. This way, the total number of turnstiles used simultaneously is going to always be lower or equal to the number of threads. This allows for an implementation involving a global pool of turnstiles. A mutex grabs a turnstile from the pool when a thread needs to be suspended and releases the turnstile when no more threads need to sleep on this mutex.

You can find more detailed information about turnstiles online or you can figure it out yourself.
