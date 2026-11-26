#!/usr/bin/env python3

import sys
import argparse
import socket
import time

# Constants for IF offsets based on mode
USB_IF_OFFSET = -1500  # USB shifts IF down by 1.5kHz
LSB_IF_OFFSET = 1500   # LSB shifts IF up by 1.5kHz
AM_FM_IF_OFFSET = 0    # No shift for AM/FM
HELP_TEXT = '''
This panadapter script is designed for keeping Gqrx's waterfall
synchronized with a FT-991/A that has had its IF tapped.

You must have CAT control of your radio to get the dial frequency.
In this case, the command may be `rigctld -m 1035 -r /dev/ttyUSB0`  

Some notes about the FT-991/A:
Because the IF of the FT-991/A is inverted, in the Input Controls
section of Gqrx, you need to check 'Swap I/Q' 
The IF is adjusted by the radio by 1.5khz in each sideband mode, but
this script takes care of that for you.

Feel free to modify this script for your radio's IF and related quirks!
'''

def get_rig_freq(rs):
    """Get frequency and mode from the rig"""
    rs.send(b'm\n')
    mode_response = rs.recv(1024).decode().strip()
    mode = mode_response.splitlines()[0] if mode_response else "USB"
    is_usb = "USB" in mode.upper()
    is_lsb = "LSB" in mode.upper()
    is_am_fm = mode in ["AM", "FM", "WFM"]
    
    rs.send(b'f\n')
    freq_response = rs.recv(1024).decode().strip()
    freq = int(freq_response) if freq_response.isdigit() else 0
    
    return (freq, mode, is_usb, is_lsb, is_am_fm)

def get_gqrx_hw_freq(gs):
    """Get the current hardware tuning frequency from GQRX"""
    gs.send(b'gqrx_get_hw_freq\n')
    response = gs.recv(1024).decode().strip()
    print(response)
    try:
        return int(response)
    except:
        return 0

def set_gqrx_hw_freq(gs, freq):
    """Set the hardware tuning frequency in GQRX"""
    set_gqrx_lnb_lo(gs, 0)      # initialize to zero first, otherwise side effects of
                                # updating the UI will cause setting the frequency to be incorrect.
                                # we can just set it back afterwards if we want.
    set_gqrx_filter_offset(gs, 0)
    msg = f'gqrx_set_hw_freq {freq}\n'.encode()
    print(msg)
    gs.send(msg)
    res = gs.recv(1024).decode().strip()
    return res

def get_gqrx_filter_offset(gs):
    """Get the filter offset from GQRX"""
    gs.send(b'gqrx_get_filter_offset\n')
    response = gs.recv(1024).decode().strip()
    parts = response.split()
    if len(parts) > 1 and parts[-1].lstrip('-').isdigit():
        return int(parts[-1])
    return 0

def set_gqrx_filter_offset(gs, freq_hz):
    msg = f'gqrx_set_filter_offset {freq_hz}\n'
    gs.send(msg.encode())
    response = gs.recv(1024).decode().strip()
    return

def get_gqrx_freq(gs):
    """Get the demodulation frequency from GQRX"""
    gs.send(b'f\n')
    response = gs.recv(1024).decode().strip()
    return int(response) if response.isdigit() else 0

def set_gqrx_freq(gs, freq):
    """Set the demodulation frequency in GQRX"""
    gs.send(f'F {freq}\n'.encode())
    return gs.recv(1024).decode().strip()

def calc_lnb_lo(dial_freq, if_freq, mode):
    """Calculate the LNB_LO value based on dial frequency, IF, and mode"""
    # Determine the mode-specific IF offset
    if mode == "USB":
        mode_offset = USB_IF_OFFSET
    elif mode == "LSB":
        mode_offset = LSB_IF_OFFSET
    else:  # AM, FM, etc.
        mode_offset = AM_FM_IF_OFFSET
    
    # For FT991A IF at 69.45MHz:
    # If dial is at 144.000MHz, we want a signal at 144.000MHz to appear at 69.45MHz
    # LNB_LO should be: 144000000 - 69450000 = 74550000
    # This is essentially: dial_freq - if_freq - mode_offset
    lnb_lo = dial_freq - if_freq - mode_offset
    
    return lnb_lo

def set_gqrx_lnb_lo(gs, lnb_lo):
    """Set the LNB_LO value in GQRX"""
    gs.send(f'LNB_LO {lnb_lo}\n'.encode())
    return gs.recv(1024).decode().strip()

def freq_to_string(freq):
    """Format frequency for display"""
    freq_str = str(freq).rjust(9, '0')
    return freq_str[0:3] + "." + freq_str[3:6] + "." + freq_str[6:9]

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-ga', '--gqrx-address', type=str, default='localhost',
            help='address that Gqrx is listening on')
    parser.add_argument('-gp', '--gqrx-port', type=int, default=7356,
            help='remote control port configured in Gqrx')
    parser.add_argument('-ra', '--rigctld-address', type=str, default='localhost',
            help='address that rigctld is listening on')
    parser.add_argument('-rp', '--rigctld-port', type=int, default=4532,
            help='listening port of rigctld')
    parser.add_argument('-i', '--interval', type=float, default=1.0,
            help='update interval in seconds')
    parser.add_argument('-f', '--ifreq', type=float, default=69.45,
            help='intermediate frequency in MHz')
    parser.add_argument('--debug', action='store_true',
            help='Enable debug output')

    args = parser.parse_args()
    debug = args.debug
    if_freq = int(args.ifreq * 1e6)  # Convert to Hz
    
    print(HELP_TEXT)

    try:
        rs = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        rs.connect((args.rigctld_address, args.rigctld_port))
    except Exception as e:
        print(f'Connection to rigctld failed: {e}', file=sys.stderr)
        return 1

    try:
        gs = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        gs.connect((args.gqrx_address, args.gqrx_port))
    except Exception as e:
        print(f'Connection to Gqrx failed: {e}', file=sys.stderr)
        return 1

    try:
        # Always set the hardware frequency to IF frequency
        if debug:
            print(f"Setting hardware frequency to IF: {if_freq} Hz")
        
        set_gqrx_hw_freq(gs, if_freq)
        
        old_mode = ""
        old_rig_freq = -1
        
        while True:
            # Get the current rig frequency and mode
            (rig_freq, mode, is_usb, is_lsb, is_am_fm) = get_rig_freq(rs)
            
            # Get the current hardware frequency from GQRX
            hw_freq = get_gqrx_hw_freq(gs)
            if not hw_freq:
                time.sleep(args.interval)
                continue
            filter_offset = get_gqrx_filter_offset(gs)
            
            if debug:
                print(f"\nRig Frequency: {freq_to_string(rig_freq)} Hz, Mode: {mode}")
                print(f"GQRX Hardware Frequency: {freq_to_string(hw_freq)} Hz")
                print(f"GQRX Filter Offset: {filter_offset} Hz")
            
            # If the rig frequency or mode has changed, update the LNB_LO
            if rig_freq != old_rig_freq or mode != old_mode:
                # Calculate the correct LNB_LO based on dial frequency and mode
                lnb_lo = calc_lnb_lo(rig_freq, if_freq, mode)            
                
                # Make sure the hardware frequency is still at the IF
                if hw_freq != if_freq:
                    if debug:
                        print(f"Resetting hardware frequency to IF: {if_freq} Hz")
                    set_gqrx_hw_freq(gs, if_freq)

                if debug:
                    print(f"Updating LNB_LO to: {lnb_lo} Hz")
                # Set the LNB_LO in GQRX
                set_gqrx_lnb_lo(gs, lnb_lo)
            
            # Update last values
            old_rig_freq = rig_freq
            old_mode = mode
            
            # Wait for the next update interval
            time.sleep(args.interval)
            
    except KeyboardInterrupt:
        if debug:
            print("\nProgram terminated by user")
    except Exception as e:
        print(f'Unexpected error: {e}', file=sys.stderr)
        return 1
    finally:
        rs.close()
        gs.close()

if __name__ == '__main__':
    sys.exit(main() or 0)