//
// Created by serket on 6/17/23.
//

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
    auto layout = new QVBoxLayout();
    setLayout(layout);
    layout->addWidget(notepad);
}


NoteView::NoteView(Notepad::Note note, QWidget* parent)
    : QWidget(parent), m_note(note)
{
    m_textEdit = new QTextEdit(this);
    connect(m_textEdit, &QTextEdit::textChanged, this, &NoteView::onTextChanged);
    m_title = new QLabel();
    m_title->setText(QString::fromStdString(m_note.title));
    m_textEdit->setText(QString::fromStdString(note.text));
    auto layout = new QVBoxLayout(parent);
    setLayout(layout);
    layout->addWidget(m_title);
    layout->addWidget(m_textEdit);
}

NoteView::~NoteView()
{

}

void NoteView::onTextChanged()
{
    QSize size = m_textEdit->document()->size().toSize();
    m_textEdit->setFixedHeight( std::max(50, size.height() + 3 ));

    m_note.text = m_textEdit->toPlainText().toStdString();

    emit textUpdated(m_note, m_textEdit->toPlainText());
}

NotepadView::NotepadView(QWidget* parent)
    : QWidget(parent)
{
    UIContext::registerNotification(this);

    m_textEdit = new QTextEdit(this);
    connect(m_textEdit, &QTextEdit::textChanged, this, &NotepadView::onTextChanged);
    m_title = new QLabel();
    auto layout = new QVBoxLayout(parent);
    setLayout(layout);
    layout->addWidget(m_title);
    layout->addWidget(m_textEdit);
    layout->addStretch(1);
    m_title->setText("Init");
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
        layout()->removeWidget(m_subnoteFrame);
        m_subnoteFrame->setVisible(false);
        m_subnoteFrame->deleteLater();
        m_subnoteFrame = nullptr;
    }

    m_subnoteFrame = new QFrame(this);
    m_subnoteFrame->setLayout(new QVBoxLayout());
    layout()->removeItem(layout()->itemAt(layout()->count()-1));
    layout()->addWidget(m_subnoteFrame);
    qobject_cast<QVBoxLayout*>(layout())->addStretch(1);
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
            connect(noteView, &NoteView::textUpdated, this,
                    [this](const Notepad::Note note, const QString string)
                {
                    BNLogInfo("%s %llx %s s: %s", note.title.c_str(), note.address, note.text.c_str(), string.toStdString().c_str());
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
    if (m_subnoteFrame && m_tempNoteWidget) {
        m_subnoteFrame->layout()->removeWidget(m_tempNoteWidget);
        m_tempNoteWidget->setVisible(false);
        m_tempNoteWidget->deleteLater();
        m_tempNoteWidget = nullptr;
    }
    if (!m_subnoteFrame)
    {
        m_subnoteFrame = new QFrame(this);
        layout()->removeItem(layout()->itemAt(layout()->count()-1));
        layout()->addWidget(m_subnoteFrame);
        qobject_cast<QVBoxLayout*>(layout())->addStretch(1);
        m_subnoteFrame->setLayout(new QVBoxLayout());
    }
    Notepad pad = Notepad();
    if (auto meta = m_activeData->QueryMetadata(NotepadMetadataKey))
        pad.LoadFromMetadata(meta);
    bool needsTempFunctionNote = false;
    bool needsTempAddressNote = false;
    if (location.getFunction())
    {
        if (!pad.GetNote(location.getFunction()).has_value())
            needsTempFunctionNote = true;
    }
    else if (location.getOffset())
    {
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
        m_subnoteFrame->layout()->addWidget(m_tempNoteWidget);
        connect(m_tempNoteWidget, &NoteView::textUpdated, this,
                [this](const Notepad::Note note, const QString string)
                {
                    BNLogInfo("%s %llx %s s: %s", note.title.c_str(), note.address, note.text.c_str(), string.toStdString().c_str());
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
        m_subnoteFrame->layout()->addWidget(m_tempNoteWidget);
        connect(m_tempNoteWidget, &NoteView::textUpdated, this,
                [this](const Notepad::Note note, const QString string)
                {
                    BNLogInfo("%s %llx %s s: %s", note.title.c_str(), note.address, note.text.c_str(), string.toStdString().c_str());
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
            layout()->removeWidget(m_subnoteFrame);
            m_subnoteFrame->deleteLater();
            m_subnoteFrame = nullptr;
        }
    }
}

