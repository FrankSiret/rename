#ifndef RENAME_H
#define RENAME_H

#include <QMainWindow>
#include <QCompleter>
#include <QFileSystemModel>
#include <QDirModel>
#include <QStack>
#include <QDebug>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QListView>
#include <QMessageBox>
#include <QSettings>
#include <QTextEdit>
#include "textedit.h"

typedef QPair<QString, QString> Par;

struct Historial{
    //atributos
    int begin, end, ind, max;
    QString *hdir;
    //ctor
    Historial(int len) {
        begin=end=ind=0;
        max=len;
        hdir = new QString[len];
    }
    //metodos
    int next(int i){ return (i+1)%max; }
    int prev(int i){ return (i+max-1)%max; }
    bool full(){ return begin==next(end); }
    bool empty(){ return begin==end; }
    int size(){ return (end-begin+max)%max; }
    void dec_ind() { ind = prev(ind); }
    void inc_ind() { ind = next(ind); }
    QString get_dir() { return hdir[ind]; }
    bool ran_ind() { return (begin < end && (begin <= ind && ind < end)) || (begin > end && (ind > end || begin >= ind)); }
    bool insert(QString path){
        if(empty()) {
            hdir[ind]=path;
            end=next(ind);
            return true;
        }
        if(hdir[ind] == path) return false;
        if(full()) {
            hdir[next(begin)]=hdir[begin];
            begin=next(begin);
        }
        ind=next(ind);
        hdir[ind]=path;
        end=next(ind);
        return true;
    }
    bool insert_up(QString path){
        if(hdir[prev(ind)] == path) {
            dec_ind();
            return true;
        }
        return insert(path);
    }
};

namespace Ui {
class Rename;
}

class ListView : public QListView
{
    Q_OBJECT
public:
    explicit ListView(QWidget * parent = nullptr) : QListView(parent) { }
    QModelIndexList selectedIndexes() const Q_DECL_OVERRIDE { return QListView::selectedIndexes(); }
};

class FileSystemModel : public QFileSystemModel
{
public:
    FileSystemModel(QObject *parent = 0) : QFileSystemModel(parent) { }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (role == Qt::DisplayRole && index.column() == 0) {
            QString path  = QDir::toNativeSeparators(filePath(index));
            if (path.endsWith(QDir::separator()))
                path.chop(1);
            return path;
        }
        return QFileSystemModel::data(index, role);
    }
};

class Rename : public QMainWindow
{
    Q_OBJECT
public:
    explicit Rename(QWidget *parent = 0, QString dShell = "");
    void insert_hist(QString);
    void actualizar();
    void selected();
    void changeModel();
    void changeCompleter();
    void selectType(int x);
    QString removePatron(QString, int, int, QString);
    QString analizePatName(QString);
    QString getNumber(QPair<int, bool>, int, int);
    void resizeEvent(QResizeEvent *);
    void Logs(QString, QString, int t = 1);
    void LogsBegin(int n, int t = 1);
    void LogsEnd(int t = 1);
    ~Rename();

private slots:
    void on_treeView_clicked(const QModelIndex &index);
    void on_listView_doubleClicked(const QModelIndex &index);
    void on_pb_atras_clicked();
    void on_pb_adelante_clicked();
    void on_pb_subir_clicked();
    void on_pb_list_clicked();
    void on_pb_icon_clicked();
    void on_cb_dir_activated(const QString &arg1);
    void on_pb_aplicar_clicked();
    void on_pb_patron_clicked();
    void on_checkBox_toggled(bool checked);
    void on_listView_clicked(const QModelIndex &index);
    void on_pb_invert_clicked();
    void on_le_patron_returnPressed();
    void on_pb_selectall_clicked();
    void on_pb_deselect_clicked();
    void on_pb_select_clicked();
    void on_pb_enter_f5_clicked();
    void on_pb_eliminar_clicked();
    void on_pb_select_2_clicked();

    void on_pb_deshacer_clicked();

    void on_pb_rehacer_clicked();

private:
    Ui::Rename *ui;
    QFileSystemModel *dirmodel;
    QFileSystemModel *filemodel;
    QCompleter *completer;
    QCompleter *completercode;
    QString AppPath;
    QString fileLogs;
    QDir *dir;
    QDir *dRen;
    bool fe;
    Historial *hPath;
    int historialSize;
    int historialId;
    QMap<int,QVector<Par>> historial;
};

#endif // RENAME_H
