// This is an open source non-commercial project.
#include "aboutform.h"
#include "ui_aboutform.h"
#include <QDesktopServices>

using namespace Qt::Literals;

AboutForm::AboutForm(QWidget* parent)
    : QDialog{parent}
    , ui(new Ui::AboutForm) {
    ui->setupUi(this);

    // Bloco de informações técnicas sem a linha do Git
    QString info = u"Build: %1<br/>"
#ifdef __MINGW32__
        u"GCC: %2.%3.%4 (MinGW %5)<br/>"
#elif __GNUG__
        u"GCC: %2<br/>"
#else
        u"MSC: %2<br/>"
#endif
        u"Application Version: %3"_s
    .arg(QString::fromUtf8(BUILD_DATE))
#ifdef __MINGW32__
    .arg(__GNUC__).arg(__GNUC_MINOR__).arg(__GNUC_PATCHLEVEL__).arg(__MINGW64_VERSION_STR)
#elif __GNUG__
    .arg(QString::fromUtf8(__VERSION__))
#else
    .arg(_MSC_VER)
#endif
    .arg(qApp->applicationVersion());

    ui->lblAbout->setText(ui->lblAbout->text().arg(info));

    connect(ui->cmdOk_2, &QPushButton::clicked, this, &AboutForm::accept);
    connect(ui->lblAbout, &QLabel::linkActivated, [](const QString& link) { QDesktopServices::openUrl(QUrl{link}); });
}

AboutForm::~AboutForm() { delete ui; }

#include "moc_aboutform.cpp"
