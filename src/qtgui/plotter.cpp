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
#ifndef _MSC_VER
#include <sys/time.h>
#else

#include <Windows.h>
#include <cstdint>

int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
    // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
    static const uint64_t EPOCH = ((uint64_t) 116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime( &system_time );
    SystemTimeToFileTime( &system_time, &file_time );
    time =  ((uint64_t)file_time.dwLowDateTime )      ;
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec  = (long) ((time - EPOCH) / 10000000L);
    tp->tv_usec = (long) (system_time.wMilliseconds * 1000);
    return 0;
}

#endif
#include <QDateTime>
#include <QDebug>
#include <QFont>
#include <QPainter>
#include <QtGlobal>
#include <QToolTip>
#include "plotter.h"
#include "bookmarks.h"

//////////////////////////////////////////////////////////////////////
// Local defines
//////////////////////////////////////////////////////////////////////
#define CUR_CUT_DELTA 5		//cursor capture delta in pixels

// dB-axis constraints
#define REF_LEVEL_MAX   0.f
#define REF_LEVEL_MIN   -100.f
#define FFT_RANGE_MIN   10.f
#define FFT_RANGE_MAX   200.f


static inline bool val_is_out_of_range(float val, float min, float max)
{
    return ((val < min) || (val > max));
}

static inline bool out_of_range(float ref, float range)
{
    return (val_is_out_of_range(ref, REF_LEVEL_MIN, REF_LEVEL_MAX) ||
            val_is_out_of_range(range, FFT_RANGE_MIN, FFT_RANGE_MAX));
}

/** Current time in milliseconds since Epoch */
static inline quint64 time_ms(void)
{
    struct timeval  tval;

    gettimeofday(&tval, NULL);

    return 1e3 * tval.tv_sec + 1e-3 * tval.tv_usec;
}

#define STATUS_TIP \
    "Click, drag or scroll on spectrum to tune. " \
    "Drag and scroll X and Y axes for pan and zoom. " \
    "Drag filter edges to adjust filter."

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CPlotter::CPlotter(QWidget *parent) :
    QFrame(parent)
{

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_PaintOnScreen,false);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setMouseTracking(true);

    setTooltipsEnabled(false);
    setStatusTip(tr(STATUS_TIP));

    // default waterfall color scheme
    for (int i = 0; i < 256; i++)
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

    m_PeakHoldActive=false;
    m_PeakHoldValid=false;

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

    m_ClickResolution = 100;
    m_FilterClickResolution = 100;
    m_CursorCaptureDelta = CUR_CUT_DELTA;

    m_FilterBoxEnabled = true;
    m_CenterLineEnabled = true;

    m_Span = 96000;
    m_SampleFreq = 96000;

    m_HorDivs = 12;
    m_VerDivs = 6;
    m_MaxdB = 0;
    m_MindB = -115;
    m_dBStepSize = std::abs(m_MaxdB-m_MindB)/m_VerDivs;

    m_FreqUnits = 1000000;
    m_CursorCaptured = NOCAP;
    m_Running = false;
    m_DrawOverlay = true;
    m_2DPixmap = QPixmap(0,0);
    m_OverlayPixmap = QPixmap(0,0);
    m_WaterfallPixmap = QPixmap(0,0);
    m_Size = QSize(0,0);
    m_GrabPosition = 0;
    m_Percent2DScreen = 30;	//percent of screen used for 2D display
    m_VdivDelta = 40;
    m_HdivDelta = 80;

    m_FreqDigits = 3;

    m_Peaks = QMap<int,int>();
    setPeakDetection(false, 2);
    m_PeakHoldValid = false;

    setFftPlotColor(QColor(0xFF,0xFF,0xFF,0xFF));
    setFftFill(false);

    // always update waterfall
    tlast_wf_ms = 0;
    msec_per_wfline = 0;
    wf_span = 0;
    fft_rate = 15;
    memset(m_wfbuf, 255, MAX_SCREENSIZE);
}

CPlotter::~CPlotter()
{
}

//////////////////////////////////////////////////////////////////////
// Sizing interface
//////////////////////////////////////////////////////////////////////
QSize CPlotter::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize CPlotter::sizeHint() const
{
    return QSize(180, 180);
}


//////////////////////////////////////////////////////////////////////
// Called when mouse moves and does different things depending
//on the state of the mouse buttons for dragging the demod bar or
// filter edges.
//////////////////////////////////////////////////////////////////////
void CPlotter::mouseMoveEvent(QMouseEvent* event)
{

    QPoint pt = event->pos();

    /* mouse enter / mouse leave events */
    if (m_OverlayPixmap.rect().contains(pt))
    {	//is in Overlay bitmap region
        if (event->buttons() == Qt::NoButton)
        {
            bool onTag=false;
            if(pt.y()<15*10) //FIXME
            {
                for(int i=0; i<m_BookmarkTags.size() && !onTag; i++)
                {
                    if(m_BookmarkTags[i].first.contains(event->pos()))
                        onTag=true;
                }
            }
            //if no mouse button monitor grab regions and change cursor icon
            if(onTag)
            {
                setCursor(QCursor(Qt::PointingHandCursor));
                m_CursorCaptured=BOOKMARK;
            }
            else if (isPointCloseTo(pt.x(), m_DemodFreqX, m_CursorCaptureDelta))
            {	//in move demod box center frequency region
                if (CENTER != m_CursorCaptured)
                    setCursor(QCursor(Qt::SizeHorCursor));
                m_CursorCaptured = CENTER;
                if (m_TooltipsEnabled)
                    QToolTip::showText(event->globalPos(),
                                       QString("Demod: %1 kHz")
                                       .arg(m_DemodCenterFreq/1.e3f, 0, 'f', 3),
                                       this, rect());
            }
            else if (isPointCloseTo(pt.x(), m_DemodHiCutFreqX, m_CursorCaptureDelta))
            {	//in move demod hicut region
                if (RIGHT != m_CursorCaptured)
                    setCursor(QCursor(Qt::SizeFDiagCursor));
                m_CursorCaptured = RIGHT;
                if (m_TooltipsEnabled)
                    QToolTip::showText(event->globalPos(),
                                       QString("High cut: %1 Hz")
                                       .arg(m_DemodHiCutFreq),
                                       this, rect());
            }
            else if (isPointCloseTo(pt.x(), m_DemodLowCutFreqX, m_CursorCaptureDelta))
            {	//in move demod lowcut region
                if (LEFT != m_CursorCaptured)
                    setCursor(QCursor(Qt::SizeBDiagCursor));
                m_CursorCaptured = LEFT;
                if (m_TooltipsEnabled)
                    QToolTip::showText(event->globalPos(),
                                       QString("Low cut: %1 Hz")
                                       .arg(m_DemodLowCutFreq),
                                       this, rect());
            }
            else if (isPointCloseTo(pt.x(), m_YAxisWidth/2, m_YAxisWidth/2))
            {
                if (YAXIS != m_CursorCaptured)
                    setCursor(QCursor(Qt::OpenHandCursor));
                m_CursorCaptured = YAXIS;
                if (m_TooltipsEnabled)
                    QToolTip::hideText();
            }
            else if (isPointCloseTo(pt.y(), m_XAxisYCenter, m_CursorCaptureDelta+5))
            {
                if (XAXIS != m_CursorCaptured)
                    setCursor(QCursor(Qt::OpenHandCursor));
                m_CursorCaptured = XAXIS;
                if (m_TooltipsEnabled)
                    QToolTip::hideText();
            }
            else
            {	//if not near any grab boundaries
                if (NOCAP != m_CursorCaptured)
                {
                    setCursor(QCursor(Qt::ArrowCursor));
                    m_CursorCaptured = NOCAP;
                }
                if (m_TooltipsEnabled)
                    QToolTip::showText(event->globalPos(),
                                       QString("F: %1 kHz")
                                       .arg(freqFromX(pt.x())/1.e3f, 0, 'f', 3),
                                       this, rect());
            }
            m_GrabPosition = 0;
        }
    }
    else
    {	//not in Overlay region
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
            tt.setMSecsSinceEpoch(msecFromY(pt.y()));

            QToolTip::showText(event->globalPos(),
                               QString("%1\n%2 kHz")
                               .arg(tt.toString("yyyy.MM.dd hh:mm:ss.zzz"))
                               .arg(freqFromX(pt.x())/1.e3f, 0, 'f', 3),
                               this, rect());
        }
    }
    // process mouse moves while in cursor capture modes
    if (YAXIS == m_CursorCaptured)
    {
        if (event->buttons() & Qt::LeftButton)
        {
            setCursor(QCursor(Qt::ClosedHandCursor));
            // move Y scale up/down
            float delta_px = m_Yzero - pt.y();
            float delta_db = delta_px * fabs(m_MindB-m_MaxdB)/(float)m_OverlayPixmap.height();
            m_MindB -= delta_db;
            m_MaxdB -= delta_db;
            if (out_of_range(m_MaxdB, m_MaxdB - m_MindB))
            {
                m_MindB += delta_db;
                m_MaxdB += delta_db;
            }
            else
            {
                emit fftRangeChanged(m_MaxdB, m_MaxdB - m_MindB);

                if (m_Running)
                    m_DrawOverlay = true;
                else
                    drawOverlay();

                m_PeakHoldValid = false;

                m_Yzero = pt.y();
            }
        }
    }
    else if (XAXIS == m_CursorCaptured)
    {
        if (event->buttons() & (Qt::LeftButton | Qt::MidButton))
        {
            setCursor(QCursor(Qt::ClosedHandCursor));
            // pan viewable range or move center frequency
            int delta_px = m_Xzero - pt.x();
            qint64 delta_hz = delta_px * m_Span / m_OverlayPixmap.width();
            if (event->buttons() & Qt::MidButton)
            {
                m_CenterFreq += delta_hz;
                m_DemodCenterFreq += delta_hz;
                emit newCenterFreq(m_CenterFreq);
            }
            else
            {
                setFftCenterFreq(m_FftCenter + delta_hz);
            }
            if (m_Running)
                m_DrawOverlay = true;
            else
                drawOverlay();

            m_PeakHoldValid = false;

            m_Xzero = pt.x();
        }
    }
    else if (LEFT == m_CursorCaptured)
    {   // moving in demod lowcut region
        if (event->buttons() & (Qt::LeftButton|Qt::RightButton))
        {   //moving in demod lowcut region with left button held
            if (m_GrabPosition != 0)
            {
                m_DemodLowCutFreq = freqFromX(pt.x()-m_GrabPosition ) - m_DemodCenterFreq;
                m_DemodLowCutFreq = roundFreq(m_DemodLowCutFreq, m_FilterClickResolution);

                if (m_symetric && (event->buttons() & Qt::LeftButton))  // symetric adjustment
                {
                    m_DemodHiCutFreq = -m_DemodLowCutFreq;
                }
                clampDemodParameters();

                emit newFilterFreq(m_DemodLowCutFreq, m_DemodHiCutFreq);
                if (m_Running)
                    m_DrawOverlay = true;  // schedule update of overlay during draw()
                else
                    drawOverlay();  // not running so update overlay now
            }
            else
            {	//save initial grab postion from m_DemodFreqX
                m_GrabPosition = pt.x()-m_DemodLowCutFreqX;
            }
        }
        else if (event->buttons() & ~Qt::NoButton)
        {
            setCursor(QCursor(Qt::ArrowCursor));
            m_CursorCaptured = NOCAP;
        }
    }
    else if (RIGHT == m_CursorCaptured)
    {   // moving in demod highcut region
        if (event->buttons() & (Qt::LeftButton|Qt::RightButton))
        {   // moving in demod highcut region with right button held
            if (m_GrabPosition != 0)
            {
                m_DemodHiCutFreq = freqFromX( pt.x()-m_GrabPosition ) - m_DemodCenterFreq;
                m_DemodHiCutFreq = roundFreq(m_DemodHiCutFreq, m_FilterClickResolution);

                if (m_symetric && (event->buttons() & Qt::LeftButton)) // symetric adjustment
                {
                    m_DemodLowCutFreq = -m_DemodHiCutFreq;
                }
                clampDemodParameters();

                emit newFilterFreq(m_DemodLowCutFreq, m_DemodHiCutFreq);
                if (m_Running)
                    m_DrawOverlay = true;  // schedule update of overlay during draw()
                else
                    drawOverlay();  // not running so update oiverlay now
            }
            else
            {	//save initial grab postion from m_DemodFreqX
                m_GrabPosition = pt.x()-m_DemodHiCutFreqX;
            }
        }
        else if (event->buttons() & ~Qt::NoButton)
        {
            setCursor(QCursor(Qt::ArrowCursor));
            m_CursorCaptured = NOCAP;
        }
    }
    else if (CENTER == m_CursorCaptured)
    {   // moving inbetween demod lowcut and highcut region
        if (event->buttons() & Qt::LeftButton)
        {   // moving inbetween demod lowcut and highcut region with left button held
            if (m_GrabPosition != 0)
            {
                m_DemodCenterFreq = roundFreq(freqFromX(pt.x()-m_GrabPosition), m_ClickResolution );
                emit newDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq-m_CenterFreq);

                if (m_Running)
                    m_DrawOverlay = true;  // schedule update of overlay during draw()
                else
                    drawOverlay();  // not running so update oiverlay now

                m_PeakHoldValid = false;
            }
            else
            {	//save initial grab postion from m_DemodFreqX
                m_GrabPosition = pt.x()-m_DemodFreqX;
            }
        }
        else if (event->buttons() & ~Qt::NoButton)
        {
            setCursor(QCursor(Qt::ArrowCursor));
            m_CursorCaptured = NOCAP;
        }
    }
    else	//if cursor not captured
    {
        m_GrabPosition = 0;
    }
    if (!this->rect().contains(pt))
    {
        if(NOCAP != m_CursorCaptured)
            setCursor(QCursor(Qt::ArrowCursor));
        m_CursorCaptured = NOCAP;
    }
}


int CPlotter::getNearestPeak(QPoint pt)
{
    QMap<int, int>::const_iterator i = m_Peaks.lowerBound(pt.x()-PEAK_CLICK_MAX_H_DISTANCE);
    QMap<int, int>::const_iterator upperBound = m_Peaks.upperBound(pt.x()+PEAK_CLICK_MAX_H_DISTANCE);
    float   dist = 1.0e10;
    int     best = -1;
    for( ; i != upperBound; i++)
    {
        int x = i.key();
        int y = i.value();

        if (abs(y - pt.y()) > PEAK_CLICK_MAX_V_DISTANCE)
            continue;

        float d = powf(y - pt.y(), 2) + powf(x - pt.x(), 2);
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
    msec_per_wfline = wf_span / m_WaterfallPixmap.height();
    clearWaterfall();
}

void CPlotter::clearWaterfall()
{
    m_WaterfallPixmap.fill(Qt::black);
    memset(m_wfbuf, 255, MAX_SCREENSIZE);
}

/**
 * @brief Save waterfall to a graphics file
 * @param filename
 * @return TRUE if the save successful, FALSE if an erorr occurred.
 *
 * We assume that frequency strings are up to date
 */
bool CPlotter::saveWaterfall(const QString & filename) const
{
    QBrush      axis_brush(QColor(0x00, 0x00, 0x00, 0x70), Qt::SolidPattern);
    QPixmap     pixmap(m_WaterfallPixmap);
    QPainter    painter(&pixmap);
    QRect       rect;
    QDateTime   tt;
    QFont       font("sans-serif");
    QFontMetrics    font_metrics(font);
    float       pixperdiv;
    int         x, y, w, h;
    int         hxa, wya = 85;
    int         i;

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
    tt.setOffsetFromUtc(0); // ensure time is in UTC
    for (i = 1; i < tdivs; i++)
    {
        y = (int)((float)i * pixperdiv);
        if (msec_per_wfline > 0)
            msec =  tlast_wf_ms - y * msec_per_wfline;
        else
            msec =  tlast_wf_ms - y * 1000 / fft_rate;

        tt.setMSecsSinceEpoch(msec);
        rect.setRect(0, y - font_metrics.height(), wya - 5, font_metrics.height());
        painter.drawText(rect, Qt::AlignRight|Qt::AlignVCenter, tt.toString("yyyy.MM.dd"));
        painter.drawLine(wya - 5, y, wya, y);
        rect.setRect(0, y, wya - 5, font_metrics.height());
        painter.drawText(rect, Qt::AlignRight|Qt::AlignVCenter, tt.toString("hh:mm:ss"));
    }

    return pixmap.save(filename, 0, -1);
}

/** Get waterfall time resolution in milleconds / line. */
quint64 CPlotter::getWfTimeRes(void)
{
    if (msec_per_wfline)
        return msec_per_wfline;
    else
        return 1000 * fft_rate / m_WaterfallPixmap.height(); // Auto mode
}

void CPlotter::setFftRate(int rate_hz)
{
    fft_rate = rate_hz;
    clearWaterfall();
}

//////////////////////////////////////////////////////////////////////
// Called when a mouse button is pressed
//////////////////////////////////////////////////////////////////////
void CPlotter::mousePressEvent(QMouseEvent * event)
{
    QPoint pt = event->pos();

    if (NOCAP == m_CursorCaptured)
    {
        if (isPointCloseTo(pt.x(), m_DemodFreqX, m_CursorCaptureDelta))
        {	//in move demod box center frequency region
            m_CursorCaptured = CENTER;
            m_GrabPosition = pt.x()-m_DemodFreqX;
        }
        else if (isPointCloseTo(pt.x(), m_DemodLowCutFreqX, m_CursorCaptureDelta))
        {   // filter low cut
            m_CursorCaptured = LEFT;
            m_GrabPosition = pt.x()-m_DemodLowCutFreqX;
        }
        else if (isPointCloseTo(pt.x(), m_DemodHiCutFreqX, m_CursorCaptureDelta))
        {   // filter high cut
            m_CursorCaptured = RIGHT;
            m_GrabPosition = pt.x()-m_DemodHiCutFreqX;
        }
        else
        {
            if (event->buttons() == Qt::LeftButton)
            {
                int best=-1;
                if(m_PeakDetection>0)
                    best = getNearestPeak(pt);

                if(best!=-1)
                    m_DemodCenterFreq = freqFromX(best);
                else
                    m_DemodCenterFreq = roundFreq(freqFromX(pt.x()),m_ClickResolution );

                //if cursor not captured set demod frequency and start demod box capture
                emit newDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq-m_CenterFreq);

                //save initial grab postion from m_DemodFreqX
                //setCursor(QCursor(Qt::CrossCursor));
                m_CursorCaptured = CENTER;
                m_GrabPosition = 1;
                //m_GrabPosition = pt.x()-m_DemodFreqX;
                drawOverlay();
            }
            else if (event->buttons() == Qt::MidButton)
            {
                // set center freq
                m_CenterFreq = roundFreq(freqFromX(pt.x()), m_ClickResolution);
                m_DemodCenterFreq = m_CenterFreq;
                emit newCenterFreq(m_CenterFreq);
                emit newDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq-m_CenterFreq);
            }
        }
    }
    else
    {
        if (m_CursorCaptured == YAXIS)
            // get ready for moving Y axis
            m_Yzero = pt.y();
        else if (m_CursorCaptured == XAXIS)
            m_Xzero = pt.x();
        else if(m_CursorCaptured==BOOKMARK)
        {
            for(int i=0; i<m_BookmarkTags.size(); i++)
            {
                if(m_BookmarkTags[i].first.contains(event->pos()))
                {
                    m_DemodCenterFreq = m_BookmarkTags[i].second;
                    emit newDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq-m_CenterFreq);
                    break;
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////
// Called when a mouse button is released
//////////////////////////////////////////////////////////////////////
void CPlotter::mouseReleaseEvent(QMouseEvent * event)
{
    QPoint pt = event->pos();

    if (!m_OverlayPixmap.rect().contains(pt))
    { //not in Overlay region
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
    float new_range = qBound(10.0f,
                             (float)(m_Span) * step,
                             (float)(m_SampleFreq) * 10.0f);

    // Frequency where event occured is kept fixed under mouse
    float ratio = (float)x / (float)m_OverlayPixmap.width();
    float fixed_hz = freqFromX(x);
    float f_max = fixed_hz + (1.0 - ratio) * new_range;
    float f_min = f_max - new_range;

    qint64 fc = (qint64)(f_min + (f_max - f_min) / 2.0);

    setFftCenterFreq(fc - m_CenterFreq);
    setSpanFreq((quint32)new_range);

    float factor = (float)m_SampleFreq / (float)m_Span;
    qDebug() << QString("Spectrum zoom: %1x").arg(factor, 0, 'f', 1);

    m_PeakHoldValid = false;
}

// Zoom on X axis (absolute level)
void CPlotter::zoomOnXAxis(float level)
{
    float current_level = (float)m_SampleFreq / (float)m_Span;

    zoomStepX(current_level / level, xFromFreq(m_DemodCenterFreq));
}

//////////////////////////////////////////////////////////////////////
// Called when a mouse wheel is turned
//////////////////////////////////////////////////////////////////////
void CPlotter::wheelEvent(QWheelEvent * event)
{
    QPoint pt = event->pos();
    int numDegrees = event->delta() / 8;
    int numSteps = numDegrees / 15;  /** FIXME: Only used for direction **/

    /** FIXME: zooming could use some optimisation **/
    if (m_CursorCaptured == YAXIS)
    {
        // Vertical zoom. Wheel down: zoom out, wheel up: zoom in
        // During zoom we try to keep the point (dB or kHz) under the cursor fixed
        float zoom_fac = event->delta() < 0 ? 1.1 : 0.9;
        float ratio = (float)pt.y() / (float)m_OverlayPixmap.height();
        float db_range = (float)(m_MaxdB - m_MindB);
        float y_range = (float)m_OverlayPixmap.height();
        float db_per_pix = db_range / y_range;
        float fixed_db = m_MaxdB - pt.y() * db_per_pix;

        db_range = qBound(FFT_RANGE_MIN, db_range * zoom_fac, FFT_RANGE_MAX);
        m_MaxdB = fixed_db + ratio*db_range;
        if (m_MaxdB > REF_LEVEL_MAX)
            m_MaxdB = REF_LEVEL_MAX;

        m_MindB = m_MaxdB - db_range;
        m_PeakHoldValid = false;

        emit fftRangeChanged(m_MaxdB, db_range);
    }
    else if (m_CursorCaptured == XAXIS)
    {
        zoomStepX(event->delta() < 0 ? 1.1 : 0.9, pt.x());
    }
    else if (event->modifiers() & Qt::ControlModifier)
    {
        // filter width
        m_DemodLowCutFreq -= numSteps*m_ClickResolution;
        m_DemodHiCutFreq += numSteps*m_ClickResolution;
        clampDemodParameters();
        emit newFilterFreq(m_DemodLowCutFreq, m_DemodHiCutFreq);
    }

    else if (event->modifiers() & Qt::ShiftModifier)
    {
        // filter shift
        m_DemodLowCutFreq += numSteps*m_ClickResolution;
        m_DemodHiCutFreq += numSteps*m_ClickResolution;
        clampDemodParameters();
        emit newFilterFreq(m_DemodLowCutFreq, m_DemodHiCutFreq);
    }
    else
    {
        // inc/dec demod frequency
        m_DemodCenterFreq += (numSteps*m_ClickResolution);
        m_DemodCenterFreq = roundFreq(m_DemodCenterFreq, m_ClickResolution );
        emit newDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq-m_CenterFreq);
    }

    if (m_Running)
        m_DrawOverlay = true;
    else
        drawOverlay();
}

//////////////////////////////////////////////////////////////////////
// Called when screen size changes so must recalculate bitmaps
//////////////////////////////////////////////////////////////////////
void CPlotter::resizeEvent(QResizeEvent* )
{
    if (!size().isValid())
        return;

    if (m_Size != size())
    {	//if changed, resize pixmaps to new screensize
        m_Size = size();
        m_OverlayPixmap = QPixmap(m_Size.width(), m_Percent2DScreen * m_Size.height() / 100);
        m_OverlayPixmap.fill(Qt::black);
        m_2DPixmap = QPixmap(m_Size.width(), m_Percent2DScreen * m_Size.height() / 100);
        m_2DPixmap.fill(Qt::black);

        int height = (100 - m_Percent2DScreen) * m_Size.height() / 100;
        if (m_WaterfallPixmap.isNull()) {
            m_WaterfallPixmap = QPixmap(m_Size.width(), height);
            m_WaterfallPixmap.fill(Qt::black);
        } else {
            m_WaterfallPixmap = m_WaterfallPixmap.scaled(m_Size.width(), height,
                                                         Qt::IgnoreAspectRatio,
                                                         Qt::SmoothTransformation);
        }

        m_PeakHoldValid = false;

        if (wf_span > 0)
            msec_per_wfline = wf_span / height;
        memset(m_wfbuf, 255, MAX_SCREENSIZE);
    }

    drawOverlay();
}

//////////////////////////////////////////////////////////////////////
// Called by QT when screen needs to be redrawn
//////////////////////////////////////////////////////////////////////
void CPlotter::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.drawPixmap(0, 0, m_2DPixmap);
    painter.drawPixmap(0, m_Percent2DScreen * m_Size.height() / 100,
                       m_WaterfallPixmap);
}


//////////////////////////////////////////////////////////////////////
// Called to update spectrum data for displaying on the screen
//////////////////////////////////////////////////////////////////////
void CPlotter::draw()
{
    int i, n;
    int w;
    int h;
    int xmin, xmax;

    if (m_DrawOverlay)
    {
        drawOverlay();
        m_DrawOverlay = false;
    }

    QPoint LineBuf[MAX_SCREENSIZE];

    if (!m_Running)
        return;

    // get/draw the waterfall
    w = m_WaterfallPixmap.width();
    h = m_WaterfallPixmap.height();

    // no need to draw if pixmap is invisible
    if ((w != 0) || (h != 0))
    {
        quint64     tnow_ms = time_ms();

        // get scaled FFT data
        n = qMin(w, MAX_SCREENSIZE);
        getScreenIntegerFFTData(255, n, m_MaxdB, m_MindB,
                                m_FftCenter - (qint64)m_Span / 2,
                                m_FftCenter + (qint64)m_Span / 2,
                                m_wfData, m_fftbuf,
                                &xmin, &xmax);

        if (msec_per_wfline > 0)
        {
            // not in "auto" mode, so accumulate waterfall data
            for (i = 0; i < n; i++)
            {
                // average
                //m_wfbuf[i] = (m_wfbuf[i] + m_fftbuf[i]) / 2;

                // peak (0..255 where 255 is min)
                if (m_fftbuf[i] < m_wfbuf[i])
                    m_wfbuf[i] = m_fftbuf[i];
            }
        }

        // is it time to update waterfall?
        if (tnow_ms - tlast_wf_ms >= msec_per_wfline)
        {
            tlast_wf_ms = tnow_ms;

            // move current data down one line(must do before attaching a QPainter object)
            m_WaterfallPixmap.scroll(0, 1, 0, 0, w, h);

            QPainter painter1(&m_WaterfallPixmap);

            // draw new line of fft data at top of waterfall bitmap
            painter1.setPen(QColor(0, 0, 0));
            for (i = 0; i < xmin; i++)
                painter1.drawPoint(i, 0);
            for (i = xmax; i < w; i++)
                painter1.drawPoint(i, 0);

            if (msec_per_wfline > 0)
            {
                // user set time span
                for (i = xmin; i < xmax; i++)
                {
                    painter1.setPen(m_ColorTbl[255 - m_wfbuf[i]]);
                    painter1.drawPoint(i, 0);
                    m_wfbuf[i] = 255;
                }
            }
            else
            {
                for (i = xmin; i < xmax; i++)
                {
                    painter1.setPen(m_ColorTbl[255 - m_fftbuf[i]]);
                    painter1.drawPoint(i, 0);
                }
            }
        }
    }

    // get/draw the 2D spectrum
    w = m_2DPixmap.width();
    h = m_2DPixmap.height();

    if ((w != 0) || (h != 0))
    {
        // first copy into 2Dbitmap the overlay bitmap.
        m_2DPixmap = m_OverlayPixmap.copy(0,0,w,h);

        QPainter painter2(&m_2DPixmap);

// workaround for "fixed" line drawing since Qt 5
// see http://stackoverflow.com/questions/16990326
#if QT_VERSION >= 0x050000
        painter2.translate(0.5, 0.5);
#endif

        // get new scaled fft data
        getScreenIntegerFFTData(h, qMin(w, MAX_SCREENSIZE),
                                m_MaxdB, m_MindB,
                                m_FftCenter - (qint64)m_Span/2,
                                m_FftCenter + (qint64)m_Span/2,
                                m_fftData, m_fftbuf,
                                &xmin, &xmax);

        // draw the pandapter
        painter2.setPen(m_FftColor);
        n = xmax - xmin;
        for (i = 0; i < n; i++)
        {
            LineBuf[i].setX(i + xmin);
            LineBuf[i].setY(m_fftbuf[i + xmin]);
        }

        if (m_FftFill)
        {
            QLinearGradient linGrad(QPointF(xmin, h), QPointF(xmin, 0));
            linGrad.setColorAt(0.0, m_FftCol0);
            linGrad.setColorAt(1.0, m_FftCol1);
            painter2.setBrush(QBrush(QGradient(linGrad)));
            if (n < MAX_SCREENSIZE-2)
            {
                LineBuf[n].setX(xmax-1);
                LineBuf[n].setY(h);
                LineBuf[n+1].setX(xmin);
                LineBuf[n+1].setY(h);
                painter2.drawPolygon(LineBuf, n+2);
            }
            else
            {
                LineBuf[MAX_SCREENSIZE-2].setX(xmax-1);
                LineBuf[MAX_SCREENSIZE-2].setY(h);
                LineBuf[MAX_SCREENSIZE-1].setX(xmin);
                LineBuf[MAX_SCREENSIZE-1].setY(h);
                painter2.drawPolygon(LineBuf, n);
            }
        }
        else
        {
            painter2.drawPolyline(LineBuf, n);
        }

        //Peak detection
        if (m_PeakDetection > 0)
        {
            m_Peaks.clear();

            float   mean = 0;
            float   sum_of_sq = 0;
            for (i = 0; i < n; i++)
            {
                mean += m_fftbuf[i + xmin];
                sum_of_sq += m_fftbuf[i + xmin] * m_fftbuf[i + xmin];
            }
            mean /= n;
            float stdev= sqrt( sum_of_sq/n-mean*mean );

            int lastPeak=-1;
            for (i = 0; i < n; i++)
            {
                //m_PeakDetection times the std over the mean or better than current peak
                float d = (lastPeak==-1) ? (mean - m_PeakDetection * stdev) :
                                           m_fftbuf[lastPeak+xmin];

                if (m_fftbuf[i + xmin] < d)
                    lastPeak=i;

                if (lastPeak != -1 && (i - lastPeak > PEAK_H_TOLERANCE || i == n-1))
                {
                    m_Peaks.insert(lastPeak+xmin, m_fftbuf[lastPeak + xmin]);
                    painter2.drawEllipse(lastPeak+xmin-5, m_fftbuf[lastPeak + xmin]-5, 10, 10);
                    lastPeak=-1;
                }
            }
        }

        //Peak hold
        if (m_PeakHoldActive)
        {
            for (i = 0; i < n; i++)
            {
                if(!m_PeakHoldValid || m_fftbuf[i] < m_fftPeakHoldBuf[i])
                    m_fftPeakHoldBuf[i] = m_fftbuf[i];

                LineBuf[i].setX(i + xmin);
                LineBuf[i].setY(m_fftPeakHoldBuf[i + xmin]);
            }
            painter2.setPen(m_PeakHoldColor);
            painter2.drawPolyline(LineBuf, n);

            m_PeakHoldValid=true;
        }

      painter2.end();

    }

    // trigger a new paintEvent
    update();
}

/*! \brief Set new FFT data.
 *  \param fftData Pointer to the new FFT data (same data for pandapter and waterfall).
 *  \param size The FFT size.
 *
 * When FFT data is set using this method, the same data will be used for both the
 * pandapter and the waterfall.
 */
void CPlotter::setNewFttData(float *fftData, int size)
{

    /** FIXME **/
    if (!m_Running)
        m_Running = true;

    m_wfData = fftData;
    m_fftData = fftData;
    m_fftDataSize = size;

    draw();
}

/*! \brief Set new FFT data.
 *  \param fftData Pointer to the new FFT data used on the pandapter.
 *  \param wfData Pointer to the FFT data used in the waterfall.
 *  \param size The FFT size.
 *
 * This method can be used to set different FFT data set for the pandapter and the
 * waterfall.
 */

void CPlotter::setNewFttData(float *fftData, float *wfData, int size)
{

    /** FIXME **/
    if (!m_Running)
        m_Running = true;

    m_wfData = wfData;
    m_fftData = fftData;
    m_fftDataSize = size;

    draw();
}

void CPlotter::getScreenIntegerFFTData(qint32 plotHeight, qint32 plotWidth,
                                       float maxdB, float mindB,
                                       qint64 startFreq, qint64 stopFreq,
                                       float *inBuf, qint32 *outBuf,
                                       int *xmin, int *xmax)
{
    qint32 i;
    qint32 y;
    qint32 x;
    qint32 ymax = 10000;
    qint32 xprev = -1;
    qint32 minbin, maxbin;
    qint32 m_BinMin, m_BinMax;
    qint32 m_FFTSize = m_fftDataSize;
    float *m_pFFTAveBuf = inBuf;
    float  dBGainFactor = ((float)plotHeight) / fabs(maxdB - mindB);
    qint32* m_pTranslateTbl = new qint32[qMax(m_FFTSize, plotWidth)];

    /** FIXME: qint64 -> qint32 **/
    m_BinMin = (qint32)((float)startFreq * (float)m_FFTSize / m_SampleFreq);
    m_BinMin += (m_FFTSize/2);
    m_BinMax = (qint32)((float)stopFreq * (float)m_FFTSize / m_SampleFreq);
    m_BinMax += (m_FFTSize/2);

    minbin = m_BinMin < 0 ? 0 : m_BinMin;
    if (m_BinMin > m_FFTSize)
        m_BinMin = m_FFTSize - 1;
    if (m_BinMax <= m_BinMin)
        m_BinMax = m_BinMin + 1;
    maxbin = m_BinMax < m_FFTSize ? m_BinMax : m_FFTSize;
    bool largeFft = (m_BinMax-m_BinMin) > plotWidth; // true if more fft point than plot points

    if (largeFft)
    {
        // more FFT points than plot points
        for (i = minbin; i < maxbin; i++)
            m_pTranslateTbl[i] = ((i-m_BinMin)*plotWidth) / (m_BinMax - m_BinMin);
        *xmin = m_pTranslateTbl[minbin];
        *xmax = m_pTranslateTbl[maxbin - 1];
    }
    else
    {
        // more plot points than FFT points
        for (i = 0; i < plotWidth; i++)
            m_pTranslateTbl[i] = m_BinMin + (i*(m_BinMax - m_BinMin)) / plotWidth;
        *xmin = 0;
        *xmax = plotWidth;
    }

    if (largeFft)
    {
        // more FFT points than plot points
        for (i = minbin; i < maxbin; i++ )
        {
            y = (qint32)(dBGainFactor*(maxdB-m_pFFTAveBuf[i]));

            if (y > plotHeight)
                y = plotHeight;
            else if (y < 0)
                y = 0;

            x = m_pTranslateTbl[i];	//get fft bin to plot x coordinate transform

            if (x == xprev)   // still mappped to same fft bin coordinate
            {
                if (y < ymax) // store only the max value
                {
                    outBuf[x] = y;
                    ymax = y;
                }

            }
            else
            {
                outBuf[x] = y;
                xprev = x;
                ymax = y;
            }
        }
    }
    else
    {
        // more plot points than FFT points
        for (x = 0; x < plotWidth; x++ )
        {
            i = m_pTranslateTbl[x]; // get plot to fft bin coordinate transform
            y = (qint32)(dBGainFactor*(maxdB-m_pFFTAveBuf[i]));

            if (y > plotHeight)
                y = plotHeight;
            else if (y < 0)
                y = 0;

            outBuf[x] = y;
        }
    }

    delete [] m_pTranslateTbl;
}

/*! \brief Set limits of dB scale. */
void CPlotter::setMinMaxDB(float min, float max)
{
    m_MaxdB = max;
    m_MindB = min;

    if (m_Running)
        m_DrawOverlay = true;
    else
        drawOverlay();

    m_PeakHoldValid = false;
}

void CPlotter::setFftRange(float reflevel, float range)
{
    if (out_of_range(reflevel, range))
        return;

    setMinMaxDB(reflevel - range, reflevel);
}


//////////////////////////////////////////////////////////////////////
// Called to draw an overlay bitmap containing grid and text that
// does not need to be recreated every fft data update.
//////////////////////////////////////////////////////////////////////
void CPlotter::drawOverlay()
{
    if (m_OverlayPixmap.isNull())
        return;

    int w = m_OverlayPixmap.width();
    int h = m_OverlayPixmap.height();
    int x,y;
    float pixperdiv;
    QRect rect;
    QPainter painter(&m_OverlayPixmap);
    painter.initFrom(this);

    // horizontal grids (size and grid calcs could be moved to resize)
    m_VerDivs = h/m_VdivDelta+1;
    m_HorDivs = qMin(w/m_HdivDelta, HORZ_DIVS_MAX);
    if (m_HorDivs % 2)
        m_HorDivs++;   // we want an odd number of divs so that we have a center line

    //m_OverlayPixmap.fill(Qt::black);
    // fill background with gradient
    QLinearGradient gradient(0, 0, 0 ,h);
    gradient.setColorAt(0, QColor(0x2F,0x2F,0x2F,0xFF));
    gradient.setColorAt(1, QColor(0x00,0x00,0x00,0xFF));
    painter.setBrush(gradient);
    painter.drawRect(0, 0, w, h);

    // Draw demod filter box
    if (m_FilterBoxEnabled)
    {
        // Clamping no longer necessary as we do it in mouseMove()
        //ClampDemodParameters();

        m_DemodFreqX = xFromFreq(m_DemodCenterFreq);
        m_DemodLowCutFreqX = xFromFreq(m_DemodCenterFreq + m_DemodLowCutFreq);
        m_DemodHiCutFreqX = xFromFreq(m_DemodCenterFreq + m_DemodHiCutFreq);

        int dw = m_DemodHiCutFreqX - m_DemodLowCutFreqX;

        painter.setBrush(Qt::SolidPattern);
        painter.setOpacity(0.3);
        painter.fillRect(m_DemodLowCutFreqX, 0, dw, h, Qt::gray);

        painter.setOpacity(1.0);
        painter.setPen(QPen(QColor(0xFF,0x71,0x71,0xFF), 1, Qt::SolidLine));
        painter.drawLine(m_DemodFreqX, 0, m_DemodFreqX, h);
    }

    QFontMetrics metrics(m_Font);
    painter.setFont(m_Font);

    // draw vertical grids
    pixperdiv = (float)w / (float)m_HorDivs;
    y = h - h/m_VerDivs/2;
    painter.setPen(QPen(QColor(0xF0,0xF0,0xF0,0x30), 1, Qt::DotLine));
    for (int i = 1; i < m_HorDivs; i++)
    {
        x = (int)((float)i*pixperdiv);
        painter.drawLine(x, 0, x, y);
    }

    //Draw Bookmark Tags
    m_BookmarkTags.clear();
    static const QFontMetrics fm(painter.font());
    static const int fontHeight = fm.ascent()+1; // height();
    static const int slant = 5;
    static const int levelHeight = fontHeight+5;
    static const int nLevels = 10;
    QList<BookmarkInfo> bookmarks = Bookmarks::Get().getBookmarksInRange(m_CenterFreq+m_FftCenter-m_Span/2, m_CenterFreq+m_FftCenter+m_Span/2);
    int tagEnd[nLevels] = {0};
    for(int i=0; i<bookmarks.size(); i++)
    {
        x=xFromFreq(bookmarks[i].frequency);
#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64)
        int nameWidth= fm.width(bookmarks[i].name);
#else
        int nameWidth= fm.boundingRect(bookmarks[i].name).width();
#endif

        int level = 0;
        for(; level<nLevels && tagEnd[level]>x; level++);
        level%=nLevels;

        tagEnd[level]=x+nameWidth+slant-1;
        m_BookmarkTags.append(qMakePair<QRect, qint64>(QRect(x, level*levelHeight, nameWidth+slant, fontHeight), bookmarks[i].frequency));

        QColor color = QColor(bookmarks[i].GetColor());
        color.setAlpha(0x60);

        painter.setPen(QPen(color, 1, Qt::DashLine));
        painter.drawLine(x, level*levelHeight+fontHeight+slant, x, y); //Vertical line

        painter.setPen(QPen(color, 1, Qt::SolidLine));
        painter.drawLine(x+slant, level*levelHeight+fontHeight, x+nameWidth+slant-1, level*levelHeight+fontHeight); //Horizontal line
        painter.drawLine(x+1,level*levelHeight+fontHeight+slant-1, x+slant-1, level*levelHeight+fontHeight+1); //Diagonal line
/*
        painter.setPen(QPen(QColor(0xF0,0xF0,0xF0,0xB0), 1, Qt::SolidLine));
        QPolygon polygon(6);
        polygon.setPoint(0, 0, 10);
        polygon.setPoint(1, 5, 15);
        polygon.setPoint(2, 5+nameWidth, 15);
        polygon.setPoint(3, 5+nameWidth, 0);
        polygon.setPoint(4, 5, 0);
        polygon.setPoint(5, 0, 5);
        polygon.translate(x, level*18);
        painter.drawPolygon(polygon);
*/

        color.setAlpha(0xFF);
        painter.setPen(QPen(color, 2, Qt::SolidLine));
        painter.drawText(x+slant,level*levelHeight, nameWidth, fontHeight, Qt::AlignVCenter | Qt::AlignHCenter, bookmarks[i].name);
    }

    if (m_CenterLineEnabled)
    {
        // center line
        x = xFromFreq(m_CenterFreq);
        if (x > 0 && x < w)
        {
            painter.setPen(QPen(QColor(0x78,0x82,0x96,0xFF), 1, Qt::SolidLine));
            painter.drawLine(x, 0, x, y);
        }
    }

    // draw frequency values
    makeFrequencyStrs();
    painter.setPen(QColor(0xD8,0xBA,0xA1,0xFF));
    y = h - (h/m_VerDivs);
    m_XAxisYCenter = h - metrics.height()/2;
    for (int i = 1; i < m_HorDivs; i++)
    {
        x = (int)((float)i*pixperdiv - pixperdiv/2);
        rect.setRect(x, y, (int)pixperdiv, h/m_VerDivs);
        painter.drawText(rect, Qt::AlignHCenter|Qt::AlignBottom, m_HDivText[i]);
    }

    m_dBStepSize = fabs(m_MaxdB - m_MindB)/(float)m_VerDivs;
    pixperdiv = (float)h / (float)m_VerDivs;
    painter.setPen(QPen(QColor(0xF0,0xF0,0xF0,0x30), 1,Qt::DotLine));
    for (int i = 1; i < m_VerDivs; i++)
    {
        y = (int)((float) i*pixperdiv);
        painter.drawLine(5*metrics.width("0",-1), y, w, y);
    }

    // draw amplitude values
    painter.setPen(QColor(0xD8,0xBA,0xA1,0xFF));

    int dB = m_MaxdB;
    m_YAxisWidth = metrics.width("-120 ");
    for (int i = 1; i < m_VerDivs; i++)
    {
        dB -= m_dBStepSize;  // move to end if want to include maxdb
        y = (int)((float)i*pixperdiv);
        rect.setRect(0, y-metrics.height()/2, m_YAxisWidth, metrics.height());
        painter.drawText(rect, Qt::AlignRight|Qt::AlignVCenter, QString::number(dB));
    }

    if (!m_Running)
    {
        // if not running so is no data updates to draw to screen
        // copy into 2Dbitmap the overlay bitmap.
        m_2DPixmap = m_OverlayPixmap.copy(0,0,w,h);

        // trigger a new paintEvent
        update();
    }

    painter.end();
}

//////////////////////////////////////////////////////////////////////
// Helper function Called to create all the frequency division text
//strings based on start frequency, span frequency, frequency units.
//Places in QString array m_HDivText
//Keeps all strings the same fractional length
//////////////////////////////////////////////////////////////////////
void CPlotter::makeFrequencyStrs()
{
    qint64 FreqPerDiv = m_Span/m_HorDivs;
    qint64 StartFreq = m_CenterFreq + m_FftCenter - m_Span/2;
    float freq;
    int i,j;

    if ((1 == m_FreqUnits) || (m_FreqDigits == 0))
    {	//if units is Hz then just output integer freq
        for (int i = 0; i <= m_HorDivs; i++)
        {
            freq = (float)StartFreq/(float)m_FreqUnits;
            m_HDivText[i].setNum((int)freq);
            StartFreq += FreqPerDiv;
        }
        return;
    }
    // here if is fractional frequency values
    // so create max sized text based on frequency units
    for (int i = 0; i <= m_HorDivs; i++)
    {
        freq = (float)StartFreq/(float)m_FreqUnits;
        m_HDivText[i].setNum(freq,'f', m_FreqDigits);
        StartFreq += FreqPerDiv;
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
        if ((j-dp) > max)
            max = j-dp;
    }
    // truncate all strings to maximum fractional length
    StartFreq = m_CenterFreq + m_FftCenter - m_Span/2;
    for (i = 0; i <= m_HorDivs; i++)
    {
        freq = (float)StartFreq/(float)m_FreqUnits;
        m_HDivText[i].setNum(freq,'f', max);
        StartFreq += FreqPerDiv;
    }
}

//////////////////////////////////////////////////////////////////////
// Helper functions to convert to/from screen coordinates to frequency
//////////////////////////////////////////////////////////////////////
int CPlotter::xFromFreq(qint64 freq)
{
    int w = m_OverlayPixmap.width();
    qint64 StartFreq = m_CenterFreq + m_FftCenter - m_Span/2;
    int x = (int) w * ((float)freq - StartFreq)/(float)m_Span;
    if (x < 0)
        return 0;
    if (x > (int)w)
        return m_OverlayPixmap.width();
    return x;
}

qint64 CPlotter::freqFromX(int x)
{
    int w = m_OverlayPixmap.width();
    qint64 StartFreq = m_CenterFreq + m_FftCenter - m_Span/2;
    qint64 f = (qint64)(StartFreq + (float)m_Span * (float)x/(float)w );
    return f;
}

/** Calculate time offset of a given line on the waterfall */
quint64 CPlotter::msecFromY(int y)
{
    // ensure we are in the waterfall region
    if (y < m_OverlayPixmap.height())
        return 0;

    int dy = y - m_OverlayPixmap.height();

    if (msec_per_wfline > 0)
        return tlast_wf_ms - dy * msec_per_wfline;
    else
        return tlast_wf_ms - dy * 1000 / fft_rate;
}

//////////////////////////////////////////////////////////////////////
// Helper function to round frequency to click resolution value
//////////////////////////////////////////////////////////////////////
qint64 CPlotter::roundFreq(qint64 freq, int resolution)
{
    qint64 delta = resolution;
    qint64 delta_2 = delta/2;
    if (freq >= 0)
        return ( freq - (freq+delta_2)%delta + delta_2);
    else
        return ( freq - (freq+delta_2)%delta - delta_2);
}

//////////////////////////////////////////////////////////////////////
// Helper function clamps demod freqeuency limits of
// m_DemodCenterFreq
//////////////////////////////////////////////////////////////////////
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

void CPlotter::setDemodRanges(int FLowCmin, int FLowCmax, int FHiCmin, int FHiCmax, bool symetric)
{
    m_FLowCmin=FLowCmin;
    m_FLowCmax=FLowCmax;
    m_FHiCmin=FHiCmin;
    m_FHiCmax=FHiCmax;
    m_symetric=symetric;
    clampDemodParameters();

    if (m_Running)
        m_DrawOverlay = true;
    else
        drawOverlay();
}

void CPlotter::setCenterFreq(quint64 f)
{
    if((quint64)m_CenterFreq==f)
        return;

    qint64 offset = m_CenterFreq - m_DemodCenterFreq;

    m_CenterFreq = f;
    m_DemodCenterFreq = m_CenterFreq - offset;

    updateOverlay();

    m_PeakHoldValid = false;
}

void CPlotter::updateOverlay()
{
    if (m_Running)
        m_DrawOverlay = true;
    else
        drawOverlay();
}

/*! \brief Reset horizontal zoom to 100% and centered around 0. */
void CPlotter::resetHorizontalZoom(void)
{
    setFftCenterFreq(0);
    setSpanFreq((qint32)m_SampleFreq);
}

/*! \brief Center FFT plot around 0 (corresponds to center freq). */
void CPlotter::moveToCenterFreq(void)
{
    setFftCenterFreq(0);
    if (m_Running)
        m_DrawOverlay = true;
    else
        drawOverlay();

    m_PeakHoldValid = false;
}

/*! \brief Center FFT plot around the demodulator frequency. */
void CPlotter::moveToDemodFreq(void)
{
    setFftCenterFreq(m_DemodCenterFreq-m_CenterFreq);
    if (m_Running)
        m_DrawOverlay = true;
    else
        drawOverlay();

    m_PeakHoldValid = false;
}

/*! Set FFT plot color. */
void CPlotter::setFftPlotColor(const QColor color)
{
    m_FftColor = color;
    m_FftCol0 = color;
    m_FftCol0.setAlpha(0x00);
    m_FftCol1 = color;
    m_FftCol1.setAlpha(0xA0);
    m_PeakHoldColor = color;
    m_PeakHoldColor.setAlpha(60);
}

/*! Enable/disable filling the area below the FFT plot. */
void CPlotter::setFftFill(bool enabled)
{
    m_FftFill = enabled;
}

/*! \brief Set peak hold on or off.
 *  \param enabled The new state of peak hold.
 */
void CPlotter::setPeakHold(bool enabled)
{
    m_PeakHoldActive = enabled;
    m_PeakHoldValid = false;
}

/*! \brief Set peak detection on or off.
 *  \param enabled The new state of peak detection.
 *  \param c Minimum distance of peaks from mean, in multiples of standard deviation.
 */
void CPlotter::setPeakDetection(bool enabled, float c)
{
    if(!enabled || c <= 0)
        m_PeakDetection = -1;
    else
        m_PeakDetection = c;
}
