//
// Created by serket on 6/17/23.
//

#include <QScrollArea>
#include <QScrollBar>
#include <libkbinja/hex.h>
#include "NotepadUI.h"

void NotepadNotifications::init()
{
    m_instance = new NotepadNotifications;
    UIContext::registerNotification(m_instance);
}

void NotepadNotifications::OnContextOpen(UIContext* context)
{
    auto globalArea = context->globalArea();
    if (!globalArea)
        return;
    auto widget = new GlobalNotepadAreaWidget("Notepad");
    m_widgetForCtx[context] = widget;
    globalArea->addWidget(widget);
}

void NotepadNotifications::OnContextClose(UIContext* context)
{
    delete m_widgetForCtx[context];
}

GlobalNotepadAreaWidget::GlobalNotepadAreaWidget(const QString& title)
    : GlobalAreaWidget(title)
{
    auto notepad = new NotepadView(this);
    auto layout = new QVBoxLayout(this);
    layout->addWidget(notepad);
}


NoteView::NoteView(Notepad::Note note, QWidget* parent)
    : QWidget(parent), m_note(note)
{
    setObjectName("#noteView");
    m_textEdit = new QTextEdit(this);
    connect(m_textEdit, &QTextEdit::textChanged, this, &NoteView::onTextChanged);
    m_title = new QLabel();
    m_title->setText(QString::fromStdString(m_note.title));
    m_title->setStyleSheet("font-weight: bold; color: white; background-color: transparent");
    m_textEdit->setText(QString::fromStdString(note.text));
    m_textEdit->setStyleSheet("background-color: transparent;");
    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_title);
    layout->addWidget(m_textEdit);
    setAttribute(Qt::WA_StyledBackground);
    setStyleSheet("background-color: #10ffffff; border-radius: 5px");
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    QSize size = m_textEdit->document()->size().toSize();
    m_textEdit->setMinimumHeight( std::max(50, size.height() + 3 ));
}

NoteView::~NoteView()
{

}


void NoteView::resizeForText()
{
    QSize size = m_textEdit->document()->size().toSize();
    m_textEdit->setMinimumHeight( std::max(50, size.height() + 3 ) + 40);
    setMinimumHeight(std::max(50, size.height() + 3 ) + 40);
}

void NoteView::onTextChanged()
{
    QSize size = m_textEdit->document()->size().toSize();
    m_textEdit->setMinimumHeight( std::max(50, size.height() + 3 ) + 40);
    setMinimumHeight(std::max(50, size.height() + 3 ) + 40);

    m_note.text = m_textEdit->toPlainText().toStdString();

    emit textUpdated(m_note, m_textEdit->toPlainText());
}

NotepadView::NotepadView(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("notepadView");
    UIContext::registerNotification(this);
    QHBoxLayout* lyt = new QHBoxLayout(this);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    lyt->addWidget(m_scrollArea);

    m_scrollAreaWidget = new QWidget(m_scrollArea);
    m_scrollAreaWidget->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));

    m_textEdit = new QTextEdit(m_scrollArea);
    connect(m_textEdit, &QTextEdit::textChanged, this, &NotepadView::onTextChanged);
    m_title = new QLabel();
    m_title->setStyleSheet("font-weight: bold; background-color: transparent;");
    m_textEdit->setStyleSheet("background-color: transparent;");

    auto layout = new QVBoxLayout(m_scrollAreaWidget);
    layout->addWidget(m_title);
    layout->addWidget(m_textEdit);
    layout->addStretch(1);
    m_title->setText("Init");
    setStyleSheet("background-color: #10ffffff;"
                  "border-radius: 5px;");

    // MUST BE LAST
    m_scrollArea->setWidget(m_scrollAreaWidget);
    m_scrollArea->setWidgetResizable(true);
}

NotepadView::~NotepadView()
{
    UIContext::unregisterNotification(this);
}


void NotepadView::onTextChanged()
{
    QSize size = m_textEdit->document()->size().toSize();
    m_textEdit->setFixedHeight( std::max(50, size.height() + 3 ));

    if (!m_activeData)
        return;

    Notepad pad = Notepad();
    if (auto meta = m_activeData->QueryMetadata(NotepadMetadataKey))
        pad.LoadFromMetadata(meta);
    pad.SetGlobalNoteText(m_textEdit->document()->toPlainText().toStdString());
    m_activeData->StoreMetadata(NotepadMetadataKey, pad.AsMetadata());
}


void NotepadView::loadNotes()
{
    if (m_subnoteFrame) {
        m_scrollAreaWidget->layout()->removeWidget(m_subnoteFrame);
        m_subnoteFrame->setVisible(false);
        m_subnoteFrame->deleteLater();
        m_subnoteFrame = nullptr;
    }

    m_subnoteFrame = new QFrame(this);
    m_subnoteFrame->setStyleSheet("background-color: transparent;");
    new QVBoxLayout(m_subnoteFrame);
    m_scrollAreaWidget->layout()->removeItem(m_scrollAreaWidget->layout()->itemAt(m_scrollAreaWidget->layout()->count()-1));
    m_scrollAreaWidget->layout()->addWidget(m_subnoteFrame);
    qobject_cast<QVBoxLayout*>(m_scrollAreaWidget->layout())->addStretch(1);
    if (m_tempNoteWidget)
    {
        m_subnoteFrame->layout()->addWidget(m_tempNoteWidget);
    }

    Notepad pad = Notepad();
    if (auto meta = m_activeData->QueryMetadata(NotepadMetadataKey))
    {
        pad.LoadFromMetadata(meta);
        for (const auto& _note : pad.GetNotes(m_activeData))
        {
            if (_note.type == Notepad::NoteType::FullFileNote)
                continue;
            auto noteView = new NoteView(_note, m_subnoteFrame);
            m_noteViews[_note.address] = noteView;
            connect(this, &NotepadView::resizeDisplayedNotes, noteView, &NoteView::resizeForText);
            connect(noteView, &NoteView::textUpdated, this,
                    [this](const Notepad::Note note, const QString string)
                {
                    Notepad pad = Notepad();
                    if (auto meta = m_activeData->QueryMetadata(NotepadMetadataKey))
                        pad.LoadFromMetadata(meta);
                    if (note.type == Notepad::AddressNote)
                        pad.SetNoteText(note.address, string.toStdString());
                    else if (note.type == Notepad::FunctionNote)
                    {
                        auto funcs = m_activeData->GetAnalysisFunctionsForAddress(note.address);
                        Ref<Function> func;
                        if (!funcs.empty())
                            func = funcs[0];
                        if (func)
                            pad.SetNoteText(func, string.toStdString());
                    }
                    m_activeData->StoreMetadata(NotepadMetadataKey, pad.AsMetadata());
                });
            m_subnoteFrame->layout()->addWidget(noteView);
        }
    }
    repaint();
}

void NotepadView::OnAddressChange(UIContext *context, ViewFrame *frame, View *view, const ViewLocation &location)
{
    uint64_t address;
    if (!m_activeData)
    {
        // Takes dorky code to get here but this is unfortunately a valid possible state
        if (m_subnoteFrame && m_tempNoteWidget) {
            // Go ahead and clear stuff bc we're in a weird spot
            m_subnoteFrame->layout()->removeWidget(m_tempNoteWidget);
            m_tempNoteWidget->setVisible(false);
            m_tempNoteWidget->deleteLater();
            m_tempNoteWidget = nullptr;
        }
        return;
    }
    if (m_subnoteFrame && m_tempNoteWidget) {
        m_subnoteFrame->layout()->removeWidget(m_tempNoteWidget);
        m_tempNoteWidget->setVisible(false);
        m_tempNoteWidget->deleteLater();
        m_tempNoteWidget = nullptr;
    }
    if (!m_subnoteFrame)
    {
        m_subnoteFrame = new QFrame(this);
        m_subnoteFrame->setStyleSheet("background-color: transparent;");
        m_scrollAreaWidget->layout()->removeItem(m_scrollAreaWidget->layout()->itemAt(m_scrollAreaWidget->layout()->count()-1));
        m_scrollAreaWidget->layout()->addWidget(m_subnoteFrame);
        qobject_cast<QVBoxLayout*>(m_scrollAreaWidget->layout())->addStretch(1);
        new QVBoxLayout(m_subnoteFrame);
    }
    Notepad pad = Notepad();
    if (auto meta = m_activeData->QueryMetadata(NotepadMetadataKey))
        pad.LoadFromMetadata(meta);
    bool needsTempFunctionNote = false;
    bool needsTempAddressNote = false;
    if (location.getFunction())
    {
        address = location.getFunction()->GetStart();
        if (!pad.GetNote(location.getFunction()).has_value())
            needsTempFunctionNote = true;
    }
    else if (location.getOffset())
    {
        address = location.getOffset();
        if (!pad.GetNote(m_activeData, location.getOffset()).has_value())
            needsTempAddressNote = true;
    }
    if (needsTempFunctionNote)
    {
        Notepad::Note note;
        auto symbol = location.getFunction()->GetSymbol();
        if (symbol)
            note.title = symbol->GetFullName();
        else
            note.title = "sub_" + formatAddress(location.getFunction()->GetStart());
        note.text = "";
        note.address = location.getFunction()->GetStart();
        note.type = Notepad::FunctionNote;
        m_tempNoteWidget = new NoteView(note, m_subnoteFrame);
        m_noteViews[note.address] = m_tempNoteWidget;
        connect(this, &NotepadView::resizeDisplayedNotes, m_tempNoteWidget, &NoteView::resizeForText);
        m_subnoteFrame->layout()->addWidget(m_tempNoteWidget);
        connect(m_tempNoteWidget, &NoteView::textUpdated, this,
                [this](const Notepad::Note note, const QString string)
                {
                    // BNLogInfo("%s %llx %s s: %s", note.title.c_str(), note.address, note.text.c_str(), string.toStdString().c_str());
                    Notepad pad = Notepad();
                    if (auto meta = m_activeData->QueryMetadata(NotepadMetadataKey))
                        pad.LoadFromMetadata(meta);
                    if (note.type == Notepad::AddressNote)
                        pad.SetNoteText(note.address, string.toStdString());
                    else if (note.type == Notepad::FunctionNote)
                    {
                        auto funcs = m_activeData->GetAnalysisFunctionsForAddress(note.address);
                        Ref<Function> func;
                        if (!funcs.empty())
                            func = funcs[0];
                        if (func)
                            pad.SetNoteText(func, string.toStdString());
                    }
                    m_activeData->StoreMetadata(NotepadMetadataKey, pad.AsMetadata());
                });
    }
    else if (needsTempAddressNote)
    {
        Notepad::Note note;
        auto symbol = m_activeData->GetSymbolByAddress(location.getOffset());
        if (symbol)
            note.title = symbol->GetFullName();
        else
            note.title = "0x" + formatAddress(location.getOffset());
        note.text = "";
        note.address = location.getOffset();
        note.type = Notepad::AddressNote;
        m_tempNoteWidget = new NoteView(note, m_subnoteFrame);
        m_noteViews[note.address] = m_tempNoteWidget;
        connect(this, &NotepadView::resizeDisplayedNotes, m_tempNoteWidget, &NoteView::resizeForText);
        m_subnoteFrame->layout()->addWidget(m_tempNoteWidget);
        connect(m_tempNoteWidget, &NoteView::textUpdated, this,
                [this](const Notepad::Note note, const QString string)
                {
                    Notepad pad = Notepad();
                    if (auto meta = m_activeData->QueryMetadata(NotepadMetadataKey))
                        pad.LoadFromMetadata(meta);
                    if (note.type == Notepad::AddressNote)
                        pad.SetNoteText(note.address, string.toStdString());
                    else if (note.type == Notepad::FunctionNote)
                    {
                        auto funcs = m_activeData->GetAnalysisFunctionsForAddress(note.address);
                        Ref<Function> func;
                        if (!funcs.empty())
                            func = funcs[0];
                        if (func)
                            pad.SetNoteText(func, string.toStdString());
                    }
                    m_activeData->StoreMetadata(NotepadMetadataKey, pad.AsMetadata());
                });
    }

    repaint();
    emit resizeDisplayedNotes();

    if (auto it = m_noteViews.find(address); it != m_noteViews.end())
    {
        if (auto noteView = it->second)
        {
            //m_scrollArea->verticalScrollBar()->setValue(m_scrollArea->verticalScrollBar()->value() + 10);
            int position = 0;
            position += noteView->y();
            position += noteView->parentWidget()->y();
            position += m_subnoteFrame->y();
            m_scrollArea->verticalScrollBar()->setValue(position);
            //noteView->setFocus(Qt::OtherFocusReason);
        }
    }
}

void NotepadView::OnViewChange(UIContext *context, ViewFrame *frame, const QString &type)
{
    BinaryViewRef view;
    QString name;
    if (frame) {
        view = frame->getCurrentBinaryView();
        m_activeData = view;
        name = context->GetNameForFile(frame->getFileContext());
    }
    if (view)
    {
        m_title->setText(name);
        m_textEdit->setEnabled(true);

        Notepad pad = Notepad();
        if (auto meta = m_activeData->QueryMetadata(NotepadMetadataKey))
        {
            pad.LoadFromMetadata(meta);
            m_textEdit->setText(QString::fromStdString(pad.GetGlobalNote(view).text));
            loadNotes();
        }
        else
            m_textEdit->setText("");
    }
    else
    {
        m_title->setText("No View Open");
        m_textEdit->setEnabled(false);
        m_textEdit->setText("");
        if (m_tempNoteWidget)
        {
            m_tempNoteWidget->setVisible(false);
            m_tempNoteWidget->deleteLater();
            m_tempNoteWidget = nullptr;
        }
        if (m_subnoteFrame)
        {
            m_subnoteFrame->setVisible(false);
            m_scrollAreaWidget->layout()->removeWidget(m_subnoteFrame);
            m_subnoteFrame->deleteLater();
            m_subnoteFrame = nullptr;
        }
    }
}

