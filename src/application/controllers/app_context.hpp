// Aether application layer: shared context passed to every page (state + task/status signalling).
// Uses QObject for signals only; contains no numerical code.
#pragma once

#include <QObject>
#include <QString>

#include "../services/workspace.hpp"

class AppContext : public QObject {
    Q_OBJECT
public:
    aether::application::WorkspaceState state;

    void log(const QString& msg) { state.log(msg.toStdString()); emit stateChanged(); }

signals:
    void stateChanged();                                   // overview should refresh
    void taskStarted(const QString& task, bool cancellable);
    void taskProgress(double fraction);                    // 0..1, <0 = indeterminate
    void taskFinished(const QString& message);
    void taskFailed(const QString& message);
    void cancelRequested();
    void navigate(int pageIndex);
    void projectChanged();                                 // after open/new: pages may reload files
};
