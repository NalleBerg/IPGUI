#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QProcess>
#include <QRegularExpression>
#include <QString>
#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

QString getBasicNetworkInfo() {
    QString ipAddress = "Not found";
    QString subnetMask = "Not found";
    IP_ADAPTER_INFO adapterInfo[16];
    DWORD bufLen = sizeof(adapterInfo);

    DWORD status = GetAdaptersInfo(adapterInfo, &bufLen);
    if (status == ERROR_SUCCESS) {
        PIP_ADAPTER_INFO pAdapter = adapterInfo;
        while (pAdapter) {
            QString ip = QString::fromLocal8Bit(pAdapter->IpAddressList.IpAddress.String);
            QString mask = QString::fromLocal8Bit(pAdapter->IpAddressList.IpMask.String);
            if (ip != "0.0.0.0" && ip != "127.0.0.1") {
                ipAddress = ip;
                subnetMask = mask;
                break;
            }
            pAdapter = pAdapter->Next;
        }
    }
    return QString(
        "<table border='1' cellpadding='6' cellspacing='0'>"
        "<tr><th align='left'>Property</th><th align='left'>Value</th></tr>"
        "<tr><td><b>IP Address</b></td><td style='color:blue;'>%1</td></tr>"
        "<tr><td><b>Subnet Mask</b></td><td style='color:blue;'>%2</td></tr>"
        "</table>"
    ).arg(ipAddress, subnetMask);
}

QString getIpconfigAll() {
    QProcess process;
    process.start("cmd.exe", QStringList() << "/C" << "ipconfig /all");
    if (!process.waitForStarted(3000)) {
        return "Failed to start process";
    }
    if (!process.waitForFinished(3000)) {
        return "Process timed out";
    }
    return process.readAllStandardOutput();
}

void renewIpWithElevation(QWidget *parent = nullptr) {
    ShellExecuteW(
        nullptr, L"runas", L"cmd.exe",
        L"/C ipconfig /renew && pause",
        nullptr, SW_SHOW
    );
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QMainWindow window;
    window.setWindowTitle("NetGui by Nalle Berg");

    QWidget *centralWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    // Show basic info on startup
    QLabel *label = new QLabel(getBasicNetworkInfo());
    label->setTextFormat(Qt::RichText);
    layout->addWidget(label, 0, Qt::AlignCenter);

    // Text box for ipconfig /all output
    QTextEdit *outputEdit = new QTextEdit;
    outputEdit->setReadOnly(true);
    outputEdit->hide(); // Hidden until "Show Details" is pressed
    layout->addWidget(outputEdit);

    // Buttons
    QPushButton *detailsBtn = new QPushButton("Show Details (/all)");
    QPushButton *renewBtn = new QPushButton("Renew IP (UAC)");
    layout->addWidget(detailsBtn);
    layout->addWidget(renewBtn);

    window.setCentralWidget(centralWidget);

    // Menu bar
    QMenuBar *menuBar = window.menuBar();
    QMenu *fileMenu = menuBar->addMenu("&File");
    QAction *exitAction = fileMenu->addAction("E&xit");
    QObject::connect(exitAction, &QAction::triggered, &window, &QWidget::close);

    QMenu *helpMenu = menuBar->addMenu("&Help");
    QAction *aboutAction = helpMenu->addAction("&About");
    QObject::connect(aboutAction, &QAction::triggered, &window, [&window]() {
        QDialog dlg(&window);
        dlg.setWindowTitle("About");
        QVBoxLayout layout(&dlg);
        QLabel label("This is a Qt example application for network troubleshooting.\n"
                     "- Shows IP and subnet mask\n"
                     "- Shows ipconfig /all output\n"
                     "- Can renew IP with admin rights");
        layout.addWidget(&label);
        QPushButton ok("OK");
        layout.addWidget(&ok);
        QObject::connect(&ok, &QPushButton::clicked, &dlg, &QDialog::accept);
        dlg.exec();
    });

    // Button actions
    QObject::connect(detailsBtn, &QPushButton::clicked, [=]() {
        outputEdit->setPlainText(getIpconfigAll());
        outputEdit->show();
    });

    QObject::connect(renewBtn, &QPushButton::clicked, [&window]() {
        renewIpWithElevation(&window);
    });

    window.resize(700, 400);
    window.show();

    return app.exec();
}