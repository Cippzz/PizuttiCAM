// Este é um projeto de código aberto não comercial.
/********************************************************************************
 * Autor    : Damir Bakiev                                                      *
 * Versão   : na                                                                *
 * Data     : XXXXX XX, 2026                                                    *
 * Website  : na                                                                *
 * Copyright: Damir Bakiev 2016-2026                                            *
 * Licença  :                                                                   *
 * Uso, modificação e distribuição estão sujeitos à Boost Software License Ver 1*
 * http://www.boost.org/LICENSE_1_0.txt                                         *
 ********************************************************************************/
#include "abstract_fileplugin.h"
#include "gc_plugin.h"
#include "gc_types.h"
#include "mainwindow.h"
#include "openglcheck.h"
#include "settingsdialog.h"
#include "shapepluginin.h"
#include "stacktrace_and_logs.h"
#include "tool_database.h"
#include "version.h"

#include <QCommandLineParser>
#include <QDir>
#include <QPluginLoader>
#include <QStandardPaths>
#include <QSystemSemaphore>
#include <QThread>
#include <QTimer>

int main(int argc, char* argv[]) {
    stacktraceAndOutput();

    QApplication::setAttribute(Qt::AA_Use96Dpi);

    qputenv("QT_ENABLE_HIGHDPI_SCALING", "0"_ba);

    // qputenv("QT_QPA_PLATFORM", "windows:darkmode=1"_ba); // 1|2

    Q_INIT_RESOURCE(resources);

    // Nomes necessários antes do QApplication: o OpenGlCheck busca o QSettings
    // e o diretório para o marcador de inicialização incompleta através deles.
    QApplication::setApplicationName(u"GGEasy"_s);
    QApplication::setOrganizationName(VER_COMPANYNAME_STR);
    QApplication::setApplicationVersion(VER_PRODUCTVERSION_STR);

    // Seleção do modo OpenGL. Apenas antes do QApplication: os atributos
    // Qt::AA_Use*OpenGL e QT_OPENGL são lidos durante sua construção.
    OpenGlCheck::selectMode(argc, argv);

    QApplication app{argc, argv};

    // #ifdef Q_OS_WIN
    // QSettings settings{u"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"_s, QSettings::NativeFormat};
    // if (settings.value(u"AppsUseLightTheme"_s) == 0) {
    // qApp->setStyleSheet(u"QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }"_s);
    // }
    // #endif

    [[maybe_unused]] App appSingleton;
    [[maybe_unused]] GCode::Settings gcSingleton;
    App::gcSettings.set(&gcSingleton);
    App::settingsPath() = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).constFirst();
    App::toolHolder().readTools();

    if(QDir dir(App::settingsPath()); !dir.exists())
        dir.mkpath(App::settingsPath());

    // Verificação do driver em segundo plano -- antes do primeiro QOpenGLWidget:
    // ele já é criado no construtor do GraphicsView dentro do MainWindow.
    OpenGlCheck::probe();
    // QSettings::setDefaultFormat(QSettings::IniFormat);
    // QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, {});

    // Configuração de amostras GL
    // QGLFormat glf = QGLFormat::defaultFormat();
    // glf.setSampleBuffers(true);
    // glf.setSamples(16);
    // QGLFormat::setDefaultFormat(glf);

    if constexpr(0) {
        QSystemSemaphore semaphore{u"GGEasySemaphore"_s, 1}; // Cria o semáforo
        semaphore.acquire();                                   // Eleva o semáforo para impedir que outras instâncias acessem a memória compartilhada
#ifdef Q_OS_UNIX
        // No Linux/Unix a memória compartilhada não é liberada em caso de encerramento anormal do aplicativo,
        // por isso é necessário limpar estes resíduos
        QSharedMemory nix_fix_shared_memory{u"GGEasyMemory"_s};
        if(nix_fix_shared_memory.attach())
            nix_fix_shared_memory.detach();
#endif
        // MainWindow* mainWin = nullptr;
        QSharedMemory sharedMemory{u"GGEasyMemory"_s}; // Cria a instância de memória compartilhada
        auto instance = [&sharedMemory]() -> MainWindow*& { return *static_cast<MainWindow**>(sharedMemory.data()); };
        bool is_running{};          // Variável para verificar se o aplicativo já está em execução
        if(sharedMemory.attach()) { // Tenta anexar a instância de memória compartilhada a um segmento existente
            is_running = true;      // Se bem-sucedido, determina que já existe uma instância rodando
        } else {
            sharedMemory.create(sizeof(void*)); // Caso contrário, aloca o tamanho de um ponteiro de memória
            is_running = false;                 // E determina que nenhuma outra instância está rodando
        }
        semaphore.release(); // Libera o semáforo
        QCommandLineParser parser;
        parser.addPositionalArgument(u"url"_s, u"Url do arquivo a ser aberto"_s);
        parser.process(app);
        if(is_running) {
            system("pause");
            if(parser.positionalArguments().length())
                for(const QString& fileName: parser.positionalArguments())
                    instance()->loadFile(fileName);
            return 1;
        } else {
        }
    }

    { // Tela de Apresentação (Splash Screen)
        QPixmap pixmap{u":/256.png"_s};
        if(!pixmap.isNull()) {
            pixmap = pixmap.scaled(600, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        auto splash = new QSplashScreen{pixmap};
        splash->setAttribute(Qt::WA_DeleteOnClose);
        splash->show();
        splash->connect(splash, &QObject::destroyed, splash, [] { App::splashScreen.set(nullptr); });
        App::splashScreen.set(splash);
    }

    { // Idioma e Tradução
        QSettings settings;
        settings.beginGroup(u"MainWindow"_s);
        QString locale(settings.value(u"locale"_s).toString());
        if(locale.isEmpty())
            locale = QLocale().name().left(2);
        settings.setValue(u"locale"_s, locale);
        settings.endGroup();
        MainWindow::translate(locale);
    }

    if(0) return ToolDatabase{}.exec();

    // Instâncias dos plugins são removidas AQUI e não pelo destrutor estático do Qt
    // (QLibraryStore::cleanup durante __cxa_finalize): este é disparado após a
    // destruição do QApplication, o que causaria crash nos destrutores das janelas/editores dos plugins.
    // Declarado antes da mainWin para ser destruído após ela.
    struct PluginHolder {
        std::vector<std::unique_ptr<QPluginLoader>> loaders;
        ~PluginHolder() {
            for(auto& loader: loaders)
                if(loader->isLoaded()) delete loader->instance();
        }
    } pluginHolder;
    auto& loaders = pluginHolder.loaders;

    MainWindow mainWin;
    mainWin.setObjectName(u"MainWindow"_s);

    /*
    Plataforma       Sufixos válidos
    Windows          .dll, .DLL
    Unix/Linux       .so
    AIX              .a
    HP-UX            .sl, .so (HP-UXi)
    macOS e iOS      .dylib, .bundle, .so
    */
#ifdef Q_OS_UNIX
    #ifdef QT_DEBUG
    const QString suffix{u"*.so"_s};
    #else
    const QString suffix{u"*.so"_s};
    #endif
#elif defined(Q_OS_WIN)
    const auto suffix = u"*.dll"_s;
#else
    static_assert(false, u"Selecione o Sistema Operacional"_s);
#endif

    // Carregamento de plugins
    QDir dir(QApplication::applicationDirPath() + u"/plugins"_s);
    if(dir.exists()) { // Busca todos os arquivos no diretório u"plugins"_s
        QStringList listFiles(dir.entryList(QStringList(suffix), QDir::Files));
        auto text = QObject::tr("Carregando plugin %1\n\n\n");
        for(const auto& str: listFiles) { // Itera sobre todos os arquivos
            App::splashScreen().showMessage(text.arg(str),
                Qt::AlignBottom | Qt::AlignHCenter, Qt::white);
            loaders.emplace_back(std::make_unique<QPluginLoader>(dir.absolutePath() + u"/"_s + str));
            if(auto* pobj = loaders.back()->instance(); pobj) { // Carrega o plugin
                PluginData* pd{};
                if(auto* gCode = qobject_cast<GCode::Plugin*>(pobj)) {
                    pd = App::gCodePlugins()
                             .try_emplace(gCode->type(), gCode)
                             .first->second;
                } else if(auto* file = qobject_cast<AbstractFilePlugin*>(pobj)) {
                    pd = App::filePlugins()
                             .try_emplace(file->type(), file)
                             .first->second;
                } else if(auto* shape = qobject_cast<Shapes::Plugin*>(pobj)) {
                    pd = App::shapePlugins()
                             .try_emplace(shape->type(), shape)
                             .first->second;
                } else
                    continue;
                pd->setInfo(loaders.back()
                        ->metaData()
                        .value(u"MetaData"_s)
                        .toObject());
            } else {
                qDebug() << str << loaders.back()->errorString();
                loaders.pop_back();
            }
        }
    }

    SettingsDialog();
    MainWindow::updateTheme();
    mainWin.init(); // Conecta os plugins

    QCommandLineParser parser;
    parser.addPositionalArgument(u"url"_s, u"Url do arquivo a ser aberto"_s);
    // O valor é lido pelo OpenGlCheck::selectMode antes do QApplication; a opção é
    // declarada aqui para que o process() não a rejeite como desconhecida e ela apareça no --help.
    parser.addOption(OpenGlCheck::commandLineOption());
    parser.process(app);
    std::vector<std::string_view> cli{
        std::from_range, std::span{++argv, static_cast<size_t>(--argc)}
    };
    mainWin.cli(cli);

    for(const QString& fileName: parser.positionalArguments())
        mainWin.loadFile(fileName);

    mainWin.show();

    // Mantém a tela de apresentação (splash) visível por pelo menos 2.5 segundos
    qApp->processEvents();
    QThread::msleep(2500);
    App::splashScreen().finish(&mainWin);

    // Registro do status da placa de vídeo e OpenGL
    OpenGlCheck::logStatus();

    // O marcador de inicialização bem-sucedida é removido com atraso de 5 segundos
    QTimer::singleShot(5000, &mainWin, [] { OpenGlCheck::markStartupSucceeded(); });

    int retCode = app.exec();

    // Os plugins NÃO são descarregados na saída: na cena e modelos ainda existem
    // objetos apontando para a vtable das bibliotecas (.so), e invocar dlclose
    // antes de sua destruição causaria uma falha de segmentação (crash).
    // O sistema operacional liberará as bibliotecas ao encerrar o processo.

    return retCode;
}
