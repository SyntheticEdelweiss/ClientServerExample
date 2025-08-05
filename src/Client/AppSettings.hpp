#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QDialog>

#include "Net/NetUtils.hpp"

namespace Ui {
class AppSettings;
}

class AppSettings : public QDialog
{
    Q_OBJECT
public:
    AppSettings() = delete;
    explicit AppSettings(QWidget* parent);
    ~AppSettings();
    AppSettings(const AppSettings&) = delete;            // Copy constructor
    AppSettings(AppSettings&&) = delete;                 // Move constructor
    AppSettings& operator=(const AppSettings&) = delete; // Copy assignment
    AppSettings& operator=(AppSettings&&) = delete;      // Move assignment

public:
    Net::ConnectionSettings m_netSettings;

public:
    void init(Net::ConnectionSettings a_netSettings);

private slots:
    void onAccept();
    void onReject();
    void onReset();

private:
    Ui::AppSettings* ui;
};
