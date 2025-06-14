// License: This project is licensed under the GNU General Public License v2.0 (GPL-2.0).
// Project author: Nalle Berg
// Project name: IPGui
// Project description: A simple IP lookup/renew tool for Windows.
// Project version: 1.0.0
// Compiler: MSVC 19.29.30133.0
// Target platform: Windows
// Target architecture: x64
// Build configuration: x64 Release

#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QPushButton>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QAbstractSocket>
#include <QProcess>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QRegularExpression>
#include <QScreen>

// Windows API for gateway
#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")


// Function to get the default gateway for a given IP address
QString getDefaultGateway(const QString& ipAddress) {
    // Try Windows API first
    ULONG outBufLen = 15000;
    IP_ADAPTER_ADDRESSES* addresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
    if (!addresses) return "Unavailable";
    QString gateway = "Unavailable";
    DWORD dwRetVal = GetAdaptersAddresses(AF_INET, 0, NULL, addresses, &outBufLen);
    if (dwRetVal == NO_ERROR) {
        for (IP_ADAPTER_ADDRESSES* aa = addresses; aa; aa = aa->Next) {
            for (IP_ADAPTER_UNICAST_ADDRESS* ua = aa->FirstUnicastAddress; ua; ua = ua->Next) {
                SOCKADDR_IN* sa_in = (SOCKADDR_IN*)ua->Address.lpSockaddr;
                QString addr = QString::fromUtf8(inet_ntoa(sa_in->sin_addr));
                if (addr == ipAddress) {
                    for (IP_ADAPTER_GATEWAY_ADDRESS_LH* ga = aa->FirstGatewayAddress; ga; ga = ga->Next) {
                        SOCKADDR* gwSock = ga->Address.lpSockaddr;
                        if (gwSock->sa_family == AF_INET) {
                            SOCKADDR_IN* gw = (SOCKADDR_IN*)gwSock;
                            QString gwAddr = QString::fromUtf8(inet_ntoa(gw->sin_addr));
                            if (gwAddr != "0.0.0.0" && !gwAddr.isEmpty()) {
                                gateway = gwAddr;
                                free(addresses);
                                return gateway;
                            }
                        }
                    }
                }
            }
        }
    }
    free(addresses);

    // Fallback: parse ipconfig output
    QProcess proc;
    proc.start("ipconfig");
    proc.waitForFinished();
    QString ipconfigOutput = proc.readAllStandardOutput();
    QStringList lines = ipconfigOutput.split('\n');
    bool inSection = false, foundIp = false;
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        if (line.contains(ipAddress)) {
            inSection = true;
            foundIp = true;
        }
        if (inSection && line.startsWith("Default Gateway")) {
            // Try to extract IPv4 from this line
            QRegularExpression re(R"((\d{1,3}\.){3}\d{1,3})");
            QRegularExpressionMatch match = re.match(line);
            if (match.hasMatch()) {
                return match.captured(0);
            }
            // Or from the next non-empty line
            for (int j = i + 1; j < lines.size(); ++j) {
                QString nextLine = lines[j].trimmed();
                if (!nextLine.isEmpty()) {
                    QRegularExpressionMatch match2 = re.match(nextLine);
                    if (match2.hasMatch()) {
                        return match2.captured(0);
                    }
                } else {
                    break;
                }
            }
        }
        // End of section
        if (foundIp && line.isEmpty()) break;
    }
    return gateway;
}

// Main function
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QString ipAddress;
    QString subnetMask;
    QString defaultGateway = "Unavailable";
    QString externalIp = "Checking...";

    // Get local IP and subnet mask (first non-loopback, non-virtual, up, running)
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        if (!(iface.flags() & QNetworkInterface::IsUp) || !(iface.flags() & QNetworkInterface::IsRunning) || (iface.flags() & QNetworkInterface::IsLoopBack))
            continue;
        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isLoopback()) {
                ipAddress = entry.ip().toString();
                subnetMask = entry.netmask().toString();
                break;
            }
        }
        if (!ipAddress.isEmpty()) break;
    }

    // Get default gateway using hybrid method
    defaultGateway = getDefaultGateway(ipAddress);

    // Get external IP address synchronously (with timeout and error handling)
    {
        QNetworkAccessManager manager;
        QNetworkRequest request(QUrl("https://api.ipify.org"));
        QNetworkReply *reply = manager.get(request);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        // Set a timeout for the request
        // If the request takes too long, we will set a default value
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(3000);

        loop.exec();
        // Check if the reply is finished and if there was no error
        // If the reply is not finished, it means we timed out or there was an error
        if (reply->isFinished() && reply->error() == QNetworkReply::NoError) {
            externalIp = reply->readAll();
            if (externalIp.trimmed().isEmpty())
                externalIp = "Unavailable";
        } else if (timer.isActive()) {
            externalIp = "Unavailable (no network or server down)";
        } else {
            externalIp = "Timeout (no network?)";
        }
        reply->deleteLater();
    }
    // Get the adapter name (first non-loopback, non-virtual, up, running)
    // This is a simple way to get the adapter name, but it may not be the most reliable
    QString adapterName;

    
for (const QNetworkInterface &iface : interfaces) {
    if (!(iface.flags() & QNetworkInterface::IsUp) || !(iface.flags() & QNetworkInterface::IsRunning) || (iface.flags() & QNetworkInterface::IsLoopBack))
        continue;
    for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
        if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isLoopback()) {
            ipAddress = entry.ip().toString();
            subnetMask = entry.netmask().toString();
            adapterName = iface.humanReadableName();
            break;
        }
    }
    if (!ipAddress.isEmpty()) break;
}
    
    QMainWindow window;
    window.setWindowTitle("IPGui");

    QWidget *centralWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    // Info display
    QTextEdit *infoBox = new QTextEdit();
    infoBox->setReadOnly(true);

    // Store the basic info HTML
    QString basicInfoHtml =
    "<div align='center'>"
        "<table border='0' cellpadding='6' cellspacing='0'>"
        "<tr><th align='left' style='color:#9a1321;'>Property</th><th align='left' style='color:#9a1321;'>Value</th></tr>"
        "<tr><td><b>Adapter Type</b></td><td style='color:blue;' align='center'>" + adapterName + "</td></tr>"
        "<tr><td><b>IP Address</b></td><td style='color:blue;' align='right'>" + ipAddress + "</td></tr>"
        "<tr><td><b>Subnet Mask</b></td><td style='color:blue;' align='right'>" + subnetMask + "</td></tr>"
        "<tr><td><b>Default Gateway</b></td><td style='color:blue;' align='right'>" + defaultGateway + "</td></tr>"
        "<tr><td><b>External Address</b></td><td style='color:blue;' align='right'>" + externalIp + "</td></tr>"
        "</table>"
    "</div>";
    infoBox->setHtml(basicInfoHtml);
    layout->addWidget(infoBox);

    // Buttons
    // Create a horizontal layout for the buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *expandBtn = new QPushButton("Advanced");
    QPushButton *renewBtn = new QPushButton("Renew IP");

    // Set tooltips (hover text)
    expandBtn->setToolTip("Show advanced network details\nLike: «ipconfig /all»");
    renewBtn->setToolTip("Renew your IP address (DHCP)");

    // Make button text bold and give it color
    QString boldStyle = "font-weight: bold; color: #595e12;";
    expandBtn->setStyleSheet(boldStyle);
    renewBtn->setStyleSheet(boldStyle);
    buttonLayout->addWidget(expandBtn);
    buttonLayout->addWidget(renewBtn);
    layout->addLayout(buttonLayout);

    window.setCentralWidget(centralWidget);

    // Toggle state for expand/collapse
    bool expanded = false;
    
    // Connect the expand/collapse button
    QObject::connect(expandBtn, &QPushButton::clicked, [&]() mutable {
    if (!expanded) {
        QProcess proc;
        proc.start("ipconfig", QStringList() << "/all");
        proc.waitForFinished();
        QString output = proc.readAllStandardOutput();
        infoBox->setFontFamily("Consolas"); // Monospace font
        infoBox->setLineWrapMode(QTextEdit::NoWrap); // Disable wrapping
        infoBox->setPlainText(output);
        expandBtn->setText("Basic");
        expanded = true;
        // Set a minimum width/height for advanced mode
        int minWidth = 650;
        int minHeight = 500;
        window.resize(minWidth, minHeight);

        // Center the window
        QScreen *screen = QGuiApplication::primaryScreen();
        QRect screenGeometry = screen->availableGeometry();
        QRect windowGeometry = window.frameGeometry();
        int x = screenGeometry.x() + (screenGeometry.width() - windowGeometry.width()) / 2;
        int y = screenGeometry.y() + (screenGeometry.height() - windowGeometry.height()) / 2;
        window.move(x, y);
    } else {
        infoBox->setLineWrapMode(QTextEdit::WidgetWidth); // Restore wrapping
        infoBox->setFontFamily(""); // Restore default font
        infoBox->setHtml(basicInfoHtml);
        expandBtn->setText("Advanced");
        expanded = false;
        window.resize(235, 270);

        // Center the window
        QScreen *screen = QGuiApplication::primaryScreen();
        QRect screenGeometry = screen->availableGeometry();
        QRect windowGeometry = window.frameGeometry();
        int x = screenGeometry.x() + (screenGeometry.width() - windowGeometry.width()) / 2;
        int y = screenGeometry.y() + (screenGeometry.height() - windowGeometry.height()) / 2;
        window.move(x, y);
    }
});
    // Menu bar
    QMenuBar *menuBar = window.menuBar();

    // File menu
    QMenu *fileMenu = menuBar->addMenu("&Actions");

    // Add "Always on top" toggle
    QAction *alwaysOnTopAction = fileMenu->addAction("Always on top");
    alwaysOnTopAction->setCheckable(true);
    QObject::connect(alwaysOnTopAction, &QAction::toggled, &window, [&](bool checked) {
    window.setWindowFlag(Qt::WindowStaysOnTopHint, checked);
    window.show(); // Needed to apply the flag change
});

// Add this block for Renew IP:
QObject::connect(renewBtn, &QPushButton::clicked, [&]() {
    QString command = "Start-Process ipconfig -ArgumentList '/renew' -Verb runAs -WindowStyle Hidden";
    QProcess::startDetached("powershell", QStringList() << "-WindowStyle" << "Hidden" << "-Command" << command);
});

    QAction *exitAction = fileMenu->addAction("E&xit");
    QObject::connect(exitAction, &QAction::triggered, &window, &QWidget::close);

    // Help menu
    QMenu *helpMenu = menuBar->addMenu("&Help");
    QAction *aboutAction = helpMenu->addAction("&About");
   QObject::connect(aboutAction, &QAction::triggered, &window, [&window]() {
    QDialog dlg(&window);
    dlg.setWindowTitle("About");
    QVBoxLayout layout(&dlg);

    // Add icon (adjust path as needed)
    QLabel *iconLabel = new QLabel;
    iconLabel->setPixmap(QPixmap("NSB-logo.png").scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout.addWidget(iconLabel, 0, Qt::AlignHCenter);

    QLabel label(
        "<b>IPGui by Nalle Berg</b><br>"
        "<b>Copyleft 2025</b><br><br>"
        "A simple IP lookup/renew -tool.<br>"
        "Visit my programming <a href='https://prog.nalle.no'> web page</a>.<br>"
        "&nbsp;<br>"
        "<b>Version:</b> 1.0.0<br>"
        "<B>License:</B> <a href='https://www.gnu.org/licenses/old-licenses/gpl-2.0.html'>GPLv2</a><br>"
    );
    label.setOpenExternalLinks(true);
    layout.addWidget(&label);

    QPushButton ok("OK");
    layout.addWidget(&ok);
    QObject::connect(&ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    dlg.adjustSize();
    dlg.exec();
});

    window.resize(235, 270);
    window.show();

    return app.exec();
}