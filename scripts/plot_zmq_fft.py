#!/usr/bin/env python

# Read data file and plot.

import sys
import json
import numpy as np
from scipy import signal as sig
from scipy import ndimage as ndi
import matplotlib.pyplot as plt
import matplotlib.colors as colors

URI = 'ipc:///tmp/gqrx_data'
TOPIC = b'data.fft.linear'
META_FILE = '/tmp/fft.meta'
DATA_FILE = '/tmp/fft.data'
STD_FILE = '/tmp/fft.std'

META_FILE = '/tmp/fft.meta'
DATA_FILE = '/tmp/fft.data'

NUM_BINS = 0 # all bins
# NUM_BINS = 2000
START_BIN = 635000

LENGTH = 0 # all
START_TIME = 0

WIDTH = 0
HEIGHT = 0


# Create gqrx colormap for matplotlib, based on code in plotter.cpp
# The viridis, plasma, and turbo maps are already available in NumPy.
def gqrx_colormap():
    cmap_data = []
    for i in range(256):
        if (i < 20):
            cmap_data.append((0, 0, 0))
        elif ((i >= 20) and (i < 70)):
            cmap_data.append((0, 0, 140*(i-20)/50))
        elif ((i >= 70) and (i < 100)):
            cmap_data.append((60*(i-70)/30, 125*(i-70)/30, 115*(i-70)/30 + 140))
        elif ((i >= 100) and (i < 150)):
            cmap_data.append((195*(i-100)/50 + 60, 130*(i-100)/50 + 125, 255-(255*(i-100)/50)))
        elif ((i >= 150) and (i < 250)):
            cmap_data.append((255, 255-255*(i-150)/100, 0))
        elif (i >= 250):
            cmap_data.append((255, 255*(i-250)/5, 255*(i-250)/5))

    cmap_data = [[c/255 for c in x] for x in cmap_data]
    cmap = colors.LinearSegmentedColormap.from_list(
        name='gqrx',
        colors=cmap_data,
        N=256,
        gamma=1.0)
    return cmap


cmap = gqrx_colormap()

# Read in metadata. Use first fftsize and rate found in metadata and assume it
# does not change
fftsize = None
fftdecim = None
rate = None
metas = []
with open(META_FILE, 'r') as metaf:
    for line in metaf:
        meta = json.loads(line)
        if fftsize is None:
            fftsize = meta.get('fftsize')
        if fftdecim is None:
            fftdecim = meta.get('fftdecim')
        if rate is None:
            rate = meta.get('rate')
        metas.append(meta)

if fftdecim:
    fftsize //= fftdecim

if len(metas) < 1:
    print('No data')
    sys.exit(1)

start_time = metas[0]['timestamp']
end_time = metas[-1]['timestamp']
print(f'{len(metas)} lines in {end_time - start_time:.3f} seconds')

a = np.memmap(DATA_FILE, mode='r', dtype=np.float32)
a = a.reshape((-1, fftsize))


# Use a subset of bins if requested
if NUM_BINS != 0:
    print(f'Using bins {START_BIN} to {START_BIN+NUM_BINS-1}')
    a = a[:, START_BIN:START_BIN+NUM_BINS]
if LENGTH != 0:
    print(f'Using interval {START_TIME} to {START_TIME+LENGTH-1}')
    a = a[START_TIME:LENGTH, :]

# Rescale bins if reqested
if WIDTH:
    print(f'Resampling to width {WIDTH}')
    a = sig.resample(a, WIDTH, axis=0)
if HEIGHT:
    print(f'Resampling to height {HEIGHT}')
    a = sig.resample(a, HEIGHT, axis=1)

# Apply power scale for units and effects of decimation and take log10.
#
# Uncomment for dBV/RBW
# pwr_scale = 1.0 / (fftsize * fftsize)
# Uncomment for dBm/Hz into 50 ohms
pwr_scale = 1000.0 / (2.0 * fftsize * rate * 50.0)
if fftdecim:
    pwr_scale /= fftdecim
a_dBm = 10.0 * np.log10(a * pwr_scale)

# Statistics
avg_db = np.average(a_dBm)
std_db = np.std(a_dBm)
min_db = np.min(a_dBm)
max_db = np.max(a_dBm)

# Set plot min/max using statistics
plot_min_db = avg_db - 3 * std_db
plot_max_db = avg_db + 6 * std_db

print(f'Drawing, min dB {min_db}, max dB {max_db}')
plt.style.use('dark_background')
plt.rcParams['lines.linewidth'] = 0.5
# plt.style.use('Solarize_Light2')
plot = plt.pcolormesh(a_dBm, vmin=plot_min_db, vmax=plot_max_db, cmap=cmap, antialiased=False)
plot = plt.pcolormesh(a_dBm, antialiased=False)
# colorbar(plot)
plt.show()
sys.exit()

fig, ax = plt.subplots(nrows=7)

dbm_avg_v = np.average(a_dBm, axis=0)
dbm_std_v = np.std(a_dBm, axis=0)
dbm_max_v = np.max(a_dBm, axis=0)

diffs = np.diff(a_dBm)
box_width = 5
box = np.ones(box_width) / box_width

# diffs_avg = np.average(diffs, axis=0)
diffs_avg = np.diff(dbm_avg_v)
diffs_avg_smooth = np.convolve(diffs_avg, box, 'same')
diffs_smooth_std = np.std(diffs_avg_smooth)
diffs_thresh = np.abs(diffs_avg_smooth) > diffs_smooth_std
# iterations = int(box_width/2 - 1)
iterations = 2
diffs_eroded = ndi.binary_erosion(diffs_thresh, iterations=iterations)

diff_avg_v = np.convolve(np.average(diffs, axis=0), box, 'same')
diff_ero_v = diffs_eroded * np.sign(diffs_avg_smooth)

ax[0].plot(dbm_max_v)
ax[1].plot(dbm_std_v)
ax[2].plot(dbm_avg_v)
ax[3].plot(diffs_avg)
ax[4].plot(diffs_avg_smooth)
ax[5].plot(diffs_thresh)
ax[6].plot(diff_ero_v)
# ax5.plot(diffs_smooth_std)
# ax6.plot(diffs_std)

plt.show()
