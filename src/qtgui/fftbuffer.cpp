#include "fftbuffer.h"
#include <math.h>

FftBuffer::FftBuffer()
    : _max_size(0)
    , _size(0)
    , _index(0)
    , _max_mem(1024*1024*256)
{
}

void
FftBuffer::clear()
{
    _index = 0;
    _size = 0;
}

void FftBuffer::addData(float *fftData, size_t size, qint64 minFreq, qint64 maxFreq)
{
    float min = fftData[0];
    float max = fftData[0];

    for(size_t i = 1; i < size; i++) {
        if(fftData[i] < min) {
            min = fftData[i];
        }
        if(fftData[i] > max) {
            max = fftData[i];
        }
    }

    if(_index < _size) {
        free(_data[_index].data);
    }
    _data[_index].data = (quint8*) malloc(size);
    _data[_index].minDB = min;
    _data[_index].maxDB = max;
    _data[_index].minFreq = minFreq;
    _data[_index].maxFreq = maxFreq;
    _data[_index].size = size;
    for(size_t i = 0; i < size; i++) {
        _data[_index].data[i] = (quint8)(255 * (fftData[i] - min) / (max-min));
    }

    _index++;
    _index %= _max_size;
    if(_size < _max_size) {
        _size++;
    }
}

bool FftBuffer::getLine(int line,
                        int height, int width,
                        float mindB, float maxdB,
                        qint64 minHz, qint64 maxHz,
                        int *xmin, int *xmax,
                        qint32 *out) const
{
    if(line > _size) {
        *xmin = *xmax = 0;
        return false;
    }
    line = _index - line;
    if(line < 0) {
        line += _max_size;
    }

    Row *row = _data + line;

    float freqscale = (maxHz - minHz) / (float) width;
    float indexscale = (float)row->size / (row->maxFreq - row->minFreq);
    float gain1 = (row->maxDB - row->minDB) / 255;
    float gain2 = 1. / (maxdB - mindB);

    *xmin = (qint32) ceil((row->minFreq - minHz) / freqscale);
    *xmax = (qint32) floor((row->maxFreq - minHz) / freqscale);

    if(*xmin < 0) {
        *xmin = 0;
    }
    if(*xmax > width) {
        *xmax = width;
    }

    qint64 source_start = (minHz - row->minFreq) * indexscale,
           source_end   = (maxHz - row->minFreq) * indexscale;

    if(source_end - source_start > *xmax - *xmin) {
        int xprev = -1;
        int x = 0;

        if(source_start < 0) {
            source_start = 0;
        }
        if(source_end > row->size) {
            source_end = row->size;
        }

        while(source_start < source_end) {
            x = (source_start - (minHz - row->minFreq) * indexscale) / indexscale / freqscale;
            float v = row->minDB + row->data[source_start] * gain1;
            if(v < mindB) {
                v = 0;
            } else if(v > maxdB) {
                v = 1;
            } else {
                v = (v - mindB) * gain2;
            }
            qint32 vi = (qint32) (height * (1-v));

            if(xprev != x || vi < out[x]) {
                xprev = x;
                out[x] = vi;
            }
            source_start++;
        }
    } else {
        int i = *xmin;
        for( ; i < *xmax; i++) {
            float f = minHz + i * freqscale;
            float j = (f - row->minFreq) * indexscale;
            float v = row->data[(int)(j + 0.5)];

            v = row->minDB + v * gain1;

            if(v < mindB) {
                v = 0;
            } else if(v > maxdB) {
                v = 1;
            } else {
                v = (v - mindB) * gain2;
            }
            out[i] = (qint32) (height * (1-v));
        }
    }

    return true;
}

void FftBuffer::setSize(size_t s) {
    // FIXME: Make this not drop existing data
    _index = 0;
    _size = 0;
    if(_data) {
        free(_data);
    }
    _data = (Row*) malloc(s * sizeof(Row));
    _max_size = s;
}
