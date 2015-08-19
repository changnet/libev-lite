/* cpp test */

#include <iostream>
#include "ev++.h"

/* test class */
class CTest
{
private:
    ev::io    m_io;
    ev::timer m_timer;
    
    ev::loop_ref loop;
public:
    explicit CTest( ev_loop *loop)
    : loop(loop)
    {
        
    }

    void io_cb(ev::io &w,int revents)
    {
        if ( ev::ERROR & revents )
        {
            std::cerr << "ev error" << std::endl;
            w.stop();
            return;
        }
        
        std::cout << "io_cb ... break" << std::endl;
        loop.break_loop();
    }

    void timer_cb(ev::timer &w,int revents)
    {
        if ( ev::ERROR & revents ) /* ev error */
        {
            std::cerr << "ev error" << std::endl;
            w.stop();
            return;
        }

        std::cout << "timer_cb ..." << std::endl;
    }

    void start()
    {
        m_io.set( loop );
        m_io.set<CTest,&CTest::io_cb>(this);
        m_io.start( /*STDIN_FILENO*/ 0,ev::READ );
        
        m_timer.set(loop);
        m_timer.set<CTest,&CTest::timer_cb>(this);
        m_timer.start( 5,1 );
    }
};

int main()
{
    struct ev_loop *loop = ev_loop_new();
    CTest t(loop);

    t.start();

    ev_run(loop);

    return 0;
}
