#include "PersistentProgressDialog.hpp"

#include <qevent.h>

PersistentProgressDialog::PersistentProgressDialog(const QString& labelText, const QString& cancelButtonText, int minimum, int maximum, QWidget* parent, Qt::WindowFlags f)
    : QProgressDialog(labelText, cancelButtonText, minimum, maximum, parent, f)
{
    init();
}

PersistentProgressDialog::PersistentProgressDialog(QWidget* parent, Qt::WindowFlags f)
    : QProgressDialog(parent, f)
{
    init();
    setLabelText(QObject::tr("Awaiting task completion..."));
}

void PersistentProgressDialog::init()
{
    setAutoClose(true);
    setAutoReset(false);
    setWindowModality(Qt::NonModal);
    setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint & ~Qt::WindowContextHelpButtonHint);
    setMinimumDuration(1000);
    // QProgressDialog constructor starts a timer connected to forceShow() slot, with only way to prevent unnecessary show() is to fulfill forceShow() early return condition
    QProgressDialog::cancel();
    // Need to disconnect original cancel and instead connect redefined one
    QObject::disconnect(this, &QProgressDialog::canceled, 0, 0);
    // Setting labelText here because cancel() can be called to no avail, while actual cancelation is tied to canceled() signal
    QObject::connect(this, &QProgressDialog::canceled, this, [this](){ setLabelText(QObject::tr("Canceling...")); });
    QObject::connect(this, &QProgressDialog::canceled, this, &PersistentProgressDialog::cancel);
}

bool PersistentProgressDialog::event(QEvent* event)
{
    // Handle ShortcutOverride to prevent Esc key from closing
    if (event->type() == QEvent::ShortcutOverride)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        QKeySequence seq(keyEvent->modifiers() | keyEvent->key());
        if (seq.matches(QKeySequence::Cancel) == QKeySequence::ExactMatch && !m_isCloseEnabled)
        {
            event->accept();
            return true;
        }
    }
    return QProgressDialog::event(event);
}

void PersistentProgressDialog::closeEvent(QCloseEvent* event)
{
    if (m_isCloseEnabled)
        QProgressDialog::closeEvent(event);
    else
        event->accept();
}

void PersistentProgressDialog::keyPressEvent(QKeyEvent* event)
{
    // Probably doesn't make much sense doing it here, and instead should handle QEvent::ShortcutOverride in event() override
    QKeySequence seq(event->modifiers() | event->key());
    if (seq.matches(QKeySequence::Cancel) == QKeySequence::ExactMatch && !m_isCloseEnabled)
        event->accept();
    else
        QProgressDialog::keyPressEvent(event);
}

void PersistentProgressDialog::reject()
{
    if (m_isCloseEnabled)
        QProgressDialog::reject();
}

void PersistentProgressDialog::cancel()
{
    if (m_isCloseEnabled)
        QProgressDialog::cancel();
}

void PersistentProgressDialog::reset()
{
    setLabelText(m_defaultLabelText);
    // No need to check m_isCloseEnabled since reset() is not force called from qt internals
    QProgressDialog::reset();
}
