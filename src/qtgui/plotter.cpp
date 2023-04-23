/* -*- c++ -*- */
/* + + +   This Software is released under the "Simplified BSD License"  + + +
 * Copyright 2010 Moe Wheatley. All rights reserved.
 * Copyright 2011-2013 Alexandru Csete OZ9AEC
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY Moe Wheatley ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Moe Wheatley OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Moe Wheatley.
 */
#include <cmath>
#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QFont>
#include <QPainter>
#include <QtGlobal>
#include <QToolTip>
#include "plotter.h"
#include "bandplan.h"
#include "bookmarks.h"
#include "dxc_spots.h"

Q_LOGGING_CATEGORY(plotter, "plotter")

#define CUR_CUT_DELTA         5     // cursor capture delta in pixels
#define CLICK_FREQ_RESOLUTION 100   // frequency rounding for set via click
#define VDIV_DELTA            30

#define FFT_MIN_DB     -160.f
#define FFT_MAX_DB      30.f

#define FILTER_WIDTH_MIN_HZ 200

// Colors of type QRgb in 0xAARRGGBB format (unsigned int)
#define PLOTTER_BGD_COLOR           0xFF1F1D1D
#define PLOTTER_GRID_COLOR          0xFF444242
#define PLOTTER_TEXT_COLOR          0xFFDADADA
#define PLOTTER_CENTER_LINE_COLOR   0xFF788296
#define PLOTTER_FILTER_LINE_COLOR   0xFFFF7171
#define PLOTTER_FILTER_BOX_COLOR    0xFFA0A0A4
#define PLOTTER_MARKER_COLOR        0XFF7FFF7F
// FIXME: Should cache the QColors also

#define HOR_MARGIN 5
#define VER_MARGIN 5

int F2B(float f)
{
    int b = (f >= 1.0 ? 255 : (f <= 0.0 ? 0 : (int)floor(f * 256.0)));
    return b;
}

static inline bool val_is_out_of_range(float val, float min, float max)
{
    return (val < min || val > max);
}

static inline bool out_of_range(float min, float max)
{
    return (val_is_out_of_range(min, FFT_MIN_DB, FFT_MAX_DB) ||
            val_is_out_of_range(max, FFT_MIN_DB, FFT_MAX_DB) ||
            max < min + 10.f);
}

#define STATUS_TIP \
    "Click, drag or scroll on spectrum to tune. " \
    "Drag and scroll X and Y axes for pan and zoom. " \
    "Drag filter edges to adjust filter."

CPlotter::CPlotter(QWidget *parent) : QFrame(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_PaintOnScreen,false);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setMouseTracking(true);

    m_TooltipsEnabled = false;
    setStatusTip(tr(STATUS_TIP));
    setWfColormap("gqrx");

    m_MaxHoldActive = false;
    m_MaxHoldValid = false;
    m_MinHoldActive = false;
    m_MinHoldValid = false;
    m_IIRValid = false;
    m_histIIRValid = false;
    m_alpha = 1.0;
    m_histMaxIIR = std::numeric_limits<float>::min();

    m_FftCenter = 0;
    m_CenterFreq = 144500000;
    m_DemodCenterFreq = 144500000;
    m_DemodHiCutFreq = 5000;
    m_DemodLowCutFreq = -5000;

    m_FLowCmin = -25000;
    m_FLowCmax = -1000;
    m_FHiCmin = 1000;
    m_FHiCmax = 25000;
    m_symetric = true;

    m_ClickResolution = CLICK_FREQ_RESOLUTION;
    m_FilterClickResolution = CLICK_FREQ_RESOLUTION;
    m_CursorCaptureDelta = CUR_CUT_DELTA;
    m_WaterfallMode = WATERFALL_MODE_MAX;
    m_PlotMode = PLOT_MODE_MAX;
    m_PlotScale = PLOT_SCALE_V;
    m_PlotPer = PLOT_PER_RBW;

    m_FilterBoxEnabled = true;
    m_CenterLineEnabled = true;
    m_MarkersEnabled = true;
    m_BandPlanEnabled = true;
    m_BookmarksEnabled = true;
    m_InvertScrolling = false;
    m_DXCSpotsEnabled = true;

    m_Span = 96000;
    m_SampleFreq = 96000;

    m_HorDivs = 12;
    m_VerDivs = 6;
    m_PandMaxdB = m_WfMaxdB = 0.f;
    m_PandMindB = m_WfMindB = FFT_MAX_DB;

    m_FreqUnits = 1000000;
    m_CursorCaptured = NOCAP;
    m_Running = false;
    m_DrawOverlay = true;
    m_Frozen = false;
    m_2DPixmap = QPixmap();
    m_OverlayPixmap = QPixmap();
    m_WaterfallPixmap = QPixmap();
    m_Size = QSize(0,0);
    m_GrabPosition = 0;
    m_Percent2DScreen = 35;	//percent of screen used for 2D display
    m_VdivDelta = VDIV_DELTA;
    m_BandPlanHeight = 0.0;

    m_FreqDigits = 6;

    m_Peaks = QMap<int,qreal>();
    enablePeakDetect(false);

    setFftPlotColor(QColor(0xFF,0xFF,0xFF,0xFF));
    enableFftFill(false);

    // always update waterfall
    tlast_wf_ms = 0;
    tlast_wf_drawn_ms = 0;
    msec_per_wfline = 0;
    tlast_peaks_ms = 0;
    wf_epoch = 0;
    wf_count = 0;
    wf_span = 0;
    fft_rate = 15;
}

CPlotter::~CPlotter()
= default;

QSize CPlotter::minimumSizeHint() const
{
    return {50, 50};
}

QSize CPlotter::sizeHint() const
{
    return {180, 180};
}

void CPlotter::mouseMoveEvent(QMouseEvent* event)
{
    QPoint pt = event->pos();

    int w = m_OverlayPixmap.width();
    int h = m_OverlayPixmap.height();
    int px = qRound((qreal)pt.x() * m_DPR);
    int py = qRound((qreal)pt.y() * m_DPR);
    QPoint ppos = QPoint(px, py);

    /* mouse enter / mouse leave events */
    if (py < h)
    {
        //is in Overlay bitmap region
        if (event->buttons() == Qt::NoButton)
        {
            bool onTag = false;
            if(py < 15 * 10) // FIXME
            {
                if(m_BookmarksEnabled || m_DXCSpotsEnabled)
                {
                    for(int i = 0; i < m_Taglist.size() && !onTag; i++)
                    {
                        if (m_Taglist[i].first.contains(ppos))
                            onTag = true;
                    }
                }
            }
            // if no mouse button monitor grab regions and change cursor icon
            if (onTag)
            {
                setCursor(QCursor(Qt::PointingHandCursor));
                m_CursorCaptured = TAG;
            }
            else if (isPointCloseTo(px, m_YAxisWidth/2, m_YAxisWidth/2))
            {
                if (YAXIS != m_CursorCaptured)
                    setCursor(QCursor(Qt::OpenHandCursor));
                m_CursorCaptured = YAXIS;
                if (m_TooltipsEnabled)
                    QToolTip::hideText();
            }
            else if (isPointCloseTo(py, m_XAxisYCenter, m_CursorCaptureDelta+20))
            {
                if (XAXIS != m_CursorCaptured)
                    setCursor(QCursor(Qt::OpenHandCursor));
                m_CursorCaptured = XAXIS;
                if (m_TooltipsEnabled)
                    QToolTip::hideText();
            }
            else if (isPointCloseTo(px, m_DemodFreqX, m_CursorCaptureDelta))
            {
                // in move demod box center frequency region
                if (CENTER != m_CursorCaptured)
                    setCursor(QCursor(Qt::SizeHorCursor));
                m_CursorCaptured = CENTER;
                if (m_TooltipsEnabled)
                    showToolTip(event, QString("Demod: %1 kHz").arg(m_DemodCenterFreq/1.e3, 0, 'f', 3));
            }
            else if (isPointCloseTo(px, m_DemodHiCutFreqX, m_CursorCaptureDelta))
            {
                // in move demod hicut region
                if (RIGHT != m_CursorCaptured)
                    setCursor(QCursor(Qt::SizeFDiagCursor));
                m_CursorCaptured = RIGHT;
                if (m_TooltipsEnabled)
                    showToolTip(event, QString("High cut: %1 Hz").arg(m_DemodHiCutFreq));
            }
            else if (isPointCloseTo(px, m_DemodLowCutFreqX, m_CursorCaptureDelta))
            {
                // in move demod lowcut region
                if (LEFT != m_CursorCaptured)
                    setCursor(QCursor(Qt::SizeBDiagCursor));
                m_CursorCaptured = LEFT;
                if (m_TooltipsEnabled)
                    showToolTip(event, QString("Low cut: %1 Hz").arg(m_DemodLowCutFreq));
            }
            else if (m_MarkersEnabled && isPointCloseTo(px, m_MarkerAX, m_CursorCaptureDelta))
            {
                if (MARKER_A != m_CursorCaptured && m_MarkerFreqA != MARKER_OFF)
                    setCursor(QCursor(Qt::OpenHandCursor));
                m_CursorCaptured = MARKER_A;
                if (m_TooltipsEnabled)
                    showToolTip(event, QString("Marker A: %1 kHz").arg(m_MarkerFreqA/1.e3, 0, 'f', 3));
            }
            else if (m_MarkersEnabled && isPointCloseTo(px, m_MarkerBX, m_CursorCaptureDelta))
            {
                if (MARKER_B != m_CursorCaptured && m_MarkerFreqB != MARKER_OFF)
                    setCursor(QCursor(Qt::OpenHandCursor));
                m_CursorCaptured = MARKER_B;
                if (m_TooltipsEnabled)
                    showToolTip(event, QString("Marker B: %1 kHz").arg(m_MarkerFreqB/1.e3, 0, 'f', 3));
            }
            else
            {	//if not near any grab boundaries
                if (NOCAP != m_CursorCaptured)
                {
                    setCursor(QCursor(Qt::ArrowCursor));
                    m_CursorCaptured = NOCAP;
                }
                if (m_TooltipsEnabled)
                {
                    QString toolTipText;
                    qint64 hoverFrequency = freqFromX(px);
                    toolTipText = QString("%1 kHz\nΔ %2 kHz")
                                          .arg(hoverFrequency/1.e3, 0, 'f', 3)
                                          .arg(locale().toString((hoverFrequency - m_DemodCenterFreq)/1.e3, 'f', 3));

                    QFontMetricsF metrics(m_Font);
                    qreal bandTopY = ((qreal)h) - metrics.height() - 2 * VER_MARGIN - m_BandPlanHeight;
                    QList<BandInfo> hoverBands = BandPlan::Get().getBandsEncompassing(hoverFrequency);
                    if(m_BandPlanEnabled && py > bandTopY && !hoverBands.empty())
                    {
                        for (auto & hoverBand : hoverBands)
                            toolTipText.append("\n" + hoverBand.name);
                    }
                    showToolTip(event, toolTipText);
                }
            }
            m_GrabPosition = 0;
        }
    }
    else
    {
        // not in Overlay region
        if (event->buttons() == Qt::NoButton)
        {
            if (NOCAP != m_CursorCaptured)
                setCursor(QCursor(Qt::ArrowCursor));

            m_CursorCaptured = NOCAP;
            m_GrabPosition = 0;
        }
        if (m_TooltipsEnabled)
        {
            QDateTime tt;
            tt.setMSecsSinceEpoch(msecFromY(py));

            showToolTip(event, QString("%1\n%2 kHz")
                                       .arg(tt.toString("yyyy.MM.dd hh:mm:ss.zzz"))
                                       .arg(freqFromX(px)/1.e3, 0, 'f', 3));
        }
    }
    // process mouse moves while in cursor capture modes
    if (YAXIS == m_CursorCaptured)
    {
        if (event->buttons() & Qt::LeftButton)
        {
            setCursor(QCursor(Qt::ClosedHandCursor));
            // move Y scale up/down
            qreal delta_px = m_Yzero - py;
            qreal delta_db = delta_px * fabs(m_PandMindB - m_PandMaxdB) /
                             (qreal)h;
            m_PandMindB -= delta_db;
            m_PandMaxdB -= delta_db;
            if (out_of_range(m_PandMindB, m_PandMaxdB))
            {
                m_PandMindB += delta_db;
                m_PandMaxdB += delta_db;
            }
            else
            {
                emit pandapterRangeChanged(m_PandMindB, m_PandMaxdB);
                updateOverlay();

                m_MaxHoldValid = false;
                m_MinHoldValid = false;
                m_histIIRValid = false;

                m_Yzero = py;

                updateOverlay();
            }
        }
    }
    else if (XAXIS == m_CursorCaptured)
    {
        if (event->buttons() & (Qt::LeftButton | Qt::MiddleButton))
        {
            setCursor(QCursor(Qt::ClosedHandCursor));
            // pan viewable range or move center frequency
            int delta_px = m_Xzero - px;
            qint64 delta_hz = qRound64((qreal)delta_px * (qreal)m_Span / (qreal)w);
            if (delta_hz != 0) // update m_Xzero only on real change
            {
                if (event->buttons() & Qt::MiddleButton)
                {
                    m_CenterFreq += delta_hz;
                    m_DemodCenterFreq += delta_hz;
                    emit newDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq - m_CenterFreq);
                }
                else
                {
                    setFftCenterFreq(m_FftCenter + delta_hz);
                }

                m_MaxHoldValid = false;
                m_MinHoldValid = false;
                m_histIIRValid = false;

                m_Xzero = px;

                updateOverlay();
            }
        }
    }
    else if (LEFT == m_CursorCaptured)
    {
        // moving in demod lowcut region
        if (event->buttons() & (Qt::LeftButton | Qt::RightButton))
        {
            // moving in demod lowcut region with left button held
            if (m_GrabPosition != 0)
            {
                m_DemodLowCutFreq = freqFromX(px - m_GrabPosition ) - m_DemodCenterFreq;
                m_DemodLowCutFreq = std::min(m_DemodLowCutFreq, m_DemodHiCutFreq - FILTER_WIDTH_MIN_HZ);
                m_DemodLowCutFreq = roundFreq(m_DemodLowCutFreq, m_FilterClickResolution);

                if (m_symetric && (event->buttons() & Qt::LeftButton))  // symmetric adjustment
                {
                    m_DemodHiCutFreq = -m_DemodLowCutFreq;
                }
                clampDemodParameters();

                emit newFilterFreq(m_DemodLowCutFreq, m_DemodHiCutFreq);
                updateOverlay();
            }
            else
            {
                // save initial grab position from m_DemodFreqX
                m_GrabPosition = px-m_DemodLowCutFreqX;
            }
        }
        else if (event->buttons() & ~Qt::NoButton)
        {
            setCursor(QCursor(Qt::ArrowCursor));
            m_CursorCaptured = NOCAP;
        }
    }
    else if (RIGHT == m_CursorCaptured)
    {
        // moving in demod highcut region
        if (event->buttons() & (Qt::LeftButton | Qt::RightButton))
        {
            // moving in demod highcut region with right button held
            if (m_GrabPosition != 0)
            {
                m_DemodHiCutFreq = freqFromX(px-m_GrabPosition) - m_DemodCenterFreq;
                m_DemodHiCutFreq = std::max(m_DemodHiCutFreq, m_DemodLowCutFreq + FILTER_WIDTH_MIN_HZ);
                m_DemodHiCutFreq = roundFreq(m_DemodHiCutFreq, m_FilterClickResolution);

                if (m_symetric && (event->buttons() & Qt::LeftButton)) // symmetric adjustment
                {
                    m_DemodLowCutFreq = -m_DemodHiCutFreq;
                }
                clampDemodParameters();

                emit newFilterFreq(m_DemodLowCutFreq, m_DemodHiCutFreq);
                updateOverlay();
            }
            else
            {
                // save initial grab position from m_DemodFreqX
                m_GrabPosition = px - m_DemodHiCutFreqX;
            }
        }
        else if (event->buttons() & ~Qt::NoButton)
        {
            setCursor(QCursor(Qt::ArrowCursor));
            m_CursorCaptured = NOCAP;
        }
    }
    else if (CENTER == m_CursorCaptured)
    {
        // moving inbetween demod lowcut and highcut region
        if (event->buttons() & Qt::LeftButton)
        {   // moving inbetween demod lowcut and highcut region with left button held
            if (m_GrabPosition != 0)
            {
                m_DemodCenterFreq = roundFreq(freqFromX(px - m_GrabPosition),
                                              m_ClickResolution );
                emit newDemodFreq(m_DemodCenterFreq,
                                  m_DemodCenterFreq - m_CenterFreq);
                m_MaxHoldValid = false;
                m_MinHoldValid = false;
                m_histIIRValid = false;
                updateOverlay();
            }
            else
            {
                // save initial grab position from m_DemodFreqX
                m_GrabPosition = px - m_DemodFreqX;
            }
        }
        else if (event->buttons() & ~Qt::NoButton)
        {
            setCursor(QCursor(Qt::ArrowCursor));
            m_CursorCaptured = NOCAP;
        }
    }
    else if (MARKER_A == m_CursorCaptured
             && px < w - m_CursorCaptureDelta
             && px > m_YAxisWidth + m_CursorCaptureDelta)
    {
        if (event->buttons() & Qt::LeftButton)
        {
            qint64 prevA = m_MarkerFreqA;
            m_MarkerFreqA = freqFromX(px);
            emit markerSelectA(m_MarkerFreqA);
            // Shift-drag moves both markers
            if ((event->modifiers() & Qt::ShiftModifier) && m_MarkerFreqB != MARKER_OFF) {
                qint64 df = m_MarkerFreqA - prevA;
                m_MarkerFreqB += df;
                emit markerSelectB(m_MarkerFreqB);
            }
        }
        else if (event->buttons() & ~Qt::NoButton)
        {
            setCursor(QCursor(Qt::ArrowCursor));
            m_CursorCaptured = NOCAP;
        }
    }
    else if (MARKER_B == m_CursorCaptured
             && px < w - m_CursorCaptureDelta
             && px > m_YAxisWidth + m_CursorCaptureDelta)
    {
        if (event->buttons() & Qt::LeftButton)
        {
            qint64 prevB = m_MarkerFreqB;
            m_MarkerFreqB = freqFromX(px);
            emit markerSelectB(m_MarkerFreqB);
            // Shift-drag moves both markers
            if ((event->modifiers() & Qt::ShiftModifier) && m_MarkerFreqA != MARKER_OFF) {
                qint64 df = m_MarkerFreqB - prevB;
                m_MarkerFreqA += df;
                emit markerSelectA(m_MarkerFreqA);
            }
        }
        else if (event->buttons() & ~Qt::NoButton)
        {
            setCursor(QCursor(Qt::ArrowCursor));
            m_CursorCaptured = NOCAP;
        }
    }
    else
    {
        // cursor not captured
        m_GrabPosition = 0;
    }
    if (!this->rect().contains(pt))
    {
        if (NOCAP != m_CursorCaptured)
            setCursor(QCursor(Qt::ArrowCursor));
        m_CursorCaptured = NOCAP;
    }
}


int CPlotter::getNearestPeak(QPoint pt)
{
    int px = qRound((qreal)pt.x() * m_DPR);
    int py = qRound((qreal)pt.y() * m_DPR);

    QMap<int, qreal>::const_iterator i = m_Peaks.lowerBound(px - PEAK_CLICK_MAX_H_DISTANCE);
    QMap<int, qreal>::const_iterator upperBound = m_Peaks.upperBound(px + PEAK_CLICK_MAX_H_DISTANCE);
    qreal   dist = 1.0e10;
    int     best = -1;

    for ( ; i != upperBound; i++)
    {
        int x = i.key();
        qreal y = i.value();

        if (abs(y - py) > PEAK_CLICK_MAX_V_DISTANCE)
            continue;

        qreal d = powf(y - py, 2) + powf(x - px, 2);
        if (d < dist)
        {
            dist = d;
            best = x;
        }
    }

    return best;
}

/** Set waterfall span in milliseconds */
void CPlotter::setWaterfallSpan(quint64 span_ms)
{
    wf_span = span_ms;
    if (!m_WaterfallPixmap.isNull()) {
        wf_epoch = QDateTime::currentMSecsSinceEpoch();
        wf_count = 0;
        msec_per_wfline = (double)wf_span / (qreal)m_WaterfallPixmap.height();
    }
    clearWaterfall();
}

void CPlotter::clearWaterfall()
{
    if (!m_WaterfallPixmap.isNull())
        m_WaterfallPixmap.fill(Qt::black);
    clearWaterfallBuf();
}

void CPlotter::clearWaterfallBuf()
{
    for (int i = 0; i < MAX_SCREENSIZE; i++)
        m_wfbuf[i] = 0.0;
}

/**
 * @brief Save waterfall to a graphics file
 * @param filename
 * @return TRUE if the save successful, FALSE if an error occurred.
 *
 * We assume that frequency strings are up to date
 */
bool CPlotter::saveWaterfall(const QString & filename) const
{
    QBrush          axis_brush(QColor(0x00, 0x00, 0x00, 0x70), Qt::SolidPattern);
    QPixmap         pixmap(m_WaterfallPixmap);
    QPainter        painter(&pixmap);
    QRect           rect;
    QDateTime       tt;
    QFont           font("sans-serif");
    QFontMetricsF   font_metrics(font);
    float           pixperdiv;
    int             x, y, w, h;
    int             hxa, wya = 85;
    int             i;

    w = pixmap.width();
    h = pixmap.height();
    hxa = font_metrics.height() + 5;    // height of X axis
    y = h - hxa;
    pixperdiv = (float) w / (float) m_HorDivs;

    painter.setBrush(axis_brush);
    painter.setPen(QColor(0x0, 0x0, 0x0, 0x70));
    painter.drawRect(0, y, w, hxa);
    painter.drawRect(0, 0, wya, h - hxa - 1);
    painter.setFont(font);
    painter.setPen(QColor(0xFF, 0xFF, 0xFF, 0xFF));

    // skip last frequency entry
    for (i = 2; i < m_HorDivs - 1; i++)
    {
        // frequency tick marks
        x = (int)((float)i * pixperdiv);
        painter.drawLine(x, y, x, y + 5);

        // frequency strings
        x = (int)((float)i * pixperdiv - pixperdiv / 2.0);
        rect.setRect(x, y, (int)pixperdiv, hxa);
        painter.drawText(rect, Qt::AlignHCenter|Qt::AlignBottom, m_HDivText[i]);
    }
    rect.setRect(w - pixperdiv - 10, y, pixperdiv, hxa);
    painter.drawText(rect, Qt::AlignRight|Qt::AlignBottom, tr("MHz"));

    quint64 msec;
    int tdivs = h / 70 + 1;
    pixperdiv = (float) h / (float) tdivs;
    tt.setTimeSpec(Qt::OffsetFromUTC);
    for (i = 1; i < tdivs; i++)
    {
        y = (int)((float)i * pixperdiv);
        if (msec_per_wfline > 0)
            msec =  tlast_wf_ms - qRound(y * msec_per_wfline);
        else
            msec =  tlast_wf_ms - qRound(y * 1000.0 / fft_rate);

        tt.setMSecsSinceEpoch(msec);
        rect.setRect(0, y - font_metrics.height(), wya - 5, font_metrics.height());
        painter.drawText(rect, Qt::AlignRight|Qt::AlignVCenter, tt.toString("yyyy.MM.dd"));
        painter.drawLine(wya - 5, y, wya, y);
        rect.setRect(0, y, wya - 5, font_metrics.height());
        painter.drawText(rect, Qt::AlignRight|Qt::AlignVCenter, tt.toString("hh:mm:ss"));
    }

    return pixmap.save(filename, nullptr, -1);
}

void CPlotter::toggleFreeze()
{
    m_Frozen = !m_Frozen;
}

/** Get waterfall time resolution in milleconds / line. */
quint64 CPlotter::getWfTimeRes() const
{
    if (msec_per_wfline)
        return msec_per_wfline;
    else
        return qRound64(1000.0 / (double)fft_rate); // Auto mode
}

void CPlotter::setFftRate(int rate_hz)
{
    fft_rate = rate_hz;
    clearWaterfall();
}

// Called when a mouse button is pressed
void CPlotter::mousePressEvent(QMouseEvent * event)
{
    QPoint pt = event->pos();
    int px = qRound((qreal)pt.x() * m_DPR);
    int py = qRound((qreal)pt.y() * m_DPR);
    QPoint ppos = QPoint(px, py);

    if (NOCAP == m_CursorCaptured)
    {
        if (isPointCloseTo(px, m_DemodFreqX, m_CursorCaptureDelta))
        {
            // move demod box center frequency region
            m_CursorCaptured = CENTER;
            m_GrabPosition = px - m_DemodFreqX;
        }
        else if (isPointCloseTo(px, m_DemodLowCutFreqX, m_CursorCaptureDelta))
        {
            // filter low cut
            m_CursorCaptured = LEFT;
            m_GrabPosition = px - m_DemodLowCutFreqX;
        }
        else if (isPointCloseTo(px, m_DemodHiCutFreqX, m_CursorCaptureDelta))
        {
            // filter high cut
            m_CursorCaptured = RIGHT;
            m_GrabPosition = px - m_DemodHiCutFreqX;
        }
        else
        {
            if (event->buttons() == Qt::LeftButton)
            {
                // {shift|ctrl|ctrl-shift}-left-click: set ab markers around signal at cursor
                quint32 mods = event->modifiers() & (Qt::ShiftModifier|Qt::ControlModifier);
                if (m_MarkersEnabled && ((event->modifiers() & mods) != 0))
                {
                    float *selectBuf = nullptr;

                    // when max hold is valid, ctrl-shift selects max hold
                    if (m_MaxHoldActive && (mods == (Qt::ShiftModifier | Qt::ControlModifier)))
                    {
                        selectBuf = m_fftMaxHoldBuf;
                    }
                    // in max mode, shift selects max
                    else if (m_PlotMode == PLOT_MODE_MAX && (mods == Qt::ShiftModifier))
                    {
                        selectBuf = m_fftMaxBuf;
                    }
                    // in avg mode, shift select avg
                    else if (m_PlotMode == PLOT_MODE_AVG && (mods == Qt::ShiftModifier))
                    {
                        selectBuf = m_fftAvgBuf;
                    }
                    // in filled and histogram modes, shift selects max, ctrl selects avg
                    else if (m_PlotMode == PLOT_MODE_FILLED || m_PlotMode == PLOT_MODE_HISTOGRAM)
                    {
                        if (mods == Qt::ShiftModifier)
                        {
                            selectBuf = m_fftAvgBuf;
                        }
                        else if (mods == Qt::ControlModifier)
                        {
                            selectBuf = m_fftMaxBuf;
                        }
                    }

                    // ignore if data source is not valid
                    if (m_fftDataSize && selectBuf)
                    {
                        // Find the data value of the click y()

                        const qreal plotHeight = m_2DPixmap.height();
                        const float panddBGainFactor = plotHeight / fabs(m_PandMaxdB - m_PandMindB);
                        const float vlog = m_PandMaxdB - py / panddBGainFactor;
                        const float logFactor = m_PlotScale == PLOT_SCALE_V ? 10.0 : 10.0;
                        // const float logFactor = m_PlotScale == PLOT_SCALE_V ? 20.0 : 10.0;
                        const float v = powf(10.0, vlog / logFactor);

                        // Ignore clicks exactly on the plot, below the
                        // pandapter, or when uninitialized
                        if (v != selectBuf[px]
                            && py < plotHeight
                            && m_fftDataSize > 0)
                        {
                            int xLeft = px;
                            int xRight = px;
                            // Select span below the plot
                            if (v < selectBuf[px])
                            {
                                for(; xLeft > 0 && selectBuf[xLeft] > v; --xLeft);
                                for(; xRight < m_fftDataSize && selectBuf[xRight] > v; ++xRight);
                            }
                            // Select span above the plot
                            else
                            {
                                for(; xLeft > 0 && selectBuf[xLeft] < v; --xLeft);
                                for(; xRight < m_fftDataSize && selectBuf[xRight] < v; ++xRight);
                            }
                            qint64 freqLeft = freqFromX(xLeft);
                            qint64 freqRight = freqFromX(xRight);

                            emit markerSelectA(freqLeft);
                            emit markerSelectB(freqRight);
                        }
                    }
                }

                // left-click with no modifiers: set center frequency
                else if (mods == 0) {
                    int best = -1;

                    if (m_PeakDetectActive > 0)
                        best = getNearestPeak(pt);
                    if (best != -1)
                        m_DemodCenterFreq = freqFromX(best);
                    else
                        m_DemodCenterFreq = roundFreq(freqFromX(px), m_ClickResolution);

                    // if cursor not captured set demod frequency and start demod box capture
                    emit newDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq - m_CenterFreq);

                    // save initial grab position from m_DemodFreqX
                    // setCursor(QCursor(Qt::CrossCursor));
                    m_CursorCaptured = CENTER;
                    m_GrabPosition = 1;
                    updateOverlay();
                }
            }
            else if (event->buttons() == Qt::MiddleButton)
            {
                // set center freq
                m_CenterFreq = roundFreq(freqFromX(px), m_ClickResolution);
                m_DemodCenterFreq = m_CenterFreq;
                emit newDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq - m_CenterFreq);
                updateOverlay();
            }
            else if (event->buttons() == Qt::RightButton)
            {
                // reset frequency zoom
                resetHorizontalZoom();
            }
        }
    }
    else
    {
        if (m_CursorCaptured == YAXIS)
            // get ready for moving Y axis
            m_Yzero = py;
        else if (m_CursorCaptured == XAXIS)
        {
            m_Xzero = px;
            if (event->buttons() == Qt::RightButton)
            {
                // reset frequency zoom
                resetHorizontalZoom();
            }
        }
        else if (m_CursorCaptured == TAG)
        {
            for (auto & tag : m_Taglist)
            {
                if (tag.first.contains(ppos))
                {
                    m_DemodCenterFreq = tag.second;
                    emit newDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq - m_CenterFreq);
                    break;
                }
            }
        }
    }
}

void CPlotter::mouseReleaseEvent(QMouseEvent * event)
{
    QPoint pt = event->pos();
    int py = qRound((qreal)pt.y() * m_DPR);

    if (py >= m_OverlayPixmap.height())
    {
        // not in Overlay region
        if (NOCAP != m_CursorCaptured)
            setCursor(QCursor(Qt::ArrowCursor));

        m_CursorCaptured = NOCAP;
        m_GrabPosition = 0;
    }
    else
    {
        if (YAXIS == m_CursorCaptured)
        {
            setCursor(QCursor(Qt::OpenHandCursor));
            m_Yzero = -1;
        }
        else if (XAXIS == m_CursorCaptured)
        {
            setCursor(QCursor(Qt::OpenHandCursor));
            m_Xzero = -1;
        }
    }
}


// Make a single zoom step on the X axis.
void CPlotter::zoomStepX(float step, int x)
{
    // calculate new range shown on FFT
    double new_range = qBound(10.0, m_Span * (double)step, m_SampleFreq * 10.0);

    // Frequency where event occurred is kept fixed under mouse
    double ratio = (double)x / (qreal)m_Size.width() / m_DPR;
    qint64 fixed_hz = freqFromX(x);
    double f_max = fixed_hz + (1.0 - ratio) * new_range;
    double f_min = f_max - new_range;

    // ensure we don't go beyond the rangelimits
    bool limit_hit = false;
    double lolim = m_CenterFreq - m_SampleFreq / 2.0;
    double hilim = m_CenterFreq + m_SampleFreq / 2.0;
    if (f_min < lolim)
    {
        f_min = lolim;
        limit_hit = true;
    }
    if (f_max > hilim)
    {
        f_max = hilim;
        limit_hit = true;
    }

    // the new span
    quint32 new_span = (quint32)(f_max - f_min);
    if( new_span & 1 )
    {
        new_span++; // keep span even to avoid rounding in span/2
    }

    // find new FFT center frequency
    qint64 new_FftCenter;
    if( limit_hit ) // cannot keep fixed_hz fixed
    {
    	new_FftCenter = qRound64((f_min + f_max) / 2.0) - m_CenterFreq;
    }
    else // calculate new FFT center frequency that really keeps fixed_hz fixed
    {
    	qint64 wouldbe_hz = (m_CenterFreq + m_FftCenter - new_span / 2) + ratio * new_span;
    	new_FftCenter = m_FftCenter + (fixed_hz - wouldbe_hz);
    }
    setFftCenterFreq(new_FftCenter);
    setSpanFreq(new_span);

    float factor = (float)m_SampleFreq / (float)m_Span;
    emit newZoomLevel(factor);
    qCDebug(plotter) << QString("Spectrum zoom: %1x").arg(factor, 0, 'f', 1);

    m_MaxHoldValid = false;
    m_MinHoldValid = false;
    m_histIIRValid = false;
}

// Zoom on X axis (absolute level)
void CPlotter::zoomOnXAxis(float level)
{
    float current_level = (float)m_SampleFreq / (float)m_Span;
    zoomStepX(current_level / level, xFromFreq(m_DemodCenterFreq));
    updateOverlay();
}

void CPlotter::setPlotMode(int mode)
{
    m_PlotMode = (ePlotMode)mode;
    m_MaxHoldValid = false;
    m_MinHoldValid = false;
    // Do not need to invalidate IIR data when switching modes

    updateOverlay();
}

void CPlotter::setPlotScale(int scale)
{
    m_PlotScale = (ePlotScale)scale;
    m_MaxHoldValid = false;
    m_MinHoldValid = false;
    m_IIRValid = false;
    m_histIIRValid = false;
}

void CPlotter::setPlotPer(int per)
{
    m_PlotPer = (ePlotPer)per;
    m_MaxHoldValid = false;
    m_MinHoldValid = false;
    m_IIRValid = false;
    m_histIIRValid = false;
}

void CPlotter::setWaterfallMode(int mode)
{
    m_WaterfallMode = (eWaterfallMode)mode;
}

// Called when a mouse wheel is turned
void CPlotter::wheelEvent(QWheelEvent * event)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    QPoint pt = QPoint(event->pos());
#else
    QPointF pt = event->position();
#endif
    int h = m_OverlayPixmap.height();
    int px = qRound((qreal)pt.x() * m_DPR);
    int py = qRound((qreal)pt.y() * m_DPR);

    int delta = m_InvertScrolling? -event->angleDelta().y() : event->angleDelta().y();
    int numDegrees = delta / 8;
    int numSteps = numDegrees / 15;  /** FIXME: Only used for direction **/

    /** FIXME: zooming could use some optimisation **/
    if (m_CursorCaptured == YAXIS)
    {
        // Vertical zoom. Wheel down: zoom out, wheel up: zoom in
        // During zoom we try to keep the point (dB or kHz) under the cursor fixed
        float zoom_fac = delta < 0 ? 1.1 : 0.9;
        float ratio = (qreal)py / (qreal)(h);
        float db_range = m_PandMaxdB - m_PandMindB;
        auto y_range = (qreal)(h);
        float db_per_pix = db_range / y_range;
        float fixed_db = m_PandMaxdB - py * db_per_pix;

        db_range = qBound(10.f, db_range * zoom_fac, FFT_MAX_DB - FFT_MIN_DB);
        m_PandMaxdB = fixed_db + ratio * db_range;
        if (m_PandMaxdB > FFT_MAX_DB)
            m_PandMaxdB = FFT_MAX_DB;

        m_PandMindB = m_PandMaxdB - db_range;
        m_MaxHoldValid = false;
        m_MinHoldValid = false;
        m_histIIRValid = false;

        emit pandapterRangeChanged(m_PandMindB, m_PandMaxdB);
    }
    else if (m_CursorCaptured == XAXIS)
    {
        zoomStepX(delta < 0 ? 1.1 : 0.9, px);
    }
    else if (event->modifiers() & Qt::ControlModifier)
    {
        // filter width
        m_DemodLowCutFreq -= numSteps * m_ClickResolution;
        m_DemodHiCutFreq += numSteps * m_ClickResolution;
        clampDemodParameters();
        emit newFilterFreq(m_DemodLowCutFreq, m_DemodHiCutFreq);
    }

    else if (event->modifiers() & Qt::ShiftModifier)
    {
        // filter shift
        m_DemodLowCutFreq += numSteps * m_ClickResolution;
        m_DemodHiCutFreq += numSteps * m_ClickResolution;
        clampDemodParameters();
        emit newFilterFreq(m_DemodLowCutFreq, m_DemodHiCutFreq);
    }
    else
    {
        // inc/dec demod frequency
        m_DemodCenterFreq += (numSteps * m_ClickResolution);
        m_DemodCenterFreq = roundFreq(m_DemodCenterFreq, m_ClickResolution );
        emit newDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq-m_CenterFreq);
    }

    updateOverlay();
}

// Called when screen size changes so must recalculate bitmaps
void CPlotter::resizeEvent(QResizeEvent* )
{
    if (!size().isValid())
        return;

    m_DPR = devicePixelRatioF();
    QSize s = QSize(size().width(), size().height());
    if (m_Size != s)
    {
        m_Size = s;

        // Use scaled system font
        m_Font = QFont();
        m_Font.setPointSizeF(m_Font.pointSizeF() * m_DPR);

        // Higher resolution pixmaps are used with higher DPR. They are
        // rescaled in paintEvent().
        const int w = qRound((qreal)s.width() * m_DPR);
        const int plotHeight = qRound((qreal)m_Percent2DScreen * (qreal)s.height() / 100.0 * m_DPR);
        const int wfHeight = qRound((qreal)s.height() * m_DPR) - plotHeight;

        m_OverlayPixmap = QPixmap(w, plotHeight);
        m_OverlayPixmap.fill(Qt::transparent);

        m_2DPixmap = QPixmap(w, plotHeight);
        m_2DPixmap.fill(PLOTTER_BGD_COLOR);

        // No waterfall, use null pixmap
        if (wfHeight == 0)
        {
            m_WaterfallPixmap = QPixmap();
        }

        // New waterfall, create blank area
        else if (m_WaterfallPixmap.isNull()) {
            m_WaterfallPixmap = QPixmap(w, wfHeight);
            m_WaterfallPixmap.fill(Qt::black);
        }

        // Existing waterfall, rescale width but no height as that would
        // invalidate time
        else
        {
            QPixmap oldWaterfall = m_WaterfallPixmap.scaled(
                w, m_WaterfallPixmap.height(),
                Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            m_WaterfallPixmap = QPixmap(w, wfHeight);
            m_WaterfallPixmap.fill(Qt::black);
            QRect copyRect(0, 0,
                           qMin(w, oldWaterfall.width()),
                           qMin(wfHeight, oldWaterfall.height()));
            QPainter painter(&m_WaterfallPixmap);
            painter.drawPixmap(QPointF(0.0, 0.0), oldWaterfall, copyRect);
        }

        // Invalidate on resize
        m_MaxHoldValid = false;
        m_MinHoldValid = false;
        m_histIIRValid = false;
        // Do not need to invalidate IIR data (just histogram IIR)

        // Waterfall accumulator my be the wrong size now, so invalidate.
        if (msec_per_wfline > 0)
            clearWaterfallBuf();

        // Other things that need to scale with DPR
        m_CursorCaptureDelta = qRound((qreal)CUR_CUT_DELTA * m_DPR);
    }

    drawOverlay();
    draw(false);
    emit newSize();

    m_Frozen = false;
}

// Called by QT when screen needs to be redrawn
void CPlotter::paintEvent(QPaintEvent *)
{
    // Pixmap resolution scales with DPR. Here, they are rescaled to fit the
    // the CPlotter resolution.

    QPainter painter(this);

    int plotHeightT = 0;
    if (!m_2DPixmap.isNull())
    {
        const int plotWidthS = m_2DPixmap.width();
        const int plotHeightS = m_2DPixmap.height();
        const QRectF plotRectS(0.0, 0.0, plotWidthS, plotHeightS);

        const int plotWidthT = qRound((qreal)plotWidthS / m_DPR);
        plotHeightT = qRound((qreal)plotHeightS / m_DPR);
        const QRectF plotRectT(0.0, 0.0, plotWidthT, plotHeightT);

        painter.drawPixmap(plotRectT, m_2DPixmap, plotRectS);
    }

    if (!m_Frozen && !m_WaterfallPixmap.isNull())
    {
        const int wfWidthS = m_WaterfallPixmap.width();
        const int wfHeightS = m_WaterfallPixmap.height();
        const QRectF wfRectS(0.0, 0.0, wfWidthS, wfHeightS);

        const int wfWidthT = qRound((qreal)wfWidthS / m_DPR);
        const int wfHeightT = qRound((qreal)wfHeightS / m_DPR);
        const QRectF wfRectT(0.0, plotHeightT, wfWidthT, wfHeightT);

        painter.drawPixmap(wfRectT, m_WaterfallPixmap, wfRectS);
    }
}

// Called to update spectrum data for displaying on the screen
void CPlotter::draw(bool newData)
{
    qint32        i, j;
    qint32        xmin, xmax;
    double        histMax;
    QFontMetricsF metrics(m_Font);

    QPointF avgLineBuf[MAX_SCREENSIZE];
    QPointF maxLineBuf[MAX_SCREENSIZE];

    const quint64 tnow_ms = QDateTime::currentMSecsSinceEpoch();

    // Pixmaps might be null, so scale up m_Size to get width.
    const qreal w = m_Size.width() * m_DPR;
    const qreal plotHeight = m_2DPixmap.height();
    const qreal shadowOffset = metrics.height() / 20.0;

    // Scale plotter for graph height
    const double panddBGainFactor = plotHeight / fabs(m_PandMaxdB - m_PandMindB);
    // Scale waterfall and histogram for colormap
    const double wfdBGainFactor = 256.0 / fabs(m_WfMaxdB - m_WfMindB);

    const double fftSize = m_fftDataSize;
    const double sampleFreq = m_SampleFreq;

    const double startFreq = (double)m_FftCenter - (double)m_Span / 2.0;
    const double endFreq = (double)m_FftCenter + (double)m_Span / 2.0;

    // Start and end fft bins for requested frequency offsets. These values
    // could be outside the actual fft data range, since the display can be
    // shifted past limits while zooming.
    const qint32 startBin = std::min(startFreq * fftSize / sampleFreq + fftSize / 2.0, fftSize - 1);
    const qint32 endBin = std::max(endFreq * fftSize / sampleFreq + fftSize / 2.0, (double)startBin);

    // Start and end FFT bins to be displayed, clipped to valid fft bins
    const qint32 minbin = std::max(startBin, 0);
    const qint32 maxbin = std::min(endBin, m_fftDataSize - 1);

    // Number of bins mapped across plot, taking zoom into account
    const int numBins = endBin - startBin;
    // Pixels per mapped bin
    const double xScale = w / (double)numBins;

    const double frameTime = 1.0 / (double)fft_rate;

    // Redraw the plot if it is visible.
    const bool doPlotter = !m_2DPixmap.isNull();

    // Do not waste time with histogram calculations unless in this mode.
    const bool doHistogram = m_PlotMode == PLOT_MODE_HISTOGRAM;

    // Use fewer histogram bins when statistics are sparse
    const int histBinsDisplayed = std::min(
        MAX_HISTOGRAM_SIZE,
        std::max(32,
            qRound(32 * (double)numBins / 2048.0))
        );

    // Amount to add to histogram for each hit
    const double histWeight = 10e6 * frameTime / (double)histBinsDisplayed / fftSize;

    // Bins / dB
    const double histdBGainFactor = (double)histBinsDisplayed / fabs(m_PandMaxdB - m_PandMindB);

    // Show max and average highlights on histogram if it would not be too
    // cluttered
    const bool showHistHighlights = histBinsDisplayed >= MAX_HISTOGRAM_SIZE / 2;

    // Waterfall is advanced only if visible and running, and if there is new
    // data. Repaints for other reasons do not require any action here.
    const bool doWaterfall = !m_WaterfallPixmap.isNull() && m_Running && newData;

    // Draw avg line, except in max mode. Suppress if it would clutter histogram.
    const bool doAvgLine = m_PlotMode != PLOT_MODE_MAX
                           && (m_PlotMode != PLOT_MODE_HISTOGRAM || showHistHighlights);

    // Draw max line, except in avg and histogram modes
    const bool doMaxLine = m_PlotMode != PLOT_MODE_AVG
                           && m_PlotMode != PLOT_MODE_HISTOGRAM;

    // Initialize results
    if (doHistogram)
        memset(m_histogram, 0, sizeof(m_histogram));

    // Peak means "peak of average" in AVG mode, else "peak of max"
    const bool peakIsAverage = m_PlotMode == PLOT_MODE_AVG;
    // Min mean "min of peak" in PEAK mode, else "min of average"
    const bool minIsAverage = m_PlotMode != PLOT_MODE_MAX;

    // Scale log10 by 20 for V, 10 for dBm
    const float logFactor = m_PlotScale == PLOT_SCALE_V ? 10.0 : 10.0;

    // Make sure zeros don't get through to log calcs
    const float fmin = std::numeric_limits<float>::min();

    float vmax;
    float vmaxIIR;
    double vsum;
    double vsumIIR;

    if ((qreal)numBins >= w)
    {
        qint32 count;
        xmin = qRound((double)(minbin - startBin) * xScale);
        xmax = qRound((double)(maxbin - startBin) * xScale);
        qint32 xprev = xmin;
        bool first = true;

        for(qint32 i = minbin; i <= maxbin; i++)
        {
            const double xD = (double)(i - startBin) * xScale;
            const int x = qRound(xD);

            // Plot uses IIR output. Histogram and waterfall use raw fft data.
            const float v = m_fftData[i];
            const float viir = m_fftIIR[i];

            if (first)
            {
                vmax = v;
                vmaxIIR = viir;
                vsum = v;
                vsumIIR = viir;
                count = 1;
            }

            // Histogram increments the appropriate bin for each value. Ignore
            // out-of-range values, rather than clipping. Allocate value to two
            // closest bins using linear interpolation.
            if (doHistogram)
            {
                const double binD = histdBGainFactor * (m_PandMaxdB - logFactor * log10f(v));
                const int binLeft = std::max((int)(xD - 0.5), 0);
                const int binRight = std::min(binLeft + 1, numBins - 1);
                const int binLow = std::max((int)(binD - 0.5), 0);
                const int binHigh = std::min(binLow + 1, histBinsDisplayed - 1);
                const double wgtH = (xD - (double)binLeft) / 2.0;
                const double wgtV = (binD - (double)binLow) / 2.0;
                if (binLow >= 0 && binLow < histBinsDisplayed) {
                    m_histogram[binLeft][binLow] += (1.0 - wgtV) * (1.0 - wgtH) * histWeight;
                    m_histogram[binLeft][binHigh] += wgtV * (1.0 - wgtH) * histWeight;
                    m_histogram[binRight][binLow] += (1.0 - wgtV) * wgtH * histWeight;
                    m_histogram[binRight][binHigh] += wgtV * wgtH * histWeight;
                }
            }

            // New (or last) pixel - output values
            if (x != xprev || i == maxbin)
            {
                vmax = std::max(vmax, fmin);
                m_wfMaxBuf[xprev] = vmax;

                vmaxIIR = std::max(vmaxIIR, fmin);
                m_fftMaxBuf[xprev] = vmaxIIR;

                const float vavg = std::max((float)(vsum / count), fmin);
                m_wfAvgBuf[xprev] = vavg;
                const float vavgIIR = std::max((float)(vsumIIR / count), fmin);
                m_fftAvgBuf[xprev] = vavgIIR;

                // New peak hold value if greater, or reset
                const float currentPeak = m_fftMaxHoldBuf[xprev];
                const float newPeak = peakIsAverage ? vavgIIR : vmaxIIR;
                m_fftMaxHoldBuf[xprev] = m_MaxHoldValid ? std::max(currentPeak, newPeak) : newPeak;

                // New min hold value if less, or reset
                const float currentMin = m_fftMinHoldBuf[xprev];
                const float newMin = minIsAverage ? vavgIIR : vmaxIIR;
                m_fftMinHoldBuf[xprev] = m_MinHoldValid ? std::min(currentMin, newMin) : newMin;

                vmax = v;
                vmaxIIR = viir;
                vsum = v;
                vsumIIR = viir;
                count = 1;
                xprev = x;
            }

            else if (!first)
            {
                vmax = std::max(v, vmax);
                vmaxIIR = std::max(viir, vmaxIIR);
                vsum += v;
                vsumIIR += viir;
                ++count;
            }

            first = false;
        }

        m_MaxHoldValid = true;
        m_MinHoldValid = true;
    }
    // w > m_fftDataSize uses no averaging
    else
    {
        xmin = qRound((double)(minbin - startBin) * xScale);
        xmax = qRound((double)(maxbin - startBin) * xScale);

        for (i = xmin; i < xmax; i++)
        {
            j = qRound((double)i / xScale + startBin);
            const float v = m_fftData[j];
            const float viir = m_fftIIR[j];

            m_wfMaxBuf[i] = v;
            m_wfAvgBuf[i] = v;
            m_fftMaxBuf[i] = viir;
            m_fftAvgBuf[i] = viir;

            // New peak hold value if greater, or reset
            const float currentPeak = m_fftMaxHoldBuf[i];
            m_fftMaxHoldBuf[i] = m_MaxHoldValid ? std::max(currentPeak, viir) : viir;

            // New min hold value if less, or reset
            const float currentMin = m_fftMinHoldBuf[i];
            m_fftMinHoldBuf[i] = m_MinHoldValid ? std::min(currentMin, viir) : viir;

            // Histogram increments the appropriate bin for each value. Ignore
            // out-of-range values, rather than clipping. Allocate value to two
            // closest bins using linear interpolation.
            if (doHistogram)
            {
                const double binD = histdBGainFactor * (m_PandMaxdB - logFactor * log10f(v));
                const int binLow = std::max((int)(binD - 0.5), 0);
                const int binHigh = std::min(binLow + 1, histBinsDisplayed - 1);
                const double wgt = (binD - (double)binLow) / 2.0;
                if (binLow >= 0 && binLow < histBinsDisplayed) {
                    m_histogram[i][binLow] += (1.0 - wgt) * histWeight;
                    m_histogram[i][binHigh] += wgt * histWeight;
                }
            }
        }
    }

    const int npts = xmax - xmin;

    if (doWaterfall)
    {
        // Pick max or avg for waterfall
        float *dataSource;
        if (m_WaterfallMode == WATERFALL_MODE_AVG)
            dataSource = m_wfAvgBuf;
        else
            dataSource = m_wfMaxBuf;

        // if not in "auto" mode, store max waterfall data
        if (msec_per_wfline > 0)
        {
            for (i = 0; i < npts; ++i)
                m_wfbuf[i] = std::max(m_wfbuf[i], dataSource[i]);
        }

        // is it time to update waterfall?
        if (tnow_ms - wf_epoch > wf_count * msec_per_wfline)
        {
            ++wf_count;

            // cursor times are relative to last time drawn
            tlast_wf_ms = tnow_ms;
            if (!m_Frozen)
                tlast_wf_drawn_ms = tnow_ms;

            // move current data down one line(must do before attaching a QPainter object)
            m_WaterfallPixmap.scroll(0, 1, m_WaterfallPixmap.rect());

            QPainter painter1(&m_WaterfallPixmap);

            // draw new line of fft data at top of waterfall bitmap
            // draw black areas where data will not be draw
            painter1.setPen(QPen(Qt::black));
            painter1.drawRect(QRectF(0.0, 0.0, xmin, 1.0));
            painter1.drawRect(QRectF(xmax, 0.0, w - xmax, 1.0));

            // Use stored max if in manual mode, else current data
            const float *wfSource = msec_per_wfline > 0 ? m_wfbuf : dataSource;
            for (i = 0; i < npts; ++i)
            {
                const int ix = i + xmin;
                qint32 cidx = qRound((m_WfMaxdB - logFactor * log10f(wfSource[ix])) * wfdBGainFactor);
                cidx = std::max(std::min(cidx, 255), 0);
                painter1.setPen(m_ColorTbl[255 - cidx]);
                painter1.drawRect(QRectF(ix, 0.0, 1.0, 1.0));
            }

            if (msec_per_wfline > 0)
                clearWaterfallBuf();
        }
    }

    // get/draw the 2D spectrum
    if (doPlotter)
    {
        m_2DPixmap.fill(PLOTTER_BGD_COLOR);
        QPainter painter2(&m_2DPixmap);

        // Update histogram IIR
        const double frameTime = 1.0 / (double)fft_rate;
        if (m_PlotMode == PLOT_MODE_HISTOGRAM)
        {
            const double gamma = 1.0;
            const double a = powf(1.0 - m_alpha, gamma);
            // fast attack ... leaving alternative here in case it's useful
            const double aAttack = 1.0;
            // const double aAttack = 1.0 - a * frameTime;
            const double aDecay = 1.0 - pow(a, 4.0 * frameTime);

            histMax = 0.0;
            for (i = xmin; i < xmax; ++i) {
                for (j = 0; j < histBinsDisplayed; ++j)
                {
                    double histV;
                    const double histPrev = m_histIIR[i][j];
                    const double histNew = m_histogram[i][j];
                    // Fast response when invalid
                    if (!m_histIIRValid)
                        histV = histNew;
                    else
                        histV = histPrev + aAttack * histNew - aDecay * histPrev;
                    m_histIIR[i][j] = std::max(histV, 0.0);
                    histMax = std::max(histMax, histV);
                }
            }
            m_histIIRValid = true;

            // 5 Hz time constant for colormap adjustment
            const double histMaxAlpha = std::min(5.0 * frameTime, 1.0);
            m_histMaxIIR = m_histMaxIIR * (1.0 - histMaxAlpha) + histMax * histMaxAlpha;
        }

        // draw the pandapter
        QBrush fillBrush = QBrush(m_FftFillCol);

        // Fill between max and avg
        QColor maxFillCol = m_FftFillCol;
        maxFillCol.setAlpha(80);
        QBrush maxFillBrush = QBrush(maxFillCol);

        // Diagonal fill for area between markers. Scale the pattern to DPR.
        QColor abFillColor = QColor(PLOTTER_MARKER_COLOR);
        abFillColor.setAlpha(128);
        QBrush abFillBrush = QBrush(abFillColor, Qt::BDiagPattern);

        QColor maxLineColor = QColor(m_FftFillCol);
        if (m_PlotMode == PLOT_MODE_FILLED)
            maxLineColor.setAlpha(128);
        else
            maxLineColor.setAlpha(255);

        QPen maxLinePen = QPen(maxLineColor);

        // Same color as max in avg mode, different for filled mode
        QPen avgLinePen;
        if (m_PlotMode == PLOT_MODE_AVG || m_PlotMode == PLOT_MODE_HISTOGRAM)
        {
            QColor avgLineCol = m_FftFillCol;
            avgLineCol.setAlpha(255);
            avgLinePen = QPen(avgLineCol);
        }
        else {
            QColor avgLineCol = QColor(Qt::cyan);
            avgLineCol.setAlpha(192);
            avgLinePen = QPen(avgLineCol);
        }

        // The m_Marker{AB}X values are one cycle old, which makes for a laggy
        // effect, so get fresh values here.
        const int ax = xFromFreq(m_MarkerFreqA);
        const int bx = xFromFreq(m_MarkerFreqB);
        bool fillMarkers = (m_MarkersEnabled && m_MarkerFreqA != MARKER_OFF
                                             && m_MarkerFreqB != MARKER_OFF);
        const int minMarker = std::min(ax, bx);
        const int maxMarker = std::max(ax, bx);

        const double binSizeY = plotHeight / (double)histBinsDisplayed;
        for (i = 0; i < npts; i++)
        {
            const int ix = i + xmin;
            const qreal yMaxD = std::max(std::min(
                panddBGainFactor * (m_PandMaxdB - logFactor * log10f(m_fftMaxBuf[ix])),
                plotHeight), 0.0);
            const qreal yAvgD = std::max(std::min(
                panddBGainFactor * (m_PandMaxdB - logFactor * log10f(m_fftAvgBuf[ix])),
                plotHeight), 0.0);

            if (m_PlotMode == PLOT_MODE_HISTOGRAM)
            {
                const double *histData = m_histIIR[(ix)];
                qreal topBin = plotHeight;
                for (j = 0; j < histBinsDisplayed; ++j)
                {
                    qint32 cidx = qRound(histData[j] / m_histMaxIIR * 255.0 * .7);
                    if (cidx > 0) {
                        cidx += 65;  // 255 * 0.7 = 178, + 65 = 243
                        // Histogram IIR can cause out-of-range cidx
                        cidx = std::max(std::min(cidx, 255), 0);
                        QColor c = m_ColorTbl[cidx];
                        // Paint rectangle
                        const qreal binY = binSizeY * j;
                        topBin = std::min(topBin, binY);
                        const qreal binH = binSizeY * (j + 1) - binY;
                        painter2.fillRect(QRectF(ix, binY, 1.0, binH), c);
                    }
                }
                // Highlight the top bin, if it isn't too crowded
                if (topBin != plotHeight && showHistHighlights) {
                    painter2.fillRect(QRectF(ix, topBin, 1.0, binSizeY), maxLineColor);
                }
            }

            // Add max, average points if they will be drawn
            if (doMaxLine)
                maxLineBuf[i] = QPointF(ix + 0.5, yMaxD + 0.5);
            if (doAvgLine)
                avgLineBuf[i] = QPointF(ix + 0.5, yAvgD + 0.5);

            // Fill area between markers, even if they are off screen
            qreal yFill = m_PlotMode == PLOT_MODE_MAX ? yMaxD : yAvgD;
            if (fillMarkers && (ix) > minMarker && (ix) < maxMarker) {
                painter2.fillRect(QRectF(ix, yFill + 1.0, 1.0, plotHeight - yFill), abFillBrush);
            }
            if (m_FftFill && m_PlotMode != PLOT_MODE_HISTOGRAM)
            {
                painter2.fillRect(QRectF(ix, yFill + 1.0, 1.0, plotHeight - yFill), m_FftFillCol);
            }
            if (m_PlotMode == PLOT_MODE_FILLED)
            {
                painter2.fillRect(QRectF(ix, yMaxD + 1.0, 1.0, yAvgD - yMaxD), maxFillBrush);
            }
        }

        if (doMaxLine) {
            // NOT scaling to DPR due to performance
            painter2.setPen(maxLinePen);
            painter2.drawPolyline(maxLineBuf, npts);
        }
        if (doAvgLine) {
            // NOT scaling to DPR due to performance
            painter2.setPen(avgLinePen);
            painter2.drawPolyline(avgLineBuf, npts);
        }

        // Peak hold
        if (m_MaxHoldActive)
        {
            // Show max(max) except when showing only avg on screen
            for (i = 0; i < npts; i++)
            {
                const int ix = i + xmin;
                const qreal yMaxHoldD = std::max(std::min(
                    panddBGainFactor * (m_PandMaxdB - logFactor * log10f(m_fftMaxHoldBuf[ix])),
                    plotHeight), 0.0);
                maxLineBuf[i] = QPointF(ix, yMaxHoldD);
            }
            // NOT scaling to DPR due to performance
            painter2.setPen(m_MaxHoldColor);
            painter2.drawPolyline(maxLineBuf, npts);

            m_MaxHoldValid = true;
        }

        // Min hold
        if (m_MinHoldActive)
        {
            // Show min(avg) except when showing only max on scree
            for (i = 0; i < npts; i++)
            {
                const int ix = i + xmin;
                const qreal yMinHoldD = std::max(std::min(
                    panddBGainFactor * (m_PandMaxdB - logFactor * log10f(m_fftMinHoldBuf[ix])),
                    plotHeight), 0.0);
                maxLineBuf[i] = QPointF(ix, yMinHoldD);
            }
            // NOT scaling to DPR due to performance
            painter2.setPen(m_MinHoldColor);
            painter2.drawPolyline(maxLineBuf, npts);

            m_MinHoldValid = true;
        }

        // Peak detection
        if (m_PeakDetectActive)
        {
            const int pw = PEAK_WINDOW_HALF_WIDTH;

            // Use data source appropriate for current display mode
            float *_detectSource;
            if (m_MaxHoldActive)
                _detectSource = m_fftMaxHoldBuf;
            else if (m_PlotMode == PLOT_MODE_AVG || m_PlotMode == PLOT_MODE_HISTOGRAM)
                _detectSource = m_fftAvgBuf;
            else
                _detectSource = m_fftMaxBuf;
            const float *detectSource = _detectSource;

            // Run peak detection periodically. If overlay will be redrawn, run
            // peak detection since zoom/pan may have changed.
            if (tnow_ms > tlast_peaks_ms + PEAK_UPDATE_PERIOD || m_DrawOverlay) {
                tlast_peaks_ms = tnow_ms;
                m_Peaks.clear();
                // Collect statistics (min/max/avg of window) and find sharper peaks
                for (i = pw; i < npts - pw; ++i)
                {
                    const int ix = i + xmin;
                    const float d = detectSource[ix];
                    float maxInWindow = std::numeric_limits<float>::lowest();
                    float minInWindow = std::numeric_limits<float>::max();
                    double sum = d;
                    for (j = i - pw; j < i; ++j)
                    {
                        const float v = detectSource[j + xmin];
                        maxInWindow = std::max(maxInWindow, v);
                        minInWindow = std::min(minInWindow, v);
                        sum += v;
                    }
                    for (j = i + 1; j < i + pw; ++j)
                    {
                        const float v = detectSource[j + xmin];
                        maxInWindow = std::max(maxInWindow, v);
                        minInWindow = std::min(minInWindow, v);
                        sum += v;
                    }
                    m_peakSmoothBuf[ix] = sum / (double)(pw * 2 * 1);
                    if (d > maxInWindow && d > 2.0 * minInWindow)
                    {
                        const qreal y = std::max(std::min(
                            panddBGainFactor * (m_PandMaxdB - logFactor * log10f(d)),
                            plotHeight - 0.0), 0.0);
                        m_Peaks[ix] = y;
                    }
                }

                // Fill the ends of the d_peakSmoothBuf with valid end values
                // and run detection again on smoothed data, looking for wider
                // peaks
                for (i = 0; i < pw; ++i)
                    m_peakSmoothBuf[i + xmin] = 0; // m_peakSmoothBuf[xmin + pw];
                for (i = npts - pw; i < npts; ++i)
                    m_peakSmoothBuf[i + xmin] = 0; // m_peakSmoothBuf[xmin + npts - pw - 1];
                for (i = pw; i < npts - pw; ++i)
                {
                    const int ix = i + xmin;
                    const float d = m_peakSmoothBuf[ix];
                    float maxInWindow = std::numeric_limits<float>::lowest();
                    for (j = i - pw; j < i; ++j)
                    {
                        const float v = m_peakSmoothBuf[j + xmin];
                        maxInWindow = std::max(maxInWindow, v);
                    }
                    for (j = i + 1; j < i + pw; ++j)
                    {
                        const float v = m_peakSmoothBuf[j + xmin];
                        maxInWindow = std::max(maxInWindow, v);
                    }
                    if (d > maxInWindow
                        && d > 1.0 * m_peakSmoothBuf[ix - pw]
                        && d > 1.0 * m_peakSmoothBuf[ix + pw])
                    {
                        const qreal y = std::max(std::min(
                            panddBGainFactor * ((m_PandMaxdB - logFactor * log10f(detectSource[ix]))),
                            plotHeight - 0.0), 0.0);
                        m_Peaks[ix] = y;
                    }
                }
            }

            // Paint peaks with shadow
            QPen peakPen(m_maxFftColor, m_DPR);
            QPen peakShadowPen(Qt::black, m_DPR);
            peakPen.setWidthF(m_DPR);
            for(auto peakx : m_Peaks.keys()) {
                const qreal peakv = m_Peaks.value(peakx);
                painter2.setPen(peakShadowPen);
                painter2.drawEllipse(QRectF(
                    peakx - 5.0 * m_DPR + shadowOffset, peakv - 5.0 * m_DPR + shadowOffset,
                    10.0 * m_DPR, 10.0 * m_DPR));
                painter2.setPen(peakPen);
                painter2.drawEllipse(QRectF(peakx - 5.0 * m_DPR, peakv - 5.0 * m_DPR,
                                           10.0 * m_DPR, 10.0 * m_DPR));
            }
        }

        // Update the overlay if needed
        if (m_DrawOverlay)
        {
            drawOverlay();
            m_DrawOverlay = false;
        }

        // Draw overlay over plot
        painter2.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter2.drawPixmap(QPointF(0.0, 0.0), m_OverlayPixmap);
    }

    // trigger a new paintEvent
    update();
}

void CPlotter::setRunningState(bool running)
{
    // Reset waterfall time and clear waterfall, since time is no longer correct
    if (running and !m_Running)
    {
        setWaterfallSpan(wf_span);

        // Invalidate any existing data
        m_MaxHoldValid = false;
        m_MinHoldValid = false;
        m_IIRValid = false;
        m_histIIRValid = false;
        m_histMaxIIR = std::numeric_limits<float>::min();

    }

    m_Running = running;
}

/**
 * Set new FFT data.
 * @param fftData Pointer to the new FFT data (same data for pandapter and waterfall).
 * @param size The FFT size.
 *
 * When FFT data is set using this method, the same data will be used for both the
 * pandapter and the waterfall.
 */
void CPlotter::setNewFftData(const float *fftData, int size)
{
    // Make sure zeros don't get through to log calcs
    const float fmin = std::numeric_limits<float>::min();

    if (size != m_fftDataSize)
    {
        // Reallocate and invalidate IIRs
        m_fftData.resize(size);
        m_fftIIR.resize(size);

        m_MaxHoldValid = false;
        m_MinHoldValid = false;
        m_IIRValid = false;

        m_histIIRValid = false;
        m_histMaxIIR = std::numeric_limits<float>::min();

        m_fftDataSize = size;
    }

    // For V, V^2 -> V/RBW
    if (m_PlotScale == PLOT_SCALE_V) {
        for (int i = 0; i < size; ++i)
            // fftData[i] = sqrtf(fftData[i]) / (float)size;
            m_fftData[i] = std::max(fftData[i] / (float)size / (float)size, fmin);
    }
    // For DBM, give choose dBm/RBW or dBm/Hz, scaled to 50 ohm.
    // 1000 V^2 / 2R
    else
    {
        float _pwr_scale;
        if (m_PlotPer == PLOT_PER_RBW)
            _pwr_scale = 1000.0 / (2.0 * 50.0 * (float)size * (float)size);
        else
            _pwr_scale = 1000.0 / (2.0 * 50.0 * (float)size * (float)m_SampleFreq);
        const float pwr_scale = _pwr_scale;
        for (int i = 0; i < size; ++i)
            m_fftData[i] = std::max(fftData[i] * pwr_scale, fmin);
    }

    // Update IIR. If IIR is invalid, set alpha to use latest value. Since the
    // IIR is linear data and users would like to see symmetric attack/decay on
    // the logarithmic y-axis, IIR is in terms of multiplication rather than
    // addition.

    // Time constant, taking update rate into account. Attack and decay rate of
    // change in dB/sec should not visibly change with FFT rate.
    const double _a = pow((double)fft_rate, -1.75 * (1.0 - m_alpha));

    // Make the slider vs alpha nonlinear
    const double gamma = 0.7;
    const double a = pow(_a, gamma);

    // Shortcut expensive pow() if not needed
    const bool needIIR = m_IIRValid                         // Initializing
                      && a != 1.0;                          // IIR is NOP

    if (needIIR) {
        for (int i = 0; i < size; ++i)
        {
            const double v = m_fftData[i];
            const double iir = m_fftIIR[i];
            m_fftIIR[i] = iir * powf(v / iir, a);
        }
    }
    else
    {
        memcpy(m_fftIIR.data(), m_fftData.data(), size * sizeof(float));
    }

    m_IIRValid = true;

    draw(true);
}

void CPlotter::setFftAvg(float avg)
{
    m_alpha = avg;
}

void CPlotter::setFftRange(float min, float max)
{
    setWaterfallRange(min, max);
    setPandapterRange(min, max);
}

void CPlotter::setPandapterRange(float min, float max)
{
    if (out_of_range(min, max))
        return;

    m_PandMindB = min;
    m_PandMaxdB = max;
    m_MaxHoldValid = false;
    m_MinHoldValid = false;
    m_histIIRValid = false;
    updateOverlay();
}

void CPlotter::setWaterfallRange(float min, float max)
{
    if (out_of_range(min, max))
        return;

    m_WfMindB = min;
    m_WfMaxdB = max;
    // no overlay change is necessary
}

// Called to draw an overlay bitmap containing grid and text that
// does not need to be recreated every fft data update.
void CPlotter::drawOverlay()
{
    if (m_OverlayPixmap.isNull())
        return;

    int     x;
    qreal   pixperdiv;
    qreal   adjoffset;
    qreal   dbstepsize;
    qreal   mindbadj;
    QFontMetricsF metrics(m_Font);
    const qreal shadowOffset = metrics.height() / 20.0;
    qreal   w = m_OverlayPixmap.width();
    qreal   h = m_OverlayPixmap.height();

    m_OverlayPixmap.fill(Qt::transparent);
    QPainter painter(&m_OverlayPixmap);
    // painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(m_Font);

    QList<BookmarkInfo> tags;

    // X and Y axis areas
    m_YAxisWidth = metrics.boundingRect("-120").width() + 2 * HOR_MARGIN;
    m_XAxisYCenter = h - metrics.height()/2;
    qreal xAxisHeight = metrics.height() + 2 * VER_MARGIN;
    qreal xAxisTop = h - xAxisHeight;
    qreal fLabelTop = xAxisTop + VER_MARGIN;

    if (m_BookmarksEnabled || m_DXCSpotsEnabled)
    {
        m_Taglist.clear();
        static const QFontMetricsF fm(painter.font());
        static const qreal fontHeight = fm.ascent() + 1;
        static const qreal slant = 5;
        static const qreal levelHeight = fontHeight + 5;
        static const qreal nLevels = h / (levelHeight + slant);
        if (m_BookmarksEnabled)
        {
            tags = Bookmarks::Get().getBookmarksInRange(m_CenterFreq + m_FftCenter - m_Span / 2,
                                                        m_CenterFreq + m_FftCenter + m_Span / 2);
        }
        else
        {
            tags.clear();
        }
        if (m_DXCSpotsEnabled)
        {
            QList<DXCSpotInfo> dxcspots = DXCSpots::Get().getDXCSpotsInRange(m_CenterFreq + m_FftCenter - m_Span / 2,
                                                                             m_CenterFreq + m_FftCenter + m_Span / 2);
            QListIterator<DXCSpotInfo> iter(dxcspots);
            while(iter.hasNext())
            {
                BookmarkInfo tempDXCSpot;
                DXCSpotInfo IterDXCSpot = iter.next();
                tempDXCSpot.name = IterDXCSpot.name;
                tempDXCSpot.frequency = IterDXCSpot.frequency;
                tags.append(tempDXCSpot);
            }
            std::stable_sort(tags.begin(),tags.end());
        }
        QVector<int> tagEnd(nLevels + 1);
        for (auto & tag : tags)
        {
            x = xFromFreq(tag.frequency);
            qreal nameWidth = fm.boundingRect(tag.name).width();

            int level = 0;
            while(level < nLevels && tagEnd[level] > x)
                level++;

            if(level >= nLevels)
            {
                level = 0;
                if (tagEnd[level] > x)
                    continue; // no overwrite at level 0
            }

            tagEnd[level] = x + nameWidth + slant - 1;

            const auto levelNHeight = level * levelHeight;
            const auto levelNHeightBottom = levelNHeight + fontHeight;
            const auto levelNHeightBottomSlant = levelNHeightBottom + slant;

            m_Taglist.append(qMakePair(QRect(x, levelNHeight, nameWidth + slant, fontHeight), tag.frequency));

            QColor color = QColor(tag.GetColor());
            color.setAlpha(100);
            // Vertical line
            painter.setPen(QPen(color, m_DPR, Qt::DashLine));
            painter.drawLine(QPointF(x, levelNHeightBottomSlant), QPointF(x, xAxisTop));

            // Horizontal line
            painter.setPen(QPen(color, m_DPR, Qt::SolidLine));
            painter.drawLine(QPointF(x + slant, levelNHeightBottom),
                             QPointF(x + nameWidth + slant - 1,
                             levelNHeightBottom));
            // Diagonal line
            painter.drawLine(QPointF(x + 1, levelNHeightBottomSlant - 1),
                             QPointF(x + slant - 1, levelNHeightBottom + 1));

            color.setAlpha(255);
            painter.setPen(QPen(color, 2.0 * m_DPR, Qt::SolidLine));
            painter.drawText(x + slant, levelNHeight, nameWidth,
                             fontHeight, Qt::AlignVCenter | Qt::AlignHCenter,
                             tag.name);
        }
    }

    if (m_BandPlanEnabled)
    {
        QList<BandInfo> bands = BandPlan::Get().getBandsInRange(m_CenterFreq + m_FftCenter - m_Span / 2,
                                                                m_CenterFreq + m_FftCenter + m_Span / 2);

        m_BandPlanHeight = metrics.height() + VER_MARGIN;
        for (auto & band : bands)
        {
            int band_left = xFromFreq(band.minFrequency);
            int band_right = xFromFreq(band.maxFrequency);
            int band_width = band_right - band_left;
            QRectF rect(band_left, xAxisTop - m_BandPlanHeight, band_width, m_BandPlanHeight);
            painter.fillRect(rect, band.color);
            QString band_label = band.name + " (" + band.modulation + ")";
            qreal textWidth = metrics.boundingRect(band_label).width();
            if (band_left < w && band_width > textWidth + 20)
            {
                painter.setOpacity(1.0);
                QRectF textRect(band_left, xAxisTop - m_BandPlanHeight, band_width, metrics.height());
                painter.setPen(QPen(QColor(PLOTTER_TEXT_COLOR), m_DPR));
                painter.drawText(textRect, Qt::AlignCenter, band_label);
            }
        }
    }

    if (m_CenterLineEnabled)
    {
        x = xFromFreq(m_CenterFreq);
        if (x > 0 && x < w)
        {
            painter.setPen(QPen(QColor(PLOTTER_CENTER_LINE_COLOR), m_DPR));
            painter.drawLine(QPointF(x, 0), QPointF(x, xAxisTop));
        }
    }

    if (m_MarkersEnabled)
    {
        QBrush brush;
        brush.setColor(QColor(PLOTTER_MARKER_COLOR));
        brush.setStyle(Qt::SolidPattern);
        painter.setPen(QPen(QColor(PLOTTER_MARKER_COLOR), m_DPR));

        qreal markerSize = metrics.height() / 2;

        if (m_MarkerFreqA != MARKER_OFF) {
            x = xFromFreq(m_MarkerFreqA);
            m_MarkerAX = x;
            if (x > 0 && x < w) {
                QPolygon poly;
                QPainterPath path;
                poly << QPoint(x - markerSize/2, 0)
                     << QPoint(x + markerSize/2, 0)
                     << QPoint(x, markerSize);
                path.addPolygon(poly);
                painter.drawPolygon(poly);
                painter.fillPath(path, brush);
                painter.drawLine(x, markerSize, x, xAxisTop);
                painter.drawStaticText(QPointF(x + markerSize/2, 0), QStaticText("A"));
            }
        }

        if (m_MarkerFreqB != MARKER_OFF) {
            x = xFromFreq(m_MarkerFreqB);
            m_MarkerBX = x;
            if (x > 0 && x < w) {
                QPolygon poly;
                QPainterPath path;
                poly << QPoint(x - markerSize/2, 0)
                     << QPoint(x + markerSize/2, 0)
                     << QPoint(x, markerSize);
                path.addPolygon(poly);
                painter.drawPolygon(poly);
                painter.fillPath(path, brush);
                painter.drawLine(x, markerSize, x, xAxisTop);
                painter.drawStaticText(QPointF(x + markerSize/2, 0), QStaticText("B"));
            }
        }
    }

    // Frequency grid
    qint64  StartFreq = m_CenterFreq + m_FftCenter - m_Span / 2;
    QString label;
    label.setNum(float((StartFreq + m_Span) / m_FreqUnits), 'f', m_FreqDigits);
    calcDivSize(StartFreq, StartFreq + m_Span,
                qMin(w / (metrics.boundingRect(label).width() + metrics.boundingRect("O").width()),
                     (qreal)HORZ_DIVS_MAX),
                m_StartFreqAdj, m_FreqPerDiv, m_HorDivs);
    pixperdiv = w * (float) m_FreqPerDiv / (float) m_Span;
    adjoffset = pixperdiv * float (m_StartFreqAdj - StartFreq) / (float) m_FreqPerDiv;

    // Hairline for grid lines
    painter.setPen(QPen(QColor(PLOTTER_GRID_COLOR), 0.0, Qt::DotLine));
    for (int i = 0; i <= m_HorDivs; i++)
    {
        qreal xD = (double)i * pixperdiv + adjoffset;
        if (xD > m_YAxisWidth)
            painter.drawLine(xD, 0, xD, xAxisTop);
    }

    // draw frequency values (x axis)
    makeFrequencyStrs();
    for (int i = 0; i <= m_HorDivs; i++)
    {
        qreal xD = (qreal)i * pixperdiv + adjoffset;
        if (xD > m_YAxisWidth)
        {
            // Shadow
            QRectF shadowRect(xD + shadowOffset - w/2, fLabelTop + shadowOffset,
                              w, metrics.height());
            painter.setPen(QPen(QColor(Qt::black)));
            painter.drawText(shadowRect, Qt::AlignHCenter|Qt::AlignBottom, m_HDivText[i]);
            // Foreground
            QRectF textRect(xD - w/2, fLabelTop,
                            w, metrics.height());
            painter.setPen(QPen(QColor(PLOTTER_TEXT_COLOR)));
            painter.drawText(textRect, Qt::AlignHCenter|Qt::AlignBottom, m_HDivText[i]);
        }
    }

    // Level grid
    qint64 mindBAdj64 = 0;
    qint64 dbDivSize = 0;

    calcDivSize((qint64) m_PandMindB, (qint64) m_PandMaxdB,
                qMax(h / (m_VdivDelta * m_DPR), (qreal)VERT_DIVS_MIN),
                mindBAdj64, dbDivSize, m_VerDivs);

    dbstepsize = (float) dbDivSize;
    mindbadj = mindBAdj64;

    pixperdiv = h * (float)dbstepsize / (m_PandMaxdB - m_PandMindB);
    adjoffset = h * (mindbadj - m_PandMindB) / (m_PandMaxdB - m_PandMindB);

    qCDebug(plotter) << "minDb =" << m_PandMindB << "maxDb =" << m_PandMaxdB
                     << "mindbadj =" << mindbadj << "dbstepsize =" << dbstepsize
                     << "pixperdiv =" << pixperdiv << "adjoffset =" << adjoffset;

    // Hairline for grid lines
    painter.setPen(QPen(QColor(PLOTTER_GRID_COLOR), 0.0, Qt::DotLine));
    for (int i = 0; i <= m_VerDivs; i++)
    {
        qreal y = h - ((double)i * pixperdiv + adjoffset);
        if (y < h - xAxisHeight)
            painter.drawLine(m_YAxisWidth, y, w, y);
    }

    // draw amplitude values (y axis)
    for (int i = 0; i < m_VerDivs; i++)
    {
        qreal y = h - ((double)i * pixperdiv + adjoffset);
        qreal th = metrics.height();
        qreal shadowOffset = th / 20.0;
        if (y < h -xAxisHeight)
        {
            int dB = mindbadj + dbstepsize * i;
            // Shadow
            painter.setPen(QPen(QColor(Qt::black)));
            QRectF shadowRect(HOR_MARGIN + shadowOffset, y - th / 2 + shadowOffset,
                              m_YAxisWidth - 2 * HOR_MARGIN, th);
            painter.drawText(shadowRect, Qt::AlignRight|Qt::AlignVCenter, QString::number(dB));
            // Foreground
            painter.setPen(QPen(QColor(PLOTTER_TEXT_COLOR)));
            QRectF textRect(HOR_MARGIN, y - th / 2,
                            m_YAxisWidth - 2 * HOR_MARGIN, th);
            painter.drawText(textRect, Qt::AlignRight|Qt::AlignVCenter, QString::number(dB));
        }
    }

    // Draw demod filter box
    if (m_FilterBoxEnabled)
    {
        m_DemodFreqX = xFromFreq(m_DemodCenterFreq);
        m_DemodLowCutFreqX = xFromFreq(m_DemodCenterFreq + m_DemodLowCutFreq);
        m_DemodHiCutFreqX = xFromFreq(m_DemodCenterFreq + m_DemodHiCutFreq);

        int dw = m_DemodHiCutFreqX - m_DemodLowCutFreqX;

        painter.setOpacity(0.3);
        painter.fillRect(m_DemodLowCutFreqX, 0, dw, h,
                         QColor(PLOTTER_FILTER_BOX_COLOR));

        painter.setOpacity(1.0);
        painter.setPen(QPen(QColor(PLOTTER_FILTER_LINE_COLOR), m_DPR));
        painter.drawLine(m_DemodFreqX, 0, m_DemodFreqX, h);
    }

    // Draw a black line at the bottom of the plotter to separate it from the
    // waterfall
    painter.fillRect(QRect(0.0, h - 1.0 * m_DPR, w, 1.0 * m_DPR), Qt::black);

    painter.end();
}

// Create frequency division strings based on start frequency, span frequency,
// and frequency units.
// Places in QString array m_HDivText
// Keeps all strings the same fractional length
void CPlotter::makeFrequencyStrs()
{
    qint64  StartFreq = m_StartFreqAdj;
    double  freq;
    int     i,j;

    if ((1 == m_FreqUnits) || (m_FreqDigits == 0))
    {
        // if units is Hz then just output integer freq
        for (i = 0; i <= m_HorDivs; i++)
        {
            freq = (double)StartFreq/(double)m_FreqUnits;
            m_HDivText[i].setNum((int)freq);
            StartFreq += m_FreqPerDiv;
        }
        return;
    }
    // here if is fractional frequency values
    // so create max sized text based on frequency units
    for (i = 0; i <= m_HorDivs; i++)
    {
        freq = (double)StartFreq / (double)m_FreqUnits;
        m_HDivText[i].setNum(freq,'f', m_FreqDigits);
        StartFreq += m_FreqPerDiv;
    }
    // now find the division text with the longest non-zero digit
    // to the right of the decimal point.
    int max = 0;
    for (i = 0; i <= m_HorDivs; i++)
    {
        int dp = m_HDivText[i].indexOf('.');
        int l = m_HDivText[i].length()-1;
        for (j = l; j > dp; j--)
        {
            if (m_HDivText[i][j] != '0')
                break;
        }
        if ((j - dp) > max)
            max = j - dp;
    }
    // truncate all strings to maximum fractional length
    StartFreq = m_StartFreqAdj;
    for (i = 0; i <= m_HorDivs; i++)
    {
        freq = (double)StartFreq/(double)m_FreqUnits;
        m_HDivText[i].setNum(freq,'f', max);
        StartFreq += m_FreqPerDiv;
    }
}

// Convert from frequency to screen coordinate
int CPlotter::xFromFreq(qint64 freq)
{
    qreal w = m_Size.width() * m_DPR;
    double startFreq = (double)m_CenterFreq
                       + (double)m_FftCenter
                       - (double)m_Span / 2.0;
    int x = qRound(w * ((double)freq - startFreq) / (double)m_Span);
    if (x < 0)
        return 0;
    if (x > (int)w)
        return w;
    return x;
}

// Convert from screen coordinate to frequency
qint64 CPlotter::freqFromX(int x)
{
    double ratio = (double)x / (qreal)m_Size.width() / m_DPR;
    qint64 f = qRound64((double)m_CenterFreq + (double)m_FftCenter
                        - (double)m_Span / 2.0 + ratio * (double)m_Span);
    return f;
}

/** Calculate time offset of a given line on the waterfall */
quint64 CPlotter::msecFromY(int y)
{
    int h = m_OverlayPixmap.height();

    // ensure we are in the waterfall region
    if (y < h)
        return 0;

    qreal dy = (qreal)y - (qreal)h;

    if (msec_per_wfline > 0)
        return tlast_wf_drawn_ms - dy * msec_per_wfline;
    else
        return tlast_wf_drawn_ms - dy * 1000.0 / (double)fft_rate;
}

// Round frequency to click resolution value
qint64 CPlotter::roundFreq(qint64 freq, int resolution)
{
    qint64 delta = resolution;
    qint64 delta_2 = delta / 2;
    if (freq >= 0)
        return (freq - (freq + delta_2) % delta + delta_2);
    else
        return (freq - (freq + delta_2) % delta - delta_2);
}

// Clamp demod freqeuency limits of m_DemodCenterFreq
void CPlotter::clampDemodParameters()
{
    if(m_DemodLowCutFreq < m_FLowCmin)
        m_DemodLowCutFreq = m_FLowCmin;
    if(m_DemodLowCutFreq > m_FLowCmax)
        m_DemodLowCutFreq = m_FLowCmax;

    if(m_DemodHiCutFreq < m_FHiCmin)
        m_DemodHiCutFreq = m_FHiCmin;
    if(m_DemodHiCutFreq > m_FHiCmax)
        m_DemodHiCutFreq = m_FHiCmax;
}

void CPlotter::setDemodRanges(int FLowCmin, int FLowCmax,
                              int FHiCmin, int FHiCmax,
                              bool symetric)
{
    m_FLowCmin=FLowCmin;
    m_FLowCmax=FLowCmax;
    m_FHiCmin=FHiCmin;
    m_FHiCmax=FHiCmax;
    m_symetric=symetric;
    clampDemodParameters();
    updateOverlay();
}

void CPlotter::setCenterFreq(quint64 f)
{
    if((quint64)m_CenterFreq == f)
        return;

    qint64 offset = m_CenterFreq - m_DemodCenterFreq;

    m_CenterFreq = f;
    m_DemodCenterFreq = m_CenterFreq - offset;

    m_MaxHoldValid = false;
    m_MinHoldValid = false;
    m_histIIRValid = false;
    m_IIRValid = false;

    updateOverlay();
}

// Invalidate overlay. If not running, force a redraw.
void CPlotter::updateOverlay()
{
    m_DrawOverlay = true;
    if (!m_Running)
    {
        draw(false);
    }
}

/** Reset horizontal zoom to 100% and centered around 0. */
void CPlotter::resetHorizontalZoom(void)
{
    setFftCenterFreq(0);
    setSpanFreq((qint32)m_SampleFreq);
    emit newZoomLevel(1.0);
    m_MaxHoldValid = false;
    m_MinHoldValid = false;
    m_histIIRValid = false;
    updateOverlay();
}

/** Center FFT plot around 0 (corresponds to center freq). */
void CPlotter::moveToCenterFreq()
{
    setFftCenterFreq(0);
    m_MaxHoldValid = false;
    m_MinHoldValid = false;
    m_histIIRValid = false;
    updateOverlay();
}

/** Center FFT plot around the demodulator frequency. */
void CPlotter::moveToDemodFreq()
{
    setFftCenterFreq(m_DemodCenterFreq-m_CenterFreq);
    m_MaxHoldValid = false;
    m_MinHoldValid = false;
    m_histIIRValid = false;
    updateOverlay();
}

/** Set FFT plot color. */
void CPlotter::setFftPlotColor(const QColor& color)
{
    m_avgFftColor = color;
    m_maxFftColor = color;
    // m_maxFftColor.setAlpha(192);
    m_FftFillCol = color;
    m_FftFillCol.setAlpha(26);
    m_MaxHoldColor = color;
    m_MaxHoldColor.setAlpha(80);
    m_MinHoldColor = color;
    m_MinHoldColor.setAlpha(80);
}

/** Enable/disable filling the area below the FFT plot. */
void CPlotter::enableFftFill(bool enabled)
{
    m_FftFill = enabled;
}

/** Set peak hold on or off. */
void CPlotter::enableMaxHold(bool enabled)
{
    m_MaxHoldActive = enabled;
    m_MaxHoldValid = false;
}

/** Set min hold on or off. */
void CPlotter::enableMinHold(bool enabled)
{
    m_MinHoldActive = enabled;
    m_MinHoldValid = false;
}

/**
 * Set peak detection on or off.
 * @param enabled The new state of peak detection.
 * @param c Minimum distance of peaks from mean, in multiples of standard deviation.
 */
void CPlotter::enablePeakDetect(bool enabled)
{
    m_PeakDetectActive = enabled;
}

void CPlotter::enableBandPlan(bool enabled)
{
    m_BandPlanEnabled = enabled;
    updateOverlay();
}

void CPlotter::enableMarkers(bool enabled)
{
    m_MarkersEnabled = enabled;
}

void CPlotter::setMarkers(qint64 a, qint64 b)
{
    // Invalidate x positions
    m_MarkerAX = -1;
    m_MarkerBX = -1;

    m_MarkerFreqA = a;
    m_MarkerFreqB = b;

    updateOverlay();
}

void CPlotter::calcDivSize (qint64 low, qint64 high, int divswanted, qint64 &adjlow, qint64 &step, int& divs)
{
    qCDebug(plotter) << "low:" << low;
    qCDebug(plotter) << "high:" << high;
    qCDebug(plotter) << "divswanted:" << divswanted;

    if (divswanted == 0)
        return;

    static const qint64 stepTable[] = { 1, 2, 5 };
    static const int stepTableSize = sizeof (stepTable) / sizeof (stepTable[0]);
    qint64 multiplier = 1;
    step = 1;
    divs = high - low;
    int index = 0;
    adjlow = (low / step) * step;

    while (divs > divswanted)
    {
        step = stepTable[index] * multiplier;
        divs = int ((high - low) / step);
        adjlow = (low / step) * step;
        index = index + 1;
        if (index == stepTableSize)
        {
            index = 0;
            multiplier = multiplier * 10;
        }
    }
    if (adjlow < low)
        adjlow += step;

    qCDebug(plotter) << "adjlow:" << adjlow;
    qCDebug(plotter) << "step:" << step;
    qCDebug(plotter) << "divs:" << divs;
}

void CPlotter::showToolTip(QMouseEvent* event, QString toolTipText)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QToolTip::showText(event->globalPos(), toolTipText, this);
#else
    QToolTip::showText(event->globalPosition().toPoint(), toolTipText, this);
#endif
}

// contributed by Chris Kuethe @ckuethe
// source https://ai.googleblog.com/2019/08/turbo-improved-rainbow-colormap-for.html
unsigned char turbo[256][3] = {
        {48,18,59}, {50,21,67},    {51,24,74},
        {52,27,81},   {53,30,88},   {54,33,95},    {55,36,102},   {56,39,109},
        {57,42,115},  {58,45,121},  {59,47,128},   {60,50,134},   {61,53,139},
        {62,56,145},  {63,59,151},  {63,62,156},   {64,64,162},   {65,67,167},
        {65,70,172},  {66,73,177},  {66,75,181},   {67,78,186},   {68,81,191},
        {68,84,195},  {68,86,199},  {69,89,203},   {69,92,207},   {69,94,211},
        {70,97,214},  {70,100,218}, {70,102,221},  {70,105,224},  {70,107,227},
        {71,110,230}, {71,113,233}, {71,115,235},  {71,118,238},  {71,120,240},
        {71,123,242}, {70,125,244}, {70,128,246},  {70,130,248},  {70,133,250},
        {70,135,251}, {69,138,252}, {69,140,253},  {68,143,254},  {67,145,254},
        {66,148,255}, {65,150,255}, {64,153,255},  {62,155,254},  {61,158,254},
        {59,160,253}, {58,163,252}, {56,165,251},  {55,168,250},  {53,171,248},
        {51,173,247}, {49,175,245}, {47,178,244},  {46,180,242},  {44,183,240},
        {42,185,238}, {40,188,235}, {39,190,233},  {37,192,231},  {35,195,228},
        {34,197,226}, {32,199,223}, {31,201,221},  {30,203,218},  {28,205,216},
        {27,208,213}, {26,210,210}, {26,212,208},  {25,213,205},  {24,215,202},
        {24,217,200}, {24,219,197}, {24,221,194},  {24,222,192},  {24,224,189},
        {25,226,187}, {25,227,185}, {26,228,182},  {28,230,180},  {29,231,178},
        {31,233,175}, {32,234,172}, {34,235,170},  {37,236,167},  {39,238,164},
        {42,239,161}, {44,240,158}, {47,241,155},  {50,242,152},  {53,243,148},
        {56,244,145}, {60,245,142}, {63,246,138},  {67,247,135},  {70,248,132},
        {74,248,128}, {78,249,125}, {82,250,122},  {85,250,118},  {89,251,115},
        {93,252,111}, {97,252,108}, {101,253,105}, {105,253,102}, {109,254,98},
        {113,254,95}, {117,254,92}, {121,254,89},  {125,255,86},  {128,255,83},
        {132,255,81}, {136,255,78}, {139,255,75},  {143,255,73},  {146,255,71},
        {150,254,68}, {153,254,66}, {156,254,64},  {159,253,63},  {161,253,61},
        {164,252,60}, {167,252,58}, {169,251,57},  {172,251,56},  {175,250,55},
        {177,249,54}, {180,248,54}, {183,247,53},  {185,246,53},  {188,245,52},
        {190,244,52}, {193,243,52}, {195,241,52},  {198,240,52},  {200,239,52},
        {203,237,52}, {205,236,52}, {208,234,52},  {210,233,53},  {212,231,53},
        {215,229,53}, {217,228,54}, {219,226,54},  {221,224,55},  {223,223,55},
        {225,221,55}, {227,219,56}, {229,217,56},  {231,215,57},  {233,213,57},
        {235,211,57}, {236,209,58}, {238,207,58},  {239,205,58},  {241,203,58},
        {242,201,58}, {244,199,58}, {245,197,58},  {246,195,58},  {247,193,58},
        {248,190,57}, {249,188,57}, {250,186,57},  {251,184,56},  {251,182,55},
        {252,179,54}, {252,177,54}, {253,174,53},  {253,172,52},  {254,169,51},
        {254,167,50}, {254,164,49}, {254,161,48},  {254,158,47},  {254,155,45},
        {254,153,44}, {254,150,43}, {254,147,42},  {254,144,41},  {253,141,39},
        {253,138,38}, {252,135,37}, {252,132,35},  {251,129,34},  {251,126,33},
        {250,123,31}, {249,120,30}, {249,117,29},  {248,114,28},  {247,111,26},
        {246,108,25}, {245,105,24}, {244,102,23},  {243,99,21},   {242,96,20},
        {241,93,19},  {240,91,18},  {239,88,17},   {237,85,16},   {236,83,15},
        {235,80,14},  {234,78,13},  {232,75,12},   {231,73,12},   {229,71,11},
        {228,69,10},  {226,67,10},  {225,65,9},    {223,63,8},    {221,61,8},
        {220,59,7},   {218,57,7},   {216,55,6},    {214,53,6},    {212,51,5},
        {210,49,5},   {208,47,5},   {206,45,4},    {204,43,4},    {202,42,4},
        {200,40,3},   {197,38,3},   {195,37,3},    {193,35,2},    {190,33,2},
        {188,32,2},   {185,30,2},   {183,29,2},    {180,27,1},    {178,26,1},
        {175,24,1},   {172,23,1},   {169,22,1},    {167,20,1},    {164,19,1},
        {161,18,1},   {158,16,1},   {155,15,1},    {152,14,1},    {149,13,1},
        {146,11,1},   {142,10,1},   {139,9,2},     {136,8,2},     {133,7,2},
        {129,6,2},    {126,5,2},    {122,4,3}
};

// contributed by @devnulling
unsigned char plasma[256][3] = {
        {12, 7, 134},   {16, 7, 135},   {19, 6, 137},   {21, 6, 138},   {24, 6, 139},
        {27, 6, 140},   {29, 6, 141},   {31, 5, 142},   {33, 5, 143},   {35, 5, 144},
        {37, 5, 145},   {39, 5, 146},   {41, 5, 147},   {43, 5, 148},   {45, 4, 148},
        {47, 4, 149},   {49, 4, 150},   {51, 4, 151},   {52, 4, 152},   {54, 4, 152},
        {56, 4, 153},   {58, 4, 154},   {59, 3, 154},   {61, 3, 155},   {63, 3, 156},
        {64, 3, 156},   {66, 3, 157},   {68, 3, 158},   {69, 3, 158},   {71, 2, 159},
        {73, 2, 159},   {74, 2, 160},   {76, 2, 161},   {78, 2, 161},   {79, 2, 162},
        {81, 1, 162},   {82, 1, 163},   {84, 1, 163},   {86, 1, 163},   {87, 1, 164},
        {89, 1, 164},   {90, 0, 165},   {92, 0, 165},   {94, 0, 165},   {95, 0, 166},
        {97, 0, 166},   {98, 0, 166},   {100, 0, 167},  {101, 0, 167},  {103, 0, 167},
        {104, 0, 167},  {106, 0, 167},  {108, 0, 168},  {109, 0, 168},  {111, 0, 168},
        {112, 0, 168},  {114, 0, 168},  {115, 0, 168},  {117, 0, 168},  {118, 1, 168},
        {120, 1, 168},  {121, 1, 168},  {123, 2, 168},  {124, 2, 167},  {126, 3, 167},
        {127, 3, 167},  {129, 4, 167},  {130, 4, 167},  {132, 5, 166},  {133, 6, 166},
        {134, 7, 166},  {136, 7, 165},  {137, 8, 165},  {139, 9, 164},  {140, 10, 164},
        {142, 12, 164}, {143, 13, 163}, {144, 14, 163}, {146, 15, 162}, {147, 16, 161},
        {149, 17, 161}, {150, 18, 160}, {151, 19, 160}, {153, 20, 159}, {154, 21, 158},
        {155, 23, 158}, {157, 24, 157}, {158, 25, 156}, {159, 26, 155}, {160, 27, 155},
        {162, 28, 154}, {163, 29, 153}, {164, 30, 152}, {165, 31, 151}, {167, 33, 151},
        {168, 34, 150}, {169, 35, 149}, {170, 36, 148}, {172, 37, 147}, {173, 38, 146},
        {174, 39, 145}, {175, 40, 144}, {176, 42, 143}, {177, 43, 143}, {178, 44, 142},
        {180, 45, 141}, {181, 46, 140}, {182, 47, 139}, {183, 48, 138}, {184, 50, 137},
        {185, 51, 136}, {186, 52, 135}, {187, 53, 134}, {188, 54, 133}, {189, 55, 132},
        {190, 56, 131}, {191, 57, 130}, {192, 59, 129}, {193, 60, 128}, {194, 61, 128},
        {195, 62, 127}, {196, 63, 126}, {197, 64, 125}, {198, 65, 124}, {199, 66, 123},
        {200, 68, 122}, {201, 69, 121}, {202, 70, 120}, {203, 71, 119}, {204, 72, 118},
        {205, 73, 117}, {206, 74, 117}, {207, 75, 116}, {208, 77, 115}, {209, 78, 114},
        {209, 79, 113}, {210, 80, 112}, {211, 81, 111}, {212, 82, 110}, {213, 83, 109},
        {214, 85, 109}, {215, 86, 108}, {215, 87, 107}, {216, 88, 106}, {217, 89, 105},
        {218, 90, 104}, {219, 91, 103}, {220, 93, 102}, {220, 94, 102}, {221, 95, 101},
        {222, 96, 100}, {223, 97, 99},  {223, 98, 98},  {224, 100, 97}, {225, 101, 96},
        {226, 102, 96}, {227, 103, 95}, {227, 104, 94}, {228, 106, 93}, {229, 107, 92},
        {229, 108, 91}, {230, 109, 90}, {231, 110, 90}, {232, 112, 89}, {232, 113, 88},
        {233, 114, 87}, {234, 115, 86}, {234, 116, 85}, {235, 118, 84}, {236, 119, 84},
        {236, 120, 83}, {237, 121, 82}, {237, 123, 81}, {238, 124, 80}, {239, 125, 79},
        {239, 126, 78}, {240, 128, 77}, {240, 129, 77}, {241, 130, 76}, {242, 132, 75},
        {242, 133, 74}, {243, 134, 73}, {243, 135, 72}, {244, 137, 71}, {244, 138, 71},
        {245, 139, 70}, {245, 141, 69}, {246, 142, 68}, {246, 143, 67}, {246, 145, 66},
        {247, 146, 65}, {247, 147, 65}, {248, 149, 64}, {248, 150, 63}, {248, 152, 62},
        {249, 153, 61}, {249, 154, 60}, {250, 156, 59}, {250, 157, 58}, {250, 159, 58},
        {250, 160, 57}, {251, 162, 56}, {251, 163, 55}, {251, 164, 54}, {252, 166, 53},
        {252, 167, 53}, {252, 169, 52}, {252, 170, 51}, {252, 172, 50}, {252, 173, 49},
        {253, 175, 49}, {253, 176, 48}, {253, 178, 47}, {253, 179, 46}, {253, 181, 45},
        {253, 182, 45}, {253, 184, 44}, {253, 185, 43}, {253, 187, 43}, {253, 188, 42},
        {253, 190, 41}, {253, 192, 41}, {253, 193, 40}, {253, 195, 40}, {253, 196, 39},
        {253, 198, 38}, {252, 199, 38}, {252, 201, 38}, {252, 203, 37}, {252, 204, 37},
        {252, 206, 37}, {251, 208, 36}, {251, 209, 36}, {251, 211, 36}, {250, 213, 36},
        {250, 214, 36}, {250, 216, 36}, {249, 217, 36}, {249, 219, 36}, {248, 221, 36},
        {248, 223, 36}, {247, 224, 36}, {247, 226, 37}, {246, 228, 37}, {246, 229, 37},
        {245, 231, 38}, {245, 233, 38}, {244, 234, 38}, {243, 236, 38}, {243, 238, 38},
        {242, 240, 38}, {242, 241, 38}, {241, 243, 38}, {240, 245, 37}, {240, 246, 35},
        {239, 248, 33}
};

// contributed by @Piruzzolo
float viridis[256][3] = {
        { 0.267004, 0.004874, 0.329415 },
        { 0.268510, 0.009605, 0.335427 },
        { 0.269944, 0.014625, 0.341379 },
        { 0.271305, 0.019942, 0.347269 },
        { 0.272594, 0.025563, 0.353093 },
        { 0.273809, 0.031497, 0.358853 },
        { 0.274952, 0.037752, 0.364543 },
        { 0.276022, 0.044167, 0.370164 },
        { 0.277018, 0.050344, 0.375715 },
        { 0.277941, 0.056324, 0.381191 },
        { 0.278791, 0.062145, 0.386592 },
        { 0.279566, 0.067836, 0.391917 },
        { 0.280267, 0.073417, 0.397163 },
        { 0.280894, 0.078907, 0.402329 },
        { 0.281446, 0.084320, 0.407414 },
        { 0.281924, 0.089666, 0.412415 },
        { 0.282327, 0.094955, 0.417331 },
        { 0.282656, 0.100196, 0.422160 },
        { 0.282910, 0.105393, 0.426902 },
        { 0.283091, 0.110553, 0.431554 },
        { 0.283197, 0.115680, 0.436115 },
        { 0.283229, 0.120777, 0.440584 },
        { 0.283187, 0.125848, 0.444960 },
        { 0.283072, 0.130895, 0.449241 },
        { 0.282884, 0.135920, 0.453427 },
        { 0.282623, 0.140926, 0.457517 },
        { 0.282290, 0.145912, 0.461510 },
        { 0.281887, 0.150881, 0.465405 },
        { 0.281412, 0.155834, 0.469201 },
        { 0.280868, 0.160771, 0.472899 },
        { 0.280255, 0.165693, 0.476498 },
        { 0.279574, 0.170599, 0.479997 },
        { 0.278826, 0.175490, 0.483397 },
        { 0.278012, 0.180367, 0.486697 },
        { 0.277134, 0.185228, 0.489898 },
        { 0.276194, 0.190074, 0.493001 },
        { 0.275191, 0.194905, 0.496005 },
        { 0.274128, 0.199721, 0.498911 },
        { 0.273006, 0.204520, 0.501721 },
        { 0.271828, 0.209303, 0.504434 },
        { 0.270595, 0.214069, 0.507052 },
        { 0.269308, 0.218818, 0.509577 },
        { 0.267968, 0.223549, 0.512008 },
        { 0.266580, 0.228262, 0.514349 },
        { 0.265145, 0.232956, 0.516599 },
        { 0.263663, 0.237631, 0.518762 },
        { 0.262138, 0.242286, 0.520837 },
        { 0.260571, 0.246922, 0.522828 },
        { 0.258965, 0.251537, 0.524736 },
        { 0.257322, 0.256130, 0.526563 },
        { 0.255645, 0.260703, 0.528312 },
        { 0.253935, 0.265254, 0.529983 },
        { 0.252194, 0.269783, 0.531579 },
        { 0.250425, 0.274290, 0.533103 },
        { 0.248629, 0.278775, 0.534556 },
        { 0.246811, 0.283237, 0.535941 },
        { 0.244972, 0.287675, 0.537260 },
        { 0.243113, 0.292092, 0.538516 },
        { 0.241237, 0.296485, 0.539709 },
        { 0.239346, 0.300855, 0.540844 },
        { 0.237441, 0.305202, 0.541921 },
        { 0.235526, 0.309527, 0.542944 },
        { 0.233603, 0.313828, 0.543914 },
        { 0.231674, 0.318106, 0.544834 },
        { 0.229739, 0.322361, 0.545706 },
        { 0.227802, 0.326594, 0.546532 },
        { 0.225863, 0.330805, 0.547314 },
        { 0.223925, 0.334994, 0.548053 },
        { 0.221989, 0.339161, 0.548752 },
        { 0.220057, 0.343307, 0.549413 },
        { 0.218130, 0.347432, 0.550038 },
        { 0.216210, 0.351535, 0.550627 },
        { 0.214298, 0.355619, 0.551184 },
        { 0.212395, 0.359683, 0.551710 },
        { 0.210503, 0.363727, 0.552206 },
        { 0.208623, 0.367752, 0.552675 },
        { 0.206756, 0.371758, 0.553117 },
        { 0.204903, 0.375746, 0.553533 },
        { 0.203063, 0.379716, 0.553925 },
        { 0.201239, 0.383670, 0.554294 },
        { 0.199430, 0.387607, 0.554642 },
        { 0.197636, 0.391528, 0.554969 },
        { 0.195860, 0.395433, 0.555276 },
        { 0.194100, 0.399323, 0.555565 },
        { 0.192357, 0.403199, 0.555836 },
        { 0.190631, 0.407061, 0.556089 },
        { 0.188923, 0.410910, 0.556326 },
        { 0.187231, 0.414746, 0.556547 },
        { 0.185556, 0.418570, 0.556753 },
        { 0.183898, 0.422383, 0.556944 },
        { 0.182256, 0.426184, 0.557120 },
        { 0.180629, 0.429975, 0.557282 },
        { 0.179019, 0.433756, 0.557430 },
        { 0.177423, 0.437527, 0.557565 },
        { 0.175841, 0.441290, 0.557685 },
        { 0.174274, 0.445044, 0.557792 },
        { 0.172719, 0.448791, 0.557885 },
        { 0.171176, 0.452530, 0.557965 },
        { 0.169646, 0.456262, 0.558030 },
        { 0.168126, 0.459988, 0.558082 },
        { 0.166617, 0.463708, 0.558119 },
        { 0.165117, 0.467423, 0.558141 },
        { 0.163625, 0.471133, 0.558148 },
        { 0.162142, 0.474838, 0.558140 },
        { 0.160665, 0.478540, 0.558115 },
        { 0.159194, 0.482237, 0.558073 },
        { 0.157729, 0.485932, 0.558013 },
        { 0.156270, 0.489624, 0.557936 },
        { 0.154815, 0.493313, 0.557840 },
        { 0.153364, 0.497000, 0.557724 },
        { 0.151918, 0.500685, 0.557587 },
        { 0.150476, 0.504369, 0.557430 },
        { 0.149039, 0.508051, 0.557250 },
        { 0.147607, 0.511733, 0.557049 },
        { 0.146180, 0.515413, 0.556823 },
        { 0.144759, 0.519093, 0.556572 },
        { 0.143343, 0.522773, 0.556295 },
        { 0.141935, 0.526453, 0.555991 },
        { 0.140536, 0.530132, 0.555659 },
        { 0.139147, 0.533812, 0.555298 },
        { 0.137770, 0.537492, 0.554906 },
        { 0.136408, 0.541173, 0.554483 },
        { 0.135066, 0.544853, 0.554029 },
        { 0.133743, 0.548535, 0.553541 },
        { 0.132444, 0.552216, 0.553018 },
        { 0.131172, 0.555899, 0.552459 },
        { 0.129933, 0.559582, 0.551864 },
        { 0.128729, 0.563265, 0.551229 },
        { 0.127568, 0.566949, 0.550556 },
        { 0.126453, 0.570633, 0.549841 },
        { 0.125394, 0.574318, 0.549086 },
        { 0.124395, 0.578002, 0.548287 },
        { 0.123463, 0.581687, 0.547445 },
        { 0.122606, 0.585371, 0.546557 },
        { 0.121831, 0.589055, 0.545623 },
        { 0.121148, 0.592739, 0.544641 },
        { 0.120565, 0.596422, 0.543611 },
        { 0.120092, 0.600104, 0.542530 },
        { 0.119738, 0.603785, 0.541400 },
        { 0.119512, 0.607464, 0.540218 },
        { 0.119423, 0.611141, 0.538982 },
        { 0.119483, 0.614817, 0.537692 },
        { 0.119699, 0.618490, 0.536347 },
        { 0.120081, 0.622161, 0.534946 },
        { 0.120638, 0.625828, 0.533488 },
        { 0.121380, 0.629492, 0.531973 },
        { 0.122312, 0.633153, 0.530398 },
        { 0.123444, 0.636809, 0.528763 },
        { 0.124780, 0.640461, 0.527068 },
        { 0.126326, 0.644107, 0.525311 },
        { 0.128087, 0.647749, 0.523491 },
        { 0.130067, 0.651384, 0.521608 },
        { 0.132268, 0.655014, 0.519661 },
        { 0.134692, 0.658636, 0.517649 },
        { 0.137339, 0.662252, 0.515571 },
        { 0.140210, 0.665859, 0.513427 },
        { 0.143303, 0.669459, 0.511215 },
        { 0.146616, 0.673050, 0.508936 },
        { 0.150148, 0.676631, 0.506589 },
        { 0.153894, 0.680203, 0.504172 },
        { 0.157851, 0.683765, 0.501686 },
        { 0.162016, 0.687316, 0.499129 },
        { 0.166383, 0.690856, 0.496502 },
        { 0.170948, 0.694384, 0.493803 },
        { 0.175707, 0.697900, 0.491033 },
        { 0.180653, 0.701402, 0.488189 },
        { 0.185783, 0.704891, 0.485273 },
        { 0.191090, 0.708366, 0.482284 },
        { 0.196571, 0.711827, 0.479221 },
        { 0.202219, 0.715272, 0.476084 },
        { 0.208030, 0.718701, 0.472873 },
        { 0.214000, 0.722114, 0.469588 },
        { 0.220124, 0.725509, 0.466226 },
        { 0.226397, 0.728888, 0.462789 },
        { 0.232815, 0.732247, 0.459277 },
        { 0.239374, 0.735588, 0.455688 },
        { 0.246070, 0.738910, 0.452024 },
        { 0.252899, 0.742211, 0.448284 },
        { 0.259857, 0.745492, 0.444467 },
        { 0.266941, 0.748751, 0.440573 },
        { 0.274149, 0.751988, 0.436601 },
        { 0.281477, 0.755203, 0.432552 },
        { 0.288921, 0.758394, 0.428426 },
        { 0.296479, 0.761561, 0.424223 },
        { 0.304148, 0.764704, 0.419943 },
        { 0.311925, 0.767822, 0.415586 },
        { 0.319809, 0.770914, 0.411152 },
        { 0.327796, 0.773980, 0.406640 },
        { 0.335885, 0.777018, 0.402049 },
        { 0.344074, 0.780029, 0.397381 },
        { 0.352360, 0.783011, 0.392636 },
        { 0.360741, 0.785964, 0.387814 },
        { 0.369214, 0.788888, 0.382914 },
        { 0.377779, 0.791781, 0.377939 },
        { 0.386433, 0.794644, 0.372886 },
        { 0.395174, 0.797475, 0.367757 },
        { 0.404001, 0.800275, 0.362552 },
        { 0.412913, 0.803041, 0.357269 },
        { 0.421908, 0.805774, 0.351910 },
        { 0.430983, 0.808473, 0.346476 },
        { 0.440137, 0.811138, 0.340967 },
        { 0.449368, 0.813768, 0.335384 },
        { 0.458674, 0.816363, 0.329727 },
        { 0.468053, 0.818921, 0.323998 },
        { 0.477504, 0.821444, 0.318195 },
        { 0.487026, 0.823929, 0.312321 },
        { 0.496615, 0.826376, 0.306377 },
        { 0.506271, 0.828786, 0.300362 },
        { 0.515992, 0.831158, 0.294279 },
        { 0.525776, 0.833491, 0.288127 },
        { 0.535621, 0.835785, 0.281908 },
        { 0.545524, 0.838039, 0.275626 },
        { 0.555484, 0.840254, 0.269281 },
        { 0.565498, 0.842430, 0.262877 },
        { 0.575563, 0.844566, 0.256415 },
        { 0.585678, 0.846661, 0.249897 },
        { 0.595839, 0.848717, 0.243329 },
        { 0.606045, 0.850733, 0.236712 },
        { 0.616293, 0.852709, 0.230052 },
        { 0.626579, 0.854645, 0.223353 },
        { 0.636902, 0.856542, 0.216620 },
        { 0.647257, 0.858400, 0.209861 },
        { 0.657642, 0.860219, 0.203082 },
        { 0.668054, 0.861999, 0.196293 },
        { 0.678489, 0.863742, 0.189503 },
        { 0.688944, 0.865448, 0.182725 },
        { 0.699415, 0.867117, 0.175971 },
        { 0.709898, 0.868751, 0.169257 },
        { 0.720391, 0.870350, 0.162603 },
        { 0.730889, 0.871916, 0.156029 },
        { 0.741388, 0.873449, 0.149561 },
        { 0.751884, 0.874951, 0.143228 },
        { 0.762373, 0.876424, 0.137064 },
        { 0.772852, 0.877868, 0.131109 },
        { 0.783315, 0.879285, 0.125405 },
        { 0.793760, 0.880678, 0.120005 },
        { 0.804182, 0.882046, 0.114965 },
        { 0.814576, 0.883393, 0.110347 },
        { 0.824940, 0.884720, 0.106217 },
        { 0.835270, 0.886029, 0.102646 },
        { 0.845561, 0.887322, 0.099702 },
        { 0.855810, 0.888601, 0.097452 },
        { 0.866013, 0.889868, 0.095953 },
        { 0.876168, 0.891125, 0.095250 },
        { 0.886271, 0.892374, 0.095374 },
        { 0.896320, 0.893616, 0.096335 },
        { 0.906311, 0.894855, 0.098125 },
        { 0.916242, 0.896091, 0.100717 },
        { 0.926106, 0.897330, 0.104071 },
        { 0.935904, 0.898570, 0.108131 },
        { 0.945636, 0.899815, 0.112838 },
        { 0.955300, 0.901065, 0.118128 },
        { 0.964894, 0.902323, 0.123941 },
        { 0.974417, 0.903590, 0.130215 },
        { 0.983868, 0.904867, 0.136897 },
        { 0.993248, 0.906157, 0.143936 }
};


void CPlotter::setWfColormap(const QString &cmap)
{
    int i;

    if (cmap.compare("gqrx", Qt::CaseInsensitive) == 0)
    {
        for (i = 0; i < 256; i++)
        {
            // level 0: black background
            if (i < 20)
                m_ColorTbl[i].setRgb(0, 0, 0);
                // level 1: black -> blue
            else if ((i >= 20) && (i < 70))
                m_ColorTbl[i].setRgb(0, 0, 140*(i-20)/50);
                // level 2: blue -> light-blue / greenish
            else if ((i >= 70) && (i < 100))
                m_ColorTbl[i].setRgb(60*(i-70)/30, 125*(i-70)/30, 115*(i-70)/30 + 140);
                // level 3: light blue -> yellow
            else if ((i >= 100) && (i < 150))
                m_ColorTbl[i].setRgb(195*(i-100)/50 + 60, 130*(i-100)/50 + 125, 255-(255*(i-100)/50));
                // level 4: yellow -> red
            else if ((i >= 150) && (i < 250))
                m_ColorTbl[i].setRgb(255, 255-255*(i-150)/100, 0);
                // level 5: red -> white
            else if (i >= 250)
                m_ColorTbl[i].setRgb(255, 255*(i-250)/5, 255*(i-250)/5);
        }
    }
    else if (cmap.compare("turbo", Qt::CaseInsensitive) == 0)
    {
        for (i = 0; i < 256; i++)
            m_ColorTbl[i].setRgb(turbo[i][0], turbo[i][1], turbo[i][2]);
    }
    else if (cmap.compare("plasma",Qt::CaseInsensitive) == 0)
    {
        for (i = 0; i < 256; i++)
            m_ColorTbl[i].setRgb(plasma[i][0], plasma[i][1], plasma[i][2]);
    }
    else if (cmap.compare("whitehotcompressed",Qt::CaseInsensitive) == 0)
    {
        // contributed by @drmpeg @devnulling
        // for use with high quality spectrum paining
        // see https://gist.github.com/drmpeg/31a9a7dd6918856aeb60
        for (i = 0; i < 256; i++)
        {
            if (i < 64)
            {
                m_ColorTbl[i].setRgb(i*4, i*4, i*4);
            }
            else
            {
                m_ColorTbl[i].setRgb(255, 255, 255);
            }
        }
    }
    else if (cmap.compare("whitehot",Qt::CaseInsensitive) == 0)
    {
        for (i = 0; i < 256; i++)
            m_ColorTbl[i].setRgb(i, i, i);
    }
    else if (cmap.compare("blackhot",Qt::CaseInsensitive) == 0)
    {
        for (i = 0; i < 256; i++)
            m_ColorTbl[i].setRgb(255-i, 255-i, 255-i);
    }
    else if (cmap.compare("viridis",Qt::CaseInsensitive) == 0)
    {
        for (i = 0; i < 256; i++)
            m_ColorTbl[i].setRgb(F2B(viridis[i][0]), F2B(viridis[i][1]), F2B(viridis[i][2]));
    }
}
