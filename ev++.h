/*
 * libev simple C++ wrapper classes base on libev 4.20
 * modify by xzc 2015-08-19
 */

#ifndef EVPP_H__
#define EVPP_H__

#include "ev.h"
#include <stdexcept>

namespace ev {

  typedef ev_tstamp tstamp;

  enum {
    UNDEF    = EV_UNDEF,
    NONE     = EV_NONE,
    READ     = EV_READ,
    WRITE    = EV_WRITE,
    TIMER    = EV_TIMER,
#   undef ERROR // some systems stupidly #define ERROR
    ERROR    = EV_ERROR
  };

  struct bad_loop
  : std::runtime_error
  {
    bad_loop ()
    : std::runtime_error ("libev event loop cannot be initialized, bad value of LIBEV_FLAGS?")
    {
    }
  };

#ifdef EV_AX
#  undef EV_AX
#endif

#ifdef EV_AX_
#  undef EV_AX_
#endif

#define EV_AX  raw_loop
#define EV_AX_ raw_loop,

  struct loop_ref
  {
    loop_ref (struct ev_loop *loop) throw ()
    : EV_AX (loop)
    {
    }

    bool operator == (const loop_ref &other) const throw ()
    {
      return EV_AX == other.EV_AX;
    }

    bool operator != (const loop_ref &other) const throw ()
    {
      return ! (*this == other);
    }

    bool operator == (const struct ev_loop *loop) const throw ()
    {
      return this->EV_AX == loop;
    }

    bool operator != (const struct ev_loop *loop) const throw ()
    {
      return (*this == loop);
    }

    operator struct ev_loop * () const throw ()
    {
      return EV_AX;
    }

    operator const struct ev_loop * () const throw ()
    {
      return EV_AX;
    }

    void run ()
    {
      ev_run (EV_AX);
    }

    void break_loop () throw ()
    {
      ev_break (EV_AX);
    }

    tstamp now () const throw ()
    {
      return ev_now (EV_AX);
    }

    template<class K, void (K::*method)(int)>
    static void method_thunk (int revents, void *arg)
    {
      (static_cast<K *>(arg)->*method)
        (revents);
    }

    template<class K, void (K::*method)()>
    static void method_noargs_thunk (int revents, void *arg)
    {
      (static_cast<K *>(arg)->*method)
        ();
    }

    template<void (*cb)(int)>
    static void simpler_func_thunk (int revents, void *arg)
    {
      (*cb)
        (revents);
    }

    template<void (*cb)()>
    static void simplest_func_thunk (int revents, void *arg)
    {
      (*cb)
        ();
    }

    struct ev_loop* EV_AX;
  };

#undef EV_AX
#undef EV_AX_

#undef EV_PX
#undef EV_PX_

#define EV_PX  loop_ref loop
#define EV_PX_ loop_ref loop,


  template<class ev_watcher, class watcher>
  struct base : ev_watcher
  {
    EV_PX;

    // loop set
    void set (struct ev_loop *loop) throw ()
    {
      this->loop = loop;
    }

    base (EV_PX) throw ()
      : loop (loop)
    {
      ev_init (this, 0);
    }

    void set_ (const void *data, void (*cb)(struct ev_loop *loop, ev_watcher *w, int revents)) throw ()
    {
      this->data = (void *)data;
      ev_set_cb (static_cast<ev_watcher *>(this), cb);
    }

    // function callback
    template<void (*function)(watcher &w, int)>
    void set (void *data = 0) throw ()
    {
      set_ (data, function_thunk<function>);
    }

    template<void (*function)(watcher &w, int)>
    static void function_thunk (struct ev_loop *loop, ev_watcher *w, int revents)
    {
      function
        (*static_cast<watcher *>(w), revents);
    }

    // method callback
    template<class K, void (K::*method)(watcher &w, int)>
    void set (K *object) throw ()
    {
      set_ (object, method_thunk<K, method>);
    }

    // default method == operator ()
    template<class K>
    void set (K *object) throw ()
    {
      set_ (object, method_thunk<K, &K::operator ()>);
    }

    template<class K, void (K::*method)(watcher &w, int)>
    static void method_thunk (struct ev_loop *loop, ev_watcher *w, int revents)
    {
      (static_cast<K *>(w->data)->*method)
        (*static_cast<watcher *>(w), revents);
    }

    // no-argument callback
    template<class K, void (K::*method)()>
    void set (K *object) throw ()
    {
      set_ (object, method_noargs_thunk<K, method>);
    }

    template<class K, void (K::*method)()>
    static void method_noargs_thunk (struct ev_loop *loop, ev_watcher *w, int revents)
    {
      (static_cast<K *>(w->data)->*method)
        ();
    }

    void operator ()(int events = EV_UNDEF)
    {
      return
        ev_cb (static_cast<ev_watcher *>(this))
          (static_cast<ev_watcher *>(this), events);
    }

    bool is_active () const throw ()
    {
      return ev_is_active (static_cast<const ev_watcher *>(this));
    }
  };

  inline tstamp now (struct ev_loop *loop) throw ()
  {
    return ev_now (loop);
  }

  inline void delay (tstamp interval) throw ()
  {
    ev_sleep (interval);
  }

  inline int version_major () throw ()
  {
    return ev_version_major ();
  }

  inline int version_minor () throw ()
  {
    return ev_version_minor ();
  }

  inline void set_allocator (void *(*cb)(void *ptr, long size) throw ()) throw ()
  {
    ev_set_allocator (cb);
  }

  inline void set_syserr_cb (void (*cb)(const char *msg) throw ()) throw ()
  {
    ev_set_syserr_cb (cb);
  }

  #define EV_CONSTRUCT(cppstem,cstem)	                                                \
    (EV_PX = 0) throw ()                                                              \
      : base<ev_ ## cstem, cppstem> (loop)                                            \
    {                                                                                 \
    }

  /* using a template here would require quite a bit more lines,
   * so a macro solution was chosen */
  #define EV_BEGIN_WATCHER(cppstem,cstem)	                                        \
                                                                                        \
  struct cppstem : base<ev_ ## cstem, cppstem>                                          \
  {                                                                                     \
    void start () throw ()                                                              \
    {                                                                                   \
      ev_ ## cstem ## _start (loop, static_cast<ev_ ## cstem *>(this));                 \
    }                                                                                   \
                                                                                        \
    void stop () throw ()                                                               \
    {                                                                                   \
      ev_ ## cstem ## _stop (loop, static_cast<ev_ ## cstem *>(this));                  \
    }                                                                                   \
                                                                                        \
    cppstem EV_CONSTRUCT(cppstem,cstem)                                                 \
                                                                                        \
    ~cppstem () throw ()                                                                \
    {                                                                                   \
      stop ();                                                                          \
    }                                                                                   \
                                                                                        \
    using base<ev_ ## cstem, cppstem>::set;                                             \
                                                                                        \
  private:                                                                              \
                                                                                        \
    cppstem (const cppstem &o);                                                         \
                                                                                        \
    cppstem &operator =(const cppstem &o);                                              \
                                                                                        \
  public:

  #define EV_END_WATCHER(cppstem,cstem)	                                                \
  };

  EV_BEGIN_WATCHER (io, io)
    void set (int fd, int events) throw ()
    {
      int active = is_active ();
      if (active) stop ();
      ev_io_set (static_cast<ev_io *>(this), fd, events);
      if (active) start ();
    }

    void set (int events) throw ()
    {
      int active = is_active ();
      if (active) stop ();
      ev_io_set (static_cast<ev_io *>(this), fd, events);
      if (active) start ();
    }

    void start (int fd, int events) throw ()
    {
      set (fd, events);
      start ();
    }
  EV_END_WATCHER (io, io)

  EV_BEGIN_WATCHER (timer, timer)
    void set (ev_tstamp after, ev_tstamp repeat = 0.) throw ()
    {
      int active = is_active ();
      if (active) stop ();
      ev_timer_set (static_cast<ev_timer *>(this), after, repeat);
      if (active) start ();
    }

    void start (ev_tstamp after, ev_tstamp repeat = 0.) throw ()
    {
      set (after, repeat);
      start ();
    }

    void again () throw ()
    {
      ev_timer_again (loop, static_cast<ev_timer *>(this));
    }

    ev_tstamp remaining ()
    {
      return ev_timer_remaining (loop, static_cast<ev_timer *>(this));
    }
  EV_END_WATCHER (timer, timer)

  #undef EV_PX
  #undef EV_PX_
  #undef EV_CONSTRUCT
  #undef EV_BEGIN_WATCHER
  #undef EV_END_WATCHER
}

#endif
