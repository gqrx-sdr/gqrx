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
#define CTRL_MARGIN 0.07        // left/right margin
#define CTRL_MAJOR_START 0.3    // top of major tic line
#define CTRL_MINOR_START 0.34   // top of minor tic line
#define CTRL_XAXIS_HEGHT 0.4    // vertical position of horizontal axis
#define CTRL_NEEDLE_TOP 0.4     // vertical position of top of needle triangle

#define MIN_DB -100.0f
#define MAX_DB +0.0f

#define ALPHA_DECAY     0.25f
#define ALPHA_RISE      0.70f

CMeter::CMeter(QWidget *parent) : QFrame(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_dBFS = MIN_DB;
    m_Sql = -150.0;
    m_font = QFont("Arial");
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

void CMeter::setLevel(float dbfs)
{
    const float old = m_dBFS;
    float alpha = dbfs < m_dBFS ? ALPHA_DECAY : ALPHA_RISE;
    m_dBFS -= alpha * (m_dBFS - dbfs);
    // only redraw when the label needs to change
    if (qRound(m_dBFS * 10) != qRound(old * 10))
    {
        update();
    }
}

void CMeter::setSqlLevel(float dbfs)
{
    m_Sql = dbfs;
    update();
}

// Called by QT when screen needs to be redrawn
void CMeter::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    drawOverlay(painter);
    draw(painter);
}

// Called to update s-meter data for displaying on the screen
void CMeter::draw(QPainter &painter)
{
    // Draw current position indicator
    qreal hline = (qreal) height() * CTRL_XAXIS_HEGHT;
    qreal marg = (qreal) width() * CTRL_MARGIN;
    qreal ht = (qreal) height() * CTRL_NEEDLE_TOP;
    qreal pixperdb = (width() - 2 * CTRL_MARGIN * width()) / (qreal)(MAX_DB - MIN_DB);

    if (m_dBFS > MIN_DB)
    {
        QColor color(0, 190, 0, 255);
        QPen pen(color);
        pen.setJoinStyle(Qt::MiterJoin);
        painter.setPen(pen);
        painter.setBrush(QBrush(color));
        painter.drawRect(QRectF(marg, ht + 2, (qreal)(std::min(m_dBFS, MAX_DB) - MIN_DB) * pixperdb, 4));
    }

    if (m_Sql > MIN_DB)
    {
        qreal x = marg + (qreal)(m_Sql - MIN_DB) * pixperdb;
        painter.setPen(QPen(Qt::yellow, 1, Qt::SolidLine));
        painter.drawLine(QLineF(x, hline, x, hline + 8));
    }

    m_font.setPixelSize(height() / 4);
    painter.setFont(m_font);

    painter.setPen(QColor(0xDA, 0xDA, 0xDA, 0xFF));
    painter.drawText(marg, height() - 2, QString::number((double)m_dBFS, 'f', 1) + " dBFS" );
}

// Called to draw an overlay bitmap containing items that
// does not need to be recreated every fft data update.
void CMeter::drawOverlay(QPainter &painter)
{
    // Draw scale lines
    qreal marg = (qreal) width() * CTRL_MARGIN;
    qreal hline = (qreal) height() * CTRL_XAXIS_HEGHT;
    qreal majstart = (qreal) height() * CTRL_MAJOR_START;
    qreal minstart = (qreal) height() * CTRL_MINOR_START;
    qreal hstop = (qreal) width() - marg;
    painter.setPen(QPen(Qt::white, 1, Qt::SolidLine));
    painter.drawLine(QLineF(marg, hline, hstop, hline));        // top line
    painter.drawLine(QLineF(marg, hline+8, hstop, hline+8));    // bottom line
    qreal xpos = marg;
    for (int x = 0; x <= 10; x++) {
        if (x & 1)
            //minor tics
            painter.drawLine(QLineF(xpos, minstart, xpos, hline));
        else
            painter.drawLine(QLineF(xpos, majstart, xpos, hline));
        xpos += (hstop-marg) / 10.0;
    }

    // draw scale text
    m_font.setPixelSize(height() / 4);
    painter.setFont(m_font);
    qreal rwidth = (hstop - marg) / 5.0;
    QRectF rect(marg - rwidth / 2, 0, rwidth, majstart);

    for (int x = MIN_DB; x <= MAX_DB; x += 20)
    {
        painter.drawText(rect, Qt::AlignHCenter|Qt::AlignVCenter, QString::number(x));
        rect.translate(rwidth, 0);
    }
}
