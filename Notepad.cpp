//
// Created by serket on 6/17/23.
//

#include "Notepad.h"

#include <algorithm>
#include <utility>

std::vector<Notepad::Note> Notepad::GetNotes(const Ref<BinaryView>& view)
{
    std::vector<Notepad::Note> notes;

    notes.push_back(GetGlobalNote(view));

    for (const auto& n : m_addressNotes)
    {
        Notepad::Note note;
        DataVariable var;
        if (Ref<Symbol> sym = view->GetSymbolByAddress(n.first))
        {
            note.title = sym->GetFullName();
        }
        else
            note.title = "0x" + formatAddress(n.first);
        note.text = n.second;
        note.type = AddressNote;
        note.address = n.first;
        notes.push_back(note);
    }

    for (const auto& n : m_functionNotes)
    {
        Notepad::Note note;
        DataVariable var;
        if (Ref<Symbol> sym = view->GetSymbolByAddress(n.first))
        {
            note.title = sym->GetFullName();
        }
        else
            note.title = "sub_" + formatAddress(n.first);
        note.text = n.second;
        note.type = FunctionNote;
        note.address = n.first;
        notes.push_back(note);
    }

    // Sort by address
    std::sort(notes.begin(), notes.end());

    return notes;
}

Notepad::Note Notepad::GetGlobalNote(const Ref<BinaryView>& view)
{
    Notepad::Note note;
    note.address = 0;
    note.text = m_fullNotes;
    note.type = FullFileNote;
    note.title = view->GetFile()->GetFilename();
    return note;
}

std::optional<Notepad::Note> Notepad::GetNote(const Ref<Function>& func)
{
    if (auto it = m_functionNotes.find(func->GetStart()); it != m_functionNotes.end())
    {
        Notepad::Note note;
        note.address = it->first;
        note.text = it->second;
        note.type = FunctionNote;
        if (Ref<Symbol> sym = func->GetView()->GetSymbolByAddress(it->first))
        {
            note.title = sym->GetFullName();
        }
        return note;
    }
    return {};
}
std::optional<Notepad::Note> Notepad::GetNote(const Ref<BinaryView>& view, uint64_t addr)
{
    if (auto it = m_functionNotes.find(addr); it != m_functionNotes.end())
    {
        Notepad::Note note;
        note.address = it->first;
        note.text = it->second;
        note.type = FunctionNote;
        if (Ref<Symbol> sym = view->GetSymbolByAddress(it->first))
        {
            note.title = sym->GetFullName();
        }
        return note;
    }
    return {};
}
void Notepad::SetGlobalNoteText(std::string text)
{
    m_fullNotes = std::move(text);
}
void Notepad::SetNoteText(const Ref<Function>& func, std::string text)
{
    m_functionNotes[func->GetStart()] = std::move(text);
}
void Notepad::SetNoteText(uint64_t addr, std::string text)
{
    m_addressNotes[addr] = std::move(text);
}
