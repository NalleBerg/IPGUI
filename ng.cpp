
// Windows API - Must be included before Qt headers.
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <wlanapi.h>
#include <objbase.h>
#include <wtypes.h>
#include <wincrypt.h>
#include <shellapi.h>
#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "shell32.lib")


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
const QString VersionNumber = "4.1.8";
const QString html = QString("<b>Version:</b> %1<br>").arg(VersionNumber);

// Version checking function
void checkForUpdates(QWidget *parent, bool silent = false) {
    QNetworkAccessManager *manager = new QNetworkAccessManager(parent);
    QNetworkRequest request(QUrl("https://prog.nalle.no/user/data/apps/version.php"));
    request.setHeader(QNetworkRequest::UserAgentHeader, "IPGui Version Checker");
    
    QNetworkReply *reply = manager->get(request);
    QTimer *timer = new QTimer(parent);
    timer->setSingleShot(true);
    
    QObject::connect(reply, &QNetworkReply::finished, [=]() {
        timer->stop();
        timer->deleteLater();
        
        if (reply->error() != QNetworkReply::NoError) {
            if (!silent) {
                QMessageBox msgBox(parent);
                msgBox.setWindowTitle("Update Check");
                msgBox.setText("Could not check for updates.\nPlease check your internet connection.");
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setStyleSheet(
                    "QMessageBox { "
                    "    background-color: #f8f9fa; "
                    "    border: 2px solid #34495e; "
                    "    border-radius: 8px; "
                    "} "
                    "QPushButton { "
                    "    background-color: #e74c3c; "
                    "    color: white; "
                    "    border: none; "
                    "    border-radius: 5px; "
                    "    font-weight: bold; "
                    "    padding: 8px 16px; "
                    "    font-size: 10pt; "
                    "} "
                    "QPushButton:hover { background-color: #c0392b; } "
                    "QPushButton:pressed { background-color: #a93226; }"
                );
                msgBox.exec();
            }
            reply->deleteLater();
            manager->deleteLater();
            return;
        }
        
        QByteArray data = reply->readAll();
        reply->deleteLater();
        manager->deleteLater();
        
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        
        if (error.error != QJsonParseError::NoError) {
            if (!silent) {
                QMessageBox::warning(parent, "Update Check", "Could not parse version information.");
            }
            return;
        }
        
        QJsonObject obj = doc.object();
        if (!obj.value("success").toBool()) {
            if (!silent) {
                QMessageBox::warning(parent, "Update Check", "Version check failed on server.");
            }
            return;
        }
        
        QString latestVersion = obj.value("latest_version").toString();
        QString downloadUrl = obj.value("download_url").toString();
        QString filename = obj.value("filename").toString();
        double fileSizeMB = obj.value("filesize_mb").toDouble();
        QString lastModified = obj.value("last_modified").toString();
        
        // Compare versions (simple string comparison for now)
        if (latestVersion > VersionNumber) {
            QDialog dlg(parent);
            dlg.setWindowTitle("Update Available");
            dlg.setStyleSheet(
                "QDialog { "
                "    background-color: #f8f9fa; "
                "    border: 2px solid #34495e; "
                "    border-radius: 8px; "
                "} "
                "QPushButton { "
                "    background-color: #3498db; "
                "    color: white; "
                "    border: none; "
                "    border-radius: 5px; "
                "    font-weight: bold; "
                "    padding: 8px 16px; "
                "    font-size: 10pt; "
                "    margin: 2px; "
                "} "
                "QPushButton:hover { background-color: #2980b9; } "
                "QPushButton:pressed { background-color: #1f618d; } "
                "QPushButton#downloadBtn { background-color: #27ae60; } "
                "QPushButton#downloadBtn:hover { background-color: #2ecc71; } "
                "QPushButton#downloadBtn:pressed { background-color: #229954; }"
            );
            
            QVBoxLayout *layout = new QVBoxLayout(&dlg);
            
            QLabel *title = new QLabel("<b>New Version Available!</b>");
            title->setAlignment(Qt::AlignCenter);
            title->setStyleSheet("font-size: 14pt; color: #27ae60; font-weight: bold; margin: 10px;");
            layout->addWidget(title);
            
            QLabel *info = new QLabel(QString(
                "<b>Current Version:</b> %1<br>"
                "<b>Latest Version:</b> %2<br>"
                "<b>File:</b> %3<br>"
                "<b>Size:</b> %.1f MB<br>"
                "<b>Released:</b> %4<br><br>"
                "A newer version is available. Download it here <a href='%5'>%5</a>.<br>"
                "Uninstall the old version before you install the new one."
            ).arg(VersionNumber, latestVersion, filename).arg(fileSizeMB).arg(lastModified, downloadUrl));
            info->setTextFormat(Qt::RichText);
            info->setTextInteractionFlags(Qt::TextBrowserInteraction);
            info->setOpenExternalLinks(true);
            info->setStyleSheet("color: #2c3e50; font-size: 10pt; margin: 10px;");
            layout->addWidget(info);
            
            QHBoxLayout *btnLayout = new QHBoxLayout();
            QPushButton *downloadBtn = new QPushButton("Download");
            downloadBtn->setObjectName("downloadBtn");
            QPushButton *laterBtn = new QPushButton("Later");
            
            btnLayout->addWidget(downloadBtn);
            btnLayout->addWidget(laterBtn);
            layout->addLayout(btnLayout);
            
            QObject::connect(downloadBtn, &QPushButton::clicked, [&dlg, downloadUrl]() {
                QDesktopServices::openUrl(QUrl(downloadUrl));
                dlg.accept();
            });
            
            QObject::connect(laterBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
            
            dlg.exec();
        } else {
            if (!silent) {
                QMessageBox msgBox(parent);
                msgBox.setWindowTitle("Update Check");
                msgBox.setText(QString("You have the latest version (%1)!").arg(VersionNumber));
                msgBox.setIcon(QMessageBox::Information);
                msgBox.setStyleSheet(
                    "QMessageBox { "
                    "    background-color: #f8f9fa; "
                    "    border: 2px solid #34495e; "
                    "    border-radius: 8px; "
                    "} "
                    "QPushButton { "
                    "    background-color: #27ae60; "
                    "    color: white; "
                    "    border: none; "
                    "    border-radius: 5px; "
                    "    font-weight: bold; "
                    "    padding: 8px 16px; "
                    "    font-size: 10pt; "
                    "} "
                    "QPushButton:hover { background-color: #2ecc71; } "
                    "QPushButton:pressed { background-color: #229954; }"
                );
                msgBox.exec();
            }
        }
    });
    
    QObject::connect(timer, &QTimer::timeout, [=]() {
        reply->abort();
        reply->deleteLater();
        manager->deleteLater();
        timer->deleteLater();
        if (!silent) {
            QMessageBox::warning(parent, "Update Check", "Update check timed out.");
        }
    });
    
    timer->start(10000); // 10 second timeout
}

// Declaring functions for port scanner dialog
void showPortScanDialog(QWidget *parent, const QString &initialTarget = QString());
void showLanSharesDialog(QWidget *parent, const QString &singleTarget = QString());
void showSslCertificateDialog(QWidget *parent = nullptr);
void showPingDialog(QWidget *parent);
void showTracerouteDialog(QWidget *parent);

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

// Helper: Simple clickable label widget for text links
class SimpleClickableLabel : public QLabel {
public:
    SimpleClickableLabel(const QString &text, std::function<void()> clickHandler, QWidget *parent = nullptr)
        : QLabel(text, parent), m_clickHandler(clickHandler) {
        setCursor(Qt::PointingHandCursor);
    }

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            m_clickHandler();
        }
        QLabel::mousePressEvent(event);
    }

private:
    std::function<void()> m_clickHandler;
};

void showNetworkScannerDialog(QWidget *parent) {
    QDialog dlg;
    dlg.setWindowTitle("Network IP Scanner");
    dlg.setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    QScreen *screen = QApplication::primaryScreen();
    qreal dpiRatio = screen->devicePixelRatio();
    int physicalWidth = static_cast<int>(1200 / dpiRatio);
    int physicalHeight = static_cast<int>(800 / dpiRatio);
    dlg.setFixedSize(physicalWidth, physicalHeight);
    dlg.resize(physicalWidth, physicalHeight);

    dlg.setStyleSheet(
        "QDialog { "
        "    background-color: #f8f9fa; "
        "    border: 2px solid #34495e; "
        "    border-radius: 8px; "
        "}"
    );

    QLabel *prompt = new QLabel("Network IP Scanner - Scan for devices in your local network:", &dlg);
    prompt->setGeometry(10, 10, physicalWidth-20, 25);
    prompt->setStyleSheet("QLabel { color: #2c3e50; font-weight: bold; font-size: 11pt; }");

    QLabel *fromLabel = new QLabel("From IP:", &dlg);
    fromLabel->setGeometry(10, 45, 60, 25);
    fromLabel->setStyleSheet("QLabel { color: #2c3e50; font-weight: bold; }");

    QLineEdit *fromEdit = new QLineEdit(&dlg);
    fromEdit->setGeometry(75, 45, 150, 25);
    fromEdit->setStyleSheet(
        "QLineEdit { "
        "    border: 2px solid #3498db; "
        "    border-radius: 4px; "
        "    padding: 4px; "
        "    font-size: 10pt; "
        "} "
        "QLineEdit:focus { border-color: #2980b9; }"
    );

    QLabel *toLabel = new QLabel("To IP:", &dlg);
    toLabel->setGeometry(240, 45, 45, 25);
    toLabel->setStyleSheet("QLabel { color: #2c3e50; font-weight: bold; }");

    QLineEdit *toEdit = new QLineEdit(&dlg);
    toEdit->setGeometry(290, 45, 150, 25);
    toEdit->setStyleSheet(
        "QLineEdit { "
        "    border: 2px solid #3498db; "
        "    border-radius: 4px; "
        "    padding: 4px; "
        "    font-size: 10pt; "
        "} "
        "QLineEdit:focus { border-color: #2980b9; }"
    );

    QHBoxLayout *progressLayout = new QHBoxLayout();
    progressLayout->setContentsMargins(10, 0, 10, 0);
    progressLayout->setSpacing(0);
    QProgressBar *progress = new QProgressBar(&dlg);
    progress->setMinimum(0);
    progress->setMaximum(254);
    progress->setValue(0);
    progress->setTextVisible(true);
    progress->setStyleSheet(
        "QProgressBar { "
        "    background-color: #f8f9fa; "
        "    border: 2px solid #34495e; "
        "    border-radius: 8px; "
        "    text-align: center; "
        "    font-weight: bold; "
        "    color: #2c3e50; "
        "    min-height: 22px; "
        "} "
        "QProgressBar::chunk { "
        "    background-color: #3498db; "
        "    border-radius: 6px; "
        "}"
    );
    progressLayout->addWidget(progress, 1);
    QWidget *progressWidget = new QWidget(&dlg);
    progressWidget->setGeometry(10, 78, physicalWidth-20, 32);
    progressWidget->setLayout(progressLayout);
    progressWidget->show();

    QLabel *foundLabel = new QLabel("Devices found: 0", &dlg);
    foundLabel->setGeometry(10, 109, physicalWidth-20, 25);
    foundLabel->setAlignment(Qt::AlignCenter);
    foundLabel->setStyleSheet("QLabel { color: #2c3e50; font-weight: bold; font-size: 10pt; }");

    QTableWidget *deviceTable = new QTableWidget(&dlg);
    deviceTable->setColumnCount(4);
    QStringList headers = {"IP Address", "Host Name", "Port Scan", "Shares"};
    deviceTable->setHorizontalHeaderLabels(headers);
    deviceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    deviceTable->setSelectionMode(QAbstractItemView::NoSelection);
    deviceTable->setFocusPolicy(Qt::NoFocus);
    deviceTable->verticalHeader()->setVisible(false);

    int tableWidth = physicalWidth - 20;
    int tableHeight = physicalHeight - 195;
    deviceTable->setGeometry(10, 145, tableWidth, tableHeight);
    deviceTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    deviceTable->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    deviceTable->setStyleSheet(
        "QTableWidget { "
        "    background-color: #ecf0f1; "
        "    border: 2px solid #34495e; "
        "    border-radius: 5px; "
        "    gridline-color: #bdc3c7; "
        "} "
        "QTableWidget::item { "
        "    padding: 6px; "
        "    border-bottom: 1px solid #d5dbdb; "
        "} "
        "QHeaderView::section { "
        "    background-color: #34495e; "
        "    color: white; "
        "    padding: 8px; "
        "    border: none; "
        "    font-weight: bold; "
        "}"
    );

    QFont headerFont = deviceTable->horizontalHeader()->font();
    headerFont.setBold(true);
    deviceTable->horizontalHeader()->setFont(headerFont);

    int colWidths[] = {120, 200, 120, 120};
    for (int i = 0; i < 4; i++) {
        deviceTable->setColumnWidth(i, colWidths[i]);
    }

    int buttonY = physicalHeight - 47;
    int buttonWidth = 100;
    int buttonSpacing = 15;
    int totalButtonWidth = (buttonWidth * 2) + buttonSpacing;
    int startX = (physicalWidth - totalButtonWidth) / 2;

    QPushButton *scanBtn = new QPushButton("Start Scan", &dlg);
    scanBtn->setGeometry(startX, buttonY, buttonWidth, 35);
    scanBtn->setStyleSheet(
        "QPushButton { "
        "    background-color: #27ae60; "
        "    color: white; "
        "    border: none; "
        "    border-radius: 5px; "
        "    font-weight: bold; "
        "    font-size: 10pt; "
        "} "
        "QPushButton:hover { background-color: #2ecc71; } "
        "QPushButton:pressed { background-color: #229954; }"
    );

    QPushButton *stopCloseBtn = new QPushButton("Close", &dlg);
    stopCloseBtn->setGeometry(startX + buttonWidth + buttonSpacing, buttonY, buttonWidth, 35);
    stopCloseBtn->setStyleSheet(
        "QPushButton { "
        "    background-color: #34495e; "
        "    color: white; "
        "    border: none; "
        "    border-radius: 5px; "
        "    font-weight: bold; "
        "    font-size: 10pt; "
        "} "
        "QPushButton:hover { background-color: #2c3e50; } "
        "QPushButton:pressed { background-color: #1a252f; }"
    );

    QFrame *hr = new QFrame(&dlg);
    int hrWidth = int(physicalWidth * 0.75);
    int hrX = (physicalWidth - hrWidth) / 2;
    int hrY = buttonY + 35 + 28;
    hr->setGeometry(hrX, hrY, hrWidth, 2);
    hr->setFrameShape(QFrame::HLine);
    hr->setFrameShadow(QFrame::Sunken);
    hr->setLineWidth(2);
    hr->setStyleSheet("QFrame { background: #b0b6c3; border-radius: 2px; }");

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

    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), &dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, &dlg, &QDialog::accept);

    QThreadPool *pool = new QThreadPool(&dlg);
    pool->setMaxThreadCount(16);

    struct DeviceInfo {
        QString ip;
        QString host;
        bool hasHttp;
        bool hasHttps;
    };
    auto foundDevices = std::make_shared<QList<DeviceInfo>>();

    auto scanRunning = std::make_shared<bool>(false);
    auto cancelRequested = std::make_shared<bool>(false);

    auto updateStopCloseBtn = [&]() {
        if (*scanRunning) {
            stopCloseBtn->setText("Stop");
            stopCloseBtn->setToolTip("Stop the current scan");
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
            closeShortcut->setEnabled(false);
        } else {
            stopCloseBtn->setText("Close");
            stopCloseBtn->setToolTip("Close this dialog");
            stopCloseBtn->setStyleSheet(
                "QPushButton { "
                "    background-color: #34495e; "
                "    color: white; "
                "    border: none; "
                "    border-radius: 5px; "
                "    font-weight: bold; "
                "} "
                "QPushButton:hover { background-color: #2c3e50; } "
                "QPushButton:pressed { background-color: #1a252f; }"
            );
            closeShortcut->setEnabled(true);
        }
    };

    QObject::connect(scanBtn, &QPushButton::clicked, [=, &dlg]() {
        QString fromIp = fromEdit->text().trimmed();
        QString toIp = toEdit->text().trimmed();

        QHostAddress fromAddr(fromIp), toAddr(toIp);
        if (fromAddr.protocol() != QAbstractSocket::IPv4Protocol ||
            toAddr.protocol() != QAbstractSocket::IPv4Protocol) {
            QMessageBox msgBox(&dlg);
            msgBox.setWindowTitle("Input Error");
            msgBox.setText("Invalid IP address format.");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStyleSheet(
                "QMessageBox { "
                "    background-color: #f8f9fa; "
                "    border: 2px solid #34495e; "
                "    border-radius: 8px; "
                "} "
                "QPushButton { "
                "    background-color: #e74c3c; "
                "    color: white; "
                "    border: none; "
                "    border-radius: 5px; "
                "    font-weight: bold; "
                "    padding: 8px 16px; "
                "    font-size: 10pt; "
                "} "
                "QPushButton:hover { background-color: #c0392b; } "
                "QPushButton:pressed { background-color: #a93226; }"
            );
            msgBox.exec();
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
            pool->start([=, &dlg]() {
                if (*cancelRequested) return;

                QString ip = ipList[i];
                QProcess ping;
                ping.start("ping", QStringList() << "-n" << "1" << "-w" << "100" << ip);
                ping.waitForFinished(300);
                QString result = ping.readAllStandardOutput();
                bool alive = result.contains("TTL=");

                if (alive) {
                    QString host = QHostInfo::fromName(ip).hostName();
                    if (host == ip) host = "";

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

                    QList<DeviceInfo> sorted = *foundDevices;
                    std::sort(sorted.begin(), sorted.end(), [](const DeviceInfo &a, const DeviceInfo &b) {
                        return QHostAddress(a.ip).toIPv4Address() < QHostAddress(b.ip).toIPv4Address();
                    });

                    QMetaObject::invokeMethod(deviceTable, [=, dlgPtr = &dlg]() {
                        deviceTable->setRowCount(0);
                        for (int idx = 0; idx < sorted.size(); ++idx) {
                            const DeviceInfo &dev = sorted[idx];
                            int row = deviceTable->rowCount();
                            deviceTable->insertRow(row);

                            QTableWidgetItem *ipItem = new QTableWidgetItem(dev.ip);
                            ipItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
                            if (dev.hasHttps || dev.hasHttp) {
                                ipItem->setForeground(QBrush(QColor("#3498db")));
                                QFont linkFont = ipItem->font();
                                linkFont.setUnderline(true);
                                linkFont.setBold(true);
                                ipItem->setFont(linkFont);
                                ipItem->setToolTip("Click to open web interface");
                                ipItem->setData(Qt::UserRole + 10, true);
                            }
                            deviceTable->setItem(row, 0, ipItem);

                            QTableWidgetItem *hostItem = new QTableWidgetItem(dev.host);
                            hostItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                            deviceTable->setItem(row, 1, hostItem);

                            QWidget *portWidget = new QWidget;
                            QHBoxLayout *portLayout = new QHBoxLayout(portWidget);
                            portLayout->setContentsMargins(8, 0, 0, 0);
                            portLayout->setSpacing(0);

                            auto portScanLink = new SimpleClickableLabel("Port Scan ►", [dlgPtr, ip = dev.ip]() {
                                QTimer::singleShot(0, [dlgPtr, ip]() {
                                    showPortScanDialog(static_cast<QWidget*>(dlgPtr), ip);
                                });
                            });
                            portScanLink->setToolTip("Scan ports on this device");
                            portScanLink->setStyleSheet(
                                "QLabel { "
                                "    color: #3498db; "
                                "    font-size: 10pt; "
                                "    padding: 2px; "
                                "} "
                                "QLabel:hover { "
                                "    color: #e74c3c; "
                                "}"
                            );
                            portLayout->addWidget(portScanLink);
                            portLayout->addStretch();
                            deviceTable->setCellWidget(row, 2, portWidget);

                            QWidget *sharesWidget = new QWidget;
                            QHBoxLayout *sharesLayout = new QHBoxLayout(sharesWidget);
                            sharesLayout->setContentsMargins(8, 0, 0, 0);
                            sharesLayout->setSpacing(0);

                            auto sharesLink = new SimpleClickableLabel("Shares ►", [dlgPtr, ip = dev.ip]() {
                                QTimer::singleShot(0, [dlgPtr, ip]() {
                                    showLanSharesDialog(static_cast<QWidget*>(dlgPtr), ip);
                                });
                            });
                            sharesLink->setToolTip("Show shared folders on this device");
                            sharesLink->setStyleSheet(
                                "QLabel { "
                                "    color: #e67e22; "
                                "    font-size: 10pt; "
                                "    padding: 2px; "
                                "} "
                                "QLabel:hover { "
                                "    color: #e74c3c; "
                                "}"
                            );
                            sharesLayout->addWidget(sharesLink);
                            sharesLayout->addStretch();
                            deviceTable->setCellWidget(row, 3, sharesWidget);
                        }
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
                        progress->setValue(progress->maximum());
                    }
                }, Qt::QueuedConnection);
            });
        }
    });

    QObject::connect(deviceTable, &QTableWidget::itemClicked, [=](QTableWidgetItem *item) {
        if (item->column() == 0) {
            QString ip = item->text();
            for (const DeviceInfo &dev : *foundDevices) {
                if (dev.ip == ip) {
                    if (dev.hasHttps) {
                        QDesktopServices::openUrl(QUrl("https://" + ip));
                    } else if (dev.hasHttp) {
                        QDesktopServices::openUrl(QUrl("http://" + ip));
                    }
                    break;
                }
            }
        }
    });

    QObject::connect(stopCloseBtn, &QPushButton::clicked, [=, &dlg]() {
        if (*scanRunning) {
            *cancelRequested = true;
            *scanRunning = false;
            scanBtn->setEnabled(true);
            updateStopCloseBtn();
            progress->setValue(progress->maximum());
        } else {
            dlg.accept();
        }
    });

    updateStopCloseBtn();
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
        "<tr><td><b>External IP</b></td><td align='right'>" + externalIp + "</td></tr>"
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
    // Always allocate dialog on the heap and set WA_DeleteOnClose for safety
    QDialog *dlg = new QDialog(parent);
    dlg->setWindowTitle("Port Scan");
    dlg->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowStaysOnTopHint);
    dlg->setAttribute(Qt::WA_DeleteOnClose); // Ensure proper cleanup

    // Apply modern styling to dialog
    dlg->setStyleSheet(
        "QDialog { "
        "    background-color: #f8f9fa; "
        "    border: 2px solid #34495e; "
        "    border-radius: 8px; "
        "}"
    );

    QVBoxLayout *layout = new QVBoxLayout(dlg);

    QLabel *inputLabel = new QLabel("Target (IP or hostname):");
    inputLabel->setStyleSheet("QLabel { color: #34495e; font-weight: bold; font-size: 10pt; }");

    QLineEdit *targetEdit = new QLineEdit(initialTarget.isEmpty() ? "127.0.0.1" : initialTarget);
    targetEdit->setToolTip("Enter the IP address or hostname to scan.");
    targetEdit->setStyleSheet(
        "QLineEdit { "
        "    border: 2px solid #3498db; "
        "    border-radius: 4px; "
        "    padding: 6px; "
        "    font-size: 10pt; "
        "} "
        "QLineEdit:focus { border-color: #2980b9; }"
    );

    QLabel *rangeLabel = new QLabel("Port range (e.g. 1-1024):");
    rangeLabel->setStyleSheet("QLabel { color: #34495e; font-weight: bold; font-size: 10pt; }");

    QLineEdit *rangeEdit = new QLineEdit("1-1024");
    rangeEdit->setToolTip("<div style='white-space:nowrap;'>Enter the port range to scan. You can use a format like 1-1024.<BR>"
                           "The default is 1-1024, which is the most common range for services.<BR>"
                           "You can also specify a single port.</div>");
    rangeEdit->setStyleSheet(
        "QLineEdit { "
        "    border: 2px solid #3498db; "
        "    border-radius: 4px; "
        "    padding: 6px; "
        "    font-size: 10pt; "
        "} "
        "QLineEdit:focus { border-color: #2980b9; }"
    );

    layout->addWidget(inputLabel);
    layout->addWidget(targetEdit);
    layout->addWidget(rangeLabel);
    layout->addWidget(rangeEdit);

    QHBoxLayout *currentPortLayout = new QHBoxLayout();
    QLabel *checkingLabel = new QLabel("Checking port number:");
    checkingLabel->setStyleSheet("QLabel { color: #34495e; font-weight: bold; font-size: 10pt; }");

    QLineEdit *currentPortEdit = new QLineEdit;
    currentPortEdit->setToolTip("The port that is being checked.");
    currentPortEdit->setReadOnly(true);
    currentPortEdit->setAlignment(Qt::AlignCenter);
    QFont font = currentPortEdit->font();
    font.setPointSize(14);
    font.setBold(true);
    currentPortEdit->setFont(font);
    currentPortEdit->setFixedWidth(100);
    currentPortEdit->setStyleSheet(
        "QLineEdit { "
        "    border: 2px solid #e67e22; "
        "    border-radius: 4px; "
        "    padding: 6px; "
        "    background-color: #fdf2e9; "
        "    color: #d35400; "
        "    font-weight: bold; "
        "}"
    );

    QLabel *progressLabel = new QLabel("0 % done");
    progressLabel->setToolTip("<div style='white-space:nowrap;'>Progress of the scan in percent.<BR>"
                              "This will update as ports are checked.</div>");
    progressLabel->setFixedWidth(120);
    progressLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    progressLabel->setStyleSheet(
        "QLabel { "
        "    color: #27ae60; "
        "    font-weight: bold; "
        "    font-size: 11pt; "
        "    background-color: #e8f5e8; "
        "    border: 2px solid #27ae60; "
        "    border-radius: 4px; "
        "    padding: 4px; "
        "}"
    );

    currentPortLayout->addWidget(checkingLabel);
    currentPortLayout->addWidget(currentPortEdit);
    currentPortLayout->addWidget(progressLabel);
    layout->addLayout(currentPortLayout);

    QLabel *etaLabel = new QLabel("Estimated time remaining: --");
    etaLabel->setToolTip("Estimated time remaining for your scan to complete.");
    etaLabel->setMinimumWidth(320);
    etaLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    etaLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    etaLabel->setStyleSheet("QLabel { color: #34495e; font-weight: bold; font-size: 10pt; }");
    layout->addWidget(etaLabel);

    QTextEdit *output = new QTextEdit;
    output->setReadOnly(true);
    output->setLineWrapMode(QTextEdit::NoWrap);
    output->setMinimumHeight(120);
    output->setTextInteractionFlags(Qt::NoTextInteraction); // Disable all text interaction including selection
    output->setStyleSheet(
        "QTextEdit { "
        "    background-color: #ecf0f1; "
        "    border: 2px solid #34495e; "
        "    border-radius: 5px; "
        "    font-family: 'Consolas', 'Courier New', monospace; "
        "    font-size: 9pt; "
        "    padding: 6px; "
        "}"
    );
    layout->addWidget(output);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *scanBtn = new QPushButton("Scan");
    scanBtn->setToolTip("Start scanning the specified port range on the target.");
    scanBtn->setStyleSheet(
        "QPushButton { "
        "    background-color: #27ae60; "
        "    color: white; "
        "    border: none; "
        "    border-radius: 5px; "
        "    font-weight: bold; "
        "    padding: 8px 16px; "
        "    font-size: 10pt; "
        "} "
        "QPushButton:hover { background-color: #2ecc71; } "
        "QPushButton:pressed { background-color: #229954; }"
    );

    QPushButton *stopCloseBtn = new QPushButton("Close");
    stopCloseBtn->setToolTip("Close the dialog.");
    stopCloseBtn->setStyleSheet(
        "QPushButton { "
        "    background-color: #34495e; "
        "    color: white; "
        "    border: none; "
        "    border-radius: 5px; "
        "    font-weight: bold; "
        "    padding: 8px 16px; "
        "    font-size: 10pt; "
        "} "
        "QPushButton:hover { background-color: #3c5872; } "
        "QPushButton:pressed { background-color: #22313a; }"
    );

    btnLayout->addWidget(scanBtn);
    btnLayout->addWidget(stopCloseBtn);
    layout->addLayout(btnLayout);

    auto scanRunning = std::make_shared<bool>(false);

    auto updateStopCloseText = [=]() {
        if (*scanRunning) {
            stopCloseBtn->setText("Stop");
            stopCloseBtn->setToolTip("Stop current scan");
        } else {
            stopCloseBtn->setText("Close");
            stopCloseBtn->setToolTip("Close the dialog");
        }
    };
    updateStopCloseText();

    // Add Ctrl+W shortcut specifically for this dialog (allows force close during scanning)
    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), dlg);
    closeShortcut->setContext(Qt::WidgetShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, [dlg]() {
        dlg->close();
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
                            QString("<span style='color:#2c3e50; font-weight:bold;'>%1</span> "
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

    QObject::connect(stopCloseBtn, &QPushButton::clicked, [=]() mutable {
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

    // Handle dialog finished signal (proper cleanup)
    QObject::connect(dlg, &QDialog::finished, [=]() {
        *scanRunning = false;
        // No manual delete needed; WA_DeleteOnClose is set
    });

    // Also handle reject (X button, Escape key, etc.)
    QObject::connect(dlg, &QDialog::rejected, [=]() {
        *scanRunning = false;
        if (dlg) dlg->close();
    });

    dlg->adjustSize();
    dlg->exec(); // Use exec() for proper modal behavior and event handling
    dlg->raise(); // Bring to front
    dlg->activateWindow(); // Give focus
    dlg->setFocus(); // Ensure focus
}


void showTracerouteDialog(QWidget *parent) {
    QDialog *dlg = new QDialog(parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle("Traceroute Host");
    dlg->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    QScreen *screen = QApplication::primaryScreen();
    qreal dpiRatio = screen->devicePixelRatio();
    int physicalWidth = static_cast<int>(1400 / dpiRatio);
    int physicalHeight = static_cast<int>(850 / dpiRatio);
    dlg->setFixedSize(physicalWidth, physicalHeight);

    QLabel *prompt = new QLabel("Enter host or IP to trace:", dlg);
    prompt->setGeometry(10, 10, physicalWidth-20, 25);
    prompt->setStyleSheet("QLabel { color: #2c3e50; font-weight: bold; }");

    QLineEdit *input = new QLineEdit(dlg);
    input->setPlaceholderText("e.g. 8.8.8.8 or www.google.com");
    input->setGeometry(10, 40, physicalWidth-20, 25);
    input->setStyleSheet("QLineEdit { border: 2px solid #3498db; border-radius: 4px; padding: 2px; }");

    QLabel *hopsLabel = new QLabel("Max hops:", dlg);
    hopsLabel->setGeometry(10, 75, 80, 25);
    hopsLabel->setStyleSheet("QLabel { color: #34495e; font-weight: bold; }");

    QSpinBox *hopsSpin = new QSpinBox(dlg);
    hopsSpin->setToolTip("Maximum number of hops to trace. Default is 30.");
    hopsSpin->setRange(1, 64);
    hopsSpin->setValue(30);
    hopsSpin->setGeometry(100, 75, 80, 25);
    hopsSpin->setStyleSheet("QSpinBox { border: 1px solid #bdc3c7; border-radius: 3px; }");

    int tableY = 145;
    QTableWidget *table = new QTableWidget(dlg);
    table->setColumnCount(6);
    QStringList headers = {"Hop", "IP/Host", "RTT 1", "RTT 2", "RTT 3", "Comments"};
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setVisible(false);

    int tableWidth = physicalWidth - 20;
    int tableHeight = physicalHeight - 200;
    table->setGeometry(10, tableY, tableWidth, tableHeight);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    table->setStyleSheet(
        "QTableWidget { background-color: #ecf0f1; border: 2px solid #34495e; border-radius: 5px; gridline-color: #bdc3c7; } "
        "QTableWidget::item { padding: 4px; border-bottom: 1px solid #d5dbdb; } "
        "QTableWidget::item:selected { background-color: #3498db; color: white; } "
        "QHeaderView::section { background-color: #34495e; color: white; padding: 5px; border: none; font-weight: bold; }"
    );
    QFont headerFont = table->horizontalHeader()->font();
    headerFont.setBold(true);
    table->horizontalHeader()->setFont(headerFont);

    int colWidths[] = {50, 250, 70, 70, 70, tableWidth-510};
    for (int i = 0; i < 6; i++) table->setColumnWidth(i, colWidths[i]);

    int buttonY = physicalHeight - 45;
    int buttonWidth = 80;
    int buttonHeight = 35;
    int buttonSpacing = 20;
    int numButtons = 4;
    int totalButtonWidth = (buttonWidth * numButtons) + (buttonSpacing * (numButtons - 1));
    int startX = (physicalWidth - totalButtonWidth) / 2;

    int spinnerW = 28, spinnerH = 28;
    QFrame *spinnerFrame = new QFrame(dlg);
    spinnerFrame->setFixedSize(spinnerW + 8, spinnerH + 8);
    spinnerFrame->setStyleSheet("QFrame { border: 2px solid black; border-radius: 8px; background: transparent; }");
    int spinnerX = startX - spinnerFrame->width() - 16;
    int spinnerY = buttonY + (buttonHeight - spinnerFrame->height()) / 2;
    spinnerFrame->setGeometry(spinnerX, spinnerY, spinnerFrame->width(), spinnerFrame->height());
    spinnerFrame->setVisible(true);

    QLabel *spinnerLabel = new QLabel(spinnerFrame);
    spinnerLabel->setFixedSize(spinnerW, spinnerH);
    spinnerLabel->move(4, 4);
    spinnerLabel->setAlignment(Qt::AlignCenter);
    spinnerLabel->setStyleSheet("QLabel { font-size: 18pt; color: #e67e22; background: transparent; }");
    spinnerLabel->setVisible(false);

    QStringList spinnerFrames = {"⠋","⠙","⠹","⠸","⠼","⠴","⠦","⠧","⠇","⠏"};
    int spinnerFrameIdx = 0;
    QTimer *spinnerTimer = new QTimer(dlg);
    spinnerTimer->setInterval(80);
    QObject::connect(spinnerTimer, &QTimer::timeout, [spinnerLabel, spinnerFrame, &spinnerFrameIdx, spinnerFrames]() mutable {
        spinnerLabel->setText(spinnerFrames[spinnerFrameIdx]);
        spinnerFrame->setVisible(true);
        spinnerLabel->setVisible(true);
        spinnerFrameIdx = (spinnerFrameIdx + 1) % spinnerFrames.size();
    });

    QPushButton *traceBtn = new QPushButton("Start", dlg);
    traceBtn->setToolTip("Start the traceroute to the specified host.");
    traceBtn->setGeometry(startX, buttonY, buttonWidth, buttonHeight);
    traceBtn->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; border: none; border-radius: 5px; font-weight: bold; } "
        "QPushButton:hover { background-color: #2ecc71; } "
        "QPushButton:pressed { background-color: #229954; }"
    );

    QPushButton *bottomBtn = new QPushButton("Bottom", dlg);
    bottomBtn->setToolTip("Scroll to the bottom of the output.");
    bottomBtn->setGeometry(startX + (buttonWidth + buttonSpacing) * 1, buttonY, buttonWidth, buttonHeight);
    bottomBtn->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; border: none; border-radius: 5px; font-weight: bold; } "
        "QPushButton:hover { background-color: #2980b9; } "
        "QPushButton:pressed { background-color: #1f618d; }"
    );

    QPushButton *copyBtn = new QPushButton("Copy All", dlg);
    copyBtn->setToolTip("Copy all traceroute results to clipboard in CSV format.");
    copyBtn->setGeometry(startX + (buttonWidth + buttonSpacing) * 2, buttonY, buttonWidth, buttonHeight);
    copyBtn->setStyleSheet(
        "QPushButton { background-color: #9b59b6; color: white; border: none; border-radius: 5px; font-weight: bold; } "
        "QPushButton:hover { background-color: #8e44ad; } "
        "QPushButton:pressed { background-color: #7d3c98; }"
    );

    QPushButton *stopCloseBtn = new QPushButton("Close", dlg);
    stopCloseBtn->setToolTip("Stop the traceroute or close the dialog.");
    stopCloseBtn->setGeometry(startX + (buttonWidth + buttonSpacing) * 3, buttonY, buttonWidth, buttonHeight);
    stopCloseBtn->setStyleSheet(
        "QPushButton { background-color: #e74c3c; color: white; border: none; border-radius: 5px; font-weight: bold; } "
        "QPushButton:hover { background-color: #c0392b; } "
        "QPushButton:pressed { background-color: #a93226; }"
    );

    auto isTracing = std::make_shared<bool>(false);
    QProcess **lastProcPtr = new QProcess*(nullptr);

    auto updateStopCloseText = [=]() {
        if (*isTracing) {
            stopCloseBtn->setText("Stop");
            stopCloseBtn->setToolTip("Stop the traceroute");
            stopCloseBtn->setStyleSheet(
                "QPushButton { background-color: #e74c3c; color: white; border: none; border-radius: 5px; font-weight: bold; } "
                "QPushButton:hover { background-color: #c0392b; } "
                "QPushButton:pressed { background-color: #a93226; }"
            );
            spinnerFrame->setVisible(true);
            spinnerLabel->setVisible(true);
            spinnerTimer->start();
        } else {
            stopCloseBtn->setText("Close");
            stopCloseBtn->setToolTip("Close the dialog");
            stopCloseBtn->setStyleSheet(
                "QPushButton { background-color: #34495e; color: white; border: none; border-radius: 5px; font-weight: bold; } "
                "QPushButton:hover { background-color: #3c5872; } "
                "QPushButton:pressed { background-color: #22313a; }"
            );
            spinnerFrame->setVisible(false);
            spinnerLabel->setVisible(false);
            spinnerTimer->stop();
        }
    };
    updateStopCloseText();

    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, [dlg, isTracing]() {
        if (!*isTracing) dlg->close();
    });

    int sortColumn = 0;
    Qt::SortOrder sortOrder = Qt::AscendingOrder;
    auto updateHeaderArrows = [=]() {
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
    auto hops = std::make_shared<QList<HopEntry>>();

    auto applyRowColors = [=](int row, const HopEntry &e) {
        QColor rowColor;
        if (e.ipHost == "*" || e.comment.contains("timed out")) rowColor = QColor("#ffebee");
        else if (e.ipHost.startsWith("192.168.") || e.ipHost.startsWith("10.") || e.ipHost.startsWith("172.")) rowColor = QColor("#e8f5e8");
        else rowColor = QColor("#f0f8ff");
        for (int col = 0; col < 6; col++) {
            if (table->item(row, col)) {
                table->item(row, col)->setBackground(rowColor);
                if (col == 0) table->item(row, col)->setForeground(QColor("#2c3e50"));
                else if (col == 1) table->item(row, col)->setForeground(QColor("#27ae60"));
                else if (col >= 2 && col <= 4) table->item(row, col)->setForeground(QColor("#e67e22"));
                else table->item(row, col)->setForeground(QColor("#8e44ad"));
            }
        }
    };

    auto fillTable = [=]() {
        table->setRowCount(0);
        QMap<int, HopEntry> uniqueHops;
        for (const HopEntry &e : *hops) if (e.hop > 0) uniqueHops[e.hop] = e;
        QList<HopEntry> sortedHops = uniqueHops.values();
        std::sort(sortedHops.begin(), sortedHops.end(), [](const HopEntry &a, const HopEntry &b) { return a.hop < b.hop; });
        for (const HopEntry &e : sortedHops) {
            int row = table->rowCount();
            table->insertRow(row);
            QTableWidgetItem *hopItem = new QTableWidgetItem(QString::number(e.hop));
            QTableWidgetItem *ipHostItem = new QTableWidgetItem(e.ipHost);
            QTableWidgetItem *rtt1Item = new QTableWidgetItem(e.rtt1);
            QTableWidgetItem *rtt2Item = new QTableWidgetItem(e.rtt2);
            QTableWidgetItem *rtt3Item = new QTableWidgetItem(e.rtt3);
            QTableWidgetItem *commentItem = new QTableWidgetItem(e.comment);
            hopItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            rtt1Item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            rtt2Item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            rtt3Item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
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
        if (table->rowCount() > 0) table->scrollToBottom();
    };

    QObject::connect(traceBtn, &QPushButton::clicked, [=]() {
        QString target = input->text().trimmed();
        if (target.isEmpty()) {
            input->setFocus();
            input->setPlaceholderText("Please enter a host or IP address!");
            return;
        }
        hops->clear();
        fillTable();
        *isTracing = true;
        updateStopCloseText();
        input->setReadOnly(true);
        traceBtn->setEnabled(false);
        hopsSpin->setEnabled(false);

        if (*lastProcPtr) { (*lastProcPtr)->deleteLater(); *lastProcPtr = nullptr; }
        QProcess *lastProc = new QProcess(dlg);
        *lastProcPtr = lastProc;
        lastProc->setProgram("tracert");
        lastProc->setArguments({"-h", QString::number(hopsSpin->value()), target});

        static QString outputBuffer;
        QObject::connect(lastProc, &QProcess::readyReadStandardOutput, [=]() {
            QByteArray data = (*lastProcPtr)->readAllStandardOutput();
            QString newOutput = QString::fromLocal8Bit(data);
            outputBuffer += newOutput;
            QStringList lines = outputBuffer.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
            if (!newOutput.endsWith('\n') && !newOutput.endsWith('\r')) {
                if (!lines.isEmpty()) outputBuffer = lines.takeLast();
                else outputBuffer.clear();
            } else outputBuffer.clear();
            for (const QString &line : lines) {
                QString cleaned = line.trimmed();
                if (cleaned.isEmpty()) continue;
                if (cleaned.contains("Tracing route to") || cleaned.contains("over a maximum of") ||
                    cleaned.contains("hops:") || cleaned.contains("Trace complete")) continue;
                QRegularExpression hopNumRe(R"(^\s*(\d+)\s+(.*)$)");
                QRegularExpressionMatch numMatch = hopNumRe.match(cleaned);
                if (numMatch.hasMatch()) {
                    int hopNum = numMatch.captured(1).toInt();
                    QString rest = numMatch.captured(2).trimmed();
                    if (hopNum <= 0 || hopNum > 64) continue;
                    HopEntry entry;
                    entry.hop = hopNum;
                    if (rest.contains("Request timed out") || rest.startsWith("*") || rest.contains("* * *")) {
                        entry.ipHost = "*";
                        entry.rtt1 = "*";
                        entry.rtt2 = "*";
                        entry.rtt3 = "*";
                        entry.comment = "Request timed out";
                    } else {
                        QRegularExpression tracertRegex(R"(^\s*([*]|[<>]?\d+\s*ms|\d+\s*ms)\s+([*]|[<>]?\d+\s*ms|\d+\s*ms)\s+([*]|[<>]?\d+\s*ms|\d+\s*ms)\s*(.*)$)");
                        QRegularExpressionMatch match = tracertRegex.match(rest);
                        if (match.hasMatch()) {
                            entry.rtt1 = match.captured(1).trimmed();
                            entry.rtt2 = match.captured(2).trimmed();
                            entry.rtt3 = match.captured(3).trimmed();
                            QString hostPart = match.captured(4).trimmed();
                            if (!hostPart.isEmpty()) {
                                QRegularExpression hostIpRegex(R"(^(.+?)\s*\[([^\]]+)\]$)");
                                QRegularExpressionMatch hostMatch = hostIpRegex.match(hostPart);
                                if (hostMatch.hasMatch()) {
                                    entry.comment = hostMatch.captured(1).trimmed();
                                    entry.ipHost = hostMatch.captured(2).trimmed();
                                } else {
                                    QRegularExpression ipRegex(R"(^\d+\.\d+\.\d+\.\d+$)");
                                    if (ipRegex.match(hostPart).hasMatch()) {
                                        entry.ipHost = hostPart;
                                        entry.comment = "";
                                    } else {
                                        entry.ipHost = hostPart;
                                        entry.comment = "";
                                    }
                                }
                            } else {
                                entry.ipHost = "Unknown";
                                entry.comment = "";
                            }
                        } else {
                            QStringList tokens = rest.split(QRegularExpression(R"(\s+)"), Qt::SkipEmptyParts);
                            QStringList rttValues;
                            int tokenIdx = 0;
                            while (tokenIdx < tokens.size() && rttValues.size() < 3) {
                                QString token = tokens[tokenIdx];
                                if (token == "*" || token.contains("ms") ||
                                    (tokenIdx + 1 < tokens.size() && tokens[tokenIdx + 1] == "ms")) {
                                    if (tokenIdx + 1 < tokens.size() && tokens[tokenIdx + 1] == "ms") {
                                        rttValues.append(token + " ms");
                                        tokenIdx += 2;
                                    } else {
                                        rttValues.append(token);
                                        tokenIdx++;
                                    }
                                } else break;
                            }
                            entry.rtt1 = rttValues.size() > 0 ? rttValues[0] : "";
                            entry.rtt2 = rttValues.size() > 1 ? rttValues[1] : "";
                            entry.rtt3 = rttValues.size() > 2 ? rttValues[2] : "";
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
                    if (entry.hop > 0 && entry.hop <= 64) {
                        hops->append(entry);
                        fillTable();
                    }
                }
            }
        });

        QObject::connect(lastProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                         [=](int /*exitCode*/, QProcess::ExitStatus /*exitStatus*/) {
            outputBuffer.clear();
            *isTracing = false;
            updateStopCloseText();
            input->setReadOnly(false);
            traceBtn->setEnabled(true);
            hopsSpin->setEnabled(true);
            if (*lastProcPtr) { (*lastProcPtr)->deleteLater(); *lastProcPtr = nullptr; }
        });

        lastProc->start();
    });

    QObject::connect(stopCloseBtn, &QPushButton::clicked, [=]() {
        if (*isTracing) {
            if (*lastProcPtr) (*lastProcPtr)->kill();
        } else {
            dlg->close();
        }
    });

    QObject::connect(bottomBtn, &QPushButton::clicked, [=]() {
        if (table->rowCount() > 0) table->scrollToBottom();
    });

    QObject::connect(copyBtn, &QPushButton::clicked, [=]() {
        QString clipboardText;
        clipboardText += "\"Hop\",\"IP/Host\",\"RTT 1\",\"RTT 2\",\"RTT 3\",\"Comments\"\n";
        for (int row = 0; row < table->rowCount(); ++row) {
            QStringList rowData;
            for (int col = 0; col < table->columnCount(); ++col) {
                QTableWidgetItem *item = table->item(row, col);
                QString cellText = item ? item->text() : "";
                cellText.replace("\"", "\"\"");
                rowData.append("\"" + cellText + "\"");
            }
            clipboardText += rowData.join(",") + "\n";
        }
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setText(clipboardText);
        copyBtn->setText("Copied!");
        QTimer::singleShot(1000, [copyBtn]() { copyBtn->setText("Copy All"); });
    });

    // --- FIX: capture sortColumn and sortOrder by reference for sorting ---
    QObject::connect(table->horizontalHeader(), &QHeaderView::sectionClicked, [=, &sortColumn, &sortOrder](int logicalIndex) {
        if (logicalIndex == sortColumn) {
            sortOrder = (sortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
        } else {
            sortColumn = logicalIndex;
            sortOrder = Qt::AscendingOrder;
        }
        std::sort(hops->begin(), hops->end(), [&](const HopEntry &a, const HopEntry &b) {
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
    dlg->exec();
}

void showDhcpStatusDialog(QWidget *parent) {
    QDialog *dlg = new QDialog(parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle("DHCP Status");
    dlg->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    QScreen *screen = QApplication::primaryScreen();
    qreal dpiRatio = screen->devicePixelRatio();
    int physicalWidth = static_cast<int>(900 / dpiRatio);
    int physicalHeight = static_cast<int>(700 / dpiRatio);
    dlg->setFixedSize(physicalWidth, physicalHeight);

    dlg->setStyleSheet(
        "QDialog { "
        "    background-color: #f8f9fa; "
        "    border: 2px solid #34495e; "
        "    border-radius: 8px; "
        "}"
    );

    QLabel *prompt = new QLabel("DHCP Status Information:", dlg);
    prompt->setGeometry(10, 10, physicalWidth-20, 25);
    prompt->setStyleSheet("QLabel { color: #2c3e50; font-weight: bold; font-size: 11pt; }");

    QTableWidget *table = new QTableWidget(dlg);
    table->setColumnCount(2);
    QStringList headers = {"Property", "Value"};
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setVisible(false);

    int tableWidth = physicalWidth - 20;
    int tableHeight = physicalHeight - 120;
    table->setGeometry(10, 45, tableWidth, tableHeight);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    table->setStyleSheet(
        "QTableWidget { "
        "    background-color: #ecf0f1; "
        "    border: 2px solid #34495e; "
        "    border-radius: 5px; "
        "    gridline-color: #bdc3c7; "
        "} "
        "QTableWidget::item { "
        "    padding: 6px; "
        "    border-bottom: 1px solid #d5dbdb; "
        "} "
        "QTableWidget::item:selected { "
        "    background-color: #3498db; "
        "    color: white; "
        "} "
        "QHeaderView::section { "
        "    background-color: #34495e; "
        "    color: white; "
        "    padding: 8px; "
        "    border: none; "
        "    font-weight: bold; "
        "}"
    );

    QFont headerFont = table->horizontalHeader()->font();
    headerFont.setBold(true);
    table->horizontalHeader()->setFont(headerFont);

    table->setColumnWidth(0, 200);
    table->setColumnWidth(1, tableWidth - 200);

    int buttonY = physicalHeight - 45;
    int buttonWidth = 80;
    int buttonSpacing = 10;
    int totalButtonWidth = (buttonWidth * 3) + (buttonSpacing * 2);
    int startX = (physicalWidth - totalButtonWidth) / 2;

    QPushButton *refreshBtn = new QPushButton("Refresh", dlg);
    refreshBtn->setToolTip("Refresh DHCP status information.");
    refreshBtn->setGeometry(startX, buttonY, buttonWidth, 35);
    refreshBtn->setStyleSheet(
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

    QPushButton *copyBtn = new QPushButton("Copy", dlg);
    copyBtn->setToolTip("Copy DHCP information to clipboard.");
    copyBtn->setGeometry(startX + buttonWidth + buttonSpacing, buttonY, buttonWidth, 35);
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

    QPushButton *closeBtn = new QPushButton("Close", dlg);
    closeBtn->setToolTip("Close the dialog.");
    closeBtn->setGeometry(startX + (buttonWidth + buttonSpacing) * 2, buttonY, buttonWidth, 35);
    closeBtn->setStyleSheet(
        "QPushButton { "
        "    background-color: #34495e; "
        "    color: white; "
        "    border: none; "
        "    border-radius: 5px; "
        "    font-weight: bold; "
        "} "
        "QPushButton:hover { background-color: #3c5872; } "
        "QPushButton:pressed { background-color: #22313a; }"
    );

    // Function to populate table with DHCP data
    auto populateTable = [=]() {
        table->setRowCount(0);

        QProcess proc;
        proc.start("ipconfig", QStringList() << "/all");
        proc.waitForFinished();
        QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());

        QStringList blocks = output.split(QRegularExpression(R"(\r?\n\r?\n)"), Qt::SkipEmptyParts);

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
                QString ipv4Address   = extract("IPv4 Address");
                QString subnetMask    = extract("Subnet Mask");
                QString defaultGw     = extract("Default Gateway");

                int row = table->rowCount();
                table->insertRow(row);

                QTableWidgetItem *headerItem = new QTableWidgetItem("=== " + ifaceName + " ===");
                headerItem->setFont(QFont("", -1, QFont::Bold));
                headerItem->setBackground(QColor("#3498db"));
                headerItem->setForeground(QColor("white"));
                table->setItem(row, 0, headerItem);

                QTableWidgetItem *emptyItem = new QTableWidgetItem("");
                emptyItem->setBackground(QColor("#3498db"));
                table->setItem(row, 1, emptyItem);

                QStringList properties = {"DHCP Enabled", "IPv4 Address", "Subnet Mask", "Default Gateway",
                                         "DHCP Server", "Lease Obtained", "Lease Expires", "DHCP Client ID"};
                QStringList values = {"Yes", ipv4Address, subnetMask, defaultGw,
                                      dhcpServer, leaseObtained, leaseExpires, clientId};

                for (int i = 0; i < properties.size(); ++i) {
                    int dataRow = table->rowCount();
                    table->insertRow(dataRow);

                    table->setItem(dataRow, 0, new QTableWidgetItem(properties[i]));

                    QTableWidgetItem *valueItem = new QTableWidgetItem(values[i]);
                    if (properties[i] == "DHCP Enabled") {
                        valueItem->setBackground(QColor("#d5f4e6"));
                        valueItem->setForeground(QColor("#27ae60"));
                    }
                    table->setItem(dataRow, 1, valueItem);
                }

                int spacingRow = table->rowCount();
                table->insertRow(spacingRow);
                table->setItem(spacingRow, 0, new QTableWidgetItem(""));
                table->setItem(spacingRow, 1, new QTableWidgetItem(""));
                table->setRowHeight(spacingRow, 10);

                break; // Only show first DHCP-enabled adapter
            }
        }

        if (table->rowCount() == 0) {
            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem("Status"));
            QTableWidgetItem *noDataItem = new QTableWidgetItem("No active DHCP-enabled interface found");
            noDataItem->setBackground(QColor("#fadbd8"));
            noDataItem->setForeground(QColor("#e74c3c"));
            table->setItem(row, 1, noDataItem);
        }
    };

    populateTable();

    QObject::connect(refreshBtn, &QPushButton::clicked, populateTable);
    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    QObject::connect(copyBtn, &QPushButton::clicked, [=]() {
        QString csvData = "Property,Value\n";
        for (int row = 0; row < table->rowCount(); ++row) {
            QTableWidgetItem *propItem = table->item(row, 0);
            QTableWidgetItem *valueItem = table->item(row, 1);
            if (propItem && valueItem) {
                QString prop = propItem->text().replace(',', ';');
                QString value = valueItem->text().replace(',', ';');
                if (!prop.isEmpty() || !value.isEmpty()) {
                    csvData += QString("%1,%2\n").arg(prop, value);
                }
            }
        }
        QApplication::clipboard()->setText(csvData);
        copyBtn->setText("Copied!");
        QTimer::singleShot(1500, [copyBtn]() {
            copyBtn->setText("Copy");
        });
    });

    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, dlg, &QDialog::accept);

    dlg->exec();
}

void showNslookupDialog(QWidget *parent) {
    // Always allocate dialog on the heap and set WA_DeleteOnClose for safety
    auto *inputDlg = new QDialog(parent);
    inputDlg->setAttribute(Qt::WA_DeleteOnClose);
    inputDlg->setWindowTitle("NS Lookup");
    inputDlg->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    // DPI scaling
    QScreen *screen = QApplication::primaryScreen();
    qreal dpiRatio = screen->devicePixelRatio();
    int physicalWidth = static_cast<int>(500 / dpiRatio);
    int physicalHeight = static_cast<int>(300 / dpiRatio);
    inputDlg->setFixedSize(physicalWidth, physicalHeight);

    inputDlg->setStyleSheet(
        "QDialog { background-color: #f8f9fa; border: 2px solid #34495e; border-radius: 8px; }"
    );

    QLabel *prompt = new QLabel("Enter hostname or IP:", inputDlg);
    prompt->setGeometry(20, 18, physicalWidth-40, 26);
    prompt->setStyleSheet("QLabel { color: #34495e; font-weight: bold; font-size: 11pt; }");

    QLineEdit *inputEdit = new QLineEdit(inputDlg);
    inputEdit->setGeometry(20, 48, physicalWidth-40, 32);
    inputEdit->setPlaceholderText("e.g. www.google.com or 8.8.8.8");
    inputEdit->setToolTip("Enter a hostname (like www.google.com) or an IP address to look up.");
    inputEdit->setStyleSheet(
        "QLineEdit { border: 2px solid #3498db; border-radius: 4px; padding: 8px; font-size: 10pt; } "
        "QLineEdit:focus { border-color: #2980b9; }"
    );

    int buttonY = 48 + 32 + 38 + 20;
    if (buttonY < 130) buttonY = 130;
    if (buttonY > physicalHeight - 50) buttonY = physicalHeight - 50;
    int buttonWidth = 90;
    int buttonSpacing = 16;
    int totalButtonWidth = (buttonWidth * 2) + buttonSpacing;
    int startX = (physicalWidth - totalButtonWidth) / 2;

    QPushButton *okBtn = new QPushButton("Look it up", inputDlg);
    okBtn->setGeometry(startX, buttonY, buttonWidth, 35);
    okBtn->setToolTip("Start the DNS lookup for the entered hostname or IP.");
    okBtn->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; border: none; border-radius: 5px; font-weight: bold; font-size: 10pt; } "
        "QPushButton:hover { background-color: #2ecc71; } "
        "QPushButton:pressed { background-color: #229954; }"
    );

    QPushButton *cancelBtn = new QPushButton("Close", inputDlg);
    cancelBtn->setGeometry(startX + buttonWidth + buttonSpacing, buttonY, buttonWidth, 35);
    cancelBtn->setToolTip("Close this dialog.");
    cancelBtn->setStyleSheet(
        "QPushButton { background-color: #23272b; color: white; border: none; border-radius: 5px; font-weight: bold; font-size: 10pt; } "
        "QPushButton:hover { background-color: #181a1b; } "
        "QPushButton:pressed { background-color: #101112; }"
    );

    okBtn->setEnabled(false);
    QObject::connect(inputEdit, &QLineEdit::textChanged, [okBtn, inputEdit]() {
        okBtn->setEnabled(!inputEdit->text().trimmed().isEmpty());
    });
    QObject::connect(okBtn, &QPushButton::clicked, inputDlg, &QDialog::accept);
    QObject::connect(cancelBtn, &QPushButton::clicked, inputDlg, &QDialog::reject);

    QShortcut *inputCloseShortcut = new QShortcut(QKeySequence("Ctrl+W"), inputDlg);
    inputCloseShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(inputCloseShortcut, &QShortcut::activated, inputDlg, &QDialog::reject);

    // Only one modal dialog at a time, so use a loop with exec() and always create result dialog on heap
    while (true) {
        int result = inputDlg->exec();
        if (result != QDialog::Accepted)
            break;

        QString host = inputEdit->text().trimmed();
        if (host.isEmpty())
            continue;

        QProcess proc;
        proc.start("nslookup", QStringList() << host);
        proc.waitForFinished();
        QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());

        QString server, serverAddr, name;
        QStringList ipv4List, ipv6List;
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);

        for (int i = 0; i < lines.size(); ++i) {
            QString line = lines[i].trimmed();
            if (line.startsWith("Server:", Qt::CaseInsensitive)) {
                server = line.section(':', 1).trimmed();
                continue;
            } else if (line.startsWith("Address:", Qt::CaseInsensitive) && serverAddr.isEmpty()) {
                serverAddr = line.section(':', 1).trimmed();
                continue;
            }
            if (line.startsWith("Name:", Qt::CaseInsensitive)) {
                name = line.section(':', 1).trimmed();
                continue;
            }
            if (line.startsWith("Addresses:", Qt::CaseInsensitive)) {
                QString addr = line.section(':', 1).trimmed();
                if (!addr.isEmpty()) {
                    if (addr.contains(':')) ipv6List << addr;
                    else if (addr.contains('.')) ipv4List << addr;
                }
                for (int j = i+1; j < lines.size(); ++j) {
                    QString l2 = lines[j].trimmed();
                    if (l2.isEmpty() || l2.startsWith("Name:") || l2.startsWith("Server:") || l2.startsWith("Address:")) break;
                    if (l2.contains(':')) ipv6List << l2;
                    else if (l2.contains('.')) ipv4List << l2;
                }
            } else if (line.startsWith("Address:", Qt::CaseInsensitive)) {
                QString addr = line.section(':', 1).trimmed();
                if (!addr.isEmpty()) {
                    if (addr.contains(':')) ipv6List << addr;
                    else if (addr.contains('.')) ipv4List << addr;
                }
            }
        }

        QString info = "<pre style='font-family:monospace'>";
        if (!server.isEmpty())
            info += QString("<b>Server:</b>    \t%1\n").arg(server);
        if (!serverAddr.isEmpty())
            info += QString("<b>Address:</b>   \t%1\n").arg(serverAddr);
        if (!name.isEmpty())
            info += QString("<b>Name:</b>      \t%1\n").arg(name);

        if (!ipv4List.isEmpty() || !ipv6List.isEmpty()) {
            info += "<b>Addresses:</b>  \t";
            bool first = true;
            for (const QString &addr : ipv4List) {
                if (!first) info += "             \t";
                info += addr + "\n";
                first = false;
            }
            for (const QString &addr : ipv6List) {
                if (first) { info += addr + "\n"; first = false; }
                else info += "             \t" + addr + "\n";
            }
        }
        info += "</pre>";

        // Always create result dialog on heap, parented to nullptr (not inputDlg)
        QDialog *resultDlg = new QDialog(nullptr);
        resultDlg->setAttribute(Qt::WA_DeleteOnClose);
        resultDlg->setWindowTitle("NS Lookup Result");
        resultDlg->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
        resultDlg->setModal(true);

        resultDlg->setStyleSheet(
            "QDialog { background-color: #f8f9fa; border: 2px solid #34495e; border-radius: 8px; }"
        );

        QLabel *resultLabel = new QLabel(resultDlg);
        resultLabel->setText(info);
        resultLabel->setTextFormat(Qt::RichText);
        resultLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        resultLabel->setStyleSheet("QLabel { color: #34495e; font-size: 10pt; font-weight: normal; background: transparent; }");
        resultLabel->adjustSize();

        int dlgW = qMax(420, resultLabel->width() + 36);
        int dlgH = qMax(180, resultLabel->height() + 80);
        resultDlg->setFixedSize(dlgW, dlgH);

        resultLabel->setGeometry(18, 18, dlgW-36, dlgH-80);

        int resultButtonWidth = 90;
        int resultButtonY = dlgH - 48;
        int resultButtonX = (dlgW - resultButtonWidth) / 2;
        QPushButton *resultCloseBtn = new QPushButton("Close", resultDlg);
        resultCloseBtn->setGeometry(resultButtonX, resultButtonY, resultButtonWidth, 32);
        resultCloseBtn->setToolTip("Close this dialog");
        resultCloseBtn->setStyleSheet(
            "QPushButton { background-color: #23272b; color: white; border: none; border-radius: 5px; font-weight: bold; font-size: 10pt; } "
            "QPushButton:hover { background-color: #181a1b; } "
            "QPushButton:pressed { background-color: #101112; }"
        );
        QObject::connect(resultCloseBtn, &QPushButton::clicked, resultDlg, &QDialog::accept);

        QShortcut *resultCloseShortcut = new QShortcut(QKeySequence("Ctrl+W"), resultDlg);
        resultCloseShortcut->setContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(resultCloseShortcut, &QShortcut::activated, resultDlg, &QDialog::accept);

        resultDlg->exec();
        // resultDlg is deleted automatically due to WA_DeleteOnClose
    }
    // Do not call inputDlg->deleteLater(); let Qt clean up after close
}

void showArpDialog(QWidget *parent) {
    // Always allocate dialog on the heap and set WA_DeleteOnClose for safety
    QDialog *dlg = new QDialog(parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle("ARP Table");

    // Modern styling
    dlg->setStyleSheet(
        "QDialog { background-color: #f8f9fa; border: 2px solid #34495e; border-radius: 8px; }"
    );

    QVBoxLayout *layout = new QVBoxLayout(dlg);

    // Remove arp entries section
    QHBoxLayout *delLayout = new QHBoxLayout();
    QLabel *removeLabel = new QLabel("Remove arp entries:");
    removeLabel->setStyleSheet("QLabel { color: #34495e; font-weight: bold; font-size: 10pt; }");

    QLineEdit *ipEdit = new QLineEdit("*");
    ipEdit->setPlaceholderText("IP to delete (e.g. 192.168.1.1 or * for all)");
    ipEdit->setToolTip("Insert IP from the ARP table<br>or leave as it is to delete all ARP entries");
    ipEdit->setStyleSheet(
        "QLineEdit { border: 2px solid #3498db; border-radius: 4px; padding: 6px; font-size: 10pt; } "
        "QLineEdit:focus { border-color: #2980b9; }"
    );

    QPushButton *delBtn = new QPushButton("Delete");
    delBtn->setStyleSheet(
        "QPushButton { background-color: #e74c3c; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 8px 16px; font-size: 10pt; } "
        "QPushButton:hover { background-color: #c0392b; } "
        "QPushButton:pressed { background-color: #a93226; }"
    );

    delLayout->addWidget(removeLabel);
    delLayout->addWidget(ipEdit);
    delLayout->addWidget(delBtn);
    layout->addLayout(delLayout);

    // Table
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

    table->setStyleSheet(
        "QTableWidget { background-color: #ecf0f1; border: 2px solid #34495e; border-radius: 5px; gridline-color: #bdc3c7; font-size: 10pt; } "
        "QTableWidget::item { padding: 6px; border-bottom: 1px solid #d5dbdb; } "
        "QTableWidget::item:selected { background-color: #3498db; color: white; } "
        "QHeaderView::section { background-color: #34495e; color: white; padding: 8px; border: none; font-weight: bold; font-size: 10pt; }"
    );

    layout->addWidget(table);

    // Advanced/Refresh/Close buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton *advBtn = new QPushButton("Advanced");
    advBtn->setStyleSheet(
        "QPushButton { background-color: #9b59b6; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 8px 16px; font-size: 10pt; } "
        "QPushButton:hover { background-color: #8e44ad; } "
        "QPushButton:pressed { background-color: #7d3c98; }"
    );
    QPushButton *refreshBtn = new QPushButton("Refresh");
    refreshBtn->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 8px 16px; font-size: 10pt; } "
        "QPushButton:hover { background-color: #2980b9; } "
        "QPushButton:pressed { background-color: #1f618d; }"
    );
    QPushButton *closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background-color: #23272b; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 8px 16px; font-size: 10pt; } "
        "QPushButton:hover { background-color: #181a1b; } "
        "QPushButton:pressed { background-color: #101112; }"
    );
    btnLayout->addWidget(advBtn);
    btnLayout->addWidget(refreshBtn);
    btnLayout->addWidget(closeBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    advBtn->setToolTip("Toggle between advanced and basic ARP table views");
    refreshBtn->setToolTip("Refresh the ARP table");
    closeBtn->setToolTip("Close the ARP table");

    // Helper: adjust table width to fit content
    auto adjustWidths = [table, dlg, layout]() {
        table->resizeColumnsToContents();
        QCoreApplication::processEvents();
        int totalWidth = table->verticalHeader()->width();
        for (int i = 0; i < table->columnCount(); ++i)
            totalWidth += table->columnWidth(i);
        totalWidth += table->frameWidth() * 2;
        if (table->verticalScrollBar()->isVisible())
            totalWidth += table->verticalScrollBar()->width();
        totalWidth += 18;
        table->setMinimumWidth(totalWidth);
        table->setMaximumWidth(totalWidth);
        dlg->setFixedWidth(totalWidth + layout->contentsMargins().left() + layout->contentsMargins().right());
    };

    // Helper: fill table from arp output
    auto fillTable = [table, adjustWidths](const QString &output) {
        table->setRowCount(0);
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            QString trimmed = line.trimmed();
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

    // Initial ARP table load
    bool advanced = false;
    QProcess proc;
    proc.start("arp", QStringList() << "-a");
    proc.waitForFinished();
    QString arpOutput = QString::fromLocal8Bit(proc.readAllStandardOutput());
    fillTable(arpOutput);

    // Delete button logic (with UAC elevation)
    QObject::connect(delBtn, &QPushButton::clicked, [ipEdit, dlg, table, fillTable, &advanced]() {
        QString ip = ipEdit->text().trimmed();
        if (ip.isEmpty())
            return;
        QString command = QString("Start-Process arp -ArgumentList '-d %1' -Verb runAs -WindowStyle Hidden").arg(ip);
        int result = QProcess::execute("powershell", QStringList() << "-WindowStyle" << "Hidden" << "-Command" << command);
        if (result == 0) {
            QMessageBox::information(dlg, "ARP", "ARP entry deleted (or all entries deleted).");
        } else {
            QMessageBox::warning(dlg, "ARP", "Failed to delete ARP entry. (You may need to accept the UAC prompt.)");
        }
        QProcess proc;
        proc.start("arp", QStringList() << (advanced ? "-av" : "-a"));
        proc.waitForFinished();
        fillTable(QString::fromLocal8Bit(proc.readAllStandardOutput()));
    });

    // Advanced/Basic toggle logic
    QObject::connect(advBtn, &QPushButton::clicked, [advBtn, &advanced, fillTable, table]() {
        advanced = !advanced;
        advBtn->setText(advanced ? "Basic" : "Advanced");
        QProcess proc;
        proc.start("arp", QStringList() << (advanced ? "-av" : "-a"));
        proc.waitForFinished();
        fillTable(QString::fromLocal8Bit(proc.readAllStandardOutput()));
    });

    // Refresh button logic
    QObject::connect(refreshBtn, &QPushButton::clicked, [fillTable, &advanced]() {
        QProcess proc;
        proc.start("arp", QStringList() << (advanced ? "-av" : "-a"));
        proc.waitForFinished();
        fillTable(QString::fromLocal8Bit(proc.readAllStandardOutput()));
    });

    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    // Ctrl+W shortcut for close
    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, dlg, &QDialog::accept);

    dlg->exec();
}

void showWifiScanDialog(QWidget *parent) {
    QDialog dlg;
    dlg.setWindowTitle("WiFi Networks");
    dlg.setStyleSheet(
        "QDialog { "
        "    background-color: #f8f9fa; "
        "    border: 2px solid #34495e; "
        "    border-radius: 8px; "
        "}"
    );
    addCtrlWClose(&dlg);
    dlg.setFixedWidth(650);

    QVBoxLayout *layout = new QVBoxLayout(&dlg);

    QLabel *msgLabel = new QLabel;
    msgLabel->setWordWrap(true);
    msgLabel->setAlignment(Qt::AlignCenter);
    QFont msgFont = msgLabel->font();
    msgFont.setBold(true);
    msgFont.setPointSize(11);
    msgLabel->setFont(msgFont);
    msgLabel->setStyleSheet(
        "QLabel { color: #e67e22; background-color: #fdf2e9; border: 2px solid #f39c12; border-radius: 6px; padding: 8px; }"
    );
    msgLabel->setVisible(false);
    layout->addWidget(msgLabel);

    QLabel *info = new QLabel("Nearby WiFi networks (signal in dBm):");
    info->setStyleSheet("QLabel { color: #34495e; font-weight: bold; font-size: 11pt; }");
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

    table->setStyleSheet(
        "QTableWidget { background-color: #ecf0f1; border: 2px solid #34495e; border-radius: 5px; gridline-color: #bdc3c7; font-size: 10pt; } "
        "QTableWidget::item { padding: 6px; border-bottom: 1px solid #d5dbdb; } "
        "QTableWidget::item:selected { background-color: #3498db; color: white; } "
        "QHeaderView::section { background-color: #34495e; color: white; padding: 8px; border: none; font-weight: bold; font-size: 10pt; }"
    );
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    table->setColumnWidth(0, 160); // SSID
    table->setColumnWidth(1, 180); // BSSID
    table->setColumnWidth(2, 110); // Signal
    table->setColumnWidth(3, 140); // Channel

    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    layout->addWidget(table);

    QPushButton *refreshBtn = new QPushButton("Refresh");
    refreshBtn->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 8px 16px; font-size: 10pt; } "
        "QPushButton:hover { background-color: #2980b9; } "
        "QPushButton:pressed { background-color: #1f618d; }"
    );
    QPushButton *closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background-color: #95a5a6; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 8px 16px; font-size: 10pt; } "
        "QPushButton:hover { background-color: #7f8c8d; } "
        "QPushButton:pressed { background-color: #6c7b7d; }"
    );

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    btnLayout->addWidget(refreshBtn);
    btnLayout->addWidget(closeBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    int sortColumn = 2; // Default: Signal
    Qt::SortOrder sortOrder = Qt::DescendingOrder;

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

    struct WifiEntry {
        QString ssid, bssid, signal, channel;
        int dbm;
    };
    QList<WifiEntry> entries;

    auto fillTable = [&]() {
        table->setRowCount(0);
        QList<WifiEntry> sorted = entries;
        std::function<bool(const WifiEntry&, const WifiEntry&)> cmp;
        switch (sortColumn) {
            case 0:
                cmp = [&](const WifiEntry &a, const WifiEntry &b) { return sortOrder == Qt::AscendingOrder ? a.ssid < b.ssid : a.ssid > b.ssid; }; break;
            case 1:
                cmp = [&](const WifiEntry &a, const WifiEntry &b) { return sortOrder == Qt::AscendingOrder ? a.bssid < b.bssid : a.bssid > b.bssid; }; break;
            case 2:
                cmp = [&](const WifiEntry &a, const WifiEntry &b) { return sortOrder == Qt::AscendingOrder ? a.dbm < b.dbm : a.dbm > b.dbm; }; break;
            case 3:
                cmp = [&](const WifiEntry &a, const WifiEntry &b) { return sortOrder == Qt::AscendingOrder ? a.channel < b.channel : a.channel > b.channel; }; break;
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

        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        HANDLE hClient = NULL;
        DWORD dwMaxClient = 2;
        DWORD dwCurVersion = 0;
        DWORD dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
        if (dwResult != ERROR_SUCCESS) {
            showTableOrMsg(false, "WLAN API not available.");
            if (SUCCEEDED(hr)) CoUninitialize();
            return;
        }

        PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
        dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
        if (dwResult != ERROR_SUCCESS || !pIfList || pIfList->dwNumberOfItems == 0) {
            showTableOrMsg(false, "No WiFi interface found.");
            if (pIfList) WlanFreeMemory(pIfList);
            WlanCloseHandle(hClient, NULL);
            if (SUCCEEDED(hr)) CoUninitialize();
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
                    .arg(QString("%1").arg(bss.dot11Bssid[0], 2, 16, QChar('0')))
                    .arg(QString("%1").arg(bss.dot11Bssid[1], 2, 16, QChar('0')))
                    .arg(QString("%1").arg(bss.dot11Bssid[2], 2, 16, QChar('0')))
                    .arg(QString("%1").arg(bss.dot11Bssid[3], 2, 16, QChar('0')))
                    .arg(QString("%1").arg(bss.dot11Bssid[4], 2, 16, QChar('0')))
                    .arg(QString("%1").arg(bss.dot11Bssid[5], 2, 16, QChar('0')))
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
        if (SUCCEEDED(hr)) CoUninitialize();

        if (!entries.isEmpty()) {
            showTableOrMsg(true);
            fillTable();
        } else {
            showTableOrMsg(false, "No WiFi networks found.");
        }
    };

    QObject::connect(table->horizontalHeader(), &QHeaderView::sectionClicked, [&](int col) {
        if (sortColumn == col) {
            sortOrder = (sortOrder == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
        } else {
            sortColumn = col;
            sortOrder = (col == 2) ? Qt::DescendingOrder : Qt::AscendingOrder;
        }
        fillTable();
    });

    QObject::connect(refreshBtn, &QPushButton::clicked, scanWifi);
    QObject::connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), &dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, &dlg, &QDialog::accept);

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
    QDialog *dlg = new QDialog(parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle("Network usage");
    dlg->setStyleSheet(
        "QDialog { background-color: #f8f9fa; border: 2px solid #34495e; border-radius: 8px; }"
    );
    dlg->setMinimumWidth(420);

    QVBoxLayout *layout = new QVBoxLayout(dlg);
    layout->setContentsMargins(8, 8, 8, 8);

    // Title
    QLabel *usageLabel = new QLabel("Network usage");
    usageLabel->setAlignment(Qt::AlignCenter);
    usageLabel->setStyleSheet(
        "QLabel { color: #34495e; font-weight: bold; font-size: 13pt; padding-bottom: 4px; margin-bottom: 0px; }"
    );
    layout->addWidget(usageLabel);

    // Table: 2 rows, 3 columns (Received, Sent, Uptime)
    QTableWidget *table = new QTableWidget(2, 3, dlg);
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
    table->setFixedHeight(104);
    table->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    table->setStyleSheet(
        "QTableWidget { background-color: #ecf0f1; border: 2px solid #34495e; border-radius: 5px; gridline-color: #bdc3c7; font-size: 9pt; } "
        "QTableWidget::item { padding: 4px; border-bottom: 1px solid #d5dbdb; } "
        "QHeaderView::section { background-color: #34495e; color: white; padding: 6px; border: none; font-weight: bold; font-size: 9pt; }"
    );
    layout->addWidget(table);

    layout->setSpacing(8);
    layout->setContentsMargins(10, 10, 10, 10);

    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *resetTripBtn = new QPushButton("Reset Trip Counter", dlg);
    resetTripBtn->setToolTip("Reset the trip counter to zero.\nUse this to measure network usage for a specific task.");
    resetTripBtn->setStyleSheet(
        "QPushButton { background-color: #e67e22; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 6px 12px; font-size: 9pt; } "
        "QPushButton:hover { background-color: #d35400; } "
        "QPushButton:pressed { background-color: #ba4a00; }"
    );
    QPushButton *closeBtn = new QPushButton("Close", dlg);
    closeBtn->setToolTip("Close this dialog");
    closeBtn->setStyleSheet(
        "QPushButton { background-color: #95a5a6; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 6px 12px; font-size: 9pt; } "
        "QPushButton:hover { background-color: #7f8c8d; } "
        "QPushButton:pressed { background-color: #6c7b7d; }"
    );
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

    QTimer *timer = new QTimer(dlg);
    QObject::connect(timer, &QTimer::timeout, [=]() {
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
    timer->start(1000);
    updateStats();

    QObject::connect(resetTripBtn, &QPushButton::clicked, [=]() {
        while (true) {
            bool ok = false;
            QString timeStr = QInputDialog::getText(
                dlg,
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
                    QMessageBox::warning(dlg, "Format Error",
                        "Please enter the time as hh:mm:ss (e.g. 1:00:00 for 1 hour, 0:30:00 for 30 minutes, 0:00:10 for 10 seconds, or 0 for unlimited).");
                    continue;
                }
                int h = m.captured(1).toInt();
                int m_ = m.captured(2).toInt();
                int s = m.captured(3).toInt();
                if (m_ > 59 || s > 59) {
                    QMessageBox::warning(dlg, "Format Error",
                        "Minutes and seconds must be between 0 and 59.");
                    continue;
                }
                newTripDurationSecs = h * 3600 + m_ * 60 + s;
                if (newTripDurationSecs == 0) newTripDurationSecs = 0;
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
                QObject::connect(&tripLimitTimer, &QTimer::timeout, [=]() mutable {
                    tripActive = false;
                    updateStats();
                });
                tripLimitTimer.start(tripDurationSecs * 1000);
            }
            break;
        }
    });

    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    // Ctrl+W shortcut
    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, dlg, &QDialog::accept);

    dlg->exec();
    timer->stop();
    if (tripLimitTimer.isActive()) tripLimitTimer.stop();
    dlg->deleteLater();
}

void showNetworkAdaptersDialog(QWidget *parent) {
    QDialog *dlg = new QDialog(parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle("Network Adapters");
    dlg->setStyleSheet(
        "QDialog { background-color: #f8f9fa; border: 2px solid #34495e; border-radius: 8px; }"
    );

    QVBoxLayout *layout = new QVBoxLayout(dlg);

    QTableWidget *table = new QTableWidget(dlg);
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

    table->setStyleSheet(
        "QTableWidget { background-color: #ecf0f1; border: 2px solid #34495e; border-radius: 5px; gridline-color: #bdc3c7; font-size: 10pt; } "
        "QTableWidget::item { padding: 6px; border-bottom: 1px solid #d5dbdb; } "
        "QTableWidget::item:selected { background-color: #3498db; color: white; } "
        "QHeaderView::section { background-color: #34495e; color: white; padding: 8px; border: none; font-weight: bold; font-size: 10pt; }"
    );

    layout->addWidget(table);

    QFont headerFont = table->horizontalHeader()->font();
    headerFont.setBold(true);
    table->horizontalHeader()->setFont(headerFont);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *rescanBtn = new QPushButton("Rescan", dlg);
    rescanBtn->setToolTip("Manually rescan the network adapters");
    rescanBtn->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 8px 16px; font-size: 10pt; } "
        "QPushButton:hover { background-color: #2980b9; } "
        "QPushButton:pressed { background-color: #1f618d; }"
    );

    QPushButton *closeBtn = new QPushButton("Close", dlg);
    closeBtn->setToolTip("Close the dialog");
    closeBtn->setStyleSheet(
        "QPushButton { background-color: #95a5a6; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 8px 16px; font-size: 10pt; } "
        "QPushButton:hover { background-color: #7f8c8d; } "
        "QPushButton:pressed { background-color: #6c7b7d; }"
    );

    btnLayout->addStretch();
    btnLayout->addWidget(rescanBtn);
    btnLayout->addWidget(closeBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    // Type detection
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
            descItem->setToolTip(desc);
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
        totalWidth += 40;

        int screenWidth = QApplication::primaryScreen()->availableGeometry().width();
        int maxWidth = qMin(1920, screenWidth - 80);
        totalWidth = qMin(totalWidth, maxWidth);

        dlg->resize(totalWidth, dlg->sizeHint().height());
    };

    fillTable();

    QObject::connect(rescanBtn, &QPushButton::clicked, [=]() {
        fillTable();

        // Show popup with 3s timeout and OK button
        QDialog popup(dlg);
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

    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    // Ctrl+W shortcut
    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, dlg, &QDialog::accept);

    fillTable();
    dlg->exec();
}

// Helper function to read PMTUD (Path MTU Discovery) status from Windows Registry
static QString getPMTUDStatus() {
    HKEY hKey;
    LONG result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters",
                                0,
                                KEY_READ,
                                &hKey);
    if (result != ERROR_SUCCESS) {
        return "unknown";
    }

    DWORD value = 0;
    DWORD valueSize = sizeof(DWORD);
    result = RegQueryValueExW(hKey, L"EnablePMTUDiscovery", NULL, NULL, (LPBYTE)&value, &valueSize);
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS) {
        // If the registry key doesn't exist, PMTUD is enabled by default in Windows
        return "enabled";
    }

    switch (value) {
        case 0:
            return "disabled";
        case 1:
        case 2:
            return "enabled";
        default:
            return "unknown";
    }
}

// Helper function to set PMTUD status using reg.exe with UAC elevation
static bool setPMTUDStatusWithUAC(bool enable) {
    // Build the reg.exe command
    QString command = QString("reg add \"HKLM\\SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\" /v EnablePMTUDiscovery /t REG_DWORD /d %1 /f")
                        .arg(enable ? "1" : "0");
    
    // Convert to wide string
    std::wstring wCommand = command.toStdWString();
    
    // Use ShellExecute with "runas" to trigger UAC elevation
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"runas";  // This triggers UAC
    sei.lpFile = L"cmd.exe";
    sei.lpParameters = (L"/C " + wCommand).c_str();
    sei.nShow = SW_HIDE;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    
    if (!ShellExecuteExW(&sei)) {
        DWORD error = GetLastError();
        if (error == ERROR_CANCELLED) {
            // User cancelled UAC prompt
            return false;
        }
        return false;
    }
    
    // Wait for the command to complete
    if (sei.hProcess) {
        WaitForSingleObject(sei.hProcess, INFINITE);
        DWORD exitCode = 0;
        GetExitCodeProcess(sei.hProcess, &exitCode);
        CloseHandle(sei.hProcess);
        return (exitCode == 0);
    }
    
    return false;
}

void showMtuDiscoveryDialog(QWidget *parent) {
    QDialog *dlg = new QDialog(parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle("MTU Discovery");
    dlg->setStyleSheet(
        "QDialog { background-color: #f8f9fa; border: 2px solid #34495e; border-radius: 8px; }"
    );

    QVBoxLayout *layout = new QVBoxLayout(dlg);

    QLabel *prompt = new QLabel("Find the optimal MTU for your connection (largest packet size without fragmentation):");
    prompt->setStyleSheet("QLabel { color: #34495e; font-weight: bold; font-size: 11pt; }");
    layout->addWidget(prompt);

    // Host input
    QHBoxLayout *hostLayout = new QHBoxLayout();
    QLabel *hostLabel = new QLabel("Host:");
    hostLabel->setStyleSheet("QLabel { color: #34495e; font-weight: bold; font-size: 10pt; }");
    QLineEdit *hostEdit = new QLineEdit();
    hostEdit->setPlaceholderText("e.g. 8.8.8.8 or vg.no");
    hostEdit->setToolTip("Enter the host to ping.\nExamples: 8.8.8.8 (Google DNS), vg.no (Norwegian news), or your router IP.");
    hostEdit->setStyleSheet(
        "QLineEdit { border: 2px solid #3498db; border-radius: 4px; padding: 6px; font-size: 10pt; } "
        "QLineEdit:focus { border-color: #2980b9; }"
    );
    hostLayout->addWidget(hostLabel);
    hostLayout->addWidget(hostEdit);
    layout->addLayout(hostLayout);

    // Range and step input
    QHBoxLayout *rangeLayout = new QHBoxLayout();
    QLabel *minLabel = new QLabel("Min size:");
    minLabel->setStyleSheet("QLabel { color: #34495e; font-weight: bold; font-size: 10pt; }");
    QSpinBox *minSpin = new QSpinBox;
    minSpin->setRange(12, 2000);
    minSpin->setValue(1200);
    minSpin->setStyleSheet(
        "QSpinBox { border: 2px solid #3498db; border-radius: 4px; padding: 4px; font-size: 10pt; } "
        "QSpinBox:focus { border-color: #2980b9; }"
    );
    QLabel *maxLabel = new QLabel("Max size:");
    maxLabel->setStyleSheet("QLabel { color: #34495e; font-weight: bold; font-size: 10pt; }");
    QSpinBox *maxSpin = new QSpinBox;
    maxSpin->setRange(12, 2000);
    maxSpin->setValue(1500);
    maxSpin->setStyleSheet(
        "QSpinBox { border: 2px solid #3498db; border-radius: 4px; padding: 4px; font-size: 10pt; } "
        "QSpinBox:focus { border-color: #2980b9; }"
    );
    QLabel *stepLabel = new QLabel("Step:");
    stepLabel->setStyleSheet("QLabel { color: #34495e; font-weight: bold; font-size: 10pt; }");
    QSpinBox *stepSpin = new QSpinBox;
    stepSpin->setRange(1, 200);
    stepSpin->setValue(10);
    stepSpin->setStyleSheet(
        "QSpinBox { border: 2px solid #3498db; border-radius: 4px; padding: 4px; font-size: 10pt; } "
        "QSpinBox:focus { border-color: #2980b9; }"
    );
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
    output->setStyleSheet(
        "QTextEdit { background-color: #ecf0f1; border: 2px solid #34495e; border-radius: 5px; font-family: 'Consolas', 'Courier New', monospace; font-size: 9pt; padding: 6px; }"
    );
    layout->addWidget(output);

    // --- Interface and MTU setting controls ---
    QHBoxLayout *mtuLayout = new QHBoxLayout();
    QLabel *ifaceLabel = new QLabel("Interface:");
    ifaceLabel->setStyleSheet("QLabel { color: #34495e; font-weight: bold; font-size: 10pt; }");
    QComboBox *ifaceCombo = new QComboBox;
    ifaceCombo->setStyleSheet(
        "QComboBox { border: 2px solid #3498db; border-radius: 4px; padding: 4px; font-size: 10pt; } "
        "QComboBox:focus { border-color: #2980b9; }"
    );
    QLabel *mtuLabel = new QLabel("Set MTU:");
    mtuLabel->setStyleSheet("QLabel { color: #34495e; font-weight: bold; font-size: 10pt; }");
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

    // PMTUD Status Indicator
    QString pmtudStatus = getPMTUDStatus();
    QHBoxLayout *pmtudLayout = new QHBoxLayout();
    pmtudLayout->addStretch();
    
    // Question mark icon with blue background
    QLabel *questionMark = new QLabel("?");
    questionMark->setStyleSheet(
        "QLabel { "
        "background-color: #3498db; "
        "color: white; "
        "border-radius: 10px; "
        "font-weight: bold; "
        "font-size: 10pt; "
        "min-width: 20px; "
        "max-width: 20px; "
        "min-height: 20px; "
        "max-height: 20px; "
        "qproperty-alignment: AlignCenter; "
        "}"
    );
    questionMark->setToolTip(
        "<b>Path MTU Discovery (PMTUD)</b><br/><br/>"
        "Automatically determines the optimal packet size to avoid fragmentation "
        "across network paths. When enabled, your system can dynamically adjust "
        "packet sizes for better performance and reliability."
    );
    
    // Status text
    QLabel *pmtudLabel = new QLabel();
    if (pmtudStatus == "enabled") {
        pmtudLabel->setText("PMTUD: ✓ Enabled");
        pmtudLabel->setStyleSheet("QLabel { color: #27ae60; font-weight: bold; font-size: 10pt; }");
    } else if (pmtudStatus == "disabled") {
        pmtudLabel->setText("PMTUD: ✗ Disabled");
        pmtudLabel->setStyleSheet("QLabel { color: #e74c3c; font-weight: bold; font-size: 10pt; }");
    } else {
        pmtudLabel->setText("PMTUD: ? Unknown");
        pmtudLabel->setStyleSheet("QLabel { color: #f39c12; font-weight: bold; font-size: 10pt; }");
    }
    pmtudLabel->setToolTip(questionMark->toolTip());
    
    // Toggle button
    QPushButton *togglePmtudBtn = new QPushButton(pmtudStatus == "enabled" ? "Disable" : "Enable");
    togglePmtudBtn->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; border: none; border-radius: 4px; font-weight: bold; min-width: 70px; min-height: 24px; font-size: 9pt; padding: 0 12px; } "
        "QPushButton:hover { background-color: #2980b9; } "
        "QPushButton:pressed { background-color: #21618c; }"
    );
    togglePmtudBtn->setToolTip("Toggle PMTUD on/off (requires administrator privileges)");
    if (pmtudStatus == "unknown") {
        togglePmtudBtn->setEnabled(false);
    }
    
    pmtudLayout->addWidget(questionMark);
    pmtudLayout->addSpacing(8);
    pmtudLayout->addWidget(pmtudLabel);
    pmtudLayout->addSpacing(8);
    pmtudLayout->addWidget(togglePmtudBtn);
    pmtudLayout->addStretch();
    
    // Connect toggle button
    QObject::connect(togglePmtudBtn, &QPushButton::clicked, [=]() {
        QString currentStatus = getPMTUDStatus();
        bool shouldEnable = (currentStatus == "disabled");
        
        // Disable button during operation
        togglePmtudBtn->setEnabled(false);
        togglePmtudBtn->setText("Please wait...");
        QApplication::processEvents();
        
        if (setPMTUDStatusWithUAC(shouldEnable)) {
            // Update UI
            QString newStatus = getPMTUDStatus();
            if (newStatus == "enabled") {
                pmtudLabel->setText("PMTUD: ✓ Enabled");
                pmtudLabel->setStyleSheet("QLabel { color: #27ae60; font-weight: bold; font-size: 10pt; }");
                togglePmtudBtn->setText("Disable");
            } else if (newStatus == "disabled") {
                pmtudLabel->setText("PMTUD: ✗ Disabled");
                pmtudLabel->setStyleSheet("QLabel { color: #e74c3c; font-weight: bold; font-size: 10pt; }");
                togglePmtudBtn->setText("Enable");
            }
            togglePmtudBtn->setEnabled(true);
            QMessageBox::information(dlg, "PMTUD Status", 
                QString("PMTUD has been %1. Changes take effect immediately for new connections.").arg(shouldEnable ? "enabled" : "disabled"));
        } else {
            // Re-enable button and restore text
            togglePmtudBtn->setText(currentStatus == "enabled" ? "Disable" : "Enable");
            togglePmtudBtn->setEnabled(true);
            QMessageBox::warning(dlg, "Operation Cancelled", 
                "PMTUD status was not changed. You may have cancelled the UAC prompt or the operation failed.");
        }
    });
    layout->addLayout(pmtudLayout);
    layout->addSpacing(10);

    // Buttons (modern ARP/Traceroute style)
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton *startBtn = new QPushButton("Start Scan");
    startBtn->setToolTip("Begin scanning for the optimal MTU.");
    startBtn->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; border: none; border-radius: 5px; font-weight: bold; min-width: 90px; min-height: 32px; font-size: 10.5pt; padding: 0 16px; } "
        "QPushButton:hover { background-color: #2ecc71; } "
        "QPushButton:pressed { background-color: #229954; }"
    );
    btnLayout->addWidget(startBtn);
    btnLayout->addSpacing(10);
    QPushButton *stopBtn = new QPushButton("Stop");
    stopBtn->setToolTip("Stop the scan in progress.");
    stopBtn->setStyleSheet(
        "QPushButton { background-color: #e74c3c; color: white; border: none; border-radius: 5px; font-weight: bold; min-width: 90px; min-height: 32px; font-size: 10.5pt; padding: 0 16px; } "
        "QPushButton:hover { background-color: #c0392b; } "
        "QPushButton:pressed { background-color: #a93226; }"
    );
    btnLayout->addWidget(stopBtn);
    btnLayout->addSpacing(10);
    QPushButton *closeBtn = new QPushButton("Close");
    closeBtn->setToolTip("Close this dialog.");
    closeBtn->setStyleSheet(
        "QPushButton { background-color: #34495e; color: white; border: none; border-radius: 5px; font-weight: bold; min-width: 90px; min-height: 32px; font-size: 10.5pt; padding: 0 16px; } "
        "QPushButton:hover { background-color: #2c3e50; } "
        "QPushButton:pressed { background-color: #1a252f; }"
    );
    btnLayout->addWidget(closeBtn);
    btnLayout->addStretch();
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

    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
    QObject::connect(stopBtn, &QPushButton::clicked, [=]() {
        *running = false;
        setUiEnabled(true);
        if (*procPtr) {
            (*procPtr)->kill();
            (*procPtr)->deleteLater();
            *procPtr = nullptr;
        }
        timer->stop();
    });

    auto startScan = [=]() mutable {
        QObject::disconnect(timer.get(), &QTimer::timeout, nullptr, nullptr);

        QString host = hostEdit->text().trimmed();
        int minSize = minSpin->value();
        int maxSize = maxSpin->value();
        *step = stepSpin->value();
        if (host.isEmpty() || minSize > maxSize || *step <= 0) {
            QMessageBox::warning(dlg, "Input Error", "Please enter a valid host, size range, and step.");
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
    };
    QObject::connect(startBtn, &QPushButton::clicked, startScan);
    QObject::connect(hostEdit, &QLineEdit::returnPressed, startScan);

    // --- Set MTU Button ---
    QObject::connect(setMtuBtn, &QPushButton::clicked, [=]() {
        if (*best <= 0) {
            QMessageBox::warning(dlg, "Set MTU", "No MTU value to set. Run the test first.");
            return;
        }
        if (ifaceCombo->currentText().isEmpty()) {
            QMessageBox::warning(dlg, "Set MTU", "No network interface selected.");
            return;
        }
        int mtu = *best;
        int userMtu = mtuSpin->value();
        if (userMtu > mtu) {
            int cont = QMessageBox::warning(
                dlg, "MTU Warning",
                QString("You entered an MTU (%1) above the discovered maximum (%2).<br>"
                        "This may cause fragmentation or connectivity issues.<br><br>"
                        "Are you sure you want to continue?")
                    .arg(userMtu).arg(mtu),
                QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel
            );
            if (cont != QMessageBox::Yes)
                return;
        }

        int ret = QMessageBox::question(dlg, "Set MTU",
            QString("Set MTU for interface <b>%1</b> to <b>%2</b>?<br><br>"
                    "This requires administrator rights.").arg(ifaceCombo->currentText()).arg(userMtu),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (ret != QMessageBox::Yes)
            return;

        QString psCmd = QString(
            "Start-Process netsh -ArgumentList 'interface ipv4 set subinterface \"%1\" mtu=%2 store=persistent' -Verb runAs -WindowStyle Hidden"
        ).arg(ifaceCombo->currentText()).arg(userMtu);

        int result = QProcess::execute("powershell", QStringList() << "-WindowStyle" << "Hidden" << "-Command" << psCmd);

        if (result == 0) {
            QMessageBox::information(dlg, "Set MTU", QString("MTU set to %1 for interface %2.<br><br>You may need to reconnect or restart your network adapter for the change to take effect.").arg(userMtu).arg(ifaceCombo->currentText()));
        } else {
            int openSettings = QMessageBox::question(dlg, "Set MTU",
                "Failed to set MTU automatically.<br><br>"
                "Would you like to open the Windows network settings to set it manually?",
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            if (openSettings == QMessageBox::Yes) {
                QProcess::startDetached("ms-settings:network");
            }
        }
    });

    // Ctrl+W shortcut
    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, dlg, &QDialog::accept);

    dlg->adjustSize();
    dlg->exec();
    if (*procPtr) {
        (*procPtr)->kill();
        (*procPtr)->deleteLater();
        *procPtr = nullptr;
    }
    timer->stop();
}


void showHostsFileEditor(QWidget *parent) {
    // --- Robust, DPI-aware, modern hosts file editor with backup/restore, UAC, and unsaved changes prompt ---
    const QString hostsPath = "C:/Windows/System32/drivers/etc/hosts";
    const QString backupDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/IPGui";
    const QString backupPath = backupDir + "/hosts.bak";

    QDir().mkpath(backupDir);

    QDialog *dlg = new QDialog(parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle("Hosts File Editor");
    dlg->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    // DPI-aware sizing
    QScreen *screen = QApplication::primaryScreen();
    qreal dpiRatio = screen ? screen->devicePixelRatio() : 1.0;
    int physicalWidth = static_cast<int>(1300 / dpiRatio);
    int physicalHeight = static_cast<int>(700 / dpiRatio);
    dlg->setFixedSize(physicalWidth, physicalHeight);

    dlg->setStyleSheet(
        "QDialog { background-color: #f8f9fa; border: 2px solid #34495e; border-radius: 8px; }"
    );

    QLabel *hint = new QLabel(
        "<b>Windows Hosts File Editor</b><br>"
        "<span style='color:#7f8c8d;'>Edit the system hosts file. Changes require administrator rights.</span>", dlg);
    hint->setTextFormat(Qt::RichText);
    hint->setGeometry(10, 10, physicalWidth-20, 40);
    hint->setStyleSheet("QLabel { color: #2c3e50; font-weight: bold; font-size: 10pt; background-color: #ecf0f1; border: 1px solid #bdc3c7; border-radius: 4px; padding: 8px; }");

    QPlainTextEdit *editor = new QPlainTextEdit(dlg);
    QFont mono("Consolas");
    mono.setStyleHint(QFont::Monospace);
    mono.setPointSize(10);
    editor->setFont(mono);
    int editorWidth = physicalWidth - 20;
    int editorHeight = physicalHeight - 140;
    editor->setGeometry(10, 60, editorWidth, editorHeight);
    editor->setStyleSheet(
        "QPlainTextEdit { background-color: #ffffff; border: 2px solid #34495e; border-radius: 5px; color: #2c3e50; selection-background-color: #3498db; selection-color: white; padding: 8px; }"
    );

    int buttonY = physicalHeight - 45;
    int buttonWidth = 120;
    int buttonSpacing = 15;
    int totalButtonWidth = (buttonWidth * 3) + (buttonSpacing * 2);
    int startX = (physicalWidth - totalButtonWidth) / 2;

    QPushButton *saveBtn = new QPushButton("Save", dlg);
    saveBtn->setToolTip("Save changes to hosts file (requires admin rights).");
    saveBtn->setGeometry(startX, buttonY, buttonWidth, 35);
    saveBtn->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; border: none; border-radius: 5px; font-weight: bold; font-size: 10pt; } "
        "QPushButton:hover { background-color: #2ecc71; } "
        "QPushButton:pressed { background-color: #229954; }"
    );

    QPushButton *restoreBtn = new QPushButton("Restore Backup", dlg);
    restoreBtn->setToolTip("Restore hosts file from backup.");
    restoreBtn->setGeometry(startX + buttonWidth + buttonSpacing, buttonY, buttonWidth, 35);
    restoreBtn->setStyleSheet(
        "QPushButton { background-color: #f39c12; color: white; border: none; border-radius: 5px; font-weight: bold; font-size: 10pt; } "
        "QPushButton:hover { background-color: #e67e22; } "
        "QPushButton:pressed { background-color: #d35400; }"
    );

    QPushButton *closeBtn = new QPushButton("Close", dlg);
    closeBtn->setToolTip("Close the hosts file editor.");
    closeBtn->setGeometry(startX + (buttonWidth + buttonSpacing) * 2, buttonY, buttonWidth, 35);
    closeBtn->setStyleSheet(
        "QPushButton { background-color: #34495e; color: white; border: none; border-radius: 5px; font-weight: bold; font-size: 10pt; } "
        "QPushButton:hover { background-color: #2c3e50; } "
        "QPushButton:pressed { background-color: #1a252f; }"
    );

    // Load hosts file
    QString originalText;
    auto loadHosts = [editor, &originalText, dlg, hostsPath]() {
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
    auto backupHosts = [dlg, hostsPath, backupPath]() -> bool {
        QFileInfo fi(hostsPath);
        if (!fi.exists() || !fi.isFile()) {
            QMessageBox::warning(dlg, "Backup Error", "Hosts file does not exist, cannot create backup.");
            return false;
        }
        QString tmpBackupPath = backupPath + ".tmp";
        QFile::remove(tmpBackupPath);
        QFile file(hostsPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::warning(dlg, "Backup Error", "Could not read hosts file to create backup.");
            return false;
        }
        QFile tmpBackup(tmpBackupPath);
        if (!tmpBackup.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            QMessageBox::warning(dlg, "Backup Error", "Could not write temporary backup file.");
            file.close();
            return false;
        }
        QTextStream in(&file), out(&tmpBackup);
        out << in.readAll();
        file.close();
        tmpBackup.close();
        QFile::remove(backupPath);
        if (!QFile::rename(tmpBackupPath, backupPath)) {
            QMessageBox::warning(dlg, "Backup Error", "Could not finalize backup file.");
            return false;
        }
        return true;
    };

    // Save hosts file (no backup here!)
    auto saveHosts = [hostsPath](const QString &text) -> bool {
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
    auto restoreBackup = [dlg, backupPath, hostsPath, loadHosts]() {
        QFileInfo fi(backupPath);
        if (!fi.exists() || !fi.isFile()) {
            QMessageBox::warning(dlg, "Restore Backup", "No backup file found at:\n" + backupPath);
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
                        QMessageBox::information(dlg, "Restore Backup", "Backup restored.");
                        return;
                    }
                }
            }
            QMessageBox::warning(dlg, "Restore Backup", "Backup copied, but could not verify hosts file.");
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
                    QMessageBox::information(dlg, "Restore Backup", "Backup restored (with administrator rights).");
                    return;
                }
            }
        }
        QMessageBox::critical(dlg, "Restore Backup", "Failed to restore backup, even with elevation.");
        loadHosts();
    };

    loadHosts();

    // Track unsaved changes
    bool isDirty = false;
    QObject::connect(editor, &QPlainTextEdit::textChanged, [editor, &originalText, dlg, &isDirty]() mutable {
        isDirty = (editor->toPlainText() != originalText);
        dlg->setWindowTitle(QString("Hosts File Editor%1").arg(isDirty ? " *" : ""));
    });

    // Save logic
    QObject::connect(saveBtn, &QPushButton::clicked, [editor, &originalText, dlg, validateHosts, backupHosts, saveHosts, hostsPath, loadHosts, &isDirty]() mutable {
        QString text = editor->toPlainText();
        QString err = validateHosts(text);
        if (!err.isEmpty()) {
            QMessageBox::warning(dlg, "Syntax Error", "Hosts file not saved:\n" + err);
            return;
        }
        if (!backupHosts()) {
            QMessageBox::critical(dlg, "Backup Failed", "Could not create backup. Save aborted.");
            return;
        }
        bool saved = saveHosts(text);
        bool elevated = false;
        if (!saved) {
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
                    QMessageBox::critical(dlg, "Save Failed", "Could not save hosts file, even with elevation.");
                    loadHosts();
                    return;
                }
            } else {
                QMessageBox::critical(dlg, "Save Failed", "Could not write temporary file for elevation.");
                loadHosts();
                return;
            }
        }
        QThread::msleep(200);
        QFile verifyFile(hostsPath);
        if (verifyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString diskText = QTextStream(&verifyFile).readAll();
            verifyFile.close();
            if (diskText == text) {
                QMessageBox::information(dlg, "Saved", elevated
                    ? "Hosts file saved with administrator rights."
                    : "Hosts file saved successfully.");
                originalText = text;
                isDirty = false;
                dlg->setWindowTitle("Hosts File Editor");
                editor->setPlainText(text);
            } else {
                QMessageBox::critical(dlg, "Save Failed", "The hosts file could not be updated. (Check permissions, UAC prompt, or antivirus lock.)");
                loadHosts();
            }
        } else {
            QMessageBox::critical(dlg, "Save Failed", "Could not read hosts file after saving.");
            loadHosts();
        }
    });

    // Restore backup logic
    QObject::connect(restoreBtn, &QPushButton::clicked, restoreBackup);

    // Close logic with unsaved changes prompt
    auto tryClose = [dlg, &isDirty]() {
        if (isDirty) {
            auto ret = QMessageBox::question(
                dlg,
                "Unsaved Changes",
                "You have unsaved changes. Do you want to close without saving?",
                QMessageBox::Yes | QMessageBox::Cancel,
                QMessageBox::Cancel
            );
            if (ret != QMessageBox::Yes)
                return;
        }
        dlg->accept();
    };
    QObject::connect(closeBtn, &QPushButton::clicked, tryClose);

    // Ctrl+W shortcut for close with prompt
    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, tryClose);

    dlg->exec();
    dlg->deleteLater();
}

void showNetstatStatisticsDialog(QWidget *parent) {
    // --- Robust, modern, DPI-aware netstat statistics dialog with tooltips and color coding ---
    // This version avoids any blocking or resource issues on close.

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

    const QMap<QString, QString> tooltips = {
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

    // --- Run netstat -s asynchronously to avoid blocking the UI/main thread ---
    QPointer<QDialog> dlg = new QDialog(parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle("Netstat Statistics");
    QScreen *screen = QApplication::primaryScreen();
    qreal dpiRatio = screen ? screen->devicePixelRatio() : 1.0;
    int minWidth = static_cast<int>(700 / dpiRatio);
    dlg->setMinimumWidth(minWidth);
    dlg->setSizeGripEnabled(true);
    dlg->setStyleSheet(
        "QDialog { background-color: #f8f9fa; border: 2px solid #34495e; border-radius: 8px; }"
    );

    QVBoxLayout *layout = new QVBoxLayout(dlg);

    QLabel *title = new QLabel("<b>Netstat Protocol Statistics</b><br>"
        "<span style='color:gray;'>Shows key IPv4 and IPv6 network health counters.<br>"
        "Hover any value for a detailed explanation.</span>");
    title->setTextFormat(Qt::RichText);
    layout->addWidget(title);

    QTableWidget *table = new QTableWidget(dlg);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels(QStringList() << "Statistic" << "IPv4" << "IPv6");
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setFocusPolicy(Qt::NoFocus);
    table->setStyleSheet(
        "QTableWidget { background-color: #ecf0f1; border: 2px solid #34495e; border-radius: 5px; gridline-color: #bdc3c7; font-size: 10pt; } "
        "QTableWidget::item { padding: 6px; border-bottom: 1px solid #d5dbdb; } "
        "QTableWidget::item:selected { background-color: #3498db; color: white; } "
        "QHeaderView::section { background-color: #34495e; color: white; padding: 8px; border: none; font-weight: bold; font-size: 10pt; }"
    );
    layout->addWidget(table);

    QLabel *legend = new QLabel(
        "<span style='color:#1a7d2c; font-weight:bold;'>Green:</span> Normal/healthy<br>"
        "<span style='color:#c80000; font-weight:bold;'>Red:</span> Errors, discards, or failures<br>"
        "<span style='color:#888;'>Gray:</span> Zero (not seen or not applicable)"
    );
    legend->setTextFormat(Qt::RichText);
    layout->addWidget(legend);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setAlignment(Qt::AlignCenter);
    QPushButton *closeBtn = new QPushButton("Close", dlg);
    closeBtn->setToolTip("Close this dialog");
    closeBtn->setStyleSheet(
        "QPushButton { "
        "    background-color: #34495e; "
        "    color: white; "
        "    border: none; "
        "    border-radius: 5px; "
        "    font-weight: bold; "
        "    padding: 8px 16px; "
        "    font-size: 10pt; "
        "} "
        "QPushButton:hover { background-color: #3c5872; } "
        "QPushButton:pressed { background-color: #22313a; }"
    );
    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, dlg, &QDialog::accept);

    // --- Asynchronous QProcess for netstat ---
    QProcess *proc = new QProcess(dlg);
    QObject::connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
        [=](int, QProcess::ExitStatus) {
            if (!dlg) return;
            QString output = QString::fromLocal8Bit(proc->readAllStandardOutput());
            proc->deleteLater();

            QMap<QString, QMap<QString, quint64>> stats;
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

            QStringList statNames = stats["IPv4"].keys();
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
                table->setItem(row, 0, nameItem);

                quint64 v4 = stats["IPv4"].value(stat, 0);
                QTableWidgetItem *v4Item = new QTableWidgetItem(humanize(v4));
                v4Item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                v4Item->setToolTip(QString("%1\n\n%2").arg(v4).arg(tooltips.value(stat, "")));
                v4Item->setForeground(v4 == 0 ? gray : (stat.contains("Error", Qt::CaseInsensitive) || stat.contains("Fail", Qt::CaseInsensitive) || stat.contains("Discard", Qt::CaseInsensitive) ? red : green));
                table->setItem(row, 1, v4Item);

                quint64 v6 = stats["IPv6"].value(stat, 0);
                QTableWidgetItem *v6Item = new QTableWidgetItem(humanize(v6));
                v6Item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                v6Item->setToolTip(QString("%1\n\n%2").arg(v6).arg(tooltips.value(stat, "")));
                v6Item->setForeground(v6 == 0 ? gray : (stat.contains("Error", Qt::CaseInsensitive) || stat.contains("Fail", Qt::CaseInsensitive) || stat.contains("Discard", Qt::CaseInsensitive) ? red : green));
                table->setItem(row, 2, v6Item);
            }
            table->resizeColumnsToContents();
        }
    );
    QObject::connect(proc, &QProcess::errorOccurred, [=](QProcess::ProcessError) {
        if (dlg)
            QMessageBox::critical(dlg, "Netstat Error", "Failed to run netstat -s.");
        proc->deleteLater();
    });

    proc->start("netstat", QStringList() << "-s");

    dlg->resize(static_cast<int>(900 / dpiRatio), static_cast<int>(600 / dpiRatio));
    dlg->exec();
    // No deleteLater() needed; Qt will clean up on close due to WA_DeleteOnClose.
}

void showRouteTableDialog(QWidget *parent) {
    // DPI-aware sizing
    QScreen *screen = QApplication::primaryScreen();
    qreal dpiRatio = screen ? screen->devicePixelRatio() : 1.0;
    int dialogWidth = static_cast<int>(1250 / dpiRatio);
    int dialogHeight = static_cast<int>(1000 / dpiRatio);

    // Use heap allocation and WA_DeleteOnClose for robust dialog lifetime
    QDialog *dlg = new QDialog(parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle("Route Table Viewer/Editor");
    dlg->resize(dialogWidth, dialogHeight);
    addCtrlWClose(dlg);

    QVBoxLayout *layout = new QVBoxLayout(dlg);

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
        "Destination", "Metric", "If", "Gateway", "Action"
    };
    QStringList ipv6Tips = {
        "Destination IPv6 network/prefix.\n(Technical: Network address in CIDR notation.)",
        "Route metric (priority).\n(Technical: Lower is preferred.)",
        "Interface index.\n(Technical: Windows adapter index for this route.)",
        "Gateway IPv6 address or On-link.\n(Technical: Next hop for this route.)",
        "Delete this route."
    };

    auto createTable = [&](const QStringList &headers, const QStringList &tips, int actionCol) -> QTableWidget* {
        QTableWidget *table = new QTableWidget();
        table->setColumnCount(headers.size());
        table->setHorizontalHeaderLabels(headers);
        for (int i = 0; i < headers.size(); ++i) {
            if (i == actionCol) {
                table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
            } else {
                table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
            }
            table->horizontalHeaderItem(i)->setToolTip(tips[i]);
        }
        table->verticalHeader()->setVisible(false);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setFocusPolicy(Qt::NoFocus);
        table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        table->horizontalHeader()->setMinimumSectionSize(80);
        table->setWordWrap(false);
        return table;
    };

    QTableWidget *ipv4Table = createTable(ipv4Headers, ipv4Tips, 5); // Action column is 5
    QTableWidget *ipv6Table = createTable(ipv6Headers, ipv6Tips, 4); // Action column is 4

    QBrush blue(QColor("#1c2684"));
    QBrush green(QColor("#1a7d2c"));
    QBrush red(QColor("#c80000"));
    QBrush gray(QColor("#888"));

    // Use std::function for recursion/capture
    std::function<void(QTableWidget*)> fillIPv4Table;
    fillIPv4Table = [=](QTableWidget *table) {
        table->setRowCount(0);
        table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
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
                    if (col == 0 || col == 2 || col == 3) {
                        item->setToolTip(parts[col]);
                    } else {
                        item->setToolTip(ipv4Tips[col]);
                    }
                    item->setFont(QFont("Segoe UI", 10, QFont::Bold));
                    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                    table->setItem(row, col, item);
                }
                QPushButton *delBtn = new QPushButton("Delete");
                delBtn->setToolTip("Delete this route.\n(Technical: route delete <destination> mask <netmask> <gateway>)");
                delBtn->setStyleSheet(
                    "QPushButton { background-color: #e74c3c; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 0 16px; min-width: 80px; min-height: 32px; font-size: 10.5pt; } "
                    "QPushButton:hover { background-color: #c0392b; } "
                    "QPushButton:pressed { background-color: #a93226; }"
                );
                table->setCellWidget(row, 5, delBtn);
                QObject::connect(delBtn, &QPushButton::clicked, [=]() {
                    QString dest = parts[0], mask = parts[1], gw = parts[2];
                    QMessageBox msgBox(QMessageBox::Question, "Delete Route",
                        QString("<div style='font-size:11pt;'><b>Are you sure you want to delete this route?</b><br><br>"
                                "<b>Destination:</b> %1<br>"
                                "<b>Netmask:</b> %2<br>"
                                "<b>Gateway:</b> %3</div>").arg(dest, mask, gw),
                        QMessageBox::Yes | QMessageBox::Cancel, dlg);
                    msgBox.setWindowIcon(QIcon(":/ip-address.ico"));
                    msgBox.setDefaultButton(QMessageBox::Cancel);
                    msgBox.setStyleSheet(
                        "QLabel { color: #222; font-size: 11pt; } "
                        "QPushButton { background-color: #e74c3c; color: white; border: none; border-radius: 5px; font-weight: bold; min-width: 90px; min-height: 32px; font-size: 10.5pt; padding: 0 16px; } "
                        "QPushButton:hover { background-color: #c0392b; } "
                        "QPushButton:pressed { background-color: #a93226; } "
                    );
                    int ret = msgBox.exec();
                    if (ret != QMessageBox::Yes) return;
                    QString psCmd = QString(
                        "Start-Process route -ArgumentList 'delete %1 mask %2 %3' -Verb runAs -WindowStyle Hidden"
                    ).arg(dest, mask, gw);
                    int result = QProcess::execute("powershell", QStringList() << "-WindowStyle" << "Hidden" << "-Command" << psCmd);
                    if (result == 0) {
                        QMessageBox msgBox(QMessageBox::Information, "Route Deleted",
                            "<b>Route deleted successfully.</b>",
                            QMessageBox::Ok, dlg);
                        msgBox.setWindowIcon(QIcon(":/ip-address.ico"));
                        msgBox.exec();
                        fillIPv4Table(table);
                    } else {
                        QMessageBox msgBox(QMessageBox::Critical, "Delete Route Failed",
                            "<b>Failed to delete route.</b><br>Administrator rights may be required or the operation was cancelled.",
                            QMessageBox::Ok, dlg);
                        msgBox.setWindowIcon(QIcon(":/ip-address.ico"));
                        msgBox.exec();
                    }
                });
            }
        }
    };

    std::function<void(QTableWidget*)> fillIPv6Table;
    fillIPv6Table = [=](QTableWidget *table) {
        table->setRowCount(0);
        table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        QProcess proc;
        proc.start("route", QStringList() << "print");
        proc.waitForFinished(2000);
        QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());

        int metricCol = 1, ifCol = 2, destCol = 0, gwCol = 3;
        int metricWidth = 48;
        int ifWidth = 32;
        int actionCol = 4;
        int actionWidth = 110;
        int totalWidth = table->viewport()->width();
        if (totalWidth < 400) totalWidth = 1200;
        int surplus = totalWidth - (metricWidth + ifWidth + actionWidth);
        int destWidth = surplus * 0.6;
        int gwWidth = surplus - destWidth;
        if (destWidth < 150) destWidth = 150;
        if (gwWidth < 120) gwWidth = 120;
        table->setColumnWidth(metricCol, metricWidth);
        table->setColumnWidth(ifCol, ifWidth);
        table->setColumnWidth(actionCol, actionWidth);
        table->setColumnWidth(destCol, destWidth);
        table->setColumnWidth(gwCol, gwWidth);

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
                    QString dest = parts[2];
                    QString metric = parts[1];
                    QString iface = parts[0];
                    QString gateway = parts[3];
                    int row = table->rowCount();
                    table->insertRow(row);
                    QTableWidgetItem *destItem = new QTableWidgetItem(dest);
                    destItem->setForeground(blue);
                    destItem->setToolTip(dest);
                    destItem->setFont(QFont("Segoe UI", 10, QFont::Bold));
                    destItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                    table->setItem(row, 0, destItem);
                    QTableWidgetItem *metricItem = new QTableWidgetItem(metric);
                    metricItem->setForeground(gray);
                    metricItem->setToolTip(ipv6Tips[1]);
                    metricItem->setFont(QFont("Segoe UI", 10, QFont::Bold));
                    metricItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                    table->setItem(row, 1, metricItem);
                    QTableWidgetItem *ifaceItem = new QTableWidgetItem(iface);
                    ifaceItem->setForeground(gray);
                    ifaceItem->setToolTip(ipv6Tips[2]);
                    ifaceItem->setFont(QFont("Segoe UI", 10, QFont::Bold));
                    ifaceItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                    table->setItem(row, 2, ifaceItem);
                    QTableWidgetItem *gwItem = new QTableWidgetItem(gateway);
                    gwItem->setForeground(green);
                    gwItem->setToolTip(gateway);
                    gwItem->setFont(QFont("Segoe UI", 10, QFont::Bold));
                    gwItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                    table->setItem(row, 3, gwItem);
                    QPushButton *delBtn = new QPushButton("Delete");
                    delBtn->setToolTip("Delete this route.\n(Technical: route delete <destination> -6)");
                    delBtn->setStyleSheet(
                        "QPushButton { background-color: #e74c3c; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 0 16px; min-width: 80px; min-height: 32px; font-size: 10.5pt; } "
                        "QPushButton:hover { background-color: #c0392b; } "
                        "QPushButton:pressed { background-color: #a93226; }"
                    );
                    table->setCellWidget(row, 4, delBtn);
                    QObject::connect(delBtn, &QPushButton::clicked, [=]() {
                        QMessageBox msgBox(QMessageBox::Question, "Delete IPv6 Route",
                            QString("<div style='font-size:11pt;'><b>Are you sure you want to delete this IPv6 route?</b><br><br>"
                                    "<b>Destination:</b> %1</div>").arg(dest),
                            QMessageBox::Yes | QMessageBox::Cancel, dlg);
                        msgBox.setWindowIcon(QIcon(":/ip-address.ico"));
                        msgBox.setDefaultButton(QMessageBox::Cancel);
                        msgBox.setStyleSheet(
                            "QLabel { color: #222; font-size: 11pt; } "
                            "QPushButton { background-color: #e74c3c; color: white; border: none; border-radius: 5px; font-weight: bold; min-width: 90px; min-height: 32px; font-size: 10.5pt; padding: 0 16px; } "
                            "QPushButton:hover { background-color: #c0392b; } "
                            "QPushButton:pressed { background-color: #a93226; } "
                        );
                        int ret = msgBox.exec();
                        if (ret != QMessageBox::Yes) return;
                        QString psCmd = QString(
                            "Start-Process route -ArgumentList 'delete %1 -6' -Verb runAs -WindowStyle Hidden"
                        ).arg(dest);
                        int result = QProcess::execute("powershell", QStringList() << "-WindowStyle" << "Hidden" << "-Command" << psCmd);
                        if (result == 0) {
                            QMessageBox msgBox(QMessageBox::Information, "IPv6 Route Deleted",
                                "<b>IPv6 route deleted successfully.</b>",
                                QMessageBox::Ok, dlg);
                            msgBox.setWindowIcon(QIcon(":/ip-address.ico"));
                            msgBox.exec();
                            fillIPv6Table(table);
                        } else {
                            QMessageBox msgBox(QMessageBox::Critical, "Delete IPv6 Route Failed",
                                "<b>Failed to delete IPv6 route.</b><br>Administrator rights may be required or the operation was cancelled.",
                                QMessageBox::Ok, dlg);
                            msgBox.setWindowIcon(QIcon(":/ip-address.ico"));
                            msgBox.exec();
                        }
                    });
                }
            }
        }
    };

    fillIPv4Table(ipv4Table);
    fillIPv6Table(ipv6Table);

    QLabel *ipv4Label = new QLabel("<b>IPv4 Routes</b>");
    ipv4Label->setTextFormat(Qt::RichText);
    layout->addWidget(ipv4Label);
    layout->addWidget(ipv4Table, 1);

    QLabel *ipv6Label = new QLabel("<b>IPv6 Routes</b>");
    ipv6Label->setTextFormat(Qt::RichText);
    layout->addWidget(ipv6Label);
    layout->addWidget(ipv6Table, 1);

    QHBoxLayout *btnRowLayout = new QHBoxLayout();
    QPushButton *showAddRouteBtn = new QPushButton("Add Route");
    showAddRouteBtn->setToolTip("Add a new route to the table.");
    showAddRouteBtn->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 0 32px; min-width: 120px; min-height: 38px; font-size: 12pt; } "
        "QPushButton:hover { background-color: #2ecc71; } "
        "QPushButton:pressed { background-color: #229954; }"
    );
    btnRowLayout->addWidget(showAddRouteBtn);
    btnRowLayout->addStretch();
    QPushButton *closeBtn = new QPushButton("Close");
    closeBtn->setToolTip("Close the dialog");
    closeBtn->setStyleSheet(
        "QPushButton { background-color: #34495e; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 0 32px; min-width: 120px; min-height: 38px; font-size: 12pt; } "
        "QPushButton:hover { background-color: #2c3e50; } "
        "QPushButton:pressed { background-color: #1a252f; }"
    );
    btnRowLayout->addWidget(closeBtn);
    layout->addLayout(btnRowLayout);

    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
    QObject::connect(showAddRouteBtn, &QPushButton::clicked, [=]() {
        QDialog addDlg(dlg);
        addDlg.setWindowTitle("Add Route");
        addDlg.setModal(true);
        addDlg.setMinimumWidth(420);
        addDlg.setMaximumWidth(600);
        addDlg.setStyleSheet(
            "QDialog { background-color: #f8f9fa; border: 2px solid #34495e; border-radius: 8px; }"
        );
        addCtrlWClose(&addDlg);
        QVBoxLayout *addLayout = new QVBoxLayout(&addDlg);
        QLabel *addTitle = new QLabel("<b>Add Route</b><br><span style='color:gray;'>Add a new IPv4 or IPv6 route. All fields required.</span>");
        addTitle->setTextFormat(Qt::RichText);
        addLayout->addWidget(addTitle);
        QFormLayout *form = new QFormLayout();
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
        form->addRow("Type:", protoCombo);
        form->addRow("Destination:", destEdit);
        form->addRow("Netmask:", maskEdit);
        form->addRow("Gateway:", gwEdit);
        form->addRow("Interface:", ifaceEdit);
        form->addRow("Metric:", metricSpin);
        addLayout->addLayout(form);
        QHBoxLayout *btnRow = new QHBoxLayout();
        btnRow->addStretch();
        QPushButton *addBtn = new QPushButton("Add");
        addBtn->setStyleSheet(
            "QPushButton { background-color: #27ae60; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 0 16px; min-width: 90px; min-height: 32px; font-size: 10.5pt; } "
            "QPushButton:hover { background-color: #2ecc71; } "
            "QPushButton:pressed { background-color: #229954; }"
        );
        QPushButton *cancelBtn = new QPushButton("Cancel");
        cancelBtn->setStyleSheet(
            "QPushButton { background-color: #e74c3c; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 0 16px; min-width: 90px; min-height: 32px; font-size: 10.5pt; } "
            "QPushButton:hover { background-color: #c0392b; } "
            "QPushButton:pressed { background-color: #a93226; }"
        );
        btnRow->addWidget(addBtn);
        btnRow->addSpacing(16);
        btnRow->addWidget(cancelBtn);
        btnRow->addStretch();
        addLayout->addSpacing(8);
        addLayout->addLayout(btnRow);
        QObject::connect(cancelBtn, &QPushButton::clicked, &addDlg, &QDialog::reject);
        QObject::connect(addBtn, &QPushButton::clicked, [&]() {
            QString proto = protoCombo->currentText();
            QString dest = destEdit->text().trimmed();
            QString mask = maskEdit->text().trimmed();
            QString gw = gwEdit->text().trimmed();
            QString iface = ifaceEdit->text().trimmed();
            int metric = metricSpin->value();
            if (dest.isEmpty() || gw.isEmpty() || iface.isEmpty() || (proto == "IPv4" && mask.isEmpty())) {
                QMessageBox msgBox(QMessageBox::Warning, "Missing Information",
                    "<div style='font-size:11pt;'><b>Please fill in all required fields.</b><br>All fields must be completed to add a route.</div>",
                    QMessageBox::Ok, &addDlg);
                msgBox.setWindowIcon(QIcon(":/ip-address.ico"));
                msgBox.setStyleSheet(
                    "QLabel { color: #222; font-size: 11pt; } "
                    "QPushButton { background-color: #e74c3c; color: white; border: none; border-radius: 5px; font-weight: bold; min-width: 90px; min-height: 32px; font-size: 10.5pt; padding: 0 16px; } "
                    "QPushButton:hover { background-color: #c0392b; } "
                    "QPushButton:pressed { background-color: #a93226; } "
                );
                msgBox.exec();
                return;
            }
            int ret = QMessageBox::question(&addDlg, "Add Route",
                QString("<b>Are you sure you want to add this route?</b><br><br>"
                        "<b>Type:</b> %1<br>"
                        "<b>Destination:</b> %2<br>"
                        "<b>Netmask:</b> %3<br>"
                        "<b>Gateway:</b> %4<br>"
                        "<b>Interface:</b> %5<br>"
                        "<b>Metric:</b> %6")
                    .arg(proto, dest, mask, gw, iface).arg(metric),
                QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
            if (ret != QMessageBox::Yes) return;
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
            if (result == 0) {
                QMessageBox msgBox(QMessageBox::Information, "Route Added",
                    "<b>Route added successfully.</b>",
                    QMessageBox::Ok, &addDlg);
                msgBox.setWindowIcon(QIcon(":/ip-address.ico"));
                msgBox.exec();
            } else {
                QMessageBox msgBox(QMessageBox::Critical, "Add Route Failed",
                    "<b>Failed to add route.</b><br>Administrator rights may be required.",
                    QMessageBox::Ok, &addDlg);
                msgBox.setWindowIcon(QIcon(":/ip-address.ico"));
                msgBox.exec();
            }
            fillIPv4Table(ipv4Table);
            fillIPv6Table(ipv6Table);
            addDlg.accept();
        });
        addDlg.exec();
    });

    dlg->exec();
    // No deleteLater needed; WA_DeleteOnClose ensures safe cleanup.
}

void showDnsCacheDialog(QWidget *parent) {
    // --- Robust, modern, DPI-aware DNS cache viewer dialog, auto-sizing columns and dialog height for 6 rows ---
    QDialog *dlg = new QDialog(parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle("DNS Cache Viewer");
    QScreen *screen = QApplication::primaryScreen();
    qreal dpiRatio = screen ? screen->devicePixelRatio() : 1.0;
    int minWidth = static_cast<int>(600 / dpiRatio);
    int minHeight = static_cast<int>(400 / dpiRatio);
    dlg->resize(minWidth, minHeight);
    dlg->setStyleSheet(
        "QDialog { background-color: #f8f9fa; border: 2px solid #34495e; border-radius: 8px; }"
    );
    addCtrlWClose(dlg);

    QVBoxLayout *layout = new QVBoxLayout(dlg);

    QLabel *title = new QLabel(
        "<b>Windows DNS Cache</b><br>"
        "<span style='color:gray;'>Shows all cached DNS entries.<br>"
        "You can flush the cache or copy entries.<br>"
        "Mouseover any value for technical details.</span>");
    title->setTextFormat(Qt::RichText);
    layout->addWidget(title);

    QTableWidget *table = new QTableWidget(dlg);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels(QStringList() << "Host Name" << "Type" << "IP Address" << "TTL (s)" << "Flags");
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->setFocusPolicy(Qt::NoFocus);
    table->setWordWrap(false);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    table->setStyleSheet(
        "QTableWidget { background-color: #ecf0f1; border: 2px solid #34495e; border-radius: 5px; gridline-color: #bdc3c7; font-size: 10pt; } "
        "QTableWidget::item { padding: 6px; border-bottom: 1px solid #d5dbdb; } "
        "QTableWidget::item:selected { background-color: #3498db; color: white; } "
        "QHeaderView::section { background-color: #34495e; color: white; padding: 8px; border: none; font-weight: bold; font-size: 10pt; }"
    );
    layout->addWidget(table, 1);

    // Helper: parse ipconfig /displaydns output
    auto parseDnsCache = [](const QString &output) {
        struct Entry { QString host, type, ip, ttl, flags; };
        QList<Entry> entries;
        QString currentHost, currentType, currentIp, currentTtl, currentFlags;
        for (const QString &line : output.split('\n')) {
            QString trimmed = line.trimmed();
            if (trimmed.startsWith("Record Name", Qt::CaseInsensitive)) {
                currentHost = trimmed.section(':', 1).trimmed();
            } else if (trimmed.startsWith("Record Type", Qt::CaseInsensitive)) {
                currentType = trimmed.section(':', 1).trimmed();
            } else if (trimmed.startsWith("Data", Qt::CaseInsensitive)) {
                currentIp = trimmed.section(':', 1).trimmed();
            } else if (trimmed.startsWith("Time To Live", Qt::CaseInsensitive)) {
                currentTtl = trimmed.section(':', 1).trimmed();
            } else if (trimmed.startsWith("Section", Qt::CaseInsensitive)) {
                currentFlags = trimmed.section(':', 1).trimmed();
            } else if (trimmed.isEmpty() && !currentHost.isEmpty()) {
                entries.append({currentHost, currentType, currentIp, currentTtl, currentFlags});
                currentHost.clear(); currentType.clear(); currentIp.clear(); currentTtl.clear(); currentFlags.clear();
            }
        }
        if (!currentHost.isEmpty())
            entries.append({currentHost, currentType, currentIp, currentTtl, currentFlags});
        return entries;
    };

    // Fill table with DNS cache and auto-size dialog
    auto fillTable = [=]() {
        table->setRowCount(0);
        QProcess proc;
        proc.start("ipconfig", QStringList() << "/displaydns");
        proc.waitForFinished(2000);
        QString output = QString::fromLocal8Bit(proc.readAllStandardOutput());
        auto entries = parseDnsCache(output);
        table->setRowCount(entries.size());
        for (int i = 0; i < entries.size(); ++i) {
            const auto &e = entries[i];
            table->setItem(i, 0, new QTableWidgetItem(e.host));
            table->setItem(i, 1, new QTableWidgetItem(e.type));
            table->setItem(i, 2, new QTableWidgetItem(e.ip));
            table->setItem(i, 3, new QTableWidgetItem(e.ttl));
            table->setItem(i, 4, new QTableWidgetItem(e.flags));
        }
        table->resizeColumnsToContents();

        // Calculate total width needed for all columns
        int totalWidth = table->verticalHeader()->width();
        for (int i = 0; i < table->columnCount(); ++i)
            totalWidth += table->columnWidth(i);
        totalWidth += table->frameWidth() * 2;
        if (table->verticalScrollBar()->isVisible())
            totalWidth += table->verticalScrollBar()->width();
        totalWidth += 32;
        int minDialogWidth = qMax(totalWidth, 420);

        // Calculate height for 6 rows (plus header)
        int rowHeight = 0;
        int rowsToMeasure = qMin(6, table->rowCount());
        for (int i = 0; i < rowsToMeasure; ++i)
            rowHeight += table->rowHeight(i);
        int headerHeight = table->horizontalHeader()->height();
        int extra = 60; // for margins, title, buttons
        int minDialogHeight = headerHeight + rowHeight + extra + 60; // +60 for buttons and layout

        dlg->resize(minDialogWidth, minDialogHeight);
        dlg->setMinimumWidth(minDialogWidth);
        dlg->setMinimumHeight(minDialogHeight);
    };

    fillTable();

    // Buttons
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *flushBtn = new QPushButton("Flush Cache");
    flushBtn->setToolTip("Flush the DNS resolver cache (requires admin rights).");
    flushBtn->setStyleSheet(
        "QPushButton { background-color: #e67e22; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 8px 16px; font-size: 10pt; } "
        "QPushButton:hover { background-color: #d35400; } "
        "QPushButton:pressed { background-color: #ba4a00; }"
    );
    QPushButton *copyBtn = new QPushButton("Copy Selected");
    copyBtn->setToolTip("Copy selected DNS entries to clipboard.");
    copyBtn->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 8px 16px; font-size: 10pt; } "
        "QPushButton:hover { background-color: #2980b9; } "
        "QPushButton:pressed { background-color: #1f618d; }"
    );
    QPushButton *closeBtn = new QPushButton("Close");
    closeBtn->setToolTip("Close this dialog");
    closeBtn->setStyleSheet(
        "QPushButton { background-color: #34495e; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 8px 16px; font-size: 10pt; } "
        "QPushButton:hover { background-color: #2c3e50; } "
        "QPushButton:pressed { background-color: #1a252f; }"
    );
    btnLayout->addWidget(flushBtn);
    btnLayout->addWidget(copyBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    QObject::connect(flushBtn, &QPushButton::clicked, [=]() {
        int ret = QMessageBox::question(dlg, "Flush DNS Cache",
            "Are you sure you want to flush the DNS resolver cache?\nThis requires administrator rights.",
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
        if (ret != QMessageBox::Yes) return;
        QString psCmd = "Start-Process ipconfig -ArgumentList '/flushdns' -Verb runAs -WindowStyle Hidden";
        int result = QProcess::execute("powershell", QStringList() << "-WindowStyle" << "Hidden" << "-Command" << psCmd);
        if (result == 0) {
            QMessageBox::information(dlg, "Flushed", "DNS cache flushed successfully.");
            fillTable();
        } else {
            QMessageBox::warning(dlg, "Flush Failed", "Failed to flush DNS cache. (Admin rights needed or cancelled.)");
        }
    });

    QObject::connect(copyBtn, &QPushButton::clicked, [=]() {
        QList<QTableWidgetSelectionRange> ranges = table->selectedRanges();
        if (ranges.isEmpty()) return;
        QStringList lines;
        for (const auto &range : ranges) {
            for (int row = range.topRow(); row <= range.bottomRow(); ++row) {
                QStringList rowVals;
                for (int col = range.leftColumn(); col <= range.rightColumn(); ++col) {
                    QTableWidgetItem *item = table->item(row, col);
                    rowVals << (item ? item->text() : "");
                }
                lines << rowVals.join('\t');
            }
        }
        QApplication::clipboard()->setText(lines.join('\n'));
    });

    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, dlg, &QDialog::accept);

    dlg->exec();
    // No deleteLater needed; WA_DeleteOnClose ensures safe cleanup.
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
    // DPI-aware sizing
    QScreen *screen = QApplication::primaryScreen();
    qreal dpiRatio = screen ? screen->devicePixelRatio() : 1.0;
    int dialogWidth = static_cast<int>(540 / dpiRatio);
    int dialogHeight = static_cast<int>(420 / dpiRatio);

    QDialog *dlg = new QDialog(parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle("RDAP Domain Lookup");
    dlg->resize(dialogWidth, dialogHeight);
    dlg->setMinimumWidth(dialogWidth);
    dlg->setMaximumWidth(qMax(dialogWidth, 700));
    addCtrlWClose(dlg);

    dlg->setStyleSheet(
        "QDialog { "
        "    background-color: #f8f9fa; "
        "    border: 2px solid #222; "
        "    border-radius: 10px; "
        "}"
    );

    QVBoxLayout *layout = new QVBoxLayout(dlg);

    QLabel *title = new QLabel(
        "<b>RDAP Domain Lookup</b><br>"
        "<span style='color:gray;'>Enter a domain (any TLD).<br>"
        "The correct RDAP server is chosen automatically.</span>");
    title->setTextFormat(Qt::RichText);
    layout->addWidget(title);

    // --- Input field, modern style ---
    QLineEdit *domainEdit = new QLineEdit;
    domainEdit->setPlaceholderText("e.g. www.google.com");
    domainEdit->setToolTip("Enter a domain name (any TLD).");
    domainEdit->setMinimumHeight(36);
    domainEdit->setStyleSheet(
        "QLineEdit { border: 2px solid #3498db; border-radius: 5px; padding: 0 12px; font-size: 11pt; } "
        "QLineEdit:focus { border-color: #2980b9; }"
    );
    layout->addWidget(domainEdit);

    // --- Output window, modern style ---
    QTextBrowser *output = new QTextBrowser;
    output->setReadOnly(true);
    output->setFontFamily("Consolas");
    int outputMinWidth = 420;
    int outputMaxWidth = 1100;
    int outputFixedHeight = 260;
    output->setMinimumWidth(outputMinWidth);
    output->setMaximumWidth(outputMaxWidth);
    output->setMinimumHeight(outputFixedHeight);
    output->setMaximumHeight(outputFixedHeight);
    output->setStyleSheet("QTextBrowser { background: #fff; color: #222; border: 2px solid #bbb; border-radius: 5px; font-size: 10.5pt; padding: 8px; }");
    output->setOpenExternalLinks(true);
    layout->addWidget(output);

    // Helper to resize dialog width to fit output content (up to max)
    auto adaptDialogWidth = [&]() {
        output->document()->adjustSize();
        int docWidth = int(output->document()->idealWidth()) + 32;
        int newWidth = qBound(outputMinWidth, docWidth, outputMaxWidth);
        dlg->resize(newWidth, dlg->height());
    };

    // --- Centered button row below output ---
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton *lookupBtn = new QPushButton("Lookup");
    lookupBtn->setToolTip("Perform RDAP lookup for the entered domain.");
    lookupBtn->setEnabled(false);
    lookupBtn->setMinimumWidth(110);
    lookupBtn->setMinimumHeight(36);
    lookupBtn->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 0 24px; font-size: 11pt; } "
        "QPushButton:hover { background-color: #2ecc71; } "
        "QPushButton:pressed { background-color: #229954; }"
    );
    btnLayout->addWidget(lookupBtn);
    btnLayout->addSpacing(24);
    QPushButton *closeBtn = new QPushButton("Close");
    closeBtn->setMinimumWidth(110);
    closeBtn->setMinimumHeight(36);
    closeBtn->setStyleSheet(
        "QPushButton { background-color: #34495e; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 0 24px; font-size: 11pt; } "
        "QPushButton:hover { background-color: #2c3e50; } "
        "QPushButton:pressed { background-color: #1a252f; }"
    );
    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    QObject::connect(domainEdit, &QLineEdit::textChanged, [=]() {
        lookupBtn->setEnabled(!domainEdit->text().trimmed().isEmpty());
    });

    QObject::connect(lookupBtn, &QPushButton::clicked, [=]() {
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
            adaptDialogWidth();
            return;
        }
        // Compose RDAP URL
        QString rdapUrl = rdapServer;
        if (!rdapUrl.endsWith('/')) rdapUrl += '/';
        rdapUrl += "domain/" + domain;

        output->setPlainText(QString("Querying RDAP server:\n%1 ...").arg(rdapUrl));
        output->setToolTip(QString("RDAP server: %1").arg(rdapServer));
        adaptDialogWidth();

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

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(response, &err);
        if (doc.isNull()) {
            output->setPlainText("Received non-JSON response:\n" + QString::fromUtf8(response.left(4096)));
            return;
        }

        QJsonObject obj = doc.object();
        QString html = "<b>RDAP Domain Information</b><br><table cellpadding='4' cellspacing='0'>";

        auto val = [&](const QString &s) {
            return "<span style='color:#1c2684; font-size:10pt;'>" + makeLinksClickable(s) + "</span>";
        };

        QString domainName = obj.value("ldhName").toString();
        if (!domainName.isEmpty())
            html += "<tr><td><b>Domain:</b></td><td>" + val(domainName) + "</td></tr>";

        QString handle = obj.value("handle").toString();
        if (!handle.isEmpty())
            html += "<tr><td><b>Handle:</b></td><td>" + val(handle) + "</td></tr>";

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

        QJsonObject secDns = obj.value("secureDNS").toObject();
        if (!secDns.isEmpty()) {
            bool ds = secDns.value("delegationSigned").toBool(false);
            html += "<tr><td><b>DNSSEC:</b></td><td>" + val(ds ? "Yes" : "No") + "</td></tr>";
        }

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

        if (html.count("<tr>") < 2) {
            html = "<pre>" + QString::fromUtf8(doc.toJson(QJsonDocument::Indented)).toHtmlEscaped() + "</pre>";
        }

        output->setHtml(html);
        output->setToolTip(QString("RDAP server: %1").arg(rdapServer));
        adaptDialogWidth();
    });

    QObject::connect(output->document(), &QTextDocument::contentsChanged, adaptDialogWidth);

    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, dlg, &QDialog::accept);

    dlg->exec();
    // No deleteLater needed; WA_DeleteOnClose ensures safe cleanup.
}



void showLanSharesDialog(QWidget *parent, const QString &singleTarget) {
    QDialog *dlg = new QDialog(parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle("LAN Shares Browser");
    dlg->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    QScreen *screen = QApplication::primaryScreen();
    qreal dpiRatio = screen ? screen->devicePixelRatio() : 1.0;
    int physicalWidth = static_cast<int>(1300 / dpiRatio);
    int physicalHeight = static_cast<int>(900 / dpiRatio);
    dlg->setFixedSize(physicalWidth, physicalHeight);
    dlg->resize(physicalWidth, physicalHeight);
    dlg->setStyleSheet(
        "QDialog { background-color: #f8f9fa; border: 2px solid #34495e; border-radius: 8px; }"
    );

    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, dlg, &QDialog::reject);

    QVBoxLayout *layout = new QVBoxLayout(dlg);

    QLabel *title = new QLabel(
        "<b>LAN Shares Browser</b><br>"
        "<span style='color:gray;'>Scan your network for computers and shared folders.<br>"
        "Expand a computer to see its shares. Click a share to copy its UNC path.</span>");
    title->setTextFormat(Qt::RichText);
    layout->addWidget(title);

    QHBoxLayout *progressLayout = new QHBoxLayout();
    QWidget *progressBarContainer = new QWidget(dlg);
    int progressBarWidth = static_cast<int>(physicalWidth * 0.95);
    progressBarContainer->setFixedWidth(progressBarWidth);
    QHBoxLayout *progressBarInnerLayout = new QHBoxLayout(progressBarContainer);
    QProgressBar *progress = new QProgressBar(dlg);
    progress->setMinimum(0);
    progress->setMaximum(100);
    progress->setValue(0);
    progress->setTextVisible(true);
    progress->setStyleSheet(
        "QProgressBar { background-color: #f8f9fa; border: 2px solid #34495e; border-radius: 8px; text-align: center; font-weight: bold; color: #2c3e50; min-height: 22px; } "
        "QProgressBar::chunk { background-color: #3498db; border-radius: 6px; }"
    );
    progressBarInnerLayout->addWidget(progress);
    progressLayout->addStretch();
    progressLayout->addWidget(progressBarContainer);
    progressLayout->addStretch();
    layout->addLayout(progressLayout);

    QGroupBox *rangeBox = new QGroupBox("Scan Range / Target");
    rangeBox->setStyleSheet(
        "QGroupBox { background-color: #f8f9fa; border: 2px solid #34495e; border-radius: 6px; margin-top: 16px; font-weight: bold; padding-top: 18px; padding-bottom: 8px; padding-left: 12px; padding-right: 12px; } "
        "QGroupBox::title { subcontrol-origin: margin; left: 18px; top: 2px; padding: 0 8px; color: #34495e; background: #f8f9fa; font-size: 11pt; }"
    );
    QHBoxLayout *rangeLayout = new QHBoxLayout(rangeBox);
    QLabel *fromLabel = new QLabel("<b>From:</b>");
    fromLabel->setMinimumWidth(55);
    QLineEdit *fromEdit = new QLineEdit;
    fromEdit->setMinimumWidth(120);
    fromEdit->setMaximumWidth(180);
    fromEdit->setStyleSheet(
        "QLineEdit { background-color: #fff; border: 1.5px solid #bbb; border-radius: 4px; padding: 4px 8px; font-size: 10.5pt; } "
        "QLineEdit:focus { border: 2px solid #1c2684; }"
    );
    QLabel *toLabel = new QLabel("<b>To:</b>");
    toLabel->setMinimumWidth(32);
    QLineEdit *toEdit = new QLineEdit;
    toEdit->setMinimumWidth(120);
    toEdit->setMaximumWidth(180);
    toEdit->setStyleSheet(
        "QLineEdit { background-color: #fff; border: 1.5px solid #bbb; border-radius: 4px; padding: 4px 8px; font-size: 10.5pt; } "
        "QLineEdit:focus { border: 2px solid #1c2684; }"
    );
    QPushButton *scanBtn = new QPushButton("Scan");
    scanBtn->setToolTip("Start scanning the specified range or target.");
    scanBtn->setMinimumWidth(80);
    scanBtn->setStyleSheet(
        "QPushButton { background-color: #1c2684; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 0 16px; min-width: 80px; min-height: 32px; } "
        "QPushButton:hover { background-color: #34495e; } "
        "QPushButton:pressed { background-color: #1a252f; }"
    );
    QPushButton *stopBtn = new QPushButton("Stop");
    stopBtn->setToolTip("Stop the current scan.");
    stopBtn->setMinimumWidth(80);
    stopBtn->setEnabled(false);
    stopBtn->setStyleSheet(
        "QPushButton { background-color: #e74c3c; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 0 16px; min-width: 80px; min-height: 32px; } "
        "QPushButton:hover { background-color: #c0392b; } "
        "QPushButton:pressed { background-color: #a93226; } "
        "QPushButton:disabled { background-color: #95a5a6; }"
    );
    rangeLayout->addWidget(fromLabel);
    rangeLayout->addWidget(fromEdit);
    rangeLayout->addSpacing(8);
    rangeLayout->addWidget(toLabel);
    rangeLayout->addWidget(toEdit);
    rangeLayout->addSpacing(12);
    rangeLayout->addWidget(scanBtn);
    rangeLayout->addSpacing(8);
    rangeLayout->addWidget(stopBtn);
    rangeLayout->addStretch();
    layout->addWidget(rangeBox);

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

    if (!singleTarget.isEmpty() && !singleTarget.trimmed().isEmpty()) {
        fromEdit->setText(singleTarget.trimmed());
        toEdit->setText("");
        fromEdit->setEnabled(false);
        toEdit->setEnabled(false);
        rangeBox->setTitle("Target");
    }

    QTreeWidget *tree = new QTreeWidget(dlg);
    tree->setHeaderLabels(QStringList() << "Computer / Share" << "Type" << "Comment");
    tree->setColumnWidth(0, 220);
    tree->setColumnWidth(1, 60);
    tree->setColumnWidth(2, 200);
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    tree->setExpandsOnDoubleClick(true);
    tree->setRootIsDecorated(true);
    tree->setStyleSheet(
        "QTreeWidget { background-color: #ecf0f1; border: 2px solid #34495e; border-radius: 5px; gridline-color: #bdc3c7; alternate-background-color: #f8f9fa; } "
        "QTreeWidget::item { padding: 6px; border-bottom: 1px solid #d5dbdb; } "
        "QTreeWidget::item:selected { background-color: #3498db; color: white; } "
        "QHeaderView::section { background-color: #34495e; color: white; padding: 8px; border: none; font-weight: bold; }"
    );
    tree->setAlternatingRowColors(true);
    QFont headerFont = tree->header()->font();
    headerFont.setBold(true);
    tree->header()->setFont(headerFont);
    layout->addWidget(tree, 1);

    QHBoxLayout *expColLayout = new QHBoxLayout();
    expColLayout->addStretch();
    QPushButton *expandAllBtn = new QPushButton("Expand all");
    expandAllBtn->setToolTip("Expand all computers and shares.");
    expandAllBtn->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 0 16px; min-width: 80px; min-height: 32px; } "
        "QPushButton:hover { background-color: #2ecc71; } "
        "QPushButton:pressed { background-color: #229954; }"
    );
    QPushButton *collapseAllBtn = new QPushButton("Collapse all");
    collapseAllBtn->setToolTip("Collapse all computers and shares.");
    collapseAllBtn->setStyleSheet(
        "QPushButton { background-color: #9b59b6; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 0 16px; min-width: 80px; min-height: 32px; } "
        "QPushButton:hover { background-color: #8e44ad; } "
        "QPushButton:pressed { background-color: #7d3c98; }"
    );
    QPushButton *closeBtn = new QPushButton("Close");
    closeBtn->setToolTip("Close the dialog.");
    closeBtn->setStyleSheet(
        "QPushButton { background-color: #34495e; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 0 16px; min-width: 80px; min-height: 32px; } "
        "QPushButton:hover { background-color: #2c3e50; } "
        "QPushButton:pressed { background-color: #1a252f; }"
    );
    expColLayout->addWidget(expandAllBtn);
    expColLayout->addWidget(collapseAllBtn);
    expColLayout->addWidget(closeBtn);
    expColLayout->addStretch();
    layout->addLayout(expColLayout);

    QObject::connect(expandAllBtn, &QPushButton::clicked, tree, &QTreeWidget::expandAll);
    QObject::connect(collapseAllBtn, &QPushButton::clicked, tree, &QTreeWidget::collapseAll);
    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::reject);

    QObject::connect(tree, &QTreeWidget::itemDoubleClicked, dlg, [tree, dlg](QTreeWidgetItem *item, int) {
        if (item->parent()) {
            QString unc = QString("\\\\%1\\%2").arg(item->parent()->text(0), item->text(0));
            QApplication::clipboard()->setText(unc);
            QMessageBox::information(dlg, "UNC Path Copied", QString("Copied to clipboard:\n%1").arg(unc));
        }
    });

    QThreadPool *pool = new QThreadPool(dlg);
    pool->setMaxThreadCount(16);

    auto enableControls = [=]() {
        bool enabled = singleTarget.isEmpty() || singleTarget.trimmed().isEmpty();
        if (fromEdit) fromEdit->setEnabled(enabled);
        if (toEdit) toEdit->setEnabled(enabled);
        if (scanBtn) scanBtn->setEnabled(true);
        if (stopBtn) stopBtn->setEnabled(false);
    };

    auto scanRunning = std::make_shared<bool>(false);
    auto cancelRequested = std::make_shared<bool>(false);

    auto doScan = [=]() {
        if (!tree || !progress) return;
        tree->clear();
        progress->setValue(0);

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

        int totalSteps = targets.size();
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
        uiTimer->start();

        auto aliveHosts = std::make_shared<QVector<QString>>();
        QMutex *aliveMutex = new QMutex;
        auto pingCompleted = std::make_shared<int>(0);

        for (int i = 0; i < targets.size(); ++i) {
            pool->start([=]() {
                if (*cancelRequested) return;
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
                    if (!*scanRunning) return;
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
                    int shareSteps = aliveHosts->size();
                    QMetaObject::invokeMethod(progress, [=]() {
                        progress->setMaximum(totalSteps + shareSteps);
                    }, Qt::QueuedConnection);

                    if (!*scanRunning) return;
                    auto completed = std::make_shared<int>(0);
                    auto foundHosts = std::make_shared<int>(0);

                    QSet<QString> *seenHosts = new QSet<QString>();
                    QMutex *resultsMutex = new QMutex;

                    for (int j = 0; j < aliveHosts->size(); ++j) {
                        pool->start([=]() {
                            if (*cancelRequested) return;
                            QString ip = (*aliveHosts)[j];
                            QList<QList<QString>> shares;
                            try {
                                shares = getSharesOnHost(ip);
                            } catch (...) {}

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
                                        if (!*scanRunning) return;
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
                                if (!*scanRunning) return;
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
                                    *scanRunning = false;
                                    scanBtn->setEnabled(true);
                                    stopBtn->setEnabled(false);
                                    progress->setValue(progress->maximum());
                                    enableControls();
                                    uiTimer->stop();
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
        if (pool) {
            pool->clear();
            pool->waitForDone();
        }
        enableControls();
    });

    auto safeCleanup = [=]() {
        *cancelRequested = true;
        *scanRunning = false;
        if (pool) {
            pool->clear();
            pool->waitForDone();
        }
    };
    QObject::connect(dlg, &QDialog::finished, safeCleanup);
    QObject::connect(dlg, &QDialog::rejected, safeCleanup);
    QObject::connect(dlg, &QDialog::destroyed, [=]() {
        *cancelRequested = true;
        *scanRunning = false;
        pool->clear();
        pool->waitForDone();
        delete pool;
    });

    enableControls();

    dlg->adjustSize();
    dlg->exec();
    // No deleteLater needed; WA_DeleteOnClose ensures safe cleanup and parent is never affected.
}

void showSslCertificateDialog(QWidget *parent) {
    // Always heap-allocate and set WA_DeleteOnClose for safety
    QDialog *dlg = new QDialog(parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle("SSL Certificate Check");
    dlg->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    // DPI-aware sizing
    QScreen *screen = QApplication::primaryScreen();
    qreal dpiRatio = screen ? screen->devicePixelRatio() : 1.0;
    int physicalWidth = static_cast<int>(1300 / dpiRatio);
    int physicalHeight = static_cast<int>(900 / dpiRatio);
    dlg->setFixedSize(physicalWidth, physicalHeight);
    dlg->resize(physicalWidth, physicalHeight);

    dlg->setStyleSheet(
        "QDialog { background-color: #f8f9fa; border: 2px solid #34495e; border-radius: 8px; }"
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(dlg);

    QLabel *prompt = new QLabel("SSL Certificate Checker - Enter host and port to check SSL certificates:");
    prompt->setStyleSheet("QLabel { color: #2c3e50; font-weight: bold; font-size: 11pt; }");
    mainLayout->addWidget(prompt);

    // Input row
    QHBoxLayout *inputLayout = new QHBoxLayout();
    QLabel *hostLabel = new QLabel("Host:");
    hostLabel->setStyleSheet("QLabel { color: #2c3e50; font-weight: bold; }");
    QLineEdit *hostEdit = new QLineEdit;
    hostEdit->setPlaceholderText("e.g. www.google.com");
    hostEdit->setMinimumWidth(200);
    QLabel *portLabel = new QLabel("Port:");
    portLabel->setStyleSheet("QLabel { color: #2c3e50; font-weight: bold; }");
    QLineEdit *portEdit = new QLineEdit("443");
    portEdit->setValidator(new QIntValidator(1, 65535, portEdit));
    portEdit->setMaximumWidth(60);
    QPushButton *checkBtn = new QPushButton("Check Certificate");
    checkBtn->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; border: none; border-radius: 5px; font-weight: bold; min-width: 120px; } "
        "QPushButton:hover { background-color: #2ecc71; } "
        "QPushButton:pressed { background-color: #229954; }"
    );
    inputLayout->addWidget(hostLabel);
    inputLayout->addWidget(hostEdit);
    inputLayout->addSpacing(10);
    inputLayout->addWidget(portLabel);
    inputLayout->addWidget(portEdit);
    inputLayout->addSpacing(10);
    inputLayout->addWidget(checkBtn);
    inputLayout->addStretch();
    mainLayout->addLayout(inputLayout);

    // Table for certificate list
    QTableWidget *table = new QTableWidget(dlg);
    table->setColumnCount(5);
    QStringList headers = {"Certificate", "Subject", "Issuer", "Expires", "In Store"};
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setVisible(false);
    table->setMinimumHeight(physicalHeight * 0.35);
    table->setStyleSheet(
        "QTableWidget { background-color: #ecf0f1; border: 2px solid #34495e; border-radius: 5px; gridline-color: #bdc3c7; } "
        "QTableWidget::item { padding: 6px; border-bottom: 1px solid #d5dbdb; } "
        "QTableWidget::item:selected { background-color: #3498db; color: white; } "
        "QHeaderView::section { background-color: #34495e; color: white; padding: 8px; border: none; font-weight: bold; }"
    );
    QFont headerFont = table->horizontalHeader()->font();
    headerFont.setBold(true);
    table->horizontalHeader()->setFont(headerFont);
    mainLayout->addWidget(table);

    // Certificate details section
    QLabel *detailsLabel = new QLabel("Certificate Details:");
    detailsLabel->setStyleSheet("QLabel { color: #2c3e50; font-weight: bold; font-size: 11pt; }");
    mainLayout->addWidget(detailsLabel);

    QTextEdit *detailsText = new QTextEdit;
    detailsText->setReadOnly(true);
    detailsText->setMinimumHeight(physicalHeight * 0.25);
    detailsText->setStyleSheet(
        "QTextEdit { background-color: #ecf0f1; border: 2px solid #34495e; border-radius: 5px; font-family: 'Consolas', 'Courier New', monospace; font-size: 9pt; color: #2c3e50; }"
    );
    mainLayout->addWidget(detailsText);

    // Button row at bottom
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    QPushButton *removeBtn = new QPushButton("Remove");
    removeBtn->setToolTip("Remove selected certificate from Windows store.");
    removeBtn->setEnabled(false);
    removeBtn->setStyleSheet(
        "QPushButton { background-color: #e74c3c; color: white; border: none; border-radius: 5px; font-weight: bold; min-width: 80px; } "
        "QPushButton:hover { background-color: #c0392b; } "
        "QPushButton:pressed { background-color: #a93226; } "
        "QPushButton:disabled { background-color: #95a5a6; }"
    );
    QPushButton *closeBtn = new QPushButton("Close");
    closeBtn->setToolTip("Close the dialog.");
    closeBtn->setStyleSheet(
        "QPushButton { background-color: #34495e; color: white; border: none; border-radius: 5px; font-weight: bold; min-width: 80px; } "
        "QPushButton:hover { background-color: #2c3e50; } "
        "QPushButton:pressed { background-color: #1a252f; }"
    );
    btnLayout->addWidget(removeBtn);
    btnLayout->addSpacing(10);
    btnLayout->addWidget(closeBtn);
    btnLayout->addStretch();
    mainLayout->addLayout(btnLayout);

    // Store certificate data
    struct CertInfo {
        QSslCertificate cert;
        QByteArray sha1;
        QString store;
        bool inStore;
    };
    QList<CertInfo> certList;

    // Certificate store detection (Windows API)
    auto certInStore = [](const QByteArray &sha1, QString &storeOut) -> bool {
        const char* storeNames[] = {"ROOT", "CA", "MY", "TrustedPeople"};
        const DWORD storeLocations[] = {CERT_SYSTEM_STORE_CURRENT_USER, CERT_SYSTEM_STORE_LOCAL_MACHINE};
        const char* locationNames[] = {"CurrentUser", "LocalMachine"};
        for (int loc = 0; loc < 2; loc++) {
            for (int store = 0; store < 4; store++) {
                HCERTSTORE hStore = CertOpenStore(
                    CERT_STORE_PROV_SYSTEM_A,
                    0,
                    0,
                    storeLocations[loc] | CERT_STORE_READONLY_FLAG,
                    storeNames[store]
                );
                if (!hStore) continue;
                PCCERT_CONTEXT pCertContext = NULL;
                bool found = false;
                while ((pCertContext = CertEnumCertificatesInStore(hStore, pCertContext)) != NULL) {
                    BYTE certHash[20];
                    DWORD hashSize = 20;
                    if (CertGetCertificateContextProperty(pCertContext, CERT_SHA1_HASH_PROP_ID, certHash, &hashSize)) {
                        QByteArray certSha1((const char*)certHash, hashSize);
                        if (certSha1 == sha1) {
                            found = true;
                            storeOut = QString("%1\\%2").arg(locationNames[loc], storeNames[store]);
                            break;
                        }
                    }
                }
                CertCloseStore(hStore, 0);
                if (found) return true;
            }
        }
        return false;
    };

    // Certificate check function
    auto doCheck = [&]() {
        QString host = hostEdit->text().trimmed();
        int port = portEdit->text().toInt();
        if (host.isEmpty() || port < 1 || port > 65535) {
            detailsText->setPlainText("Please enter a valid host and port.");
            return;
        }
        table->setRowCount(0);
        certList.clear();
        detailsText->clear();
        removeBtn->setEnabled(false);

        QSslSocket socket;
        socket.connectToHostEncrypted(host, port);
        bool ok = socket.waitForEncrypted(5000);

        if (!ok) {
            detailsText->setPlainText("Could not connect or handshake failed:\n" + socket.errorString());
            return;
        }

        QList<QSslCertificate> certs = socket.peerCertificateChain();
        if (certs.isEmpty()) {
            detailsText->setPlainText("No certificate received.");
            return;
        }

        for (const QSslCertificate &cert : certs) {
            QByteArray sha1 = cert.digest(QCryptographicHash::Sha1);
            QString store;
            bool inStore = certInStore(sha1, store);
            certList.append(CertInfo{cert, sha1, store, inStore});
        }
        std::sort(certList.begin(), certList.end(), [](const CertInfo &a, const CertInfo &b) {
            return a.inStore > b.inStore;
        });

        table->setRowCount(certList.size());
        for (int i = 0; i < certList.size(); ++i) {
            const CertInfo &ci = certList[i];
            table->setItem(i, 0, new QTableWidgetItem(QString("Cert #%1").arg(i + 1)));
            table->setItem(i, 1, new QTableWidgetItem(ci.cert.subjectInfo(QSslCertificate::CommonName).join(", ")));
            table->setItem(i, 2, new QTableWidgetItem(ci.cert.issuerInfo(QSslCertificate::CommonName).join(", ")));
            table->setItem(i, 3, new QTableWidgetItem(ci.cert.expiryDate().toString("yyyy-MM-dd")));
            QString storeText = ci.inStore ? "Yes" : "No";
            table->setItem(i, 4, new QTableWidgetItem(storeText));
            if (ci.inStore) {
                table->item(i, 4)->setBackground(QColor("#2ecc71"));
                table->item(i, 4)->setForeground(QColor("#2c3e50"));
                table->item(i, 4)->setFont(QFont("", -1, QFont::Bold));
            } else {
                table->item(i, 4)->setBackground(QColor("#3498db"));
                table->item(i, 4)->setForeground(QColor("#2c3e50"));
                table->item(i, 4)->setFont(QFont("", -1, QFont::Bold));
            }
        }
        if (!certList.isEmpty()) {
            table->selectRow(0);
        }
    };

    QObject::connect(table, &QTableWidget::itemSelectionChanged, [&]() {
        int row = table->currentRow();
        if (row >= 0 && row < certList.size()) {
            const CertInfo &ci = certList[row];
            QString details;
            details += QString("Certificate #%1\n").arg(row + 1);
            details += QString("Subject:     %1\n").arg(ci.cert.subjectInfo(QSslCertificate::CommonName).join(", "));
            details += QString("Issuer:      %1\n").arg(ci.cert.issuerInfo(QSslCertificate::CommonName).join(", "));
            details += QString("Valid from:  %1\n").arg(ci.cert.effectiveDate().toString());
            details += QString("Valid to:    %1\n").arg(ci.cert.expiryDate().toString());
            details += QString("Serial:      %1\n").arg(ci.cert.serialNumber());
            details += QString("SHA1:        %1\n").arg(QString::fromLatin1(ci.sha1.toHex()));
            details += QString("In Store:    %1\n").arg(ci.inStore ? QString("Yes (%1)").arg(ci.store) : "No");
            detailsText->setPlainText(details);
            removeBtn->setEnabled(ci.inStore);
        } else {
            detailsText->clear();
            removeBtn->setEnabled(false);
        }
    });

    QObject::connect(removeBtn, &QPushButton::clicked, [&]() {
        int row = table->currentRow();
        if (row >= 0 && row < certList.size()) {
            const CertInfo &ci = certList[row];
            if (!ci.inStore) return;
            QMessageBox msgBox(dlg);
            msgBox.setWindowTitle("Remove Certificate");
            msgBox.setText(QString("Are you sure you want to remove the certificate for [%1]?").arg(hostEdit->text()));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes) {
                // Remove from store using PowerShell (for user safety, not direct API)
                QString psCmd = QString("Remove-Item -Path Cert:\\LocalMachine\\%1\\%2 -Force").arg(ci.store, QString::fromLatin1(ci.sha1.toHex().toUpper()));
                int result = QProcess::execute("powershell", QStringList() << "-Command" << psCmd);
                if (result == 0) {
                    QMessageBox::information(dlg, "Certificate Removed", "Certificate removed from store.");
                    doCheck();
                } else {
                    QMessageBox::warning(dlg, "Failed", "Failed to remove certificate.");
                }
            }
        }
    });

    QObject::connect(checkBtn, &QPushButton::clicked, doCheck);
    QObject::connect(hostEdit, &QLineEdit::returnPressed, doCheck);
    QObject::connect(portEdit, &QLineEdit::returnPressed, doCheck);
    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::reject);

    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, dlg, &QDialog::reject);

    // On dialog finished/rejected/destroyed, always just close this dialog, never the parent
    auto safeCleanup = [=]() {
        // No special cleanup needed, but this ensures future extensibility
    };
    QObject::connect(dlg, &QDialog::finished, safeCleanup);
    QObject::connect(dlg, &QDialog::rejected, safeCleanup);
    QObject::connect(dlg, &QDialog::destroyed, safeCleanup);

    dlg->exec();
    // No deleteLater needed; WA_DeleteOnClose ensures safe cleanup and parent is never affected.
}

void showPingDialog(QWidget *parent) {
    QDialog *dlg = new QDialog(parent);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setWindowTitle("Ping Host");
    dlg->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    QScreen *screen = QApplication::primaryScreen();
    qreal dpiRatio = screen ? screen->devicePixelRatio() : 1.0;
    int minWidth = static_cast<int>(900 / dpiRatio);
    int minHeight = static_cast<int>(350 / dpiRatio);
    int maxHeight = static_cast<int>(800 / dpiRatio);
    int startHeight = static_cast<int>(420 / dpiRatio);
    dlg->setMinimumSize(minWidth, minHeight);
    dlg->setMaximumSize(minWidth, maxHeight);
    dlg->resize(minWidth, startHeight);

    dlg->setStyleSheet(
        "QDialog { background-color: #f8f9fa; border: 2px solid #34495e; border-radius: 8px; }"
    );

    QShortcut *closeShortcut = new QShortcut(QKeySequence("Ctrl+W"), dlg);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    QObject::connect(closeShortcut, &QShortcut::activated, dlg, &QDialog::reject);

    QVBoxLayout *pingLayout = new QVBoxLayout(dlg);
    pingLayout->setContentsMargins(18, 18, 18, 18);
    pingLayout->setSpacing(12);
    pingLayout->setSizeConstraint(QLayout::SetFixedSize);

    QLabel *prompt = new QLabel("Enter host or IP to ping:");
    pingLayout->addWidget(prompt);

    QLineEdit *input = new QLineEdit;
    input->setPlaceholderText("e.g. 8.8.8.8 or www.google.com");
    input->setStyleSheet(
        "QLineEdit { background-color: #fff; border: 2px solid #3498db; border-radius: 4px; padding: 6px 12px; font-size: 11pt; color: #222; } "
        "QLineEdit:focus { border: 2px solid #1c2684; } "
        "QLineEdit::placeholder { color: #888; font-style: italic; }"
    );
    pingLayout->addWidget(input);

    QLabel *counterLabel = new QLabel;
    int pingCount = 0;
    counterLabel->setText("<span style='color:blue;'>Pings: 0</span>");
    pingLayout->addWidget(counterLabel, 0, Qt::AlignLeft);

    QTextEdit *output = new QTextEdit;
    output->setReadOnly(true);
    output->setLineWrapMode(QTextEdit::NoWrap);
    output->setMinimumHeight(220);
    output->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));
    output->setStyleSheet(
        "QTextEdit { background-color: #ecf0f1; border: 2px solid #34495e; border-radius: 5px; font-family: 'Consolas', 'Courier New', monospace; font-size: 10pt; color: #222; padding: 8px; } "
        "QTextEdit:focus { border: 2px solid #1c2684; }"
    );
    pingLayout->addWidget(output, 1);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(18);
    btnLayout->addStretch();
    QPushButton *pingBtn = new QPushButton("Start");
    pingBtn->setToolTip("Start the ping to the specified host.");
    pingBtn->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 0 16px; min-width: 80px; min-height: 32px; } "
        "QPushButton:hover { background-color: #2ecc71; } "
        "QPushButton:pressed { background-color: #229954; }"
    );
    btnLayout->addWidget(pingBtn);
    QPushButton *bottomBtn = new QPushButton("Bottom");
    bottomBtn->setToolTip("Scroll to the bottom of the output.");
    bottomBtn->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 0 16px; min-width: 80px; min-height: 32px; } "
        "QPushButton:hover { background-color: #2980b9; } "
        "QPushButton:pressed { background-color: #1f618d; }"
    );
    btnLayout->addWidget(bottomBtn);
    QPushButton *stopCloseBtn = new QPushButton("Close");
    stopCloseBtn->setToolTip("Stop pinging or close the dialog.");
    stopCloseBtn->setStyleSheet(
        "QPushButton { background-color: #34495e; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 0 16px; min-width: 80px; min-height: 32px; } "
        "QPushButton:hover { background-color: #2c3e50; } "
        "QPushButton:pressed { background-color: #1a252f; } "
        "QPushButton:disabled { background-color: #95a5a6; }"
    );
    btnLayout->addWidget(stopCloseBtn);
    btnLayout->addStretch();
    pingLayout->addLayout(btnLayout);

    QObject::connect(bottomBtn, &QPushButton::clicked, [output]() {
        output->verticalScrollBar()->setValue(output->verticalScrollBar()->maximum());
    });

    QTimer *pingTimer = new QTimer(dlg);
    QProcess *pingProc = nullptr;
    bool isPinging = false;

    QTimer *autoScrollTimer = new QTimer(dlg);
    autoScrollTimer->setSingleShot(true);
    QScrollBar *vScroll = output->verticalScrollBar();
    bool userIsScrolling = false;

    QObject::connect(vScroll, &QScrollBar::sliderPressed, [=, &userIsScrolling]() {
        userIsScrolling = true;
        autoScrollTimer->stop();
    });
    QObject::connect(vScroll, &QScrollBar::sliderReleased, [=, &userIsScrolling]() {
        userIsScrolling = false;
        autoScrollTimer->start(3000);
    });
    QObject::connect(autoScrollTimer, &QTimer::timeout, [=, &userIsScrolling]() {
        if (!userIsScrolling && isPinging) {
            output->verticalScrollBar()->setValue(output->verticalScrollBar()->maximum());
        }
    });

    auto updateStopCloseText = [=]() {
        if (isPinging) {
            stopCloseBtn->setText("Stop");
            stopCloseBtn->setToolTip("Stop the ping");
        } else {
            stopCloseBtn->setText("Close");
            stopCloseBtn->setToolTip("Close the dialog");
        }
    };
    updateStopCloseText();

    auto stopPinging = [=, &pingProc, &isPinging]() mutable {
        pingTimer->stop();
        if (pingProc) {
            pingProc->kill();
            pingProc->deleteLater();
            pingProc = nullptr;
        }
        isPinging = false;
        pingBtn->setEnabled(true);
        updateStopCloseText();
    };

    QObject::connect(pingBtn, &QPushButton::clicked, [=, &pingCount, &pingProc, &isPinging]() mutable {
        QString host = input->text().trimmed();
        if (host.isEmpty()) {
            QMessageBox msgBox(dlg);
            msgBox.setWindowTitle("Input Error");
            msgBox.setText("Please enter a host or IP address to ping.");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStyleSheet(
                "QMessageBox { background-color: #f8f9fa; border: 2px solid #34495e; border-radius: 8px; } "
                "QPushButton { background-color: #e74c3c; color: white; border: none; border-radius: 5px; font-weight: bold; padding: 8px 16px; font-size: 10pt; } "
                "QPushButton:hover { background-color: #c0392b; } "
                "QPushButton:pressed { background-color: #a93226; }"
            );
            msgBox.exec();
            return;
        }
        isPinging = true;
        updateStopCloseText();
        pingBtn->setEnabled(false);
        pingCount = 0;
        output->clear();
        counterLabel->setText("<span style='color:blue;'>Pings: 0</span>");
        pingTimer->start(1000);
    });

    QObject::connect(stopCloseBtn, &QPushButton::clicked, [=, &pingProc, &isPinging]() mutable {
        if (isPinging) {
            stopPinging();
            output->append("<b>Ping stopped.</b>");
        } else {
            dlg->reject();
        }
    });

    QObject::connect(pingTimer, &QTimer::timeout, [=, &pingCount, &pingProc, &isPinging]() mutable {
        if (!isPinging) return;
        QString host = input->text().trimmed();
        if (host.isEmpty()) return;

        if (pingProc) {
            pingProc->kill();
            pingProc->deleteLater();
            pingProc = nullptr;
        }
        pingProc = new QProcess(dlg);
        QObject::connect(pingProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=, &pingCount, &pingProc](int, QProcess::ExitStatus) mutable {
                QString result = pingProc->readAllStandardOutput();
                ++pingCount;
                counterLabel->setText(QString("<span style='color:blue;'>Pings: %1</span>").arg(pingCount));
                QStringList lines = result.trimmed().split('\n');
                if (!lines.isEmpty()) {
                    output->append(QString("<span style='color:blue;'>[%1]</span> %2").arg(pingCount).arg(lines.first().trimmed()));
                    for (int i = 1; i < lines.size(); ++i)
                        output->append(lines[i].trimmed());
                }
            });
        pingProc->start("ping", QStringList() << "-n" << "1" << host);
    });

    auto safeCleanup = [=, &pingProc, &isPinging]() mutable {
        stopPinging();
    };
    QObject::connect(dlg, &QDialog::finished, safeCleanup);
    QObject::connect(dlg, &QDialog::rejected, safeCleanup);
    QObject::connect(dlg, &QDialog::destroyed, safeCleanup);

    dlg->exec();
    // No deleteLater needed; WA_DeleteOnClose ensures safe cleanup and parent is never affected.
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    int argc = 0;
    char** argv = nullptr;
    QApplication app(argc, argv);
#else
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
#endif

    // Force application-wide color palette to prevent white-on-white issues
    QPalette appPalette;
    appPalette.setColor(QPalette::Window, QColor(248, 249, 250));          // #f8f9fa
    appPalette.setColor(QPalette::WindowText, QColor(28, 40, 51));         // #1c2833 - darker
    appPalette.setColor(QPalette::Base, QColor(236, 240, 241));            // #ecf0f1
    appPalette.setColor(QPalette::AlternateBase, QColor(248, 249, 250));   // #f8f9fa
    appPalette.setColor(QPalette::Text, QColor(28, 40, 51));               // #1c2833 - darker
    appPalette.setColor(QPalette::Button, QColor(52, 73, 94));             // #34495e
    appPalette.setColor(QPalette::ButtonText, Qt::white);
    appPalette.setColor(QPalette::BrightText, Qt::white);
    appPalette.setColor(QPalette::Highlight, QColor(52, 152, 219));        // #3498db
    appPalette.setColor(QPalette::HighlightedText, Qt::white);
    appPalette.setColor(QPalette::Link, QColor(52, 152, 219));             // #3498db
    appPalette.setColor(QPalette::LinkVisited, QColor(155, 89, 182));      // #9b59b6
    app.setPalette(appPalette);

    // Global stylesheet for consistent appearance
    app.setStyleSheet(
        "* { color: #1c2833; background-color: #f8f9fa; } "
        "QWidget { color: #1c2833; } "
        "QLabel { color: #1c2833; background-color: transparent; } "
        "QTextEdit, QPlainTextEdit, QTextBrowser { "
        "    color: #1c2833; "
        "    background-color: #ecf0f1; "
        "    selection-color: white; "
        "    selection-background-color: #3498db; "
        "} "
        "QLineEdit, QSpinBox { "
        "    color: #1c2833; "
        "    background-color: white; "
        "    border: 2px solid #bdc3c7; "
        "    selection-color: white; "
        "    selection-background-color: #3498db; "
        "} "
        "QComboBox { "
        "    color: #1c2833; "
        "    background-color: white; "
        "    border: 2px solid #bdc3c7; "
        "    selection-color: white; "
        "    selection-background-color: #3498db; "
        "} "
        "QComboBox QAbstractItemView { "
        "    color: #1c2833; "
        "    background-color: white; "
        "    selection-color: white; "
        "    selection-background-color: #3498db; "
        "} "
        "QTableWidget, QTreeWidget { "
        "    color: #1c2833; "
        "    background-color: white; "
        "    alternate-background-color: #ecf0f1; "
        "    selection-color: white; "
        "    selection-background-color: #3498db; "
        "} "
        "QHeaderView::section { "
        "    color: white; "
        "    background-color: #34495e; "
        "    font-weight: bold; "
        "    border: 1px solid #2c3e50; "
        "} "
        "QGroupBox { "
        "    color: #1c2833; "
        "    background-color: transparent; "
        "    border: 2px solid #bdc3c7; "
        "} "
        "QGroupBox::title { "
        "    color: #1c2833; "
        "    background-color: transparent; "
        "} "
        "QDialog { "
        "    color: #1c2833; "
        "    background-color: #f8f9fa; "
        "} "
        "QMessageBox { "
        "    color: #1c2833; "
        "    background-color: #f8f9fa; "
        "} "
        "QScrollBar:vertical, QScrollBar:horizontal { "
        "    background-color: #ecf0f1; "
        "} "
        "QScrollBar::handle:vertical, QScrollBar::handle:horizontal { "
        "    background-color: #95a5a6; "
        "} "
        "QScrollBar::handle:vertical:hover, QScrollBar::handle:horizontal:hover { "
        "    background-color: #7f8c8d; "
        "} "
    );


    // ...reverted: using standard QMainWindow...


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
    
    // Apply modern styling to main window (no visible frame)
    window.setStyleSheet(
        "QMainWindow { "
        "    background-color: #f8f9fa; "
        "    border: none; "
        "} "
        "QMenuBar { "
        "    background-color: #34495e; "
        "    color: white; "
        "    border: none; "
        "    font-weight: bold; "
        "    font-size: 10pt; "
        "    margin-left: 2px; "
        "    margin-right: 2px; "
        "    padding: 1px; "
        "} "
        "QMenuBar::item { "
        "    background-color: transparent; "
        "    padding: 4px 16px; "
        "    margin: 2px; "
        "    border-radius: 4px; "
        "} "
        "QMenuBar::item:selected { "
        "    background-color: #2c3e50; "
        "    color: white; "
        "} "
        "QMenuBar::item:hover { "
        "    background-color: #2c3e50; "
        "    color: white; "
        "} "
        "QMenu { "
        "    background-color: #34495e; "
        "    border: 2px solid #2c3e50; "
        "    border-radius: 8px; "
        "    font-size: 10pt; "
        "    padding: 4px; "
        "} "
        "QMenu::item { "
        "    padding: 8px 16px; "
        "    color: white; "
        "    border-radius: 4px; "
        "    margin: 1px; "
        "} "
        "QMenu::item:selected { "
        "    background-color: #2c3e50; "
        "    color: white; "
        "    border: none; "
        "} "
        "QMenu::item:hover { "
        "    background-color: #2c3e50; "
        "    color: white; "
        "} "
        "QMenu::separator { "
        "    height: 2px; "
        "    background-color: #bdc3c7; "
        "    margin: 4px 8px; "
        "} "
        "QMenu::icon { "
        "    margin: 2px; "
        "}"
    );

    QWidget *centralWidget = new QWidget();
    centralWidget->setStyleSheet(
        "QWidget { "
        "    background-color: #f8f9fa; "
        "    border-radius: 8px; "
        "}"
    );
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    // Info box with modern styling
    QTextEdit *infoBox = new QTextEdit();
    infoBox->setReadOnly(true);
    infoBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    infoBox->setStyleSheet(
        "QTextEdit { "
        "    background-color: #ecf0f1; "
        "    border: none; "
        "    font-family: 'Consolas', 'Courier New', monospace; "
        "    font-size: 10pt; "
        "    padding: 6px; "
        "    color: #1c2833; "
        "}"
    );
    layout->addWidget(infoBox);
    updateIpDisplay(infoBox);

    // Buttons with modern styling
    QVBoxLayout *buttonRows = new QVBoxLayout();
    QHBoxLayout *row1 = new QHBoxLayout();
    QPushButton *expandBtn = new QPushButton("Advanced");
    expandBtn->setStyleSheet(
        "QPushButton { "
        "    background-color: #9b59b6; "
        "    color: white; "
        "    border: none; "
        "    border-radius: 5px; "
        "    font-weight: bold; "
        "    padding: 8px 16px; "
        "    font-size: 10pt; "
        "} "
        "QPushButton:hover { background-color: #8e44ad; } "
        "QPushButton:pressed { background-color: #7d3c98; }"
    );
    
    QPushButton *flushBtn = new QPushButton("Flush DNS");
    flushBtn->setStyleSheet(
        "QPushButton { "
        "    background-color: #3498db; "
        "    color: white; "
        "    border: none; "
        "    border-radius: 5px; "
        "    font-weight: bold; "
        "    padding: 8px 16px; "
        "    font-size: 10pt; "
        "} "
        "QPushButton:hover { background-color: #2980b9; } "
        "QPushButton:pressed { background-color: #1f618d; }"
    );
    
    row1->addWidget(expandBtn);
    row1->addWidget(flushBtn);

    QHBoxLayout *row2 = new QHBoxLayout();
    QPushButton *releaseBtn = new QPushButton("Release IP");
    releaseBtn->setStyleSheet(
        "QPushButton { "
        "    background-color: #e74c3c; "
        "    color: white; "
        "    border: none; "
        "    border-radius: 5px; "
        "    font-weight: bold; "
        "    padding: 8px 16px; "
        "    font-size: 10pt; "
        "} "
        "QPushButton:hover { background-color: #c0392b; } "
        "QPushButton:pressed { background-color: #a93226; }"
    );
    
    QPushButton *renewBtn = new QPushButton("Renew IP");
    renewBtn->setStyleSheet(
        "QPushButton { "
        "    background-color: #27ae60; "
        "    color: white; "
        "    border: none; "
        "    border-radius: 5px; "
        "    font-weight: bold; "
        "    padding: 8px 16px; "
        "    font-size: 10pt; "
        "} "
        "QPushButton:hover { background-color: #2ecc71; } "
        "QPushButton:pressed { background-color: #229954; }"
    );
    
    row2->addWidget(releaseBtn);
    row2->addWidget(renewBtn);

    buttonRows->addLayout(row1);
    buttonRows->addLayout(row2);
    layout->addLayout(buttonRows);

    QObject::connect(flushBtn, &QPushButton::clicked, [&]() {
        int result = QProcess::execute("ipconfig", QStringList() << "/flushdns");
        if (result == 0) {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Flush DNS");
            msgBox.setText("DNS cache flushed successfully!");
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setStyleSheet(
                "QMessageBox { "
                "    background-color: #f8f9fa; "
                "    border: 2px solid #34495e; "
                "    border-radius: 8px; "
                "} "
                "QPushButton { "
                "    background-color: #3498db; "
                "    color: white; "
                "    border: none; "
                "    border-radius: 5px; "
                "    font-weight: bold; "
                "    padding: 8px 16px; "
                "    font-size: 10pt; "
                "} "
                "QPushButton:hover { background-color: #2980b9; } "
                "QPushButton:pressed { background-color: #1f618d; }"
            );
            msgBox.exec();
        } else {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Flush DNS");
            msgBox.setText("Failed to flush DNS cache.");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStyleSheet(
                "QMessageBox { "
                "    background-color: #f8f9fa; "
                "    border: 2px solid #34495e; "
                "    border-radius: 8px; "
                "} "
                "QPushButton { "
                "    background-color: #e74c3c; "
                "    color: white; "
                "    border: none; "
                "    border-radius: 5px; "
                "    font-weight: bold; "
                "    padding: 8px 16px; "
                "    font-size: 10pt; "
                "} "
                "QPushButton:hover { background-color: #c0392b; } "
                "QPushButton:pressed { background-color: #a93226; }"
            );
            msgBox.exec();
        }
    });

    QObject::connect(releaseBtn, &QPushButton::clicked, [&]() {
        int result = QProcess::execute("ipconfig", QStringList() << "/release");
        if (result == 0) {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Release IP");
            msgBox.setText("IP address released successfully!");
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setStyleSheet(
                "QMessageBox { "
                "    background-color: #f8f9fa; "
                "    border: 2px solid #34495e; "
                "    border-radius: 8px; "
                "} "
                "QPushButton { "
                "    background-color: #27ae60; "
                "    color: white; "
                "    border: none; "
                "    border-radius: 5px; "
                "    font-weight: bold; "
                "    padding: 8px 16px; "
                "    font-size: 10pt; "
                "} "
                "QPushButton:hover { background-color: #2ecc71; } "
                "QPushButton:pressed { background-color: #229954; }"
            );
            msgBox.exec();
            updateIpDisplay(infoBox); // Refresh the display
        } else {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Release IP");
            msgBox.setText("Failed to release IP address.");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStyleSheet(
                "QMessageBox { "
                "    background-color: #f8f9fa; "
                "    border: 2px solid #34495e; "
                "    border-radius: 8px; "
                "} "
                "QPushButton { "
                "    background-color: #e74c3c; "
                "    color: white; "
                "    border: none; "
                "    border-radius: 5px; "
                "    font-weight: bold; "
                "    padding: 8px 16px; "
                "    font-size: 10pt; "
                "} "
                "QPushButton:hover { background-color: #c0392b; } "
                "QPushButton:pressed { background-color: #a93226; }"
            );
            msgBox.exec();
        }
    });

    QObject::connect(renewBtn, &QPushButton::clicked, [&]() {
        int result = QProcess::execute("ipconfig", QStringList() << "/renew");
        if (result == 0) {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Renew IP");
            msgBox.setText("IP address renewed successfully!");
            msgBox.setIcon(QMessageBox::Information);
            msgBox.setStyleSheet(
                "QMessageBox { "
                "    background-color: #f8f9fa; "
                "    border: 2px solid #34495e; "
                "    border-radius: 8px; "
                "} "
                "QPushButton { "
                "    background-color: #27ae60; "
                "    color: white; "
                "    border: none; "
                "    border-radius: 5px; "
                "    font-weight: bold; "
                "    padding: 8px 16px; "
                "    font-size: 10pt; "
                "} "
                "QPushButton:hover { background-color: #2ecc71; } "
                "QPushButton:pressed { background-color: #229954; }"
            );
            msgBox.exec();
            updateIpDisplay(infoBox); // Refresh the display
        } else {
            QMessageBox msgBox;
            msgBox.setWindowTitle("Renew IP");
            msgBox.setText("Failed to renew IP address.");
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setStyleSheet(
                "QMessageBox { "
                "    background-color: #f8f9fa; "
                "    border: 2px solid #34495e; "
                "    border-radius: 8px; "
                "} "
                "QPushButton { "
                "    background-color: #e74c3c; "
                "    color: white; "
                "    border: none; "
                "    border-radius: 5px; "
                "    font-weight: bold; "
                "    padding: 8px 16px; "
                "    font-size: 10pt; "
                "} "
                "QPushButton:hover { background-color: #c0392b; } "
                "QPushButton:pressed { background-color: #a93226; }"
            );
            msgBox.exec();
        }
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
            window.resize(300, 380);  // Compact size while showing all content
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

    QAction *arpAction           = netToolsMenu->addAction("🔗 Arp");
    QAction *dhcpStatusAction    = netToolsMenu->addAction("📋 DHCP Status");
    QAction *dnsCacheAction      = netToolsMenu->addAction("🗂️ DNS Cache Viewer");
    QAction *hostsFileAction     = netToolsMenu->addAction("📝 Hosts File Editor");
    QAction *sslCertAction       = netToolsMenu->addAction("🔒 HTTPS Certificate Check");
    QAction *netscanAction       = netToolsMenu->addAction("🔍 IP Scanner");
    QAction *lanSharesAction     = netToolsMenu->addAction("📁 LAN Shares");
    QAction *mtuDiscoveryAction  = netToolsMenu->addAction("📏 MTU Discovery");
    QAction *netstatStatsAction  = netToolsMenu->addAction("📊 Netstat Statistics");
    QAction *adaptersAction      = netToolsMenu->addAction("🔌 Network Adapters");
    QAction *netUsageAction      = netToolsMenu->addAction("📈 Network Usage");
    QAction *nslookupAction      = netToolsMenu->addAction("🔎 NS Lookup");
    QAction *pingAction          = netToolsMenu->addAction("📡 Ping");
    QAction *portscanAction      = netToolsMenu->addAction("🚪 Port Scan");
    QAction *routeTableAction    = netToolsMenu->addAction("🗺️ Route Table Viewer/Editor");
    QAction *tracertAction       = netToolsMenu->addAction("🛤️ Traceroute");
    QAction *whoisLookupAction   = netToolsMenu->addAction("📇 Whois (RDAP) Lookup");
    QAction *wifiScanAction      = netToolsMenu->addAction("📶 WiFi Scan");

    QAction *alwaysOnTopAction = fileMenu->addAction("📌 Always on top");
    QAction *deleteTempAction = fileMenu->addAction("🗑️ Delete Temporary Files");
    fileMenu->addSeparator();
    QAction *exitAction = fileMenu->addAction("🚪 E&xit");
    exitAction->setShortcut(QKeySequence::Quit);

    // Function to update always on top action appearance
    auto updateAlwaysOnTopState = [&alwaysOnTopAction, &window]() {
        bool isOnTop = window.windowFlags() & Qt::WindowStaysOnTopHint;
        if (isOnTop) {
            alwaysOnTopAction->setText("🔴 Always on top");
        } else {
            alwaysOnTopAction->setText("◯ Always on top");
        }
    };

    // Initialize the state
    updateAlwaysOnTopState();

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
    QObject::connect(alwaysOnTopAction, &QAction::triggered, [&, updateAlwaysOnTopState]() {
        bool onTop = !(window.windowFlags() & Qt::WindowStaysOnTopHint);
        window.setWindowFlag(Qt::WindowStaysOnTopHint, onTop);
        window.show();
        updateAlwaysOnTopState();
    });
    QObject::connect(deleteTempAction, &QAction::triggered, [&window]() {
        // Delete IPGui temporary files directory
        QString ipguiTempDir = "C:/Users/Public/AppData/Local/IPGui";
        QDir dir(ipguiTempDir);
        
        if (!dir.exists()) {
            QMessageBox::information(&window, "Delete Temporary Files", 
                QString("No temporary files found.\n\nDirectory: %1").arg(ipguiTempDir));
            return;
        }
        
        // Confirm deletion
        int ret = QMessageBox::question(&window, "Delete Temporary Files", 
            QString("Are you sure you want to delete all IPGui temporary files?\n\n"
                    "Directory: %1\n\n"
                    "This will delete all cached files, backups, and temporary data.")
                .arg(ipguiTempDir),
            QMessageBox::Yes | QMessageBox::Cancel, 
            QMessageBox::Cancel);
            
        if (ret == QMessageBox::Yes) {
            // Remove the entire IPGui directory
            bool success = dir.removeRecursively();
            
            if (success) {
                QMessageBox::information(&window, "Delete Temporary Files", 
                    "Temporary files deleted successfully!");
            } else {
                QMessageBox::warning(&window, "Delete Temporary Files", 
                    "Failed to delete some temporary files.\n\n"
                    "Some files may be in use or require administrator privileges.");
            }
        }
    });
    QObject::connect(exitAction, &QAction::triggered, [&window]() {
        window.close();
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
QAction *manualAction = helpMenu->addAction("📖 User Manual (PDF)");
QAction *checkUpdatesAction = helpMenu->addAction("🔄 Check for Updates");
helpMenu->addSeparator();
QAction *aboutAction = helpMenu->addAction("ℹ️ &About");

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

QObject::connect(checkUpdatesAction, &QAction::triggered, [&window]() {
    checkForUpdates(&window, false); // false = not silent, show result dialog
});

QObject::connect(aboutAction, &QAction::triggered, [&window]() {
    QDialog dlg(&window);
    dlg.setWindowTitle("About");
    QVBoxLayout layout(&dlg);

    QLabel *iconLabel = new QLabel;
    iconLabel->setPixmap(QPixmap("ipgui_logo.png").scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
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



    window.resize(300, 380);  // Compact initial size showing all content
    window.show();



    // Ensure all modal dialogs close on app quit
    QObject::connect(&app, &QApplication::aboutToQuit, []() {
        for (QWidget *w : QApplication::topLevelWidgets()) {
            if (qobject_cast<QDialog*>(w))
                w->close();
        }
    });

    // Check for updates on startup (silent check)
    QTimer::singleShot(2000, [&window]() {
        checkForUpdates(&window, true); // true = silent, only show if update available
    });

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
// Modern SSL Certificate Dialog - New Implementation
// This is the complete modern version with table-based interface

