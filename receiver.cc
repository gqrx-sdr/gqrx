#include <receiver.h>


receiver::receiver(const std::string input_device, const std::string audio_device)
{
    d_tb = gr_make_top_block("gqrx");

    d_fcd = fcd_make_source_c(input_device);

    d_sink = gr_make_null_sink(sizeof(gr_complex));

    d_tb->connect(d_fcd, 0, d_sink, 0);

}

receiver::~receiver()
{
  d_tb->stop();
  d_tb->wait();

  /* FIXME: delete blocks? */
}


void receiver::start()
{
    /* FIXME: Check that flow graph is not running */
    d_tb->start();
}

void receiver::stop()
{
    d_tb->stop();
}


rx_status_t receiver::set_rf_freq(float freq_hz)
{
    return RX_STATUS_OK;
}


rx_status_t receiver::set_rf_gain(float gain_db)
{
    return RX_STATUS_OK;
}


rx_status_t receiver::set_tuning_offset(float offset_hz)
{
    return RX_STATUS_OK;
}


rx_status_t receiver::set_filter_low(float freq_hz)
{
    return RX_STATUS_OK;
}


rx_status_t receiver::set_filter_high(float freq_hz)
{
    return RX_STATUS_OK;
}


rx_status_t receiver::set_demod(rx_demod_t demod)
{
    return RX_STATUS_OK;
}


rx_status_t receiver::set_af_gain(float gain_db)
{
    return RX_STATUS_OK;
}

