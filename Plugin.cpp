#include <binaryninjaapi.h>
#include <uitypes.h>
#include "NotepadUI.h"

extern "C" {

BN_DECLARE_CORE_ABI_VERSION
BN_DECLARE_UI_ABI_VERSION
BINARYNINJAPLUGIN bool CorePluginInit() {

    return true;
}

BINARYNINJAPLUGIN bool UIPluginInit() {

    NotepadNotifications::init();

    return true;
}

}