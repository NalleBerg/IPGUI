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