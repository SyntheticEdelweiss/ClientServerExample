#pragma once

#include <QtWidgets/QProgressDialog>

/* Should prevent user from closing it, while allowing to do it programmaticaly.
 * Cancel button press is connected to QProgressDialog::canceled, and there's no easy way around that like overriding event or slot.
 * So you either prevent undesired canceled() emits, or prevent things connected to canceled(), or disconnect QProgressDialog::canceled from QProgressDialog::cancel.
 * */
class PersistentProgressDialog : public QProgressDialog
{
    Q_OBJECT
public:
    PersistentProgressDialog(const QString& labelText, const QString& cancelButtonText, int minimum, int maximum, QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    PersistentProgressDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    ~PersistentProgressDialog() { setCloseEnabled(true); }

    inline void setCloseEnabled(bool isEnabled) { m_isCloseEnabled = isEnabled; }
    inline void setDefaultLabelText(QString const& text) { m_defaultLabelText = text; }

    void setCancelButton(QPushButton* cancelButton) { Q_ASSERT(false); } // just don't call it

protected:
    bool m_isCloseEnabled = false;
    QString m_defaultLabelText{QObject::tr("Awaiting task completion...")};

    void init();
    virtual bool event(QEvent* event) override;
    virtual void closeEvent(QCloseEvent* event) override;
    virtual void keyPressEvent(QKeyEvent* event) override;

public slots:
    virtual void reject() override;
    void cancel(); // really wish it was virtual
    void reset();
};
