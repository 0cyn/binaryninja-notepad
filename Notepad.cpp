//
// Created by serket on 6/17/23.
//

#include "Notepad.h"

#include <utility>

std::string formatAddress(uint64_t address, uint8_t padSize)
{
    // put this in apiiiiii :(
    static const char hex[513] =
            "000102030405060708090a0b0c0d0e0f"
            "101112131415161718191a1b1c1d1e1f"
            "202122232425262728292a2b2c2d2e2f"
            "303132333435363738393a3b3c3d3e3f"
            "404142434445464748494a4b4c4d4e4f"
            "505152535455565758595a5b5c5d5e5f"
            "606162636465666768696a6b6c6d6e6f"
            "707172737475767778797a7b7c7d7e7f"
            "808182838485868788898a8b8c8d8e8f"
            "909192939495969798999a9b9c9d9e9f"
            "a0a1a2a3a4a5a6a7a8a9aaabacadaeaf"
            "b0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
            "c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"
            "d0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
            "e0e1e2e3e4e5e6e7e8e9eaebecedeeef"
            "f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff";
    char temp[16];
    char fin[17];
    int i = 0;
    for (; i < 16; i += 2)
    {
        temp[i] = hex[(address & 0xff) * 2];
        temp[i + 1] = hex[((address & 0xff) * 2) + 1];
        address = address >> 8;
        if (!address && i >= (padSize - 1))
            break;
    }
    int j = 0;
    int len = i + 1;
    for (; j < len; j += 2)
    {
        fin[j] = temp[i];
        fin[j + 1] = temp[i + 1];
        i -= 2;
    }
    fin[j] = '\0';
    // chop leading zero
    if (j > padSize && fin[0] == '0')
        return {&(fin[1])};
    return {fin};
};

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
