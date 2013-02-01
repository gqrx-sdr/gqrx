/* -*- c++ -*- */
#ifndef PLOTTER_H
#define PLOTTER_H

#include <QtGui>
#include <QFrame>
#include <QImage>
#include <vector>

#define HORZ_DIVS_MAX 50 //12
#define MAX_SCREENSIZE 4096

class CPlotter : public QFrame
{
    Q_OBJECT

public:
    explicit CPlotter(QWidget *parent = 0);
    ~CPlotter();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

    //void SetSdrInterface(CSdrInterface* ptr){m_pSdrInterface = ptr;}
    void draw();		//call to draw new fft data onto screen plot
    void setRunningState(bool running) { m_Running = running; }
    void setClickResolution(int clickres) { m_ClickResolution = clickres; }
    void setFilterClickResolution(int clickres) { m_FilterClickResolution = clickres; }
    void setFilterBoxEnabled(bool enabled) { m_FilterBoxEnabled = enabled; }
    void setCenterLineEnabled(bool enabled) { m_CenterLineEnabled = enabled; }
    void setPercent2DScreen(int percent)
    {
        m_Percent2DScreen = percent;
        m_Size = QSize(0,0);
        resizeEvent(NULL);
    }

    void setNewFttData(double *fftData, int size);
    void setNewFttData(double *fftData, double *wfData, int size);

    void setCenterFreq(quint64 f);
    void setFreqUnits(qint32 unit) { m_FreqUnits = unit; }

    void setDemodCenterFreq(quint64 f) { m_DemodCenterFreq = f; }

    /*! \brief Move the filter to freq_hz from center. */
    void setFilterOffset(qint64 freq_hz)
    {
        m_DemodCenterFreq = m_CenterFreq + freq_hz;
        drawOverlay();
    }
    qint64 getFilterOffset(void)
    {
        return m_DemodCenterFreq - m_CenterFreq;
    }

    void setHiLowCutFrequencies(int LowCut, int HiCut)
    {
        m_DemodLowCutFreq = LowCut;
        m_DemodHiCutFreq = HiCut;
        drawOverlay();
    }

    void setDemodRanges(int FLowCmin, int FLowCmax, int FHiCmin, int FHiCmax, bool symetric);

    /* Shown bandwidth around SetCenterFreq() */
    void setSpanFreq(quint32 s)
    {
        m_Span = (qint32)s;
        drawOverlay();
    }
    void updateOverlay() { drawOverlay(); }

    void setMaxDB(qint32 max);
    void setMinDB(qint32 min);
    void setMinMaxDB(qint32 min, qint32 max);

    void setFontSize(int points) { m_FontSize = points; }
    void setHdivDelta(int delta) { m_HdivDelta = delta; }
    void setVdivDelta(int delta) { m_VdivDelta = delta; }

    void setFreqDigits(int digits) { m_FreqDigits = digits>=0 ? digits : 0; }

    /* Determines full bandwidth. */
    void setSampleRate(double rate)
    {
        if (rate > 0.0)
        {
            m_SampleFreq = rate;
            drawOverlay();
        }
    }

    double getSampleRate(void)
    {
        return m_SampleFreq;
    }

    void setFftCenterFreq(qint64 f) { m_FftCenter = f; }

signals:
    void newCenterFreq(qint64 f);
    void newDemodFreq(qint64 freq, qint64 delta); /* delta is the offset from the center */
    void newLowCutFreq(int f);
    void newHighCutFreq(int f);
    void newFilterFreq(int low, int high);  /* substute for NewLow / NewHigh */

public slots:
    // zoom functions
    void resetHorizontalZoom(void);
    void moveToCenterFreq(void);
    void moveToDemodFreq(void);

protected:
    //re-implemented widget event handlers
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent* event);
    void mouseMoveEvent(QMouseEvent * event);
    void mousePressEvent(QMouseEvent * event);
    void mouseReleaseEvent(QMouseEvent * event);
    void wheelEvent( QWheelEvent * event );

private:
    enum eCapturetype {
        NONE,
        LEFT,
        CENTER,
        RIGHT,
        YAXIS,
        XAXIS
    };
    void drawOverlay();
    void makeFrequencyStrs();
    int xFromFreq(qint64 freq);
    qint64 freqFromX(int x);
    qint64 roundFreq(qint64 freq, int resolution);
    bool isPointCloseTo(int x, int xr, int delta){return ((x > (xr-delta) ) && ( x<(xr+delta)) );}
    void clampDemodParameters();

    void getScreenIntegerFFTData(qint32 plotHeight, qint32 plotWidth,
                                 double maxdB, double mindB,
                                 qint32 startFreq, qint32 stopFreq,
                                 double *inBuf, qint32 *outBuf,
                                 qint32 *maxbin, qint32 *minbin);

    qint32 m_fftbuf[MAX_SCREENSIZE];
    double *m_fftData;     /*! pointer to incoming FFT data */
    double *m_wfData;
    int     m_fftDataSize;

    int m_XAxisYCenter;
    int m_YAxisWidth;

    eCapturetype m_CursorCaptured;
    QPixmap m_2DPixmap;
    QPixmap m_OverlayPixmap;
    QPixmap m_WaterfallPixmap;
    QColor m_ColorTbl[256];
    QSize m_Size;
    QString m_Str;
    QString m_HDivText[HORZ_DIVS_MAX+1];
    bool m_Running;
    bool m_DrawOverlay;
    qint64 m_CenterFreq;
    qint64 m_FftCenter;
    qint64 m_DemodCenterFreq;
    bool m_CenterLineEnabled;  /*!< Distinguish center line. */
    bool m_FilterBoxEnabled;   /*!< Draw filter box. */
    int m_DemodHiCutFreq;
    int m_DemodLowCutFreq;
    int m_DemodFreqX;		//screen coordinate x position
    int m_DemodHiCutFreqX;	//screen coordinate x position
    int m_DemodLowCutFreqX;	//screen coordinate x position
    int m_CursorCaptureDelta;
    int m_GrabPosition;
    int m_Percent2DScreen;

    int m_FLowCmin;
    int m_FLowCmax;
    int m_FHiCmin;
    int m_FHiCmax;
    bool m_symetric;

    int    m_HorDivs;   /*!< Current number of horizontal divisions. Calculated from width. */
    int    m_VerDivs;   /*!< Current number of vertical divisions. Calculated from height. */
    qint32 m_MaxdB;
    qint32 m_MindB;
    qint32 m_dBStepSize;
    qint32 m_Span;
    double m_SampleFreq;    /*!< Sample rate. */
    qint32 m_FreqUnits;
    int m_ClickResolution;
    int m_FilterClickResolution;

    int m_Xzero;
    int m_Yzero;  /*!< Used to measure mouse drag direction. */
    int m_FreqDigits;  /*!< Number of decimal digits in frequency strings. */

    int m_FontSize;  /*!< Font size in points. */
    int m_HdivDelta; /*!< Minimum distance in pixels between two horizontal grid lines (vertical division). */
    int m_VdivDelta; /*!< Minimum distance in pixels between two vertical grid lines (horizontal division). */

    quint32 m_LastSampleRate;

};

#endif // PLOTTER_H
