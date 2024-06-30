#ifndef TIMER_REFRESHCD_H
#define TIMER_REFRESHCD_H

#include <QObject>
#include <QTimer>

class QTimer;
class Timer_RefreshCD : public QObject
{
    Q_OBJECT
public:
    explicit Timer_RefreshCD(QObject *parent = nullptr);

signals:
    void Timeout();
};

#endif // TIMER_REFRESHCD_H
