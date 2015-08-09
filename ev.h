/*
 * libev native API header base on 4.20
 * modifyed by xzc 2015-08-09
 */

#ifndef EV_H_
#define EV_H_

#ifdef __cplusplus
# define EV_CPP(x) x
# if __cplusplus >= 201103L
#  define EV_THROW noexcept
# else
#  define EV_THROW throw ()
# endif
#else
# define EV_CPP(x)
# define EV_THROW
#endif

EV_CPP(extern "C" {)

/*****************************************************************************/

typedef double ev_tstamp;

//TODO need to delete
# define EV_P void
# define EV_P_
# define EV_A
# define EV_A_
# define EV_DEFAULT
# define EV_DEFAULT_
# define EV_DEFAULT_UC
# define EV_DEFAULT_UC_

/* EV_INLINE is used for functions in header files */
#if __STDC_VERSION__ >= 199901L || __GNUC__ >= 3
# define EV_INLINE static inline
#else
# define EV_INLINE static
#endif

//TODO remain unknow
#ifdef EV_API_STATIC
# define EV_API_DECL static
#else
# define EV_API_DECL extern
#endif

/*****************************************************************************/

#define EV_VERSION_MAJOR 4
#define EV_VERSION_MINOR 20

/* eventmask, revents, events... */
enum {
  EV_UNDEF    = (int)0xFFFFFFFF, /* guaranteed to be invalid */
  EV_NONE     =            0x00, /* no events */
  EV_READ     =            0x01, /* ev_io detected read will not block */
  EV_WRITE    =            0x02, /* ev_io detected write will not block */
  EV__IOFDSET =            0x80, /* internal use only */
  EV_IO       =         EV_READ, /* alias for type-detection */
  EV_TIMER    =      0x00000100, /* timer timed out */
  EV_CUSTOM   =      0x01000000, /* for use by user code */
  EV_ERROR    = (int)0x80000000  /* sent when an error occurs */
};

#ifndef EV_CB_DECLARE
# define EV_CB_DECLARE(type) void (*cb)(EV_P_ struct type *w, int revents);
#endif
#ifndef EV_CB_INVOKE
# define EV_CB_INVOKE(watcher,revents) (watcher)->cb (EV_A_ (watcher), (revents))
#endif

/*
 * struct member types:
 * private: you may look at them, but not change them,
 *          and they might not mean anything to you.
 * ro: can be read anytime, but only changed when the watcher isn't active.
 * rw: can be read and modified anytime, even when the watcher is active.
 *
 * some internal details that might be helpful for debugging:
 *
 * active is either 0, which means the watcher is not active,
 *           or the array index of the watcher (periodics, timers)
 *           or the array index + 1 (most other watchers)
 *           or simply 1 for watchers that aren't in some array.
 * pending is either 0, in which case the watcher isn't,
 *           or the array index + 1 in the pendings array.
 */

/* shared by all watchers */
#define EV_WATCHER(type)            \
  int active; /* private */            \
  int pending; /* private */            \
  EV_COMMON /* rw */                \
  EV_CB_DECLARE (type) /* private */

#define EV_WATCHER_LIST(type)            \
  EV_WATCHER (type)                \
  struct ev_watcher_list *next; /* private */

#define EV_WATCHER_TIME(type)            \
  EV_WATCHER (type)                \
  ev_tstamp at;     /* private */

/* base class, nothing to see here unless you subclass */
typedef struct ev_watcher
{
  EV_WATCHER (ev_watcher)
} ev_watcher;

/* base class, nothing to see here unless you subclass */
typedef struct ev_watcher_list
{
  EV_WATCHER_LIST (ev_watcher_list)
} ev_watcher_list;

/* base class, nothing to see here unless you subclass */
typedef struct ev_watcher_time
{
  EV_WATCHER_TIME (ev_watcher_time)
} ev_watcher_time;

/* invoked when fd is either EV_READable or EV_WRITEable */
/* revent EV_READ, EV_WRITE */
typedef struct ev_io
{
  EV_WATCHER_LIST (ev_io)

  int fd;     /* ro */
  int events; /* ro */
} ev_io;

/* invoked after a specific time, repeatable (based on monotonic clock) */
/* revent EV_TIMEOUT */
typedef struct ev_timer
{
  EV_WATCHER_TIME (ev_timer)

  ev_tstamp repeat; /* rw */
} ev_timer;


EV_API_DECL int ev_default_loop (unsigned int flags EV_CPP (= 0)) EV_THROW; /* returns true when successful */

EV_API_DECL ev_tstamp ev_rt_now;

EV_INLINE ev_tstamp
ev_now (void) EV_THROW
{
  return ev_rt_now;
}

/* looks weird, but ev_is_default_loop (EV_A) still works if this exists */
EV_INLINE int
ev_is_default_loop (void) EV_THROW
{
  return 1;
}

/* destroy event loops, also works for the default loop */
EV_API_DECL void ev_loop_destroy (EV_P);

/* this needs to be called after fork, to duplicate the loop */
/* when you want to re-use it in the child */
/* you can call it in either the parent or the child */
/* you can actually call it at any time, anywhere :) */
EV_API_DECL void ev_loop_fork (EV_P) EV_THROW;

EV_API_DECL unsigned int ev_backend (EV_P) EV_THROW; /* backend in use by loop */

EV_API_DECL void ev_now_update (EV_P) EV_THROW; /* update event loop time */

/* ev_break how values */
enum {
  EVBREAK_CANCEL = 0, /* undo unloop */
  EVBREAK_ONE    = 1, /* unloop once */
  EVBREAK_ALL    = 2  /* unloop all loops */
};

EV_API_DECL int  ev_run (EV_P_ int flags EV_CPP (= 0));
EV_API_DECL void ev_break (EV_P_ int how EV_CPP (= EVBREAK_ONE)) EV_THROW; /* break out of the loop */

EV_API_DECL unsigned int ev_iteration (EV_P) EV_THROW; /* number of loop iterations */
EV_API_DECL unsigned int ev_depth     (EV_P) EV_THROW; /* #ev_loop enters - #ev_loop leaves */
EV_API_DECL void         ev_verify    (EV_P) EV_THROW; /* abort if loop data corrupted */

typedef void (*ev_loop_callback)(EV_P);
EV_API_DECL void ev_set_invoke_pending_cb (EV_P_ ev_loop_callback invoke_pending_cb) EV_THROW;
/* C++ doesn't allow the use of the ev_loop_callback typedef here, so we need to spell it out */
EV_API_DECL void ev_set_loop_release_cb (EV_P_ void (*release)(EV_P) EV_THROW, void (*acquire)(EV_P) EV_THROW) EV_THROW;

EV_API_DECL unsigned int ev_pending_count (EV_P) EV_THROW; /* number of pending events, if any */
EV_API_DECL void ev_invoke_pending (EV_P); /* invoke all pending watchers */

/*
 * stop/start the timer handling.
 */
EV_API_DECL void ev_suspend (EV_P) EV_THROW;
EV_API_DECL void ev_resume  (EV_P) EV_THROW;

/* these may evaluate ev multiple times, and the other arguments at most once */
/* either use ev_init + ev_TYPE_set, or the ev_TYPE_init macro, below, to first initialise a watcher */
#define ev_init(ev,cb_) do {            \
  ((ev_watcher *)(void *)(ev))->active  =    \
  ((ev_watcher *)(void *)(ev))->pending = 0;    \
  ev_set_cb ((ev), cb_);            \
} while (0)

#define ev_io_set(ev,fd_,events_)            do { (ev)->fd = (fd_); (ev)->events = (events_) | EV__IOFDSET; } while (0)
#define ev_timer_set(ev,after_,repeat_)      do { ((ev_watcher_time *)(ev))->at = (after_); (ev)->repeat = (repeat_); } while (0)

#define ev_io_init(ev,cb,fd,events)          do { ev_init ((ev), (cb)); ev_io_set ((ev),(fd),(events)); } while (0)
#define ev_timer_init(ev,cb,after,repeat)    do { ev_init ((ev), (cb)); ev_timer_set ((ev),(after),(repeat)); } while (0)

#define ev_is_pending(ev)                    (0 + ((ev_watcher *)(void *)(ev))->pending) /* ro, true when watcher is waiting for callback invocation */
#define ev_is_active(ev)                     (0 + ((ev_watcher *)(void *)(ev))->active) /* ro, true when the watcher has been started */

/* feeds an event into a watcher as if the event actually occurred */
/* accepts any ev_watcher type */
EV_API_DECL void ev_feed_event     (EV_P_ void *w, int revents) EV_THROW;
EV_API_DECL void ev_feed_fd_event  (EV_P_ int fd, int revents) EV_THROW;

EV_API_DECL void ev_invoke         (EV_P_ void *w, int revents);
EV_API_DECL int  ev_clear_pending  (EV_P_ void *w) EV_THROW;

EV_API_DECL void ev_io_start       (EV_P_ ev_io *w) EV_THROW;
EV_API_DECL void ev_io_stop        (EV_P_ ev_io *w) EV_THROW;

EV_API_DECL void ev_timer_start    (EV_P_ ev_timer *w) EV_THROW;
EV_API_DECL void ev_timer_stop     (EV_P_ ev_timer *w) EV_THROW;
/* stops if active and no repeat, restarts if active and repeating, starts if inactive and repeating */
EV_API_DECL void ev_timer_again    (EV_P_ ev_timer *w) EV_THROW;
/* return remaining time */
EV_API_DECL ev_tstamp ev_timer_remaining (EV_P_ ev_timer *w) EV_THROW;

EV_CPP(})

#endif
