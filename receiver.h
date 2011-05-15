#ifndef RECEIVER_H
#define RECEIVER_H

#include <gr_top_block.h>
#include <gr_null_source.h>
#include <gr_null_sink.h>
#include <gr_throttle.h>
#include <fcd/fcd_source_c.h>


/*! \defgroup DSP Digital signal processing library based on GNU Radio */


typedef enum {
    RX_STATUS_OK    = 0,
    RX_STATUS_ERROR = 1
} rx_status_t;


typedef enum {
    RX_DEMOD_NONE = 0,
    RX_DEMOD_SSB  = 1,
    RX_DEMOD_AM   = 2,
    RX_DEMOD_AMS  = 3,
    RX_DEMOD_FMN  = 4,
    RX_DEMOD_APT  = 5,
    RX_DEMOD_FMW  = 6,
    RX_DEMOD_B1K  = 7
} rx_demod_t;


/*! \brief Top-level receiver class.
 *  \ingroup DSP
 *
 * This class encapsulates the GNU Radio flow graph for the receiver.
 * Front-ends should only control the receiver through the interface provided
 * by this class.
 *
 */
class receiver
{

public:
    /*! \brief Public contructor.
     *  \param input_device Input device specifier, e.g. hw:1 for FCD source.
     *  \param audio_device Audio output device specifier,
     *                      e.g. hw:0 when using ALSA or Portaudio.
     *
     * \todo Option to use UHD device instead of FCD.
     */
    receiver(const std::string input_device="", const std::string audio_device="");

    /*! \brief Public destructor. */
    ~receiver();

    /*! \brief Start the receiver. */
    void start();

    /*! \brief Stop the receiver. */
    void stop();

    /*! \brief Set RF frequency.
     *  \param freq_hz The desired frequency in Hz.
     *  \return True if an error occurs, e.g. the frequency is out of range.
     */
    rx_status_t set_rf_freq(float freq_hz);

    /*! \brief Set RF gain.
     *  \param gain_db The desired gain in dB.
     *  \return TRUE if an error occurs, e.g. the gain is out of valid range.
     */
    rx_status_t set_rf_gain(float gain_db);

    /*! \brief Set tuning offset.
     *  \param offset_hz The desired tuning offset in Hz.
     *  \return True if the tuning offset is out of range.
     *
     * This method sets a new tuning offset fir the receiver. The tuning offset is used
     * to tune within the passband, i.e. select a specific channel within the received
     * spectrum.
     *
     * The valid range for the tuning is +/- 0.5 * the bandwidth although this is just a
     * logical limit.
     */
    rx_status_t set_tuning_offset(float offset_hz);

    rx_status_t set_filter_low(float freq_hz);
    rx_status_t set_filter_high(float freq_hz);

    rx_status_t set_demod(rx_demod_t demod);

    rx_status_t set_af_gain(float gain_db);


private:
    gr_top_block_sptr d_tb;  /*! \brief The GNU Radio top block. */
    fcd_source_c_sptr d_fcd; /*! \brief Funcube Dongle source. */


    gr_null_sink_sptr d_sink;



protected:


};

#endif // RECEIVER_H
