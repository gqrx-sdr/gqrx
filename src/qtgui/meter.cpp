/* -*- c++ -*- */
/* + + +   This Software is released under the "Simplified BSD License"  + + +
 *
 * Copyright 2010 Moe Wheatley. All rights reserved.
 * Copyright 2011-2013 Alexandru Csete OZ9AEC.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY Moe Wheatley ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL Moe Wheatley OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of Moe Wheatley.
 */

#include <cmath>
#include <QDebug>
#include "meter.h"

// ratio to total control width or height
#define CTRL_MARGIN 0.07		// left/right margin
#define CTRL_MAJOR_START 0.3	// top of major tic line
#define CTRL_MINOR_START 0.3	// top of minor tic line
#define CTRL_XAXIS_HEGHT 0.4	// vertical position of horizontal axis
#define CTRL_NEEDLE_TOP 0.4		// vertical position of top of needle triangle

#define MIN_DB -100.0f
#define MAX_DB +0.0f

#define ALPHA_DECAY     0.25f
#define ALPHA_RISE      0.70f

CMeter::CMeter(QWidget *parent) : QFrame(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_PaintOnScreen,false);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setMouseTracking(true);

    m_Font = QFont("Arial");
    m_Font.setWeight(QFont::Normal);
    m_2DPixmap = QPixmap(0,0);
    m_OverlayPixmap = QPixmap(0,0);
    m_Size = QSize(0,0);
    m_Siglevel = 0;
    m_dBFS = MIN_DB;
    m_SqlLevel = 0.0f;
}

CMeter::~CMeter()
{
}

QSize CMeter::minimumSizeHint() const
{
    return QSize(20, 10);
}

QSize CMeter::sizeHint() const
{
    return QSize(100, 30);
}

void CMeter::resizeEvent(QResizeEvent *)
{
    if (!size().isValid())
        return;

    if (m_Size != size())
    {
        // if size changed, resize pixmaps to new screensize
        m_Size = size();
        m_OverlayPixmap = QPixmap(m_Size.width(), m_Size.height());
        m_OverlayPixmap.fill(Qt::black);
        m_2DPixmap = QPixmap(m_Size.width(), m_Size.height());
        m_2DPixmap.fill(Qt::black);

        qreal w = m_2DPixmap.width() - 2 * CTRL_MARGIN * m_2DPixmap.width();
        m_pixperdb = w / fabs(MAX_DB - MIN_DB);
        setSqlLevel(m_Sql);
    }

    DrawOverlay();
    draw();
}

void CMeter::setLevel(float dbfs)
{
    if (dbfs < MIN_DB)
        dbfs = MIN_DB;
    else if (dbfs > MAX_DB)
        dbfs = MAX_DB;

    float level = m_dBFS;
    float alpha  = dbfs < level ? ALPHA_DECAY : ALPHA_RISE;
    m_dBFS -= alpha * (level - dbfs);
    m_Siglevel = (int)((level - MIN_DB) * m_pixperdb);

    draw();
}

void CMeter::setSqlLevel(float dbfs)
{
    if (dbfs >= 0.f)
        m_SqlLevel = 0.0f;
    else
        m_SqlLevel = (dbfs - MIN_DB) * m_pixperdb;

    if (m_SqlLevel < 0.0f)
        m_SqlLevel = 0.0f;

    m_Sql = dbfs;
}

// Called by QT when screen needs to be redrawn
void CMeter::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.drawPixmap(0, 0, m_2DPixmap);
    return;
}

// Called to update s-meter data for displaying on the screen
void CMeter::draw()
{
    int w;
    int h;

    if (m_2DPixmap.isNull())
        return;

    // get/draw the 2D spectrum
    w = m_2DPixmap.width();
    h = m_2DPixmap.height();

    // first copy into 2Dbitmap the overlay bitmap.
    m_2DPixmap = m_OverlayPixmap.copy(0, 0, w, h);
    QPainter painter(&m_2DPixmap);

    // DrawCurrent position indicator
    qreal hline = (qreal) h * CTRL_XAXIS_HEGHT;
    qreal marg = (qreal) w * CTRL_MARGIN;
    qreal ht = (qreal) h * CTRL_NEEDLE_TOP;
    qreal x = marg + m_Siglevel;
    QPoint pts[3];
    pts[0].setX(x);
    pts[0].setY(ht + 2);
    pts[1].setX(x - 6);
    pts[1].setY(hline + 8);
    pts[2].setX(x + 6);
    pts[2].setY(hline + 8);

    painter.setBrush(QBrush(QColor(0, 190, 0, 255)));
    painter.setOpacity(1.0);

    // Qt 4.8+ has a 1-pixel error (or they fixed line drawing)
    // see http://stackoverflow.com/questions/16990326
#if QT_VERSION >= 0x040800
    painter.drawRect(marg - 1, ht + 1, x - marg, 6);
#else
    painter.drawRect(marg, ht + 2, x - marg, 6);
#endif

    if (m_SqlLevel > 0.0f)
    {
        x = marg + m_SqlLevel;
        painter.setPen(QPen(Qt::yellow, 1, Qt::SolidLine));
        painter.drawLine(QLineF(x, hline, x, hline + 8));
    }

    int y = (h) / 4;
    m_Font.setPixelSize(y);
    painter.setFont(m_Font);

    painter.setPen(QColor(0xDA, 0xDA, 0xDA, 0xFF));
    painter.setOpacity(1.0);
    m_Str.setNum(m_dBFS);
    painter.drawText(marg, h - 2, m_Str + " dBFS" );

    update();
}

// Called to draw an overlay bitmap containing items that
// does not need to be recreated every fft data update.
void CMeter::DrawOverlay()
{
    if (m_OverlayPixmap.isNull())
        return;

    int w = m_OverlayPixmap.width();
    int h = m_OverlayPixmap.height();
    int x,y;
    QRect rect;
    QPainter painter(&m_OverlayPixmap);

    m_OverlayPixmap.fill(QColor(0x1F, 0x1D, 0x1D, 0xFF));

    // Draw scale lines
    qreal marg = (qreal) w * CTRL_MARGIN;
    qreal hline = (qreal)h * CTRL_XAXIS_HEGHT;
    qreal magstart = (qreal) h * CTRL_MAJOR_START;
    qreal minstart = (qreal) h * CTRL_MINOR_START;
    qreal hstop = (qreal) w - marg;
    painter.setPen(QPen(Qt::white, 1, Qt::SolidLine));
    painter.drawLine(QLineF(marg, hline, hstop, hline));        // top line
    painter.drawLine(QLineF(marg, hline+8, hstop, hline+8));    // bottom line
    qreal xpos = marg;
    for (x = 0; x < 11; x++) {
        if (x & 1)
            //minor tics
            painter.drawLine(QLineF(xpos, minstart, xpos, hline));
        else
            painter.drawLine(QLineF(xpos, magstart, xpos, hline));
        xpos += (hstop-marg) / 10.0;
    }

    // draw scale text
    y = h / 4;
    m_Font.setPixelSize(y);
    painter.setFont(m_Font);
    int rwidth = (int)((hstop - marg) / 5.0);
    m_Str = "-100";
    rect.setRect(marg / 2 - 5, 0, rwidth, magstart);

    for (x = MIN_DB; x <= MAX_DB; x += 20)
    {
        m_Str.setNum(x);
        painter.drawText(rect, Qt::AlignHCenter|Qt::AlignVCenter, m_Str);
        rect.translate(rwidth, 0);
    }
}

