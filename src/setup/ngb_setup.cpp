/*
 * ngb-setup -- the first-run window: asks for the player's own disc and
 * installs it (src/install.c), with progress.
 *
 *   ngb-setup --install-to <dir> [--portable]
 *
 * The game runs it when it finds no game files. On success it prints the
 * game folder and exits 0; closed, it exits 1. --portable offers portable mode
 * (src/main.c): chosen, it makes portable.txt, prints "portable:" and the disc
 * image or folder picked, and exits 0 -- the game starts again with it. Anything else (no display,
 * missing libraries) and the game falls back to a desktop file dialog.
 */
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <cstdio>

#include "../install.h"

class SetupWindow : public QWidget {
public:
    SetupWindow(const QString &installTo, bool offerPortable) : m_installTo(installTo)
    {
        setWindowTitle("Ninja Gaiden Black - Setup");
        setWindowIcon(QIcon(":/ngb.png"));
        setAcceptDrops(true);

        auto *icon = new QLabel;
        icon->setPixmap(QPixmap(":/ngb.png").scaled(72, 72, Qt::KeepAspectRatio,
                                                    Qt::SmoothTransformation));
        auto *title = new QLabel("<b style='font-size:15pt'>Ninja Gaiden Black</b><br>"
                                 "Static recompilation");
        auto *head = new QHBoxLayout;
        head->addWidget(icon);
        head->addSpacing(8);
        head->addWidget(title, 1);

        /* Page 0: choosing */
        auto *intro = new QLabel(QString(
            "The game itself is not included. To play, choose your own copy "
            "of the Xbox game (the retail USA release):"
            "<ul style='margin-left:-20px'>"
            "<li>a disc image: <i>.iso</i>, including a full Redump dump, or the "
            "<i>.7z</i> / <i>.zip</i> it came in</li>"
            "<li>or a folder of extracted files, with <i>default.xbe</i></li></ul>"
            "A disc image is installed once (about 6 GB) into<br><tt>%1</tt>"
            "<br>and the game starts from there from then on. You can also drop "
            "the file on this window.").arg(shortPath(m_installTo).toHtmlEscaped()));
        intro->setWordWrap(true);
        intro->setTextFormat(Qt::RichText);

        auto *image = new QPushButton("Choose disc image...");
        auto *folder = new QPushButton("Choose folder...");
        auto *quit = new QPushButton("Quit");
        image->setDefault(true);
        connect(image, &QPushButton::clicked, this, &SetupWindow::chooseImage);
        connect(folder, &QPushButton::clicked, this, &SetupWindow::chooseFolder);
        connect(quit, &QPushButton::clicked, this, &QWidget::close);
        auto *buttons = new QHBoxLayout;
        buttons->addWidget(quit);
        buttons->addStretch(1);
        buttons->addWidget(folder);
        buttons->addWidget(image);

        /* The text scrolls if the window cannot be tall enough for it. */
        auto *scroll = new QScrollArea;
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setWidgetResizable(true);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setWidget(intro);
        intro->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        auto *choose = new QWidget;
        auto *cl = new QVBoxLayout(choose);
        cl->setContentsMargins(0, 0, 0, 0);
        cl->addWidget(scroll, 1);
        char base[4096];
        if (offerPortable && ngb_portable_base(base, sizeof base)) {   /* issue #4 */
            m_portable = new QCheckBox("Keep the game, saves and settings next to the program (portable)");
            m_portable->setToolTip("In " + shortPath(QFile::decodeName(base)) + "/userdata");
            cl->addWidget(m_portable);
        }
        cl->addLayout(buttons);

        /* Page 1: installing */
        m_status = new QLabel;
        m_status->setWordWrap(true);
        m_bar = new QProgressBar;
        m_bar->setTextVisible(true);
        m_cancel = new QPushButton("Cancel");
        connect(m_cancel, &QPushButton::clicked, this, [this] {
            m_cancel->setEnabled(false);
            m_status->setText("Cancelling...");
            ngb_install_cancel();
        });
        auto *cancelRow = new QHBoxLayout;
        cancelRow->addStretch(1);
        cancelRow->addWidget(m_cancel);

        auto *work = new QWidget;
        auto *wl = new QVBoxLayout(work);
        wl->setContentsMargins(0, 0, 0, 0);
        wl->addStretch(1);
        wl->addWidget(m_status);
        wl->addWidget(m_bar);
        wl->addStretch(1);
        wl->addLayout(cancelRow);

        m_pages = new QStackedWidget;
        m_pages->addWidget(choose);
        m_pages->addWidget(work);

        auto *root = new QVBoxLayout(this);
        root->setContentsMargins(20, 18, 20, 16);
        root->addLayout(head);
        root->addSpacing(10);
        root->addWidget(m_pages, 1);

        /* Sized to the text at the font in use (large on a Steam Deck), within
         * the screen. */
        const QRect avail = screen() ? screen()->availableGeometry() : QRect(0, 0, 1280, 800);
        const int w = qMin(qMax(560, fontMetrics().averageCharWidth() * 80), avail.width() - 32);
        const int inner = w - 40 - scroll->verticalScrollBar()->sizeHint().width();
        const int h = 18 + head->sizeHint().height() + 10 + intro->heightForWidth(inner)
                      + 2 * cl->spacing() + buttons->sizeHint().height() + 16 + 8
                      + (m_portable ? m_portable->sizeHint().height() + cl->spacing() : 0);
        resize(w, qMin(qMax(h, 300), avail.height() - 48));
    }

    bool installed() const { return m_done; }
    void installForTest(const QString &src) { chosen(src); }
    QString portableSource() const { return m_portableSrc; }
    QString gameDir() const { return m_gameDir; }

protected:
    void dragEnterEvent(QDragEnterEvent *e) override
    {
        if (m_pages->currentIndex() == 0 && e->mimeData()->hasUrls())
            e->acceptProposedAction();
    }

    void dropEvent(QDropEvent *e) override
    {
        const QList<QUrl> urls = e->mimeData()->urls();
        if (!urls.isEmpty() && urls.first().isLocalFile())
            chosen(urls.first().toLocalFile());
    }

    void closeEvent(QCloseEvent *e) override
    {
        if (m_thread) {
            /* Stop the install first; the window closes when it has. */
            m_closing = true;
            m_cancel->click();
            e->ignore();
            return;
        }
        e->accept();
    }

private:
    void chooseImage()
    {
        QString f = ask(QFileDialog::ExistingFile, "Choose your Ninja Gaiden Black disc image",
                        "Xbox disc images and archives (*.iso *.xiso *.7z *.zip *.xbe);;"
                        "All files (*)");
        if (!f.isEmpty())
            chosen(f);
    }

    void chooseFolder()
    {
        QString d = ask(QFileDialog::Directory,
                        "Choose the folder with the game's files (default.xbe)", QString());
        if (!d.isEmpty())
            chosen(d);
    }

    /* Portable mode ticked: the game starts again with it (src/install.c), its folders beside it. */
    void chosen(const QString &src)
    {
        if (m_portable && m_portable->isChecked()) {
            if (ngb_portable_enable()) {
                m_portableSrc = src;
                m_done = true;
                close();
                return;
            }
            QMessageBox::warning(this, "Ninja Gaiden Black",
                                 "Portable mode needs a folder the program can write to. The game "
                                 "is installed in your profile instead.");
        }
        install(src);
    }

    /* A file dialog no bigger than the screen (Qt's own one, when the desktop
     * offers none, opens at a size that overflows a Steam Deck's). */
    QString ask(QFileDialog::FileMode mode, const QString &title, const QString &filter)
    {
        QFileDialog dlg(this, title, startDir(), filter);
        dlg.setFileMode(mode);
        if (mode == QFileDialog::Directory)
            dlg.setOption(QFileDialog::ShowDirsOnly);
        const QRect avail = screen() ? screen()->availableGeometry() : QRect(0, 0, 1280, 800);
        dlg.resize(qMin(900, avail.width() - 32), qMin(600, avail.height() - 48));
        if (dlg.exec() != QDialog::Accepted || dlg.selectedFiles().isEmpty())
            return QString();
        return dlg.selectedFiles().first();
    }

    static QString shortPath(const QString &p)
    {
        const QString home = QDir::homePath();
        return p.startsWith(home + "/") ? "~" + p.mid(home.size()) : p;
    }

    QString startDir() const
    {
        QString d = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        return QDir(d).exists() ? d : QDir::homePath();
    }

    static void onProgress(uint64_t done, uint64_t total, void *ud)
    {
        auto *self = static_cast<SetupWindow *>(ud);
        int permille = total ? int(done * 1000 / total) : -1;
        if (permille == self->m_lastPermille)
            return;
        self->m_lastPermille = permille;
        QMetaObject::invokeMethod(self, [self, permille, done] {
            if (permille < 0) {
                self->m_status->setText("Unpacking the archive (this needs some "
                                        "free space for a while)...");
                self->m_bar->setRange(0, 0);
            } else {
                self->m_status->setText(QString("Installing the game... %1 MB")
                                            .arg(qulonglong(done >> 20)));
                self->m_bar->setRange(0, 1000);
                self->m_bar->setValue(permille);
                self->m_bar->setFormat(QString("%1%").arg(permille / 10));
            }
        }, Qt::QueuedConnection);
    }

    void install(const QString &src)
    {
        m_pages->setCurrentIndex(1);
        m_status->setText("Reading " + QFileInfo(src).fileName() + "...");
        m_bar->setRange(0, 0);
        m_cancel->setEnabled(true);
        m_lastPermille = -2;

        const QByteArray s = QFile::encodeName(src);
        const QByteArray to = QFile::encodeName(m_installTo);
        m_thread = QThread::create([this, s, to] {
            char out[4096];
            ngb_install_set_progress(onProgress, this);
            int rc = ngb_install(s.constData(), to.constData(), out, sizeof out);
            QString err = QString::fromLocal8Bit(ngb_install_error());
            QString dir = QFile::decodeName(out);
            QMetaObject::invokeMethod(this, [this, rc, err, dir] {
                finished(rc, err, dir);
            }, Qt::QueuedConnection);
        });
        m_thread->start();
    }

    void finished(int rc, const QString &err, const QString &dir)
    {
        m_thread->wait();
        delete m_thread;
        m_thread = nullptr;
        if (rc == 0) {
            m_done = true;
            m_gameDir = dir;
            close();
            return;
        }
        if (m_closing) {
            close();
            return;
        }
        m_pages->setCurrentIndex(0);
        if (err != "Cancelled.")
            QMessageBox::warning(this, "Ninja Gaiden Black",
                                 "The game could not be installed.\n\n" + err);
    }

    QString m_installTo, m_gameDir, m_portableSrc;
    QCheckBox *m_portable = nullptr;
    QStackedWidget *m_pages;
    QLabel *m_status;
    QProgressBar *m_bar;
    QPushButton *m_cancel;
    QThread *m_thread = nullptr;
    int m_lastPermille = -2;
    bool m_done = false, m_closing = false;
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Ninja Gaiden Black");
    QApplication::setDesktopFileName("ninja-gaiden-black");

    QString installTo;
    const QStringList args = app.arguments();
    int i = args.indexOf("--install-to");
    if (i >= 0 && i + 1 < args.size())
        installTo = args[i + 1];
    if (installTo.isEmpty()) {
        std::fprintf(stderr, "usage: ngb-setup --install-to <dir>\n");
        return 2;
    }

    SetupWindow w(installTo, args.contains("--portable"));
    w.show();
    /* Tests: NGB_SETUP_SHOT=prefix saves the window every second as
     * prefix-N.png; NGB_SETUP_AUTO=path installs it as if chosen. */
    QByteArray shot = qgetenv("NGB_SETUP_SHOT");
    if (!shot.isEmpty()) {
        auto *t = new QTimer(&w);
        QObject::connect(t, &QTimer::timeout, &w, [&w, shot] {
            static int n;
            w.grab().save(QString("%1-%2.png").arg(QString(shot)).arg(n++));
        });
        t->start(1000);
    }
    QByteArray autoSrc = qgetenv("NGB_SETUP_AUTO");
    if (!autoSrc.isEmpty())
        QTimer::singleShot(1500, &w, [&w, autoSrc] { w.installForTest(QFile::decodeName(autoSrc)); });
    app.exec();
    if (!w.installed())
        return 1;
    if (!w.portableSource().isEmpty())
        std::printf("portable:%s\n", QFile::encodeName(w.portableSource()).constData());
    else
        std::printf("%s\n", QFile::encodeName(w.gameDir()).constData());
    return 0;
}
