#include <receiver.h>


receiver::receiver()
{
    tb = gr_make_top_block("gqrx");
    src = gr_make_null_source(sizeof(gr_complex));
    sink = gr_make_null_sink(sizeof(gr_complex));
    throttle = gr_make_throttle(sizeof(gr_complex), 98000.0);

    tb->connect(src, 0, throttle, 0);
    tb->connect(throttle, 0, sink, 0);
}

receiver::~receiver()
{
  tb->stop();
  tb->wait();

  /* FIXME: delete blocks? */
}


void receiver::start()
{
    /* FIXME: Check that flow graph is not running */
    tb->start();
}

void receiver::stop()
{
    tb->stop();
}
