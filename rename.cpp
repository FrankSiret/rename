#include <QtWidgets>
#include "rename.h"
#include "ui_rename.h"

#define FILES 1001
#define FOLDER 1002
#define ALL 1003

Rename::Rename(QWidget *parent, QString dShell) :
    QMainWindow(parent),
    ui(new Ui::Rename)
{
    ui->setupUi(this);

    const QString key = "HKEY_CURRENT_USER\\Software\\Classes\\Folder\\shell\\Rename Files\\command";
    QSettings settings(key, QSettings::NativeFormat);
    QString value = settings.value("Default", "").toString();
    fe = 1;
    ui->checkBox->setChecked(!value.isEmpty());

    QString sPath = "D:";
    hPath = new Historial(20);
    hPath->insert(sPath);
    ui->pb_list->setEnabled(0);
    qDebug() << "Home Path " << sPath;
    insert_hist(sPath);

    if(!dShell.isEmpty()){
        dShell.remove('"');
        dShell = QDir::toNativeSeparators(dShell);
        sPath = dShell;
        hPath->insert(sPath);
        qDebug() << "Dir Shell " << sPath;
        insert_hist(sPath);
    }
    dir = new QDir();
    dir->setFilter(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files);
    dir->setSorting(QDir::DirsFirst | QDir::IgnoreCase | QDir::Name);

    dRen = new QDir();
    dRen->setFilter(QDir::Files);

    actualizar();

    dirmodel = new QFileSystemModel(this);
    dirmodel->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs);
    QModelIndex index = dirmodel->setRootPath(sPath);

    ui->treeView->setModel(dirmodel);
    ui->treeView->resizeColumnToContents(0);
    ui->treeView->setColumnHidden(1,1);
    ui->treeView->setColumnHidden(2,1);
    ui->treeView->setColumnHidden(3,1);
    ui->treeView->expand(index);
    ui->treeView->scrollTo(index);

    filemodel = new QFileSystemModel(this);
    filemodel->setFilter(QDir::NoDotAndDotDot | QDir::AllEntries);

    ui->listView->setModel(filemodel);
    ui->listView->setRootIndex(filemodel->setRootPath(sPath));

    ui->l_newname->setToolTip("1 Enumumerar con/sin ceros delante y dado un numero inicial]\n"
                              "  <#> | <#0[5]> | <#[5]> | <#0>\n"
                              "2 Mantener nombre original del archivo\n"
                              "  <@>\n"
                              "3 Nuevo Nombre\n"
                              "e.g.:  \"<#> - <@> [FrankSiret]\"");
    ui->groupBox_7->setGeometry(0,0,600,0);
    changeModel();
    changeCompleter();
    ui->l_subdatos->setVisible(0);

    QString fileName = "conf.ini";
    QString filePath = QDir::toNativeSeparators(QApplication::applicationDirPath());
    if(filePath.endsWith(QDir::separator())) filePath.chop(1);
    AppPath = filePath;

    QSettings sett(filePath+"\\"+fileName, QSettings::IniFormat);
    QString i = sett.value("le_inicio", "").toString();
    QString f = sett.value("le_fin", "").toString();
    int c1 = sett.value("cb_ini", 0).toInt();
    int c2 = sett.value("cb_fin", 0).toInt();
    ui->le_inicio->setText(i);
    ui->le_fin->setText(f);
    ui->cb_ini->setChecked(c1);
    ui->cb_fin->setChecked(c2);

    fileLogs = AppPath + "\\logs.ini";

    historialSize = 0;
    historialId = -1;
}

void Rename::insert_hist(QString text)
{
    for(int i=0; i<ui->cb_dir->count(); i++)
        if(ui->cb_dir->itemText(i) == text) {
            ui->cb_dir->setCurrentText(text);
            return;
        }
    ui->cb_dir->addItem(text);
    ui->cb_dir->setCurrentText(text);
}

void Rename::actualizar()
{
    ui->pb_atras->setEnabled(hPath->begin != hPath->ind);
    ui->pb_adelante->setEnabled(hPath->next(hPath->ind) != hPath->end);
    ui->pb_subir->setEnabled(!hPath->get_dir().endsWith(':'));

    dir->setPath(hPath->get_dir()+"\\");
    dRen->setPath(hPath->get_dir()+"\\");

    int n = dir->count();
    ui->l_datos->setText(QString::number(n) + " element" + ((n==1)?' ':'s'));
}

void Rename::selected()
{
    //
}

void Rename::changeModel()
{
    completer = new QCompleter(this);
    completer->setMaxVisibleItems(7);

    // FileSystemModel that shows full paths
    FileSystemModel *fsModel = new FileSystemModel(completer);
    completer->setModel(fsModel);
    fsModel->setRootPath("");

    completer->setCompletionMode(QCompleter::PopupCompletion);
    ui->cb_dir->setCompleter(completer);
}

void Rename::changeCompleter()
{
    completercode = new QCompleter(this);
    completercode->setMaxVisibleItems(5);

    QStringList words;
    words << "<@>" << "<#>" << "<#0>" << "<#s>" << "<#0[5]>" << "<#[5]>" << "<#s[5]>";

    completercode->setModel(new QStringListModel(words, completercode));

    ui->l_newname->setCompleter(completercode);
}

void Rename::selectType(int x)
{
    ui->listView->selectAll();
    QModelIndexList lisp = ui->listView->selectedIndexes();
    foreach (QModelIndex index, lisp) {
        if((x == FILES && dirmodel->fileInfo(index).isDir()) ||
                (x == FOLDER && dirmodel->fileInfo(index).isFile())) {
            //qDebug() << dirmodel->fileInfo(index).fileName();
            ui->listView->selectionModel()->select(index,QItemSelectionModel::Deselect);
        }
    }
}

QString Rename::removePatron(QString n, int p, int l, QString r)
{
    if (p == -1) return n;
    QString res = n.left(p) + r + n.rightRef(n.size()-p-l);
    return res;
}

void Rename::resizeEvent(QResizeEvent *)
{
    ui->listView->setRootIndex(ui->listView->rootIndex());
}

void Rename::LogsBegin(int n, int t)
{
    QFile file(fileLogs);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text | QIODevice::Append))
        return;

    QTextStream out(&file);
    out << n << "\n";
    if(t) {
        historialSize = ++historialId;
        historial[historialId].clear();
    }
}

void Rename::Logs(QString name1, QString name2, int t)
{
    QFile file(fileLogs);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text | QIODevice::Append))
        return;

    QTextStream out(&file);
    out << QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss ap") << " \"" << name1 << "\" \"" << name2 << "\"\n";
    if(t)
        historial[historialId].push_back(Par(name1, name2));
}

void Rename::LogsEnd(int t)
{
    QFile file(fileLogs);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text | QIODevice::Append))
        return;

    QTextStream out(&file);
    out << "\n";
    if(t)
        historialSize++;
}

Rename::~Rename()
{
    delete ui;
}

void Rename::on_treeView_clicked(const QModelIndex &index)
{
    QString sPath = dirmodel->fileInfo(index).absoluteFilePath();
    if (sPath.endsWith(QDir::separator()))
        sPath.chop(1);
    ui->listView->setRootIndex(filemodel->setRootPath(sPath));
    hPath->insert(sPath);
    qDebug() << "Select " << sPath;
    insert_hist(sPath);
    actualizar();
    ui->treeView->selectAll();
}

void Rename::on_listView_doubleClicked(const QModelIndex &index)
{
    if(dirmodel->fileInfo(index).isDir()) {
        QString sPath = dirmodel->fileInfo(index).absoluteFilePath();
        sPath = QDir::toNativeSeparators(sPath);
        if (sPath.endsWith(QDir::separator()))
            sPath.chop(1);
        ui->listView->setRootIndex(filemodel->setRootPath(sPath));
        hPath->insert(sPath);
        qDebug() << "Entrar " << sPath;
        insert_hist(sPath);
    }
    actualizar();
}

void Rename::on_pb_atras_clicked()
{
    if(hPath->ran_ind() && hPath->begin != hPath->ind) {
        hPath->dec_ind();
        QString sPath = hPath->get_dir();
        ui->listView->setRootIndex(filemodel->setRootPath(sPath));
        qDebug() << "Atrás " << sPath;
        ui->cb_dir->setCurrentText(sPath);
    }
    actualizar();
}

void Rename::on_pb_adelante_clicked()
{
    if(hPath->ran_ind() && hPath->next(hPath->ind) != hPath->end) {
        hPath->inc_ind();
        QString sPath = hPath->get_dir();
        ui->listView->setRootIndex(filemodel->setRootPath(sPath));
        qDebug() << "Adelante" << sPath;
        ui->cb_dir->setCurrentText(sPath);
    }
    actualizar();
}

void Rename::on_pb_subir_clicked()
{
    QString sPath = hPath->get_dir();
    if(!sPath.endsWith(':')) {
        sPath = sPath.section("\\",0,-2);
        hPath->insert_up(sPath);
        ui->listView->setRootIndex(filemodel->setRootPath(sPath));
        qDebug() << "Subir" << sPath;
        insert_hist(sPath);
    }
    actualizar();
}

void Rename::on_pb_list_clicked()
{
    ui->listView->setViewMode(QListView::ListMode);
    ui->listView->setSpacing(0);
    ui->pb_icon->setEnabled(1);
    ui->pb_list->setEnabled(0);
}

void Rename::on_pb_icon_clicked()
{
    ui->listView->setViewMode(QListView::IconMode);
    ui->listView->setSpacing(5);
    ui->pb_icon->setEnabled(0);
    ui->pb_list->setEnabled(1);
}

void Rename::on_cb_dir_activated(const QString &arg1)
{
    QString sPath = QDir::toNativeSeparators(arg1);
    if(!QDir(sPath).exists() || QDir(sPath).isEmpty()) return;

    if (sPath.endsWith(QDir::separator()))
        sPath.chop(1);

    ui->listView->setRootIndex(filemodel->setRootPath(sPath));

    hPath->insert(sPath);
    qDebug() << "Edit" << sPath;
    insert_hist(sPath);
    actualizar();
}

QVector<QPair<int, bool>> number;

QString Rename::analizePatName(QString p)
{
    QString r; bool cero; QString dig; number.clear();
    for(int i=0; i<p.size();) {
        if(p[i] == '<') {
            i++;
            if(p[i] == '@') {
                i++; r+="*";
                if(p[i]=='>') i++;
                else return "";
            }
            else if(p[i++]=='#') {
                r+=":"; dig="1"; cero=0;
                if(p[i]=='0') { cero = 1; i++; }
                if(p[i]=='[') {
                    i++;
                    if(p[i].isDigit()||p[i]=='-') {
                        dig.clear();
                        while(p[i].isDigit()||p[i]=='-') { dig+=p[i]; i++; }
                    }
                    else return "";
                    if(p[i++]!=']') return "";
                }
                if(p[i++]=='>') number.push_back(qMakePair(dig.toInt(),cero));
                else return "";
            }
            else return "";
        }
        else if(p[i]!='\\'&&p[i]!='/'&&p[i]!=':'&&p[i]!='*'&&p[i]!='?'&&p[i]!='\"'&&p[i]!='<'&&p[i]!='>'&&p[i]!='|') {
            r+=p[i++];
        }
        else return "";
    }
    return r;
}

QString Rename::getNumber(QPair<int, bool> p, int c, int n){
    int lenn = QString::number(n).size();
    int lenp = QString::number(p.first).size();
    QString r;
    if(p.second)
        for(int i=0; i<lenn-lenp; i++){
            r+='0';
        }
    else if(p.first < 10)
        r = "0";
    number[c] = qMakePair(p.first+1,p.second);
    return r+QString::number(p.first);
}

void Rename::on_pb_aplicar_clicked()
{
    // <Enumerar CerosDelante [ NumeroInicial ] >
    // <#> | <#0[3]> | <#[4]> | <#0>  | <#s> | <#s[5]>
    // 01-n  03-0n     04-n     01-0n   1-n    3-n
    // <MantenerNombreOriginal
    // <@>
    // New_Name
    // e.g. "<@> - <#0[4]>_New_<#>"

    switch (ui->cb_sel1->currentIndex()) {
    case 1:
        selectType(FILES);
        break;
    case 2:
        selectType(FOLDER);
        break;
    case 3:
        selectType(ALL);
        break;
    default:
        break;
    }

    QString newName;
    QString patName = ui->l_newname->toPlainText();
    if(ui->cb_ini->isChecked())
        patName = ui->le_inicio->text() + patName;
     if(ui->cb_fin->isChecked())
        patName = patName + ui->le_fin->text();
    qDebug() << patName;
    QString expName = analizePatName(patName);
    qDebug() << "expName" << expName;
    if(expName.isEmpty()) return;

    qDebug() << "Items Selected";

    QModelIndexList lisp = ui->listView->selectedIndexes();
    int n = lisp.size();

    QVector<Par> vector;

    foreach (QModelIndex index, lisp) {
        QString path = dirmodel->fileInfo(index).absolutePath();
        QString name1 = dirmodel->fileInfo(index).fileName();
        int j=0;
        newName.clear();
        for(int i=0; i<expName.size(); i++){
            if(expName[i]=='*')
                newName += dirmodel->fileInfo(index).completeBaseName();
            else if(expName[i]==':'){
                newName += getNumber(number[j], j, n);
                j++;
            }
            else newName += expName[i];
        }
        QString name2 = newName + "." + dirmodel->fileInfo(index).suffix();
        dRen->rename(name1, name2);
        vector.push_back(Par(path + "/" + name1, path + "/" + name2));
    }

    int m = vector.size();
    if(m > 0) {
        LogsBegin(m);
        foreach (Par p, vector) {
            Logs(p.first, p.second);
        }
        LogsEnd();
    }

    QString fileName = "conf.ini";
    QString filePath = QDir::toNativeSeparators(QApplication::applicationDirPath());
    if(filePath.endsWith(QDir::separator())) filePath.chop(1);
    QSettings settings(filePath+"\\"+fileName, QSettings::IniFormat);
    settings.setValue("le_inicio", ui->le_inicio->text());
    settings.setValue("le_fin", ui->le_fin->text());
    if(ui->cb_ini->isChecked())
        settings.setValue("cb_ini", 1);
    else settings.setValue("cb_ini", 0);
    if(ui->cb_fin->isChecked())
        settings.setValue("cb_fin", 1);
    else settings.setValue("cb_fin", 0);
}

void Rename::on_pb_patron_clicked()
{
    switch (ui->cb_sel2->currentIndex()) {
    case 1:
        selectType(FILES);
        break;
    case 2:
        selectType(FOLDER);
        break;
    case 3:
        selectType(ALL);
        break;
    default:
        break;
    }

    QString pat = ui->le_patron->text();
    QString remp = ui->le_remp->text();

    QModelIndexList lisp = ui->listView->selectedIndexes();

    QVector<Par> vector;

    foreach (QModelIndex index, lisp) {
        QString path = dirmodel->fileInfo(index).absolutePath();
        QString name1 = dirmodel->fileInfo(index).fileName();
        if(dirmodel->fileInfo(index).isDir())
            name1 += ".";
        QRegExp rx(pat);
        int pos = 0, len;
        if ((pos = rx.indexIn(name1, pos)) != -1) {
            len = rx.matchedLength();
            QString newName = removePatron(name1, pos, len, remp);
            QString name2 = newName;
            dRen->rename(name1, name2);
            vector.push_back(Par(path + "/" + name1, path + "/" + name2));
        }
    }

    int n = vector.size();
    if(n > 0) {
        LogsBegin(n);
        foreach (Par p, vector) {
            Logs(p.first, p.second);
        }
        LogsEnd();
    }
}

void Rename::on_checkBox_toggled(bool checked)
{
    if(ui->checkBox->isChecked()) {
        const QString key = "HKEY_CURRENT_USER\\Software\\Classes";
        QString filePath = QDir::toNativeSeparators(QApplication::applicationFilePath());
        QSettings settings(key, QSettings::NativeFormat);
        settings.beginGroup("Folder");
        settings.beginGroup("shell");
        settings.beginGroup("Rename Files");
        qDebug() << settings.group();
        settings.setValue("Icon", filePath + ",0");
        settings.beginGroup("command");
        settings.setValue("Default", filePath + " \"%1\"");
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
        settings.beginGroup("Directory");
        settings.beginGroup("background");
        settings.beginGroup("shell");
        settings.beginGroup("Rename Files");
        settings.setValue("Icon", filePath + ",0");
        settings.beginGroup("command");
        settings.setValue("Default", filePath + " \"%V\"");
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
        settings.endGroup();
    }
    else {
        const QString key1 = "HKEY_CURRENT_USER\\Software\\Classes\\Folder\\shell";
        const QString key2 = "HKEY_CURRENT_USER\\Software\\Classes\\Directory\\background\\shell";
        QSettings settings1(key1, QSettings::NativeFormat);
        settings1.remove("Rename Files");
        QSettings settings2(key2, QSettings::NativeFormat);
        settings2.remove("Rename Files");
    }
}

void Rename::on_listView_clicked(const QModelIndex &index)
{
    ui->l_filename->setText(filemodel->fileName(index));
}

void Rename::on_pb_invert_clicked()
{
    QModelIndexList lisp = ui->listView->selectedIndexes();
    ui->listView->selectAll();
    foreach (QModelIndex index, lisp)
        ui->listView->selectionModel()->select(index,QItemSelectionModel::Deselect);
}

void Rename::on_le_patron_returnPressed()
{
    //ui->pb_patron
    on_pb_patron_clicked();
}

void Rename::on_pb_selectall_clicked()
{
    ui->listView->selectAll();
}

void Rename::on_pb_deselect_clicked()
{
    ui->listView->clearSelection();
}

void Rename::on_pb_select_clicked()
{
    ui->listView->selectAll();
    QModelIndexList lisp = ui->listView->selectedIndexes();
    foreach (QModelIndex index, lisp) {
        QString fn = dirmodel->fileInfo(index).fileName();
        QRegExp rx(ui->le_select->text());
        if(rx.indexIn(fn) == -1)
            ui->listView->selectionModel()->select(index,QItemSelectionModel::Deselect);
    }
}

void Rename::on_pb_enter_f5_clicked()
{
    ui->listView->update(ui->listView->rootIndex());
}

void Rename::on_pb_eliminar_clicked()
{
    QString b = QInputDialog::getText(this,"Eliminar archivos seleccionados","Escriba \"BORRAR\" para confirmar");
    if(b != "BORRAR") return;
    QModelIndexList lisp = ui->listView->selectedIndexes();
    foreach (QModelIndex index, lisp) {
        dRen->remove(dirmodel->fileInfo(index).fileName());
    }
}

void Rename::on_pb_select_2_clicked()
{
    QMessageBox::about(this, tr("About Rename Files"),
             tr("<html><head/><body>"
                "<p>Programmed and designed by:</p>"
                "<p><b>Frank Rodríguez Siret</b></p>"
                "<p>Email: <a href=\"mailto:frank.siret@gmail.com\">frank.siret@gmail.com</a></p>"
                "</body></html>"));
}

void Rename::on_pb_deshacer_clicked()
{
    if(historialSize <= 0 || historialId < 0) return;

    QVector<Par> vector;
    QVector<Par> h = historial[historialId];
    foreach (Par i, h) {
        QString n1 = i.first;
        QString n2 = i.second;
        dRen->rename(n2, n1);
        vector.push_back(Par(n2, n1));
    }
    historialId --;

    int m = vector.size();
    if(m > 0) {
        LogsBegin(m, 0);
        foreach (Par p, vector) {
            Logs(p.first, p.second, 0);
        }
        LogsEnd(0);
    }
}

void Rename::on_pb_rehacer_clicked()
{
    if(historialSize <= historialId+1) return;
    historialId ++;
    QVector<Par> vector;
    QVector<Par> h = historial[historialId];
    foreach (Par i, h) {
        QString n1 = i.first;
        QString n2 = i.second;
        dRen->rename(n1, n2);
        vector.push_back(Par(n1, n2));
    }

    int m = vector.size();
    if(m > 0) {
        LogsBegin(m, 0);
        foreach (Par p, vector) {
            Logs(p.first, p.second, 0);
        }
        LogsEnd(0);
    }
}
