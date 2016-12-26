#ifndef FFTBUFFER_HPP
#define FFTBUFFER_HPP
#include <stdlib.h>
#include <QtGlobal>

class FftBuffer
{
public:
    FftBuffer();

    void setSize(size_t s);
    size_t size() const;

    void addData(float *fftData, size_t size, qint64 minFreq, qint64 maxFreq);
    void clear();
    bool getLine(unsigned int age,
                 int height, int width,
                 float mindB, float maxdB,
                 qint64 minHz, qint64 maxHz,
                 int *xmin, int *xmax, qint32 *out) const;
    void setMaxMemory(quint64 mem);

private:
    struct Row {
        quint8 *data;
        size_t size;
        qint64 minFreq, maxFreq;
        float minDB, maxDB;
    };

    size_t _max_size;
    size_t _size;
    size_t _index;
    size_t _max_mem;

    struct Row *_data;
};

#endif // FFTBUFFER_HPP
