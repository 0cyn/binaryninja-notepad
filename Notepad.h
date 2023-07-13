//
// Created by serket on 6/17/23.
//
#include "libkbinja/MetadataSerializable.hpp"

#ifndef BNNOTEPAD_NOTEPAD_H
#define BNNOTEPAD_NOTEPAD_H


const std::string NotepadMetadataKey = "BNNotepad-MetadataStore";


class Notepad : public MetadataSerializable {

public:
    enum NoteType {
        FullFileNote,
        AddressNote,
        FunctionNote
    };
    struct Note {
        NoteType type;
        uint64_t address = 0;
        std::string title;
        std::string text;
        bool operator< (const Note &b) const {
            return address < b.address;
        }
    };
private:
    std::string m_fullNotes = "";
    std::unordered_map<uint64_t, std::string> m_functionNotes;
    std::unordered_map<uint64_t, std::string> m_addressNotes;
public:
    std::vector<Note> GetNotes(const Ref<BinaryView>& view);
    Note GetGlobalNote(const Ref<BinaryView>& view);
    std::optional<Note> GetNote(const Ref<Function>& func);
    std::optional<Note> GetNote(const Ref<BinaryView>& view, uint64_t addr);
    void SetGlobalNoteText(std::string text);
    void SetNoteText(const Ref<Function>& func, std::string text);
    void SetNoteText(uint64_t addr, std::string text);

    void Store() override
    {
        MSS(m_fullNotes);
        MSS(m_functionNotes);
        MSS(m_addressNotes);
    }
    void Load() override
    {
        MSL(m_fullNotes);
        MSL(m_functionNotes);
        MSL(m_addressNotes);
    }
};


#endif //BNNOTEPAD_NOTEPAD_H
