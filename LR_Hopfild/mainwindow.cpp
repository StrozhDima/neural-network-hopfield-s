#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QDebug"
#include "QMessageBox"
#include "QImageReader"
#include "QImageWriter"
#include "QDir"
#include "QFileDialog"
#include "QStandardPaths"
#include "QHBoxLayout"
#include "QWidget"
#include "QScreen"
#include "QThread"

MainWindow::MainWindow(QWidget *parent):QMainWindow(parent), ui(new Ui::MainWindow), imageLabel(new QLabel)
{
    ui->setupUi(this);
    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    setCentralWidget(imageLabel);
    createActions();
    createMenus();
    setWindowTitle("Hopfield's neuron network");
    setMinimumSize(500, 550);
    setMaximumSize(500, 550);
}

#ifndef QT_NO_CONTEXTMENU
void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction(addFormAct);
    menu.addAction(findFormAct);
    menu.exec(event->globalPos());
}
#endif // QT_NO_CONTEXTMENU


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createActions()
{
    openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, &QAction::triggered, this, &MainWindow::menu_open);

    closeAct = new QAction(tr("&Close..."), this);
    closeAct->setShortcuts(QKeySequence::Close);
    closeAct->setStatusTip(tr("Close the current file"));
    connect(closeAct, &QAction::triggered, this, &MainWindow::menu_close);

    exitAct = new QAction(tr("&Exit"), this);
    exitAct->setShortcut(tr("Ctrl+Q"));
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the info about the application"));
    connect(aboutAct, &QAction::triggered, this, &MainWindow::menu_about);

    findFormAct = new QAction(tr("&Find form"), this);
    findFormAct->setStatusTip(tr("Find form from memory"));
    connect(findFormAct, &QAction::triggered, this, &MainWindow::menu_findForm);

    addFormAct = new QAction(tr("&Add to memory"), this);
    addFormAct->setStatusTip(tr("Add form to memory"));
    connect(addFormAct, &QAction::triggered, this, &MainWindow::menu_addForm);

    deleteAllFormsAct = new QAction(tr("&Delete all forms"), this);
    deleteAllFormsAct->setStatusTip(tr("Delete all forms from memory"));
    connect(deleteAllFormsAct, &QAction::triggered, this, &MainWindow::menu_deleteAllForms);

    showFormsAct = new QAction(tr("&Show forms"), this);
    showFormsAct->setStatusTip(tr("Show all forms in a file"));
    connect(showFormsAct, &QAction::triggered, this, &MainWindow::menu_showForms);
}

void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openAct);
    closeAct->setEnabled(false);
    fileMenu->addAction(closeAct);
    fileMenu->addAction(exitAct);

    actionMenu = menuBar()->addMenu(tr("&Action"));
    findFormAct->setEnabled(false);
    actionMenu->addAction(findFormAct);
    addFormAct->setEnabled(false);
    actionMenu->addAction(addFormAct);
    deleteAllFormsAct->setEnabled(false);
    actionMenu->addAction(deleteAllFormsAct);
    showFormsAct->setEnabled(false);
    actionMenu->addAction(showFormsAct);

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
}

static void initializeImageFileDialog(QFileDialog &dialog, QFileDialog::AcceptMode acceptMode)
{
    static bool firstDialog = true;

    if (firstDialog) {
        firstDialog = false;
        const QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
        dialog.setDirectory(picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.last());
    }

    QStringList mimeTypeFilters;
    const QByteArrayList supportedMimeTypes = acceptMode == QFileDialog::AcceptOpen
            ? QImageReader::supportedMimeTypes() : QImageWriter::supportedMimeTypes();
    foreach (const QByteArray &mimeTypeName, supportedMimeTypes)
        mimeTypeFilters.append(mimeTypeName);
    mimeTypeFilters.sort();
    dialog.setMimeTypeFilters(mimeTypeFilters);
    dialog.selectMimeTypeFilter("image/bmp");
    if (acceptMode == QFileDialog::AcceptSave)
        dialog.setDefaultSuffix("bmp");
}

bool MainWindow::loadFile(const QString &fileName)
{
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    const QImage newImage = reader.read();
    if(newImage.isNull() || ((newImage.height() != 10) && (newImage.width() != 10)) || (newImage.depth() != 1))
    {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("Cannot open %1: %2")
                                 .arg(QDir::toNativeSeparators(fileName), reader.errorString()));
        return false;
    }

    setImage(newImage);
    setWindowFilePath(fileName);

    const QString message = tr("Size: %1x%2, Depth: %3")
            .arg(image.width()).arg(image.height()).arg(image.depth());
    statusBar()->showMessage(message);
    return true;
}

void MainWindow::setImage(const QImage &newImage)
{
    image = newImage;
    imageLabel->setPixmap(QPixmap::fromImage(image).scaled(500, 500));
    closeAct->setEnabled(true);
    addFormAct->setEnabled(true);
}

QVector<int> MainWindow::imageToArray(const QImage &newImage)
{
    QVector<int> pixels(0);

    for(int x = 0; x < newImage.width(); ++x)
    {
        for(int y = 0; y < newImage.height(); ++y)
        {
            QColor clrCurrent(newImage.pixel(y, x));

            if(clrCurrent.value() <= 0)
            {
                pixels.append(-1);
            }
            else pixels.append(1);
        }
    }
    return pixels;
}

void MainWindow::learning(QVector<QVector<int>> &forms, QVector<QVector<double>> &w) //добавить образ в матрицу весов (обучение)
{
    int n = forms.at(0).size();
    w.resize(n);
    for(int i = 0; i < n; i++)
    {
        w[i].resize(n);
    }

    for(int i = 0; i < n; i++)
    {
        for(int j = 0; j < n; j++)
        {
            if(i == j)
            {
                w[i][j] = 0;
            }
            else {
                for(int k = 0; k < forms.size(); k++)
                {
                    //заносим значения весов
                    w[i][j] += forms[k][i] * forms[k][j];
                }
            }
        }
    }
}

bool MainWindow::saveFormsToFile(QVector<QVector<int>> forms)
{
    QString filename = "weight_matrix.txt";
    QFile file(filename);
    if(file.open(QFile::ReadWrite | QFile::Text))
    {
        QTextStream out(&file);
        int i = 0;
        for(int y = 0; y < forms.size(); ++y)
        {
            for (int x = 0; x < forms.value(y).size(); ++x)
            {
                if(i%10 == 0) out << "\n";
                out << forms.value(y).value(x);
                i++;
            }
            out << "\n\n";
        }
        return true;
    }
    else {
        return false;
    }
}

void MainWindow::addForm(QVector<int> form, QVector<QVector<int>> &forms)
{
    if(!(forms.contains(form)))
    {
        forms.append(form);
        findFormAct->setEnabled(true);
        showFormsAct->setEnabled(true);
        deleteAllFormsAct->setEnabled(true);
        QMessageBox::about(this, tr("Info"), tr("Form added!"));
    }
    else {
        QMessageBox::about(this, tr("Error"), tr("This form already exists!"));
    }
}

QVector<int> MainWindow::findForm(QVector<int> a, QVector<QVector<double>> &w, QVector<QVector<int>> &forms) //поиск образа
{
    QVector<int> temp;

    int n = a.size();

    for(int k = 0; k < 10000; k++)
    {
        for(int i = 0; i < n; i++)
        {
            double x = 0;

            for(int j = 0; j < n; j++)
            {
                x += w[i][j] * a[j];
            }
            //функция активации
            if(x > 0)
            {
                temp.append(1);
            }
            else
            {
                temp.append(-1);
            }
            //конец функции активации
        }
        if(forms.contains(temp))
        {
            return temp;
        }
    }
}

QImage MainWindow::arrayToImage(QVector<int> pixels)
{
    QVector<int> temp;
    QImage newImage(10, 10, QImage::Format_Mono);
    if(!(pixels.isEmpty()))
    {
        for (int x = 0; x < pixels.size(); ++x)
        {
            if(pixels.at(x) >= 0)
            {
                temp.append(1);
            }
            else temp.append(0);
        }
        for (int h=0; h<10; ++h)
        {
            for (int w=0; w<10; ++w)
            {
                newImage.setPixel(w, h, temp.at(h*10+w));
            }
        }
        return newImage;
    }
    else {
        newImage.~QImage();
        return newImage;
    }
}

void MainWindow::menu_open()
{
    QFileDialog dialog(this, tr("Open File"));
    initializeImageFileDialog(dialog, QFileDialog::AcceptOpen);
    while (dialog.exec() == QDialog::Accepted && !loadFile(dialog.selectedFiles().first())) {}
}

void MainWindow::menu_close()
{
    image.~QImage();
    imageLabel->clear();
    closeAct->setEnabled(false);
    addFormAct->setEnabled(false);
    findFormAct->setEnabled(false);
}

void MainWindow::menu_about()
{
    QMessageBox::about(this, tr("About the programm"), tr("Лабораторная работа по предмету ЦОСиИ\nВыполнил студент группы 300541 Строж Д.А.\nБГУИР 2017 г."));
}

void MainWindow::menu_findForm()
{
    QVector<int> pixels = findForm(imageToArray(image), matrix, all_forms);
    if(!(pixels).isEmpty())
    {
        setImage(arrayToImage(pixels));
        QMessageBox::about(this, tr("Info"), tr("Find form"));
    }
    else {
        QMessageBox::about(this, tr("Info"), tr("Form don't find!"));
    }

    //показ массива в консоли
    /*QDebug debug = qDebug();
    int i = 0;
    for (int x = 0; x < pixels.size(); ++x)
    {
        if(i%10 == 0) debug << "\n";
        debug << pixels.value(x);
        i++;
    }*/
}

void MainWindow::menu_addForm()
{
    addForm(imageToArray(image), all_forms);
    learning(all_forms, matrix);

    //показ массива в консоли
    /*QDebug debug = qDebug();
    int i = 0;
    for(int y = 0; y < all_forms.size(); ++y)
    {
        for (int x = 0; x < all_forms.value(y).size(); ++x)
        {
            if(i%10 == 0) debug << "\n";
            debug << all_forms.value(y).value(x);
            i++;
        }
        debug << "\n\n";
    }*/
}

void MainWindow::menu_deleteAllForms()
{
    all_forms.clear();
    matrix.clear();
    findFormAct->setEnabled(false);
    showFormsAct->setEnabled(false);
    deleteAllFormsAct->setEnabled(false);
    QMessageBox::about(this, tr("Info"), tr("All forms has been deleted!"));
}

void MainWindow::menu_showForms()
{
    if(saveFormsToFile(all_forms))
    {
        QMessageBox::about(this, tr("Info"), tr("Form wrote to file!"));
    }
    else {
        QMessageBox::about(this, tr("Error"), tr("Form didn't write to file!"));
    }
}
