#!/usr/bin/env python
#
# Simple multi mode receiver implemented in GNU Radio with Qt GUI
# This program is based on usrp_display.py from GNU Radio.
#
# Copyright 2009 Free Software Foundation, Inc.
# Copyright 2010 Alexandru Csete
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

from gnuradio import gr, audio, blks2
from gnuradio import usrp
from gnuradio import eng_notation
from gnuradio.gr import firdes
from gnuradio.eng_option import eng_option
from optparse import OptionParser
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
    print "Error: could not find grx_qtgui.py:"
    print "\t\"pyuic4 grx_qtgui.ui -o grx_qtgui.py\""
    sys.exit(1)



# Qt interface
class main_window(QtGui.QMainWindow):
    def __init__(self, snk, fg, parent=None):

        QtGui.QWidget.__init__(self, parent)
        self.gui = Ui_MainWindow()
        self.gui.setupUi(self)

        self.fg = fg

        # Add the qtsnk widgets to the layout box
        self.gui.sinkLayout.addWidget(snk)

        # set up range for RF gain spin box
        g = self.fg.subdev.gain_range()
        self.gui.gainSpin.setRange(g[0], g[1])
        self.gui.gainSpin.setValue(self.fg.options.gain)
        
        # Populate the Bandwidth combo and select 250 kHz
        self.gui.bandwidthCombo.addItem("250 kHz", None)
        self.gui.bandwidthCombo.addItem("500 kHz", None)
        self.gui.bandwidthCombo.addItem("1 MHz", None)
        self.gui.bandwidthCombo.addItem("2 MHz", None)
        self.gui.bandwidthCombo.addItem("4 MHz", None)
        self.gui.bandwidthCombo.setEnabled(False)
        
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
        
        # AGC selector combo
        self.gui.agcCombo.addItem("Fast", None)
        self.gui.agcCombo.addItem("Medium", None)
        self.gui.agcCombo.addItem("Slow", None)
        self.gui.agcCombo.addItem("Off", None)
        self.gui.agcCombo.setCurrentIndex(1)

        # Connect up some signals
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
        #self.connect(self.gui.bandwidthEdit, QtCore.SIGNAL("editingFinished()"),
        #             self.bandwidthEditText)
        
        self.connect(self.gui.gainSpin, QtCore.SIGNAL("valueChanged(int)"),
                     self.gainChanged)
        self.connect(self.gui.pauseButton, QtCore.SIGNAL("clicked()"),
                     self.pauseFg)
                     
        # Filter controls
        self.connect(self.gui.tuningSlider, QtCore.SIGNAL("valueChanged(int)"),
                     self.tuningValueSet)
        self.connect(self.gui.filterWidthSlider, QtCore.SIGNAL("valueChanged(int)"),
                     self.filterWidthSet)
        self.connect(self.gui.filterCenterSlider, QtCore.SIGNAL("valueChanged(int)"),
                     self.filterCenterSet)
        self.connect(self.gui.filterShapeCombo, QtCore.SIGNAL("activated(int)"),
                     self.filterShapeChanged)

        # Mode change combo
        self.connect(self.gui.modeCombo, QtCore.SIGNAL("activated(int)"),
                     self.modeChanged)
                     
        # AGC selector combo
        self.connect(self.gui.agcCombo, QtCore.SIGNAL("activated(int)"),
                     self.agcChanged)
                     
        # Squelch threshold
        self.connect(self.gui.sqlSlider, QtCore.SIGNAL("valueChanged(int)"),
                     self.squelchSet)

        # misc
        self.connect(self.gui.actionSaveData, QtCore.SIGNAL("activated()"),
                     self.saveData)
        self.gui.actionSaveData.setShortcut(QtGui.QKeySequence.Save)


    # Functions to set the values in the GUI
    def set_frequency(self, freq):
        self.freq = freq
        sfreq = eng_notation.num_to_str(self.freq)
        self.gui.frequencyEdit.setText(QtCore.QString("%1").arg(sfreq))
        
    def set_bandwidth(self, bw):
        self.bw = bw
        sbw = eng_notation.num_to_str(self.bw)
        self.gui.bandwidthEdit.setText(QtCore.QString("%1").arg(sbw))

    def set_amplifier(self, amp):
        self.amp = amp
        self.gui.amplifierEdit.setText(QtCore.QString("%1").arg(self.amp))

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
   
    def gainChanged(self, gain):
        self.gain = gain
        self.fg.set_gain(gain)     
                
    def bandwidthEditText(self):
        try:
            bw = eng_notation.str_to_num(self.gui.bandwidthEdit.text().toAscii())
            self.fg.set_bandwidth(bw)
            self.bw = bw
        except ValueError:
            pass
        
    def tuningValueSet(self, value):
        "Tuning value changed"
        self.fg.set_xlate_offset(value)

    def filterWidthSet(self, value):
        self.fw = value
        self.fg.set_filter_width(value)

    def filterCenterSet(self, value):
        self.fc = value
        self.fg.set_filter_offset(value)

    def filterShapeChanged(self, index):
        "Filter shape changed."
        self.fs = index
        self.fg.set_filter_shape(index)

    def modeChanged(self, mode):
        "New mode selected."
        self.mode = mode
        self.fg.set_mode(mode)

    def agcChanged(self, agc):
        "New AGC selected."
        self.agc = agc
        self.fg.set_agc(agc)

    def squelchSet(self, sql):
        "New squelch threshold set."
        self.sql = sql
        self.fg.set_squelch(sql)

    def saveData(self):
        fileName = QtGui.QFileDialog.getSaveFileName(self, "Save data to file", ".");
        if(len(fileName)):
            self.fg.save_to_file(str(fileName))
        
        
def pick_subdevice(u):
    """
    The user didn't specify a subdevice on the command line.
    If there's a daughterboard on A, select A.
    If there's a daughterboard on B, select B.
    Otherwise, select A.
    """
    if u.db(0, 0).dbid() >= 0:       # dbid is < 0 if there's no d'board or a problem
        return (0, 0)
    if u.db(1, 0).dbid() >= 0:
        return (1, 0)
    return (0, 0)

class my_top_block(gr.top_block):
    def __init__(self):
        gr.top_block.__init__(self)
        
        # Variables
        self._xlate_offset = 0        # tuning offset of the xlating filter
        self._filter_offset = 0
        self._filter_low = -5000
        self._filter_high = 5000
        self._filter_trans = 1000

        parser = OptionParser(option_class=eng_option)
        parser.add_option("-w", "--which", type="int", default=0,
                          help="select which USRP (0, 1, ...) default is %default",
			  metavar="NUM")
        parser.add_option("-R", "--rx-subdev-spec", type="subdev", default=None,
                          help="select USRP Rx side A or B (default=first one with a daughterboard)")
        parser.add_option("-A", "--antenna", default=None,
                          help="select Rx Antenna (only on RFX-series boards)")
        parser.add_option("-W", "--bw", type="int", default=250e3,
                          help="set bandwidth of receiver [default=%default]")
        parser.add_option("-f", "--freq", type="eng_float", default=None,
                          help="set frequency to FREQ", metavar="FREQ")
        parser.add_option("-g", "--gain", type="eng_float", default=None,
                          help="set gain in dB [default is midpoint]")
        parser.add_option("-8", "--width-8", action="store_true", default=False,
                          help="Enable 8-bit samples across USB")
        parser.add_option( "--no-hb", action="store_true", default=False,
                          help="don't use halfband filter in usrp")
        parser.add_option("-S", "--oscilloscope", action="store_true", default=False,
                          help="Enable oscilloscope display")
        parser.add_option("", "--avg-alpha", type="eng_float", default=1e-1,
                          help="Set fftsink averaging factor, [default=%default]")
        parser.add_option("", "--ref-scale", type="eng_float", default=13490.0,
                          help="Set dBFS=0dB input value, [default=%default]")
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

        self.u = usrp.source_c(which=options.which)
        self._adc_rate = self.u.converter_rate()
        self.set_bandwidth(options.bw)

        if options.rx_subdev_spec is None:
            options.rx_subdev_spec = pick_subdevice(self.u)
    
        self._rx_subdev_spec = options.rx_subdev_spec
        self.u.set_mux(usrp.determine_rx_mux_value(self.u, self._rx_subdev_spec))
        self.subdev = usrp.selected_subdev(self.u, self._rx_subdev_spec)

        self._gain_range = self.subdev.gain_range()
        if options.gain is None:
            # if no gain was specified, use the mid-point in dB
            g = self._gain_range
            options.gain = float(g[0]+g[1])/2
        self.set_gain(options.gain)

        if options.freq is None:
            # if no frequency was specified, use the mid-point of the subdev
            f = self.subdev.freq_range()
            options.freq = float(f[0]+f[1])/2
        self.set_frequency(options.freq)

        # FIXME: find a better name for snk
        self.snk = qtgui.sink_c(self._fftsize, firdes.WIN_BLACKMAN_hARRIS,
                                self._freq, self._bandwidth,
                                "USRP Display",
                                True, True, False, False, False)
      
        # frequency xlating filter used for tuning and first decimation
        # to bring "IF rate" down to 250 ksps regardless of USRP decimation
        self.xlf = gr.freq_xlating_fir_filter_ccc(1,
                      firdes.low_pass(1, 250000, 125000, 25000, firdes.WIN_HAMMING, 6.76),
                      0, 250000)
                                     
        # complex band pass filter 250 ksps
        self.bpf = gr.fir_filter_ccc(5, firdes.complex_band_pass(1,
            250000, self._filter_low, self._filter_high, self._filter_trans,
            firdes.WIN_HAMMING, 6.76))
                
                                                      
        # rational resampler 50k -> audio rate (44.1 or 48k)
        self.resampler = blks2.rational_resampler_ccc(interpolation=441,
                                                      decimation=500,
                                                      taps=None,
                                                      fractional_bw=None)
        # AM demodulator
        self.demod_am = blks2.am_demod_cf(channel_rate=44100,
                                       audio_decim=1,
                                       audio_pass=5000,
                                       audio_stop=5500)

        # TODO: FM demodulator
        
        # TODO: SSB demodulator

        # Select AM as default demodulator
        self.demod = self.demod_am
        
        # audio sink
        self.audio_sink = audio.sink(44100, "", True)
        

        # Connect the flow graph
        self.connect(self.u, self.snk)
        self.connect(self.u, self.xlf, self.bpf, self.resampler, self.demod, self.audio_sink)


        # Get the reference pointer to the SpectrumDisplayForm QWidget
        # Wrap the pointer as a PyQt SIP object
        #     This can now be manipulated as a PyQt4.QtGui.QWidget
        self.pysink = sip.wrapinstance(self.snk.pyqwidget(), QtGui.QWidget)

        self.main_win = main_window(self.pysink, self)

        self.main_win.set_frequency(self._freq)
        self.main_win.set_bandwidth(self._bandwidth)

        # Window title string
        if self._rx_subdev_spec[0] == 0:
            self.main_win.setWindowTitle("GQRX: " + self.subdev.name() + " on side A")
        else:
            self.main_win.setWindowTitle("GQRX: " + self.subdev.name() + " on side B")

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

    def set_gain(self, gain):
        "Set USRP gain"
        self._gain = gain
        self.subdev.set_gain(self._gain)


    def set_frequency(self, freq):
        """
        Tune USRP to new frequency.
        If tuning is successful, update the frequency entry widget and the spectrum display.
        """
        self._freq = freq
        r = self.u.tune(0, self.subdev, self._freq)

        if r:
            print "New freq BB:", r.baseband_freq, " DDC:", r.dxc_freq
            try:
                self.main_win.set_frequency(self._freq)
                self.snk.set_frequency_range(self._freq, self._bandwidth)
            except:
                pass
        else:
            print "Failed to set frequency to ", freq


    def set_bandwidth(self, bw):
        "Set USRP bandwidth"        
        self._bandwidth = bw
        self._decim = int(self._adc_rate / self._bandwidth)
        self.u.set_decim_rate(self._decim)

        try:
            self.snk.set_frequency_range(self._freq, self._bandwidth)
        except:
            pass

    def set_xlate_offset(self, offset):
        "Set xlating filter offset"
        self._xlate_offset = -offset
        self.xlf.set_center_freq(self._xlate_offset)

    def set_filter_width(self, width):
        "Set new filter bandpass filter width"
        self._filter_low = self._filter_offset - int(width/2)
        self._filter_high = self._filter_offset + int(width/2)
        self.bpf.set_taps(firdes.complex_band_pass(1, 250000,
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
        self.bpf.set_taps(firdes.complex_band_pass(1, 250000,
                                                   self._filter_low,
                                                   self._filter_high,
                                                   self._filter_trans,
                                                   firdes.WIN_HAMMING, 6.76))

    def set_filter_shape(self, index):
        """
        Set the filter shape to soft, normal or sharp.
        The filter shape is determined by the transition width.
        Soft: 20% of the filter width
        Normal: 10% of the filter width
        Sharp: 5% of the filter width
        """
        width = self._filter_high - self._filter_low
        if index == 0:
            self._filter_trans = int(0.2*width)  # soft, 20% of filter width
        elif index == 1:
            self._filter_trans = int(0.1*width)  # normal, 10% of filter width
        elif index == 2:
            self._filter_trans = int(0.05*width) # sharp, 5% of filter width
        else:
            raise RuntimeError("Unknown filter shape")
            
        self.bpf.set_taps(firdes.complex_band_pass(1, 250000,
                                                   self._filter_low,
                                                   self._filter_high,
                                                   self._filter_trans,
                                                   firdes.WIN_HAMMING, 6.76))

    def set_mode(self, mode):
        """
        Set new operating mode. The parameter has a numeric value corresponding
        to the modes indicated below
        """
        if mode == 0:
            print "New mode: AM"
        elif mode == 1:
            print "New mode: FM-N (not implemented)"
        elif mode == 2:
            print "New mode: FM-W (not implemented)"
        elif mode == 3:
            print "New mode: LSB (not implemented)"
        elif mode == 4:
            print "New mode: USB (not implemented)"
        elif mode == 5:
            print "New mode: CW-L (not implemented)"
        elif mode == 6:
            print "New mode: CW-U (not implemented)"
        else:
            print "Invalid mode: ", mode

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
        """Set new squelch threshold"""
        print "New squelch threshold: ", sql, " (not implemented)"


if __name__ == "__main__":
    tb = my_top_block();
    tb.start()
    tb.qapp.exec_()
