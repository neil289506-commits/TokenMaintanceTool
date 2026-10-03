#pragma once
// Runs a slow function (PBKDF2 / RSA keygen) on a worker thread while showing a modal,
// non-cancellable "working" dialog. The GUI thread keeps pumping events.
#include <QDialog>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QCloseEvent>
#include <QtConcurrent>

class BusyDialog : public QDialog {
public:
    BusyDialog(const QString &text, QWidget *parent) : QDialog(parent, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint)
    {
        setModal(true);
        setWindowTitle(tr("請稍候"));
        auto *l = new QVBoxLayout(this);
        l->addWidget(new QLabel(text));
        auto *bar = new QProgressBar;
        bar->setRange(0, 0);
        bar->setTextVisible(false);
        l->addWidget(bar);
        setMinimumWidth(320);
    }
    void reject() override {}
    void closeEvent(QCloseEvent *e) override { e->ignore(); }
};

template<typename T, typename F>
T runBusy(QWidget *parent, const QString &text, F fn)
{
    BusyDialog dlg(text, parent);
    QFutureWatcher<T> watcher;
    QEventLoop loop;
    QObject::connect(&watcher, &QFutureWatcher<T>::finished, &loop, &QEventLoop::quit);
    watcher.setFuture(QtConcurrent::run(fn));
    dlg.show();
    if (!watcher.isFinished())
        loop.exec();
    dlg.hide();
    return watcher.result();
}
