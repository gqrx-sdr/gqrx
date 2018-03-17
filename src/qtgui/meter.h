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
#pragma once

#include <QtGui>
#include <QFrame>
#include <QImage>

class CMeter : public QFrame
{
    Q_OBJECT

public:
    explicit CMeter(QWidget *parent = 0);
    explicit CMeter(float min_level = -100.0, float max_level = 10.0,
                    QWidget *parent = 0);
    ~CMeter();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

    void setMin(float min_level);
    void setMax(float max_level);
    void setRange(float min_level, float max_level);

    void draw();
    void UpdateOverlay(){DrawOverlay();}

public slots:
    void setLevel(float dbfs);
    void setSqlLevel(float dbfs);

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent* event);

private:
    void DrawOverlay();

    QFont   m_Font;
    QPixmap m_2DPixmap;
    QPixmap m_OverlayPixmap;
    QSize   m_Size;
    QString m_Str;
    qreal   m_pixperdb;     // pixels / dB
    int     m_Siglevel;
    int     m_dBFS;
    qreal   m_Sql;
    qreal   m_SqlLevel;
};
