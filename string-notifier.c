/*
 * skin-notifier
 * Copyright (C) 2023 skin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Command-execute
 * Copyright (C) 2010 tymm
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#define PURPLE_PLUGINS

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "notify.h"
#include "plugin.h"
#include "version.h"
#include "debug.h"
#include "prefs.h"

#define PLUGIN_ID "core-skin-string-notifier"
/* #define STRINGNOTIFIER_PLUGIN_VERSION "0.1.0" */

#ifndef _PIDGIN_CONVERSATION_H_
typedef enum
{
        PIDGIN_UNSEEN_NONE,
        PIDGIN_UNSEEN_EVENT,
        PIDGIN_UNSEEN_NO_LOG,
        PIDGIN_UNSEEN_TEXT,
        PIDGIN_UNSEEN_NICK
} PidginUnseenState;
#endif

void execute(const char *cmd) {
       if(strcmp(cmd,"") != 0) {
               /* Execute command */
               if(g_spawn_command_line_async(cmd, NULL) == TRUE) {
                       purple_debug_info(PLUGIN_ID, "Command %s executed\n", cmd);
               } else {
                       purple_debug_warning(PLUGIN_ID, "There was a problem executing the command\n");
               }
       } else {
               purple_debug_warning(PLUGIN_ID, "No command found\n");
       }
}



static gboolean check_for(PurpleAccount *account, PurpleConversation *conv, const char *check_name, const char *checked, const char *against,
    const char *sender, char *buffer) {
    purple_debug_info(PLUGIN_ID, "begin check.\n");

    if (against == NULL || strlen(against) == 0)
    {
        purple_debug_info(PLUGIN_ID, "comma separated list is empty.\n");
        return FALSE;
    }

    const char *token = against;
    const char *until;
    char *write;
    while (*token != '\0') {
        if (*token == ',') token++;
        until = token;
        while (*until != '\0' && *until != ',') until++;

        for (write = buffer; token < until; write++) {
            *write = *token;
            token++;
        }
        *write = '\0';
        purple_debug_info(PLUGIN_ID, "check %s against %s\n", buffer, checked);
        if (strstr(checked, buffer) != NULL) {
            purple_debug_info(PLUGIN_ID, "matched: %s\n", buffer);
            return TRUE;
        }
    }
    return FALSE;
}


static gboolean check_cmd(PurpleAccount *account, PurpleConversation *conv, const char *check_name, const char *checked, const char *against,
    const char *conversation_title,
    const char *sender,
    const char *cmd) {
    char buffer[1024];
    char check_buffer[1024];
    gboolean check_result = check_for(account, conv, check_name, checked, against, sender, check_buffer);
    if (check_result) {
        size_t cmd_len = strlen(cmd);
        size_t check_name_len = strlen(check_name);
        size_t conversation_title_len = strlen(conversation_title);
        size_t sender_len = strlen(sender);
        size_t matched_len = strlen(buffer);
        size_t total_length = cmd_len +
                conversation_title_len +
                sender_len +
                check_name_len +
                matched_len + 5;
        buffer[total_length] = '\0';
        strcpy(buffer+cmd_len+conversation_title_len+sender_len+check_name_len+4, check_buffer);
        strcpy(buffer+cmd_len+conversation_title_len+sender_len+3, check_name);
        strcpy(buffer+cmd_len+conversation_title_len+2, sender);
        strcpy(buffer+cmd_len+1, conversation_title);
        strcpy(buffer, cmd);
        buffer[cmd_len] = ' ';
        buffer[cmd_len+conversation_title_len+1] = ' ';
        buffer[cmd_len+conversation_title_len+sender_len+2] = ' ';
        buffer[cmd_len+conversation_title_len+sender_len+check_name_len+3] = ' ';
        purple_debug_info(PLUGIN_ID, "executing: %s\n", buffer);
        execute(buffer);
    }
    return check_result;
}

static gboolean check_buzz(PurpleAccount *account, PurpleConversation *conv, const char *check_name, const char *checked, const char *against,
    const char *sender) {
    char buffer[1024];
    gboolean check_result = check_for(account, conv, check_name, checked, against, sender, buffer);
    if (check_result) {
        purple_debug_info(PLUGIN_ID, "presenting!\n");
        purple_conversation_present(conv);
        purple_debug_info(PLUGIN_ID, "emitting...\n");
        purple_signal_emit(purple_conversations_get_handle(), "got-attention",
            account, sender, conv, 0);
    }
    return check_result;
}



static void cmdexe_received_msg(PurpleAccount *account,
                    const char *sender,
                     const char *message,
                    PurpleConversation *conv,
                    PurpleMessageFlags flags) {

    if (conv == NULL) {
        purple_debug_info(PLUGIN_ID, "conversation is null.\n");
        return;
    }
    const char *conversation_title = purple_conversation_get_title(conv);
    if (conversation_title == NULL) {
        purple_debug_info(PLUGIN_ID, "conversation_title is null, autosetting it.\n");
        purple_conversation_autoset_title(conv);
    conversation_title = purple_conversation_get_title(conv);
    }
    if (conversation_title == NULL) {
        purple_debug_info(PLUGIN_ID, "conversation_title is null, setting it to name.\n");
    conversation_title = purple_conversation_get_name(conv);
    }
    if (conversation_title == NULL) {
        purple_debug_info(PLUGIN_ID, "conversation_title is STILL null.\n");
        return;
    }


    purple_debug_info(PLUGIN_ID, "let's do this.\n");
    const char *buzz_conversations = purple_prefs_get_string("/plugins/core/skin-string-notifier/buzz-conversations");
    const char *buzz_senders = purple_prefs_get_string("/plugins/core/skin-string-notifier/buzz-senders");
    const char *buzz_keywords = purple_prefs_get_string("/plugins/core/skin-string-notifier/buzz-keywords");
    const char *cmd = purple_prefs_get_string("/plugins/core/skin-string-notifier/command");
    const char *cmd_conversations = purple_prefs_get_string("/plugins/core/skin-string-notifier/cmd-conversations");
    const char *cmd_senders = purple_prefs_get_string("/plugins/core/skin-string-notifier/cmd-senders");
    const char *cmd_keywords = purple_prefs_get_string("/plugins/core/skin-string-notifier/cmd-keywords");

    if (cmd == NULL) {
        purple_debug_info(PLUGIN_ID, "cmd is null.\n");
        return;
    }
    /* Check buzz first, then cmd if buzz doesn't match */

    gboolean check_matched = check_buzz(account, conv, "conversations", conversation_title, buzz_conversations, sender);

    if (!check_matched)
        check_matched = check_buzz(account, conv, "senders", sender, buzz_senders, sender);
    if (!check_matched)
        check_matched = check_buzz(account, conv, "keywords", message, buzz_keywords, sender);
    if (!check_matched)
        check_matched = check_cmd(account, conv, "conversations", conversation_title, cmd_conversations, sender, conversation_title, cmd);
    if (!check_matched)
        check_matched = check_cmd(account, conv, "senders", sender, cmd_senders, sender, conversation_title, cmd);
    if (!check_matched)
        check_matched = check_cmd(account, conv, "keywords", message, cmd_keywords, sender, conversation_title, cmd);
}

static gboolean plugin_load(PurplePlugin *plugin) {
    /* Connect the received-im-msg signal to the callback function cmdexe_received_im_msg.
     * This signal is emitted everytime an IM message is received */
    purple_signal_connect(purple_conversations_get_handle(), "received-im-msg", plugin, PURPLE_CALLBACK(cmdexe_received_msg), NULL);

    /* Connect the received-chat-msg signal to the callback function cmdexe_received_chat_msg.
     * This signal is emitted everytime a Chat message is received */
    purple_signal_connect(purple_conversations_get_handle(), "received-chat-msg", plugin, PURPLE_CALLBACK(cmdexe_received_msg), NULL);
    return TRUE;
}

static gboolean plugin_unload(PurplePlugin *plugin) {
    /* Disconnect the signal if the plugin is getting unloaded */
    purple_signal_disconnect(purple_conversations_get_handle(), "received-im-msg", plugin, PURPLE_CALLBACK(cmdexe_received_msg));
    purple_signal_disconnect(purple_conversations_get_handle(), "received-chat-msg", plugin, PURPLE_CALLBACK(cmdexe_received_msg));
    return TRUE;
}

/* UI */
static PurplePluginPrefFrame *plugin_config_frame(PurplePlugin *plugin) {
    PurplePluginPrefFrame *frame;
    PurplePluginPref *ppref;

    frame = purple_plugin_pref_frame_new();

    ppref = purple_plugin_pref_new_with_label("String Notifier Settings:");
    purple_plugin_pref_frame_add(frame, ppref);
    ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/skin-string-notifier/buzz-conversations", "Buzz Conversations");
    purple_plugin_pref_frame_add(frame, ppref);
    ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/skin-string-notifier/buzz-senders", "Buzz Senders");
    purple_plugin_pref_frame_add(frame, ppref);
    ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/skin-string-notifier/buzz-keywords", "Buzz Keywords");
    purple_plugin_pref_frame_add(frame, ppref);

    ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/skin-string-notifier/command", "Command");
    purple_plugin_pref_frame_add(frame, ppref);

    ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/skin-string-notifier/cmd-conversations", "Command Conversations");
    purple_plugin_pref_frame_add(frame, ppref);
    ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/skin-string-notifier/cmd-senders", "Command Senders");
    purple_plugin_pref_frame_add(frame, ppref);
    ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/skin-string-notifier/cmd-keywords", "Command Keywords");
    purple_plugin_pref_frame_add(frame, ppref);


    return frame;
}

static PurplePluginUiInfo prefs_info = {
    plugin_config_frame,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static PurplePluginInfo info = {
    PURPLE_PLUGIN_MAGIC,
    PURPLE_MAJOR_VERSION,
    PURPLE_MINOR_VERSION,
    PURPLE_PLUGIN_STANDARD,
    NULL,
    0,
    NULL,
    PURPLE_PRIORITY_DEFAULT,
    PLUGIN_ID,
    "String Notifier",
    STRINGNOTIFIER_PLUGIN_VERSION,
    "Notifier for pidgin and finch based on string search",
    "Takes a command which will be executed either on every new IM or on every conversation message. Searches sender, conversation title, and keywords."
    "skin <me@djha.skin>",
    "https://git.sr.ht/~skin/string-notifier",
    "This is so much better",
    plugin_load,
    plugin_unload,
    NULL, // plugin_destroy
    NULL,
    NULL,
    &prefs_info,
    NULL, // plugin_actions function here
    NULL,
    NULL,
    NULL,
    NULL
};

static void init_plugin(PurplePlugin *plugin) {
    purple_prefs_add_none("/plugins/core/skin-string-notifier");
    purple_prefs_add_string("/plugins/core/skin-string-notifier/buzz-conversations", "");
    purple_prefs_add_string("/plugins/core/skin-string-notifier/buzz-senders", "");
    purple_prefs_add_string("/plugins/core/skin-string-notifier/buzz-keywords", "");
    purple_prefs_add_string("/plugins/core/skin-string-notifier/command", "");
    purple_prefs_add_string("/plugins/core/skin-string-notifier/cmd-conversations", "");
    purple_prefs_add_string("/plugins/core/skin-string-notifier/cmd-senders", "");
    purple_prefs_add_string("/plugins/core/skin-string-notifier/cmd-keywords", "");
}

PURPLE_INIT_PLUGIN(command-execute, init_plugin, info)
