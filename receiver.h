#ifndef RECEIVER_H
#define RECEIVER_H

#include <gr_top_block.h>
#include <gr_null_source.h>
#include <gr_null_sink.h>
#include <gr_throttle.h>


/*! \defgroup DSP Digital signal processing library based on GNU Radio */

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
    /*! \brief Public contructor. */
    receiver();

    /*! \brief Public destructor. */
    ~receiver();

    /*! \brief Start the receiver. */
    void start();

    /*! \brief Stop the receiver. */
    void stop();


private:

    /*! \brief The GNU Radio top block. */
    gr_top_block_sptr tb;

    gr_null_source_sptr src;
    gr_null_sink_sptr sink;
    gr_throttle::sptr throttle;


protected:


};

#endif // RECEIVER_H
