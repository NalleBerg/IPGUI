// License: This project is licensed under the GNU General Public License v2.0 (GPL-2.0).
// Project author: Nalle Berg
// Project name: IPGui
// Project description: A simple IP lookup/renew tool for Windows.
// Project version: 3.0.0
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


// Windows API for gateway
#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")


//Global variables
const QString VersionNumber = "3.0.0";
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
                //  I do that, so I can add a link to the device 
                //  if it has a web interface.
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
    // bottomBtn->setEnabled(false);
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

void showNslookupDialog(QWidget *parent = nullptr) {
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
    QPushButton cancelBtn("Cancel");
    cancelBtn.setToolTip("Cancel and close this dialog.");
    btnBox.addWidget(&okBtn);
    btnBox.addWidget(&cancelBtn);
    vbox.addLayout(&btnBox);

    okBtn.setEnabled(false);

    QObject::connect(&inputEdit, &QLineEdit::textChanged, [&]() {
        okBtn.setEnabled(!inputEdit.text().trimmed().isEmpty());
    });
    QObject::connect(&okBtn, &QPushButton::clicked, &inputDlg, &QDialog::accept);
    QObject::connect(&cancelBtn, &QPushButton::clicked, &inputDlg, &QDialog::reject);

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

    dlg.adjustSize();
    dlg.exec();
}

void showArpDialog(QWidget *parent = nullptr) {
    QDialog dlg(parent);
    dlg.setWindowTitle("ARP Table");

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
QAction *pingAction = netToolsMenu->addAction("Ping...");
QAction *netscanAction = netToolsMenu->addAction("IP Scanner...");
QAction *nslookupAction = netToolsMenu->addAction("NS Lookup...");
QAction *tracertAction = netToolsMenu->addAction("Traceroute...");
QAction *portscanAction = netToolsMenu->addAction("Port Scan...");
QAction *dhcpStatusAction = netToolsMenu->addAction("DHCP Status...");
QAction *netUsageAction = netToolsMenu->addAction("Network Usage...");
QAction *arpAction = netToolsMenu->addAction("Arp...");
QAction *wifiScanAction = netToolsMenu->addAction("WiFi Scan...");
QAction *adaptersAction = netToolsMenu->addAction("Network Adapters");


QAction *alwaysOnTopAction = fileMenu->addAction("🔵 Always on top"); // Dot first, no checkmark

// Do NOT call setCheckable(true)!

auto updateAlwaysOnTopText = [&]() {
    if (window.windowFlags() & Qt::WindowStaysOnTopHint) {
        alwaysOnTopAction->setText("🟢 Always on top");
    } else {
        alwaysOnTopAction->setText("◯ Always on top");
    }
};
updateAlwaysOnTopText();


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
    bool needDownload = !fi.exists();

    // Check remote timestamp if file exists
    if (!needDownload) {
        QDateTime remote = getRemoteTimestamp(url);
        if (remote.isValid() && fi.lastModified().toUTC() < remote) {
            needDownload = true;
        }
    }

    if (needDownload) {
        if (!downloadManual(url, localPath)) {
            QMessageBox::warning(&window, "Manual", "Could not download the latest manual.");
            return;
        }
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

    return app.exec();
}