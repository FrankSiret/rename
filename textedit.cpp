#include "textedit.h"
#include <QCompleter>
#include <QKeyEvent>
#include <QAbstractItemView>
#include <QtDebug>
#include <QApplication>
#include <QModelIndex>
#include <QAbstractItemModel>
#include <QScrollBar>

TextEdit::TextEdit(QWidget *parent) : QTextEdit(parent), c(0)
{
    setPlainText("<@>");
}

TextEdit::~TextEdit()
{
    // code
}

void TextEdit::setCompleter(QCompleter *completer)
{
    if (c) QObject::disconnect(c, 0, this, 0);
    c = completer;
    if (!c) return;
    c->setWidget(this);
    c->setCompletionMode(QCompleter::PopupCompletion);
    c->setCaseSensitivity(Qt::CaseInsensitive);
    QObject::connect(c, SIGNAL(activated(QString)),
                     this, SLOT(insertCompletion(QString)));
}

QCompleter *TextEdit::completer() const
{
    return c;
}

void TextEdit::insertCompletion(const QString& completion)
{
    if (c->widget() != this)
        return;
    QTextCursor tc = textCursor();
    int extra = completion.length() - c->completionPrefix().length();
    tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(completion.right(extra));
    setTextCursor(tc);
}

QString TextEdit::textUnderCursor() const
{
    QTextCursor tc = textCursor();
    int t = tc.position();
    tc.select(QTextCursor::Document);
    QString r = tc.selectedText();
    QString p = "";
    for(int i=t-1; i>=0; i--) {
        if(r[i] == '<')
            return '<' + p;
        else if(r[i] == ' ')
            return p;
        else p = r[i] + p;
    }
    return p;
}

void TextEdit::focusInEvent(QFocusEvent *e)
{
    if (c) c->setWidget(this);
    QTextEdit::focusInEvent(e);
}

void TextEdit::keyPressEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_Return) {
        e->ignore();
        return;
    }

    if (c && c->popup()->isVisible()) {
        // The following keys are forwarded by the completer to the widget
       switch (e->key()) {
       case Qt::Key_Enter:
       case Qt::Key_Return:
       case Qt::Key_Escape:
       case Qt::Key_Tab:
       case Qt::Key_Backtab:
            e->ignore();
            return; // let the completer do default behavior
       default:
           break;
       }
    }

    bool isShortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_E); // CTRL+E
    if (!c || !isShortcut) // do not process the shortcut when we have a completer
        QTextEdit::keyPressEvent(e);

    const bool ctrlOrShift = e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
    if (!c || (ctrlOrShift && e->text().isEmpty()))
        return;

    bool hasModifier = (e->modifiers() != Qt::NoModifier) && !ctrlOrShift;
    QString cp = textUnderCursor();

    QStringList words;
    words << "<#>" << "<#0>" << "<#0[5]>" << "<#[5]>" << "<@>";

    bool bn = 0;
    foreach (QString a, words)
        if(cp == a.left(cp.size()))
            bn = 1;

    if (!isShortcut && (hasModifier || e->text().isEmpty() || cp.length() < 1 || !bn)) {
        c->popup()->hide();
        return;
    }

    if (cp != c->completionPrefix()) {
        c->setCompletionPrefix(cp);
        c->popup()->setCurrentIndex(c->completionModel()->index(0, 0));
    }
    QRect cr = cursorRect();
    cr.setWidth(c->popup()->sizeHintForColumn(0)
                + c->popup()->verticalScrollBar()->sizeHint().width());
    c->complete(cr); // popup it up!
}
