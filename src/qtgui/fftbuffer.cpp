#include "fftbuffer.hpp"
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

    *xmin = 0;
    *xmax = width;

    float freqscale = (maxHz - minHz) / (float) width;
    float indexscale = (float) _data[line].size / (_data[line].maxFreq - _data[line].minFreq);
    float gain1 = (_data[line].maxDB - _data[line].minDB) / 255;
    float gain2 = 1. / (maxdB - mindB);

    *xmin = (qint32) ceil((_data[line].minFreq - minHz) / freqscale);
    *xmax = width;

    if(*xmin < 0) {
        *xmin = 0;
    }

    int i = *xmin;
    for( ; i < width; i++) {
        float f = minHz + i * freqscale;
        float j = (f - _data[line].minFreq) * indexscale;

        if(j > _data[line].size) {
            *xmax = i;
            break;
        }
        float v = _data[line].data[(int)(j + 0.5)];


        // v is now in [0..255]. Rescale to real dB values.
        v = _data[line].minDB + v * gain1;

        if(v < mindB) {
            v = 0;
        } else if(v > maxdB) {
            v = 1;
        } else {
            v = (v - mindB) * gain2;
        }
        out[i] = (qint32) (height * (1-v));
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
