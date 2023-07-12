//
// Created by serket on 6/17/23.
//
#include <binaryninjaapi.h>
#include <QTextEdit>
#include <qscrollarea.h>
#include "binaryninja-api/ui/uitypes.h"
#include "binaryninja-api/ui/uicontext.h"
#include "binaryninja-api/ui/globalarea.h"
#include "Notepad.h"

using namespace BinaryNinja;

#ifndef BNNOTEPAD_NOTEPADUI_H
#define BNNOTEPAD_NOTEPADUI_H

class Notepad;
class GlobalNotepadAreaWidget;

class NotepadNotifications;

static NotepadNotifications* m_instance;

class NotepadNotifications : public UIContextNotification {
public:
    std::unordered_map<UIContext*, GlobalNotepadAreaWidget*> m_widgetForCtx;

public:
    virtual void OnContextOpen(UIContext* context) override;
    virtual void OnContextClose(UIContext* context) override;
    static void init();
};

class NoteView : public QWidget
{
    Q_OBJECT

    Notepad::Note m_note;
    QLabel* m_title;
    QTextEdit* m_textEdit;

public:
    NoteView(Notepad::Note note, QWidget* parent = nullptr);
    ~NoteView();

signals:
    void textUpdated(const Notepad::Note note, const QString string);

public slots:
    /// Resize logic and emits textUpdated.
    void onTextChanged();
    void resizeForText();
};

/*!
 * Mostly same as NoteView but this one contains NoteViews, this one represents the entire file.
 *
 * */
class NotepadView : public QWidget, public UIContextNotification
{
    Q_OBJECT

    BinaryViewRef m_activeData;
    QScrollArea* m_scrollArea;
    QWidget* m_scrollAreaWidget;
    QFrame* m_subnoteFrame = nullptr;
    NoteView* m_tempNoteWidget = nullptr;
    QLabel* m_title = nullptr;
    QTextEdit* m_textEdit = nullptr;

    std::unordered_map<uint64_t, NoteView*> m_noteViews;
public:

    NotepadView(QWidget* parent = nullptr);
    ~NotepadView();

    virtual void OnViewChange(UIContext *context, ViewFrame *frame, const QString &type);
    virtual void OnAddressChange(UIContext *context, ViewFrame *frame, View *view, const ViewLocation &location);

    void loadNotes();

signals:
    void resizeDisplayedNotes();

public slots:
    void onTextChanged();
};

class GlobalNotepadAreaWidget : public GlobalAreaWidget
{
    Q_OBJECT

    ViewFrame* m_currentFrame;
    QHash<ViewFrame*, NotepadView*> m_widgetMap;

    NotepadView* currentWidget() const;

    void freeWidgetForView(QObject*);

public:
    GlobalNotepadAreaWidget(const QString& title);
};

#endif //BNNOTEPAD_NOTEPADUI_H
