#!/usr/bin/env python
#
# Simple multi mode receiver implemented in GNU Radio with Qt GUI
# This program is based on usrp_display.py from GNU Radio.
#
# Copyright 2009 Free Software Foundation, Inc.
# Copyright 2010 Alexandru Csete
# 
# GNU Radio and gqrx are free software; you can redistribute and/or modify
# them under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# GNU Radio and gqrx are distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

from gnuradio import gr, gru, audio, blks2
from gnuradio import uhd
from gnuradio import eng_notation
from gnuradio.gr import firdes
from gnuradio.eng_option import eng_option
from optparse import OptionParser
from datetime import datetime
import sys

try:
    from gnuradio.qtgui import qtgui
    from PyQt4 import QtGui, QtCore
    import sip
except ImportError:
    print "Please install gr-qtgui."
    sys.exit(1)

try:
    from gqrx_qtgui import Ui_MainWindow
except ImportError:
    print "Error: could not find gwrx_qtgui.py:"
    print "\t\"pyuic4 gqrx_qtgui.ui -o gqrx_qtgui.py\""
    sys.exit(1)


# USRP sample rates supported by the receiver
# 250k must be there and the others must be an integer multiple of 250k
srtable = [250000, 500000, 1000000, 2000000, 4000000]
srstr = ["250 kHz", "500 kHz", "1 MHz", "2 MHz", "4 MHz"]

# file format used for recordings
rec_format = "%Y.%m.%d-%H.%M.%S"


# Qt interface
class main_window(QtGui.QMainWindow):
    def __init__(self, snk, fg, parent=None):

        QtGui.QWidget.__init__(self, parent)
        self.gui = Ui_MainWindow()
        self.gui.setupUi(self)

        self.fg = fg

        # Add the qtsnk widgets to the layout box
        self.gui.sinkLayout.addWidget(snk)

        self.tuning_offset = 0

        # set up range for RF gain spin box
        self.gui.rfGainSpin.setRange(self.fg.gain_range["start"], self.fg.gain_range["stop"])
        self.gui.rfGainSpin.setValue(self.fg.options.gain)
        
        # Populate the bandwidth combo
        for srlabel in srstr:
            self.gui.bandwidthCombo.addItem(srlabel, None)

        # Populate the filter shape combo box and select "Normal"
        self.gui.filterShapeCombo.addItem("Soft", None)
        self.gui.filterShapeCombo.addItem("Normal", None)
        self.gui.filterShapeCombo.addItem("Sharp", None)
        self.gui.filterShapeCombo.setCurrentIndex(1)

        # Mode selector combo
        self.gui.modeCombo.addItem("AM", None)
        self.gui.modeCombo.addItem("FM-N", None)
        self.gui.modeCombo.addItem("FM-W", None)
        self.gui.modeCombo.addItem("LSB", None)
        self.gui.modeCombo.addItem("USB", None)
        self.gui.modeCombo.addItem("CW-L", None)
        self.gui.modeCombo.addItem("CW-U", None)
        self.gui.modeCombo.setCurrentIndex(1)
        
        # AGC selector combo
        self.gui.agcCombo.addItem("Fast", None)
        self.gui.agcCombo.addItem("Medium", None)
        self.gui.agcCombo.addItem("Slow", None)
        self.gui.agcCombo.addItem("Off", None)
        self.gui.agcCombo.setCurrentIndex(1)
        
        ### Disable functions that have not been implemented yet
        self.gui.recSpectrumButton.setEnabled(False)
        self.gui.agcCombo.setEnabled(False)  # There is an AGC block but with fixed values

        # Connect signals
        # Frequency controls
        self.connect(self.gui.freqUpBut1, QtCore.SIGNAL("clicked()"),
                     self.freqUpBut1Clicked)
        self.connect(self.gui.freqUpBut2, QtCore.SIGNAL("clicked()"),
                     self.freqUpBut2Clicked)
        self.connect(self.gui.freqDownBut1, QtCore.SIGNAL("clicked()"),
                     self.freqDownBut1Clicked)
        self.connect(self.gui.freqDownBut2, QtCore.SIGNAL("clicked()"),
                     self.freqDownBut2Clicked)
        self.connect(self.gui.frequencyEdit, QtCore.SIGNAL("editingFinished()"),
                     self.frequencyEditText)

        # Bandwidth selector
        self.connect(self.gui.bandwidthCombo, QtCore.SIGNAL("activated(int)"),
                     self.bandwidth_changed)

        # RF and BB gain
        self.connect(self.gui.rfGainSpin, QtCore.SIGNAL("valueChanged(int)"),
                     self.rfGainChanged)
        self.connect(self.gui.bbGainSpin, QtCore.SIGNAL("valueChanged(int)"),
                     self.bbGainChanged)


        # Pause
        self.connect(self.gui.pauseButton, QtCore.SIGNAL("clicked()"),
                     self.pauseFg)

        # Filter controls
        self.connect(self.gui.tuningSlider, QtCore.SIGNAL("valueChanged(int)"),
                     self.tuning_changed)
        self.connect(self.gui.filterWidthSlider, QtCore.SIGNAL("valueChanged(int)"),
                     self.filter_width_changed)
        self.connect(self.gui.filterCenterSlider, QtCore.SIGNAL("valueChanged(int)"),
                     self.filter_center_changed)
        self.connect(self.gui.filterShapeCombo, QtCore.SIGNAL("activated(int)"),
                     self.filter_shape_changed)

        # Mode change combo
        self.connect(self.gui.modeCombo, QtCore.SIGNAL("activated(int)"),
                     self.mode_changed)
                     
        # AGC selector combo
        self.connect(self.gui.agcCombo, QtCore.SIGNAL("activated(int)"),
                     self.agc_changed)
                     
        # Squelch threshold
        self.connect(self.gui.sqlSlider, QtCore.SIGNAL("valueChanged(int)"),
                     self.squelch_changed)
                     
        # Audio gain, recording and playback
        self.connect(self.gui.volSlider, QtCore.SIGNAL("valueChanged(int)"),
                     self.af_gain_changed)
        self.connect(self.gui.recAudioButton, QtCore.SIGNAL("toggled(bool)"),
                     self.af_rec_toggled)
        self.connect(self.gui.playAudioButton, QtCore.SIGNAL("toggled(bool)"),
                     self.af_play_toggled)

        # misc
        self.connect(self.gui.actionSaveData, QtCore.SIGNAL("activated()"),
                     self.saveData)
        self.gui.actionSaveData.setShortcut(QtGui.QKeySequence.Save)


    # Functions to set the values in the GUI
    def set_frequency(self, freq):
        self.freq = freq
        sfreq = eng_notation.num_to_str(self.freq)
        self.gui.frequencyEdit.setText(QtCore.QString("%1").arg(sfreq))
        self.update_rx_freq_label()

        
    # FIXME: not needed?
    def set_bandwidth(self, bw):
        "Update bandwidth selector combo"
        self.bw = bw
        #sbw = eng_notation.num_to_str(self.bw)
        #self.gui.bandwidthEdit.setText(QtCore.QString("%1").arg(sbw))
        if bw == 250000:
            self.gui.bandwidthCombo.setCurrentIndex(0)
        elif bw == 500000:
            self.gui.bandwidthCombo.setCurrentIndex(1)
        elif bw == 1000000:
            self.gui.bandwidthCombo.setCurrentIndex(2)
        elif bw == 2000000:
            self.gui.bandwidthCombo.setCurrentIndex(3)
        elif bw == 4000000:
            self.gui.bandwidthCombo.setCurrentIndex(4)
        else:
            print "Invalid bandwidth for bandwidthCombo: ", bw

       
    def set_filter_width_slider_value(self, width):
        """
        This function will update the state of the filter width slider.
        This will trigger the valueChanged() signal, which in turn will
        update the filter width of the receiver.
        """
        self.fw = width
        self.gui.filterWidthSlider.setValue(width)

    def set_filter_center_slider_value(self, offset):
        """
        This function will update the state of the filter center slider.
        This will trigger the valueChanged() signal, which in turn will
        update the filter width of the receiver.
        """
        self.fc = offset
        self.gui.filterCenterSlider.setValue(offset)


    def set_filter_width_range(self, lower=1000, upper=15000, step=100):
        """
        Set new lower and upper limit for the filter width widgets.
        The function updates both the range of the slider and the spin box.
        Standard ranges and step sizes:
           FM-W: 50-200 kHz with 1 kHz step
           AM and FM-N: 1-15 kHz with 100 Hz step
           LSB and USB: 1-5 kHz with 50 Hz step
           CW: 0.1-3 kHz with 10 Hz step
        """
        if lower > upper:
            print "Invalid filter range: ",lower," > ",upper
            return
        
        # slider
        self.gui.filterWidthSlider.setRange(lower, upper)
        self.gui.filterWidthSlider.setSingleStep(step)
        self.gui.filterWidthSlider.setPageStep(10*step)
        # spin box
        self.gui.filterWidthSpin.setRange(lower, upper)
        self.gui.filterWidthSpin.setSingleStep(step)


    def set_tuning_range(self, rng):
        """
        Set new tuning range.
        This function will update the limits of the tuning slider and spin box
        to +/- rng
        """
        self.gui.tuningSlider.setRange(-rng, rng)
        self.gui.tuningSpin.setRange(-rng, rng)

    def update_rx_freq_label(self):
        """
        Update RX frequency label. The RX frequency label shows the frequency
        we are actually receiving. This is a sum of USRP frequency and the offset
        of the frequency xlating filter.
        """
        # Calculate actual frequency in MHz
        rxfreq = (self.freq + self.tuning_offset) / 1e6
        #self.gui.rxFreqLabel.setText("RX: " + format(rxfreq, ".6f") + "MHz")
        self.gui.rxFreqLabel.setText(QtCore.QString("RX: %1 MHz").arg(format(rxfreq, ".6f")))

    # TODO: missing implementations, but do we really need them?


    # Functions called when signals are triggered in the GUI
    def pauseFg(self):
        if(self.gui.pauseButton.text() == "Pause"):
            self.fg.stop()
            self.fg.wait()
            self.gui.pauseButton.setText("Unpause")
        else:
            self.fg.start()
            self.gui.pauseButton.setText("Pause")
      
    
    def frequencyEditText(self):
        "Function called when a new frqeuency is entered into the text field."
        try:
            freq = eng_notation.str_to_num(self.gui.frequencyEdit.text().toAscii()) 
            self.fg.set_frequency(freq)
            self.freq = freq
        except RuntimeError:
            pass

    def freqUpBut1Clicked(self):
        """
        Function called when the > button is clicked.
        It increases the USRP frequency by 1/10 of the bandwidth.
        """
        self.freq += int(self.bw/10)
        self.fg.set_frequency(self.freq)

    def freqUpBut2Clicked(self):
        "Function called when the >> button is clicked."
        self.freq += self.bw
        self.fg.set_frequency(self.freq)

    def freqDownBut1Clicked(self):
        """
        Function called when the < button is clicked.
        It increases the USRP frequency by 1/10 of the bandwidth.
        """
        self.freq -= int(self.bw/10)
        self.fg.set_frequency(self.freq)

    def freqDownBut2Clicked(self):
        "Function called when the << button is clicked"
        self.freq -= self.bw
        self.fg.set_frequency(self.freq)
   
    def rfGainChanged(self, gain):
        "RF gain changed"
        self.gain = gain
        self.fg.set_rf_gain(gain)

    def bbGainChanged(self, gain):
        "Digital baseband gain changed (value in dB)"
        # convert from gain in dB to absolute gain
        self.bb_gain = 10.0 ** (gain/10.0)
        self.fg.set_bb_gain(self.bb_gain)
                
    def bandwidth_changed(self, bw):
        "New bandwidth selected."
        if bw < 5:
            nbw = srtable[bw]
        else:
            print "Invalid bandwidth: ", bw
            nbw = self.bw
        
        if nbw != self.bw:
            self.bw = nbw
            #print "New bandwidth: ", self.bw
            self.fg.set_sample_rate(self.bw)
        

        
    def tuning_changed(self, value):
        "Tuning value changed"
        self.tuning_offset = value
        self.fg.set_xlate_offset(value)
        self.update_rx_freq_label()

    def filter_width_changed(self, value):
        "Filter width changed."
        self.fw = value
        self.fg.set_filter_width(value)

    def filter_center_changed(self, value):
        "Filter center changed."
        self.fc = value
        #self.fg.set_filter_offset(value)
        self.fg.set_filter_offset(-value) # opposite when complex BPF is in the xlating_filter

    def filter_shape_changed(self, index):
        "Filter shape changed."
        self.fs = index
        self.fg.set_filter_shape(index)

    def mode_changed(self, mode):
        "New mode selected."
        self.mode = mode
        self.fg.set_mode(mode)

    def agc_changed(self, agc):
        "New AGC selected."
        self.agc = agc
        self.fg.set_agc(agc)

    def squelch_changed(self, sql):
        "New squelch threshold set."
        self.sql = sql
        self.fg.set_squelch(self.sql)

    def af_gain_changed(self, vol):
        "New AF gain value set."
        self.afg = vol
        self.fg.set_af_gain(vol/10.0) # slider is int 0-50, real value 0.0-5.0
        
    def af_rec_toggled(self, checked):
        """
        The REC button has been toggled. If "checked = True" then the REC button
        is in (checked) otherwise it is out (not checked)
        """
        if checked == True:
            self.fg.start_audio_recording()
        else:
            self.fg.stop_audio_recording()

    def af_play_toggled(self, checked):
        """
        The Play button has been toggled. If "checked = True" then the Play button
        is in (checked) otherwise it is out (not checked)
        """
        if checked == True:
             if self.fg.start_audio_playback():
                 # there was an error
                 self.gui.playAudioButton.setChecked(False)
                 
        else:
            self.fg.stop_audio_playback()



    def saveData(self):
        fileName = QtGui.QFileDialog.getSaveFileName(self, "Save data to file", ".");
        if(len(fileName)):
            self.fg.save_to_file(str(fileName))
        

class my_top_block(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        
        # Variables
        self._current_mode = 1     # initial mode is FMN
        self._xlate_offset = 0     # tuning offset of the xlating filter
        self._filter_offset = 0
        self._filter_low = -5000
        self._filter_high = 5000
        self._filter_trans = 2000
        self._agc_decay = 50e-6
        self._demod_rate = 50000    # sample rate at the input of demodulators (except WFM)
        self._audio_rate = 44100    # Sample rate of sound card
        self._cur_audio_rec = None  # Current audio recording
        self._prev_audio_rec = None # Previous audio recording (needed for speedy playback)
        

        parser = OptionParser(option_class=eng_option)
        parser.add_option("-w", "--which", type="int", default=0,
                          help="select which UHD device (0, 1, ...) default is %default",
                          metavar="NUM")  ## FIXME
        parser.add_option("-R", "--rx-subdev-spec", type="string", default="A:",
                          help="UHD subdevice specification [default=%default]")
        parser.add_option("-A", "--antenna", type="string", default=None,
                          help="select Rx Antenna (only on WBX and RFX boards)")
        parser.add_option("-S", "--sample-rate", type="int", default=250e3,
                          help="set receiver sample rate [default=%default]")
        parser.add_option("-W", "--bw", type="int", default=None,
                          help="set filter bandwidth (not supproted by all daughterboards)")
        parser.add_option("-f", "--freq", type="eng_float", default=None,
                          help="set frequency to FREQ", metavar="FREQ")
        parser.add_option("-g", "--gain", type="eng_float", default=None,
                          help="set RF gain in dB [default is midpoint]")
        parser.add_option("-a", "--ar", type="int", default=44100,
                          help="set sample rate for soundcard [default=%default]")
        parser.add_option("-O", "--audio-output", type="string", default="",
                          help="audio device name, e.g. plughw:0,0")
        parser.add_option("", "--fft-size", type="int", default=2048,
                          help="Set FFT frame size, [default=%default]");

        (options, args) = parser.parse_args()
        if len(args) != 0:
            parser.print_help()
            sys.exit(1)
        self.options = options
        self.show_debug_info = True

        # Call this before creating the Qt sink
        self.qapp = QtGui.QApplication(sys.argv)

        self._fftsize = options.fft_size

        self.u = uhd.single_usrp_source(device_addr="",   # FIXME: Add option
                                        io_type=uhd.io_type_t.COMPLEX_FLOAT32, # FIXME: Add option
                                        num_channels=1,
                                       )

        self.u.set_subdev_spec(options.rx_subdev_spec)
        
        if options.sample_rate in srtable:
            self._sample_rate = options.sample_rate
        else:
            self._sample_rate = srtable[0]

        self.u.set_samp_rate(self._sample_rate)
        # TODO: print actual sample rate

        # RF gain
        self.gain_range = { "start" : self.u.get_gain_range(0).start(),
                            "stop"  : self.u.get_gain_range(0).stop(),
                            "step"  : self.u.get_gain_range(0).step()
                           }
                           
        if options.gain is None:
            # if no gain was specified, use the mid-point in dB
            options.gain = float(self.gain_range["start"] + self.gain_range["stop"])/2

        self.set_rf_gain(options.gain)
        # TODO: print actual gain
        

        # intitial center frequency
        self._freq_range = { "start" : self.u.get_freq_range(0).start(),
                             "stop"  : self.u.get_freq_range(0).stop(),
                             "step"  : self.u.get_freq_range(0).step()
                            }
                            
        if options.freq is None:
            # if no frequency was specified, use the mid-point
            options.freq = float(self._freq_range["start"] + self._freq_range["stop"])/2

        self.set_frequency(options.freq)
        # TODO: print actual frequency


        # Select antenna connector
        if options.antenna is not None:
            print "Selecting antenna %s" % options.antenna
            self.u.set_antenna(options.antenna, 0)
            # -> self.uhd_single_usrp_source_0.set_antenna("RX2", 0)


        # set soundcard sample rate 
        self._audio_rate = options.ar

        # Create FFT scope and waterfall sinks
        self.snk = qtgui.sink_c(self._fftsize, firdes.WIN_BLACKMAN_hARRIS,
                                self._freq, self._sample_rate,
                                "USRP Display",
                                True, False, False, False)
      
        # frequency xlating filter used for tuning and decimation
        # to bring usrp rate down to 50 ksps regardless of USRP decimation (250k for FM-W)
        taps = firdes.complex_band_pass(1, self._sample_rate,
                                           self._filter_low,
                                           self._filter_high,
                                           self._filter_trans,
                                           firdes.WIN_HAMMING, 6.76)
        self.xlf = gr.freq_xlating_fir_filter_ccc(5,   # decimation 250k -> 50k
                                                  taps,
                                                  0,    # center offset
                                                  self._sample_rate)

        # Digital baseband gain used to increase signal level
        self.bb_gain = gr.multiply_const_cc(1.0)

        # Squelch
        # alpha determines the "hang time" but SNR also has influence on that
        self.sql = gr.simple_squelch_cc(threshold_db=-100.0, alpha=0.0003)

        # AGC (FIXME: parameters are from non-uhd version)
        self.agc = gr.agc2_cc(attack_rate=0.1,
                              decay_rate=self._agc_decay,
                              reference=0.5,
                              gain=1.0,
                              max_gain=0.6)

        # AM demodulator
        self.demod_am = blks2.am_demod_cf(channel_rate=self._demod_rate,
                                          audio_decim=1,
                                          audio_pass=5000,
                                          audio_stop=5500)

        # Narrow FM demodulator
        self.demod_fmn = blks2.nbfm_rx(audio_rate=self._demod_rate,
                                       quad_rate=self._demod_rate,
                                       tau=75e-6, max_dev=5e3)
        
        # Wide FM demodulator
        self.demod_fmw = blks2.wfm_rcv(quad_rate=250000,
                                       audio_decimation=5)
           
        # SSB/CW demodulator
        self.demod_ssb = gr.complex_to_real(1)

        # Select FM-N as default demodulator
        self.demod = self.demod_fmn
      
        # audio resampler 50k -> audio_rate (44.1k or 48k)
        interp = int(gru.lcm(self._demod_rate, self._audio_rate) / self._demod_rate)
        decim  = int(gru.lcm(self._demod_rate, self._audio_rate) / self._audio_rate)
        self.audio_rr = blks2.rational_resampler_fff(interpolation=interp,
                                                     decimation=decim,
                                                     taps=None,
                                                     fractional_bw=None)

        # audio gain and sink
        self.audio_gain = gr.multiply_const_ff(1.0)
        self.audio_sink = audio.sink(self._audio_rate, options.audio_output, True)

        # Audio recorder block
        # Create using dummy filename then close it right away
        self.audio_recorder = gr.wavfile_sink(filename="/dev/null",
                                              n_channels=1,
                                              sample_rate=self._audio_rate,
                                              bits_per_sample=16)
        self.audio_recorder.close()
        
        # NULL sink required during audio playback
        self.audio_nullsink = gr.null_sink(gr.sizeof_float)
        
        # audio_player is created when playback is started; however, we need
        # to declare it because (audio_player == None) is used to determine
        # whether a playback is ongoing or not (see audio_payback and recording functions)
        self.audio_player = None 

        # Connect the flow graph
        self.connect(self.u, self.bb_gain)
        self.connect(self.bb_gain, self.snk)
        self.connect(self.bb_gain, self.xlf, self.sql, self.demod,
                     self.audio_rr, self.audio_gain, self.audio_sink)


        # Get the reference pointer to the SpectrumDisplayForm QWidget
        # Wrap the pointer as a PyQt SIP object
        #     This can now be manipulated as a PyQt4.QtGui.QWidget
        self.pysink = sip.wrapinstance(self.snk.pyqwidget(), QtGui.QWidget)

        self.main_win = main_window(self.pysink, self)

        self.main_win.set_frequency(self._freq)
        self.main_win.set_bandwidth(self._sample_rate)

        # FIXME: Window title string
        #if self._rx_subdev_spec[0] == 0:
        #    self.main_win.setWindowTitle("GQRX: " + self.subdev.name() + " on side A")
        #else:
        #    self.main_win.setWindowTitle("GQRX: " + self.subdev.name() + " on side B")

        self.main_win.show()


    def save_to_file(self, name):
        # Pause the flow graph
        self.stop()
        self.wait()

        # Add file sink to save data
        self.file_sink = gr.file_sink(gr.sizeof_gr_complex, name)
        self.connect(self.amp, self.file_sink)

        # Restart flow graph
        self.start()

    def set_rf_gain(self, gain):
        "Set USRP gain in dB"
        self._gain = gain
        self.u.set_gain(self._gain, 0)

    def set_bb_gain(self, gain):
        "Set digital baseband gain (absolute value)"
        self._bb_gain = gain
        self.bb_gain.set_k(gain)

    def set_frequency(self, freq):
        """
        Tune USRP to new frequency.
        If tuning is successful, update the frequency entry widget and the spectrum display.
        """
        self._freq = freq
        r = self.u.set_center_freq(self._freq, 0)
        
        if r:
            #print "New freq BB:", r.baseband_freq, " DDC:", r.dxc_freq
            # FIXME
            print "TIF:", r.target_inter_freq, " AIF:", r.actual_inter_freq, " TDF:", r.target_dsp_freq, "ADF:", r.actual_dsp_freq
            try:
                self.main_win.set_frequency(self._freq)
                self.snk.set_frequency_range(self._freq, self._sample_rate)
            except:
                pass
        else:
            print "Failed to set frequency to ", self._freq


    def set_sample_rate(self, sr):
        "Set USRP sample rate"
        
        if sr not in srtable:
            print "Invalid sample rate: ", sr
            return

        self.lock()
        
        self._sample_rate = sr
        print "New sample rate:"

        self.u.set_samp_rate(self._sample_rate)
        
        print "  Set: ", self._sample_rate, "   Actual: ", self.u.get_samp_rate()

        # finally, update the tuning slider and spinbox
        self.main_win.set_tuning_range(int(self._sample_rate/2))

        # offset could be out of range when switching to a lower bandwidth
        # set_tuning_range() has already limited it, we just need to grab the new value
        self._xlate_offset = self.main_win.gui.tuningSlider.value()

        # disconnect filter
        self.disconnect(self.bb_gain, self.xlf, self.sql)

        # reconfigure frequency xlating filter
        # FIXME: I'm not exactly sure about this...
        del self.xlf
        taps = firdes.complex_band_pass(1, self._sample_rate,
                                        self._filter_low,
                                        self._filter_high,
                                        self._filter_trans,
                                        firdes.WIN_HAMMING, 6.76)

        # for FMW we deen 250ksps, all other modes 50ksps
        if self._current_mode == 2:
            xlf_decim = int(self._sample_rate/250000)
        else:
            xlf_decim = int(self._sample_rate/50000)
        
        self.xlf = gr.freq_xlating_fir_filter_ccc(xlf_decim,
                                                  taps,
                                                  self._xlate_offset, sr)
        print "  New filter decimation: ", xlf_decim

        # reconnect new filter
        self.connect(self.bb_gain, self.xlf, self.sql)

        try:
            self.snk.set_frequency_range(self._freq, self._sample_rate)
        except:
            pass
        
        self.unlock()
        
    def set_xlate_offset(self, offset):
        "Set xlating filter offset"
        self._xlate_offset = -offset
        self.xlf.set_center_freq(self._xlate_offset)

    def set_filter_width(self, width):
        "Set new filter bandpass filter width"
        self._filter_low = self._filter_offset - int(width/2)
        self._filter_high = self._filter_offset + int(width/2)
        self.xlf.set_taps(firdes.complex_band_pass(1, self._sample_rate,
                                                   self._filter_low,
                                                   self._filter_high,
                                                   self._filter_trans,
                                                   firdes.WIN_HAMMING, 6.76))


    def set_filter_offset(self, offset):
        "Set new offset for bandpass filter"
        self._filter_offset = offset
        width = self._filter_high - self._filter_low
        self._filter_low = self._filter_offset - int(width/2)
        self._filter_high = self._filter_offset + int(width/2)
        # we need to update the filter shape as we go
        self.set_filter_shape(self.main_win.gui.filterShapeCombo.currentIndex())
        self.xlf.set_taps(firdes.complex_band_pass(1, self._sample_rate,
                                                   self._filter_low,
                                                   self._filter_high,
                                                   self._filter_trans,
                                                   firdes.WIN_HAMMING, 6.76))

    def set_filter_shape(self, index):
        """
        Set the filter shape to soft, normal or sharp.
        The filter shape is determined by the transition width.
        Soft: 40% of the filter width
        Normal: 25% of the filter width
        Sharp: 10% of the filter width
        """
        width = self._filter_high - self._filter_low
        if index == 0:
            self._filter_trans = int(0.4*width)  # soft, 20% of filter width
        elif index == 1:
            self._filter_trans = int(0.25*width)  # normal, 10% of filter width
        elif index == 2:
            self._filter_trans = int(0.1*width) # sharp, 5% of filter width
        else:
            raise RuntimeError("Unknown filter shape")
            
        # lower than this will probably not work
        if self._filter_trans < 500:
            self._filter_trans = 500
            
        self.xlf.set_taps(firdes.complex_band_pass(1, self._sample_rate,
                                                   self._filter_low,
                                                   self._filter_high,
                                                   self._filter_trans,
                                                   firdes.WIN_HAMMING, 6.76))

    def set_mode(self, mode):
        """
        Set new operating mode. The parameter has a numeric value corresponding
        to the modes indicated below.
        Mode change consists of the following steps:
          1. Stop the flow graph
          2. Disconnect the squelch, demodulator, AGC, BPF and audio resampler
          3. Set new demodulator
          4. Reconnect the block
          5. ensure that filter parameters are consistent with new mode
          6. In case of transition to/from WFM, set new filter decimation
          7. Restart the flow graph
        """
        
        if mode == self._current_mode:
            return

        if ((mode == 2) or (self._current_mode == 2)):
            need_filter_reconf = True
        else:
            need_filter_reconf = False
        
        self.lock()
        
        # disconnect the blocks that need to be reconfigured
        if self._current_mode in [1,2]:
            # in FM mode we have neither AGC nor SSB downsampler
            self.disconnect(self.sql, self.demod, self.audio_rr)

        elif self._current_mode in [0,3,4,5,6]:
            # in AM, SSB and CW mode we have AGC
            self.disconnect(self.sql, self.agc, self.demod, self.audio_rr)
        else:
            raise RuntimeError("Invalid state self._current_mode = " + self._current_mode)

        if mode == 0:
            self.demod = self.demod_am
            self._fm_active = False
            self.connect(self.sql, self.agc, self.demod, self.audio_rr)
            self.main_win.set_filter_width_range(1000, 15000, 100)
            self.main_win.set_filter_center_slider_value(0)
            self.main_win.set_filter_width_slider_value(8000)
            print "New mode: AM"

        elif mode == 1:
            self.demod = self.demod_fmn
            self.connect(self.sql, self.demod, self.audio_rr)
            self.main_win.set_filter_width_range(1000, 15000, 100)
            self.main_win.set_filter_center_slider_value(0)
            self.main_win.set_filter_width_slider_value(10000)
            print "New mode: FM-N"

        elif mode == 2:
            self.demod = self.demod_fmw
            self.connect(self.sql, self.demod, self.audio_rr)
            self.main_win.set_filter_width_range(50000, 200000, 1000)
            self.main_win.set_filter_center_slider_value(0)
            self.main_win.set_filter_width_slider_value(160000)
            print "New mode: FM-W"

        elif mode == 3:
            #self.disconnect(self.agc, self.demod, self.resampler)
            self.demod = self.demod_ssb
            self.connect(self.sql, self.agc, self.demod, self.audio_rr)
            self.main_win.set_filter_width_range(1000, 5000, 50)
            self.main_win.set_filter_center_slider_value(-1500)
            self.main_win.set_filter_width_slider_value(2400)
            print "New mode: LSB"

        elif mode == 4:
            #self.disconnect(self.agc, self.demod, self.resampler)
            self.demod = self.demod_ssb
            self.connect(self.sql, self.agc, self.demod, self.audio_rr)
            self.main_win.set_filter_width_range(1000, 5000, 50)
            self.main_win.set_filter_center_slider_value(1500)
            self.main_win.set_filter_width_slider_value(2400)
            print "New mode: USB"

        elif mode == 5:
            #self.disconnect(self.agc, self.demod, self.resampler)
            self.demod = self.demod_ssb
            self.connect(self.sql, self.agc, self.demod, self.audio_rr)
            self.main_win.set_filter_width_range(100, 3000, 10)
            self.main_win.set_filter_center_slider_value(-700)
            self.main_win.set_filter_width_slider_value(1400)
            print "New mode: CW-L"

        elif mode == 6:
            #self.disconnect(self.agc, self.demod, self.resampler)
            self.demod = self.demod_ssb
            self.connect(self.sql, self.agc, self.demod, self.audio_rr)
            self.main_win.set_filter_width_range(100, 3000, 10)
            self.main_win.set_filter_center_slider_value(700)
            self.main_win.set_filter_width_slider_value(1400)
            print "New mode: CW-U"

        else:
            raise RuntimeError("Invalid mode requested: " + mode)

        # store the new mode
        self._current_mode = mode

        # if a tansition to/from FM-W has taken place we need to
        # reconfigure the filter decimation. We can do that by
        # simpl calling set_bandwidth(current_bandwidth)        
        if need_filter_reconf:
            self.set_sample_rate(self._sample_rate)

        # Restart the flow graph
        self.unlock()
    

    def set_agc(self, agc):
        """Set new AGC value"""
        if agc == 0:
            print "New AGC: Fast"
        elif agc == 1:
            print "New AGC: Medium (not implemented)"
        elif agc == 2:
            print "New AGC: Slow (not implemented)"
        elif agc == 3:
            print "New AGC: Off (not implemented)"
        else:
            print "Invalid AGC: ", agc

    def set_squelch(self, sql):
        """Set new squelch threshold (dB)"""
        self.sql.set_threshold(sql)
        

    def set_af_gain(self, afg):
        """Set new AF gain"""
        self.audio_gain.set_k(afg)
        
        
    def start_audio_recording(self):
        """    
        This function connects the wave file sink to the output of AF gain and
        starts recording the audio to a WAV file. If there already is an audio
        recording ongoing, the function will do nothing.
        """
        if self._cur_audio_rec != None:
            print "ERROR: Already recording audio: ", self._cur_audio_rec
            return
            
        # Generate new file name based on date and time
        self._cur_audio_rec = datetime.now().strftime(rec_format) + ".wav"
        print "Start audio recording: ", self._cur_audio_rec
        
        # Update GUI label
        self.main_win.gui.audioRecLabel.setText(self._cur_audio_rec)
        
        # open wav file and connect audio recorder
        self.lock()
        self.audio_recorder.open(self._cur_audio_rec)  # FIXME: correct order?
        self.connect(self.audio_gain, self.audio_recorder)
        self.unlock()


    def stop_audio_recording(self):
        """
        This function stops the ongoing audio recording (if any) and disconnects
        the wave file sink from the AF gain.
        """
        if self._cur_audio_rec == None:
            print "ERROR: There is no audio recording to stop"
            return

        # stop the flowgraph while we disconnect the audio recorder
        self.lock()
        self.disconnect(self.audio_gain, self.audio_recorder)  # FIXME: correct order?
        self.audio_recorder.close()

        self._prev_audio_rec = self._cur_audio_rec
        self._cur_audio_rec = None
        print "Audio recording stopped: ", self._prev_audio_rec

        # Restart the flowgraph
        self.unlock()


    def start_audio_playback(self):
        """
        This function starts playback of the latest audio recording. During the playback,
        the receiver flowgraph is routed to a null sink.
        The function does nothing if an audio recording is currently ongoing or there are
        no audio recordings to play.
        The audio playback will be repeating until switched off.
        """
        if self._cur_audio_rec != None:
            print "ERROR: Can not play while recording: ", self._cur_audio_rec
            return 1
            
        if self._prev_audio_rec == None:
            print "No audio recording to play"
            return 1
        
        if self.audio_player != None:
            print "ERROR: Already playing audio"
            return 1
        
        # stop the flowgraph while we re-wire
        self.lock()
        self.disconnect(self.audio_rr, self.audio_gain)
        self.connect(self.audio_rr, self.audio_nullsink)
        
        # gr.wavfile_source does not have public open() method like gr.wavfile_sink does
        # so we have to create a new instance every time :-(
        self.audio_player = gr.wavfile_source(filename=self._prev_audio_rec, repeat=True)
        self.connect(self.audio_player, self.audio_gain)
        print "Audio playback started: ", self._prev_audio_rec
        
        # restart the flowgraph
        self.unlock()
        return 0

    def stop_audio_playback(self):
        """
        This function stops the audio playback and reconnects the receiver path.
        """
        if self.audio_player == None:
            print "Audio playback not active."
            return
        
        # stop flowgraph while we re-wire
        self.lock()
        self.disconnect(self.audio_rr, self.audio_nullsink)
        self.disconnect(self.audio_player, self.audio_gain)
        
        # we must destroy the audio player object (new instance created at every play)
        del self.audio_player
        self.audio_player = None
        
        self.connect(self.audio_rr, self.audio_gain)
        print "Audio playback stopped: ", self._prev_audio_rec
        
        # restart the flowgraph
        self.unlock()


if __name__ == "__main__":
    tb = my_top_block();
    tb.start()
    tb.qapp.exec_()
