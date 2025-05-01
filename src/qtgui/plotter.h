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
#define MAX_HISTOGRAM_SIZE 128   // Multiples of 4 for alignment

#define PEAK_CLICK_MAX_H_DISTANCE 10 //Maximum horizontal distance of clicked point from peak
#define PEAK_CLICK_MAX_V_DISTANCE 20 //Maximum vertical distance of clicked point from peak
#define PEAK_WINDOW_HALF_WIDTH    10
#define PEAK_UPDATE_PERIOD       100 // msec
#define PLOTTER_UPDATE_LIMIT_MS   16 // 16ms = 62.5 Hz

#define MARKER_OFF std::numeric_limits<qint64>::min()

class CPlotter : public QFrame
{
    Q_OBJECT

public:
    explicit CPlotter(QWidget *parent = nullptr);
    ~CPlotter() override;

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    //void SetSdrInterface(CSdrInterface* ptr){m_pSdrInterface = ptr;}
    void draw(bool newData); //call to draw new fft data onto screen plot
    void setRunningState(bool running);
    void setClickResolution(int clickres) { m_ClickResolution = clickres; }
    void setFilterClickResolution(int clickres) { m_FilterClickResolution = clickres; }
    void setFilterBoxEnabled(bool enabled) { m_FilterBoxEnabled = enabled; }
    void setCenterLineEnabled(bool enabled) { m_CenterLineEnabled = enabled; }
    void setTooltipsEnabled(bool enabled) { m_TooltipsEnabled = enabled; }
    void setBookmarksEnabled(bool enabled) { m_BookmarksEnabled = enabled; }
    void setInvertScrolling(bool enabled) { m_InvertScrolling = enabled; }
    void setDXCSpotsEnabled(bool enabled) { m_DXCSpotsEnabled = enabled; }

    void setNewFftData(const float *fftData, int size);

    void setCenterFreq(quint64 f);
    void setFreqUnits(qint32 unit) { m_FreqUnits = unit; }

    void setDemodCenterFreq(quint64 f) { m_DemodCenterFreq = f; }

    /*! \brief Move the filter to freq_hz from center. */
    void setFilterOffset(qint64 freq_hz)
    {
        m_DemodCenterFreq = m_CenterFreq + freq_hz;
        updateOverlay();
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
        updateOverlay();
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
        updateOverlay();
    }

    void setVdivDelta(int delta) { m_VdivDelta = delta; }

    void setFreqDigits(int digits) { m_FreqDigits = digits>=0 ? digits : 0; }

    /* Determines full bandwidth. */
    void setSampleRate(float rate)
    {
        if (rate > 0.0f)
        {
            m_SampleFreq = rate;
            updateOverlay();
        }
    }

    float getSampleRate() const
    {
        return m_SampleFreq;
    }

    void setFftCenterFreq(qint64 f) {
        qint64 limit = ((qint64)m_SampleFreq - m_Span) / 2 - 1;
       // m_FftCenter = qBound(-limit, f, limit);
       // commented because it doesnt pass assert in debug build
    }

    qint64 getFftCenterFreq() const {
        return m_FftCenter;
    }

    int     getNearestPeak(QPoint pt);
    void    setWaterfallSpan(quint64 span_ms);
    quint64 getWfTimeRes() const;
    void    setFftRate(int rate_hz);
    void    clearWaterfallBuf();

    enum ePlotMode {
        PLOT_MODE_MAX = 0,
        PLOT_MODE_AVG = 1,
        PLOT_MODE_FILLED = 2,
        PLOT_MODE_HISTOGRAM = 3,
    };

    enum ePlotScale {
        PLOT_SCALE_DBFS = 0,
        PLOT_SCALE_DBV = 1,
        PLOT_SCALE_DBMW50 = 2
    };

    enum eWaterfallMode {
        WATERFALL_MODE_MAX = 0,
        WATERFALL_MODE_AVG = 1,
        WATERFALL_MODE_SYNC = 2
    };

signals:
    void newDemodFreq(qint64 freq, qint64 delta); /* delta is the offset from the center */
    void newLowCutFreq(int f);
    void newHighCutFreq(int f);
    void newFilterFreq(int low, int high);  /* substitute for NewLow / NewHigh */
    void pandapterRangeChanged(float min, float max);
    void newZoomLevel(float level);
    void newSize();
    void markerSelectA(qint64 freq);
    void markerSelectB(qint64 freq);

public slots:
    // zoom functions
    void resetHorizontalZoom();
    void moveToCenterFreq();
    void moveToDemodFreq();
    void zoomOnXAxis(float level);
    void setPlotMode(int mode);
    void setPlotScale(int mode, bool perHz);
    void setWaterfallMode(int mode);

    // other FFT slots
    void setFftPlotColor(const QColor& color);
    void enableFftFill(bool enabled);
    void enableMaxHold(bool enabled);
    void enableMinHold(bool enabled);
    void setFftAvg(float avg);
    void setFftRange(float min, float max);
    void setWfColormap(const QString &cmap);
    void setPandapterRange(float min, float max);
    void setWaterfallRange(float min, float max);
    void enablePeakDetect(bool enabled);
    void enableBandPlan(bool enable);
    void enableMarkers(bool enabled);
    void setMarkers(qint64 a, qint64 b);
    void clearWaterfall();
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
        TAG,
        MARKER_A,
        MARKER_B
    };

    void        drawOverlay();
    void        makeFrequencyStrs();
    int         xFromFreq(qint64 freq);
    qint64      freqFromX(int x);
    void        zoomStepX(float factor, int x);
    static qint64      roundFreq(qint64 freq, int resolution);
    quint64     msecFromY(int y);
    void        clampDemodParameters();
    static QColor      blend(QColor base, QColor over, int alpha255)
    {
        qreal alpha = alpha255 / 255.0;
        qreal oneMinusAlpha = 1.0 - alpha;
        return QColor(qRound(alpha * over.red()   + oneMinusAlpha * base.red()),
                      qRound(alpha * over.green() + oneMinusAlpha * base.green()),
                      qRound(alpha * over.blue()  + oneMinusAlpha * base.blue()));
    }
    static bool        isPointCloseTo(int x, int xr, int delta)
    {
        return ((x > (xr - delta)) && (x < (xr + delta)));
    }

    static void calcDivSize (qint64 low, qint64 high, int divswanted, qint64 &adjlow, qint64 &step, int& divs);
    void        showToolTip(QMouseEvent* event, QString toolTipText);

    bool        m_MaxHoldActive;
    bool        m_MaxHoldValid;
    bool        m_MinHoldActive;
    bool        m_MinHoldValid;
    bool        m_PeakDetectActive;
    bool        m_IIRValid;
    bool        m_histIIRValid;
    float       m_fftMaxBuf[MAX_SCREENSIZE]{};
    float       m_fftAvgBuf[MAX_SCREENSIZE]{};
    float       m_wfMaxBuf[MAX_SCREENSIZE]{};
    float       m_wfAvgBuf[MAX_SCREENSIZE]{};
    float       m_histogram[MAX_SCREENSIZE][MAX_HISTOGRAM_SIZE]{};
    float       m_histIIR[MAX_SCREENSIZE][MAX_HISTOGRAM_SIZE]{};
    QPointF     m_avgLineBuf[MAX_SCREENSIZE]{};
    QPointF     m_maxLineBuf[MAX_SCREENSIZE]{};
    QPointF     m_holdLineBuf[MAX_SCREENSIZE]{};
    float       m_histMaxIIR;
    std::vector<float> m_fftIIR;
    std::vector<float> m_fftData;
    std::vector<float> m_X;                // scratch array of matching size for local calculation
    float      m_wfbuf[MAX_SCREENSIZE]{}; // used for accumulating waterfall data at high time spans
    float       m_fftMaxHoldBuf[MAX_SCREENSIZE]{};
    float       m_fftMinHoldBuf[MAX_SCREENSIZE]{};
    float       m_peakSmoothBuf[MAX_SCREENSIZE]{}; // used in peak detection
    float      *m_wfData{};
    int         m_fftDataSize{};

    qreal       m_XAxisYCenter{};
    qreal       m_YAxisWidth{};

    eCapturetype    m_CursorCaptured;
    QPixmap     m_2DPixmap;         // Composite of everything displayed in the 2D plotter area
    QPixmap     m_OverlayPixmap;    // Grid, axes ... things that need to be drawn infrequently
    QPixmap     m_PeakPixmap;
    QImage      m_WaterfallImage;
    int         m_WaterfallOffset;
    QColor      m_ColorTbl[256];
    QSize       m_Size;
    qreal       m_DPR{};
    QString     m_HDivText[HORZ_DIVS_MAX+1];
    bool        m_Running;
    bool        m_DrawOverlay;
    qint64      m_CenterFreq;       // The HW frequency
    qint64      m_FftCenter;        // Center freq in the -span ... +span range
    qint64      m_DemodCenterFreq;
    qint64      m_MarkerFreqA{};
    qint64      m_MarkerFreqB{};
    qint64      m_StartFreqAdj{};
    qint64      m_FreqPerDiv{};
    bool        m_CenterLineEnabled;  /*!< Distinguish center line. */
    bool        m_FilterBoxEnabled;   /*!< Draw filter box. */
    bool        m_TooltipsEnabled;    /*!< Tooltips enabled */
    bool        m_BandPlanEnabled;    /*!< Show/hide band plan on spectrum */
    bool        m_BookmarksEnabled;   /*!< Show/hide bookmarks on spectrum */
    bool        m_MarkersEnabled;     /*!< Show/hide markers on spectrum */
    bool        m_InvertScrolling;
    bool        m_DXCSpotsEnabled;    /*!< Show/hide DXC Spots on spectrum */
    int         m_DemodHiCutFreq;
    int         m_DemodLowCutFreq;
    int         m_DemodFreqX{};       //screen coordinate x position
    int         m_DemodHiCutFreqX{};  //screen coordinate x position
    int         m_DemodLowCutFreqX{}; //screen coordinate x position
    int         m_MarkerAX{};
    int         m_MarkerBX{};
    int         m_CursorCaptureDelta;
    int         m_GrabPosition;
    qreal       m_Percent2DScreen;

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
    float       m_alpha;     /*!< IIR averaging. */

    qint64      m_Span;
    float       m_SampleFreq;    /*!< Sample rate. */
    qint32      m_FreqUnits;
    qint32      m_CumWheelDelta;
    qreal       m_ClickResolution;
    qreal       m_FilterClickResolution;
    ePlotMode   m_PlotMode;
    ePlotScale  m_PlotScale;
    bool        m_PlotPerHz;
    eWaterfallMode m_WaterfallMode;

    int         m_Xzero{};
    int         m_Yzero{};  /*!< Used to measure mouse drag direction. */
    int         m_FreqDigits;  /*!< Number of decimal digits in frequency strings. */

    QFont       m_Font;      /*!< Font used for plotter (system font) */
    qreal       m_VdivDelta; /*!< Minimum distance in pixels between two vertical grid lines (horizontal division). */
    qreal       m_BandPlanHeight; /*!< Height in pixels of band plan (if enabled) */

    quint32     m_LastSampleRate{};

    QColor      m_FftFillCol, m_FilledModeFillCol, m_FilledModeMaxLineCol, m_FilledModeAvgLineCol, m_MainLineCol, m_HoldLineCol;
    bool        m_FftFill{};

    QMap<int,qreal>   m_Peaks;

    QList< QPair<QRectF, qint64> >     m_Taglist;

    // Waterfall averaging
    quint64     tlast_wf_ms;        // last time waterfall has been updated
    quint64     tlast_plot_drawn_ms;// last time the plot was drawn
    quint64     tlast_wf_drawn_ms;  // last time waterfall was drawn
    quint64     wf_valid_since_ms;  // last time before action that invalidates time line
    double      msec_per_wfline{};  // milliseconds between waterfall updates
    quint64     wf_epoch;           // msec time of last waterfal rate change
    quint64     wf_count;           // waterfall lines drawn since last rate change
    quint64     wf_avg_count;       // number of frames averaged into wf buf
    quint64     tlast_peaks_ms;     // last time peaks were updated
    quint64     wf_span;            // waterfall span in milliseconds (0 = auto)
    int         fft_rate;           // expected FFT rate (needed when WF span is auto)
};

#endif // PLOTTER_H
