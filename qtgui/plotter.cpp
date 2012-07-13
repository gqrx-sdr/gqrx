/* -*- c++ -*- */
/* + + +   This Software is released under the "Simplified BSD License"  + + +
 * Copyright 2010 Moe Wheatley. All rights reserved.
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
#include "plotter.h"
#include <stdlib.h>
#include <cmath>
#include <QDebug>


//////////////////////////////////////////////////////////////////////
// Local defines
//////////////////////////////////////////////////////////////////////
#define CUR_CUT_DELTA 5		//cursor capture delta in pixels


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
    setMouseTracking ( true );

    //create a default waterfall color scheme
    // *** Need to read from file ***
    for (int i = 0; i < 256; i++)
    {
        if( (i<43) )
            m_ColorTbl[i].setRgb( 0,0, 255*(i)/43);
        if( (i>=43) && (i<87) )
            m_ColorTbl[i].setRgb( 0, 255*(i-43)/43, 255 );
        if( (i>=87) && (i<120) )
            m_ColorTbl[i].setRgb( 0,255, 255-(255*(i-87)/32));
        if( (i>=120) && (i<154) )
            m_ColorTbl[i].setRgb( (255*(i-120)/33), 255, 0);
        if( (i>=154) && (i<217) )
            m_ColorTbl[i].setRgb( 255, 255 - (255*(i-154)/62), 0);
        if( (i>=217)  )
            m_ColorTbl[i].setRgb( 255, 0, 128*(i-217)/38);
    }

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
    m_MindB = -120;
    m_dBStepSize = abs(m_MaxdB-m_MindB)/m_VerDivs;

    m_FreqUnits = 1000000;
    m_CursorCaptured = NONE;
    m_Running = false;
    m_DrawOverlay = false;
    m_2DPixmap = QPixmap(0,0);
    m_OverlayPixmap = QPixmap(0,0);
    m_WaterfallPixmap = QPixmap(0,0);
    m_Size = QSize(0,0);
    m_GrabPosition = 0;
    m_Percent2DScreen = 50;	//percent of screen used for 2D display

    m_FontSize = 9;
    m_VdivDelta = 40;
    m_HdivDelta = 60;

    m_FreqDigits = 3;
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
        {	//if no mouse button monitor grab regions and change cursor icon
            if (IsPointCloseTo(pt.x(), (m_DemodHiCutFreqX+m_DemodLowCutFreqX)/2, m_CursorCaptureDelta))
            {	//in move demod box center frequency region
                if (CENTER != m_CursorCaptured)
                    setCursor(QCursor(Qt::SizeHorCursor));
                m_CursorCaptured = CENTER;
            }
            else if (IsPointCloseTo(pt.x(), m_DemodHiCutFreqX, m_CursorCaptureDelta))
            {	//in move demod hicut region
                if (RIGHT != m_CursorCaptured)
                    setCursor(QCursor(Qt::SizeHorCursor));
                m_CursorCaptured = RIGHT;
            }
            else if (IsPointCloseTo(pt.x(), m_DemodLowCutFreqX, m_CursorCaptureDelta))
            {	//in move demod lowcut region
                if (LEFT != m_CursorCaptured)
                    setCursor(QCursor(Qt::SizeHorCursor));
                m_CursorCaptured = LEFT;
            }
            else if (IsPointCloseTo(pt.x(), m_YAxisWidth/2, m_YAxisWidth/2))
            {
                if (YAXIS != m_CursorCaptured)
                    setCursor(QCursor(Qt::OpenHandCursor));
                m_CursorCaptured = YAXIS;
            }
            else
            {	//if not near any grab boundaries
                if (NONE != m_CursorCaptured)
                {
                    setCursor(QCursor(Qt::ArrowCursor));
                    m_CursorCaptured = NONE;
                }
            }
            m_GrabPosition = 0;
        }
    }
    else
    {	//not in Overlay region
        if (event->buttons() == Qt::NoButton)
        {
            if (NONE != m_CursorCaptured)
                setCursor(QCursor(Qt::ArrowCursor));

            m_CursorCaptured = NONE;
            m_GrabPosition = 0;
        }
    }

    // process mouse moves while in cursor capture modes
    if (YAXIS == m_CursorCaptured)
    {
        if (event->buttons() & Qt::LeftButton)
        {
            setCursor(QCursor(Qt::ClosedHandCursor));
            // move Y scale up/down
            int delta_px = m_Yzero - pt.y();
            int delta_db = delta_px * abs(m_MindB-m_MaxdB)/m_OverlayPixmap.height();
            m_MindB -= delta_db;
            m_MaxdB -= delta_db;

            if (m_Running)
                m_DrawOverlay = true;
            else
                DrawOverlay();

            m_Yzero = pt.y();
        }
    }
    else if (LEFT == m_CursorCaptured)
    {   // moving in demod lowcut region
        if (event->buttons() & (Qt::LeftButton|Qt::RightButton))
        {   //moving in demod lowcut region with left button held
            if (m_GrabPosition != 0)
            {
                m_DemodLowCutFreq = FreqfromX(pt.x()-m_GrabPosition ) - m_DemodCenterFreq;
                m_DemodLowCutFreq = RoundFreq(m_DemodLowCutFreq, m_FilterClickResolution);

                if (m_symetric && (event->buttons() & Qt::LeftButton))  // symetric adjustment
                {
                    m_DemodHiCutFreq = -m_DemodLowCutFreq;
                }
                ClampDemodParameters();

                emit NewFilterFreq(m_DemodLowCutFreq, m_DemodHiCutFreq);
                if (m_Running)
                    m_DrawOverlay = true;  // schedule update of overlay during draw()
                else
                    DrawOverlay();  // not running so update oiverlay now
            }
            else
            {	//save initial grab postion from m_DemodFreqX
                m_GrabPosition = pt.x()-m_DemodLowCutFreqX;
            }
        }
        else if (event->buttons() & ~Qt::NoButton)
        {
            setCursor(QCursor(Qt::ArrowCursor));
            m_CursorCaptured = NONE;
        }
    }
    else if (RIGHT == m_CursorCaptured)
    {   // moving in demod highcut region
        if (event->buttons() & (Qt::LeftButton|Qt::RightButton))
        {   // moving in demod highcut region with right button held
            if (m_GrabPosition != 0)
            {
                m_DemodHiCutFreq = FreqfromX( pt.x()-m_GrabPosition ) - m_DemodCenterFreq;
                m_DemodHiCutFreq = RoundFreq(m_DemodHiCutFreq, m_FilterClickResolution);

                if (m_symetric && (event->buttons() & Qt::LeftButton)) // symetric adjustment
                {
                    m_DemodLowCutFreq = -m_DemodHiCutFreq;
                }
                ClampDemodParameters();

                emit NewFilterFreq(m_DemodLowCutFreq, m_DemodHiCutFreq);
                if (m_Running)
                    m_DrawOverlay = true;  // schedule update of overlay during draw()
                else
                    DrawOverlay();  // not running so update oiverlay now
            }
            else
            {	//save initial grab postion from m_DemodFreqX
                m_GrabPosition = pt.x()-m_DemodHiCutFreqX;
            }
        }
        else if (event->buttons() & ~Qt::NoButton)
        {
            setCursor(QCursor(Qt::ArrowCursor));
            m_CursorCaptured = NONE;
        }
    }
    else if (CENTER == m_CursorCaptured)
    {   // moving inbetween demod lowcut and highcut region
        if (event->buttons() & Qt::LeftButton)
        {   // moving inbetween demod lowcut and highcut region with left button held
            if (m_GrabPosition != 0)
            {
                m_DemodCenterFreq = RoundFreq(FreqfromX(pt.x()-m_GrabPosition), m_ClickResolution );
                emit NewDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq-m_CenterFreq);

                if (m_Running)
                    m_DrawOverlay = true;  // schedule update of overlay during draw()
                else
                    DrawOverlay();  // not running so update oiverlay now
            }
            else
            {	//save initial grab postion from m_DemodFreqX
                m_GrabPosition = pt.x()-m_DemodFreqX;
            }
        }
        else if (event->buttons() & ~Qt::NoButton)
        {
            setCursor(QCursor(Qt::ArrowCursor));
            m_CursorCaptured = NONE;
        }
    }
    else	//if cursor not captured
    {
        m_GrabPosition = 0;
    }
    if (!this->rect().contains(pt))
    {
        if(NONE != m_CursorCaptured)
            setCursor(QCursor(Qt::ArrowCursor));
        m_CursorCaptured = NONE;
    }
}

//////////////////////////////////////////////////////////////////////
// Called when a mouse button is pressed
//////////////////////////////////////////////////////////////////////
void CPlotter::mousePressEvent(QMouseEvent * event)
{
    QPoint pt = event->pos();

    if (event->buttons()==Qt::LeftButton)
    {

        if (IsPointCloseTo(pt.x(), m_DemodFreqX, m_CursorCaptureDelta))
        {	//in move demod box center frequency region
            if (CENTER != m_CursorCaptured)
                setCursor(QCursor(Qt::CrossCursor));
            m_CursorCaptured = CENTER;
            m_GrabPosition = pt.x()-m_DemodFreqX;
        }
        else if (IsPointCloseTo(pt.x(), m_DemodLowCutFreqX, m_CursorCaptureDelta))
        {   // filter low cut
            m_CursorCaptured = LEFT;
            m_GrabPosition = pt.x()-m_DemodLowCutFreqX;
        }
        else if (IsPointCloseTo(pt.x(), m_DemodHiCutFreqX, m_CursorCaptureDelta))
        {   // filter high cut
            m_CursorCaptured = RIGHT;
            m_GrabPosition = pt.x()-m_DemodHiCutFreqX;
        }
        else
        {
            if (m_CursorCaptured != YAXIS)
            {
                //if cursor not captured set demod frequency and start demod box capture
                m_DemodCenterFreq = RoundFreq(FreqfromX(pt.x()),m_ClickResolution );
                emit NewDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq-m_CenterFreq);

                //save initial grab postion from m_DemodFreqX
                //setCursor(QCursor(Qt::CrossCursor));
                m_CursorCaptured = CENTER;
                m_GrabPosition = 1;
                //m_GrabPosition = pt.x()-m_DemodFreqX;
                DrawOverlay();
            }
            else
            {
                // get ready for moving Y axis
                m_Yzero = pt.y();
            }
        }
    }

#if 0
    else if (event->buttons() == Qt::MiddleButton)
    {
        qDebug() << "MiddleButton";

        if (NONE == m_CursorCaptured)
        {	//if cursor not captured set center freq
            m_CenterFreq = RoundFreq(FreqfromX(pt.x()),m_ClickResolution );
            m_DemodCenterFreq = m_CenterFreq;
            emit NewCenterFreq(m_CenterFreq);
        }
    }
#endif
}

//////////////////////////////////////////////////////////////////////
// Called when a mouse button is released
//////////////////////////////////////////////////////////////////////
void CPlotter::mouseReleaseEvent(QMouseEvent * event)
{
    QPoint pt = event->pos();

    if (!m_OverlayPixmap.rect().contains(pt))
    { //not in Overlay region
        if (NONE != m_CursorCaptured)
            setCursor(QCursor(Qt::ArrowCursor));

        m_CursorCaptured = NONE;
        m_GrabPosition = 0;
    }
    else
    {
        if (YAXIS == m_CursorCaptured)
        {
            setCursor(QCursor(Qt::OpenHandCursor));
            m_Yzero = -1;
        }
    }
}

//////////////////////////////////////////////////////////////////////
// Called when a mouse wheel is turned
//////////////////////////////////////////////////////////////////////
void CPlotter::wheelEvent(QWheelEvent * event)
{
    int numDegrees = event->delta() / 8;
    int numSteps = numDegrees / 15;

    // wheel down: zoom out
    // wheel up: zoom in
    if (m_CursorCaptured == YAXIS)
    {
        m_MindB += 5*numSteps;
        m_MaxdB -= 5*numSteps;
    }
    else if (event->modifiers() & Qt::ControlModifier)
    {
        // filter width
        m_DemodLowCutFreq -= numSteps*m_ClickResolution;
        m_DemodHiCutFreq += numSteps*m_ClickResolution;
        ClampDemodParameters();
        emit NewFilterFreq(m_DemodLowCutFreq, m_DemodHiCutFreq);
    }

    else if (event->modifiers() & Qt::ShiftModifier)
    {
        // filter shift
        m_DemodLowCutFreq += numSteps*m_ClickResolution;
        m_DemodHiCutFreq += numSteps*m_ClickResolution;
        ClampDemodParameters();
        emit NewFilterFreq(m_DemodLowCutFreq, m_DemodHiCutFreq);
    }
    else
    {
        // inc/dec demod frequency
        m_DemodCenterFreq += (numSteps*m_ClickResolution);
        m_DemodCenterFreq = RoundFreq(m_DemodCenterFreq, m_ClickResolution );
        emit NewDemodFreq(m_DemodCenterFreq, m_DemodCenterFreq-m_CenterFreq);
    }

    if (m_Running)
        m_DrawOverlay = true;
    else
        DrawOverlay();
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
        m_OverlayPixmap = QPixmap(m_Size.width(), m_Percent2DScreen*m_Size.height()/100);
        m_OverlayPixmap.fill(Qt::black);
        m_2DPixmap = QPixmap(m_Size.width(), m_Percent2DScreen*m_Size.height()/100);
        m_2DPixmap.fill(Qt::black);
        m_WaterfallPixmap = QPixmap(m_Size.width(), (100-m_Percent2DScreen)*m_Size.height()/100);
    }
    m_WaterfallPixmap.fill(Qt::black);
    DrawOverlay();
}

//////////////////////////////////////////////////////////////////////
// Called by QT when screen needs to be redrawn
//////////////////////////////////////////////////////////////////////
void CPlotter::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.drawPixmap(0,0,m_2DPixmap);
    painter.drawPixmap(0, m_Percent2DScreen*m_Size.height()/100,m_WaterfallPixmap);
    //tell interface that its ok to signal a new line of fft data
    //m_pSdrInterface->ScreenUpdateDone();
    return;
}


//////////////////////////////////////////////////////////////////////
// Called to update spectrum data for displaying on the screen
//////////////////////////////////////////////////////////////////////
void CPlotter::draw()
{
    int i;
    int w;
    int h;

    if (m_DrawOverlay)
    {
        DrawOverlay();
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
        // move current data down one line(must do before attaching a QPainter object)
        m_WaterfallPixmap.scroll(0,1,0,0, w, h);

        QPainter painter1(&m_WaterfallPixmap);
        // get scaled FFT data
        GetScreenIntegerFFTData(255, w, m_MaxdB, m_MindB,
                                m_FftCenter-m_Span/2, m_FftCenter+m_Span/2,
                                m_fftbuf);

        // draw new line of fft data at top of waterfall bitmap
        for (i = 0; i < w; i++)
        {
            painter1.setPen(m_ColorTbl[ 255-m_fftbuf[i] ]);
            painter1.drawPoint(i,0);
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

        // get new scaled fft data
        GetScreenIntegerFFTData(h, w, m_MaxdB, m_MindB,
                                m_FftCenter-m_Span/2, m_FftCenter+m_Span/2,
                                m_fftbuf);

        // draw the 2D spectrum
        painter2.setPen(QColor(0x97,0xD0,0x97,0xFF));
        for (i = 0; i < w; i++)
        {
            LineBuf[i].setX(i);
            LineBuf[i].setY(m_fftbuf[i]);
        }
        painter2.drawPolyline(LineBuf,w);
    }

    // trigger a new paintEvent
    update();

}


/*! \brief Set new FFT data. */
void CPlotter::SetNewFttData(double *fftData, int size)
{

    /** FIXME **/
    if (!m_Running)
        m_Running = true;

    m_fftData = fftData;
    m_fftDataSize = size;

    draw();
}


void CPlotter::GetScreenIntegerFFTData(qint32 MaxHeight, qint32 MaxWidth,
                                       double MaxdB, double MindB,
                                       qint32 StartFreq, qint32 StopFreq,
                                       qint32* OutBuf)
{
    qint32 i;
    qint32 y;
    qint32 x;
    qint32 ymax = 10000;
    qint32 xprev = -1;
    qint32 maxbin;
    //double dBmaxOffset = 0.0;//MaxdB/10.0;   FIXME
    //double dBGainFactor = 1.0/MindB;//-1.0/(MaxdB-MindB);  FIXME
    double dBGainFactor = ((double)MaxHeight)/abs(MaxdB-MindB);

    qint32 m_PlotWidth = MaxWidth;
    qint32 m_BinMin, m_BinMax;
    qint32 m_FFTSize = m_fftDataSize;
    double* m_pFFTAveBuf = m_fftData;
    qint32* m_pTranslateTbl = new qint32[m_FFTSize];


    maxbin = m_FFTSize - 1;
    m_BinMin = (qint32)((double)StartFreq*(double)m_FFTSize/m_SampleFreq);
    m_BinMin += (m_FFTSize/2);
    m_BinMax = (qint32)((double)StopFreq*(double)m_FFTSize/m_SampleFreq);
    m_BinMax += (m_FFTSize/2);

    if (m_BinMin < 0)	//don't allow these go outside the translate table
        m_BinMin = 0;
    if (m_BinMin >= maxbin)
        m_BinMin = maxbin;
    if (m_BinMax < 0)
        m_BinMax = 0;
    if (m_BinMax >= maxbin)
        m_BinMax = maxbin;
    if ((m_BinMax-m_BinMin) > m_PlotWidth)
    {
        //if more FFT points than plot points
        for (i = m_BinMin; i <= m_BinMax; i++)
            m_pTranslateTbl[i] = ((i-m_BinMin)*m_PlotWidth) / (m_BinMax - m_BinMin);
    }
    else
    {
        //if more plot points than FFT points
        for (i = 0; i < m_PlotWidth; i++)
            m_pTranslateTbl[i] = m_BinMin + (i*(m_BinMax - m_BinMin)) / m_PlotWidth;
    }

    if ((m_BinMax-m_BinMin) > m_PlotWidth)
    {
        //if more FFT points than plot points
        for (i = m_BinMin; i <= m_BinMax; i++ )
        {
            y = (qint32)(dBGainFactor*(MaxdB-m_pFFTAveBuf[i]));

            if (y > MaxHeight)
                y = MaxHeight;
            else if (y < 0)
                y = 0;

            x = m_pTranslateTbl[i];	//get fft bin to plot x coordinate transform

            if (x == xprev)	// still mappped to same fft bin coordinate
            {
                if (y < ymax)		//store only the max value
                {
                    OutBuf[x] = y;
                    ymax = y;
                }

            }
            else
            {
                OutBuf[x] = y;
                xprev = x;
                ymax = y;
            }
        }
    }
    else
    {
        //if more plot points than FFT points
        for (x = 0; x < m_PlotWidth; x++ )
        {
            i = m_pTranslateTbl[x];	//get plot to fft bin coordinate transform
            y = (qint32)(dBGainFactor*(MaxdB-m_pFFTAveBuf[i]));

            if (y > MaxHeight)
                y = MaxHeight;
            else if (y < 0)
                y = 0;

            OutBuf[x] = y;
        }
    }

    delete [] m_pTranslateTbl;
}


/*! \brief Set upper limit of dB scale. */
void CPlotter::setMaxDB(qint32 max)
{
    m_MaxdB = max;

    if (m_Running)
        m_DrawOverlay = true;
    else
        DrawOverlay();
}

/*! \brief Set lower limit of dB scale. */
void CPlotter::setMinDB(qint32 min)
{
    m_MindB = min;

    if (m_Running)
        m_DrawOverlay = true;
    else
        DrawOverlay();
}

/*! \brief Set limits of dB scale. */
void CPlotter::setMinMaxDB(qint32 min, qint32 max)
{
    m_MaxdB = max;
    m_MindB = min;

    if (m_Running)
        m_DrawOverlay = true;
    else
        DrawOverlay();
}


//////////////////////////////////////////////////////////////////////
// Called to draw an overlay bitmap containing grid and text that
// does not need to be recreated every fft data update.
//////////////////////////////////////////////////////////////////////
void CPlotter::DrawOverlay()
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
    m_HorDivs = w/m_HdivDelta;
    if (m_HorDivs % 2)
        m_HorDivs++;   // we want an odd number of divs so that we have a center line

    //m_OverlayPixmap.fill(Qt::black);
    // fill background with gradient
    QLinearGradient gradient(0, 0, 0 ,h);
    gradient.setColorAt(0, QColor(0x20,0x20,0x20,0xFF));
    gradient.setColorAt(1, QColor(0x4F,0x4F,0x4F,0xFF));
    painter.setBrush(gradient);
    painter.drawRect(0, 0, w, h);

    // Draw demod filter box
    if (m_FilterBoxEnabled)
    {
        // Clamping no longer necessary as we do it in mouseMove()
        //ClampDemodParameters();

        m_DemodFreqX = XfromFreq(m_DemodCenterFreq);
        m_DemodLowCutFreqX = XfromFreq(m_DemodCenterFreq + m_DemodLowCutFreq);
        m_DemodHiCutFreqX = XfromFreq(m_DemodCenterFreq + m_DemodHiCutFreq);

        int dw = m_DemodHiCutFreqX - m_DemodLowCutFreqX;

        painter.setBrush(Qt::SolidPattern);
        painter.setOpacity(0.3);
        painter.fillRect(m_DemodLowCutFreqX, 0, dw, h, Qt::gray);

        painter.setOpacity(1.0);
        painter.setPen(QPen(QColor(0xFF,0x71,0x71,0xFF), 1, Qt::SolidLine));
        painter.drawLine(m_DemodFreqX, 0, m_DemodFreqX, h);
    }

    // create Font to use for scales
    QFont Font("Arial");
    Font.setPointSize(m_FontSize);
    QFontMetrics metrics(Font);

    Font.setWeight(QFont::Normal);
    painter.setFont(Font);

    // draw vertical grids
    pixperdiv = (float)w / (float)m_HorDivs;
    y = h - h/m_VerDivs/2;
    for (int i = 1; i < m_HorDivs; i++)
    {
        x = (int)((float)i*pixperdiv);
        if ((i == m_HorDivs/2) && m_CenterLineEnabled)
            // center line
            painter.setPen(QPen(QColor(0x78,0x82,0x96,0xFF), 1, Qt::SolidLine));
        else
            painter.setPen(QPen(QColor(0xF0,0xF0,0xF0,0x30), 1, Qt::DotLine));

        painter.drawLine(x, 0, x , y);
    }

    // draw frequency values
    MakeFrequencyStrs();
    painter.setPen(QColor(0xD8,0xBA,0xA1,0xFF));
    y = h - (h/m_VerDivs);
    for (int i = 1; i < m_HorDivs; i++)
    {
        x = (int)((float)i*pixperdiv - pixperdiv/2);
        rect.setRect(x, y, (int)pixperdiv, h/m_VerDivs);
        painter.drawText(rect, Qt::AlignHCenter|Qt::AlignBottom, m_HDivText[i]);
    }

    m_dBStepSize = abs(m_MaxdB-m_MindB)/(double)m_VerDivs;
    pixperdiv = (float)h / (float)m_VerDivs;
    painter.setPen(QPen(QColor(0xF0,0xF0,0xF0,0x30), 1,Qt::DotLine));
    for (int i = 1; i < m_VerDivs; i++)
    {
        y = (int)((float) i*pixperdiv);
        painter.drawLine(5*metrics.width("0",-1), y, w, y);
    }

    // draw amplitude values
    painter.setPen(QColor(0xD8,0xBA,0xA1,0xFF));
    //Font.setWeight(QFont::Light);
    painter.setFont(Font);
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
}

//////////////////////////////////////////////////////////////////////
// Helper function Called to create all the frequency division text
//strings based on start frequency, span frequency, frequency units.
//Places in QString array m_HDivText
//Keeps all strings the same fractional length
//////////////////////////////////////////////////////////////////////
void CPlotter::MakeFrequencyStrs()
{
    qint64 FreqPerDiv = m_Span/m_HorDivs;
    qint64 StartFreq = m_CenterFreq - m_Span/2;
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
    StartFreq = m_CenterFreq - m_Span/2;
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
int CPlotter::XfromFreq(qint64 freq)
{
    float w = m_OverlayPixmap.width();
    float StartFreq = (float)m_CenterFreq - (float)m_Span/2.;
    int x = (int) w * ((float)freq - StartFreq)/(float)m_Span;
    if (x < 0)
        return 0;
    if (x > (int)w)
        return m_OverlayPixmap.width();
    return x;
}

qint64 CPlotter::FreqfromX(int x)
{
    float w = m_OverlayPixmap.width();
    float StartFreq = (float)m_CenterFreq - (float)m_Span/2.;
    qint64 f = (int)(StartFreq + (float)m_Span * (float)x/(float)w );
    return f;
}

//////////////////////////////////////////////////////////////////////
// Helper function to round frequency to click resolution value
//////////////////////////////////////////////////////////////////////
qint64 CPlotter::RoundFreq(qint64 freq, int resolution)
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
void CPlotter::ClampDemodParameters()
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

void CPlotter::SetDemodRanges(int FLowCmin, int FLowCmax, int FHiCmin, int FHiCmax, bool symetric)
{
    m_FLowCmin=FLowCmin;
    m_FLowCmax=FLowCmax;
    m_FHiCmin=FHiCmin;
    m_FHiCmax=FHiCmax;
    m_symetric=symetric;
    ClampDemodParameters();
    DrawOverlay();
}


void CPlotter::SetCenterFreq(quint64 f)
{
    qint64 offset = m_CenterFreq - m_DemodCenterFreq;

    m_CenterFreq = f;
    m_DemodCenterFreq = m_CenterFreq - offset;

    DrawOverlay();
}
