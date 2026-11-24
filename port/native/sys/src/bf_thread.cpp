// Threads with Win32-like handles, and a recursive mutex.

#include "bf_sys.h"
#include "bf_trace.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/qos.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

struct bf_sys_thread {
	bf_sys_thread_fn fn;
	void *argument;
	char name[64];
	std::mutex lock;
	std::condition_variable doneSignal;
	bool done;
	uint32_t result;
	std::atomic<int> references;   // the creator's handle and the running thread
};

struct bf_sys_mutex {
	pthread_mutex_t mutex;
};

namespace {

void Release(bf_sys_thread *thread)
{
	if (thread->references.fetch_sub(1, std::memory_order_acq_rel) == 1)
		delete thread;
}

void *Entry(void *raw)
{
	bf_sys_thread *thread = (bf_sys_thread *)raw;
	if (thread->name[0])
		pthread_setname_np(thread->name);
	uint32_t result = thread->fn(thread->argument);
	{
		std::lock_guard<std::mutex> guard(thread->lock);
		thread->result = result;
		thread->done = true;
	}
	thread->doneSignal.notify_all();
	Release(thread);
	return NULL;
}

} // namespace

extern "C" bf_sys_thread *bf_sys_thread_create(bf_sys_thread_fn fn, void *argument, const char *name)
{
	if (!fn)
		return NULL;
	bf_sys_thread *thread = new bf_sys_thread;
	thread->fn = fn;
	thread->argument = argument;
	thread->name[0] = 0;
	if (name)
		strlcpy(thread->name, name, sizeof(thread->name));
	thread->done = false;
	thread->result = 0;
	thread->references.store(2);

	pthread_attr_t attributes;
	pthread_attr_init(&attributes);
	pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
	// Win32 gives a thread 1 MB of stack; the client's loaders recurse.
	pthread_attr_setstacksize(&attributes, 1 << 20);
	pthread_t id;
	int error = pthread_create(&id, &attributes, Entry, thread);
	pthread_attr_destroy(&attributes);
	if (error) {
		bf_tracef(BFT_ERR, "sys: pthread_create failed: %s", strerror(error));
		delete thread;
		return NULL;
	}
	return thread;
}

extern "C" int bf_sys_thread_wait(bf_sys_thread *thread, uint32_t timeout_ms, uint32_t *result)
{
	if (!thread)
		return 0;
	std::unique_lock<std::mutex> guard(thread->lock);
	if (timeout_ms == BF_SYS_WAIT_FOREVER) {
		thread->doneSignal.wait(guard, [thread] { return thread->done; });
	} else if (!thread->doneSignal.wait_for(guard, std::chrono::milliseconds(timeout_ms),
	                                        [thread] { return thread->done; })) {
		return 0;
	}
	if (result)
		*result = thread->result;
	return 1;
}

extern "C" void bf_sys_thread_release(bf_sys_thread *thread)
{
	if (thread)
		Release(thread);
}

extern "C" uint32_t bf_sys_thread_join(bf_sys_thread *thread)
{
	uint32_t result = 0;
	if (thread) {
		bf_sys_thread_wait(thread, BF_SYS_WAIT_FOREVER, &result);
		Release(thread);
	}
	return result;
}

extern "C" uint64_t bf_sys_thread_id(void)
{
	uint64_t id = 0;
	pthread_threadid_np(NULL, &id);
	return id ? id : 1;
}

extern "C" int bf_sys_is_main_thread(void)
{
	return pthread_main_np() ? 1 : 0;
}

extern "C" void bf_sys_thread_set_priority(int priority)
{
	qos_class_t qos;
	if (priority >= 2)       qos = QOS_CLASS_USER_INTERACTIVE;
	else if (priority == 1)  qos = QOS_CLASS_USER_INITIATED;
	else if (priority == 0)  qos = QOS_CLASS_DEFAULT;
	else if (priority == -1) qos = QOS_CLASS_UTILITY;
	else                     qos = QOS_CLASS_BACKGROUND;
	pthread_set_qos_class_self_np(qos, 0);
}

extern "C" bf_sys_mutex *bf_sys_mutex_create(void)
{
	bf_sys_mutex *mutex = (bf_sys_mutex *)malloc(sizeof(bf_sys_mutex));
	if (!mutex)
		return NULL;
	pthread_mutexattr_t attributes;
	pthread_mutexattr_init(&attributes);
	pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&mutex->mutex, &attributes);
	pthread_mutexattr_destroy(&attributes);
	return mutex;
}

extern "C" void bf_sys_mutex_lock(bf_sys_mutex *mutex)
{
	pthread_mutex_lock(&mutex->mutex);
}

extern "C" int bf_sys_mutex_try_lock(bf_sys_mutex *mutex)
{
	return pthread_mutex_trylock(&mutex->mutex) == 0 ? 1 : 0;
}

extern "C" void bf_sys_mutex_unlock(bf_sys_mutex *mutex)
{
	pthread_mutex_unlock(&mutex->mutex);
}

extern "C" void bf_sys_mutex_destroy(bf_sys_mutex *mutex)
{
	if (mutex) {
		pthread_mutex_destroy(&mutex->mutex);
		free(mutex);
	}
}

extern "C" const char *bf_sys_app_name(void)
{
	return BF_APP_NAME;
}
