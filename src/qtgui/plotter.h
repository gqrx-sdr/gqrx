/* -*- c++ -*- */
#ifndef PLOTTER_H
#define PLOTTER_H

#include <QtGui>
#include <QFont>
#include <QFrame>
#include <QImage>
#include <vector>
#include <QMap>

#define HORZ_DIVS_MAX 12    //50
#define VERT_DIVS_MIN 5
#define MAX_SCREENSIZE 16384

#define PEAK_CLICK_MAX_H_DISTANCE 10 //Maximum horizontal distance of clicked point from peak
#define PEAK_CLICK_MAX_V_DISTANCE 20 //Maximum vertical distance of clicked point from peak
#define PEAK_H_TOLERANCE 2


class CPlotter : public QFrame
{
    Q_OBJECT

public:
    explicit CPlotter(QWidget *parent = nullptr);
    ~CPlotter() override;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    //void SetSdrInterface(CSdrInterface* ptr){m_pSdrInterface = ptr;}
    void draw(); //call to draw new fft data onto screen plot
    void setRunningState(bool running) { m_Running = running; }
    void setClickResolution(int clickres) { m_ClickResolution = clickres; }
    void setFilterClickResolution(int clickres) { m_FilterClickResolution = clickres; }
    void setFilterBoxEnabled(bool enabled) { m_FilterBoxEnabled = enabled; }
    void setCenterLineEnabled(bool enabled) { m_CenterLineEnabled = enabled; }
    void setTooltipsEnabled(bool enabled) { m_TooltipsEnabled = enabled; }
    void setBookmarksEnabled(bool enabled) { m_BookmarksEnabled = enabled; }
    void setInvertScrolling(bool enabled) { m_InvertScrolling = enabled; }
    void setBandPlanEnabled(bool enabled) { m_BandPlanEnabled = enabled; }
    void setDXCSpotsEnabled(bool enabled) { m_DXCSpotsEnabled = enabled; }

    void setNewFftData(float *fftData, int size);
    void setNewFftData(float *fftData, float *wfData, int size);

    void setCenterFreq(quint64 f);
    void setFreqUnits(qint32 unit) { m_FreqUnits = unit; }

    void setDemodCenterFreq(quint64 f) { m_DemodCenterFreq = f; }

    /*! \brief Move the filter to freq_hz from center. */
    void setFilterOffset(qint64 freq_hz)
    {
        m_DemodCenterFreq = m_CenterFreq + freq_hz;
        drawOverlay();
    }
    qint64 getFilterOffset() const
    {
        return m_DemodCenterFreq - m_CenterFreq;
    }

    int getFilterBw() const
    {
        return m_DemodHiCutFreq - m_DemodLowCutFreq;
    }

    void setHiLowCutFrequencies(int LowCut, int HiCut)
    {
        m_DemodLowCutFreq = LowCut;
        m_DemodHiCutFreq = HiCut;
        drawOverlay();
    }

    void getHiLowCutFrequencies(int *LowCut, int *HiCut) const
    {
        *LowCut = m_DemodLowCutFreq;
        *HiCut = m_DemodHiCutFreq;
    }

    void setDemodRanges(int FLowCmin, int FLowCmax, int FHiCmin, int FHiCmax, bool symetric);

    /* Shown bandwidth around SetCenterFreq() */
    void setSpanFreq(quint32 s)
    {
        if (s > 0 && s < INT_MAX) {
            m_Span = (qint32)s;
            setFftCenterFreq(m_FftCenter);
        }
        drawOverlay();
    }

    void setHdivDelta(int delta) { m_HdivDelta = delta; }
    void setVdivDelta(int delta) { m_VdivDelta = delta; }

    void setFreqDigits(int digits) { m_FreqDigits = digits>=0 ? digits : 0; }

    /* Determines full bandwidth. */
    void setSampleRate(float rate)
    {
        if (rate > 0.0)
        {
            m_SampleFreq = rate;
            drawOverlay();
        }
    }

    float getSampleRate() const
    {
        return m_SampleFreq;
    }

    void setFftCenterFreq(qint64 f) {
        qint64 limit = ((qint64)m_SampleFreq + m_Span) / 2 - 1;
        m_FftCenter = qBound(-limit, f, limit);
    }

    int     getNearestPeak(QPoint pt);
    void    setWaterfallSpan(quint64 span_ms);
    quint64 getWfTimeRes() const;
    void    setFftRate(int rate_hz);
    void    clearWaterfall();
    bool    saveWaterfall(const QString & filename) const;

signals:
    void newDemodFreq(qint64 freq, qint64 delta); /* delta is the offset from the center */
    void newLowCutFreq(int f);
    void newHighCutFreq(int f);
    void newFilterFreq(int low, int high);  /* substitute for NewLow / NewHigh */
    void pandapterRangeChanged(float min, float max);
    void newZoomLevel(float level);
    void newSize();

public slots:
    // zoom functions
    void resetHorizontalZoom();
    void moveToCenterFreq();
    void moveToDemodFreq();
    void zoomOnXAxis(float level);

    // other FFT slots
    void setFftPlotColor(const QColor& color);
    void setFftFill(bool enabled);
    void setPeakHold(bool enabled);
    void setMinHold(bool enabled);
    void setFftRange(float min, float max);
    void setWfColormap(const QString &cmap);
    void setPandapterRange(float min, float max);
    void setWaterfallRange(float min, float max);
    void setPeakDetection(bool enabled, float c);
    void toggleBandPlan(bool state);
    void updateOverlay();

    void setPercent2DScreen(int percent)
    {
        m_Percent2DScreen = percent;
        m_Size = QSize(0,0);
        resizeEvent(nullptr);
    }

protected:
    //re-implemented widget event handlers
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent * event) override;
    void mousePressEvent(QMouseEvent * event) override;
    void mouseReleaseEvent(QMouseEvent * event) override;
    void wheelEvent( QWheelEvent * event ) override;

private:
    enum eCapturetype {
        NOCAP,
        LEFT,
        CENTER,
        RIGHT,
        YAXIS,
        XAXIS,
        TAG
    };

    void        drawOverlay();
    void        makeFrequencyStrs();
    int         xFromFreq(qint64 freq);
    qint64      freqFromX(int x);
    void        zoomStepX(float factor, int x);
    static qint64      roundFreq(qint64 freq, int resolution);
    quint64     msecFromY(int y);
    void        clampDemodParameters();
    static bool        isPointCloseTo(int x, int xr, int delta)
    {
        return ((x > (xr - delta)) && (x < (xr + delta)));
    }
    void getScreenIntegerFFTData(qint32 plotHeight, qint32 plotWidth,
                                 float maxdB, float mindB,
                                 qint64 startFreq, qint64 stopFreq,
                                 float *inBuf, qint32 *outBuf,
                                 qint32 *maxbin, qint32 *minbin) const;
    static void calcDivSize (qint64 low, qint64 high, int divswanted, qint64 &adjlow, qint64 &step, int& divs);
    void        showToolTip(QMouseEvent* event, QString toolTipText);

    bool        m_PeakHoldActive;
    bool        m_PeakHoldValid;
    bool        m_MinHoldActive;
    bool        m_MinHoldValid;
    qint32      m_fftbuf[MAX_SCREENSIZE]{};
    quint8      m_wfbuf[MAX_SCREENSIZE]{}; // used for accumulating waterfall data at high time spans
    qint32      m_fftPeakHoldBuf[MAX_SCREENSIZE]{};
    qint32      m_fftMinHoldBuf[MAX_SCREENSIZE]{};
    float      *m_fftData{};     /*! pointer to incoming FFT data */
    float      *m_wfData{};
    int         m_fftDataSize{};

    int         m_XAxisYCenter{};
    int         m_YAxisWidth{};

    eCapturetype    m_CursorCaptured;
    QPixmap     m_2DPixmap;
    QPixmap     m_OverlayPixmap;
    QPixmap     m_WaterfallPixmap;
    QColor      m_ColorTbl[256];
    QSize       m_Size;
    qreal       m_DPR{};
    QString     m_HDivText[HORZ_DIVS_MAX+1];
    bool        m_Running;
    bool        m_DrawOverlay;
    qint64      m_CenterFreq;       // The HW frequency
    qint64      m_FftCenter;        // Center freq in the -span ... +span range
    qint64      m_DemodCenterFreq;
    qint64      m_StartFreqAdj{};
    qint64      m_FreqPerDiv{};
    bool        m_CenterLineEnabled;  /*!< Distinguish center line. */
    bool        m_FilterBoxEnabled;   /*!< Draw filter box. */
    bool        m_TooltipsEnabled{};  /*!< Tooltips enabled */
    bool        m_BandPlanEnabled;    /*!< Show/hide band plan on spectrum */
    bool        m_BookmarksEnabled;   /*!< Show/hide bookmarks on spectrum */
    bool        m_InvertScrolling;
    bool        m_DXCSpotsEnabled;    /*!< Show/hide DXC Spots on spectrum */
    int         m_DemodHiCutFreq;
    int         m_DemodLowCutFreq;
    int         m_DemodFreqX{};       //screen coordinate x position
    int         m_DemodHiCutFreqX{};  //screen coordinate x position
    int         m_DemodLowCutFreqX{}; //screen coordinate x position
    int         m_CursorCaptureDelta;
    int         m_GrabPosition;
    int         m_Percent2DScreen;

    int         m_FLowCmin;
    int         m_FLowCmax;
    int         m_FHiCmin;
    int         m_FHiCmax;
    bool        m_symetric;

    int         m_HorDivs;   /*!< Current number of horizontal divisions. Calculated from width. */
    int         m_VerDivs;   /*!< Current number of vertical divisions. Calculated from height. */

    float       m_PandMindB;
    float       m_PandMaxdB;
    float       m_WfMindB;
    float       m_WfMaxdB;

    qint64      m_Span;
    float       m_SampleFreq;    /*!< Sample rate. */
    qint32      m_FreqUnits;
    int         m_ClickResolution;
    int         m_FilterClickResolution;

    int         m_Xzero{};
    int         m_Yzero{};  /*!< Used to measure mouse drag direction. */
    int         m_FreqDigits;  /*!< Number of decimal digits in frequency strings. */

    QFont       m_Font;      /*!< Font used for plotter (system font) */
    int         m_HdivDelta; /*!< Minimum distance in pixels between two horizontal grid lines (vertical division). */
    int         m_VdivDelta; /*!< Minimum distance in pixels between two vertical grid lines (horizontal division). */
    int         m_BandPlanHeight; /*!< Height in pixels of band plan (if enabled) */

    quint32     m_LastSampleRate{};

    QColor      m_FftColor, m_FftFillCol, m_PeakHoldColor, m_MinHoldColor;
    bool        m_FftFill{};

    float       m_PeakDetection{};
    QMap<int,int>   m_Peaks;

    QList< QPair<QRect, qint64> >     m_Taglist;

    // Waterfall averaging
    quint64     tlast_wf_ms;        // last time waterfall has been updated
    quint64     msec_per_wfline;    // milliseconds between waterfall updates
    quint64     wf_span;            // waterfall span in milliseconds (0 = auto)
    int         fft_rate;           // expected FFT rate (needed when WF span is auto)
};

#endif // PLOTTER_H
