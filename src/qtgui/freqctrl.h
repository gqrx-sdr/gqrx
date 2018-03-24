/*
 * Frequency controller widget (originally from CuteSDR)
 */
#pragma once

#include <QFrame>
#include <QImage>
#include <QtGui>

enum FctlUnit {
    FCTL_UNIT_NONE,            // Freq displayed without unit: 14.236.000
    FCTL_UNIT_HZ,
    FCTL_UNIT_KHZ,
    FCTL_UNIT_MHZ,
    FCTL_UNIT_GHZ,
    FCTL_UNIT_SEC,
    FCTL_UNIT_MSEC,
    FCTL_UNIT_USEC,
    FCTL_UNIT_NSEC
};

#define FCTL_MAX_DIGITS 12
#define FCTL_MIN_DIGITS 4

class CFreqCtrl : public QFrame
{
    Q_OBJECT

public:
    explicit CFreqCtrl(QWidget *parent = 0);
    ~CFreqCtrl();

    QSize    minimumSizeHint() const;
    QSize    sizeHint() const;

    // Use NumDigits=0 for auto
    void     setup(int NumDigits, qint64 Minf, qint64 Maxf, int MinStep,
                   FctlUnit unit);
    void     setUnit(FctlUnit unit);
    void     setDigitColor(QColor col);
    void     setBgColor(QColor col);
    void     setUnitsColor(QColor col);
    void     setHighlightColor(QColor col);
    qint64 getFrequency() const
    {
        return m_freq;
    }

    void setResetLowerDigits(bool reset)
    {
        m_ResetLowerDigits = reset;
    }

signals:
    void    newFrequency(qint64 freq); // emitted when frequency has changed

public slots:
    void    setFrequency(qint64 freq);

protected:
    void    paintEvent(QPaintEvent *);
    void    resizeEvent(QResizeEvent *);
    void    mouseMoveEvent(QMouseEvent *);
    void    mousePressEvent(QMouseEvent *);
    void    wheelEvent(QWheelEvent *);
    void    leaveEvent(QEvent *);
    void    keyPressEvent(QKeyEvent *);

private:
    void    updateCtrl(bool all);
    void    drawBkGround(QPainter &Painter);
    void    drawDigits(QPainter &Painter);
    void    incDigit();
    void    decDigit();
    void    incFreq();
    void    decFreq();
    void    clearFreq();
    void    cursorHome();
    void    cursorEnd();
    void    moveCursorLeft();
    void    moveCursorRight();
    bool    inRect(QRect &rect, QPoint &point);

    bool        m_UpdateAll;
    bool        m_ExternalKeyActive;
    bool        m_LRMouseFreqSel;   /* Use left/right mouse buttons. If FALSE click area determines up/down. */

    bool        m_ResetLowerDigits; /* If TRUE digits below the active one will be reset to 0
                                     *  when the active digit is incremented or decremented. */

    int         m_FirstEditableDigit;
    int         m_LastLeadZeroPos;
    int         m_LeadZeroPos;
    int         m_NumDigits;
    int         m_NumDigitsForUnit;     // number of digits allocated for unit (kHz, MHz, ...)
    int         m_DigStart;
    int         m_ActiveEditDigit;
    int         m_LastEditDigit;
    int         m_DecPos;
    int         m_NumSeps;

    qint64      m_MinStep;
    qint64      m_freq;
    qint64      m_Oldfreq;
    qint64      m_MinFreq;
    qint64      m_MaxFreq;

    QColor      m_DigitColor;
    QColor      m_BkColor;
    QColor      m_UnitsColor;
    QColor      m_HighlightColor;

    QPixmap     m_Pixmap;
    QSize       m_Size;
    FctlUnit    m_Unit;

    QRect       m_rectCtrl;                 // main control rectangle
    QRect       m_UnitsRect;                // rectangle where Units text goes
    QRect       m_SepRect[FCTL_MAX_DIGITS]; // separation rectangles for commas, decimal point, etc.

    QString     m_UnitString;

    QFont       m_DigitFont;
    QFont       m_UnitsFont;

    struct DigStuct {
        qint64    weight;      // decimal weight of this digit
        qint64    incval;      // value this digit increments or decrements
        QRect     dQRect;      // Digit bounding rectangle
        int       val;         // value of this digit(0-9)
        bool      modified;    // set if this digit has been modified
        bool      editmode;    // set if this digit is selected for editing
    } m_DigitInfo[FCTL_MAX_DIGITS];
};
