#include <binaryninjaapi.h>

#ifdef UI_BUILD
#include "NotepadUI.h"
#include "binaryninja-api/ui/uitypes.h"
#endif

extern "C" {

BN_DECLARE_CORE_ABI_VERSION
BN_DECLARE_UI_ABI_VERSION
BINARYNINJAPLUGIN bool CorePluginInit() {

    return true;
}

#ifdef UI_BUILD
BINARYNINJAPLUGIN bool UIPluginInit() {

    NotepadNotifications::init();

    return true;
}
#endif

}