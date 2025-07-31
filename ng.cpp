// License: This project is licensed under the GNU General Public License v2.0 (GPL-2.0).
// Project author: Nalle Berg
// Project name: IPGui
// Project description: A simple IP lookup/renew tool for Windows.
// Project version: 3.8.7
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
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QClipboard>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QMutex>
#include <QStyledItemDelegate>
#include <QMouseEvent>
#include <QPainter> // Required for QStyledItemDelegate paint()
#include <QDebug>



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
const QString VersionNumber = "3.8.7";
const QString html = QString("<b>Version:</b> %1<br>").arg(VersionNumber);

// Declaring functions for port scanner dialog
void showPortScanDialog(QWidget *parent, const QString &initialTarget = QString());
void showLanSharesDialog(QWidget *parent, const QString &singleTarget = QString());

// Helper: Belong to showNworkScannerDialog
class LinkDelegate : public QStyledItemDelegate {
public:
    LinkDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QString ip = index.data().toString();
        bool hasHttps = index.data(Qt::UserRole + 1).toBool();
        bool hasHttp = index.data(Qt::UserRole + 2).toBool();
        painter->save();
        QRect rect = option.rect;
        QFont font = option.font;
        if (hasHttps || hasHttp) {
            font.setUnderline(true);
            painter->setFont(font);
            painter->setPen(QColor("#1c2684"));
        } else {
            painter->setFont(font);
            painter->setPen(QColor("#222"));
        }
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, ip);
        painter->restore();
    }

    bool editorEvent(QEvent *event, QAbstractItemModel *model,
                     const QStyleOptionViewItem &option, const QModelIndex &index) override {
        bool hasHttps = index.data(Qt::UserRole + 1).toBool();
        bool hasHttp = index.data(Qt::UserRole + 2).toBool();
        QString ip = index.data().toString();
        if (hasHttps || hasHttp) {
            if (event->type() == QEvent::MouseButtonRelease) {
                if (hasHttps)
                    QDesktopServices::openUrl(QUrl(QString("https://%1").arg(ip)));
                else if (hasHttp)
                    QDesktopServices::openUrl(QUrl(QString("http://%1").arg(ip)));
                QApplication::restoreOverrideCursor();
                return true;
            }
        }
        return false;
    }

    bool helpEvent(QHelpEvent *event, QAbstractItemView *view,
                   const QStyleOptionViewItem &option, const QModelIndex &index) override {
        bool hasHttps = index.data(Qt::UserRole + 1).toBool();
        bool hasHttp = index.data(Qt::UserRole + 2).toBool();
        if (hasHttps || hasHttp) {
            QApplication::setOverrideCursor(Qt::PointingHandCursor);
        } else {
            QApplication::restoreOverrideCursor();
        }
        return QStyledItemDelegate::helpEvent(event, view, option, index);
    }
};

// Helper: Enumerate shares on a host using NetShareEnum
QList<QList<QString>> getSharesOnHost(const QString &host) {
    QList<QList<QString>> shares;
    SHARE_INFO_1 *pBuf = nullptr;
    DWORD entriesRead = 0, totalEntries = 0;
    NET_API_STATUS nStatus = NetShareEnum(
        (wchar_t*)host.utf16(), 1, (LPBYTE*)&pBuf, MAX_PREFERRED_LENGTH, &entriesRead, &totalEntries, nullptr);
    if (nStatus == NERR_Success && pBuf) {
        SHARE_INFO_1 *p = pBuf;
        for (DWORD i = 0; i < entriesRead; ++i, ++p) {
            QString name = QString::fromWCharArray(p->shi1_netname);
            QString type = (p->shi1_type == STYPE_DISKTREE) ? "Disk" :
                           (p->shi1_type == STYPE_PRINTQ) ? "Printer" :
                           (p->shi1_type == STYPE_DEVICE) ? "Device" :
                           (p->shi1_type == STYPE_IPC) ? "IPC" : "Other";
            QString comment = p->shi1_remark ? QString::fromWCharArray(p->shi1_remark) : "";
            shares.append({name, type, comment});
        }
        NetApiBufferFree(pBuf);
    }
    return shares;
}

QString getPortInfoCsvPath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty())
        dir = QDir::homePath() + "/AppData/Local/IPGui";
    QDir().mkpath(dir);
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

void showNetworkScannerDialog(QWidget *parent) {
    QDialog *dlg = new QDialog(parent);
    dlg->setWindowTitle("IP Scanner");
    dlg->setMinimumWidth(525);
    dlg->setMaximumWidth(525);
    dlg->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint);

    QVBoxLayout *layout = new QVBoxLayout(dlg);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    QLabel *prompt = new QLabel("Scan for devices in your local network:");
    prompt->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(prompt);

    layout->addSpacing(20);

    QHBoxLayout *ipRangeLayout = new QHBoxLayout();
    ipRangeLayout->setSpacing(0);
    ipRangeLayout->setContentsMargins(0, 0, 0, 0);
    QLabel *fromLabel = new QLabel("<b>From:</b>");
    QLineEdit *fromEdit = new QLineEdit;
    QLabel *toLabel = new QLabel("<b>To:</b>");
    QLineEdit *toEdit = new QLineEdit;
    ipRangeLayout->addWidget(fromLabel);
    ipRangeLayout->addWidget(fromEdit);
    ipRangeLayout->addSpacing(8);
    ipRangeLayout->addWidget(toLabel);
    ipRangeLayout->addWidget(toEdit);
    layout->addLayout(ipRangeLayout);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(0);
    btnLayout->setContentsMargins(0, 0, 0, 0);
    QPushButton *scanBtn = new QPushButton("Scan");
    QPushButton *stopCloseBtn = new QPushButton("Close");
    btnLayout->addStretch();
    btnLayout->addWidget(scanBtn);
    btnLayout->addWidget(stopCloseBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    QProgressBar *progress = new QProgressBar;
    progress->setMinimum(0);
    progress->setMaximum(254);
    progress->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    progress->setFixedHeight(14);
    progress->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(progress);

    QLabel *foundLabel = new QLabel("Devices found: 0");
    foundLabel->setAlignment(Qt::AlignLeft);
    foundLabel->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(foundLabel);

    layout->addSpacing(20);

    QTableWidget *deviceTable = new QTableWidget();
    deviceTable->setColumnCount(3);
    deviceTable->setRowCount(0);
    deviceTable->setHorizontalHeaderLabels(QStringList() << "IP Address" << "Host Name" << "Actions");
    deviceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    deviceTable->verticalHeader()->setVisible(false);
    deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    deviceTable->setSelectionMode(QAbstractItemView::NoSelection);

    // --- Only add scrollbars, do not change any sizes ---
    deviceTable->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    deviceTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    // ----------------------------------------------------

    deviceTable->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    deviceTable->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(deviceTable);

    deviceTable->setItemDelegateForColumn(0, new LinkDelegate(deviceTable));

    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    // State for scan
    auto scanRunning = std::make_shared<bool>(false);
    auto cancelRequested = std::make_shared<bool>(false);

    // --- Stop/Close button text logic ---
    auto updateStopCloseBtn = [&]() {
        if (*scanRunning) {
            stopCloseBtn->setText("Stop");
            stopCloseBtn->setToolTip("Stop the scan");
            closeShortcut->setEnabled(false);
        } else {
            stopCloseBtn->setText("Close");
            stopCloseBtn->setToolTip("Close this dialog");
            closeShortcut->setEnabled(true);
        }
    };

    // Autofill IP range
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
    toEdit->setText(defaultBase + ".254");

    QThreadPool *pool = new QThreadPool(dlg);
    pool->setMaxThreadCount(16);

    // Store found devices for sorting and updating
    struct DeviceInfo {
        QString ip;
        QString host;
        bool hasHttp;
        bool hasHttps;
    };
    auto foundDevices = std::make_shared<QList<DeviceInfo>>();

    QObject::connect(scanBtn, &QPushButton::clicked, [=]() {
        QString fromIp = fromEdit->text().trimmed();
        QString toIp = toEdit->text().trimmed();

        QHostAddress fromAddr(fromIp), toAddr(toIp);
        if (fromAddr.protocol() != QAbstractSocket::IPv4Protocol ||
            toAddr.protocol() != QAbstractSocket::IPv4Protocol) {
            QMessageBox::warning(dlg, "Input Error", "Invalid IP address format.");
            return;
        }
        quint32 from = fromAddr.toIPv4Address();
        quint32 to = toAddr.toIPv4Address();
        if (from > to) std::swap(from, to);

        int total = to - from + 1;
        progress->setMaximum(total);
        progress->setValue(0);
        foundLabel->setText("Devices found: 0");
        deviceTable->setRowCount(0);

        *scanRunning = true;
        *cancelRequested = false;
        scanBtn->setEnabled(false);
        updateStopCloseBtn();

        foundDevices->clear();
        auto foundCount = std::make_shared<int>(0);
        auto progressCount = std::make_shared<int>(0);
        auto finishedCount = std::make_shared<int>(0);
        auto tableMutex = std::make_shared<QMutex>();

        QList<QString> ipList;
        for (quint32 ipInt = from; ipInt <= to; ++ipInt)
            ipList << QHostAddress(ipInt).toString();

        for (int i = 0; i < ipList.size(); ++i) {
            pool->start([=]() {
                if (*cancelRequested) return;
                QString ip = ipList[i];
                QProcess ping;
                ping.start("ping", QStringList() << "-n" << "1" << "-w" << "100" << ip);
                ping.waitForFinished(300);
                QString result = ping.readAllStandardOutput();
                bool alive = result.contains("TTL=");
                if (alive) {
                    QString host = QHostInfo::fromName(ip).hostName();

                    // HTTP/HTTPS check
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

                    QMutexLocker locker(tableMutex.get());
                    foundDevices->append(DeviceInfo{ip, host, hasHttp, hasHttps});

                    // Sort by IP address numerically
                    QList<DeviceInfo> sorted = *foundDevices;
                    std::sort(sorted.begin(), sorted.end(), [](const DeviceInfo &a, const DeviceInfo &b) {
                        return QHostAddress(a.ip).toIPv4Address() < QHostAddress(b.ip).toIPv4Address();
                    });

                    QMetaObject::invokeMethod(deviceTable, [=]() {
                        deviceTable->setRowCount(0);
                        for (int idx = 0; idx < sorted.size(); ++idx) {
                            const DeviceInfo &dev = sorted[idx];
                            int row = deviceTable->rowCount();
                            deviceTable->insertRow(row);

                            QTableWidgetItem *ipItem = new QTableWidgetItem(dev.ip);
                            ipItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                            ipItem->setFlags(ipItem->flags() & ~Qt::ItemIsEditable);
                            ipItem->setToolTip(dev.hasHttps ? "Click to open HTTPS" :
                                               dev.hasHttp ? "Click to open HTTP" : "No web interface detected");
                            ipItem->setData(Qt::UserRole + 1, dev.hasHttps);
                            ipItem->setData(Qt::UserRole + 2, dev.hasHttp);
                            if (dev.hasHttps || dev.hasHttp)
                                ipItem->setForeground(QBrush(QColor("#1c2684")));
                            else
                                ipItem->setForeground(QBrush(QColor("#222")));
                            deviceTable->setItem(row, 0, ipItem);

                            QTableWidgetItem *hostItem = new QTableWidgetItem((dev.host.isEmpty() || dev.host == dev.ip) ? "" : dev.host);
                            deviceTable->setItem(row, 1, hostItem);

                            // --- Actions column: Port Scan and Shares buttons ---
                            QWidget *btnWidget = new QWidget;
                            QHBoxLayout *btnLayout = new QHBoxLayout(btnWidget);
                            btnLayout->setContentsMargins(0, 0, 0, 0);
                            btnLayout->setSpacing(4);

                            QPushButton *portScanBtn = new QPushButton("Port Scan");
                            portScanBtn->setToolTip("Scan ports on this device");
                            QPushButton *sharesBtn = new QPushButton("Shares");
                            sharesBtn->setToolTip("Show shared folders on this device");

                            btnLayout->addWidget(portScanBtn);
                            btnLayout->addWidget(sharesBtn);
                            btnWidget->setLayout(btnLayout);

                            deviceTable->setCellWidget(row, 2, btnWidget);

                            QObject::connect(portScanBtn, &QPushButton::clicked, [=]() {
                                dlg->accept();
                                showPortScanDialog(parent, dev.ip);
                            });
                            QObject::connect(sharesBtn, &QPushButton::clicked, [=]() {
                                dlg->accept();
                                showLanSharesDialog(parent, dev.ip);
                            });
                        }
                        // --- Add an empty row for air at the bottom ---
                        int emptyRow = deviceTable->rowCount();
                        deviceTable->insertRow(emptyRow);
                        for (int col = 0; col < deviceTable->columnCount(); ++col) {
                            QTableWidgetItem *emptyItem = new QTableWidgetItem("");
                            emptyItem->setFlags(emptyItem->flags() & ~Qt::ItemIsEditable);
                            deviceTable->setItem(emptyRow, col, emptyItem);
                        }
                        deviceTable->setRowHeight(emptyRow, 24); // Adjust height for more/less air
                        // -------------------------------------------------
                    }, Qt::QueuedConnection);

                    QMetaObject::invokeMethod(foundLabel, "setText", Qt::QueuedConnection,
                        Q_ARG(QString, QString("Devices found: %1").arg(++(*foundCount))));
                }
                QMetaObject::invokeMethod(progress, "setValue", Qt::QueuedConnection, Q_ARG(int, ++(*progressCount)));
                QMetaObject::invokeMethod(progress, [=]() {
                    (*finishedCount)++;
                    if (*finishedCount == ipList.size()) {
                        *scanRunning = false;
                        scanBtn->setEnabled(true);
                        updateStopCloseBtn();
                        // --- Ensure progress bar is 100% at the end ---
                        progress->setValue(progress->maximum());
                    }
                }, Qt::QueuedConnection);
            });
        }
    });

    // Stop/Close button logic
    QObject::connect(stopCloseBtn, &QPushButton::clicked, [=]() {
        if (*scanRunning) {
            *cancelRequested = true;
            *scanRunning = false;
            scanBtn->setEnabled(true);
            updateStopCloseBtn();
            // Also force progress bar to 100% if stopped
            progress->setValue(progress->maximum());
        } else {
            dlg->accept();
        }
    });

    // Ctrl+W closes only if not scanning
    QObject::connect(closeShortcut, &QShortcut::activated, [=]() {
        if (!*scanRunning) dlg->accept();
    });

    stopCloseBtn->setEnabled(true);
    scanBtn->setEnabled(true);
    updateStopCloseBtn();

    dlg->exec();
    delete dlg;
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

void showPortScanDialog(QWidget *parent, const QString &initialTarget) {
    QDialog *dlg = new QDialog(parent);
    dlg->setWindowTitle("Port Scan");
    dlg->setWindowFlags((dlg->windowFlags() & ~Qt::WindowCloseButtonHint) | Qt::Dialog | Qt::WindowTitleHint);

    QVBoxLayout *layout = new QVBoxLayout(dlg);

    QLabel *inputLabel = new QLabel("Target (IP or hostname):");
    QLineEdit *targetEdit = new QLineEdit(initialTarget.isEmpty() ? "127.0.0.1" : initialTarget);
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

    auto scanRunning = std::make_shared<bool>(false);

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

    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, [=]() {
        if (!*scanRunning) dlg->accept();
    });

    static QMap<QPair<int, QString>, PortInfo> portInfoMap;

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
        auto checkedCount = std::make_shared<QAtomicInt>(0);
        auto openCount = std::make_shared<QAtomicInt>(0);
        auto startTime = std::make_shared<QElapsedTimer>();
        startTime->start();

        int maxThreads = 32; // Tune as needed
        QThreadPool *pool = QThreadPool::globalInstance();
        pool->setMaxThreadCount(maxThreads);

        auto scanRunningPtr = scanRunning; // for lambda capture

        for (int port = startPort; port <= endPort; ++port) {
            pool->start([=]() {
                if (!*scanRunningPtr) return;
                QTcpSocket sock;
                sock.connectToHost(target, port);
                bool connected = sock.waitForConnected(200);
                if (connected) {
                    openCount->fetchAndAddRelaxed(1);
                    PortInfo info = portInfoMap.value(qMakePair(port, QString("tcp")));
                    QString service = info.serviceName.isEmpty() ? "Unknown" : info.serviceName;
                    QString desc = info.description.isEmpty() ? "No description" : info.description;
                    QMetaObject::invokeMethod(output, [=]() {
                        output->append(
                            QString("<span style='color:blue; font-weight:bold;'>%1</span> "
                                    "<span style='color:limegreen; font-weight:bold;'>OPEN &#x1F389;</span> "
                                    "<span style='color:gray;'>&nbsp;%2</span> "
                                    "<span style='color:#888;'>&nbsp;%3</span>")
                            .arg(port)
                            .arg(service.toHtmlEscaped())
                            .arg(desc.toHtmlEscaped())
                        );
                    }, Qt::QueuedConnection);
                }
                int done = checkedCount->fetchAndAddRelaxed(1) + 1;
                int percent = int((double(done) * 100.0) / totalCount);
                QMetaObject::invokeMethod(progressLabel, "setText", Qt::QueuedConnection,
                    Q_ARG(QString, QString("%1 % done").arg(percent)));
                QMetaObject::invokeMethod(currentPortEdit, "setText", Qt::QueuedConnection,
                    Q_ARG(QString, QString::number(port)));
                qint64 elapsedMs = startTime->elapsed();
                if (done > 0 && percent < 100) {
                    double avgMsPerPort = double(elapsedMs) / done;
                    int portsLeft = totalCount - done;
                    int msLeft = int(avgMsPerPort * portsLeft);
                    int secLeft = msLeft / 1000;
                    int minLeft = secLeft / 60;
                    secLeft = secLeft % 60;
                    QMetaObject::invokeMethod(etaLabel, "setText", Qt::QueuedConnection,
                        Q_ARG(QString, QString("Estimated time remaining: %1 minute%2 and %3 second%4")
                            .arg(minLeft)
                            .arg(minLeft == 1 ? "" : "s")
                            .arg(secLeft)
                            .arg(secLeft == 1 ? "" : "s")));
                } else if (percent >= 100) {
                    QMetaObject::invokeMethod(etaLabel, "setText", Qt::QueuedConnection,
                        Q_ARG(QString, "Estimated time remaining: 0 minutes and 0 seconds"));
                }
                if (done == totalCount) {
                    QMetaObject::invokeMethod(scanBtn, "setEnabled", Qt::QueuedConnection, Q_ARG(bool, true));
                    *scanRunningPtr = false;
                    QMetaObject::invokeMethod(currentPortEdit, "clear", Qt::QueuedConnection);
                    QMetaObject::invokeMethod(progressLabel, "setText", Qt::QueuedConnection,
                        Q_ARG(QString, "100 % done"));
                    QMetaObject::invokeMethod(output, [=]() {
                        output->append(QString("<br><b>Scan complete. %1 open port%2 found.</b>")
                            .arg(openCount->loadRelaxed())
                            .arg(openCount->loadRelaxed() == 1 ? "" : "s"));
                    }, Qt::QueuedConnection);
                    QMetaObject::invokeMethod(dlg, [=]() {
                        updateStopCloseText();
                    }, Qt::QueuedConnection);
                }
            });
        }
    });

    QObject::connect(stopCloseBtn, &QPushButton::clicked, [=, &updateStopCloseText]() mutable {
        if (*scanRunning) {
            *scanRunning = false;
            scanBtn->setEnabled(true);
            progressLabel->setText("0 % done");
            etaLabel->setText("Estimated time remaining: --");
            output->append("<b>Scan stopped.</b>");
            currentPortEdit->clear();
            updateStopCloseText();
            return;
        }
        dlg->close();
    });

    dlg->adjustSize();
    dlg->exec();
    dlg->deleteLater();
}


void showTracerouteDialog(QWidget *parent) {
    // DPI-AWARE TRACEROUTE DIALOG - Full functionality with correct sizing
    QDialog dlg(parent);
    dlg.setWindowTitle("Traceroute Host");
    dlg.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    
    // GET DPI SCALING FACTOR to calculate correct sizes
    QScreen *screen = QApplication::primaryScreen();
    qreal dpiRatio = screen->devicePixelRatio();
    
    // Calculate physical sizes accounting for DPI scaling - further increased for better fit
    int physicalWidth = static_cast<int>(1400 / dpiRatio);
    int physicalHeight = static_cast<int>(850 / dpiRatio);
    
    // Force size using DPI-corrected values
    dlg.setFixedSize(physicalWidth, physicalHeight);
    dlg.resize(physicalWidth, physicalHeight);
    
    // Use absolute positioning for all widgets (DPI-corrected coordinates)
    
    QLabel *prompt = new QLabel("Enter host or IP to trace:", &dlg);
    prompt->setGeometry(10, 10, physicalWidth-20, 25);
    prompt->setStyleSheet("QLabel { color: #2c3e50; font-weight: bold; }");

    QLineEdit *input = new QLineEdit(&dlg);
    input->setPlaceholderText("e.g. 8.8.8.8 or www.google.com");
    input->setGeometry(10, 40, physicalWidth-20, 25);
    input->setStyleSheet("QLineEdit { border: 2px solid #3498db; border-radius: 4px; padding: 2px; }");

    QLabel *hopsLabel = new QLabel("Max hops:", &dlg);
    hopsLabel->setGeometry(10, 75, 80, 25);
    hopsLabel->setStyleSheet("QLabel { color: #34495e; font-weight: bold; }");
    
    QSpinBox *hopsSpin = new QSpinBox(&dlg);
    hopsSpin->setToolTip("Maximum number of hops to trace. Default is 30.");
    hopsSpin->setRange(1, 64);
    hopsSpin->setValue(30);
    hopsSpin->setGeometry(100, 75, 80, 25);
    hopsSpin->setStyleSheet("QSpinBox { border: 1px solid #bdc3c7; border-radius: 3px; }");

    // Create colorful table with DPI-corrected size
    QTableWidget *table = new QTableWidget(&dlg);
    table->setColumnCount(6);
    QStringList headers = {"Hop", "IP/Host", "RTT 1", "RTT 2", "RTT 3", "Comments"};
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setVisible(false); // Hide row numbers since we have hop numbers
    
    // Position table with proper DPI-corrected dimensions - make it 80px smaller in height
    int tableWidth = physicalWidth - 20;
    int tableHeight = physicalHeight - 200; // Reduced by 80px from 120 to clear buttons
    table->setGeometry(10, 110, tableWidth, tableHeight);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn); // Always show vertical scrollbar
    
    // Colorful table styling
    table->setStyleSheet(
        "QTableWidget { "
        "    background-color: #ecf0f1; "
        "    border: 2px solid #34495e; "
        "    border-radius: 5px; "
        "    gridline-color: #bdc3c7; "
        "} "
        "QTableWidget::item { "
        "    padding: 4px; "
        "    border-bottom: 1px solid #d5dbdb; "
        "} "
        "QTableWidget::item:selected { "
        "    background-color: #3498db; "
        "    color: white; "
        "} "
        "QHeaderView::section { "
        "    background-color: #34495e; "
        "    color: white; "
        "    padding: 5px; "
        "    border: none; "
        "    font-weight: bold; "
        "}"
    );
    
    QFont headerFont = table->horizontalHeader()->font();
    headerFont.setBold(true);
    table->horizontalHeader()->setFont(headerFont);

    // Set column widths proportional to table width - fixed distribution for proper visibility
    int colWidths[] = {50, 250, 70, 70, 70, tableWidth-510}; // Total: tableWidth
    for (int i = 0; i < 6; i++) {
        table->setColumnWidth(i, colWidths[i]);
    }

    // Colorful buttons positioned with minimal gap from table
    int buttonY = physicalHeight - 45; // Position buttons with minimal gap
    QPushButton *traceBtn = new QPushButton("Start", &dlg);
    traceBtn->setToolTip("Start the traceroute to the specified host.");
    traceBtn->setGeometry(10, buttonY, 80, 35);
    traceBtn->setStyleSheet(
        "QPushButton { "
        "    background-color: #27ae60; "
        "    color: white; "
        "    border: none; "
        "    border-radius: 5px; "
        "    font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #2ecc71; } "
        "QPushButton:pressed { background-color: #229954; }"
    );
    
    QPushButton *stopCloseBtn = new QPushButton("Close", &dlg);
    stopCloseBtn->setToolTip("Stop the traceroute or close the dialog.");
    stopCloseBtn->setGeometry(100, buttonY, 80, 35);
    stopCloseBtn->setStyleSheet(
        "QPushButton { "
        "    background-color: #e74c3c; "
        "    color: white; "
        "    border: none; "
        "    border-radius: 5px; "
        "    font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #c0392b; } "
        "QPushButton:pressed { background-color: #a93226; }"
    );
    
    QPushButton *bottomBtn = new QPushButton("Bottom", &dlg);
    bottomBtn->setToolTip("Scroll to the bottom of the output.");
    bottomBtn->setGeometry(190, buttonY, 80, 35);
    bottomBtn->setStyleSheet(
        "QPushButton { "
        "    background-color: #3498db; "
        "    color: white; "
        "    border: none; "
        "    border-radius: 5px; "
        "    font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #2980b9; } "
        "QPushButton:pressed { background-color: #1f618d; }"
    );
    
    QPushButton *copyBtn = new QPushButton("Copy All", &dlg);
    copyBtn->setToolTip("Copy all traceroute results to clipboard in CSV format.");
    copyBtn->setGeometry(280, buttonY, 80, 35);
    copyBtn->setStyleSheet(
        "QPushButton { "
        "    background-color: #9b59b6; "
        "    color: white; "
        "    border: none; "
        "    border-radius: 5px; "
        "    font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #8e44ad; } "
        "QPushButton:pressed { background-color: #7d3c98; }"
    );

    // Animated status indicator with colors - smaller and better positioned
    QLabel *scanningLabel = new QLabel(&dlg);
    scanningLabel->setAlignment(Qt::AlignCenter);
    scanningLabel->setStyleSheet(
        "QLabel { "
        "    color: #e67e22; "
        "    font-weight: bold; "
        "    font-size: 10pt; "
        "    background-color: #fdf2e9; "
        "    border: 2px solid #f39c12; "
        "    border-radius: 6px; "
        "    padding: 4px; "
        "}"
    );
    scanningLabel->setVisible(false);
    scanningLabel->setGeometry(370, buttonY, 300, 25); // Position after copy button

    // Timer for animated scanning dots
    QTimer *animTimer = new QTimer(&dlg);
    int dotCount = 0;
    
    QObject::connect(animTimer, &QTimer::timeout, [scanningLabel, &dotCount]() mutable {
        QStringList spinChars = {"\\", "|", "/", "-"};
        QString spinner = spinChars[dotCount % 4];
        scanningLabel->setText(QString("🔍 Tracing route %1").arg(spinner));
        dotCount++;
    });
    animTimer->setInterval(250); // Update every 250ms

    // State variables
    bool isTracing = false;
    QProcess *lastProc = nullptr;

    // Helper to update Stop/Close button text and colors
    auto updateStopCloseText = [&]() {
        if (isTracing) {
            stopCloseBtn->setText("Stop");
            stopCloseBtn->setToolTip("Stop the traceroute");
            stopCloseBtn->setStyleSheet(
                "QPushButton { background-color: #e74c3c; color: white; border: none; border-radius: 5px; font-weight: bold; } "
                "QPushButton:hover { background-color: #c0392b; } "
                "QPushButton:pressed { background-color: #a93226; }"
            );
            scanningLabel->setVisible(true);
            animTimer->start();
        } else {
            stopCloseBtn->setText("Close");
            stopCloseBtn->setToolTip("Close the dialog");
            stopCloseBtn->setStyleSheet(
                "QPushButton { background-color: #95a5a6; color: white; border: none; border-radius: 5px; font-weight: bold; } "
                "QPushButton:hover { background-color: #7f8c8d; } "
                "QPushButton:pressed { background-color: #6c7b7d; }"
            );
            scanningLabel->setVisible(false);
            animTimer->stop();
        }
    };
    updateStopCloseText();

    // Ctrl+W shortcut: only close if not tracing
    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), &dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, [&dlg, &isTracing]() mutable {
        if (!isTracing) dlg.close();
    });

    // Sorting state
    int sortColumn = 0;
    Qt::SortOrder sortOrder = Qt::AscendingOrder;

    auto updateHeaderArrows = [&]() {
        for (int i = 0; i < headers.size(); ++i) {
            QString label = headers[i];
            if (i == sortColumn)
                label += (sortOrder == Qt::AscendingOrder ? " ▲" : " ▼");
            table->horizontalHeaderItem(i)->setText(label);
        }
    };

    struct HopEntry {
        int hop = 0;
        QString ipHost, rtt1, rtt2, rtt3, comment;
    };
    QList<HopEntry> hops;

    // Helper to apply row colors based on hop data
    auto applyRowColors = [&](int row, const HopEntry &e) {
        QColor rowColor;
        if (e.ipHost == "*" || e.comment.contains("timed out")) {
            rowColor = QColor("#ffebee"); // Light red for timeouts
        } else if (e.ipHost.startsWith("192.168.") || e.ipHost.startsWith("10.") || e.ipHost.startsWith("172.")) {
            rowColor = QColor("#e8f5e8"); // Light green for private IPs
        } else {
            rowColor = QColor("#f0f8ff"); // Light blue for public IPs
        }
        
        for (int col = 0; col < 6; col++) {
            if (table->item(row, col)) {
                table->item(row, col)->setBackground(rowColor);
                
                // Special text colors for different columns
                if (col == 0) { // Hop number
                    table->item(row, col)->setForeground(QColor("#2c3e50"));
                } else if (col == 1) { // IP/Host
                    table->item(row, col)->setForeground(QColor("#27ae60"));
                } else if (col >= 2 && col <= 4) { // RTT columns
                    table->item(row, col)->setForeground(QColor("#e67e22"));
                } else { // Comments
                    table->item(row, col)->setForeground(QColor("#8e44ad"));
                }
            }
        }
    };

    auto fillTable = [&]() {
        table->setRowCount(0);
        
        // Remove duplicates by hop number (keep the latest entry for each hop)
        QMap<int, HopEntry> uniqueHops;
        for (const HopEntry &e : hops) {
            if (e.hop > 0) { // Only valid hop numbers
                uniqueHops[e.hop] = e;
            }
        }
        
        // Convert back to sorted list
        QList<HopEntry> sortedHops = uniqueHops.values();
        std::sort(sortedHops.begin(), sortedHops.end(), [](const HopEntry &a, const HopEntry &b) {
            return a.hop < b.hop;
        });
        
        for (const HopEntry &e : sortedHops) {
            int row = table->rowCount();
            table->insertRow(row);
            
            QTableWidgetItem *hopItem = new QTableWidgetItem(QString::number(e.hop));
            QTableWidgetItem *ipHostItem = new QTableWidgetItem(e.ipHost);
            QTableWidgetItem *rtt1Item = new QTableWidgetItem(e.rtt1);
            QTableWidgetItem *rtt2Item = new QTableWidgetItem(e.rtt2);
            QTableWidgetItem *rtt3Item = new QTableWidgetItem(e.rtt3);
            QTableWidgetItem *commentItem = new QTableWidgetItem(e.comment);
            
            // Right-align numeric columns
            hopItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            rtt1Item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            rtt2Item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            rtt3Item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            
            // Set tooltips for full text
            if (!e.ipHost.isEmpty()) ipHostItem->setToolTip(e.ipHost);
            if (!e.comment.isEmpty()) commentItem->setToolTip(e.comment);
            
            table->setItem(row, 0, hopItem);
            table->setItem(row, 1, ipHostItem);
            table->setItem(row, 2, rtt1Item);
            table->setItem(row, 3, rtt2Item);
            table->setItem(row, 4, rtt3Item);
            table->setItem(row, 5, commentItem);
            
            applyRowColors(row, e);
        }
        updateHeaderArrows();
        if (table->rowCount() > 0)
            table->scrollToBottom();
    };

    // Connect buttons
    QObject::connect(traceBtn, &QPushButton::clicked, [&]() {
        QString target = input->text().trimmed();
        if (target.isEmpty()) {
            input->setFocus();
            input->setPlaceholderText("Please enter a host or IP address!");
            return;
        }

        // Clear previous results completely
        hops.clear();
        fillTable();
        
        isTracing = true;
        updateStopCloseText();
        input->setReadOnly(true);
        traceBtn->setEnabled(false);
        hopsSpin->setEnabled(false);

        lastProc = new QProcess(&dlg);
        lastProc->setProgram("tracert");
        lastProc->setArguments({"-h", QString::number(hopsSpin->value()), target});

        // Buffer for accumulating multi-line tracert output
        static QString outputBuffer;
        
        QObject::connect(lastProc, &QProcess::readyReadStandardOutput, [&]() {
            QByteArray data = lastProc->readAllStandardOutput();
            QString newOutput = QString::fromLocal8Bit(data);
            outputBuffer += newOutput;
            
            // Process complete lines only (split by \r\n or \n)
            QStringList lines = outputBuffer.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
            
            // Keep the last line in buffer if it doesn't end with newline (might be incomplete)
            if (!newOutput.endsWith('\n') && !newOutput.endsWith('\r')) {
                if (!lines.isEmpty()) {
                    outputBuffer = lines.takeLast();
                } else {
                    outputBuffer.clear();
                }
            } else {
                outputBuffer.clear();
            }

            for (const QString &line : lines) {
                QString cleaned = line.trimmed();
                if (cleaned.isEmpty()) continue;

                // Skip header lines
                if (cleaned.contains("Tracing route to") || 
                    cleaned.contains("over a maximum of") ||
                    cleaned.contains("hops:") ||
                    cleaned.contains("Trace complete")) continue;

                // Look for hop number at start of line
                QRegularExpression hopNumRe(R"(^\s*(\d+)\s+(.*)$)");
                QRegularExpressionMatch numMatch = hopNumRe.match(cleaned);
                
                if (numMatch.hasMatch()) {
                    int hopNum = numMatch.captured(1).toInt();
                    QString rest = numMatch.captured(2).trimmed();
                    
                    // Skip if hop number is invalid
                    if (hopNum <= 0 || hopNum > 64) continue;
                    
                    qDebug() << "Processing hop" << hopNum << "with data:" << rest;
                    
                    HopEntry entry;
                    entry.hop = hopNum;
                    
                    // Handle timeout/asterisk lines
                    if (rest.contains("Request timed out") || rest.startsWith("*") || rest.contains("* * *")) {
                        entry.ipHost = "*";
                        entry.rtt1 = "*";
                        entry.rtt2 = "*";
                        entry.rtt3 = "*";
                        entry.comment = "Request timed out";
                    } else {
                        // Windows tracert format: "RTT RTT RTT hostname [IP]" or "RTT RTT RTT IP"
                        qDebug() << "DEBUG: Parsing line:" << rest;
                        
                        // Use regex to extract RTT values and hostname/IP
                        QRegularExpression tracertRegex(R"(^\s*([*]|[<>]?\d+\s*ms|\d+\s*ms)\s+([*]|[<>]?\d+\s*ms|\d+\s*ms)\s+([*]|[<>]?\d+\s*ms|\d+\s*ms)\s*(.*)$)");
                        QRegularExpressionMatch match = tracertRegex.match(rest);
                        
                        if (match.hasMatch()) {
                            // Extract RTT values
                            entry.rtt1 = match.captured(1).trimmed();
                            entry.rtt2 = match.captured(2).trimmed();
                            entry.rtt3 = match.captured(3).trimmed();
                            QString hostPart = match.captured(4).trimmed();
                            
                            qDebug() << "DEBUG: RTTs:" << entry.rtt1 << entry.rtt2 << entry.rtt3 << "Host:" << hostPart;
                            
                            // Parse hostname/IP part
                            if (!hostPart.isEmpty()) {
                                // Look for "hostname [IP]" pattern
                                QRegularExpression hostIpRegex(R"(^(.+?)\s*\[([^\]]+)\]$)");
                                QRegularExpressionMatch hostMatch = hostIpRegex.match(hostPart);
                                
                                if (hostMatch.hasMatch()) {
                                    entry.comment = hostMatch.captured(1).trimmed();
                                    entry.ipHost = hostMatch.captured(2).trimmed();
                                    qDebug() << "DEBUG: Found hostname [IP] - Host:" << entry.comment << "IP:" << entry.ipHost;
                                } else {
                                    // Check if it's just an IP address
                                    QRegularExpression ipRegex(R"(^\d+\.\d+\.\d+\.\d+$)");
                                    if (ipRegex.match(hostPart).hasMatch()) {
                                        entry.ipHost = hostPart;
                                        entry.comment = "";
                                        qDebug() << "DEBUG: Found standalone IP:" << entry.ipHost;
                                    } else {
                                        // Treat as hostname
                                        entry.ipHost = hostPart;
                                        entry.comment = "";
                                        qDebug() << "DEBUG: Found hostname:" << entry.ipHost;
                                    }
                                }
                            } else {
                                entry.ipHost = "Unknown";
                                entry.comment = "";
                                qDebug() << "DEBUG: No host part found";
                            }
                        } else {
                            // Fallback parsing for malformed lines
                            qDebug() << "DEBUG: Regex failed, using fallback parsing";
                            QStringList tokens = rest.split(QRegularExpression(R"(\s+)"), Qt::SkipEmptyParts);
                            
                            // Try to extract first 3 RTT-like tokens
                            QStringList rttValues;
                            int tokenIdx = 0;
                            while (tokenIdx < tokens.size() && rttValues.size() < 3) {
                                QString token = tokens[tokenIdx];
                                if (token == "*" || token.contains("ms") || 
                                    (tokenIdx + 1 < tokens.size() && tokens[tokenIdx + 1] == "ms")) {
                                    if (tokens[tokenIdx + 1] == "ms") {
                                        rttValues.append(token + " ms");
                                        tokenIdx += 2;
                                    } else {
                                        rttValues.append(token);
                                        tokenIdx++;
                                    }
                                } else {
                                    break;
                                }
                            }
                            
                            // Assign RTT values
                            entry.rtt1 = rttValues.size() > 0 ? rttValues[0] : "";
                            entry.rtt2 = rttValues.size() > 1 ? rttValues[1] : "";
                            entry.rtt3 = rttValues.size() > 2 ? rttValues[2] : "";
                            
                            // Remaining tokens are hostname/IP
                            if (tokenIdx < tokens.size()) {
                                QString remaining = tokens.mid(tokenIdx).join(" ");
                                entry.ipHost = remaining;
                                entry.comment = "";
                            } else {
                                entry.ipHost = "Unknown";
                                entry.comment = "";
                            }
                        }
                    }
                    
                    qDebug() << "DEBUG: FINAL - Hop:" << entry.hop << "IP:" << entry.ipHost 
                             << "RTT1:" << entry.rtt1 << "RTT2:" << entry.rtt2 << "RTT3:" << entry.rtt3 
                             << "Comment:" << entry.comment;
                    
                    // Add valid hop entries
                    if (entry.hop > 0 && entry.hop <= 64) {
                        hops.append(entry);
                        fillTable();
                    }
                }
            }
        });

        QObject::connect(lastProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                         [&](int exitCode, QProcess::ExitStatus exitStatus) {
            Q_UNUSED(exitCode)
            Q_UNUSED(exitStatus)
            
            // Clear the output buffer
            outputBuffer.clear();
            
            isTracing = false;
            updateStopCloseText();
            input->setReadOnly(false);
            traceBtn->setEnabled(true);
            hopsSpin->setEnabled(true);
            
            if (lastProc) {
                lastProc->deleteLater();
                lastProc = nullptr;
            }
        });

        lastProc->start();
    });

    QObject::connect(stopCloseBtn, &QPushButton::clicked, [&]() {
        if (isTracing) {
            if (lastProc) {
                lastProc->kill();
            }
        } else {
            dlg.close();
        }
    });

    QObject::connect(bottomBtn, &QPushButton::clicked, [&]() {
        if (table->rowCount() > 0)
            table->scrollToBottom();
    });

    QObject::connect(copyBtn, &QPushButton::clicked, [&]() {
        QString clipboardText;
        
        // Add CSV header
        clipboardText += "\"Hop\",\"IP/Host\",\"RTT 1\",\"RTT 2\",\"RTT 3\",\"Comments\"\n";
        
        // Add all rows in CSV format
        for (int row = 0; row < table->rowCount(); ++row) {
            QStringList rowData;
            for (int col = 0; col < table->columnCount(); ++col) {
                QTableWidgetItem *item = table->item(row, col);
                QString cellText = item ? item->text() : "";
                
                // Escape quotes in CSV and wrap in quotes
                cellText.replace("\"", "\"\"");
                rowData.append("\"" + cellText + "\"");
            }
            clipboardText += rowData.join(",") + "\n";
        }
        
        // Copy to clipboard
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(clipboardText);
        
        // Show brief confirmation
        copyBtn->setText("Copied!");
        QTimer::singleShot(1000, [copyBtn]() {
            copyBtn->setText("Copy All");
        });
    });

    // Double-click to sort
    QObject::connect(table->horizontalHeader(), &QHeaderView::sectionClicked, [&](int logicalIndex) {
        if (logicalIndex == sortColumn) {
            sortOrder = (sortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
        } else {
            sortColumn = logicalIndex;
            sortOrder = Qt::AscendingOrder;
        }
        
        std::sort(hops.begin(), hops.end(), [&](const HopEntry &a, const HopEntry &b) {
            QVariant aVal, bVal;
            switch (sortColumn) {
                case 0: aVal = a.hop; bVal = b.hop; break;
                case 1: aVal = a.ipHost; bVal = b.ipHost; break;
                case 2: aVal = a.rtt1; bVal = b.rtt1; break;
                case 3: aVal = a.rtt2; bVal = b.rtt2; break;
                case 4: aVal = a.rtt3; bVal = b.rtt3; break;
                case 5: aVal = a.comment; bVal = b.comment; break;
            }
            return (sortOrder == Qt::AscendingOrder) ? aVal.toString() < bVal.toString() : aVal.toString() > bVal.toString();
        });
        
        fillTable();
    });

    input->setFocus();
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

    // --- Adjusted column widths for better fit ---
    table->setColumnWidth(0, 160); // SSID
    table->setColumnWidth(1, 180); // BSSID
    table->setColumnWidth(2, 110); // Signal
    table->setColumnWidth(3, 140); // Channel (wider so header is fully visible)
    // ---------------------------------------------

    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

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


void showWhoisLookupDialog(QWidget *parent) {
    const QString localPath = "C:/Users/Public/AppData/Local/IPGui/tld-rdap-list.json";
    const QString url = "https://data.iana.org/rdap/dns.json";

    // Helper: Download JSON file if missing or older than 7 days
    auto ensureRdapList = [&]() -> bool {
        QFileInfo fi(localPath);
        bool needDownload = !fi.exists() || fi.lastModified().daysTo(QDateTime::currentDateTime()) > 7;
        if (!needDownload) return true;

        QNetworkAccessManager mgr;
        QUrl qurl(url);
        QNetworkRequest req(qurl);
        QNetworkReply *reply = mgr.get(req);

        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(8000);
        loop.exec();

        bool ok = false;
        if (reply->isFinished() && reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QDir().mkpath(QFileInfo(localPath).absolutePath());
            QFile file(localPath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                file.write(data);
                file.close();
                ok = true;
            }
        }
        reply->deleteLater();
        return ok || QFileInfo(localPath).exists();
    };

    // Helper: Load TLD->RDAP endpoint map from local JSON
    auto loadRdapMap = [&]() -> QMap<QString, QString> {
        QMap<QString, QString> map;
        QFile file(localPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return map;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject obj = doc.object();
        QJsonArray services = obj["services"].toArray();
        for (const QJsonValue &serviceVal : services) {
            QJsonArray arr = serviceVal.toArray();
            if (arr.size() >= 2) {
                QJsonArray tlds = arr[0].toArray();
                QJsonArray servers = arr[1].toArray();
                QString rdapServer;
                for (const QJsonValue &serverVal : servers) {
                    QString server = serverVal.toString();
                    if (server.startsWith("https://") || server.startsWith("http://")) {
                        rdapServer = server;
                        break;
                    }
                }
                if (!rdapServer.isEmpty()) {
                    for (const QJsonValue &tldVal : tlds) {
                        QString tld = tldVal.toString();
                        if (!tld.startsWith(".")) tld = "." + tld;
                        map[tld.toLower()] = rdapServer;
                    }
                }
            }
        }
        return map;
    };

    // Helper: Make URLs and emails clickable in text (no double escaping)
    auto makeLinksClickable = [](const QString &text) -> QString {
        QString result;
        int lastPos = 0;
        QRegularExpression re("([\\w\\.\\-]+@[\\w\\.\\-]+\\.\\w+)|(https?://[^\\s<>\"]+)");
        auto matches = re.globalMatch(text);
        while (matches.hasNext()) {
            QRegularExpressionMatch match = matches.next();
            int start = match.capturedStart();
            int end = match.capturedEnd();
            // Add text before the match, HTML-escaped
            result += text.mid(lastPos, start - lastPos).toHtmlEscaped();
            QString email = match.captured(1);
            QString url = match.captured(2);
            if (!email.isEmpty()) {
                result += "<a href=\"mailto:" + email + "\" style='color:#1c2684; font-size:10pt;'>" + email.toHtmlEscaped() + "</a>";
            } else if (!url.isEmpty()) {
                result += "<a href=\"" + url + "\" style='color:#1c2684; font-size:10pt;'>" + url.toHtmlEscaped() + "</a>";
            }
            lastPos = end;
        }
        result += text.mid(lastPos).toHtmlEscaped();
        return result;
    };

    // Ensure list is available
    if (!ensureRdapList()) {
        QMessageBox::warning(parent, "RDAP Lookup", "Could not download or load the TLD RDAP list.\nCheck your internet connection.");
        return;
    }
    QMap<QString, QString> rdapMap = loadRdapMap();
    if (rdapMap.isEmpty()) {
        QMessageBox::warning(parent, "RDAP Lookup", "Could not parse the TLD RDAP list.");
        return;
    }

    // --- Dialog UI ---
    QDialog dlg(parent);
    dlg.setWindowTitle("RDAP Domain Lookup");
    dlg.setMinimumWidth(420);
    dlg.setMaximumWidth(700);
    addCtrlWClose(&dlg);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLabel *title = new QLabel(
        "<b>RDAP Domain Lookup</b><br>"
        "<span style='color:gray;'>Enter a domain (any TLD).<br>"
        "The correct RDAP server is chosen automatically.</span>");
    title->setTextFormat(Qt::RichText);
    layout->addWidget(title);

    QLineEdit *domainEdit = new QLineEdit;
    domainEdit->setPlaceholderText("e.g. www.google.com");
    domainEdit->setToolTip("Enter a domain name (any TLD).");
    layout->addWidget(domainEdit);

    QPushButton *lookupBtn = new QPushButton("Lookup");
    lookupBtn->setToolTip("Perform RDAP lookup for the entered domain.");
    lookupBtn->setEnabled(false);
    layout->addWidget(lookupBtn);

    QTextBrowser *output = new QTextBrowser; // <-- Use QTextBrowser!
    output->setReadOnly(true);
    output->setFontFamily("Consolas");
    output->setMinimumHeight(180);
    output->setMaximumHeight(400);
    output->setStyleSheet("background:#fff; color:#222; border:1px solid #bbb; border-radius:4px;");
    output->setOpenExternalLinks(true); // <-- Make links clickable!
    layout->addWidget(output);

    QPushButton *closeBtn = new QPushButton("Close");
    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    layout->addWidget(closeBtn);

    QObject::connect(domainEdit, &QLineEdit::textChanged, [=]() {
        lookupBtn->setEnabled(!domainEdit->text().trimmed().isEmpty());
    });

    QObject::connect(lookupBtn, &QPushButton::clicked, [&]() {
        QString domain = domainEdit->text().trimmed().toLower();
        if (domain.isEmpty()) return;
        // Extract TLD (handles .co.uk etc)
        QString tld;
        int lastDot = domain.lastIndexOf('.');
        if (lastDot != -1) {
            tld = domain.mid(lastDot);
            if (tld == ".uk" && domain.endsWith(".co.uk")) tld = ".co.uk";
        }
        QString rdapServer = rdapMap.value(tld, QString());
        if (rdapServer.isEmpty()) {
            output->setPlainText("No RDAP server found for this TLD.");
            output->setToolTip("No RDAP server found.");
            return;
        }
        // Compose RDAP URL
        QString rdapUrl = rdapServer;
        if (!rdapUrl.endsWith('/')) rdapUrl += '/';
        rdapUrl += "domain/" + domain;

        output->setPlainText(QString("Querying RDAP server:\n%1 ...").arg(rdapUrl));
        output->setToolTip(QString("RDAP server: %1").arg(rdapServer));

        // RDAP query via QNetworkAccessManager
        QNetworkAccessManager mgr;
        QUrl qurl(rdapUrl);
        QNetworkRequest req(qurl);
        req.setHeader(QNetworkRequest::UserAgentHeader, "IPGui RDAP Client");
        QNetworkReply *reply = mgr.get(req);

        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(10000);
        loop.exec();

        if (!reply->isFinished() || reply->error() != QNetworkReply::NoError) {
            output->setPlainText(QString("Could not connect to RDAP server.\n%1").arg(reply->errorString()));
            reply->deleteLater();
            return;
        }

        QByteArray response = reply->readAll();
        reply->deleteLater();

        // Try to parse as JSON
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(response, &err);
        if (doc.isNull()) {
            output->setPlainText("Received non-JSON response:\n" + QString::fromUtf8(response.left(4096)));
            return;
        }

        QJsonObject obj = doc.object();
        // Table with dark blue value text, value cells smaller font
        QString html = "<b>RDAP Domain Information</b><br><table cellpadding='4' cellspacing='0'>";

        auto val = [&](const QString &s) {
            return "<span style='color:#1c2684; font-size:10pt;'>" + makeLinksClickable(s) + "</span>";
        };

        // Domain name
        QString domainName = obj.value("ldhName").toString();
        if (!domainName.isEmpty())
            html += "<tr><td><b>Domain:</b></td><td>" + val(domainName) + "</td></tr>";

        // Handle
        QString handle = obj.value("handle").toString();
        if (!handle.isEmpty())
            html += "<tr><td><b>Handle:</b></td><td>" + val(handle) + "</td></tr>";

        // Registration/last changed dates
        QJsonArray events = obj.value("events").toArray();
        QString regDate, lastChanged;
        for (const QJsonValue &ev : events) {
            QJsonObject e = ev.toObject();
            QString action = e.value("eventAction").toString();
            QString date = e.value("eventDate").toString();
            if (action == "registration") regDate = date;
            else if (action == "last changed") lastChanged = date;
        }
        if (!regDate.isEmpty())
            html += "<tr><td><b>Registered:</b></td><td>" + val(regDate) + "</td></tr>";
        if (!lastChanged.isEmpty())
            html += "<tr><td><b>Last Changed:</b></td><td>" + val(lastChanged) + "</td></tr>";

        // Registrar/Registrant/Contacts
        QJsonArray entities = obj.value("entities").toArray();
        QString registrar, registrant, techContact, adminContact;
        QString registrarEmail, registrantEmail, techEmail, adminEmail;
        QString registrarPhone, registrantPhone, techPhone, adminPhone;
        QString registrarOrg, registrantOrg;
        QString registrarUrl;
        for (const QJsonValue &entVal : entities) {
            QJsonObject ent = entVal.toObject();
            QJsonArray roles = ent.value("roles").toArray();
            QStringList roleList;
            for (const QJsonValue &r : roles) roleList << r.toString();
            QJsonArray vcard = ent.value("vcardArray").toArray();
            QString org, fn, email, phone, url;
            if (vcard.size() == 2) {
                QJsonArray vfields = vcard[1].toArray();
                for (const QJsonValue &fieldVal : vfields) {
                    QJsonArray field = fieldVal.toArray();
                    if (field.size() < 4) continue;
                    QString key = field[0].toString();
                    QString value = field[3].toString();
                    if (key == "fn") fn = value;
                    else if (key == "org") org = value;
                    else if (key == "email") email = value;
                    else if (key == "tel") phone = value;
                    else if (key == "url") url = value;
                }
            }
            if (roleList.contains("registrar")) {
                registrar = fn.isEmpty() ? org : fn;
                registrarEmail = email;
                registrarPhone = phone;
                registrarOrg = org;
                registrarUrl = url;
            }
            if (roleList.contains("registrant")) {
                registrant = fn.isEmpty() ? org : fn;
                registrantEmail = email;
                registrantPhone = phone;
                registrantOrg = org;
            }
            if (roleList.contains("technical")) {
                techContact = fn.isEmpty() ? org : fn;
                techEmail = email;
                techPhone = phone;
            }
            if (roleList.contains("administrative")) {
                adminContact = fn.isEmpty() ? org : fn;
                adminEmail = email;
                adminPhone = phone;
            }
        }
        if (!registrar.isEmpty())
            html += "<tr><td><b>Registrar:</b></td><td>" + val(registrar) + "</td></tr>";
        if (!registrarOrg.isEmpty())
            html += "<tr><td><b>Registrar Org:</b></td><td>" + val(registrarOrg) + "</td></tr>";
        if (!registrarEmail.isEmpty())
            html += "<tr><td><b>Registrar Email:</b></td><td>" + val(registrarEmail) + "</td></tr>";
        if (!registrarPhone.isEmpty())
            html += "<tr><td><b>Registrar Phone:</b></td><td>" + val(registrarPhone) + "</td></tr>";
        if (!registrarUrl.isEmpty())
            html += "<tr><td><b>Registrar URL:</b></td><td>" + val(registrarUrl) + "</td></tr>";

        if (!registrant.isEmpty())
            html += "<tr><td><b>Registrant:</b></td><td>" + val(registrant) + "</td></tr>";
        if (!registrantOrg.isEmpty())
            html += "<tr><td><b>Registrant Org:</b></td><td>" + val(registrantOrg) + "</td></tr>";
        if (!registrantEmail.isEmpty())
            html += "<tr><td><b>Registrant Email:</b></td><td>" + val(registrantEmail) + "</td></tr>";
        if (!registrantPhone.isEmpty())
            html += "<tr><td><b>Registrant Phone:</b></td><td>" + val(registrantPhone) + "</td></tr>";

        if (!techContact.isEmpty())
            html += "<tr><td><b>Tech Contact:</b></td><td>" + val(techContact) + "</td></tr>";
        if (!techEmail.isEmpty())
            html += "<tr><td><b>Tech Email:</b></td><td>" + val(techEmail) + "</td></tr>";
        if (!techPhone.isEmpty())
            html += "<tr><td><b>Tech Phone:</b></td><td>" + val(techPhone) + "</td></tr>";

        if (!adminContact.isEmpty())
            html += "<tr><td><b>Admin Contact:</b></td><td>" + val(adminContact) + "</td></tr>";
        if (!adminEmail.isEmpty())
            html += "<tr><td><b>Admin Email:</b></td><td>" + val(adminEmail) + "</td></tr>";
        if (!adminPhone.isEmpty())
            html += "<tr><td><b>Admin Phone:</b></td><td>" + val(adminPhone) + "</td></tr>";

        // Nameservers
        QJsonArray nss = obj.value("nameservers").toArray();
        if (!nss.isEmpty()) {
            QStringList nsList;
            for (const QJsonValue &nsVal : nss) {
                QJsonObject ns = nsVal.toObject();
                QString nsName = ns.value("ldhName").toString();
                if (!nsName.isEmpty())
                    nsList << val(nsName);
            }
            if (!nsList.isEmpty())
                html += "<tr><td><b>Nameservers:</b></td><td>" + nsList.join("<br>") + "</td></tr>";
        }

        // DNSSEC
        QJsonObject secDns = obj.value("secureDNS").toObject();
        if (!secDns.isEmpty()) {
            bool ds = secDns.value("delegationSigned").toBool(false);
            html += "<tr><td><b>DNSSEC:</b></td><td>" + val(ds ? "Yes" : "No") + "</td></tr>";
        }

        // Notices (with clickable links)
        QJsonArray notices = obj.value("notices").toArray();
        if (!notices.isEmpty()) {
            QStringList noticeList;
            for (const QJsonValue &nVal : notices) {
                QJsonObject n = nVal.toObject();
                QString title = n.value("title").toString();
                QJsonArray descArr = n.value("description").toArray();
                QString desc;
                for (const QJsonValue &d : descArr)
                    desc += d.toString() + " ";
                desc = desc.trimmed();
                QString descWithLinks = makeLinksClickable(desc);
                if (!title.isEmpty())
                    noticeList << "<b>" + makeLinksClickable(title) + ":</b> " + "<span style='color:#1c2684; font-size:10pt;'>" + descWithLinks + "</span>";
                else if (!desc.isEmpty())
                    noticeList << "<span style='color:#1c2684; font-size:10pt;'>" + descWithLinks + "</span>";
            }
            if (!noticeList.isEmpty())
                html += "<tr><td><b>Notices:</b></td><td>" + noticeList.join("<br>") + "</td></tr>";
        }

        html += "</table>";

        // If nothing was parsed, fallback to pretty JSON
        if (html.count("<tr>") < 2) {
            html = "<pre>" + QString::fromUtf8(doc.toJson(QJsonDocument::Indented)).toHtmlEscaped() + "</pre>";
        }

        output->setHtml(html);
        output->setToolTip(QString("RDAP server: %1").arg(rdapServer));
    });

    dlg.adjustSize();
    dlg.exec();
}


void showLanSharesDialog(QWidget *parent, const QString &singleTarget) {
    QPointer<QDialog> dlg = new QDialog(parent);
    dlg->setWindowTitle("LAN Shares Browser");
    dlg->setMinimumWidth(700);
    addCtrlWClose(dlg);

    QVBoxLayout *layout = new QVBoxLayout(dlg);

    QLabel *title = new QLabel(
        "<b>LAN Shares Browser</b><br>"
        "<span style='color:gray;'>Scan your network for computers and shared folders.<br>"
        "Expand a computer to see its shares. Click a share to copy its UNC path.</span>");
    title->setTextFormat(Qt::RichText);
    layout->addWidget(title);

    QGroupBox *rangeBox = new QGroupBox("Scan Range / Target");
    QHBoxLayout *rangeLayout = new QHBoxLayout(rangeBox);
    QLabel *fromLabel = new QLabel("From IP/Name:");
    QLineEdit *fromEdit = new QLineEdit;
    QLabel *toLabel = new QLabel("To IP:");
    QLineEdit *toEdit = new QLineEdit;
    QPushButton *scanBtn = new QPushButton("Scan");
    QPushButton *stopBtn = new QPushButton("Stop");
    stopBtn->setEnabled(false);
    rangeLayout->addWidget(fromLabel);
    rangeLayout->addWidget(fromEdit);
    rangeLayout->addWidget(toLabel);
    rangeLayout->addWidget(toEdit);
    rangeLayout->addWidget(scanBtn);
    rangeLayout->addWidget(stopBtn);
    layout->addWidget(rangeBox);

    // Autofill with local subnet, default .1 to .254
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
    toEdit->setText(defaultBase + ".254");

    // If singleTarget is set, use only that and disable range UI
    if (!singleTarget.isEmpty()) {
        fromEdit->setText(singleTarget);
        toEdit->setText("");
        fromEdit->setEnabled(false);
        toEdit->setEnabled(false);
        rangeBox->setTitle("Target");
    }

    QPointer<QTreeWidget> tree = new QTreeWidget;
    tree->setHeaderLabels(QStringList() << "Computer / Share" << "Type" << "Comment");
    tree->setColumnWidth(0, 220);
    tree->setColumnWidth(1, 60);
    tree->setColumnWidth(2, 200);
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    tree->setExpandsOnDoubleClick(true);
    tree->setRootIsDecorated(true);
    layout->addWidget(tree, 1);

    // Progress bar and time label on same row, time right-aligned
    QHBoxLayout *progressLayout = new QHBoxLayout();
    QPointer<QProgressBar> progress = new QProgressBar;
    progress->setMinimum(0);
    progress->setMaximum(100);
    progress->setValue(0);
    progress->setTextVisible(true);
    progressLayout->addWidget(progress, 1);

    QLabel *timeLabel = new QLabel("Time: 00:00 Min.");
    timeLabel->setStyleSheet("color:#1c2684; font-weight:bold; margin-left:12px;");
    progressLayout->addWidget(timeLabel, 0, Qt::AlignRight | Qt::AlignVCenter);
    layout->addLayout(progressLayout);

    // Expand/Collapse/Close buttons centered
    QHBoxLayout *expColLayout = new QHBoxLayout();
    expColLayout->addStretch();
    QPushButton *expandAllBtn = new QPushButton("Expand all");
    QPushButton *collapseAllBtn = new QPushButton("Collapse all");
    QPushButton *closeBtn = new QPushButton("Close");
    expColLayout->addWidget(expandAllBtn);
    expColLayout->addWidget(collapseAllBtn);
    expColLayout->addWidget(closeBtn);
    expColLayout->addStretch();
    layout->addLayout(expColLayout);

    QObject::connect(expandAllBtn, &QPushButton::clicked, tree, &QTreeWidget::expandAll);
    QObject::connect(collapseAllBtn, &QPushButton::clicked, tree, &QTreeWidget::collapseAll);
    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    QObject::connect(tree, &QTreeWidget::itemDoubleClicked, [dlg](QTreeWidgetItem *item, int col) {
        if (item->parent()) {
            QString unc = QString("\\\\%1\\%2").arg(item->parent()->text(0), item->text(0));
            QApplication::clipboard()->setText(unc);
            QMessageBox::information(dlg, "UNC Path Copied", QString("Copied to clipboard:\n%1").arg(unc));
        }
    });

    QThreadPool *pool = new QThreadPool(dlg);
    pool->setMaxThreadCount(16);

    auto enableControls = [=]() {
        bool enabled = singleTarget.isEmpty();
        fromEdit->setEnabled(enabled);
        toEdit->setEnabled(enabled);
        scanBtn->setEnabled(true);
        stopBtn->setEnabled(false);
    };

    auto scanRunning = std::make_shared<bool>(false);
    auto cancelRequested = std::make_shared<bool>(false);

    // Scan logic (shared for both normal and single-target mode)
    auto doScan = [=]() {
        if (!tree || !progress || !dlg) return;
        tree->clear();
        progress->setValue(0);
        timeLabel->setText("Time: 00:00 Min.");

        QString from = fromEdit->text().trimmed();
        QString to = toEdit->text().trimmed();

        QList<QString> targets;
        if (!singleTarget.isEmpty()) {
            targets << singleTarget;
        } else if (to.isEmpty()) {
            if (!from.isEmpty())
                targets << from;
        } else {
            QHostAddress fromAddr(from), toAddr(to);
            if (fromAddr.protocol() == QAbstractSocket::IPv4Protocol &&
                toAddr.protocol() == QAbstractSocket::IPv4Protocol) {
                quint32 fromInt = fromAddr.toIPv4Address();
                quint32 toInt = toAddr.toIPv4Address();
                if (fromInt > toInt) std::swap(fromInt, toInt);
                for (quint32 ip = fromInt; ip <= toInt; ++ip)
                    targets << QHostAddress(ip).toString();
            } else {
                QMessageBox::warning(dlg, "Input Error", "Invalid IP address range.");
                return;
            }
        }

        if (targets.isEmpty()) {
            QMessageBox::warning(dlg, "Input Error", "No targets to scan.");
            return;
        }

        int totalSteps = targets.size(); // ping phase
        progress->setMaximum(totalSteps);
        progress->setValue(0);

        *scanRunning = true;
        *cancelRequested = false;
        scanBtn->setEnabled(false);
        stopBtn->setEnabled(true);

        QElapsedTimer *timer = new QElapsedTimer();
        timer->start();

        QTimer *uiTimer = new QTimer(dlg);
        uiTimer->setInterval(100);
        auto progressValue = std::make_shared<int>(0);
        timeLabel->setText("Time: 00:00 Min.");
        QObject::connect(uiTimer, &QTimer::timeout, [=]() {
            qint64 elapsed = timer->elapsed() / 1000;
            int min = int(elapsed / 60);
            int sec = int(elapsed % 60);
            timeLabel->setText(QString("Time: %1:%2 Min.").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0')));
        });
        uiTimer->start();

        // Step 1: Ping all targets in parallel, collect alive hosts
        auto aliveHosts = std::make_shared<QVector<QString>>();
        QMutex *aliveMutex = new QMutex;
        auto pingCompleted = std::make_shared<int>(0);

        for (int i = 0; i < targets.size(); ++i) {
            QFuture<void> unused = QtConcurrent::run(pool, [=]() {
                if (*cancelRequested || !dlg) return;
                QString ip = targets[i];
                QProcess ping;
                ping.start("ping", QStringList() << "-n" << "1" << "-w" << "200" << ip);
                ping.waitForFinished(400);
                QString result = ping.readAllStandardOutput();
                bool alive = result.contains("TTL=");
                if (alive) {
                    QMutexLocker locker(aliveMutex);
                    aliveHosts->append(ip);
                }
                QMetaObject::invokeMethod(progress, [=]() {
                    if (!*scanRunning || !dlg) return;
                    (*progressValue)++;
                    progress->setValue(*progressValue);
                }, Qt::QueuedConnection);

                bool last = false;
                {
                    QMutexLocker locker(aliveMutex);
                    ++(*pingCompleted);
                    last = (*pingCompleted == targets.size());
                }
                if (last) {
                    // Step 2: For each alive host, scan shares (in parallel)
                    int shareSteps = aliveHosts->size();
                    QMetaObject::invokeMethod(progress, [=]() {
                        progress->setMaximum(totalSteps + shareSteps);
                    }, Qt::QueuedConnection);

                    if (!*scanRunning || !dlg) return;
                    auto completed = std::make_shared<int>(0);
                    auto foundHosts = std::make_shared<int>(0);

                    QSet<QString> *seenHosts = new QSet<QString>();
                    QMutex *resultsMutex = new QMutex;

                    for (int j = 0; j < aliveHosts->size(); ++j) {
                        QFuture<void> unused2 = QtConcurrent::run(pool, [=]() {
                            if (*cancelRequested || !dlg) return;
                            QString ip = (*aliveHosts)[j];
                            QList<QList<QString>> shares;
                            try {
                                shares = getSharesOnHost(ip);
                            } catch (...) {}

                            // Only show hosts with shares
                            if (!shares.isEmpty()) {
                                QString netbios = QHostInfo::fromName(ip).hostName();
                                QString hostLabel = netbios.isEmpty() || netbios == ip ? ip : (netbios + " " + ip);

                                bool isDuplicate = false;
                                {
                                    QMutexLocker locker(resultsMutex);
                                    if (seenHosts->contains(hostLabel)) {
                                        isDuplicate = true;
                                    } else {
                                        seenHosts->insert(hostLabel);
                                    }
                                }
                                if (!isDuplicate) {
                                    QMetaObject::invokeMethod(tree, [=]() {
                                        if (!*scanRunning || !dlg) return;
                                        QTreeWidgetItem *hostItem = new QTreeWidgetItem(tree, QStringList() << hostLabel);
                                        QFont boldFont = hostItem->font(0);
                                        boldFont.setBold(true);
                                        hostItem->setFont(0, boldFont);
                                        hostItem->setForeground(0, Qt::black);

                                        hostItem->setExpanded(false);

                                        for (const QList<QString> &share : shares) {
                                            QTreeWidgetItem *shareItem = new QTreeWidgetItem(hostItem, QStringList() << share[0] << share[1] << share[2]);
                                            QBrush blueBrush(QColor("#1c2684"));
                                            shareItem->setForeground(0, blueBrush);
                                            shareItem->setForeground(1, blueBrush);
                                            shareItem->setForeground(2, blueBrush);
                                            shareItem->setToolTip(0, QString("\\\\%1\\%2").arg(ip, share[0]));
                                        }
                                    }, Qt::QueuedConnection);
                                    (*foundHosts)++;
                                }
                            }

                            QMetaObject::invokeMethod(progress, [=]() {
                                if (!*scanRunning || !dlg) return;
                                (*progressValue)++;
                                progress->setValue(*progressValue);
                            }, Qt::QueuedConnection);

                            bool lastShare = false;
                            {
                                QMutexLocker locker(resultsMutex);
                                ++(*completed);
                                lastShare = (*completed == aliveHosts->size());
                            }
                            if (lastShare) {
                                QMetaObject::invokeMethod(dlg, [=]() {
                                    if (!dlg) return;
                                    *scanRunning = false;
                                    scanBtn->setEnabled(true);
                                    stopBtn->setEnabled(false);
                                    progress->setValue(progress->maximum());
                                    enableControls();
                                    uiTimer->stop();
                                    qint64 elapsed = timer->elapsed() / 1000;
                                    int min = int(elapsed / 60);
                                    int sec = int(elapsed % 60);
                                    timeLabel->setText(QString("Time: %1:%2 Min.").arg(min, 2, 10, QChar('0')).arg(sec, 2, 10, QChar('0')));
                                    if (*foundHosts == 0) {
                                        QMessageBox::information(dlg, "No Shares Found", "No shares found in the specified range.");
                                    }
                                    delete seenHosts;
                                    delete resultsMutex;
                                    delete timer;
                                    delete aliveMutex;
                                    uiTimer->deleteLater();
                                }, Qt::QueuedConnection);
                            }
                        });
                    }
                }
            });
        }
    };

    QObject::connect(scanBtn, &QPushButton::clicked, doScan);

    QObject::connect(stopBtn, &QPushButton::clicked, [=]() {
        *cancelRequested = true;
        *scanRunning = false;
        scanBtn->setEnabled(true);
        stopBtn->setEnabled(false);
        enableControls();
    });

    QObject::connect(dlg, &QDialog::finished, [=]() {
        *cancelRequested = true;
        *scanRunning = false;
        pool->clear();
        pool->waitForDone();
        if (dlg) dlg->deleteLater();
    });

    // If singleTarget is set, auto-scan on open and disable Scan button
    if (!singleTarget.isEmpty()) {
        scanBtn->setEnabled(false);
        stopBtn->setEnabled(true);
        QTimer::singleShot(0, doScan);
    } else {
        enableControls();
    }

    dlg->adjustSize();
    dlg->exec();

    *cancelRequested = true;
    *scanRunning = false;
    pool->clear();
    pool->waitForDone();
    delete pool;
}

void showPingDialog(QWidget *parent) {
    QDialog dlg(parent);
    dlg.setWindowTitle("Ping Host");
    dlg.setWindowFlags((dlg.windowFlags() & ~Qt::WindowCloseButtonHint) | Qt::Dialog | Qt::WindowTitleHint);

    addCtrlWClose(&dlg);

    QVBoxLayout *pingLayout = new QVBoxLayout(&dlg);

    QLabel prompt("Enter host or IP to ping:");
    pingLayout->addWidget(&prompt);

    QLineEdit input;
    input.setPlaceholderText("e.g. 8.8.8.8 or www.google.com");
    pingLayout->addWidget(&input);

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

    QObject::connect(&bottomBtn, &QPushButton::clicked, [&output]() {
        output.verticalScrollBar()->setValue(output.verticalScrollBar()->maximum());
    });

    QTimer pingTimer;
    QProcess *pingProc = nullptr;
    bool isPinging = false;

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
        autoScrollTimer.start(3000);
    });

    QObject::connect(&autoScrollTimer, &QTimer::timeout, [&]() {
        if (!userIsScrolling && isPinging) {
            output.verticalScrollBar()->setValue(output.verticalScrollBar()->maximum());
        }
    });

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
        pingTimer.start(1000);
    });

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
            QFontMetrics fm(output.font());
            int maxLineWidth = 0;
            for (const QString &line : lines) {
                int width = fm.horizontalAdvance(line);
                if (width > maxLineWidth)
                    maxLineWidth = width;
            }
            int margin = 80;
            int minWidth = 400;
            int newWidth = qMax(minWidth, maxLineWidth + margin);
            if (dlg.width() < newWidth)
                dlg.resize(newWidth, dlg.height());
        });
        pingProc->start("ping", QStringList() << "-n" << "1" << host);
    });

    dlg.adjustSize();
    dlg.exec();
    stopPinging();
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // --- Network info setup ---
    QString ipAddress, subnetMask, adapterName, defaultGateway = "Unavailable", externalIp = "Checking...";
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
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
    defaultGateway = getDefaultGateway(ipAddress);

    // Get external IP address synchronously (with timeout and error handling)
    {
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
            externalIp = "Unavailable (no network or server down)";
        } else {
            externalIp = "Timeout (no network?)";
        }
        reply->deleteLater();
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
    updateIpDisplay(infoBox);

    // Buttons
    QVBoxLayout *buttonRows = new QVBoxLayout();
    QHBoxLayout *row1 = new QHBoxLayout();
    QPushButton *expandBtn = new QPushButton("Advanced");
    QPushButton *flushBtn = new QPushButton("Flush DNS");
    row1->addWidget(expandBtn);
    row1->addWidget(flushBtn);

    QHBoxLayout *row2 = new QHBoxLayout();
    QPushButton *releaseBtn = new QPushButton("Release IP");
    QPushButton *renewBtn = new QPushButton("Renew IP");
    row2->addWidget(releaseBtn);
    row2->addWidget(renewBtn);

    buttonRows->addLayout(row1);
    buttonRows->addLayout(row2);
    layout->addLayout(buttonRows);

    QObject::connect(flushBtn, &QPushButton::clicked, [&]() {
        int result = QProcess::execute("ipconfig", QStringList() << "/flushdns");
        if (result == 0)
            QMessageBox::information(nullptr, "Flush DNS", "DNS cache flushed.");
        else
            QMessageBox::warning(nullptr, "Flush DNS", "Failed to flush DNS cache.");
    });

    window.setCentralWidget(centralWidget);

    // Expand/collapse logic
    bool expanded = false;
    QObject::connect(expandBtn, &QPushButton::clicked, [&]() mutable {
        if (!expanded) {
            QProcess proc;
            proc.start("ipconfig", QStringList() << "/all");
            proc.waitForFinished();
            QString output = proc.readAllStandardOutput();
            infoBox->setFontFamily("Consolas");
            infoBox->setLineWrapMode(QTextEdit::NoWrap);
            infoBox->setPlainText(output);
            expandBtn->setText("Basic");
            expanded = true;
            window.resize(650, 500);
            QScreen *screen = QGuiApplication::primaryScreen();
            QRect screenGeometry = screen->availableGeometry();
            QRect windowGeometry = window.frameGeometry();
            int x = screenGeometry.x() + (screenGeometry.width() - windowGeometry.width()) / 2;
            int y = screenGeometry.y() + (screenGeometry.height() - windowGeometry.height()) / 2;
            window.move(x, y);
        } else {
            infoBox->setLineWrapMode(QTextEdit::WidgetWidth);
            infoBox->setFontFamily("");
            updateIpDisplay(infoBox);
            expandBtn->setText("Advanced");
            expanded = false;
            window.resize(255, 330);
            QScreen *screen = QGuiApplication::primaryScreen();
            QRect screenGeometry = screen->availableGeometry();
            QRect windowGeometry = window.frameGeometry();
            int x = screenGeometry.x() + (screenGeometry.width() - windowGeometry.width()) / 2;
            int y = screenGeometry.y() + (screenGeometry.height() - windowGeometry.height()) / 2;
            window.move(x, y);
        }
    });

    // --- Menu bar and NetTools ---
    QMenuBar *menuBar = window.menuBar();
    QMenu *fileMenu = menuBar->addMenu("&Actions");
    QMenu *netToolsMenu = fileMenu->addMenu("NetTools");

    QAction *arpAction           = netToolsMenu->addAction("Arp");
    QAction *dhcpStatusAction    = netToolsMenu->addAction("DHCP Status");
    QAction *dnsCacheAction      = netToolsMenu->addAction("DNS Cache Viewer");
    QAction *hostsFileAction     = netToolsMenu->addAction("Hosts File Editor");
    QAction *sslCertAction       = netToolsMenu->addAction("HTTPS Certificate Check");
    QAction *netscanAction       = netToolsMenu->addAction("IP Scanner");
    QAction *lanSharesAction     = netToolsMenu->addAction("LAN Shares");
    QAction *mtuDiscoveryAction  = netToolsMenu->addAction("MTU Discovery");
    QAction *netstatStatsAction  = netToolsMenu->addAction("Netstat Statistics");
    QAction *adaptersAction      = netToolsMenu->addAction("Network Adapters");
    QAction *netUsageAction      = netToolsMenu->addAction("Network Usage");
    QAction *nslookupAction      = netToolsMenu->addAction("NS Lookup");
    QAction *pingAction          = netToolsMenu->addAction("Ping");
    QAction *portscanAction      = netToolsMenu->addAction("Port Scan");
    QAction *routeTableAction    = netToolsMenu->addAction("Route Table Viewer/Editor");
    QAction *tracertAction       = netToolsMenu->addAction("Traceroute");
    QAction *whoisLookupAction   = netToolsMenu->addAction("Whois (RDAP) Lookup");
    QAction *wifiScanAction      = netToolsMenu->addAction("WiFi Scan");

    QAction *alwaysOnTopAction = fileMenu->addAction("🔵 Always on top");
    QAction *deleteTempAction = fileMenu->addAction("Delete Temporary Files");

    // --- Connectors for dialogs ---
    QObject::connect(lanSharesAction, &QAction::triggered, [&window]() { showLanSharesDialog(&window); });
    QObject::connect(whoisLookupAction, &QAction::triggered, [&window]() { showWhoisLookupDialog(&window); });
    QObject::connect(dnsCacheAction, &QAction::triggered, [&window]() { showDnsCacheDialog(&window); });
    QObject::connect(routeTableAction, &QAction::triggered, [&window]() { showRouteTableDialog(&window); });
    QObject::connect(netstatStatsAction, &QAction::triggered, [&window]() { showNetstatStatisticsDialog(&window); });
    QObject::connect(hostsFileAction, &QAction::triggered, [&window]() { showHostsFileEditor(&window); });
    QObject::connect(mtuDiscoveryAction, &QAction::triggered, [&window]() { showMtuDiscoveryDialog(&window); });
    QObject::connect(sslCertAction, &QAction::triggered, [&window]() { showSslCertificateDialog(&window); });
    QObject::connect(adaptersAction, &QAction::triggered, [&window]() { showNetworkAdaptersDialog(&window); });
    QObject::connect(alwaysOnTopAction, &QAction::triggered, [&]() {
        bool onTop = !(window.windowFlags() & Qt::WindowStaysOnTopHint);
        window.setWindowFlag(Qt::WindowStaysOnTopHint, onTop);
        window.show();
    });
    QObject::connect(wifiScanAction, &QAction::triggered, [&window]() { showWifiScanDialog(&window); });
    QObject::connect(arpAction, &QAction::triggered, [&window]() { showArpDialog(&window); });
    QObject::connect(netUsageAction, &QAction::triggered, [&window]() { showNetUsageDialog(&window); });
    QObject::connect(nslookupAction, &QAction::triggered, [&window]() { showNslookupDialog(&window); });
    QObject::connect(dhcpStatusAction, &QAction::triggered, [&window]() { showDhcpStatusDialog(&window); });
    QObject::connect(portscanAction, &QAction::triggered, [&window]() { showPortScanDialog(&window); });
    QObject::connect(netscanAction, &QAction::triggered, [&window]() { showNetworkScannerDialog(&window); });
    QObject::connect(pingAction, &QAction::triggered, [&window]() { showPingDialog(&window); });
    QObject::connect(tracertAction, &QAction::triggered, [&window]() { showTracerouteDialog(&window); });

    // Restore Help menu
QMenu *helpMenu = menuBar->addMenu("&Help");
QAction *manualAction = helpMenu->addAction("User Manual (PDF)");
QAction *aboutAction = helpMenu->addAction("&About");

QObject::connect(manualAction, &QAction::triggered, [&window]() {
    QString dir = "C:/Users/Public/AppData/Local/IPGui";
    QString localPath = dir + "/IPGuiManual.pdf";
    QString url = "https://prog.nalle.no/user/data/manual/IPGuiManual.pdf";
    QDir().mkpath(dir);

    // Download helper
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
    bool downloaded = downloadManual(url, localPath);

    if (!downloaded && !fi.exists()) {
        QMessageBox::warning(&window, "Manual", "Could not download the manual and no local copy exists.");
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(localPath));
});

QObject::connect(aboutAction, &QAction::triggered, [&window]() {
    QDialog dlg(&window);
    dlg.setWindowTitle("About");
    QVBoxLayout layout(&dlg);

    QLabel *iconLabel = new QLabel;
    iconLabel->setPixmap(QPixmap("ip-address.png").scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout.addWidget(iconLabel, 0, Qt::AlignHCenter);

    QLabel label(
        "<b>IPGui by Nalle Berg</b><br>"
        "<b>Copyleft 2025</b><br><br>"
        "An IP lookup/renew -tool.<br>"
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
            QMessageBox::Yes
        );
        if (ret == QMessageBox::Yes)
            window.close();
    });

    return app.exec();
}