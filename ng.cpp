// License: This project is licensed under the GNU General Public License v2.0 (GPL-2.0).
// Project author: Nalle Berg
// Project name: IPGui
// Project description: A simple IP lookup/renew tool for Windows.
// Project version: 2.4.0
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



// Windows API for gateway
#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")


//Global variables
const QString VersionNumber = "2.4.6";
const QString html = QString("<b>Version:</b> %1<br>").arg(VersionNumber);


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


void showNetworkScannerDialog(QWidget *parent) {
    QDialog dlg(parent);
    dlg.setWindowTitle("Network Scanner");
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
    QPushButton *stopBtn = new QPushButton("Stop");
    QPushButton *closeBtn = new QPushButton("Close");
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


void showPortScanDialog(QWidget *parent) {
    QDialog dlg(parent);
    dlg.setWindowTitle("Port Scan");
    QVBoxLayout layout(&dlg);
    QLabel label("Port scan not implemented yet.");
    layout.addWidget(&label);
    QPushButton ok("OK");
    layout.addWidget(&ok);
    QObject::connect(&ok, &QPushButton::clicked, &dlg, &QDialog::accept);
    dlg.exec();
}

void showTracerouteDialog(QWidget *parent) {
    QDialog dlg(parent);
    dlg.setWindowTitle("Traceroute Host");
    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLabel *prompt = new QLabel("Enter host or IP to trace:");
    layout->addWidget(prompt);

    QLineEdit *input = new QLineEdit;
    input->setPlaceholderText("e.g. 8.8.8.8 or www.google.com");
    layout->addWidget(input);

    QHBoxLayout *hopsLayout = new QHBoxLayout();
    QLabel *hopsLabel = new QLabel("Max hops:");
    QSpinBox *hopsSpin = new QSpinBox;
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
    QPushButton *stopBtn = new QPushButton("Stop");
    QPushButton *bottomBtn = new QPushButton("Bottom");
    QPushButton *closeBtn = new QPushButton("Close");
    btnLayout->addWidget(traceBtn);
    btnLayout->addWidget(stopBtn);
    btnLayout->addWidget(bottomBtn);
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    stopBtn->setEnabled(false);

    QPointer<QProcess> lastProc = nullptr;
    QPointer<QDialog> scanningDlg = nullptr;

    QObject::connect(traceBtn, &QPushButton::clicked, [&]() {
        QString host = input->text().trimmed();
        int hops = hopsSpin->value();
        if (host.isEmpty()) {
            output->setPlainText("Please enter a host or IP address.");
            return;
        }
        traceBtn->setEnabled(false);
        stopBtn->setEnabled(true);
        output->clear();

        if (lastProc) {
            lastProc->kill();
            lastProc->deleteLater();
            lastProc = nullptr;
        }

        // Show scanning dialog
        if (!scanningDlg) {
            scanningDlg = new QDialog(&dlg, Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
            scanningDlg->setWindowTitle("Scanning...");
            scanningDlg->setFixedSize(150, 150);

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
            scanningDlg->setWindowFlags(scanningDlg->windowFlags() & ~Qt::WindowContextHelpButtonHint | Qt::WindowCloseButtonHint);
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

        QObject::connect(traceProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [=]() {
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
            stopBtn->setEnabled(false);
            traceProc->deleteLater();
            if (scanningDlg) scanningDlg->close();
        });

        QObject::connect(stopBtn, &QPushButton::clicked, [=]() {
            traceProc->kill();
            traceProc->deleteLater();
            traceBtn->setEnabled(true);
            stopBtn->setEnabled(false);
            if (scanningDlg) scanningDlg->close();
        });

        // If user closes the scanning dialog, just hide it (scan continues)
        QObject::connect(scanningDlg, &QDialog::rejected, [=]() {
            scanningDlg->hide();
        });

        QString cmd = QString("tracert -h %1 %2").arg(hops).arg(host);
        traceProc->start("cmd", QStringList() << "/c" << cmd);
    });

    QObject::connect(bottomBtn, &QPushButton::clicked, [&]() {
        output->verticalScrollBar()->setValue(output->verticalScrollBar()->maximum());
    });

    QObject::connect(closeBtn, &QPushButton::clicked, [&]() { dlg.close(); });

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
    bool ok;
    QString host = QInputDialog::getText(parent, "NS Lookup", "Enter hostname or IP:", QLineEdit::Normal, "", &ok);
    if (!ok || host.trimmed().isEmpty())
        return;

    QProcess proc;
    proc.start("nslookup", QStringList() << host.trimmed());
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

    QDialog dlg(parent);
    dlg.setWindowTitle("NS Lookup Result");
    QVBoxLayout layout(&dlg);

    QLabel *label = new QLabel(info);
    label->setTextFormat(Qt::RichText);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout.addWidget(label);

    QPushButton *closeBtn = new QPushButton("Close");
    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout.addWidget(closeBtn);

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

// Submenu for app-specific actions
QMenu *appMenu = fileMenu->addMenu("This app");

// Add NetTools submenu
QMenu *netToolsMenu = fileMenu->addMenu("NetTools");

// Add menu actions for Basic/Advanced, Release, Renew to the submenu
QAction *expandMenuAction = appMenu->addAction("Basic/Advanced");
QAction *flushDnsAction = appMenu->addAction("Flush DNS");
flushDnsAction->setToolTip("Flush the Windows DNS cache");
QAction *releaseMenuAction = appMenu->addAction("Release IP");
QAction *renewMenuAction = appMenu->addAction("Renew IP");

// Add action to NetTools submenu
QAction *pingAction = netToolsMenu->addAction("Ping...");
QAction *netscanAction = netToolsMenu->addAction("IP Scanner...");
QAction *nslookupAction = netToolsMenu->addAction("NS Lookup...");
QAction *tracertAction = netToolsMenu->addAction("Traceroute...");
QAction *portscanAction = netToolsMenu->addAction("Port Scan...");
QAction *dhcpStatusAction = netToolsMenu->addAction("DHCP Status...");

// Add "Always on top" toggle
QAction *alwaysOnTopAction = fileMenu->addAction("Always on top");
alwaysOnTopAction->setCheckable(true);
QObject::connect(alwaysOnTopAction, &QAction::toggled, &window, [&](bool checked) {
    window.setWindowFlag(Qt::WindowStaysOnTopHint, checked);
    window.show();
});

QObject::connect(flushDnsAction, &QAction::triggered, [&]() {
    int result = QProcess::execute("ipconfig", QStringList() << "/flushdns");
    if (result == 0)
        QMessageBox::information(nullptr, "Flush DNS", "DNS cache flushed.");
    else
        QMessageBox::warning(nullptr, "Flush DNS", "Failed to flush DNS cache.");
});

// Connect the NS Lookup action
    QObject::connect(nslookupAction, &QAction::triggered, [&]() {
    showNslookupDialog(&window); // or your main window pointer
    });

// Connect the DHCP Status action
QObject::connect(dhcpStatusAction, &QAction::triggered, [&]() {
    showDhcpStatusDialog(&window); // or your main window pointer
});

// Connect menu actions to the same slots/lambdas as the buttons
QObject::connect(expandMenuAction, &QAction::triggered, [&]() { expandBtn->click(); });
QObject::connect(releaseMenuAction, &QAction::triggered, [&]() { releaseBtn->click(); });
QObject::connect(renewMenuAction, &QAction::triggered, [&]() { renewBtn->click(); });

QObject::connect(portscanAction, &QAction::triggered, [&]() {
    showPortScanDialog(&window);
});

QObject::connect(netscanAction, &QAction::triggered, [&]() {
    showNetworkScannerDialog(&window);
});

QObject::connect(pingAction, &QAction::triggered, [&]() {
    QDialog dlg(&window);
    dlg.setWindowTitle("Ping Host");
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
    QPushButton stopBtn("Stop");
    QPushButton bottomBtn("Bottom"); // New button
    QPushButton closeBtn("Close");
    btnLayout.addWidget(&pingBtn);
    btnLayout.addWidget(&stopBtn);
    btnLayout.addWidget(&bottomBtn); // Add between Stop and Close
    btnLayout.addWidget(&closeBtn);
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

    
    auto stopPinging = [&]() {
        pingTimer.stop();
        if (pingProc) {
            pingProc->kill();
            pingProc->deleteLater();
            pingProc = nullptr;
        }
        isPinging = false;
        pingBtn.setEnabled(true);
        stopBtn.setEnabled(false);
    };

    QObject::connect(&pingBtn, &QPushButton::clicked, [&]() {
    QString host = input.text().trimmed();
    if (host.isEmpty()) {
        output.setPlainText("Please enter a host or IP address.");
        return;
    }
    output.clear();
    pingBtn.setEnabled(false);
    stopBtn.setEnabled(true);
    isPinging = true;
    pingCount = 0;
    counterLabel.setText("<span style='color:blue;'>Pings: 0</span>");
    pingTimer.start(1000);
    QMetaObject::invokeMethod(&pingTimer, "timeout");
});

    QObject::connect(&stopBtn, &QPushButton::clicked, stopPinging);
    QObject::connect(&closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

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

        stopBtn.setEnabled(false);
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
    QAction *aboutAction = helpMenu->addAction("&About");
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
        "Visit my programming <a href='https://prog.nalle.no'> web page</a>.<br>"
        "&nbsp;<br>"
        "<b>Version:</b> 2.3.0<br>"
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