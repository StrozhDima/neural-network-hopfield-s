#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QContextMenuEvent>
#include <QLabel>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool loadFile(const QString &);

protected:
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *event) override;
#endif // QT_NO_CONTEXTMENU

private:
    Ui::MainWindow *ui;
    QMenu *fileMenu;
    QMenu *actionMenu;
    QMenu *helpMenu;
    QAction *findFormAct;
    QAction *openAct;
    QAction *closeAct;
    QAction *exitAct;
    QAction *aboutAct;
    QAction *addFormAct;
    QAction *deleteAllFormsAct;
    QAction *showFormsAct;
    QImage image;
    QLabel *imageLabel;
    QVector<QVector<int>> all_forms;
    QVector<QVector<double>> matrix;

    void createActions();
    void createMenus();
    void setImage(const QImage &newImage);
    QVector<int> imageToArray(const QImage &newImage);
    void addForm(QVector<int> form, QVector<QVector<int>> &forms);
    void learning(QVector<QVector<int>> &forms, QVector<QVector<double>> &w);
    bool saveFormsToFile(QVector<QVector<int>> weightMatrix);
    QVector<int> findForm(QVector<int> a, QVector<QVector<double>> &w, QVector<QVector<int>> &forms);
    QImage arrayToImage(QVector<int> pixels);

private slots:
    void menu_open();
    void menu_close();
    void menu_about();
    void menu_findForm();
    void menu_addForm();
    void menu_deleteAllForms();
    void menu_showForms();
};

#endif // MAINWINDOW_H
