/*
 * libev event processing core, watcher management
 * modifyed by xzc 2015-08-09
 */

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stddef.h>

#include <stdio.h>

#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <time.h>
#include <limits.h>

# include "ev.h"

# include <sys/time.h>
# include <sys/wait.h>
# include <unistd.h>

/*
 * This is used to work around floating point rounding problems.
 * This value is good at least till the year 4000.
 */
#define MIN_INTERVAL  0.0001220703125 /* 1/2**13, good till 4000 */
/*#define MIN_INTERVAL  0.00000095367431640625 /* 1/2**20, good till 2200 */

#define MIN_TIMEJUMP  1. /* minimum timejump that gets detected (if monotonic clock available) */
#define MAX_BLOCKTIME 59.743 /* never wait longer than this time (to detect time jumps) */

#define EV_TS_SET(ts,t) do { ts.tv_sec = (long)t; ts.tv_nsec = (long)((t - ts.tv_sec) * 1e9); } while (0)

#define expect_false(cond) __builtin_expect (!!(cond),0)
#define expect_true(cond)  __builtin_expect (!!(cond),1)
#define noinline           __attribute__ (__noinline__)

#define inline_size        static inline

//cpp or c99,not gcc 2.5
# define inline_speed      static inline

#define ev_cold    __attribute__ (__cold__)

typedef ev_watcher *W;
typedef ev_watcher_list *WL;
typedef ev_watcher_time *WT;

#define ev_active(w) ((W)(w))->active
#define ev_at(w) ((WT)(w))->at

/*****************************************************************************/

/* define a suitable floor function (only used by periodics atm) */
# include <math.h>
# define ev_floor(v) floor (v)

/*****************************************************************************/

# include <sys/utsname.h>
static unsigned int noinline ev_cold
ev_linux_version (void)
{
#ifdef __linux
  unsigned int v = 0;
  struct utsname buf;
  int i;
  char *p = buf.release;

  if (uname (&buf))
    return 0;

  for (i = 3+1; --i; )
    {
      unsigned int c = 0;

      for (;;)
        {
          if (*p >= '0' && *p <= '9')
            c = c * 10 + *p++ - '0';
          else
            {
              p += *p == '.';
              break;
            }
        }

      v = (v << 8) | c;
    }

  return v;
}

/*****************************************************************************/

static void (*syserr_cb)(const char *msg) EV_THROW;

void ev_cold
ev_set_syserr_cb (void (*cb)(const char *msg) EV_THROW) EV_THROW
{
  syserr_cb = cb;
}

static void noinline ev_cold
ev_syserr (const char *msg = "(libev) system error")
{
  if (syserr_cb)
    syserr_cb (msg);
  else
    {
      perror (msg);
      abort ();
    }
}

static void *
ev_realloc_emul (void *ptr, long size) EV_THROW
{
  /* some systems, notably openbsd and darwin, fail to properly
   * implement realloc (x, 0) (as required by both ansi c-89 and
   * the single unix specification, so work around them here.
   * recently, also (at least) fedora and debian started breaking it,
   * despite documenting it otherwise.
   */

  if (size)
    return realloc (ptr, size);

  free (ptr);
  return 0;
}

static void *(*alloc)(void *ptr, long size) EV_THROW = ev_realloc_emul;

void ev_cold
ev_set_allocator (void *(*cb)(void *ptr, long size) EV_THROW) EV_THROW
{
  alloc = cb;
}

inline_speed void *
ev_realloc (void *ptr, long size)
{
  ptr = alloc (ptr, size);

  if (!ptr && size)
    {
      fprintf (stderr, "(libev) cannot allocate %ld bytes, aborting.", size);
      abort ();
    }

  return ptr;
}

#define ev_malloc(size) ev_realloc (0, (size))
#define ev_free(ptr)    ev_realloc ((ptr), 0)

/*****************************************************************************/

/* set in reify when reification needed */
#define EV_ANFD_REIFY 1

/* file descriptor info structure */
typedef struct
{
  WL head;
  unsigned char events; /* the events watched for */
  unsigned char reify;  /* flag set when this ANFD needs reification (EV_ANFD_REIFY, EV__IOFDSET) */
  unsigned char emask;  /* the epoll backend stores the actual kernel mask in here */
  unsigned char unused;
  unsigned int egen;    /* generation counter to counter epoll bugs */
} ANFD;

/* stores the pending event set for a given watcher */
typedef struct
{
  W w;
  int events; /* the pending event set for the given watcher */
} ANPENDING;


/* a heap element */
typedef WT ANHE;
#define ANHE_w(he)        (he)
#define ANHE_at(he)       (he)->at

/*****************************************************************************/
//here is the struct ev_loop
EV_API_DECL ev_tstamp ev_rt_now = 0; /* needs to be initialised to make it a definition despite extern */
#define VAR(name,decl) static decl;
  #include "ev_vars.h"
#undef VAR

static int ev_default_loop_ptr;
/*****************************************************************************/

ev_tstamp
ev_time (void) EV_THROW
{
  struct timespec ts;
  clock_gettime (CLOCK_REALTIME, &ts);//more precise then gettimeofday
  return ts.tv_sec + ts.tv_nsec * 1e-9;
}

inline_size ev_tstamp
get_clock (void)
{
  struct timespec ts;
  //linux kernel >= 2.6.39,otherwise CLOCK_REALTIME instead
  clock_gettime (CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec * 1e-9;
}

void
ev_sleep (ev_tstamp delay) EV_THROW
{
  if (delay > 0.)
    {
      struct timespec ts;

      EV_TS_SET (ts, delay);
      nanosleep (&ts, 0);
    }
}

/*****************************************************************************/

#define MALLOC_ROUND 4096 /* prefer to allocate in chunks of this size, must be 2**n and >> 4 longs */

/* find a suitable new size for the given array, */
/* hopefully by rounding to a nice-to-malloc size */
inline_size int
array_nextsize (int elem, int cur, int cnt)
{
  int ncur = cur + 1;

  do
    ncur <<= 1;
  while (cnt > ncur);

  /* if size is large, round to MALLOC_ROUND - 4 * longs to accommodate malloc overhead */
  if (elem * ncur > MALLOC_ROUND - sizeof (void *) * 4)
    {
      ncur *= elem;
      ncur = (ncur + elem + (MALLOC_ROUND - 1) + sizeof (void *) * 4) & ~(MALLOC_ROUND - 1);
      ncur = ncur - sizeof (void *) * 4;
      ncur /= elem;
    }

  return ncur;
}

static void * noinline ev_cold
array_realloc (int elem, void *base, int *cur, int cnt)
{
  *cur = array_nextsize (elem, *cur, cnt);
  return ev_realloc (base, elem * *cur);
}

#define array_init_zero(base,count)    \
  memset ((void *)(base), 0, sizeof (*(base)) * (count))

#define array_needsize(type,base,cur,cnt,init)            \
  if (expect_false ((cnt) > (cur)))                \
    {                                \
      int ecb_unused ocur_ = (cur);                    \
      (base) = (type *)array_realloc                \
         (sizeof (type), (base), &(cur), (cnt));        \
      init ((base) + (ocur_), (cur) - ocur_);            \
    }

#define array_free(stem, idx) \
  ev_free (stem ## s idx); stem ## cnt idx = stem ## max idx = 0; stem ## s idx = 0

/*****************************************************************************/
void noinline
ev_feed_event (EV_P_ void *w, int revents) EV_THROW
{
  W w_ = (W)w;
  int pri = ABSPRI (w_);

  if (expect_false (w_->pending))
    pendings [pri][w_->pending - 1].events |= revents;
  else
    {
      w_->pending = ++pendingcnt [pri];
      array_needsize (ANPENDING, pendings [pri], pendingmax [pri], w_->pending, EMPTY2);
      pendings [pri][w_->pending - 1].w      = w_;
      pendings [pri][w_->pending - 1].events = revents;
    }

  pendingpri = NUMPRI - 1;
}

inline_speed void
feed_reverse (EV_P_ W w)
{
  array_needsize (W, rfeeds, rfeedmax, rfeedcnt + 1, EMPTY2);
  rfeeds [rfeedcnt++] = w;
}

inline_size void
feed_reverse_done (EV_P_ int revents)
{
  do
    ev_feed_event (EV_A_ rfeeds [--rfeedcnt], revents);
  while (rfeedcnt);
}

inline_speed void
queue_events (EV_P_ W *events, int eventcnt, int type)
{
  int i;

  for (i = 0; i < eventcnt; ++i)
    ev_feed_event (EV_A_ events [i], type);
}

/*****************************************************************************/

inline_speed void
fd_event_nocheck (EV_P_ int fd, int revents)
{
  ANFD *anfd = anfds + fd;
  ev_io *w;

  for (w = (ev_io *)anfd->head; w; w = (ev_io *)((WL)w)->next)
    {
      int ev = w->events & revents;

      if (ev)
        ev_feed_event (EV_A_ (W)w, ev);
    }
}

/* do not submit kernel events for fds that have reify set */
/* because that means they changed while we were polling for new events */
inline_speed void
fd_event (EV_P_ int fd, int revents)
{
  ANFD *anfd = anfds + fd;

  if (expect_true (!anfd->reify))
    fd_event_nocheck (EV_A_ fd, revents);
}

void
ev_feed_fd_event (EV_P_ int fd, int revents) EV_THROW
{
  if (fd >= 0 && fd < anfdmax)
    fd_event_nocheck (EV_A_ fd, revents);
}

/* make sure the external fd watch events are in-sync */
/* with the kernel/libev internal state */
inline_size void
fd_reify (EV_P)
{
  int i;

  for (i = 0; i < fdchangecnt; ++i)
    {
      int fd = fdchanges [i];
      ANFD *anfd = anfds + fd;
      ev_io *w;

      unsigned char o_events = anfd->events;
      unsigned char o_reify  = anfd->reify;

      anfd->reify  = 0;

      /*if (expect_true (o_reify & EV_ANFD_REIFY)) probably a deoptimisation */
        {
          anfd->events = 0;

          for (w = (ev_io *)anfd->head; w; w = (ev_io *)((WL)w)->next)
            anfd->events |= (unsigned char)w->events;

          if (o_events != anfd->events)
            o_reify = EV__IOFDSET; /* actually |= */
        }

      if (o_reify & EV__IOFDSET)
        backend_modify (EV_A_ fd, o_events, anfd->events);
    }

  fdchangecnt = 0;
}

/* something about the given fd changed */
inline_size void
fd_change (EV_P_ int fd, int flags)
{
  unsigned char reify = anfds [fd].reify;
  anfds [fd].reify |= flags;

  if (expect_true (!reify))
    {
      ++fdchangecnt;
      array_needsize (int, fdchanges, fdchangemax, fdchangecnt, EMPTY2);
      fdchanges [fdchangecnt - 1] = fd;
    }
}

/* the given fd is invalid/unusable, so make sure it doesn't hurt us anymore */
inline_speed void ev_cold
fd_kill (EV_P_ int fd)
{
  ev_io *w;

  while ((w = (ev_io *)anfds [fd].head))
    {
      ev_io_stop (EV_A_ w);
      ev_feed_event (EV_A_ (W)w, EV_ERROR | EV_READ | EV_WRITE);
    }
}

/* check whether the given fd is actually valid, for error recovery */
inline_size int ev_cold
fd_valid (int fd)
{
  return fcntl (fd, F_GETFD) != -1;
}

/*****************************************************************************/

/*
 * the heap functions want a real array index. array index 0 is guaranteed to not
 * be in-use at any time. the first heap entry is at array [HEAP0]. DHEAP gives
 * the branching factor of the d-tree.
 */

#define HEAP0 1
#define HPARENT(k) ((k) >> 1)
#define UPHEAP_DONE(p,k) (!(p))

/* away from the root */
inline_speed void
downheap (ANHE *heap, int N, int k)
{
  ANHE he = heap [k];

  for (;;)
    {
      int c = k << 1;

      if (c >= N + HEAP0)
        break;

      c += c + 1 < N + HEAP0 && ANHE_at (heap [c]) > ANHE_at (heap [c + 1])
           ? 1 : 0;

      if (ANHE_at (he) <= ANHE_at (heap [c]))
        break;

      heap [k] = heap [c];
      ev_active (ANHE_w (heap [k])) = k;

      k = c;
    }

  heap [k] = he;
  ev_active (ANHE_w (he)) = k;
}

/* towards the root */
inline_speed void
upheap (ANHE *heap, int k)
{
  ANHE he = heap [k];

  for (;;)
    {
      int p = HPARENT (k);

      if (UPHEAP_DONE (p, k) || ANHE_at (heap [p]) <= ANHE_at (he))
        break;

      heap [k] = heap [p];
      ev_active (ANHE_w (heap [k])) = k;
      k = p;
    }

  heap [k] = he;
  ev_active (ANHE_w (he)) = k;
}

/* move an element suitably so it is in a correct place */
inline_size void
adjustheap (ANHE *heap, int N, int k)
{
  if (k > HEAP0 && ANHE_at (heap [k]) <= ANHE_at (heap [HPARENT (k)]))
    upheap (heap, k);
  else
    downheap (heap, N, k);
}

/* rebuild the heap: this function is used only once and executed rarely */
inline_size void
reheap (ANHE *heap, int N)
{
  int i;

  /* we don't use floyds algorithm, upheap is simpler and is more cache-efficient */
  /* also, this is easy to implement and correct for both 2-heaps and 4-heaps */
  for (i = 0; i < N; ++i)
    upheap (heap, i + HEAP0);
}

/*****************************************************************************/
# include "ev_epoll.c"

int ev_cold
ev_version_major (void) EV_THROW
{
  return EV_VERSION_MAJOR;
}

int ev_cold
ev_version_minor (void) EV_THROW
{
  return EV_VERSION_MINOR;
}

/* return true if we are running with elevated privileges and should ignore env variables */
int inline_size ev_cold
enable_secure (void)
{
  return getuid () != geteuid ()
      || getgid () != getegid ();
}

unsigned int
ev_backend (EV_P) EV_THROW
{
  return backend;
}

/* initialise a loop structure, must be zero-initialised */
static void noinline ev_cold
loop_init (EV_P_ unsigned int flags) EV_THROW
{
  if (!backend)
    {
      origflags = flags;

      if (!(flags & EVFLAG_NOENV)
          && !enable_secure ()
          && getenv ("LIBEV_FLAGS"))
        flags = atoi (getenv ("LIBEV_FLAGS"));

      ev_rt_now          = ev_time ();
      mn_now             = get_clock ();
      now_floor          = mn_now;
      rtmn_diff          = ev_rt_now - mn_now;

      io_blocktime       = 0.;
      timeout_blocktime  = 0.;
      backend            = 0;
      backend_fd         = -1;

      if (!(flags & EVBACKEND_MASK))
        flags |= ev_recommended_backends ();

      if (!backend && (flags & EVBACKEND_EPOLL )) backend = epoll_init  (EV_A_ flags);
    }
}

/* free up a loop structure */
void ev_cold
ev_loop_destroy (EV_P)
{
  int i;

  if (backend_fd >= 0)
    close (backend_fd);

  if (backend == EVBACKEND_EPOLL ) epoll_destroy  (EV_A);

  for (i = NUMPRI; i--; )
    {
      array_free (pending, [i]);
    }

  ev_free (anfds); anfds = 0; anfdmax = 0;

  /* have to use the microsoft-never-gets-it-right macro */
  array_free (rfeed, EMPTY);
  array_free (fdchange, EMPTY);
  array_free (timer, EMPTY);

  backend = 0;

  ev_default_loop_ptr = 0;  //TODO is it right ??
}

/******************************************************************************/
//verify function
#if EV_VERIFY
static void noinline ev_cold
verify_watcher (EV_P_ W w)
{
  assert (("libev: watcher has invalid priority", ABSPRI (w) >= 0 && ABSPRI (w) < NUMPRI));

  if (w->pending)
    assert (("libev: pending watcher not on pending queue", pendings [ABSPRI (w)][w->pending - 1].w == w));
}

static void noinline ev_cold
verify_heap (EV_P_ ANHE *heap, int N)
{
  int i;

  for (i = HEAP0; i < N + HEAP0; ++i)
    {
      assert (("libev: active index mismatch in heap", ev_active (ANHE_w (heap [i])) == i));
      assert (("libev: heap condition violated", i == HEAP0 || ANHE_at (heap [HPARENT (i)]) <= ANHE_at (heap [i])));
      assert (("libev: heap at cache mismatch", ANHE_at (heap [i]) == ev_at (ANHE_w (heap [i]))));

      verify_watcher (EV_A_ (W)ANHE_w (heap [i]));
    }
}

static void noinline ev_cold
array_verify (EV_P_ W *ws, int cnt)
{
  while (cnt--)
    {
      assert (("libev: active index mismatch", ev_active (ws [cnt]) == cnt + 1));
      verify_watcher (EV_A_ ws [cnt]);
    }
}
#endif

void ev_cold
ev_verify (EV_P) EV_THROW
{
  int i;
  WL w, w2;

  assert (activecnt >= -1);

  assert (fdchangemax >= fdchangecnt);
  for (i = 0; i < fdchangecnt; ++i)
    assert (("libev: negative fd in fdchanges", fdchanges [i] >= 0));

  assert (anfdmax >= 0);
  for (i = 0; i < anfdmax; ++i)
    {
      int j = 0;

      for (w = w2 = anfds [i].head; w; w = w->next)
        {
          verify_watcher (EV_A_ (W)w);

          if (j++ & 1)
            {
              assert (("libev: io watcher list contains a loop", w != w2));
              w2 = w2->next;
            }

          assert (("libev: inactive fd watcher on anfd list", ev_active (w) == 1));
          assert (("libev: fd mismatch between watcher and anfd", ((ev_io *)w)->fd == i));
        }
    }

  assert (timermax >= timercnt);
  verify_heap (EV_A_ timers, timercnt);

  for (i = NUMPRI; i--; )
    {
      assert (pendingmax [i] >= pendingcnt [i]);
    }
}

int
ev_default_loop (unsigned int flags) EV_THROW
{
  if (!ev_default_loop_ptr)
    {
      ev_default_loop_ptr = 1;

      loop_init (EV_A_ flags);

      if (ev_backend (EV_A))
        {
            //TODO
        }
      else
        ev_default_loop_ptr = 0;
    }

  return ev_default_loop_ptr;
}

/*****************************************************************************/

void
ev_invoke (EV_P_ void *w, int revents)
{
  EV_CB_INVOKE ((W)w, revents);
}

unsigned int
ev_pending_count (EV_P) EV_THROW
{
  int pri;
  unsigned int count = 0;

  for (pri = NUMPRI; pri--; )
    count += pendingcnt [pri];

  return count;
}

void noinline
ev_invoke_pending (EV_P)
{
  pendingpri = NUMPRI;

  while (pendingpri) /* pendingpri possibly gets modified in the inner loop */
    {
      --pendingpri;

      while (pendingcnt [pendingpri])
        {
          ANPENDING *p = pendings [pendingpri] + --pendingcnt [pendingpri];

          p->w->pending = 0;
          EV_CB_INVOKE (p->w, p->events);
          EV_FREQUENT_CHECK;
        }
    }
}

/* make timers pending */
inline_size void
timers_reify (EV_P)
{
  EV_FREQUENT_CHECK;

  if (timercnt && ANHE_at (timers [HEAP0]) < mn_now)
    {
      do
        {
          ev_timer *w = (ev_timer *)ANHE_w (timers [HEAP0]);

          /*assert (("libev: inactive timer on timer heap detected", ev_is_active (w)));*/

          /* first reschedule or stop timer */
          if (w->repeat)
            {
              ev_at (w) += w->repeat;
              if (ev_at (w) < mn_now)
                ev_at (w) = mn_now;

              assert (("libev: negative ev_timer repeat value found while processing timers", w->repeat > 0.));

              ANHE_at_cache (timers [HEAP0]);
              downheap (timers, timercnt, HEAP0);
            }
          else
            ev_timer_stop (EV_A_ w); /* nonrepeating: stop timer */

          EV_FREQUENT_CHECK;
          feed_reverse (EV_A_ (W)w);
        }
      while (timercnt && ANHE_at (timers [HEAP0]) < mn_now);

      feed_reverse_done (EV_A_ EV_TIMER);
    }
}

/* adjust all timers by a given offset */
static void noinline ev_cold
timers_reschedule (EV_P_ ev_tstamp adjust)
{
  int i;

  for (i = 0; i < timercnt; ++i)
    {
      ANHE *he = timers + i + HEAP0;
      ANHE_w (*he)->at += adjust;
    }
}

/* fetch new monotonic and realtime times from the kernel */
/* also detect if there was a timejump, and act accordingly */
inline_speed void
time_update (EV_P_ ev_tstamp max_block)
{
  int i;
  ev_tstamp odiff = rtmn_diff;

  mn_now = get_clock ();

  /* only fetch the realtime clock every 0.5*MIN_TIMEJUMP seconds */
  /* interpolate in the meantime */
  if (expect_true (mn_now - now_floor < MIN_TIMEJUMP * .5))
    {
      ev_rt_now = rtmn_diff + mn_now;
      return;
    }

  now_floor = mn_now;
  ev_rt_now = ev_time ();

  /* loop a few times, before making important decisions.
   * on the choice of "4": one iteration isn't enough,
   * in case we get preempted during the calls to
   * ev_time and get_clock. a second call is almost guaranteed
   * to succeed in that case, though. and looping a few more times
   * doesn't hurt either as we only do this on time-jumps or
   * in the unlikely event of having been preempted here.
   */
  for (i = 4; --i; )
    {
      ev_tstamp diff;
      rtmn_diff = ev_rt_now - mn_now;

      diff = odiff - rtmn_diff;

      if (expect_true ((diff < 0. ? -diff : diff) < MIN_TIMEJUMP))
        return; /* all is well */

      ev_rt_now = ev_time ();
      mn_now    = get_clock ();
      now_floor = mn_now;
    }

  /* no timer adjustment, as the monotonic clock doesn't jump */
  /* timers_reschedule (EV_A_ rtmn_diff - odiff) */
}

int
ev_run (EV_P_ int flags)
{

  assert (("libev: ev_loop recursion during release detected", loop_done != EVBREAK_RECURSE));

  loop_done = EVBREAK_CANCEL;

  EV_INVOKE_PENDING; /* in case we recurse, ensure ordering stays nice and clean */

  do
    {
#if EV_VERIFY >= 2
      ev_verify (EV_A);//TODO test
#endif

      if (expect_false (loop_done))
        break;

      /* update fd-related kernel structures */
      fd_reify (EV_A);

      /* calculate blocking time */
      {
        ev_tstamp waittime  = 0.;
        ev_tstamp sleeptime = 0.;

        /* remember old timestamp for io_blocktime calculation */
        ev_tstamp prev_mn_now = mn_now;

        /* update time to cancel out callback processing overhead */
        time_update (EV_A_ 1e100);

        if (expect_true (!(flags & EVRUN_NOWAIT || idleall || !activecnt || pipe_write_skipped)))
          {
            waittime = MAX_BLOCKTIME;

            if (timercnt)
              {
                ev_tstamp to = ANHE_at (timers [HEAP0]) - mn_now;
                if (waittime > to) waittime = to;
              }

            /* don't let timeouts decrease the waittime below timeout_blocktime */
            if (expect_false (waittime < timeout_blocktime))
              waittime = timeout_blocktime;

            /* at this point, we NEED to wait, so we have to ensure */
            /* to pass a minimum nonzero value to the backend */
            if (expect_false (waittime < backend_mintime))
              waittime = backend_mintime;

            /* extra check because io_blocktime is commonly 0 */
            if (expect_false (io_blocktime))
              {
                sleeptime = io_blocktime - (mn_now - prev_mn_now);

                if (sleeptime > waittime - backend_mintime)
                  sleeptime = waittime - backend_mintime;

                if (expect_true (sleeptime > 0.))
                  {
                    ev_sleep (sleeptime);
                    waittime -= sleeptime;
                  }
              }
          }

        assert ((loop_done = EVBREAK_RECURSE, 1)); /* assert for side effect */
        backend_poll (EV_A_ waittime);
        assert ((loop_done = EVBREAK_CANCEL, 1)); /* assert for side effect */


        /* update ev_rt_now, do magic */
        time_update (EV_A_ waittime + sleeptime);
      }

      /* queue pending timers and reschedule them */
      timers_reify (EV_A); /* relative timers called last */

      EV_INVOKE_PENDING;
    }
  while (expect_true (
    activecnt
    && !loop_done
    && !(flags & (EVRUN_ONCE | EVRUN_NOWAIT))
  ));

  if (loop_done == EVBREAK_ONE)
    loop_done = EVBREAK_CANCEL;

  return activecnt;
}

void
ev_break (EV_P_ int how) EV_THROW
{
  loop_done = how;
}

void
ev_now_update (EV_P) EV_THROW
{
  time_update (EV_A_ 1e100);
}

void
ev_suspend (EV_P) EV_THROW
{
  ev_now_update (EV_A);
}

void
ev_resume (EV_P) EV_THROW
{
  ev_tstamp mn_prev = mn_now;

  ev_now_update (EV_A);
  timers_reschedule (EV_A_ mn_now - mn_prev);
}

/*****************************************************************************/
/* singly-linked list management, used when the expected list length is short */

inline_size void
wlist_add (WL *head, WL elem)
{
  elem->next = *head;
  *head = elem;
}

inline_size void
wlist_del (WL *head, WL elem)
{
  while (*head)
    {
      if (expect_true (*head == elem))
        {
          *head = elem->next;
          break;
        }

      head = &(*head)->next;
    }
}

/* internal, faster, version of ev_clear_pending */
inline_speed void
clear_pending (EV_P_ W w)
{
  if (w->pending)
    {
      pendings [ABSPRI (w)][w->pending - 1].w = (W)&pending_w;
      w->pending = 0;
    }
}

int
ev_clear_pending (EV_P_ void *w) EV_THROW
{
  W w_ = (W)w;
  int pending = w_->pending;

  if (expect_true (pending))
    {
      ANPENDING *p = pendings [ABSPRI (w_)] + pending - 1;
      p->w = (W)&pending_w;
      w_->pending = 0;
      return p->events;
    }
  else
    return 0;
}

inline_speed void
ev_start (EV_P_ W w, int active)
{
  w->active = active;
  ev_ref (EV_A);
}

inline_size void
ev_stop (EV_P_ W w)
{
  ev_unref (EV_A);
  w->active = 0;
}

/*****************************************************************************/

void noinline
ev_io_start (EV_P_ ev_io *w) EV_THROW
{
  int fd = w->fd;

  if (expect_false (ev_is_active (w)))
    return;

  assert (("libev: ev_io_start called with negative fd", fd >= 0));
  assert (("libev: ev_io_start called with illegal event mask", !(w->events & ~(EV__IOFDSET | EV_READ | EV_WRITE))));

  EV_FREQUENT_CHECK;

  ev_start (EV_A_ (W)w, 1);
  array_needsize (ANFD, anfds, anfdmax, fd + 1, array_init_zero);
  wlist_add (&anfds[fd].head, (WL)w);

  /* common bug, apparently */
  assert (("libev: ev_io_start called with corrupted watcher", ((WL)w)->next != (WL)w));

  fd_change (EV_A_ fd, w->events & EV__IOFDSET | EV_ANFD_REIFY);
  w->events &= ~EV__IOFDSET;

  EV_FREQUENT_CHECK;
}

void noinline
ev_io_stop (EV_P_ ev_io *w) EV_THROW
{
  clear_pending (EV_A_ (W)w);
  if (expect_false (!ev_is_active (w)))
    return;

  assert (("libev: ev_io_stop called with illegal fd (must stay constant after start!)", w->fd >= 0 && w->fd < anfdmax));

  EV_FREQUENT_CHECK;

  wlist_del (&anfds[w->fd].head, (WL)w);
  ev_stop (EV_A_ (W)w);

  fd_change (EV_A_ w->fd, EV_ANFD_REIFY);

  EV_FREQUENT_CHECK;
}

void noinline
ev_timer_start (EV_P_ ev_timer *w) EV_THROW
{
  if (expect_false (ev_is_active (w)))
    return;

  ev_at (w) += mn_now;

  assert (("libev: ev_timer_start called with negative timer repeat value", w->repeat >= 0.));

  EV_FREQUENT_CHECK;

  ++timercnt;
  ev_start (EV_A_ (W)w, timercnt + HEAP0 - 1);
  array_needsize (ANHE, timers, timermax, ev_active (w) + 1, EMPTY2);
  ANHE_w (timers [ev_active (w)]) = (WT)w;
  ANHE_at_cache (timers [ev_active (w)]);
  upheap (timers, ev_active (w));

  EV_FREQUENT_CHECK;

  /*assert (("libev: internal timer heap corruption", timers [ev_active (w)] == (WT)w));*/
}

void noinline
ev_timer_stop (EV_P_ ev_timer *w) EV_THROW
{
  clear_pending (EV_A_ (W)w);
  if (expect_false (!ev_is_active (w)))
    return;

  EV_FREQUENT_CHECK;

  {
    int active = ev_active (w);

    assert (("libev: internal timer heap corruption", ANHE_w (timers [active]) == (WT)w));

    --timercnt;

    if (expect_true (active < timercnt + HEAP0))
      {
        timers [active] = timers [timercnt + HEAP0];
        adjustheap (timers, timercnt, active);
      }
  }

  ev_at (w) -= mn_now;

  ev_stop (EV_A_ (W)w);

  EV_FREQUENT_CHECK;
}

void noinline
ev_timer_again (EV_P_ ev_timer *w) EV_THROW
{
  EV_FREQUENT_CHECK;

  clear_pending (EV_A_ (W)w);

  if (ev_is_active (w))
    {
      if (w->repeat)
        {
          ev_at (w) = mn_now + w->repeat;
          ANHE_at_cache (timers [ev_active (w)]);
          adjustheap (timers, timercnt, ev_active (w));
        }
      else
        ev_timer_stop (EV_A_ w);
    }
  else if (w->repeat)
    {
      ev_at (w) = w->repeat;
      ev_timer_start (EV_A_ w);
    }

  EV_FREQUENT_CHECK;
}

ev_tstamp
ev_timer_remaining (EV_P_ ev_timer *w) EV_THROW
{
  return ev_at (w) - (ev_is_active (w) ? mn_now : 0.);
}

/*****************************************************************************/

struct ev_once
{
  ev_io io;
  ev_timer to;
  void (*cb)(int revents, void *arg);
  void *arg;
};

static void
once_cb (EV_P_ struct ev_once *once, int revents)
{
  void (*cb)(int revents, void *arg) = once->cb;
  void *arg = once->arg;

  ev_io_stop    (EV_A_ &once->io);
  ev_timer_stop (EV_A_ &once->to);
  ev_free (once);

  cb (revents, arg);
}

static void
once_cb_io (EV_P_ ev_io *w, int revents)
{
  struct ev_once *once = (struct ev_once *)(((char *)w) - offsetof (struct ev_once, io));

  once_cb (EV_A_ once, revents | ev_clear_pending (EV_A_ &once->to));
}

static void
once_cb_to (EV_P_ ev_timer *w, int revents)
{
  struct ev_once *once = (struct ev_once *)(((char *)w) - offsetof (struct ev_once, to));

  once_cb (EV_A_ once, revents | ev_clear_pending (EV_A_ &once->io));
}

void
ev_once (EV_P_ int fd, int events, ev_tstamp timeout, void (*cb)(int revents, void *arg), void *arg) EV_THROW
{
  struct ev_once *once = (struct ev_once *)ev_malloc (sizeof (struct ev_once));

  if (expect_false (!once))
    {
      cb (EV_ERROR | EV_READ | EV_WRITE | EV_TIMER, arg);
      return;
    }

  once->cb  = cb;
  once->arg = arg;

  ev_init (&once->io, once_cb_io);
  if (fd >= 0)
    {
      ev_io_set (&once->io, fd, events);
      ev_io_start (EV_A_ &once->io);
    }

  ev_init (&once->to, once_cb_to);
  if (timeout >= 0.)
    {
      ev_timer_set (&once->to, timeout, 0.);
      ev_timer_start (EV_A_ &once->to);
    }
}

/*****************************************************************************/
