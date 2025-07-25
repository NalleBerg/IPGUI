// License: This project is licensed under the GNU General Public License v2.0 (GPL-2.0).
// Project author: Nalle Berg
// Project name: IPGui
// Project description: A simple IP lookup/renew tool for Windows.
// Project version: 3.6.0
// Compiler: MSVC 19.29.30133.0
// Target platform: Windows
// Target architecture: x64
// Build configuration: x64 Release


// Windows API - Must be included before Qt headers.
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <wlanapi.h>
#include <objbase.h>
#include <wtypes.h>
#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")


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
#include <QThread>
#include <QMovie>
#include <QLineEdit>
#include <QScrollBar>
#include <QSpinBox>
#include <QTcpSocket>
#include <QHostInfo>
#include <QTcpSocket>
#include <QFile>
#include <QTextStream>
#include <QTextBrowser>
#include <QHostAddress>
#include <QHostInfo>
#include <QProgressBar>
#include <QInputDialog>
#include <QMap>
#include <QPair>
#include <QTableWidget>
#include <QHeaderView>
#include <QFormLayout>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QDesktopServices>
#include <lm.h>
#include <QShortcut>
#include <QScrollArea>
#include <QSslSocket>
#include <QSslCertificate>
#include <QDialogButtonBox>
#include <QIntValidator>
#include <QTextEdit>
#include <QCryptographicHash>
#include <QVariant>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QFileDialog>
#include <QGroupBox>



// Windows API for network functions Share-scanner
#pragma comment(lib, "Netapi32.lib")
#include <QTreeWidget>
#include <QProgressDialog>


// Windows API for gateway
#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")

// Helper function to add Ctrl+W shortcut to close a dialog
inline void addCtrlWClose(QDialog *dlg) {
    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, dlg, &QDialog::accept);
}


//Global variables
const QString VersionNumber = "3.6.0";
const QString html = QString("<b>Version:</b> %1<br>").arg(VersionNumber);

// Helper: Get the path to the shared CSV file
// this part belongs to the port scanner dialog
QString getPortInfoCsvPath() {
    // Explicitly use the Public user's AppData\Local\IPGui directory
    QString dir = "C:/Users/Public/AppData/Local/IPGui";
    QDir().mkpath(dir); // Ensure the directory exists
    return dir + "/service-names-port-numbers.csv";
}

// Helper: Download the CSV to the shared location
bool downloadPortInfoCsv(const QString &path, QWidget *parent = nullptr) {
    QNetworkAccessManager mgr;
    QNetworkRequest req(QUrl("https://www.iana.org/assignments/service-names-port-numbers/service-names-port-numbers.csv"));
    QNetworkReply *reply = mgr.get(req);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(7000); // 7s timeout
    loop.exec();
    bool ok = false;
    if (reply->isFinished() && reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write(data);
            file.close();
            ok = true;
        }
    }
    reply->deleteLater();
    if (!ok && parent) {
        QMessageBox::warning(parent, "Port Info", "Could not download port info from IANA.");
    }
    return ok;
}
// End of helper functions

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

// Function for IP scanner dialog 
void showNetworkScannerDialog(QWidget *parent) {
    QDialog dlg(parent);
    dlg.setWindowTitle("IP Scanner");
    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLabel *prompt = new QLabel("Scan for devices in your local network:");
    layout->addWidget(prompt);

    // --- From/To IP input on one line ---
    QHBoxLayout *ipRangeLayout = new QHBoxLayout();
    QLabel *fromLabel = new QLabel("<b>From:</b>");
    QLineEdit *fromEdit = new QLineEdit;
    QLabel *toLabel = new QLabel("<b>To:</b>");
    QLineEdit *toEdit = new QLineEdit;

    // Auto-fill with local segment
    QString defaultBase = "192.168.1";
    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol &&
                !entry.ip().isLoopback() &&
                !entry.ip().toString().startsWith("169.254.")) {
                QString ip = entry.ip().toString();
                QStringList parts = ip.split('.');
                if (parts.size() == 4)
                    defaultBase = QString("%1.%2.%3").arg(parts[0]).arg(parts[1]).arg(parts[2]);
                break;
            }
        }
    }
    fromEdit->setText(defaultBase + ".1");
    toEdit->setText(defaultBase + ".255");

    ipRangeLayout->addWidget(fromLabel);
    ipRangeLayout->addWidget(fromEdit);
    ipRangeLayout->addSpacing(10);
    ipRangeLayout->addWidget(toLabel);
    ipRangeLayout->addWidget(toEdit);
    layout->addLayout(ipRangeLayout);

    // --- Buttons ---
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *scanBtn = new QPushButton("Scan");
    scanBtn->setToolTip("Start scanning the selected IP range for active devices.");    
    QPushButton *stopBtn = new QPushButton("Stop");
    stopBtn->setToolTip("Stop the ongoing scan.");
    QPushButton *closeBtn = new QPushButton("Close");
    closeBtn->setToolTip("Close the scanner dialog.");
    btnLayout->addWidget(scanBtn);
    btnLayout->addWidget(stopBtn);
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    QProgressBar *progress = new QProgressBar;
    progress->setMinimum(0);
    progress->setMaximum(255);
    layout->addWidget(progress);

    QTextBrowser *output = new QTextBrowser;
    output->setReadOnly(true);
    output->setMinimumHeight(120);
    output->setOpenExternalLinks(true);
    layout->addWidget(output);

    // --- Ctrl+W shortcut to close dialog ---
    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), &dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, &dlg, &QDialog::accept);

    bool isScanning = false;
    QObject::connect(scanBtn, &QPushButton::clicked, [&]() {
        scanBtn->setEnabled(false);
        stopBtn->setEnabled(true);
        output->clear();
        isScanning = true;

        QString fromIp = fromEdit->text().trimmed();
        QString toIp = toEdit->text().trimmed();

        QHostAddress fromAddr(fromIp), toAddr(toIp);
        if (fromAddr.protocol() != QAbstractSocket::IPv4Protocol ||
            toAddr.protocol() != QAbstractSocket::IPv4Protocol) {
            output->setHtml("<b>Invalid IP address format.</b>");
            scanBtn->setEnabled(true);
            stopBtn->setEnabled(false);
            return;
        }
        quint32 from = fromAddr.toIPv4Address();
        quint32 to = toAddr.toIPv4Address();
        if (from > to) std::swap(from, to);

        int found = 0;
        int total = to - from + 1;
        progress->setMaximum(total);

        QString rows; // For HTML table rows

        for (quint32 ipInt = from, i = 0; ipInt <= to && isScanning; ++ipInt, ++i) {
            QString ip = QHostAddress(ipInt).toString();
            QProcess ping;
            ping.start("ping", QStringList() << "-n" << "1" << "-w" << "100" << ip);
            ping.waitForFinished(300);
            QString result = ping.readAllStandardOutput();
            if (result.contains("TTL=")) {
                QString host = QHostInfo::fromName(ip).hostName();

                // Check for HTTPS/HTTP
                bool hasHttps = false, hasHttp = false;
                {
                    QTcpSocket sock;
                    sock.connectToHost(ip, 443);
                    if (sock.waitForConnected(50)) {
                        hasHttps = true;
                        sock.disconnectFromHost();
                    }
                }
                if (!hasHttps) {
                    QTcpSocket sock;
                    sock.connectToHost(ip, 80);
                    if (sock.waitForConnected(50)) {
                        hasHttp = true;
                        sock.disconnectFromHost();
                    }
                }

                // Create a link for the IP address if it has a web interface
                QString ipLink;
                if (hasHttps) {
                    ipLink = QString("<a href=\"https://%1\" title=\"Open in browser\">%1</a>").arg(ip);
                } else if (hasHttp) {
                    ipLink = QString("<a href=\"http://%1\" title=\"Open in browser\">%1</a>").arg(ip);
                } else {
                    ipLink = ip;
                }

                QString shownHost = (host.isEmpty() || host == ip) ? "" : host;
                rows += QString("<tr><td style='padding-right:32px;'>%1</td><td>%2</td></tr>").arg(ipLink, shownHost);

                ++found;

                // Live update the table after each found device
                output->setHtml(
                    QString("<table>%1</table><br><b>Devices found: %2</b>")
                    .arg(rows)
                    .arg(found)
                );
            }
            progress->setValue(i + 1); // <-- Ensure last value sets to 100%
            QCoreApplication::processEvents();
        }

        // Ensure progress bar is 100% at the end
        progress->setValue(progress->maximum());

        // Final summary
        if (found == 0) {
            output->setHtml("<b>No devices found.</b>");
        } else {
            output->setHtml(
                QString("<table>%1</table><br><b>Scan complete. %2 device%3 found.</b>")
                .arg(rows)
                .arg(found)
                .arg(found == 1 ? "" : "s")
            );
        }
        scanBtn->setEnabled(true);
        stopBtn->setEnabled(false);
    });

    QObject::connect(stopBtn, &QPushButton::clicked, [&]() {
        isScanning = false;
        scanBtn->setEnabled(true);
        stopBtn->setEnabled(false);
    });
    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    stopBtn->setEnabled(false);
    dlg.adjustSize();
    dlg.exec();
}

void updateIpDisplay(QTextEdit *infoBox) {
    QString ipAddress, subnetMask, adapterName, macAddress, defaultGateway, externalIp;

    // Fetch the first active IPv4 interface (even if APIPA)
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    bool found = false;
    for (const QNetworkInterface &iface : interfaces) {
        if (!(iface.flags() & QNetworkInterface::IsUp) ||
            !(iface.flags() & QNetworkInterface::IsRunning) ||
            (iface.flags() & QNetworkInterface::IsLoopBack))
            continue;
        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isLoopback()) {
                ipAddress = entry.ip().toString();
                subnetMask = entry.netmask().toString();
                adapterName = iface.humanReadableName();
                macAddress = iface.hardwareAddress();
                found = true;
                break;
            }
        }
        if (found) break;
    }

    // If no IPv4 address at all, set Unavailable
    if (!found) {
        ipAddress = "Unavailable";
        subnetMask = "Unavailable";
        adapterName = "";
        defaultGateway = "Unavailable";
        externalIp = "Unavailable";
    } else {
        // Always update default gateway after IP change
        defaultGateway = getDefaultGateway(ipAddress);

        // Always update external IP after IP change
        QNetworkAccessManager manager;
        QNetworkRequest request(QUrl("https://api.ipify.org"));
        QNetworkReply *reply = manager.get(request);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(3000);
        loop.exec();
        if (reply->isFinished() && reply->error() == QNetworkReply::NoError) {
            externalIp = reply->readAll();
            if (externalIp.trimmed().isEmpty())
                externalIp = "Unavailable";
        } else if (timer.isActive()) {
            externalIp = "No Internet";
        } else {
            externalIp = "Timeout (no network?)";
        }
        reply->deleteLater();
    }

    // Check for APIPA (autoconfiguration) IP and set color
    QString ipColor = "";
    if (ipAddress.startsWith("169.254.")) {
        ipColor = "red";
    }

    // Update the HTML content for the infoBox
    QString basicInfoHtml =
    "<div align='center'>"
        "<table border='0' cellpadding='6' cellspacing='0'>"
        "<tr><th align='left' style='color:#9a1321;'>Property</th><th align='left' style='color:#9a1321;'>Value</th></tr>"
        "<tr><td><b>Adapter Name</b></td><td align='center'>" + adapterName + "</td></tr>"
        "<tr><td><b>MAC Address</b></td><td align='right'>" + macAddress + "</td></tr>"
        "<tr><td><b>IP Address</b></td><td align='right'>" +
            (ipColor.isEmpty() ? ipAddress : "<span style='color:" + ipColor + ";'>" + ipAddress + "</span>") +
        "</td></tr>"
        "<tr><td><b>Subnet Mask</b></td><td align='right'>" + subnetMask + "</td></tr>"
        "<tr><td><b>Default Gateway</b></td><td align='right'>" + defaultGateway + "</td></tr>"
        "<tr><td><b>External IP Address</b></td><td align='right'>" + externalIp + "</td></tr>"
        "</table>"
    "</div>";
    infoBox->setHtml(basicInfoHtml);
}

struct PortInfo {
    QString serviceName;
    QString description;
};

// Loads the port info from local CSV (downloads if missing/old), returns map: (port, proto) -> PortInfo
QMap<QPair<int, QString>, PortInfo> loadPortInfoCSV(QWidget *parent = nullptr) {
    QMap<QPair<int, QString>, PortInfo> portMap;
    QString csvPath = getPortInfoCsvPath();
    QFileInfo fi(csvPath);

    // Download if missing or older than 7 days
    bool needDownload = !fi.exists() || fi.lastModified().daysTo(QDateTime::currentDateTime()) > 7;
    if (needDownload) {
        downloadPortInfoCsv(csvPath, parent);
    }

    QFile file(csvPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (parent)
            QMessageBox::warning(parent, "Port Info", "Could not load port info from local file.");
        return portMap;
    }
    QTextStream in(&file);
    QString csvData = in.readAll();
    file.close();

    // Parse CSV
    QStringList lines = csvData.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
    if (lines.isEmpty()) return portMap;
    QStringList header = lines.takeFirst().split(',');
    int portIdx = header.indexOf("Port Number");
    int protoIdx = header.indexOf("Transport Protocol");
    int nameIdx = header.indexOf("Service Name");
    int descIdx = header.indexOf("Description");
    for (const QString &line : lines) {
        QStringList cols = line.split(',', Qt::KeepEmptyParts);
        if (cols.size() < qMax(qMax(portIdx, protoIdx), qMax(nameIdx, descIdx)) + 1)
            continue;
        bool ok = false;
        int port = cols[portIdx].toInt(&ok);
        if (!ok) continue;
        QString proto = cols[protoIdx].trimmed().toLower();
        QString name = cols[nameIdx].trimmed();
        QString desc = cols[descIdx].trimmed();
        if (proto.isEmpty()) continue;
        QPair<int, QString> key(port, proto);
        if (!portMap.contains(key)) {
            portMap[key] = PortInfo{name, desc};
        }
    }
    return portMap;
}

void showPortScanDialog(QWidget *parent) {
    QDialog *dlg = new QDialog(parent);
    dlg->setWindowTitle("Port Scan");
    // Remove the Close (X) button from the window frame
    dlg->setWindowFlags((dlg->windowFlags() & ~Qt::WindowCloseButtonHint) | Qt::Dialog | Qt::WindowTitleHint);

    QVBoxLayout *layout = new QVBoxLayout(dlg);

    QLabel *inputLabel = new QLabel("Target (IP or hostname):");
    QLineEdit *targetEdit = new QLineEdit("127.0.0.1");
    targetEdit->setToolTip("Enter the IP address or hostname to scan.");
    QLabel *rangeLabel = new QLabel("Port range (e.g. 1-1024):");
    QLineEdit *rangeEdit = new QLineEdit("1-1024");
    rangeEdit->setToolTip("<div style='white-space:nowrap;'>Enter the port range to scan. You can use a format like 1-1024.<BR>"
                           "The default is 1-1024, which is the most common range for services.<BR>"
                           "You can also specify a single port.</div>");
    layout->addWidget(inputLabel);
    layout->addWidget(targetEdit);
    layout->addWidget(rangeLabel);
    layout->addWidget(rangeEdit);

    QHBoxLayout *currentPortLayout = new QHBoxLayout();
    QLabel *checkingLabel = new QLabel("Checking port number:");
    QLineEdit *currentPortEdit = new QLineEdit;
    currentPortEdit->setToolTip("The port that is being checked.");
    currentPortEdit->setReadOnly(true);
    currentPortEdit->setAlignment(Qt::AlignCenter);
    QFont font = currentPortEdit->font();
    font.setPointSize(14);
    font.setBold(true);
    currentPortEdit->setFont(font);
    currentPortEdit->setFixedWidth(100);

    QLabel *progressLabel = new QLabel("0 % done");
    progressLabel->setToolTip("<div style='white-space:nowrap;'>Progress of the scan in percent.<BR>"
                              "This will update as ports are checked.</div>");
    progressLabel->setFixedWidth(120);
    progressLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    currentPortLayout->addWidget(checkingLabel);
    currentPortLayout->addWidget(currentPortEdit);
    currentPortLayout->addWidget(progressLabel);
    layout->addLayout(currentPortLayout);

    QLabel *etaLabel = new QLabel("Estimated time remaining: --");
    etaLabel->setToolTip("Estimated time remaining for your scan to complete.");
    etaLabel->setMinimumWidth(320);
    etaLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    etaLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    layout->addWidget(etaLabel);

    QTextEdit *output = new QTextEdit;
    output->setReadOnly(true);
    output->setLineWrapMode(QTextEdit::NoWrap);
    output->setMinimumHeight(120);
    layout->addWidget(output);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *scanBtn = new QPushButton("Scan");
    scanBtn->setToolTip("Start scanning the specified port range on the target.");
    QPushButton *stopCloseBtn = new QPushButton("Close");
    stopCloseBtn->setToolTip("Close the dialog.");
    btnLayout->addWidget(scanBtn);
    btnLayout->addWidget(stopCloseBtn);
    layout->addLayout(btnLayout);

    // State
    auto scanRunning = std::make_shared<bool>(false);

    // Helper to update Stop/Close button text
    auto updateStopCloseText = [&]() {
        if (*scanRunning) {
            stopCloseBtn->setText("Stop");
            stopCloseBtn->setToolTip("Stop current scan");
        } else {
            stopCloseBtn->setText("Close");
            stopCloseBtn->setToolTip("Close the dialog");
        }
    };
    updateStopCloseText();

    // --- Ctrl+W shortcut: only close if not scanning ---
    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, [=]() {
        if (!*scanRunning) dlg->accept();
    });

    // Example usage in your scan logic:
    QObject::connect(scanBtn, &QPushButton::clicked, [=, &updateStopCloseText]() mutable {
        *scanRunning = true;
        updateStopCloseText();
        // ... start scan ...
    });

    // Load port info once per scan
    static QMap<QPair<int, QString>, PortInfo> portInfoMap;

    QTimer *timer = new QTimer(dlg);
    timer->setSingleShot(true);

    QObject::connect(scanBtn, &QPushButton::clicked, [=]() mutable {
        QString target = targetEdit->text().trimmed();
        QString range = rangeEdit->text().trimmed();
        QRegularExpression re(R"((\d+)\s*-\s*(\d+))");
        QRegularExpressionMatch m = re.match(range);
        int startPort = 1, endPort = 1024;
        if (m.hasMatch()) {
            startPort = m.captured(1).toInt();
            endPort = m.captured(2).toInt();
        } else {
            QMessageBox::warning(dlg, "Input Error", "Invalid port range.");
            return;
        }
        if (startPort < 1 || endPort > 65535 || startPort > endPort) {
            QMessageBox::warning(dlg, "Input Error", "Port range must be 1-65535 and start <= end.");
            return;
        }

        // Load port info (only once per run)
        if (portInfoMap.isEmpty()) {
            portInfoMap = loadPortInfoCSV(dlg);
        }

        scanBtn->setEnabled(false);
        *scanRunning = true;
        updateStopCloseText();
        output->clear();
        progressLabel->setText("0 % done");
        etaLabel->setText("Estimated time remaining: --");
        currentPortEdit->clear();

        int totalCount = endPort - startPort + 1;
        auto checkedCount = std::make_shared<int>(0);
        auto openCount = std::make_shared<int>(0);
        auto startTime = std::make_shared<QElapsedTimer>();
        startTime->start();
        auto nextPort = std::make_shared<int>(startPort);

        // Disconnect any previous connections to avoid multiple triggers
        QObject::disconnect(timer, nullptr, nullptr, nullptr);

        std::function<void()> scanNextPort;
        scanNextPort = [=]() mutable {
            if (!*scanRunning || *nextPort > endPort) {
                scanBtn->setEnabled(true);
                *scanRunning = false;
                updateStopCloseText();
                progressLabel->setText("100 % done");
                etaLabel->setText("Estimated time remaining: 0 minutes and 0 seconds");
                currentPortEdit->clear();
                output->append(QString("<br><b>Scan complete. %1 open port%2 found.</b>")
                    .arg(*openCount)
                    .arg(*openCount == 1 ? "" : "s"));
                QObject::disconnect(timer, nullptr, nullptr, nullptr); // Disconnect after scan
                return;
            }
            int port = *nextPort;
            (*nextPort)++;
            currentPortEdit->setText(QString::number(port));
            QTcpSocket *sock = new QTcpSocket(dlg);
            sock->connectToHost(target, port);
            QTimer::singleShot(200, sock, [=]() {
                bool connected = (sock->state() == QAbstractSocket::ConnectedState);
                if (connected) {
                    (*openCount)++;
                    // Lookup port info
                    PortInfo info = portInfoMap.value(qMakePair(port, QString("tcp")));
                    QString service = info.serviceName.isEmpty() ? "Unknown" : info.serviceName;
                    QString desc = info.description.isEmpty() ? "No description" : info.description;
                    output->append(
                        QString("<span style='color:blue; font-weight:bold;'>%1</span> "
                                "<span style='color:limegreen; font-weight:bold;'>OPEN &#x1F389;</span> "
                                "<span style='color:gray;'>&nbsp;%2</span> "
                                "<span style='color:#888;'>&nbsp;%3</span>")
                        .arg(port)
                        .arg(service.toHtmlEscaped())
                        .arg(desc.toHtmlEscaped())
                    );
                }
                (*checkedCount)++;
                int percent = int((double(*checkedCount) * 100.0) / totalCount);
                progressLabel->setText(QString("%1 % done").arg(percent));
                // ETA calculation
                qint64 elapsedMs = startTime->elapsed();
                if (*checkedCount > 0 && percent < 100) {
                    double avgMsPerPort = double(elapsedMs) / *checkedCount;
                    int portsLeft = totalCount - *checkedCount;
                    int msLeft = int(avgMsPerPort * portsLeft);
                    int secLeft = msLeft / 1000;
                    int minLeft = secLeft / 60;
                    secLeft = secLeft % 60;
                    etaLabel->setText(QString("Estimated time remaining: %1 minute%2 and %3 second%4")
                        .arg(minLeft)
                        .arg(minLeft == 1 ? "" : "s")
                        .arg(secLeft)
                        .arg(secLeft == 1 ? "" : "s"));
                } else if (percent >= 100) {
                    etaLabel->setText("Estimated time remaining: 0 minutes and 0 seconds");
                }
                sock->abort();
                sock->deleteLater();
                if (*scanRunning)
                    timer->start(1); // Schedule next port
            });
        };

        QObject::connect(timer, &QTimer::timeout, scanNextPort);

        // Start scanning
        scanNextPort();
    });

    QObject::connect(stopCloseBtn, &QPushButton::clicked, [=, &updateStopCloseText]() mutable {
        if (*scanRunning) {
            // Stop the scan, but do NOT close the dialog
            *scanRunning = false;
            scanBtn->setEnabled(true);
            progressLabel->setText("0 % done");
            etaLabel->setText("Estimated time remaining: --");
            output->append("<b>Scan stopped.</b>");
            currentPortEdit->clear();
            updateStopCloseText();
            QObject::disconnect(timer, nullptr, nullptr, nullptr); // Disconnect after stop
            return; // <--- This prevents the dialog from closing!
        }
        // Only close the dialog if not scanning
        dlg->close();
    });

    dlg->adjustSize();
    dlg->exec();
    dlg->deleteLater();
}


void showTracerouteDialog(QWidget *parent) {
    QDialog dlg(parent);
    dlg.setWindowTitle("Traceroute Host");
    // Remove the Close (X) button from the window frame
    dlg.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLabel *prompt = new QLabel("Enter host or IP to trace:");
    layout->addWidget(prompt);

    QLineEdit *input = new QLineEdit;
    input->setPlaceholderText("e.g. 8.8.8.8 or www.google.com");
    layout->addWidget(input);

    QHBoxLayout *hopsLayout = new QHBoxLayout();
    QLabel *hopsLabel = new QLabel("Max hops:");
    QSpinBox *hopsSpin = new QSpinBox;
    hopsSpin->setToolTip("Maximum number of hops to trace. Default is 30.");
    hopsSpin->setRange(1, 64);
    hopsSpin->setValue(30);
    hopsLayout->addWidget(hopsLabel);
    hopsLayout->addWidget(hopsSpin);
    layout->addLayout(hopsLayout);

    QTextEdit *output = new QTextEdit;
    output->setReadOnly(true);
    output->setLineWrapMode(QTextEdit::NoWrap);
    output->setMinimumHeight(80);
    layout->addWidget(output);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *traceBtn = new QPushButton("Start");
    traceBtn->setToolTip("Start the traceroute to the specified host.");
    QPushButton *stopCloseBtn = new QPushButton("Close");
    stopCloseBtn->setToolTip("Stop the traceroute or close the dialog.");
    QPushButton *bottomBtn = new QPushButton("Bottom");
    bottomBtn->setToolTip("Scroll to the bottom of the output.");
    btnLayout->addWidget(traceBtn);
    btnLayout->addWidget(stopCloseBtn);
    btnLayout->addWidget(bottomBtn);
    layout->addLayout(btnLayout);

    // State
    bool isTracing = false;
    QPointer<QProcess> lastProc = nullptr;
    QPointer<QDialog> scanningDlg = nullptr;

    // Helper to update Stop/Close button text
    auto updateStopCloseText = [&]() {
        if (isTracing) {
            stopCloseBtn->setText("Stop");
            stopCloseBtn->setToolTip("Stop the traceroute");
        } else {
            stopCloseBtn->setText("Close");
            stopCloseBtn->setToolTip("Close the dialog");
        }
    };

    // --- Ctrl+W shortcut: only close if not tracing ---
    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), &dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, [&]() {
        if (!isTracing) dlg.accept();
    });

    QObject::connect(traceBtn, &QPushButton::clicked, [&]() {
        QString host = input->text().trimmed();
        int hops = hopsSpin->value();
        if (host.isEmpty()) {
            output->setPlainText("Please enter a host or IP address.");
            return;
        }
        traceBtn->setEnabled(false);
        isTracing = true;
        updateStopCloseText();
        output->clear();

        if (lastProc) {
            lastProc->kill();
            lastProc->deleteLater();
            lastProc = nullptr;
        }

        // Show scanning dialog
        if (!scanningDlg) {
            scanningDlg = new QDialog(&dlg);
            scanningDlg->setWindowTitle("Scanning...");
            scanningDlg->setFixedSize(150, 150);
            scanningDlg->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint);

            QVBoxLayout *vbox = new QVBoxLayout(scanningDlg);
            QLabel *label = new QLabel("Scanning...");
            vbox->addWidget(label, 0, Qt::AlignHCenter);

            QLabel *animLabel = new QLabel;
            animLabel->setFixedSize(96, 96);
            animLabel->setScaledContents(true);
            QMovie *movie = new QMovie("StdWorking.gif"); // <-- Set your GIF path/resource here
            animLabel->setMovie(movie);
            vbox->addWidget(animLabel, 0, Qt::AlignHCenter);
            movie->start();

            scanningDlg->setModal(false);
        }
        scanningDlg->show();
        scanningDlg->raise();
        scanningDlg->activateWindow();
        QCoreApplication::processEvents();

        QProcess *traceProc = new QProcess(&dlg);
        lastProc = traceProc;

        auto hopCount = std::make_shared<int>(0);
        auto timer = std::make_shared<QElapsedTimer>();
        timer->start();

        QObject::connect(traceProc, &QProcess::readyReadStandardOutput, [=, &dlg]() {
            while (traceProc->canReadLine()) {
                QString line = QString::fromLocal8Bit(traceProc->readLine()).trimmed();
                if (line.isEmpty()) continue;
                // Print line immediately
                output->append(line);

                // Count hops: lines that start with a number and a space (not headers)
                QRegularExpression hopNumRe(R"(^\s*(\d+)\s+)");
                QRegularExpressionMatch numMatch = hopNumRe.match(line);
                if (numMatch.hasMatch()) {
                    (*hopCount)++;
                }

                // Optionally resize window for long lines
                QFontMetrics fm(output->font());
                int margin = 80;
                int minWidth = 400;
                int width = fm.horizontalAdvance(line);
                int newWidth = qMax(minWidth, width + margin);
                if (dlg.width() < newWidth)
                    dlg.resize(newWidth, dlg.height());
            }
        });

        QObject::connect(traceProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=, &isTracing, &updateStopCloseText]() {
            // Show summary
            qint64 ms = timer->elapsed();
            int seconds = static_cast<int>((ms + 500) / 1000); // round to nearest second
            QString summary = QString("<B>Trace completed in %1 seconds over %2 hops.</B>")
                .arg(seconds)
                .arg(*hopCount);

            // Remove "Trace complete." if present and append summary
            QString text = output->toPlainText();
            QStringList lines = text.split('\n');
            if (!lines.isEmpty() && lines.last().trimmed().toLower().startsWith("trace complete")) {
                lines.removeLast();
                output->clear();
                output->append(lines.join("\n"));
            }
            output->append(summary);

            traceBtn->setEnabled(true);
            isTracing = false;
            updateStopCloseText();
            traceProc->deleteLater();
            if (scanningDlg) scanningDlg->close();
        });

        QObject::connect(scanningDlg, &QDialog::rejected, [=]() {
            scanningDlg->hide();
        });

        QString cmd = QString("tracert -h %1 %2").arg(hops).arg(host);
        traceProc->start("cmd", QStringList() << "/c" << cmd);
    });

    QObject::connect(stopCloseBtn, &QPushButton::clicked, [&]() {
        if (isTracing && lastProc) {
            lastProc->kill();
            lastProc->deleteLater();
            lastProc = nullptr;
            isTracing = false;
            traceBtn->setEnabled(true);
            updateStopCloseText();
            if (scanningDlg) scanningDlg->close();
            output->append("<b>Traceroute stopped.</b>");
        } else {
            dlg.accept();
        }
    });

    QObject::connect(bottomBtn, &QPushButton::clicked, [&]() {
        output->verticalScrollBar()->setValue(output->verticalScrollBar()->maximum());
    });

    dlg.adjustSize();
    dlg.exec();
}

void showDhcpStatusDialog(QWidget *parent = nullptr) {
    QProcess proc;
    proc.start("ipconfig", QStringList() << "/all");
    proc.waitForFinished();
    QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());

    QStringList blocks = output.split(QRegularExpression(R"(\r?\n\r?\n)"), Qt::SkipEmptyParts);

    QString info;
    for (const QString &block : blocks) {
        if (block.contains("DHCP Enabled", Qt::CaseInsensitive) &&
            block.contains(": Yes", Qt::CaseInsensitive) &&
            block.contains("IPv4 Address", Qt::CaseInsensitive)) {

            QStringList lines = block.split('\n', Qt::SkipEmptyParts);
            QString ifaceName = "Unknown";
            for (const QString &line : lines) {
                if (line.contains(':')) {
                    ifaceName = line.section(':', 0, 0).trimmed();
                    break;
                }
            }

            auto extract = [&](const QString &label) -> QString {
                QRegularExpression re(label + R"([\s\.]*:[ \t]*([^\r\n]+))");
                QRegularExpressionMatch m = re.match(block);
                return m.hasMatch() ? m.captured(1).trimmed() : "N/A";
            };

            QString dhcpServer    = extract("DHCP Server");
            QString leaseObtained = extract("Lease Obtained");
            QString leaseExpires  = extract("Lease Expires");
            QString clientId      = extract("DHCPv4 Client DUID");

            info = QString(
                "<table cellpadding='3'>"
                "<tr><td><b>Interface:</b></td><td>%1</td></tr>"
                "<tr><td><b>DHCP Enabled:</b></td><td>Yes</td></tr>"
                "<tr><td><b>DHCP Server:</b></td><td>%2</td></tr>"
                "<tr><td><b>Lease Obtained:</b></td><td>%3</td></tr>"
                "<tr><td><b>Lease Expires:</b></td><td>%4</td></tr>"
                "<tr><td><b>DHCP Client ID:</b></td><td>%5</td></tr>"
                "</table>"
            ).arg(ifaceName, dhcpServer, leaseObtained, leaseExpires, clientId);

            break;
        }
    }

    if (info.isEmpty())
        info = "No active DHCP-enabled interface found.";

    QDialog dlg(parent);
    dlg.setWindowTitle("DHCP Status");
    addCtrlWClose(&dlg);
  
    QVBoxLayout layout(&dlg);

    QLabel label(info);
    label.setTextFormat(Qt::RichText);
    label.setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout.addWidget(&label);

    QPushButton closeBtn("Close");
    QObject::connect(&closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout.addWidget(&closeBtn);

    QLabel *warnLabel = new QLabel("<b><span style='color:gray; font-size:7pt;'>"
                                "If special characters look wrong, your system<BR>encoding may not match the console output."
                                "</span></b>");
    warnLabel->setAlignment(Qt::AlignCenter);
    layout.addWidget(warnLabel);

    dlg.adjustSize(); // Make the dialog fit its content

    dlg.exec();
}

void showNslookupDialog(QWidget *parent) {
    // Custom input dialog with tooltip and custom button text
    QDialog inputDlg(parent);
    inputDlg.setWindowTitle("NS Lookup");
    QVBoxLayout vbox(&inputDlg);

    QLabel prompt("Enter hostname or IP:");
    vbox.addWidget(&prompt);

    QLineEdit inputEdit;
    inputEdit.setPlaceholderText("e.g. www.google.com or 8.8.8.8");
    inputEdit.setToolTip("Enter a hostname (like www.google.com) or an IP address to look up.");
    vbox.addWidget(&inputEdit);

    QHBoxLayout btnBox;
    QPushButton okBtn("Look it up");
    okBtn.setToolTip("Start the DNS lookup for the entered hostname or IP.");
    QPushButton cancelBtn("Close");
    cancelBtn.setToolTip("Close this dialog.");
    btnBox.addWidget(&okBtn);
    btnBox.addWidget(&cancelBtn);
    vbox.addLayout(&btnBox);

    okBtn.setEnabled(false);

    QObject::connect(&inputEdit, &QLineEdit::textChanged, [&]() {
        okBtn.setEnabled(!inputEdit.text().trimmed().isEmpty());
    });
    QObject::connect(&okBtn, &QPushButton::clicked, &inputDlg, &QDialog::accept);
    QObject::connect(&cancelBtn, &QPushButton::clicked, &inputDlg, &QDialog::reject);

    // --- Ctrl+W shortcut to close input dialog ---
    QShortcut inputCloseShortcut(QKeySequence("Ctrl+W"), &inputDlg);
    inputCloseShortcut.setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(&inputCloseShortcut, &QShortcut::activated, &inputDlg, &QDialog::reject);

    if (inputDlg.exec() != QDialog::Accepted)
        return;

    QString host = inputEdit.text().trimmed();

    QProcess proc;
    proc.start("nslookup", QStringList() << host);
    proc.waitForFinished();
    QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());

    QString server, serverAddr, name;
    QStringList ipv4List, ipv6List;
    bool inAnswerSection = false;
    bool inAddresses = false;

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    QString pendingIPv6;

    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();

        // DNS server info (always at the top)
        if (line.startsWith("Server:", Qt::CaseInsensitive)) {
            server = line.section(':', 1).trimmed();
            inAnswerSection = false;
            inAddresses = false;
            continue;
        } else if (line.startsWith("Address:", Qt::CaseInsensitive) && serverAddr.isEmpty()) {
            serverAddr = line.section(':', 1).trimmed();
            inAnswerSection = false;
            inAddresses = false;
            continue;
        }

        // Start of answer section
        if (line.startsWith("Name:", Qt::CaseInsensitive)) {
            name = line.section(':', 1).trimmed();
            inAnswerSection = true;
            inAddresses = false;
            continue;
        }

        if (inAnswerSection) {
            // Addresses: (plural)
            if (line.startsWith("Addresses:", Qt::CaseInsensitive)) {
                inAddresses = true;
                // First address may be on same line
                QString addr = line.section(':', 1).trimmed();
                if (!addr.isEmpty()) {
                    if (addr.contains('.'))
                        ipv4List << addr;
                    else
                        ipv6List << addr;
                }
                continue;
            }
            // Address: (singular)
            else if (line.startsWith("Address:", Qt::CaseInsensitive)) {
                QString addr = line.section(':', 1).trimmed();
                if (!addr.isEmpty()) {
                    if (addr.contains('.'))
                        ipv4List << addr;
                    else
                        ipv6List << addr;
                }
                inAddresses = false;
                continue;
            }
        }

        // Collect addresses, handling split IPv6 lines
        if (inAddresses) {
            // Stop if we hit a new key (Name:/Server:/Address:) at the start of a line
            if (line.startsWith("Name:", Qt::CaseInsensitive) ||
                line.startsWith("Server:", Qt::CaseInsensitive) ||
                line.startsWith("Address:", Qt::CaseInsensitive) ||
                line.startsWith("Addresses:", Qt::CaseInsensitive)) {
                inAddresses = false;
                continue;
            }
            if (!line.isEmpty()) {
                // If previous line ended with ":", join with this line (split IPv6)
                if (!pendingIPv6.isEmpty()) {
                    QString combined = pendingIPv6 + line;
                    if (combined.contains('.'))
                        ipv4List << combined;
                    else
                        ipv6List << combined;
                    pendingIPv6.clear();
                } else if (line.endsWith(":")) {
                    pendingIPv6 = line;
                } else {
                    if (line.contains('.'))
                        ipv4List << line;
                    else
                        ipv6List << line;
                }
            }
        }
    }
    // If there is a pending IPv6 part at the end, ignore or add as is
    if (!pendingIPv6.isEmpty()) {
        if (pendingIPv6.contains('.'))
            ipv4List << pendingIPv6;
        else
            ipv6List << pendingIPv6;
    }

    // Compose output in monospace, perfectly aligned style with bold labels
    QString info = "<pre style='font-family:monospace'>";
    if (!server.isEmpty())
        info += QString("<b>Server:</b>    \t%1\n").arg(server);
    if (!serverAddr.isEmpty())
        info += QString("<b>Address:</b>   \t%1\n").arg(serverAddr);
    if (!name.isEmpty())
        info += QString("<b>Name:</b>      \t%1\n").arg(name);

    if (!ipv4List.isEmpty() || !ipv6List.isEmpty()) {
        info += "<b>Addresses:</b>  \t";
        if (!ipv4List.isEmpty()) {
            info += ipv4List.first() + "\n";
            for (int i = 1; i < ipv4List.size(); ++i)
                info += "             \t" + ipv4List[i] + "\n";
        } else if (!ipv6List.isEmpty()) {
            info += ipv6List.first() + "\n";
        }
        int ipv6Start = ipv4List.isEmpty() ? 1 : 0;
        for (int i = ipv6Start; i < ipv6List.size(); ++i)
            info += "             \t" + ipv6List[i] + "\n";
    }
    info += "</pre>";

    // Create dialog with NO parent to ensure no X button
    QDialog dlg(nullptr);
    dlg.setWindowTitle("NS Lookup Result");
    dlg.setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    dlg.setModal(true);

    QVBoxLayout layout(&dlg);

    QLabel *label = new QLabel(info);
    label->setTextFormat(Qt::RichText);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout.addWidget(label);

    QPushButton *closeBtn = new QPushButton("Close");
    closeBtn->setToolTip("Close this dialog");
    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout.addWidget(closeBtn);

    // --- Ctrl+W shortcut to close dialog ---
    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), &dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, &dlg, &QDialog::accept);

    dlg.adjustSize();
    dlg.exec();
}

void showArpDialog(QWidget *parent = nullptr) {
    QDialog dlg(parent);
    dlg.setWindowTitle("ARP Table");
    addCtrlWClose(&dlg);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    // --- Remove arp entries section (moved up) ---
    QHBoxLayout *delLayout = new QHBoxLayout();
    QLabel *removeLabel = new QLabel("Remove arp entries:");
    QLineEdit *ipEdit = new QLineEdit("*");
    ipEdit->setPlaceholderText("IP to delete (e.g. 192.168.1.1 or * for all)");
    QPushButton *delBtn = new QPushButton("Delete");
    delLayout->addWidget(removeLabel);
    delLayout->addWidget(ipEdit);
    delLayout->addWidget(delBtn);
    layout->addLayout(delLayout);

    // Tooltip, two lines
    ipEdit->setToolTip("Insert IP from the ARP table<br>or leave as it is to delete all ARP entries");

    // --- Table ---
    QTableWidget *table = new QTableWidget();
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels(QStringList() << "Internet Address" << "Physical Address" << "Type");
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    table->setMaximumHeight(600);
    layout->addWidget(table);

    // --- Advanced/Refresh/Close buttons centered at bottom ---
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton *advBtn = new QPushButton("Advanced");
    QPushButton *refreshBtn = new QPushButton("Refresh");
    QPushButton *closeBtn = new QPushButton("Close");
    btnLayout->addWidget(advBtn);
    btnLayout->addWidget(refreshBtn);
    btnLayout->addWidget(closeBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    // Adding tooltips
    advBtn->setToolTip("Toggle between advanced and basic ARP table views");
    refreshBtn->setToolTip("Refresh the ARP table");
    closeBtn->setToolTip("Close the ARP table");

    // --- Helper to fill table from arp output and resize dialog ---
    auto adjustWidths = [&]() {
        table->resizeColumnsToContents();
        QCoreApplication::processEvents(); // Ensure columns are resized

        int totalWidth = table->verticalHeader()->width();
        for (int i = 0; i < table->columnCount(); ++i)
            totalWidth += table->columnWidth(i);
        totalWidth += table->frameWidth() * 2;
        if (table->verticalScrollBar()->isVisible())
            totalWidth += table->verticalScrollBar()->width();
        totalWidth += 18; // 8 + 10 px extra for comfort

        table->setMinimumWidth(totalWidth);
        table->setMaximumWidth(totalWidth);
        dlg.setFixedWidth(totalWidth + layout->contentsMargins().left() + layout->contentsMargins().right());
    };

    auto fillTable = [&](const QString &output) {
        table->setRowCount(0);
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            QString trimmed = line.trimmed();
            // Skip headers and interface lines
            if (trimmed.startsWith("Interface:") || trimmed.startsWith("Internet Address") || trimmed.isEmpty())
                continue;
            QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() == 3) {
                int row = table->rowCount();
                table->insertRow(row);
                for (int i = 0; i < 3; ++i)
                    table->setItem(row, i, new QTableWidgetItem(parts[i]));
            }
        }
        adjustWidths();
    };

    // --- Run arp -a initially ---
    bool advanced = false;
    QProcess proc;
    proc.start("arp", QStringList() << "-a");
    proc.waitForFinished();
    QString arpOutput = QString::fromLocal8Bit(proc.readAllStandardOutput());
    fillTable(arpOutput);

    // --- Delete button logic (with UAC elevation) ---
    QObject::connect(delBtn, &QPushButton::clicked, [&]() {
        QString ip = ipEdit->text().trimmed();
        if (ip.isEmpty())
            return;
        QString command = QString("Start-Process arp -ArgumentList '-d %1' -Verb runAs -WindowStyle Hidden").arg(ip);
        int result = QProcess::execute("powershell", QStringList() << "-WindowStyle" << "Hidden" << "-Command" << command);
        if (result == 0) {
            QMessageBox::information(&dlg, "ARP", "ARP entry deleted (or all entries deleted).");
        } else {
            QMessageBox::warning(&dlg, "ARP", "Failed to delete ARP entry. (You may need to accept the UAC prompt.)");
        }
        QProcess proc;
        proc.start("arp", QStringList() << (advanced ? "-av" : "-a"));
        proc.waitForFinished();
        fillTable(QString::fromLocal8Bit(proc.readAllStandardOutput()));
    });

    // --- Advanced/Basic toggle logic ---
    QObject::connect(advBtn, &QPushButton::clicked, [&]() {
        advanced = !advanced;
        advBtn->setText(advanced ? "Basic" : "Advanced");
        QProcess proc;
        proc.start("arp", QStringList() << (advanced ? "-av" : "-a"));
        proc.waitForFinished();
        fillTable(QString::fromLocal8Bit(proc.readAllStandardOutput()));
    });

    // --- Refresh button logic ---
    QObject::connect(refreshBtn, &QPushButton::clicked, [&]() {
        QProcess proc;
        proc.start("arp", QStringList() << (advanced ? "-av" : "-a"));
        proc.waitForFinished();
        fillTable(QString::fromLocal8Bit(proc.readAllStandardOutput()));
    });

    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    dlg.exec();
}

void showWifiScanDialog(QWidget *parent) {
    QDialog dlg(parent);
    dlg.setWindowTitle("WiFi Networks");
    addCtrlWClose(&dlg);
    dlg.setFixedWidth(650);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLabel *msgLabel = new QLabel;
    msgLabel->setWordWrap(true);
    msgLabel->setAlignment(Qt::AlignCenter);
    QFont msgFont = msgLabel->font();
    msgFont.setBold(false);
    msgFont.setPointSize(11);
    msgLabel->setFont(msgFont);
    msgLabel->setStyleSheet("color:#1e90ff;");
    msgLabel->setVisible(false);
    layout->addWidget(msgLabel);

    QLabel *info = new QLabel("Nearby WiFi networks (signal in dBm):");
    layout->addWidget(info);

    QTableWidget *table = new QTableWidget();
    table->setColumnCount(4);
    QStringList headers = {"SSID", "BSSID", "Signal (dBm)", "Channel"};
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectItems);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->setMinimumHeight(250);
    table->setMaximumWidth(630);
    table->setMinimumWidth(630);
    table->setFixedWidth(630);

    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    table->setColumnWidth(0, 210);
    table->setColumnWidth(1, 210);
    table->setColumnWidth(2, 110);
    table->setColumnWidth(3, 90);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    layout->addWidget(table);

    QPushButton *refreshBtn = new QPushButton("Refresh");
    QPushButton *closeBtn = new QPushButton("Close");
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(refreshBtn);
    btnLayout->addWidget(closeBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    // Sorting state
    int sortColumn = 2; // Default: Signal
    Qt::SortOrder sortOrder = Qt::DescendingOrder;

    // Helper to set header arrows
    auto updateHeaderArrows = [&]() {
        for (int i = 0; i < headers.size(); ++i) {
            QString label = headers[i];
            if (i == sortColumn) {
                label += (sortOrder == Qt::AscendingOrder) ? " ▲" : " ▼";
            }
            table->horizontalHeaderItem(i)->setText(label);
        }
    };

    auto showTableOrMsg = [&](bool showTable, const QString &msg = QString()) {
        table->setVisible(showTable);
        info->setVisible(showTable);
        msgLabel->setVisible(!showTable);
        if (!showTable) msgLabel->setText(msg);
    };

    // Store last scan results for sorting
    struct WifiEntry {
        QString ssid, bssid, signal, channel;
        int dbm;
    };
    QList<WifiEntry> entries;

    auto fillTable = [&]() {
        table->setRowCount(0);
        // Sort entries
        QList<WifiEntry> sorted = entries;
        std::function<bool(const WifiEntry&, const WifiEntry&)> cmp;
        switch (sortColumn) {
            case 0: // SSID
                cmp = [&](const WifiEntry &a, const WifiEntry &b) {
                    return sortOrder == Qt::AscendingOrder ? a.ssid < b.ssid : a.ssid > b.ssid;
                }; break;
            case 1: // BSSID
                cmp = [&](const WifiEntry &a, const WifiEntry &b) {
                    return sortOrder == Qt::AscendingOrder ? a.bssid < b.bssid : a.bssid > b.bssid;
                }; break;
            case 2: // Signal
                cmp = [&](const WifiEntry &a, const WifiEntry &b) {
                    return sortOrder == Qt::AscendingOrder ? a.dbm < b.dbm : a.dbm > b.dbm;
                }; break;
            case 3: // Channel
                cmp = [&](const WifiEntry &a, const WifiEntry &b) {
                    return sortOrder == Qt::AscendingOrder ? a.channel < b.channel : a.channel > b.channel;
                }; break;
            default: cmp = [](const WifiEntry&, const WifiEntry&) { return false; };
        }
        std::sort(sorted.begin(), sorted.end(), cmp);

        for (const WifiEntry &entry : sorted) {
            int row = table->rowCount();
            table->insertRow(row);

            QFont boldFont = table->font();
            boldFont.setBold(true);

            QFontMetrics fm(boldFont);
            QString elidedSSID = fm.elidedText(entry.ssid, Qt::ElideRight, table->columnWidth(0) - 8);
            QString elidedBSSID = fm.elidedText(entry.bssid, Qt::ElideRight, table->columnWidth(1) - 8);

            QTableWidgetItem *ssidItem = new QTableWidgetItem(elidedSSID);
            ssidItem->setFont(boldFont);
            ssidItem->setToolTip(entry.ssid);
            table->setItem(row, 0, ssidItem);

            QTableWidgetItem *bssidItem = new QTableWidgetItem(elidedBSSID);
            bssidItem->setFont(boldFont);
            bssidItem->setToolTip(entry.bssid);
            table->setItem(row, 1, bssidItem);

            QTableWidgetItem *signalItem = new QTableWidgetItem(entry.signal);
            signalItem->setFont(boldFont);
            table->setItem(row, 2, signalItem);

            QTableWidgetItem *channelItem = new QTableWidgetItem(entry.channel);
            table->setItem(row, 3, channelItem);
        }
        updateHeaderArrows();
    };

    auto scanWifi = [&]() {
        table->clearContents();
        table->setRowCount(0);
        entries.clear();
        showTableOrMsg(false, "Scanning, please wait...");

        HANDLE hClient = NULL;
        DWORD dwMaxClient = 2;
        DWORD dwCurVersion = 0;
        DWORD dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
        if (dwResult != ERROR_SUCCESS) {
            showTableOrMsg(false, "WLAN API not available.");
            return;
        }

        PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
        dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
        if (dwResult != ERROR_SUCCESS || !pIfList || pIfList->dwNumberOfItems == 0) {
            showTableOrMsg(false, "No WiFi interface found.");
            WlanCloseHandle(hClient, NULL);
            return;
        }

        for (unsigned int i = 0; i < pIfList->dwNumberOfItems; ++i) {
            PWLAN_INTERFACE_INFO pIfInfo = &pIfList->InterfaceInfo[i];
            WlanScan(hClient, &pIfInfo->InterfaceGuid, NULL, NULL, NULL);

            PWLAN_BSS_LIST pBssList = NULL;
            DWORD bssResult = WlanGetNetworkBssList(
                hClient, &pIfInfo->InterfaceGuid, NULL, dot11_BSS_type_any, FALSE, NULL, &pBssList);
            if (bssResult != ERROR_SUCCESS || !pBssList) continue;

            for (unsigned int j = 0; j < pBssList->dwNumberOfItems; ++j) {
                WLAN_BSS_ENTRY &bss = pBssList->wlanBssEntries[j];
                QString ssid = QString::fromUtf8(reinterpret_cast<const char*>(bss.dot11Ssid.ucSSID), bss.dot11Ssid.uSSIDLength);

                QString bssidStr = QString("%1:%2:%3:%4:%5:%6")
                    .arg(QString::number(bss.dot11Bssid[0], 16).rightJustified(2, '0'))
                    .arg(QString::number(bss.dot11Bssid[1], 16).rightJustified(2, '0'))
                    .arg(QString::number(bss.dot11Bssid[2], 16).rightJustified(2, '0'))
                    .arg(QString::number(bss.dot11Bssid[3], 16).rightJustified(2, '0'))
                    .arg(QString::number(bss.dot11Bssid[4], 16).rightJustified(2, '0'))
                    .arg(QString::number(bss.dot11Bssid[5], 16).rightJustified(2, '0'))
                    .toUpper();

                int dbm = int(bss.lRssi);
                QString signal = QString("%1 dBm").arg(dbm);

                int channel = 0;
                if (bss.ulChCenterFrequency > 0)
                    channel = int((bss.ulChCenterFrequency / 1000 - 2407) / 5);

                entries.append({ssid, bssidStr, signal, channel > 0 ? QString::number(channel) : "-" , dbm});
            }
            if (pBssList) WlanFreeMemory(pBssList);
        }
        if (pIfList) WlanFreeMemory(pIfList);
        WlanCloseHandle(hClient, NULL);

        if (!entries.isEmpty()) {
            showTableOrMsg(true);
            fillTable();
        } else {
            showTableOrMsg(false, "No WiFi networks found.");
        }
    };

    // Sorting: handle header clicks
    QObject::connect(table->horizontalHeader(), &QHeaderView::sectionClicked, [&](int col) {
        if (sortColumn == col) {
            sortOrder = (sortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
        } else {
            sortColumn = col;
            sortOrder = (col == 2) ? Qt::DescendingOrder : Qt::AscendingOrder; // Default: Signal desc, others asc
        }
        fillTable();
    });

    // Manual refresh
    QObject::connect(refreshBtn, &QPushButton::clicked, scanWifi);

    // Close
    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    // Auto-refresh: simulate refresh after 0.5s, then every 2s
    QTimer *autoTimer = new QTimer(&dlg);
    autoTimer->setInterval(2000);
    QObject::connect(autoTimer, &QTimer::timeout, scanWifi);

    QTimer::singleShot(500, [&]() {
        refreshBtn->click();
        autoTimer->start();
    });

    dlg.exec();
    autoTimer->stop();
}

void showNetUsageDialog(QWidget *parent) {
    QDialog dlg(parent);
    dlg.setWindowTitle("Network usage");
    addCtrlWClose(&dlg);
    dlg.setFixedSize(420, 170);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(8, 8, 8, 8);

    // --- Timers block as title ---
    QWidget *timersWidget = new QWidget;
    QVBoxLayout *timersLayout = new QVBoxLayout(timersWidget);
    timersLayout->setContentsMargins(0, 0, 0, 0);
    timersLayout->setSpacing(0);

    QLabel *usageLabel = new QLabel("Network usage");
    usageLabel->setAlignment(Qt::AlignCenter);
    usageLabel->setStyleSheet("font-weight:bold; font-size:12pt; padding-bottom:0px; margin-bottom:0px;");
    timersLayout->addWidget(usageLabel);

    timersWidget->setLayout(timersLayout);

    // Center the timers block
    QHBoxLayout *timersBlockLayout = new QHBoxLayout;
    timersBlockLayout->addStretch();
    timersBlockLayout->addWidget(timersWidget);
    timersBlockLayout->addStretch();
    layout->addLayout(timersBlockLayout);

    // Table: 2 rows, 3 columns (Received, Sent, Uptime)
    QTableWidget *table = new QTableWidget(2, 3);
    table->setVerticalHeaderLabels(QStringList() << "Total" << "Trip Counter");
    table->setHorizontalHeaderLabels(QStringList() << "Received" << "Sent" << "Uptime");
    QFont boldFont = table->horizontalHeader()->font();
    boldFont.setBold(true);
    table->horizontalHeader()->setFont(boldFont);
    table->verticalHeader()->setFont(boldFont);
    table->verticalHeader()->setToolTip("Total: Since interface started\nTrip: Since last reset");
    table->horizontalHeader()->setToolTip("Network data in bytes and formatted units");
    table->verticalHeader()->setVisible(true);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setFocusPolicy(Qt::NoFocus);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->setFixedHeight(80);
    table->setFixedWidth(400);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(table);

    // Buttons side by side
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *resetTripBtn = new QPushButton("Reset Trip Counter");
    resetTripBtn->setToolTip("Reset the trip counter to zero.\nUse this to measure network usage for a specific task.");
    QPushButton *closeBtn = new QPushButton("Close");
    closeBtn->setToolTip("Close this dialog");
    btnLayout->addStretch();
    btnLayout->addWidget(resetTripBtn);
    btnLayout->addWidget(closeBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    // Helper to format bytes
    auto formatBytes = [](quint64 bytes, int decimals = 2, int tooltipDecimals = 8) -> QPair<QString, QString> {
        double value = bytes;
        QString unit = "B";
        if (value >= 1024) { value /= 1024; unit = "KB"; }
        if (value >= 1024) { value /= 1024; unit = "MB"; }
        if (value >= 1024) { value /= 1024; unit = "GB"; }
        if (value >= 1024) { value /= 1024; unit = "TB"; }
        QString shown = QString::number(value, 'f', decimals) + " " + unit;
        QString tooltip = QString::number(value, 'f', tooltipDecimals) + " " + unit;
        return qMakePair(shown, tooltip);
    };

    // Store trip baseline values and trip start time
    static quint64 tripRxBase = 0, tripTxBase = 0;
    static QDateTime tripStartTime;
    static int tripDurationSecs = 0; // 0 = infinite
    static QTimer tripLimitTimer;
    static bool tripActive = true;
    tripRxBase = 0;
    tripTxBase = 0;
    tripStartTime = QDateTime::currentDateTime();
    tripDurationSecs = 0;
    tripActive = true;
    if (tripLimitTimer.isActive()) tripLimitTimer.stop();

    // Helper to fetch current totals
    auto getTotals = []() -> QPair<quint64, quint64> {
        QProcess proc;
        proc.start("netstat", QStringList() << "-e");
        proc.waitForFinished(1000);
        QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);

        quint64 rx = 0, tx = 0;
        for (const QString &line : lines) {
            if (line.trimmed().startsWith("Bytes")) {
                QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                if (parts.size() >= 3) {
                    rx = parts[1].toULongLong();
                    tx = parts[2].toULongLong();
                }
                break;
            }
        }
        return qMakePair(rx, tx);
    };

    // Helper to get the up time of the first non-loopback, up, running adapter (Windows only)
    auto getAdapterUptime = []() -> qint64 {
        ULONG outBufLen = 15000;
        IP_ADAPTER_ADDRESSES* addresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
        if (!addresses) return -1;
        DWORD dwRetVal = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, addresses, &outBufLen);
        qint64 seconds = -1;
        if (dwRetVal == NO_ERROR) {
            for (IP_ADAPTER_ADDRESSES* aa = addresses; aa; aa = aa->Next) {
                if (!(aa->OperStatus == IfOperStatusUp)) continue;
                if (aa->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;
                if (aa->TransmitLinkSpeed > 0) {
                    seconds = GetTickCount64() / 1000;
                    break;
                }
            }
        }
        free(addresses);
        return seconds;
    };

    // On dialog open, set trip baseline to current values and trip start time
    auto totals = getTotals();
    tripRxBase = totals.first;
    tripTxBase = totals.second;
    tripStartTime = QDateTime::currentDateTime();

    // Update stats
    auto updateStats = [&]() {
        auto totals = getTotals();
        quint64 rx = totals.first;
        quint64 tx = totals.second;

        // Total row
        auto rxFmt = formatBytes(rx);
        auto txFmt = formatBytes(tx);

        QBrush blueBrush(QColor("#1c2684")); // deep blue
        QBrush greenBrush(QColor("#1a7d2c")); // deep green
        QBrush redBrush(QColor("#c80000"));   // red for stopped trip

        QTableWidgetItem *rxItem = new QTableWidgetItem(rxFmt.first);
        QTableWidgetItem *txItem = new QTableWidgetItem(txFmt.first);
        rxItem->setForeground(blueBrush);
        txItem->setForeground(blueBrush);
        rxItem->setToolTip(QString("Total received since interface started\n%1 B\n%2").arg(rx).arg(rxFmt.second));
        txItem->setToolTip(QString("Total sent since interface started\n%1 B\n%2").arg(tx).arg(txFmt.second));
        rxItem->setTextAlignment(Qt::AlignCenter);
        txItem->setTextAlignment(Qt::AlignCenter);

        // Total uptime
        qint64 totalSecs = getAdapterUptime();
        QString totalUptimeStr = (totalSecs < 0)
            ? "Unknown"
            : QTime(0,0).addSecs(int(totalSecs)).toString("hh:mm:ss");
        QTableWidgetItem *totalUptimeItem = new QTableWidgetItem(totalUptimeStr);
        totalUptimeItem->setForeground(blueBrush);
        totalUptimeItem->setTextAlignment(Qt::AlignCenter);
        totalUptimeItem->setToolTip("Time since the network adapter was started");

        table->setItem(0, 0, rxItem);
        table->setItem(0, 1, txItem);
        table->setItem(0, 2, totalUptimeItem);

        // Trip row
        quint64 tripRx = rx >= tripRxBase ? rx - tripRxBase : 0;
        quint64 tripTx = tx >= tripTxBase ? tx - tripTxBase : 0;
        auto tripRxFmt = formatBytes(tripRx);
        auto tripTxFmt = formatBytes(tripTx);

        QTableWidgetItem *tripRxItem = new QTableWidgetItem(tripRxFmt.first);
        QTableWidgetItem *tripTxItem = new QTableWidgetItem(tripTxFmt.first);

        QBrush tripBrush = tripActive ? greenBrush : redBrush;
        tripRxItem->setForeground(tripBrush);
        tripTxItem->setForeground(tripBrush);
        tripRxItem->setToolTip(QString("Trip received since last reset\n%1 B\n%2").arg(tripRx).arg(tripRxFmt.second));
        tripTxItem->setToolTip(QString("Trip sent since last reset\n%1 B\n%2").arg(tripTx).arg(tripTxFmt.second));
        tripRxItem->setTextAlignment(Qt::AlignCenter);
        tripTxItem->setTextAlignment(Qt::AlignCenter);

        // Trip uptime
        qint64 tripSecs = tripStartTime.secsTo(QDateTime::currentDateTime());
        if (!tripActive && tripDurationSecs > 0) tripSecs = tripDurationSecs;
        QTableWidgetItem *tripUptimeItem = new QTableWidgetItem(QTime(0,0).addSecs(int(tripSecs)).toString("hh:mm:ss"));
        tripUptimeItem->setForeground(tripBrush);
        tripUptimeItem->setTextAlignment(Qt::AlignCenter);
        tripUptimeItem->setToolTip("Time since the trip counter was last reset");

        table->setItem(1, 0, tripRxItem);
        table->setItem(1, 1, tripTxItem);
        table->setItem(1, 2, tripUptimeItem);
    };

    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        if (tripActive) updateStats();
        else {
            // Only update the total row if trip is stopped
            auto totals = getTotals();
            quint64 rx = totals.first;
            quint64 tx = totals.second;
            auto rxFmt = formatBytes(rx);
            auto txFmt = formatBytes(tx);
            QBrush blueBrush(QColor("#1c2684"));
            QTableWidgetItem *rxItem = new QTableWidgetItem(rxFmt.first);
            QTableWidgetItem *txItem = new QTableWidgetItem(txFmt.first);
            rxItem->setForeground(blueBrush);
            txItem->setForeground(blueBrush);
            rxItem->setTextAlignment(Qt::AlignCenter);
            txItem->setTextAlignment(Qt::AlignCenter);
            rxItem->setToolTip(QString("Total received since interface started\n%1 B\n%2").arg(rx).arg(rxFmt.second));
            txItem->setToolTip(QString("Total sent since interface started\n%1 B\n%2").arg(tx).arg(txFmt.second));
            table->setItem(0, 0, rxItem);
            table->setItem(0, 1, txItem);

            // Uptime
            qint64 totalSecs = getAdapterUptime();
            QString totalUptimeStr = (totalSecs < 0)
                ? "Unknown"
                : QTime(0,0).addSecs(int(totalSecs)).toString("hh:mm:ss");
            QTableWidgetItem *totalUptimeItem = new QTableWidgetItem(totalUptimeStr);
            totalUptimeItem->setForeground(blueBrush);
            totalUptimeItem->setTextAlignment(Qt::AlignCenter);
            totalUptimeItem->setToolTip("Time since the network adapter was started");
            table->setItem(0, 2, totalUptimeItem);
        }
    });
    timer.start(1000);
    updateStats();

    QObject::connect(resetTripBtn, &QPushButton::clicked, [&]() {
    while (true) {
        bool ok = false;
        QString timeStr = QInputDialog::getText(
            &dlg,
            "Trip Timer",
            "Set trip duration (hh:mm:ss, 0 = unlimited):\n"
            "Examples: 1:00:00 = 1 hour, 0:30:00 = 30 min, 0:00:10 = 10 sec, 0 = unlimited",
            QLineEdit::Normal, "0", &ok
        );
        if (!ok) return;

        int newTripDurationSecs = 0;
        if (timeStr.trimmed() == "0") {
            newTripDurationSecs = 0;
        } else {
            QRegularExpression re(R"(^(\d{1,2}):(\d{1,2}):(\d{1,2})$)");
            QRegularExpressionMatch m = re.match(timeStr.trimmed());
            if (!m.hasMatch()) {
                QMessageBox::warning(&dlg, "Format Error",
                    "Please enter the time as hh:mm:ss (e.g. 1:00:00 for 1 hour, 0:30:00 for 30 minutes, 0:00:10 for 10 seconds, or 0 for unlimited).");
                continue; // Prompt again
            }
            int h = m.captured(1).toInt();
            int m_ = m.captured(2).toInt();
            int s = m.captured(3).toInt();
            if (m_ > 59 || s > 59) {
                QMessageBox::warning(&dlg, "Format Error",
                    "Minutes and seconds must be between 0 and 59.");
                continue; // Prompt again
            }
            newTripDurationSecs = h * 3600 + m_ * 60 + s;
            if (newTripDurationSecs == 0) {
                // Treat as unlimited
                newTripDurationSecs = 0;
            }
        }
        tripDurationSecs = newTripDurationSecs;

        auto totals = getTotals();
        tripRxBase = totals.first;
        tripTxBase = totals.second;
        tripStartTime = QDateTime::currentDateTime();
        tripActive = true;
        updateStats();

        if (tripLimitTimer.isActive()) tripLimitTimer.stop();
        if (tripDurationSecs > 0) {
            tripLimitTimer.setSingleShot(true);
            QObject::connect(&tripLimitTimer, &QTimer::timeout, [&]() {
                tripActive = false;
                updateStats(); // Freeze trip row and make it red
            });
            tripLimitTimer.start(tripDurationSecs * 1000);
        }
        break; // Only break if input was valid
    }
});

    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    dlg.exec();
    timer.stop();
    if (tripLimitTimer.isActive()) tripLimitTimer.stop();
}

void showNetworkAdaptersDialog(QWidget *parent) {
    QDialog dlg(parent);
    dlg.setWindowTitle("Network Adapters");
    addCtrlWClose(&dlg);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QTableWidget *table = new QTableWidget();
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels(QStringList()
        << "Adapter Name"
        << "Type"
        << "MAC Address"
        << "Flags"
        << "Description"
    );
    for (int i = 0; i < 5; ++i)
        table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);

    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    layout->addWidget(table);

    // Make header bold
    QFont headerFont = table->horizontalHeader()->font();
    headerFont.setBold(true);
    table->horizontalHeader()->setFont(headerFont);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *rescanBtn = new QPushButton("Rescan");
    rescanBtn->setToolTip("Manually rescan the network adapters");
    QPushButton *closeBtn = new QPushButton("Close");
    closeBtn->setToolTip("Close the dialog");   
    btnLayout->addStretch();
    btnLayout->addWidget(rescanBtn);
    btnLayout->addWidget(closeBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    // Simple type detection (no VPN logic)
    auto typeToString = [](const QNetworkInterface &iface, const QString &desc) {
        QString name = iface.humanReadableName().toLower();
        QString d = desc.toLower();
        if (name.contains("wi-fi") || name.contains("wifi") || name.contains("wlan"))
            return "Wi-Fi";
        if (name.contains("ethernet") || name.contains("lan"))
            return "Ethernet";
        if (name.contains("virtual") || d.contains("virtual"))
            return "Virtual";
        if (name.contains("ppp") || d.contains("ppp"))
            return "PPP";
        return "Unknown";
    };

    auto flagsToString = [](QFlags<QNetworkInterface::InterfaceFlag> flags) {
        QStringList list;
        if (flags & QNetworkInterface::IsUp) list << "Up";
        if (flags & QNetworkInterface::IsRunning) list << "Running";
        if (flags & QNetworkInterface::IsLoopBack) list << "Loopback";
        if (flags & QNetworkInterface::IsPointToPoint) list << "P2P";
        return list.join(", ");
    };

    // Windows API for description
    auto getDescription = [](const QString &ifaceName) -> QString {
#ifdef Q_OS_WIN
        ULONG outBufLen = 15000;
        IP_ADAPTER_ADDRESSES* addresses = (IP_ADAPTER_ADDRESSES*)malloc(outBufLen);
        if (!addresses) return "";
        QString desc;
        DWORD dwRetVal = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, addresses, &outBufLen);
        if (dwRetVal == NO_ERROR) {
            for (IP_ADAPTER_ADDRESSES* aa = addresses; aa; aa = aa->Next) {
                if (QString::fromWCharArray(aa->FriendlyName) == ifaceName) {
                    desc = QString::fromWCharArray(aa->Description);
                    break;
                }
            }
        }
        free(addresses);
        return desc;
#else
        Q_UNUSED(ifaceName);
        return "";
#endif
    };

    auto fillTable = [&]() {
        table->setRowCount(0);
        const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
        for (const QNetworkInterface &iface : interfaces) {
            QString name = iface.humanReadableName();
            QString lname = name.toLower();
            QString desc = getDescription(name);
            QString ldesc = desc.toLower();

            // Exclude loopback by flag or by name/description
            if ((iface.flags() & QNetworkInterface::IsLoopBack) ||
                lname.contains("loopback") || ldesc.contains("loopback"))
                continue;

            int row = table->rowCount();
            table->insertRow(row);
            QString mac = iface.hardwareAddress();

            // Set deep blue color for all text in the row
            QBrush blueBrush(QColor("#1c2684"));

            auto makeItem = [&](const QString &text) {
                QTableWidgetItem *item = new QTableWidgetItem(text);
                item->setForeground(blueBrush);
                return item;
            };

            table->setItem(row, 0, makeItem(name));
            table->setItem(row, 1, makeItem(typeToString(iface, desc)));
            table->setItem(row, 2, makeItem(mac));
            table->setItem(row, 3, makeItem(flagsToString(iface.flags())));
            QTableWidgetItem *descItem = makeItem(desc);
            descItem->setToolTip(desc); // Tooltip for full description
            table->setItem(row, 4, descItem);
        }
        table->resizeColumnsToContents();

        // Calculate total width needed for all columns
        int totalWidth = table->verticalHeader()->width();
        for (int i = 0; i < table->columnCount(); ++i)
            totalWidth += table->columnWidth(i);
        totalWidth += table->frameWidth() * 2;
        if (table->verticalScrollBar()->isVisible())
            totalWidth += table->verticalScrollBar()->width();
        totalWidth += 40; // Extra margin

        // Cap width to screen size or a max (e.g. 1920)
        int screenWidth = QApplication::primaryScreen()->availableGeometry().width();
        int maxWidth = qMin(1920, screenWidth - 80);
        totalWidth = qMin(totalWidth, maxWidth);

        dlg.resize(totalWidth, dlg.sizeHint().height());
    };

    fillTable();

    QObject::connect(rescanBtn, &QPushButton::clicked, [&]() {
        fillTable();

        // Show popup with 3s timeout and OK button
        QDialog popup(&dlg);
        popup.setWindowTitle("Refreshed!");
        QVBoxLayout vbox(&popup);
        QLabel label("<b>Adapter list refreshed!</b>");
        label.setAlignment(Qt::AlignCenter);
        vbox.addWidget(&label);

        QPushButton okBtn("OK");
        vbox.addWidget(&okBtn, 0, Qt::AlignCenter);

        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &popup, &QDialog::accept);
        QObject::connect(&okBtn, &QPushButton::clicked, &popup, &QDialog::accept);
        timer.start(3000);

        popup.setModal(true);
        popup.adjustSize();
        popup.exec();
    });

    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    fillTable(); // Ensure correct size on open
    dlg.exec();
}

void showSslCertificateDialog(QWidget *parent) {
    QDialog dlg(parent);
    dlg.setWindowTitle("SSL Certificate Check");
    dlg.setMinimumWidth(600);
    dlg.setMaximumWidth(950);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLabel *hostLabel = new QLabel("Host (e.g. www.google.com):");
    QLineEdit *hostEdit = new QLineEdit;
    hostEdit->setPlaceholderText("e.g. www.google.com");
    QLabel *portLabel = new QLabel("Port:");
    QLineEdit *portEdit = new QLineEdit("443");
    portEdit->setValidator(new QIntValidator(1, 65535, portEdit));
    layout->addWidget(hostLabel);
    layout->addWidget(hostEdit);
    layout->addWidget(portLabel);
    layout->addWidget(portEdit);

    QTextEdit *output = new QTextEdit;
    output->setReadOnly(true);
    output->setMinimumHeight(40);
    output->setMaximumHeight(80);
    output->setVisible(false);
    layout->addWidget(output);

    QScrollArea *scrollArea = new QScrollArea;
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(250);
    scrollArea->setMaximumHeight(250);
    scrollArea->setMinimumWidth(580);
    scrollArea->setMaximumWidth(930);

    QWidget *certsWidget = new QWidget;
    certsWidget->setMinimumWidth(560);
    certsWidget->setMaximumWidth(900);
    QVBoxLayout *certsLayout = new QVBoxLayout(certsWidget);
    certsLayout->setAlignment(Qt::AlignTop);
    scrollArea->setWidget(certsWidget);
    layout->addWidget(scrollArea);

    QPushButton *checkBtn = new QPushButton("Check");
    QPushButton *closeBtn = new QPushButton("Close");
    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(checkBtn);
    btnLayout->addWidget(closeBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), &dlg);
    QObject::connect(closeShortcut, &QShortcut::activated, &dlg, &QDialog::reject);
    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::reject);

    auto certInStore = [](const QByteArray &sha1, QString &storeOut) -> bool {
        QStringList stores = { "Root", "CA" };
        for (const QString &store : stores) {
            QProcess proc;
            proc.start("powershell", QStringList()
                << "-Command"
                << QString("Get-ChildItem -Path Cert:\\LocalMachine\\%1 | Where-Object { $_.Thumbprint -eq '%2' } | Select-Object -First 1 -ExpandProperty Thumbprint")
                    .arg(store, QString::fromLatin1(sha1.toHex().toUpper())));
            proc.waitForFinished(2000);
            QString thumb = proc.readAllStandardOutput().trimmed();
            if (!thumb.isEmpty()) {
                storeOut = store;
                return true;
            }
        }
        storeOut.clear();
        return false;
    };

    auto doCheck = [&]() {
        QString host = hostEdit->text().trimmed();
        int port = portEdit->text().toInt();
        if (host.isEmpty() || port < 1 || port > 65535) {
            output->setVisible(true);
            output->setPlainText("Please enter a valid host and port.");
            return;
        }
        output->clear();
        output->setVisible(false);

        // Remove previous cert widgets
        QLayoutItem *item;
        while ((item = certsLayout->takeAt(0)) != nullptr) {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }

        // --- Show scanning dialog with minimum display time ---
        QDialog workingDlg(&dlg);
        workingDlg.setWindowTitle("Querying, please wait.");
        workingDlg.setFixedSize(320, 180);
        workingDlg.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint);
        QVBoxLayout vbox(&workingDlg);
        QLabel label("Querying, please wait.");
        label.setAlignment(Qt::AlignCenter);
        vbox.addWidget(&label);
        QLabel animLabel;
        animLabel.setFixedSize(96, 96);
        animLabel.setScaledContents(true);
        QMovie movie("StdWorking.gif");
        animLabel.setMovie(&movie);
        vbox.addWidget(&animLabel, 0, Qt::AlignHCenter);
        movie.start();
        workingDlg.setModal(true);
        workingDlg.show();
        QCoreApplication::processEvents();
        QElapsedTimer timer;
        timer.start();

        // --- Network operation ---
        QSslSocket socket;
        socket.connectToHostEncrypted(host, port);
        bool ok = socket.waitForEncrypted(5000);

        int elapsed = int(timer.elapsed());
        if (elapsed < 300)
            QThread::msleep(300 - elapsed);

        workingDlg.accept();

        if (!ok) {
            output->setVisible(true);
            output->setPlainText("Could not connect or handshake failed:\n" + socket.errorString());
            return;
        }
        QList<QSslCertificate> certs = socket.peerCertificateChain();
        if (certs.isEmpty()) {
            output->setVisible(true);
            output->setPlainText("No certificate received.");
            return;
        }

        struct CertInfo {
            QSslCertificate cert;
            QByteArray sha1;
            QString store;
            bool inStore;
        };
        QList<CertInfo> certList;
        for (const QSslCertificate &cert : certs) {
            QByteArray sha1 = cert.digest(QCryptographicHash::Sha1);
            QString store;
            bool inStore = certInStore(sha1, store);
            certList.append(CertInfo{cert, sha1, store, inStore});
        }
        std::sort(certList.begin(), certList.end(), [](const CertInfo &a, const CertInfo &b) {
            return a.inStore > b.inStore;
        });

        int idx = 1;
        for (const CertInfo &ci : certList) {
            QWidget *certWidget = new QWidget;
            certWidget->setMinimumWidth(560);
            certWidget->setMaximumWidth(900);
            QVBoxLayout *certLayout = new QVBoxLayout(certWidget);
            certLayout->setContentsMargins(8, 8, 8, 8);

            // Use plain text for cert info
            QString info;
            info += QString("Certificate #%1\n").arg(idx++);
            info += QString("Subject:    %1\n").arg(ci.cert.subjectInfo(QSslCertificate::CommonName).join(", "));
            info += QString("Issuer:     %1\n").arg(ci.cert.issuerInfo(QSslCertificate::CommonName).join(", "));
            info += QString("Valid from: %1\n").arg(ci.cert.effectiveDate().toString());
            info += QString("Valid to:   %1\n").arg(ci.cert.expiryDate().toString());
            info += QString("Serial:     %1\n").arg(ci.cert.serialNumber());
            info += QString("SHA1:       %1\n").arg(QString::fromLatin1(ci.sha1.toHex()));
            if (ci.inStore)
                info += QString("Store:      [Found in Windows certificate store: %1]\n").arg(ci.store);
            else
                info += "Store:      [Not found in Windows certificate store]\n";

            QPlainTextEdit *infoEdit = new QPlainTextEdit(info);
            infoEdit->setReadOnly(true);
            infoEdit->setFrameStyle(QFrame::NoFrame);
            infoEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            infoEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            infoEdit->setMinimumWidth(540);
            infoEdit->setMaximumWidth(880);
            infoEdit->setFixedHeight(infoEdit->fontMetrics().height() * (info.count('\n') + 2));
            infoEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            certLayout->addWidget(infoEdit);

            if (ci.inStore) {
                QPushButton *removeBtn = new QPushButton("Remove");
                removeBtn->setProperty("sha1", QString::fromLatin1(ci.sha1.toHex().toUpper()));
                removeBtn->setProperty("store", ci.store);
                removeBtn->setProperty("host", host);
                QObject::connect(removeBtn, &QPushButton::clicked, [=, &dlg, &checkBtn]() {
                    QString sha1 = removeBtn->property("sha1").toString();
                    QString store = removeBtn->property("store").toString();
                    QString hostVal = removeBtn->property("host").toString();
                    QMessageBox msgBox(&dlg);
                    msgBox.setWindowTitle("Remove Certificate");
                    msgBox.setText(QString("Are you sure you want to remove the cert for [%1]?").arg(hostVal));
                    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
                    msgBox.setDefaultButton(QMessageBox::Cancel);
                    int ret = msgBox.exec();
                    if (ret == QMessageBox::Yes) {
                        QString psCmd = QString(
                            "Remove-Item -Path Cert:\\LocalMachine\\%1\\%2 -Force"
                        ).arg(store, sha1);
                        int result = QProcess::execute("powershell", QStringList() << "-Command" << psCmd);
                        if (result == 0) {
                            QMessageBox::information(&dlg, "Certificate Removed", "Certificate removed from store.");
                            checkBtn->click(); // Refresh
                        } else {
                            QMessageBox::warning(&dlg, "Failed", "Failed to remove certificate.");
                        }
                    }
                });
                certLayout->addWidget(removeBtn, 0, Qt::AlignLeft);

                QLabel *spacer = new QLabel;
                spacer->setFixedHeight(8);
                certLayout->addWidget(spacer);
            }

            certsLayout->addWidget(certWidget);
        }
        certsLayout->addStretch();
    };

    QObject::connect(checkBtn, &QPushButton::clicked, doCheck);
    QObject::connect(hostEdit, &QLineEdit::returnPressed, doCheck);
    QObject::connect(portEdit, &QLineEdit::returnPressed, doCheck);

    dlg.exec();
}

void showMtuDiscoveryDialog(QWidget *parent) {
    QDialog dlg(parent);
    dlg.setWindowTitle("MTU Discovery");
    addCtrlWClose(&dlg);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLabel *prompt = new QLabel("Find the optimal MTU for your connection (largest packet size without fragmentation):");
    layout->addWidget(prompt);

    // Host input
    QHBoxLayout *hostLayout = new QHBoxLayout();
    QLabel *hostLabel = new QLabel("Host:");
    QLineEdit *hostEdit = new QLineEdit("8.8.8.8");
    hostEdit->setToolTip("Enter the host to ping (default: 8.8.8.8)");
    hostLayout->addWidget(hostLabel);
    hostLayout->addWidget(hostEdit);
    layout->addLayout(hostLayout);

    // Range and step input
    QHBoxLayout *rangeLayout = new QHBoxLayout();
    QLabel *minLabel = new QLabel("Min size:");
    QSpinBox *minSpin = new QSpinBox;
    minSpin->setRange(12, 2000);
    minSpin->setValue(1200);
    QLabel *maxLabel = new QLabel("Max size:");
    QSpinBox *maxSpin = new QSpinBox;
    maxSpin->setRange(12, 2000);
    maxSpin->setValue(1500);
    QLabel *stepLabel = new QLabel("Step:");
    QSpinBox *stepSpin = new QSpinBox;
    stepSpin->setRange(1, 200);
    stepSpin->setValue(10);
    rangeLayout->addWidget(minLabel);
    rangeLayout->addWidget(minSpin);
    rangeLayout->addSpacing(10);
    rangeLayout->addWidget(maxLabel);
    rangeLayout->addWidget(maxSpin);
    rangeLayout->addSpacing(10);
    rangeLayout->addWidget(stepLabel);
    rangeLayout->addWidget(stepSpin);
    layout->addLayout(rangeLayout);

    // Output
    QTextEdit *output = new QTextEdit;
    output->setReadOnly(true);
    output->setMinimumHeight(120);
    layout->addWidget(output);

    // --- Interface and MTU setting controls ---
    QHBoxLayout *mtuLayout = new QHBoxLayout();
    QLabel *ifaceLabel = new QLabel("Interface:");
    QComboBox *ifaceCombo = new QComboBox;
    QLabel *mtuLabel = new QLabel("Set MTU:");
    QSpinBox *mtuSpin = new QSpinBox;
    mtuSpin->setRange(576, 2000);
    mtuSpin->setValue(1500);
    QPushButton *setMtuBtn = new QPushButton("Set MTU");
    mtuLayout->addWidget(ifaceLabel);
    mtuLayout->addWidget(ifaceCombo);
    mtuLayout->addSpacing(10);
    mtuLayout->addWidget(mtuLabel);
    mtuLayout->addWidget(mtuSpin);
    mtuLayout->addWidget(setMtuBtn);
    layout->addLayout(mtuLayout);

    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *startBtn = new QPushButton("Start Scan");
    QPushButton *stopBtn = new QPushButton("Stop");
    QPushButton *closeBtn = new QPushButton("Close");
    btnLayout->addWidget(startBtn);
    btnLayout->addWidget(stopBtn);
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    stopBtn->setEnabled(false);
    setMtuBtn->setEnabled(false);

    // State
    auto running = std::make_shared<bool>(false);
    auto timer = std::make_shared<QTimer>();
    timer->setSingleShot(true);
    auto currentSize = std::make_shared<int>(0);
    auto best = std::make_shared<int>(-1);
    auto step = std::make_shared<int>(1);
    auto tryNextPtr = std::make_shared<std::function<void()>>();
    auto procPtr = std::make_shared<QPointer<QProcess>>();

    // Populate interface combo
    auto updateInterfaces = [&]() {
        ifaceCombo->clear();
        QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
        for (const QNetworkInterface &iface : interfaces) {
            if (!(iface.flags() & QNetworkInterface::IsUp) ||
                !(iface.flags() & QNetworkInterface::IsRunning) ||
                (iface.flags() & QNetworkInterface::IsLoopBack))
                continue;
            for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isLoopback()) {
                    ifaceCombo->addItem(iface.humanReadableName());
                    break;
                }
            }
        }
    };
    updateInterfaces();

    // Helper to update UI
    auto setUiEnabled = [&](bool enabled) {
        hostEdit->setEnabled(enabled);
        minSpin->setEnabled(enabled);
        maxSpin->setEnabled(enabled);
        stepSpin->setEnabled(enabled);
        startBtn->setEnabled(enabled);
        stopBtn->setEnabled(!enabled);
        ifaceCombo->setEnabled(enabled);
        mtuSpin->setEnabled(enabled && *best > 0);
        setMtuBtn->setEnabled(enabled && *best > 0);
    };

    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    QObject::connect(stopBtn, &QPushButton::clicked, [&]() {
        *running = false;
        setUiEnabled(true);
        if (*procPtr) {
            (*procPtr)->kill();
            (*procPtr)->deleteLater();
            *procPtr = nullptr;
        }
        timer->stop();
    });

    QObject::connect(startBtn, &QPushButton::clicked, [&]() mutable {
        QObject::disconnect(timer.get(), &QTimer::timeout, nullptr, nullptr);

        QString host = hostEdit->text().trimmed();
        int minSize = minSpin->value();
        int maxSize = maxSpin->value();
        *step = stepSpin->value();
        if (host.isEmpty() || minSize > maxSize || *step <= 0) {
            QMessageBox::warning(&dlg, "Input Error", "Please enter a valid host, size range, and step.");
            return;
        }
        output->clear();
        setUiEnabled(false);
        *running = true;

        *currentSize = minSize;
        *best = -1;

        *tryNextPtr = [=]() mutable {
            if (!*running || *currentSize > maxSize) {
                setUiEnabled(true);
                if (*best > 0) {
                    output->append(QString("<b>Largest successful packet size (MTU): <span style='color:green;'>%1 bytes</span></b>").arg(*best));
                    mtuSpin->setValue(*best);
                } else {
                    output->append("<b style='color:red;'>No successful pings in range.</b>");
                }
                if (*procPtr) {
                    (*procPtr)->kill();
                    (*procPtr)->deleteLater();
                    *procPtr = nullptr;
                }
                return;
            }

            int size = *currentSize;
            QString pingMsg = QString("Pinging %1 with %2 bytes... ").arg(host).arg(size);

            QProcess *proc = new QProcess();
            *procPtr = proc;
            QTimer *killTimer = new QTimer(proc);
            killTimer->setSingleShot(true);

            QObject::connect(killTimer, &QTimer::timeout, proc, [=]() {
                if (proc->state() == QProcess::Running)
                    proc->kill();
                killTimer->deleteLater();
                if (*procPtr == proc) {
                    proc->deleteLater();
                    *procPtr = nullptr;
                }
            });

            QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                [=](int, QProcess::ExitStatus) mutable {
                if (killTimer->isActive()) {
                    killTimer->stop();
                    killTimer->deleteLater();
                }
                QString result = proc->readAllStandardOutput();
                bool success = result.contains("TTL=") && !result.contains("Packet needs to be fragmented");
                QString statusMsg;
                if (success) {
                    statusMsg = QString("<span style='color:green;'>Success (%1 bytes)</span>").arg(size);
                    if (size > *best) *best = size;
                } else {
                    statusMsg = QString("<span style='color:red;'>Failed (%1 bytes, fragmentation or timeout)</span>").arg(size);
                }
                output->append(pingMsg + statusMsg);

                proc->deleteLater();
                if (*procPtr == proc)
                    *procPtr = nullptr;
                *currentSize += *step;
                timer->start(200);
            });
            proc->start("ping", QStringList() << host << "-n" << "1" << "-l" << QString::number(size) << "-f");
            killTimer->start(2000);
        };

        QObject::connect(timer.get(), &QTimer::timeout, [=]() { (*tryNextPtr)(); });

        (*tryNextPtr)();
    });

    // --- Set MTU Button ---
    QObject::connect(setMtuBtn, &QPushButton::clicked, [&]() {
        if (*best <= 0) {
            QMessageBox::warning(&dlg, "Set MTU", "No MTU value to set. Run the test first.");
            return;
        }
        if (ifaceCombo->currentText().isEmpty()) {
            QMessageBox::warning(&dlg, "Set MTU", "No network interface selected.");
            return;
        }
        int mtu = *best;
        int userMtu = mtuSpin->value();
        if (userMtu > mtu) {
            int cont = QMessageBox::warning(
                &dlg, "MTU Warning",
                QString("You entered an MTU (%1) above the discovered maximum (%2).<br>"
                        "This may cause fragmentation or connectivity issues.<br><br>"
                        "Are you sure you want to continue?")
                    .arg(userMtu).arg(mtu),
                QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel
            );
            if (cont != QMessageBox::Yes)
                return;
        }

        // Confirm
        int ret = QMessageBox::question(&dlg, "Set MTU",
            QString("Set MTU for interface <b>%1</b> to <b>%2</b>?<br><br>"
                    "This requires administrator rights.").arg(ifaceCombo->currentText()).arg(userMtu),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (ret != QMessageBox::Yes)
            return;

        // Try to set MTU using netsh (with UAC)
        QString psCmd = QString(
            "Start-Process netsh -ArgumentList 'interface ipv4 set subinterface \"%1\" mtu=%2 store=persistent' -Verb runAs -WindowStyle Hidden"
        ).arg(ifaceCombo->currentText()).arg(userMtu);

        int result = QProcess::execute("powershell", QStringList() << "-WindowStyle" << "Hidden" << "-Command" << psCmd);

        if (result == 0) {
            QMessageBox::information(&dlg, "Set MTU", QString("MTU set to %1 for interface %2.<br><br>You may need to reconnect or restart your network adapter for the change to take effect.").arg(userMtu).arg(ifaceCombo->currentText()));
        } else {
            // If failed, offer to open network settings
            int openSettings = QMessageBox::question(&dlg, "Set MTU",
                "Failed to set MTU automatically.<br><br>"
                "Would you like to open the Windows network settings to set it manually?",
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            if (openSettings == QMessageBox::Yes) {
                QProcess::startDetached("ms-settings:network");
            }
        }
    });

    // Remove: QObject::connect(&dlg, &QDialog::shown, ...) -- not needed and not available in Qt

    dlg.adjustSize();
    dlg.exec();
}


void showHostsFileEditor(QWidget *parent) {
    const QString hostsPath = "C:/Windows/System32/drivers/etc/hosts";
    const QString backupDir = "C:/Users/Public/AppData/Local/IPGui";
    const QString backupPath = backupDir + "/hosts.bak";

    QDir().mkpath(backupDir); // Ensure backup directory exists

    QDialog dlg(parent);
    dlg.setWindowTitle("Hosts File Editor");
    dlg.resize(700, 500);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLabel *hint = new QLabel(
        "<b>Windows Hosts File Editor</b><br>"
        "Each line: <code>IP_address hostname [# comment]</code><br>"
        "Examples:<br>"
        "<code>127.0.0.1   localhost</code><br>"
        "<code>192.168.1.10   myserver.local # test server</code><br>"
        "<span style='color:gray;'>Lines starting with # are comments. Blank lines are ignored.</span>"
    );
    hint->setTextFormat(Qt::RichText);
    layout->addWidget(hint);

    QPlainTextEdit *editor = new QPlainTextEdit;
    QFont mono("Consolas");
    mono.setStyleHint(QFont::Monospace);
    editor->setFont(mono);
    layout->addWidget(editor, 1);

    // Load hosts file
    QString originalText;
    auto loadHosts = [&]() {
        QFile file(hostsPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            editor->setPlainText("# Could not open hosts file for reading.\n# Try running as administrator.");
            originalText = editor->toPlainText();
            return false;
        }
        QTextStream in(&file);
        editor->setPlainText(in.readAll());
        file.close();
        originalText = editor->toPlainText();
        return true;
    };

    // Robust backup: only overwrite if new backup is ready, just before save
    auto backupHosts = [&]() -> bool {
        QFileInfo fi(hostsPath);
        if (!fi.exists() || !fi.isFile()) {
            QMessageBox::warning(&dlg, "Backup Error", "Hosts file does not exist, cannot create backup.");
            return false;
        }
        QString tmpBackupPath = backupPath + ".tmp";
        QFile::remove(tmpBackupPath);
        QFile file(hostsPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(&dlg, "Backup Error", "Could not read hosts file to create backup.");
            return false;
        }
        QFile tmpBackup(tmpBackupPath);
        if (!tmpBackup.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            QMessageBox::warning(&dlg, "Backup Error", "Could not write temporary backup file.");
            file.close();
            return false;
        }
        QTextStream in(&file), out(&tmpBackup);
        out << in.readAll();
        file.close();
        tmpBackup.close();
        QFile::remove(backupPath);
        if (!QFile::rename(tmpBackupPath, backupPath)) {
            QMessageBox::warning(&dlg, "Backup Error", "Could not finalize backup file.");
            return false;
        }
        return true;
    };

    // Save hosts file (no backup here!)
    auto saveHosts = [&](const QString &text) -> bool {
        QFile file(hostsPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            return false;
        }
        QTextStream out(&file);
        out << text;
        file.close();
        return true;
    };

    // Validate hosts file lines (returns error string or empty if OK)
    auto validateHosts = [](const QString &text) -> QString {
        QStringList lines = text.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
        QRegularExpression ipRe(R"(^\s*(\d{1,3}\.){3}\d{1,3}\s+[\w\.\-]+)");
        for (int i = 0; i < lines.size(); ++i) {
            QString line = lines[i].trimmed();
            if (line.isEmpty() || line.startsWith('#')) continue;
            if (!ipRe.match(line).hasMatch()) {
                return QString("Line %1: Invalid format: <code>%2</code>").arg(i+1).arg(line.toHtmlEscaped());
            }
        }
        return "";
    };

    // Restore backup (with UAC if needed, always reloads file after)
    auto restoreBackup = [&]() {
        QFileInfo fi(backupPath);
        if (!fi.exists() || !fi.isFile()) {
            QMessageBox::warning(&dlg, "Restore Backup", "No backup file found at:\n" + backupPath);
            return;
        }
        QFile::remove(hostsPath);
        if (QFile::copy(backupPath, hostsPath)) {
            QThread::msleep(200);
            QFile verifyFile(hostsPath);
            if (verifyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString diskText = QTextStream(&verifyFile).readAll();
                verifyFile.close();
                QFile backupFile(backupPath);
                if (backupFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QString backupText = QTextStream(&backupFile).readAll();
                    backupFile.close();
                    if (diskText == backupText) {
                        loadHosts();
                        QMessageBox::information(&dlg, "Restore Backup", "Backup restored.");
                        return;
                    }
                }
            }
            QMessageBox::warning(&dlg, "Restore Backup", "Backup copied, but could not verify hosts file.");
            loadHosts();
            return;
        }
        // If failed, try with UAC (hidden window)
        QString safeBackupPath = QDir::toNativeSeparators(backupPath);
        QString safeHostsPath = QDir::toNativeSeparators(hostsPath);
        QString psCmd =
            QString("Copy-Item -Path \"%1\" -Destination \"%2\" -Force; exit $LASTEXITCODE")
                .arg(safeBackupPath, safeHostsPath);
        QStringList args;
        args << "-WindowStyle" << "Hidden"
             << "-Command"
             << QString("Start-Process powershell -WindowStyle Hidden -ArgumentList '-NoProfile -Command \"%1\"' -Verb runAs -Wait").arg(psCmd.replace("\"", "\\\""));
        int result = QProcess::execute("powershell", args);
        QThread::msleep(200);
        QFile verifyFile(hostsPath);
        if (verifyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString diskText = QTextStream(&verifyFile).readAll();
            verifyFile.close();
            QFile backupFile(backupPath);
            if (backupFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString backupText = QTextStream(&backupFile).readAll();
                backupFile.close();
                if (diskText == backupText) {
                    loadHosts();
                    QMessageBox::information(&dlg, "Restore Backup", "Backup restored (with administrator rights).");
                    return;
                }
            }
        }
        QMessageBox::critical(&dlg, "Restore Backup", "Failed to restore backup, even with elevation.");
        loadHosts(); // Always reload to show the real file
    };

    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *saveBtn = new QPushButton("Save");
    QPushButton *restoreBtn = new QPushButton("Restore Backup");
    QPushButton *closeBtn = new QPushButton("Close");
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(restoreBtn);
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    loadHosts();

    // Track unsaved changes
    bool isDirty = false;
    QObject::connect(editor, &QPlainTextEdit::textChanged, [&]() {
        isDirty = (editor->toPlainText() != originalText);
        dlg.setWindowTitle(QString("Hosts File Editor%1").arg(isDirty ? " *" : ""));
    });

    // Save logic
    QObject::connect(saveBtn, &QPushButton::clicked, [&]() {
        QString text = editor->toPlainText();
        QString err = validateHosts(text);
        if (!err.isEmpty()) {
            QMessageBox::warning(&dlg, "Syntax Error", "Hosts file not saved:\n" + err);
            return;
        }
        // Backup just before saving!
        if (!backupHosts()) {
            QMessageBox::critical(&dlg, "Backup Failed", "Could not create backup. Save aborted.");
            return;
        }
        // Try to save, if fails, try with UAC
        bool saved = saveHosts(text);
        bool elevated = false;
        if (!saved) {
            // Try to elevate and save using powershell (hidden window)
            QString tmpPath = QDir::temp().filePath("hosts_tmp.txt");
            QFile tmp(tmpPath);
            if (tmp.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
                QTextStream out(&tmp);
                out << text;
                tmp.close();
                QString safeTmpPath = QDir::toNativeSeparators(tmpPath);
                QString safeHostsPath = QDir::toNativeSeparators(hostsPath);
                QString psCmd =
                    QString("Copy-Item -Path \"%1\" -Destination \"%2\" -Force; exit $LASTEXITCODE")
                        .arg(safeTmpPath, safeHostsPath);
                QStringList args;
                args << "-WindowStyle" << "Hidden"
                     << "-Command"
                     << QString("Start-Process powershell -WindowStyle Hidden -ArgumentList '-NoProfile -Command \"%1\"' -Verb runAs -Wait").arg(psCmd.replace("\"", "\\\""));
                int result = QProcess::execute("powershell", args);
                QFile::remove(tmpPath);
                if (result == 0) {
                    elevated = true;
                } else {
                    QMessageBox::critical(&dlg, "Save Failed", "Could not save hosts file, even with elevation.");
                    loadHosts();
                    return;
                }
            } else {
                QMessageBox::critical(&dlg, "Save Failed", "Could not write temporary file for elevation.");
                loadHosts();
                return;
            }
        }
        // After saving (normal or elevated), check if file matches what we wanted
        QThread::msleep(200); // Give Windows a moment to finish the copy
        QFile verifyFile(hostsPath);
        if (verifyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString diskText = QTextStream(&verifyFile).readAll();
            verifyFile.close();
            if (diskText == text) {
                QMessageBox::information(&dlg, "Saved", elevated
                    ? "Hosts file saved with administrator rights."
                    : "Hosts file saved successfully.");
                originalText = text;
                isDirty = false;
                dlg.setWindowTitle("Hosts File Editor");
                editor->setPlainText(text); // Ensure editor matches disk
            } else {
                QMessageBox::critical(&dlg, "Save Failed", "The hosts file could not be updated. (Check permissions, UAC prompt, or antivirus lock.)");
                loadHosts(); // Reload actual file
            }
        } else {
            QMessageBox::critical(&dlg, "Save Failed", "Could not read hosts file after saving.");
            loadHosts();
        }
    });

    // Restore backup logic
    QObject::connect(restoreBtn, &QPushButton::clicked, restoreBackup);

    // Close logic with unsaved changes prompt
    auto tryClose = [&]() {
        if (isDirty) {
            auto ret = QMessageBox::question(
                &dlg,
                "Unsaved Changes",
                "You have unsaved changes. Do you want to close without saving?",
                QMessageBox::Yes | QMessageBox::Cancel,
                QMessageBox::Cancel
            );
            if (ret != QMessageBox::Yes)
                return;
        }
        dlg.accept();
    };
    QObject::connect(closeBtn, &QPushButton::clicked, tryClose);

    // Ctrl+W shortcut for close with prompt
    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), &dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, tryClose);

    dlg.exec();
}

void showNetstatStatisticsDialog(QWidget *parent) {
    // Helper: humanize numbers
    auto humanize = [](quint64 n) -> QString {
        double value = n;
        QStringList units = {"", "K", "M", "G", "T"};
        int unit = 0;
        while (value >= 1000 && unit < units.size() - 1) {
            value /= 1000.0;
            ++unit;
        }
        return QString::number(value, 'f', value < 10 ? 2 : (value < 100 ? 1 : 0)) + " " + units[unit];
    };

    // Map of stat name -> tooltip (all filled in, with line feeds)
    QMap<QString, QString> tooltips = {
        {"Packets Received",
         "Total number of IP packets received by this computer,\n"
         "including those with errors and those addressed to other hosts."},
        {"Received Header Errors",
         "Packets received with errors in the IP header\n"
         "(checksum, length, or version errors).\n"
         "High values may indicate network or hardware issues."},
        {"Received Address Errors",
         "Packets received with invalid destination IP addresses\n"
         "(not assigned to this host or not a valid broadcast/multicast address)."},
        {"Datagrams Forwarded",
         "Packets received and forwarded to another network\n"
         "(this computer acting as a router).\n"
         "Should be zero unless routing is enabled."},
        {"Unknown Protocols Received",
         "Packets received using an unknown or unsupported protocol\n"
         "(not TCP, UDP, ICMP, etc)."},
        {"Received Packets Discarded",
         "Packets received but discarded before delivery to higher layers\n"
         "(due to buffer overflow, congestion, or filtering).\n"
         "These packets were not addressed to this host or could not be processed."},
        {"Received Packets Delivered",
         "Packets successfully delivered to higher-layer protocols\n"
         "(such as TCP, UDP, or ICMP)."},
        {"Output Requests",
         "Total number of IP packets sent by this computer,\n"
         "including those generated locally and those forwarded (if routing is enabled)."},
        {"Routing Discards",
         "Packets discarded because no valid route was found\n"
         "in the routing table."},
        {"Discarded Output Packets",
         "Packets discarded before being sent\n"
         "(e.g., due to buffer overflow, congestion, or filtering)."},
        {"Output Packet No Route",
         "Packets that could not be sent because no route\n"
         "to the destination was found."},
        {"Reassembly Required",
         "Number of IP fragments received that required reassembly\n"
         "into complete packets."},
        {"Reassembly Successful",
         "Number of fragmented packets successfully reassembled."},
        {"Reassembly Failures",
         "Number of fragmented packets that could not be reassembled\n"
         "(due to missing fragments or errors)."},
        {"Datagrams Successfully Fragmented",
         "Number of packets that were successfully fragmented\n"
         "for transmission (when the packet was too large for the network's MTU)."},
        {"Datagrams Failing Fragmentation",
         "Number of packets that could not be fragmented\n"
         "(often due to the 'Don't Fragment' flag or size/configuration issues)."},
        {"Fragments Created",
         "Number of IP fragments created for transmission\n"
         "(when packets are split into smaller pieces to fit the network's MTU)."}
    };

    // Run netstat -s and parse output
    QProcess proc;
    proc.start("netstat", QStringList() << "-s");
    proc.waitForFinished(2000);
    QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());

    // Parse IPv4 and IPv6 blocks
    QMap<QString, QMap<QString, quint64>> stats; // "IPv4" or "IPv6" -> (stat name -> value)
    QString currentProto;
    QRegularExpression statLineRe(R"(^\s*([A-Za-z0-9 \-]+?)\s*=\s*([0-9]+))");
    for (const QString &line : output.split('\n')) {
        QString trimmed = line.trimmed();
        if (trimmed.startsWith("IPv4 Statistics", Qt::CaseInsensitive)) {
            currentProto = "IPv4";
            continue;
        }
        if (trimmed.startsWith("IPv6 Statistics", Qt::CaseInsensitive)) {
            currentProto = "IPv6";
            continue;
        }
        if (currentProto.isEmpty()) continue;
        QRegularExpressionMatch m = statLineRe.match(trimmed);
        if (m.hasMatch()) {
            QString name = m.captured(1).trimmed();
            quint64 value = m.captured(2).toULongLong();
            stats[currentProto][name] = value;
        }
    }

    // Dialog setup
    QDialog dlg(parent);
    dlg.setWindowTitle("Netstat Statistics");
    dlg.setMinimumWidth(520);
    addCtrlWClose(&dlg);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLabel *title = new QLabel("<b>Netstat Protocol Statistics</b><br>"
        "<span style='color:gray;'>Shows key IPv4 and IPv6 network health counters.<br>"
        "Hover any value for a detailed explanation.</span>");
    title->setTextFormat(Qt::RichText);
    layout->addWidget(title);

    // Table for both IPv4 and IPv6
    QTableWidget *table = new QTableWidget();
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels(QStringList() << "Statistic" << "IPv4" << "IPv6");
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setFocusPolicy(Qt::NoFocus);

    // Collect all stat names in order of IPv4 block
    QStringList statNames = stats["IPv4"].keys();
    // Add any IPv6-only stats
    for (const QString &k : stats["IPv6"].keys()) {
        if (!statNames.contains(k))
            statNames.append(k);
    }
    table->setRowCount(statNames.size());

    QBrush blue(QColor("#1c2684"));
    QBrush green(QColor("#1a7d2c"));
    QBrush red(QColor("#c80000"));
    QBrush gray(QColor("#888"));

    for (int row = 0; row < statNames.size(); ++row) {
        QString stat = statNames[row];
        QTableWidgetItem *nameItem = new QTableWidgetItem(stat);
        nameItem->setFont(QFont("Segoe UI", 10, QFont::Bold));
        nameItem->setForeground(blue);
        nameItem->setToolTip(tooltips.value(stat, stat));

        // IPv4 value
        quint64 v4 = stats["IPv4"].value(stat, 0);
        QTableWidgetItem *v4Item = new QTableWidgetItem(humanize(v4));
        v4Item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        v4Item->setToolTip(QString("%1\n\n%2").arg(v4).arg(tooltips.value(stat, "")));
        v4Item->setForeground(v4 == 0 ? gray : (stat.contains("Error", Qt::CaseInsensitive) || stat.contains("Fail", Qt::CaseInsensitive) || stat.contains("Discard", Qt::CaseInsensitive) ? red : green));
        table->setItem(row, 1, v4Item);

        // IPv6 value
        quint64 v6 = stats["IPv6"].value(stat, 0);
        QTableWidgetItem *v6Item = new QTableWidgetItem(humanize(v6));
        v6Item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        v6Item->setToolTip(QString("%1\n\n%2").arg(v6).arg(tooltips.value(stat, "")));
        v6Item->setForeground(v6 == 0 ? gray : (stat.contains("Error", Qt::CaseInsensitive) || stat.contains("Fail", Qt::CaseInsensitive) || stat.contains("Discard", Qt::CaseInsensitive) ? red : green));
        table->setItem(row, 2, v6Item);

        table->setItem(row, 0, nameItem);
    }

    table->resizeColumnsToContents();
    layout->addWidget(table);

    QLabel *legend = new QLabel(
        "<span style='color:#1a7d2c; font-weight:bold;'>Green:</span> Normal/healthy<br>"
        "<span style='color:#c80000; font-weight:bold;'>Red:</span> Errors, discards, or failures<br>"
        "<span style='color:#888;'>Gray:</span> Zero (not seen or not applicable)"
    );
    legend->setTextFormat(Qt::RichText);
    layout->addWidget(legend);

    QPushButton *closeBtn = new QPushButton("Close");
    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addWidget(closeBtn);

    dlg.adjustSize();
    dlg.exec();
}


void showRouteTableDialog(QWidget *parent) {
    QDialog dlg(parent);
    dlg.setWindowTitle("Route Table Viewer/Editor");
    dlg.setMinimumWidth(900);
    addCtrlWClose(&dlg);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLabel *title = new QLabel(
        "<b>Windows Route Table</b><br>"
        "<span style='color:gray;'>Shows all active IPv4 and IPv6 routes. - "
        "You can delete or add routes. - "
        "Mouseover any value for technical details.</span>");
    title->setTextFormat(Qt::RichText);
    layout->addWidget(title);

    // IPv4 columns
    QStringList ipv4Headers = {
        "Destination", "Netmask", "Gateway", "Interface", "Metric", "Action"
    };
    QStringList ipv4Tips = {
        "Destination IP/network for this route.\n(Technical: Network address in CIDR notation.)",
        "Subnet mask for the destination.\n(Technical: Specifies the network portion of the address.)",
        "Gateway IP for forwarding packets.\n(Technical: Next hop for this route.)",
        "Local interface used for this route.\n(Technical: Adapter IP address.)",
        "Route metric (priority).\n(Technical: Lower is preferred.)",
        "Delete this route."
    };

    // IPv6 columns
    QStringList ipv6Headers = {
        "If", "Metric", "Destination", "Gateway", "Action"
    };
    QStringList ipv6Tips = {
        "Interface index.\n(Technical: Windows adapter index for this route.)",
        "Route metric (priority).\n(Technical: Lower is preferred.)",
        "Destination IPv6 network/prefix.\n(Technical: Network address in CIDR notation.)",
        "Gateway IPv6 address or On-link.\n(Technical: Next hop for this route.)",
        "Delete this route."
    };

    auto createTable = [&](const QStringList &headers, const QStringList &tips) -> QTableWidget* {
        QTableWidget *table = new QTableWidget();
        table->setColumnCount(headers.size());
        table->setHorizontalHeaderLabels(headers);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->verticalHeader()->setVisible(false);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setFocusPolicy(Qt::NoFocus);
        for (int i = 0; i < headers.size(); ++i)
            table->horizontalHeaderItem(i)->setToolTip(tips[i]);
        return table;
    };

    QTableWidget *ipv4Table = createTable(ipv4Headers, ipv4Tips);
    QTableWidget *ipv6Table = createTable(ipv6Headers, ipv6Tips);

    QBrush blue(QColor("#1c2684"));
    QBrush green(QColor("#1a7d2c"));
    QBrush red(QColor("#c80000"));
    QBrush gray(QColor("#888"));

    // Use std::function for recursion/capture
    std::function<void(QTableWidget*)> fillIPv4Table;
    fillIPv4Table = [&](QTableWidget *table) {
        table->setRowCount(0);
        QProcess proc;
        proc.start("route", QStringList() << "print");
        proc.waitForFinished(2000);
        QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());

        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        bool inTable = false;
        for (const QString &line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.startsWith("IPv4 Route Table")) {
                inTable = false;
                continue;
            }
            if (trimmed.startsWith("Network Destination")) {
                inTable = true;
                continue;
            }
            if (inTable && (trimmed.isEmpty() || trimmed.startsWith("===") || trimmed.startsWith("Persistent Routes"))) {
                inTable = false;
                continue;
            }
            if (inTable) {
                QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                if (parts.size() < 5) continue;
                int row = table->rowCount();
                table->insertRow(row);
                for (int col = 0; col < 5; ++col) {
                    QTableWidgetItem *item = new QTableWidgetItem(parts[col]);
                    item->setForeground(col == 0 ? blue : (col == 2 ? green : gray));
                    item->setToolTip(ipv4Tips[col]);
                    item->setFont(QFont("Segoe UI", 10, QFont::Bold));
                    table->setItem(row, col, item);
                }
                // Add Delete button
                QPushButton *delBtn = new QPushButton("Delete");
                delBtn->setToolTip("Delete this route.\n(Technical: route delete <destination> mask <netmask> <gateway>)");
                table->setCellWidget(row, 5, delBtn);
                QObject::connect(delBtn, &QPushButton::clicked, [=, &dlg]() {
                    QString dest = parts[0], mask = parts[1], gw = parts[2];
                    int ret = QMessageBox::question(&dlg, "Delete Route",
                        QString("Are you sure you want to delete this route?\n\n"
                                "Destination: %1\nNetmask: %2\nGateway: %3").arg(dest, mask, gw),
                        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
                    if (ret != QMessageBox::Yes) return;
                    // Delete route (needs admin)
                    QString psCmd = QString(
                        "Start-Process route -ArgumentList 'delete %1 mask %2 %3' -Verb runAs -WindowStyle Hidden"
                    ).arg(dest, mask, gw);
                    int result = QProcess::execute("powershell", QStringList() << "-WindowStyle" << "Hidden" << "-Command" << psCmd);
                    if (result == 0) {
                        QMessageBox::information(&dlg, "Route Deleted", "Route deleted successfully.");
                        fillIPv4Table(table);
                    } else {
                        QMessageBox::warning(&dlg, "Route Delete", "Failed to delete route (admin rights needed or cancelled).");
                        // Do NOT refresh table if failed
                    }
                });
            }
        }
        table->resizeColumnsToContents();
    };

    std::function<void(QTableWidget*)> fillIPv6Table;
    fillIPv6Table = [&](QTableWidget *table) {
        table->setRowCount(0);
        QProcess proc;
        proc.start("route", QStringList() << "print");
        proc.waitForFinished(2000);
        QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());

        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        bool inTable = false;
        int colCount = 4;
        for (int i = 0; i < lines.size(); ++i) {
            QString line = lines[i].trimmed();
            if (line.startsWith("IPv6 Route Table")) {
                inTable = false;
                continue;
            }
            if (line.startsWith("Active Routes:")) {
                inTable = true;
                continue;
            }
            if (inTable && (line.isEmpty() || line.startsWith("===") || line.startsWith("Persistent Routes"))) {
                inTable = false;
                continue;
            }
            if (inTable) {
                QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                if (parts.size() == colCount) {
                    int row = table->rowCount();
                    table->insertRow(row);
                    for (int col = 0; col < colCount; ++col) {
                        QTableWidgetItem *item = new QTableWidgetItem(parts[col]);
                        item->setForeground(col == 2 ? blue : (col == 3 ? green : gray));
                        item->setToolTip(ipv6Tips[col]);
                        item->setFont(QFont("Segoe UI", 10, QFont::Bold));
                        table->setItem(row, col, item);
                    }
                    // Add Delete button
                    QPushButton *delBtn = new QPushButton("Delete");
                    delBtn->setToolTip("Delete this route.\n(Technical: route delete <destination> -6)");
                    table->setCellWidget(row, 4, delBtn);
                    QObject::connect(delBtn, &QPushButton::clicked, [=, &dlg]() {
                        QString dest = parts[2];
                        int ret = QMessageBox::question(&dlg, "Delete Route",
                            QString("Are you sure you want to delete this IPv6 route?\n\n"
                                    "Destination: %1").arg(dest),
                            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
                        if (ret != QMessageBox::Yes) return;
                        QString psCmd = QString(
                            "Start-Process route -ArgumentList 'delete %1 -6' -Verb runAs -WindowStyle Hidden"
                        ).arg(dest);
                        int result = QProcess::execute("powershell", QStringList() << "-WindowStyle" << "Hidden" << "-Command" << psCmd);
                        if (result == 0) {
                            QMessageBox::information(&dlg, "Route Deleted", "IPv6 route deleted successfully.");
                            fillIPv6Table(table);
                        } else {
                            QMessageBox::warning(&dlg, "Route Delete", "Failed to delete IPv6 route (admin rights needed or cancelled).");
                            // Do NOT refresh table if failed
                        }
                    });
                } else if (parts.size() == 3 && i + 1 < lines.size()) {
                    QString nextLine = lines[i + 1].trimmed();
                    QStringList nextParts = nextLine.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                    if (nextParts.size() == 1) {
                        int row = table->rowCount();
                        table->insertRow(row);
                        for (int col = 0; col < 3; ++col) {
                            QTableWidgetItem *item = new QTableWidgetItem(parts[col]);
                            item->setForeground(col == 2 ? blue : gray);
                            item->setToolTip(ipv6Tips[col]);
                            item->setFont(QFont("Segoe UI", 10, QFont::Bold));
                            table->setItem(row, col, item);
                        }
                        QTableWidgetItem *gwItem = new QTableWidgetItem(nextParts[0]);
                        gwItem->setForeground(green);
                        gwItem->setToolTip(ipv6Tips[3]);
                        gwItem->setFont(QFont("Segoe UI", 10, QFont::Bold));
                        table->setItem(row, 3, gwItem);
                        // Add Delete button
                        QPushButton *delBtn = new QPushButton("Delete");
                        delBtn->setToolTip("Delete this route.\n(Technical: route delete <destination> -6)");
                        table->setCellWidget(row, 4, delBtn);
                        QObject::connect(delBtn, &QPushButton::clicked, [=, &dlg]() {
                            QString dest = parts[2];
                            int ret = QMessageBox::question(&dlg, "Delete Route",
                                QString("Are you sure you want to delete this IPv6 route?\n\n"
                                        "Destination: %1").arg(dest),
                                QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
                            if (ret != QMessageBox::Yes) return;
                            QString psCmd = QString(
                                "Start-Process route -ArgumentList 'delete %1 -6' -Verb runAs -WindowStyle Hidden"
                            ).arg(dest);
                            int result = QProcess::execute("powershell", QStringList() << "-WindowStyle" << "Hidden" << "-Command" << psCmd);
                            if (result == 0) {
                                QMessageBox::information(&dlg, "Route Deleted", "IPv6 route deleted successfully.");
                                fillIPv6Table(table);
                            } else {
                                QMessageBox::warning(&dlg, "Route Delete", "Failed to delete IPv6 route (admin rights needed or cancelled).");
                                // Do NOT refresh table if failed
                            }
                        });
                        ++i;
                    }
                }
            }
        }
        table->resizeColumnsToContents();
    };

    fillIPv4Table(ipv4Table);
    fillIPv6Table(ipv6Table);

    QLabel *ipv4Label = new QLabel("<b>IPv4 Routes</b>");
    ipv4Label->setTextFormat(Qt::RichText);
    layout->addWidget(ipv4Label);
    layout->addWidget(ipv4Table);

    QLabel *ipv6Label = new QLabel("<b>IPv6 Routes</b>");
    ipv6Label->setTextFormat(Qt::RichText);
    layout->addWidget(ipv6Label);
    layout->addWidget(ipv6Table);

    // Add route section (IPv4 and IPv6)
    QGroupBox *addBox = new QGroupBox("Add Route");
    QFormLayout *form = new QFormLayout(addBox);
    QComboBox *protoCombo = new QComboBox;
    protoCombo->addItems({"IPv4", "IPv6"});
    protoCombo->setToolTip("Select route type.\n(Technical: IPv4 or IPv6 routing table.)");
    QLineEdit *destEdit = new QLineEdit;
    destEdit->setPlaceholderText("e.g. 192.168.2.0 or 2001:db8::");
    destEdit->setToolTip("Destination IP/network.\n(Technical: Network address in CIDR notation.)");
    QLineEdit *maskEdit = new QLineEdit;
    maskEdit->setPlaceholderText("e.g. 255.255.255.0 (IPv4 only)");
    maskEdit->setToolTip("Subnet mask for the destination.\n(Technical: Specifies the network portion of the address.)");
    QLineEdit *gwEdit = new QLineEdit;
    gwEdit->setPlaceholderText("e.g. 192.168.1.1 or fe80::1");
    gwEdit->setToolTip("Gateway IP for forwarding packets.\n(Technical: Next hop for this route.)");
    QLineEdit *ifaceEdit = new QLineEdit;
    ifaceEdit->setPlaceholderText("e.g. 192.168.1.100 or interface index");
    ifaceEdit->setToolTip("Local interface IP address or index.\n(Technical: Adapter used for this route.)");
    QSpinBox *metricSpin = new QSpinBox;
    metricSpin->setRange(1, 9999);
    metricSpin->setValue(10);
    metricSpin->setToolTip("Route metric (priority).\n(Technical: Lower is preferred.)");
    QPushButton *addBtn = new QPushButton("Add Route");
    addBtn->setToolTip("Add this route to the table.\n(Technical: route add <destination> mask <netmask> <gateway> metric <metric> if <interface>.)");

    form->addRow("Type:", protoCombo);
    form->addRow("Destination:", destEdit);
    form->addRow("Netmask:", maskEdit);
    form->addRow("Gateway:", gwEdit);
    form->addRow("Interface:", ifaceEdit);
    form->addRow("Metric:", metricSpin);
    form->addRow(addBtn);

    layout->addWidget(addBox);

    QObject::connect(addBtn, &QPushButton::clicked, [&]() {
        QString proto = protoCombo->currentText();
        QString dest = destEdit->text().trimmed();
        QString mask = maskEdit->text().trimmed();
        QString gw = gwEdit->text().trimmed();
        QString iface = ifaceEdit->text().trimmed();
        int metric = metricSpin->value();
        if (dest.isEmpty() || gw.isEmpty() || iface.isEmpty() || (proto == "IPv4" && mask.isEmpty())) {
            QMessageBox::warning(&dlg, "Input Error", "Please fill in all required fields.");
            return;
        }
        int ret = QMessageBox::question(&dlg, "Add Route",
            QString("Are you sure you want to add this route?\n\n"
                    "Type: %1\nDestination: %2\nNetmask: %3\nGateway: %4\nInterface: %5\nMetric: %6")
                .arg(proto, dest, mask, gw, iface).arg(metric),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (ret != QMessageBox::Yes) return;
        // Add route (needs admin)
        QString psCmd;
        if (proto == "IPv4") {
            psCmd = QString(
                "Start-Process route -ArgumentList 'add %1 mask %2 %3 metric %4 if %5' -Verb runAs -WindowStyle Hidden"
            ).arg(dest, mask, gw).arg(metric).arg(iface);
        } else {
            psCmd = QString(
                "Start-Process route -ArgumentList 'add %1 %2 metric %3 if %4 -6' -Verb runAs -WindowStyle Hidden"
            ).arg(dest, gw).arg(metric).arg(iface);
        }
        int result = QProcess::execute("powershell", QStringList() << "-WindowStyle" << "Hidden" << "-Command" << psCmd);
        if (result == 0)
            QMessageBox::information(&dlg, "Route Added", "Route added successfully.");
        else
            QMessageBox::warning(&dlg, "Route Add", "Failed to add route (admin rights needed).");
        fillIPv4Table(ipv4Table);
        fillIPv6Table(ipv6Table);
    });

    QPushButton *closeBtn = new QPushButton("Close");
    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addWidget(closeBtn);

    dlg.adjustSize();
    dlg.exec();
}

void showDnsCacheDialog(QWidget *parent) {
    QDialog dlg(parent);
    dlg.setWindowTitle("DNS Cache Viewer");
    dlg.setMinimumWidth(900);
    addCtrlWClose(&dlg);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLabel *title = new QLabel(
        "<b>DNS Cache Viewer</b><br>"
        "<span style='color:gray;'>Shows all cached DNS entries on your system. - Mouseover any value for technical details.</span>");
    title->setTextFormat(Qt::RichText);
    layout->addWidget(title);

    // Table setup
    QTableWidget *table = new QTableWidget();
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels(QStringList()
        << "Hostname"
        << "Type"
        << "IP Address"
        << "TTL (s)"
        << "Status");
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::ContiguousSelection);
    table->setSelectionBehavior(QAbstractItemView::SelectItems);
    table->setFocusPolicy(Qt::StrongFocus);
    table->setWordWrap(true);

    // Tooltips for columns
    QStringList colTips = {
        "The domain name or hostname that was resolved.\n(Technical: DNS query name.)",
        "Record type (A=IPv4, AAAA=IPv6, CNAME, etc).\n(Technical: DNS resource record type.)",
        "The resolved IP address(es), IPv4 first, then IPv6.\n(Technical: DNS answer data.)",
        "Time to live (seconds) before this entry expires.\n(Technical: Remaining TTL in cache.)",
        "Status of this entry (e.g. Success, Negative, etc).\n(Technical: Indicates if the entry is valid or failed.)"
    };
    for (int i = 0; i < table->columnCount(); ++i)
        table->horizontalHeaderItem(i)->setToolTip(colTips[i]);

    layout->addWidget(table);

    // Parse DNS cache
    struct DnsEntry {
        QString hostname, type, data, ttl, status;
    };
    QList<DnsEntry> entries;

    // Run "ipconfig /displaydns" and parse output
    QProcess proc;
    proc.start("ipconfig", QStringList() << "/displaydns");
    proc.waitForFinished(2000);
    QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());

    QString hostname, ttl, status;
    QString currentType;
    QStringList currentData;
    auto flushTypeData = [&]() {
        if (!hostname.isEmpty() && !currentType.isEmpty()) {
            for (const QString &data : currentData) {
                entries.append({hostname, currentType, data, ttl, status});
            }
        }
        currentType.clear();
        currentData.clear();
    };

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        if (line.startsWith("Record Name", Qt::CaseInsensitive)) {
            flushTypeData();
            hostname = line.section(':', 1).trimmed();
            ttl.clear();
            status = "Success";
        } else if (line.startsWith("Record Type", Qt::CaseInsensitive)) {
            flushTypeData();
            QString t = line.section(':', 1).trimmed();
            if (t == "1") currentType = "A";
            else if (t == "28") currentType = "AAAA";
            else if (t == "5") currentType = "CNAME";
            else currentType = t;
        } else if (line.startsWith("Data", Qt::CaseInsensitive)) {
            QString data = line.section(':', 1).trimmed();
            currentData.append(data);
        } else if (line.startsWith("Time To Live", Qt::CaseInsensitive)) {
            ttl = line.section(':', 1).trimmed();
        } else if (line.contains("No records", Qt::CaseInsensitive)) {
            flushTypeData();
            entries.append({hostname, "-", "-", "-", "Negative"});
        } else if (line.startsWith("-----")) {
            flushTypeData();
            hostname.clear();
            ttl.clear();
            status = "Success";
        }
    }
    flushTypeData();

    // Group by hostname for live lookup and merging
    struct HostResult {
        QString hostname;
        QStringList ipv4s;
        QStringList ipv6s;
        QString cname;
        QString ttl;
        QString status;
    };
    QMap<QString, HostResult> hostMap;

    for (const DnsEntry &e : entries) {
        if (e.status == "Negative") {
            HostResult &hr = hostMap[e.hostname];
            hr.hostname = e.hostname;
            hr.status = "Negative";
            continue;
        }
        HostResult &hr = hostMap[e.hostname];
        hr.hostname = e.hostname;
        hr.ttl = e.ttl;
        hr.status = e.status;
        if (e.type == "A" && QRegularExpression(R"(^(\d{1,3}\.){3}\d{1,3}$)").match(e.data).hasMatch()) {
            hr.ipv4s << e.data;
        } else if (e.type == "AAAA" && e.data.contains(':')) {
            hr.ipv6s << e.data;
        } else if (e.type == "CNAME" && e.data.contains('.')) {
            hr.cname = e.data;
        }
    }

    // For any host missing either IPv4 or IPv6, do a live lookup
    for (auto it = hostMap.begin(); it != hostMap.end(); ++it) {
        HostResult &hr = it.value();
        if (hr.status == "Negative") continue;
        QHostInfo info = QHostInfo::fromName(hr.hostname);
        for (const QHostAddress &addr : info.addresses()) {
            if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
                QString ip = addr.toString();
                if (!hr.ipv4s.contains(ip))
                    hr.ipv4s << ip;
            } else if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
                QString ip = addr.toString();
                if (!hr.ipv6s.contains(ip))
                    hr.ipv6s << ip;
            }
        }
    }

    // Prepare final entries for display
    QList<DnsEntry> finalEntries;
    for (const auto &hr : hostMap) {
        if (hr.status == "Negative") {
            finalEntries.append({hr.hostname, "-", "-", "-", "Negative"});
            continue;
        }
        QStringList typeLines;
        typeLines << "A" << "AAAA";
        if (!hr.cname.isEmpty()) typeLines << "CNAME";
        QString type = typeLines.join("\n");

        QStringList dataLines;
        if (!hr.ipv4s.isEmpty())
            dataLines << hr.ipv4s;
        else
            dataLines << "IPv4 not found";
        if (!hr.ipv6s.isEmpty())
            dataLines << hr.ipv6s;
        else
            dataLines << "IPv6 not found";
        if (!hr.cname.isEmpty())
            dataLines << hr.cname;
        QString data = dataLines.join("\n");

        finalEntries.append({hr.hostname, type, data, hr.ttl, hr.status});
    }

    // Sorting state
    int sortColumn = 0;
    Qt::SortOrder sortOrder = Qt::AscendingOrder;

    auto updateHeaderArrows = [&]() {
        for (int i = 0; i < table->columnCount(); ++i) {
            QString label = table->horizontalHeaderItem(i)->text();
            label = label.split(' ').first();
            if (i == sortColumn)
                label += (sortOrder == Qt::AscendingOrder ? " ▲" : " ▼");
            table->horizontalHeaderItem(i)->setText(label);
        }
    };

    auto sortEntries = [&](QList<DnsEntry> list) -> QList<DnsEntry> {
        std::function<bool(const DnsEntry&, const DnsEntry&)> cmp;
        switch (sortColumn) {
            case 0:
                cmp = [&](const DnsEntry &a, const DnsEntry &b) {
                    return sortOrder == Qt::AscendingOrder ? a.hostname.toLower() < b.hostname.toLower()
                                                           : a.hostname.toLower() > b.hostname.toLower();
                }; break;
            case 1:
                cmp = [&](const DnsEntry &a, const DnsEntry &b) {
                    return sortOrder == Qt::AscendingOrder ? a.type < b.type : a.type > b.type;
                }; break;
            case 2:
                cmp = [&](const DnsEntry &a, const DnsEntry &b) {
                    return sortOrder == Qt::AscendingOrder ? a.data < b.data : a.data > b.data;
                }; break;
            case 3:
                cmp = [&](const DnsEntry &a, const DnsEntry &b) {
                    bool okA, okB;
                    int ttlA = a.ttl.toInt(&okA), ttlB = b.ttl.toInt(&okB);
                    if (okA && okB)
                        return sortOrder == Qt::AscendingOrder ? ttlA < ttlB : ttlA > ttlB;
                    return sortOrder == Qt::AscendingOrder ? a.ttl < b.ttl : a.ttl > b.ttl;
                }; break;
            case 4:
                cmp = [&](const DnsEntry &a, const DnsEntry &b) {
                    return sortOrder == Qt::AscendingOrder ? a.status < b.status : a.status > b.status;
                }; break;
            default: cmp = [](const DnsEntry&, const DnsEntry&) { return false; };
        }
        std::sort(list.begin(), list.end(), cmp);
        return list;
    };

    auto fillTable = [&]() {
        QList<DnsEntry> sorted = sortEntries(finalEntries);
        table->setRowCount(sorted.size());
        QFont cellFont("Segoe UI", 10, QFont::Bold);
        QFontMetrics fm(cellFont);
        int minRowHeight = fm.height() + 6;
        for (int row = 0; row < sorted.size(); ++row) {
            const DnsEntry &e = sorted[row];
            QStringList fields = {e.hostname, e.type, e.data, e.ttl, e.status};
            for (int col = 0; col < fields.size(); ++col) {
                if (col == 2) {
                    QTextEdit *edit = new QTextEdit;
                    edit->setReadOnly(true);
                    edit->setFrameStyle(QFrame::NoFrame);
                    edit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                    edit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                    edit->setFont(cellFont);
                    edit->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
                    edit->setAlignment(Qt::AlignLeft | Qt::AlignTop);

                    QStringList lines = fields[col].split('\n');
                    QString html;
                    for (const QString &line : lines) {
                        QString safe = line.toHtmlEscaped();
                        if (line.trimmed() == "IPv4 not found" || line.trimmed() == "IPv6 not found")
                            html += "<span style='color:#888;'>" + safe + "</span><br>";
                        else if (QRegularExpression(R"(^(\d{1,3}\.){3}\d{1,3}$)").match(line).hasMatch())
                            html += "<span style='color:#7c3cff;'>" + safe + "</span><br>";
                        else if (line.contains(':'))
                            html += "<span style='color:#3c1c5c;'>" + safe + "</span><br>";
                        else
                            html += safe + "<br>";
                    }
                    if (html.endsWith("<br>")) html.chop(4);
                    edit->setHtml(html);
                    table->setCellWidget(row, col, edit);
                } else {
                    QTableWidgetItem *item = new QTableWidgetItem(fields[col]);
                    item->setFont(cellFont);
                    item->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);
                    if (col == 0) item->setForeground(QBrush(QColor("#1c2684")));
                    else if (col == 1) item->setForeground(QBrush(QColor("#1a7d2c")));
                    else if (col == 3) item->setForeground(QBrush(QColor("#888")));
                    else if (col == 4) {
                        QBrush statusBrush(QColor("#1a7d2c"));
                        if (e.status == "Negative")
                            statusBrush = QBrush(QColor("#c80000"));
                        item->setForeground(statusBrush);
                    }
                    item->setToolTip(colTips[col]);
                    table->setItem(row, col, item);
                }
            }
            int linesType = sorted[row].type.count('\n') + 1;
            int linesData = sorted[row].data.count('\n') + 1;
            int lines = qMax(linesType, linesData);
            table->setRowHeight(row, qMax(minRowHeight, lines * fm.lineSpacing() + 6));
        }
        table->resizeColumnsToContents();
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        for (int i = 3; i < table->columnCount(); ++i)
            table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
        updateHeaderArrows();
    };

    QObject::connect(table->horizontalHeader(), &QHeaderView::sectionClicked, [&](int col) {
        if (sortColumn == col) {
            sortOrder = (sortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
        } else {
            sortColumn = col;
            sortOrder = (col == 0) ? Qt::AscendingOrder : Qt::DescendingOrder;
        }
        fillTable();
    });

    fillTable();

    if (finalEntries.isEmpty()) {
        QLabel *emptyMsg = new QLabel("<b>No DNS cache entries found.</b><br>Try browsing some websites or running <code>nslookup</code> in a terminal, then refresh.");
        emptyMsg->setAlignment(Qt::AlignCenter);
        layout->addWidget(emptyMsg);
    }

    QGridLayout *legendLayout = new QGridLayout();
    legendLayout->setColumnStretch(0, 1);
    legendLayout->setColumnStretch(1, 1);
    legendLayout->setColumnStretch(2, 1);

    QLabel *greenLbl = new QLabel("<span style='color:#1a7d2c; font-weight:bold;'>Green:</span> Record type, valid status");
    QLabel *purple4Lbl = new QLabel("<span style='color:#7c3cff; font-weight:bold;'>Purple:</span> IPv4 address");
    QLabel *blueLbl = new QLabel("<span style='color:#1c2684;'>Blue:</span> Hostname");
    QLabel *redLbl = new QLabel("<span style='color:#c80000; font-weight:bold;'>Red:</span> Negative/failed entry or unknown type");
    QLabel *purple6Lbl = new QLabel("<span style='color:#3c1c5c; font-weight:bold;'>Dark purple:</span> IPv6 address");
    QLabel *grayLbl = new QLabel("<span style='color:#888;'>Gray:</span> Not found markers");

    greenLbl->setTextFormat(Qt::RichText);
    purple4Lbl->setTextFormat(Qt::RichText);
    blueLbl->setTextFormat(Qt::RichText);
    redLbl->setTextFormat(Qt::RichText);
    purple6Lbl->setTextFormat(Qt::RichText);
    grayLbl->setTextFormat(Qt::RichText);

    legendLayout->addWidget(greenLbl, 0, 0);
    legendLayout->addWidget(purple4Lbl, 0, 1);
    legendLayout->addWidget(blueLbl, 0, 2);
    legendLayout->addWidget(redLbl, 1, 0);
    legendLayout->addWidget(purple6Lbl, 1, 1);
    legendLayout->addWidget(grayLbl, 1, 2);

    QWidget *legendWidget = new QWidget();
    legendWidget->setLayout(legendLayout);
    layout->addWidget(legendWidget);

    QPushButton *closeBtn = new QPushButton("Close");
    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addWidget(closeBtn);

    dlg.adjustSize();
    dlg.exec();
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
    window.setWindowTitle("IPGui V. " + VersionNumber);

    QWidget *centralWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

   
    // Info box
    QTextEdit *infoBox = new QTextEdit();
    infoBox->setReadOnly(true);
    infoBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(infoBox);

    // Always use the function for consistent display and coloring
    updateIpDisplay(infoBox);

    // (No layout->addStretch(1); here!)

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QVBoxLayout *buttonRows = new QVBoxLayout();

    // First row
    QHBoxLayout *row1 = new QHBoxLayout();
    QPushButton *expandBtn = new QPushButton("Advanced");
    expandBtn->setToolTip("Like ipconfig /all on the command line");
    QPushButton *flushBtn = new QPushButton("Flush DNS");
    flushBtn->setToolTip("Flush DNS Cache");
    row1->addWidget(expandBtn);
    row1->addWidget(flushBtn);

    // Second row
    QHBoxLayout *row2 = new QHBoxLayout();
    QPushButton *releaseBtn = new QPushButton("Release IP");
    releaseBtn->setToolTip("Release current IP address");
    QPushButton *renewBtn = new QPushButton("Renew IP");
    renewBtn->setToolTip("Renew current IP address");
    row2->addWidget(releaseBtn);
    row2->addWidget(renewBtn);

    // Add both rows to the vertical layout
    buttonRows->addLayout(row1);
    buttonRows->addLayout(row2);

    // Add the buttonRows layout to your main layout
    layout->addLayout(buttonRows);

    // Connect the Flush DNS button
    QObject::connect(flushBtn, &QPushButton::clicked, [&]() {
        int result = QProcess::execute("ipconfig", QStringList() << "/flushdns");
        if (result == 0)
            QMessageBox::information(nullptr, "Flush DNS", "DNS cache flushed.");
        else
            QMessageBox::warning(nullptr, "Flush DNS", "Failed to flush DNS cache.");
    });


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
        updateIpDisplay(infoBox);
        expandBtn->setText("Advanced");
        expanded = false;
        window.resize(255, 330); // Restore minimum size for basic mode

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

// Main menu
QMenu *fileMenu = menuBar->addMenu("&Actions");


// Add NetTools submenu
QMenu *netToolsMenu = fileMenu->addMenu("NetTools");


// Add action to NetTools submenu
QAction *arpAction           = netToolsMenu->addAction("Arp");
QAction *dhcpStatusAction    = netToolsMenu->addAction("DHCP Status");
QAction *dnsCacheAction      = netToolsMenu->addAction("DNS Cache Viewer");
QAction *hostsFileAction     = netToolsMenu->addAction("Hosts File Editor");
QAction *sslCertAction       = netToolsMenu->addAction("HTTPS Certificate Check");
QAction *netscanAction       = netToolsMenu->addAction("IP Scanner");
QAction *mtuDiscoveryAction  = netToolsMenu->addAction("MTU Discovery");
QAction *netstatStatsAction  = netToolsMenu->addAction("Netstat Statistics");
QAction *adaptersAction      = netToolsMenu->addAction("Network Adapters");
QAction *netUsageAction      = netToolsMenu->addAction("Network Usage");
QAction *nslookupAction      = netToolsMenu->addAction("NS Lookup");
QAction *pingAction          = netToolsMenu->addAction("Ping");
QAction *portscanAction      = netToolsMenu->addAction("Port Scan");
QAction *routeTableAction   = netToolsMenu->addAction("Route Table Viewer/Editor");
QAction *tracertAction       = netToolsMenu->addAction("Traceroute");
QAction *wifiScanAction      = netToolsMenu->addAction("WiFi Scan");


QAction *alwaysOnTopAction = fileMenu->addAction("🔵 Always on top");

QAction *deleteTempAction = fileMenu->addAction("Delete Temporary Files");


// Add the slot/function to delete the temp folder
auto deleteTempFiles = [&window]() {
    QString tempDir = "C:/Users/Public/AppData/Local/IPGui";
    if (!QDir(tempDir).exists()) {
        QMessageBox::information(&window, "Delete Temporary Files", "No temporary files found.");
        return;
    }
    int ret = QMessageBox::question(
        &window,
        "Delete Temporary Files",
        "Are you sure you want to delete all temporary files for this application?\n\n"
        "This will remove:\n" + tempDir,
        QMessageBox::Yes | QMessageBox::Cancel,
        QMessageBox::Cancel
    );
    if (ret != QMessageBox::Yes)
        return;

    QDir dir(tempDir);
    bool ok = dir.removeRecursively();
    if (ok) {
        QMessageBox::information(&window, "Delete Temporary Files", "Temporary files deleted.");
    } else {
        QMessageBox::warning(&window, "Delete Temporary Files", "Failed to delete some or all temporary files.");
    }
};


QObject::connect(dnsCacheAction, &QAction::triggered, [&window]() {
    showDnsCacheDialog(&window);
});

QObject::connect(routeTableAction, &QAction::triggered, [&window]() {
    showRouteTableDialog(&window);
});

QObject::connect(netstatStatsAction, &QAction::triggered, [&window]() {
    showNetstatStatisticsDialog(&window);
});

// Connect the menu action
QObject::connect(deleteTempAction, &QAction::triggered, deleteTempFiles);


// Do NOT call setCheckable(true)!

auto updateAlwaysOnTopText = [&]() {
    if (window.windowFlags() & Qt::WindowStaysOnTopHint) {
        alwaysOnTopAction->setText("🟢 Always on top");
    } else {
        alwaysOnTopAction->setText("◯ Always on top");
    }
};
updateAlwaysOnTopText();


QObject::connect(hostsFileAction, &QAction::triggered, [&]() { showHostsFileEditor(&window); });

QObject::connect(mtuDiscoveryAction, &QAction::triggered, [&]() {
    showMtuDiscoveryDialog(&window);
});


QObject::connect(sslCertAction, &QAction::triggered, [&window]() {
    showSslCertificateDialog(&window);
});


QObject::connect(adaptersAction, &QAction::triggered, [&]() {
    showNetworkAdaptersDialog(&window);
});

QObject::connect(alwaysOnTopAction, &QAction::triggered, [&]() {
    bool onTop = !(window.windowFlags() & Qt::WindowStaysOnTopHint);
    window.setWindowFlag(Qt::WindowStaysOnTopHint, onTop);
    window.show();
    updateAlwaysOnTopText();
});

QObject::connect(wifiScanAction, &QAction::triggered, [&]() { showWifiScanDialog(&window); });


QObject::connect(arpAction, &QAction::triggered, [&]() {
    showArpDialog(&window);
});

QObject::connect(netUsageAction, &QAction::triggered, [&]() {
    showNetUsageDialog(&window);
});

// Connect the NS Lookup action
    QObject::connect(nslookupAction, &QAction::triggered, [&]() {
    showNslookupDialog(&window); // or your main window pointer
    });

// Connect the DHCP Status action
QObject::connect(dhcpStatusAction, &QAction::triggered, [&]() {
    showDhcpStatusDialog(&window); // or your main window pointer
});


QObject::connect(portscanAction, &QAction::triggered, [&]() {
    showPortScanDialog(&window);
});

QObject::connect(netscanAction, &QAction::triggered, [&]() {
    showNetworkScannerDialog(&window);
});

QObject::connect(pingAction, &QAction::triggered, [&]() {
    QDialog dlg(&window);

    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), &dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, &dlg, &QDialog::accept);

    dlg.setWindowTitle("Ping Host");
    // Remove the Close (X) button from the window frame
    dlg.setWindowFlags((dlg.windowFlags() & ~Qt::WindowCloseButtonHint) | Qt::Dialog | Qt::WindowTitleHint);

    QVBoxLayout *pingLayout = new QVBoxLayout(&dlg);

    QLabel prompt("Enter host or IP to ping:");
    pingLayout->addWidget(&prompt);

    QLineEdit input;
    input.setPlaceholderText("e.g. 8.8.8.8 or www.google.com");
    pingLayout->addWidget(&input);

    // Counter label
    QLabel counterLabel;
    int pingCount = 0;
    counterLabel.setText("<span style='color:blue;'>Pings: 0</span>");
    pingLayout->addWidget(&counterLabel, 0, Qt::AlignLeft);

    QHBoxLayout btnLayout;
    QPushButton pingBtn("Start");
    pingBtn.setToolTip("Start the ping to the specified host.");
    QPushButton stopCloseBtn("Close");
    stopCloseBtn.setToolTip("Close the dialog.");
    QPushButton bottomBtn("Bottom");
    bottomBtn.setToolTip("Scroll to the bottom of the output.");
    btnLayout.addWidget(&pingBtn);
    btnLayout.addWidget(&stopCloseBtn);
    btnLayout.addWidget(&bottomBtn);
    pingLayout->addLayout(&btnLayout);

    QTextEdit output;
    output.setReadOnly(true);
    output.setLineWrapMode(QTextEdit::NoWrap);
    output.setMinimumHeight(80);
    pingLayout->addWidget(&output);

    // Connect Bottom button to scroll to bottom
    QObject::connect(&bottomBtn, &QPushButton::clicked, [&output]() {
        output.verticalScrollBar()->setValue(output.verticalScrollBar()->maximum());
    });

    QTimer pingTimer;
    QProcess *pingProc = nullptr;
    bool isPinging = false;

    // --- Auto-scroll support ---
    QTimer autoScrollTimer;
    autoScrollTimer.setSingleShot(true);
    QScrollBar *vScroll = output.verticalScrollBar();
    bool userIsScrolling = false;

    QObject::connect(vScroll, &QScrollBar::sliderPressed, [&]() {
        userIsScrolling = true;
        autoScrollTimer.stop();
    });
    QObject::connect(vScroll, &QScrollBar::sliderReleased, [&]() {
        userIsScrolling = false;
        autoScrollTimer.start(3000); // Start 3s timer after user lets go
    });

    QObject::connect(&autoScrollTimer, &QTimer::timeout, [&]() {
        if (!userIsScrolling && isPinging) {
            output.verticalScrollBar()->setValue(output.verticalScrollBar()->maximum());
        }
    });
    // --- End auto-scroll support ---

    // Helper to update Stop/Close button text
    auto updateStopCloseText = [&]() {
    if (isPinging) {
        stopCloseBtn.setText("Stop");
        stopCloseBtn.setToolTip("Stop the ping");
    } else {
        stopCloseBtn.setText("Close");
        stopCloseBtn.setToolTip("Close the dialog");
    }
    };
    updateStopCloseText();

    // Use this function to stop pinging
    auto stopPinging = [&]() {
        pingTimer.stop();
        if (pingProc) {
            pingProc->kill();
            pingProc->deleteLater();
            pingProc = nullptr;
        }
        isPinging = false;
        pingBtn.setEnabled(true);
        updateStopCloseText();
    };

    // Connect Start button
    QObject::connect(&pingBtn, &QPushButton::clicked, [&]() {
    QString host = input.text().trimmed();
    if (host.isEmpty()) {
        QMessageBox::warning(&dlg, "Input Error", "Please enter a host or IP address to ping.");
        return;
    }
    isPinging = true;
    updateStopCloseText();
    pingBtn.setEnabled(false);
    pingCount = 0;
    output.clear();
    counterLabel.setText("<span style='color:blue;'>Pings: 0</span>");
    pingTimer.start(1000); // Start pinging every second (adjust as needed)
});

// Connect Stop/Close button
QObject::connect(&stopCloseBtn, &QPushButton::clicked, [&]() {
    if (isPinging) {
        stopPinging();
        output.append("<b>Ping stopped.</b>");
    } else {
        dlg.accept();
    }
});

    QObject::connect(&pingTimer, &QTimer::timeout, [&]() {
        if (!isPinging) return;
        QString host = input.text().trimmed();
        if (host.isEmpty()) return;

        if (pingProc) {
            pingProc->kill();
            pingProc->deleteLater();
            pingProc = nullptr;
        }
        pingProc = new QProcess(&dlg);
        QObject::connect(pingProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [&output, &host, &pingCount, &counterLabel, &dlg, pingProc](int, QProcess::ExitStatus) {

            QString result = pingProc->readAllStandardOutput();
            ++pingCount;
            counterLabel.setText(QString("<span style='color:blue;'>Pings: %1</span>").arg(pingCount));
            QStringList lines = result.trimmed().split('\n');
            if (!lines.isEmpty()) {
                output.append(QString("<span style='color:blue;'>[%1]</span> %2").arg(pingCount).arg(lines.first().trimmed()));
                for (int i = 1; i < lines.size(); ++i)
                    output.append(lines[i].trimmed());
            }

            // --- Auto-resize ping dialog to fit long lines ---
            QFontMetrics fm(output.font());
            int maxLineWidth = 0;
            for (const QString &line : lines) {
                int width = fm.horizontalAdvance(line);
                if (width > maxLineWidth)
                    maxLineWidth = width;
            }
            int margin = 80; // Add some margin for scrollbars and padding
            int minWidth = 400; // Minimum width for the dialog
            int newWidth = qMax(minWidth, maxLineWidth + margin);
            if (dlg.width() < newWidth)
                dlg.resize(newWidth, dlg.height());
            // --- End auto-resize ---
        });
        pingProc->start("ping", QStringList() << "-n" << "1" << host);
    });

    dlg.adjustSize();
    dlg.exec();
    stopPinging();
}); 


QObject::connect(tracertAction, &QAction::triggered, [&]() {
    showTracerouteDialog(&window);
});
    
//Add block for release IP
QObject::connect(releaseBtn, &QPushButton::clicked, [&]() {
    // Show working dialog with animation
QDialog workingDlg(&window);
workingDlg.setWindowTitle("Working...");
workingDlg.setFixedSize(150, 150); // 1.5x the original size

QVBoxLayout vbox(&workingDlg);
QLabel label("Releasing IP address.");
vbox.addWidget(&label);

QLabel animLabel;
animLabel.setFixedSize(96, 96); // 1.5x the original 64x64
animLabel.setScaledContents(true); // Force the GIF to scale to label size
QMovie movie("StdWorking.gif");
animLabel.setMovie(&movie);
vbox.addWidget(&animLabel, 0, Qt::AlignHCenter);
movie.start();
    workingDlg.setModal(true);
    workingDlg.setWindowFlags(workingDlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    workingDlg.show();
    QCoreApplication::processEvents();

    // Start the release command
    QString command = "Start-Process ipconfig -ArgumentList '/release' -Verb runAs -WindowStyle Hidden";
    QProcess::startDetached("powershell", QStringList() << "-WindowStyle" << "Hidden" << "-Command" << command);

    // Poll for up to 10 seconds for APIPA to appear, using QTimer
    int pollCount = 0;
    const int maxPolls = 100; // 100 x 100ms = 10 seconds
    bool gotApipa = false;

    QTimer pollTimer;
    QObject::connect(&pollTimer, &QTimer::timeout, [&]() {
        ++pollCount;
        QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
        for (const QNetworkInterface &iface : interfaces) {
            if (!(iface.flags() & QNetworkInterface::IsUp) ||
                !(iface.flags() & QNetworkInterface::IsRunning) ||
                (iface.flags() & QNetworkInterface::IsLoopBack))
                continue;
            for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isLoopback()) {
                    if (entry.ip().toString().startsWith("169.254.")) {
                        gotApipa = true;
                        break;
                    }
                }
            }
            if (gotApipa) break;
        }
        if (gotApipa || pollCount >= maxPolls) {
            pollTimer.stop();
            workingDlg.accept(); // Close the dialog
            updateIpDisplay(infoBox);
            QMessageBox::information(&window, "IP Released",
                gotApipa
                ? "Your IP address has been released and Windows assigned an autoconfiguration address."
                : "Your IP address has been released.");
        }
    });
    pollTimer.start(100);
    workingDlg.exec(); // Block here until dialog is closed
});

// Add this block for Renew IP:
QObject::connect(renewBtn, &QPushButton::clicked, [&]() {
    // Show working dialog with animation
    QDialog workingDlg(&window);
    workingDlg.setWindowTitle("Working...");
    workingDlg.setFixedSize(150, 150);

    QVBoxLayout vbox(&workingDlg);
    QLabel label("Renewing IP address.");
    vbox.addWidget(&label);

    QLabel animLabel;
    animLabel.setFixedSize(96, 96);
    animLabel.setScaledContents(true);
    QMovie movie("StdWorking.gif");
    animLabel.setMovie(&movie);
    vbox.addWidget(&animLabel, 0, Qt::AlignHCenter);
    movie.start();

    workingDlg.setModal(true);
    workingDlg.setWindowFlags(workingDlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    workingDlg.show();
    QCoreApplication::processEvents();

    // Start the renew command
    QString command = "Start-Process ipconfig -ArgumentList '/renew' -Verb runAs -WindowStyle Hidden";
    QProcess::startDetached("powershell", QStringList() << "-WindowStyle" << "Hidden" << "-Command" << command);

    // Poll for up to 10 seconds for a non-APIPA IPv4 address to appear
    int pollCount = 0;
    const int maxPolls = 100;
    bool gotDhcp = false;

    QTimer pollTimer;
    QObject::connect(&pollTimer, &QTimer::timeout, [&]() {
        ++pollCount;
        QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
        for (const QNetworkInterface &iface : interfaces) {
            if (!(iface.flags() & QNetworkInterface::IsUp) ||
                !(iface.flags() & QNetworkInterface::IsRunning) ||
                (iface.flags() & QNetworkInterface::IsLoopBack))
                continue;
            for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
                if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isLoopback()) {
                    if (!entry.ip().toString().startsWith("169.254.")) {
                        gotDhcp = true;
                        break;
                    }
                }
            }
            if (gotDhcp) break;
        }
        if (gotDhcp || pollCount >= maxPolls) {
            pollTimer.stop();
            workingDlg.accept();
            updateIpDisplay(infoBox);
            QMessageBox::information(&window, "IP Renewed",
                gotDhcp
                ? "Your IP address has been renewed successfully."
                : "Renew timed out or failed.");
        }
    });
    pollTimer.start(100);
    workingDlg.exec();
});

    QAction *exitAction = fileMenu->addAction("E&xit");
    QObject::connect(exitAction, &QAction::triggered, &window, &QWidget::close);

    // Help menu
    QMenu *helpMenu = menuBar->addMenu("&Help");
    QAction *manualAction = helpMenu->addAction("User Manual (PDF)");
    QAction *aboutAction = helpMenu->addAction("&About");

    QObject::connect(manualAction, &QAction::triggered, [&window]() {
    QString dir = "C:/Users/Public/AppData/Local/IPGui";
    QString localPath = dir + "/IPGuiManual.pdf";
    QString url = "https://prog.nalle.no/user/data/manual/IPGuiManual.pdf";

    // Ensure directory exists
    QDir().mkpath(dir);

    // Helper: get remote file timestamp (HTTP HEAD)
    auto getRemoteTimestamp = [](const QString &url) -> QDateTime {
        QNetworkAccessManager mgr;
        QNetworkRequest req(url);
        QNetworkReply *reply = mgr.head(req);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(5000);
        loop.exec();
        QDateTime dt;
        if (reply->isFinished() && reply->error() == QNetworkReply::NoError) {
            QByteArray lastMod = reply->rawHeader("Last-Modified");
            if (!lastMod.isEmpty()) {
                dt = QDateTime::fromString(QString::fromLatin1(lastMod), Qt::RFC2822Date);
                dt.setTimeSpec(Qt::UTC);
            }
        }
        reply->deleteLater();
        return dt;
    };

    // Helper: download file
    auto downloadManual = [&](const QString &url, const QString &path) -> bool {
        QNetworkAccessManager mgr;
        QNetworkRequest req(url);
        QNetworkReply *reply = mgr.get(req);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(15000);
        loop.exec();
        bool ok = false;
        if (reply->isFinished() && reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QFile file(path);
            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                file.write(data);
                file.close();
                ok = true;
            }
        }
        reply->deleteLater();
        return ok;
    };

    QFileInfo fi(localPath);

    // Always try to download the latest manual before opening
    bool downloaded = downloadManual(url, localPath);

    if (!downloaded && !fi.exists()) {
        QMessageBox::warning(&window, "Manual", "Could not download the manual and no local copy exists.");
        return;
    }

    // Open the PDF with the default viewer
    QDesktopServices::openUrl(QUrl::fromLocalFile(localPath));
});


   QObject::connect(aboutAction, &QAction::triggered, &window, [&window]() {
    QDialog dlg(&window);
    dlg.setWindowTitle("About");
    QVBoxLayout layout(&dlg);

    // Add icon (adjust path as needed)
    QLabel *iconLabel = new QLabel;
    iconLabel->setPixmap(QPixmap("ip-address.png").scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout.addWidget(iconLabel, 0, Qt::AlignHCenter);

    QLabel label(
        "<b>IPGui by Nalle Berg</b><br>"
        "<b>Copyleft 2025</b><br><br>"
        "A simple IP lookup/renew -tool.<br>"
        "Including network tools of most kinds.<br>"
        "Visit my programming <a href='https://prog.nalle.no'> web page</a>.<br>"
        "&nbsp;<br>"
        "<b>Version:</b> " + VersionNumber + "<br>"
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

    window.resize(255, 330);
    window.show();

QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), &window);
closeShortcut->setContext(Qt::ApplicationShortcut);
QObject::connect(closeShortcut, &QShortcut::activated, [&window]() {
    auto ret = QMessageBox::question(
        &window,
        "Exit IPGui",
        "Are you sure you want to quit?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes // <-- Default is now Yes
    );
    if (ret == QMessageBox::Yes)
        window.close();
});
    

    return app.exec();
}