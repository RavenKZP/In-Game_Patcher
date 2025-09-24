#pragma once
#include "MCP.h"
#include "Utils.h"

namespace Translations {
    namespace Strings {

        namespace Font {
            inline std::string font_name = "NotoSans - Regular";
        }
        namespace SkyPrompt {
            inline std::string move = "Move Object";
            inline std::string remove = "Remove Object";
            inline std::string save = "Save Position";
            inline std::string reset = "Reset Position";

            inline std::string enter = "Enter Patch Mode";
            inline std::string exit = "Exit Patch Mode";

            inline std::string accept = "Accept";
            inline std::string cancel = "Cancel";
            inline std::string zrotate = "Z Rotate";
            inline std::string xyzrotate = "XYZ Rotate";
            inline std::string zmove = "Z Move";
            inline std::string xyzmove = "XYZ Move";
            inline std::string basic = "Basic";
            inline std::string advanced = "Advanced";
        }
        namespace MCP {
            inline std::string menu_title = "Menu Patcher";
            inline std::string BOS_file = "BOS File";
            inline std::string BOS_resolver = "BOS Conflict Resolver";
            inline std::string KID_file = "KID File";
            inline std::string log = "Log";
            inline std::string help = "Help";

            inline std::string general_options = "General Options";
            inline std::string general_options_help_text =
                "Patching Mode: enables object interaction via SkyPrompt.\n"
                "     To patch: look directly at an object or select it in Console.\n"
                "     Console selection has priority over look-at.\n"
                "To Patch Editor Markers: Use Debug-Menu to turn on markers visability\n"
                "     Than select marker you want to patch in the console";

            inline std::string patching_mode = "Patching Mode";
            inline std::string selected_ref = "Selected Ref";
            inline std::string no_selected_ref = "No Ref Selected";

            inline std::string actor = "Actor";
            inline std::string KID_support = "KID Support";
            inline std::string KID_support_help_text =
                "Add Keywords to objects directly from the game.\n"
                "Select a keyword from the dropdown or enter a custom keyword name.\n"
                "Click 'Add Keyword to Base Object' to attach the keyword to the base object.\n"
                "These edits are saved into the KID patch file whe Add Keyword is pressed.";
            inline std::string game_keywords = "Game Keywords";
            inline std::string filter_text = "Type to filter keywords...";
            inline std::string custom_keyword = "Custom Keyword";
            inline std::string add_keyword = "Add Keyword to Base Object";
            inline std::string indirect_KID = "Indirect KID Targets";

            inline std::string BOS_ignored = "(Dynamic - BOS ignored)";
            inline std::string BOS_support = "BOS Support";
            inline std::string BOS_support_help_text =
                "Change object Position, Rotation, Scale or Disable it.\n"
                "These edits are saved into the BOS patch file when Save Transform is pressed.\n"
                "Tip: You can resize this window for easier editing.";

            inline std::string step = "Step";
            inline std::string position = "Position";
            inline std::string rotation = "Rotation";
            inline std::string scale = "Scale";

            inline std::string unsaved_changes = "Unsaved Changes";
            inline std::string save_transform = "Save Transform";
            inline std::string remove_object = "Remove Object";
            inline std::string reset_transform = "Reset Transform";

            inline std::string BOS_editor = "BOS Runtime Editor";
            inline std::string BOS_editor_help_text =
                "Edit your BOS patch file directly from the game.\n"
                "You can change the BOS file or create new path file'.\n"
                "Only this session-added changes are shown here.\n"
                "Saving will append new entries into your BOS-SWAP file.\n"
                "Make sure to keep a backup of your BOS files before editing.";

            inline std::string current_file = "Current File: ";
            inline std::string select_file = "Select File";
            inline std::string create_new_file = "Create New File";
            inline std::string enter_file_name = "Enter new file name (Without";
            inline std::string new_file_name = "New File Name";

            inline std::string create = "Create";
            inline std::string cancel = "Cancel";
            inline std::string revert = "Revert";
            inline std::string remove = "Remove";
            inline std::string transform = "Transform";


            inline std::string save_BOS_text = "Save Transform: saves the current object transform to BOS.";
            inline std::string revert_BOS_text =
                "Revert: re-enable object, restores the object to its original state, remove BOS edits.";
            inline std::string remove_BOS_text = "Remove Object: hides this reference at runtime and marks it in BOS.";
            inline std::string reset_BOS_transform =
                "Reset Transform: restores the object to its original state, removing BOS edits.";
            inline std::string auto_save_text = "Auto-Save: changes are saved automatically";
            inline std::string remove_KID_text = "Remove: remove keyword from base object, remove KID edits.";

            inline std::string removed_objects = "Removed Objects";
            inline std::string transformed_objects = "Transformed Objects";

            inline std::string KID_editor = "KID Runtime Editor";
            inline std::string KID_editor_text =
                "Only this session - added keywords are shown here.\n"
                "Saving will append new entries into your KID file.\n"
                "Existing data in the file will remain unchanged.\n"
                "This tool does not overwrite or remove original entries.\n"
                "Always keep a backup of your KID files before editing.";

            inline std::string KID_entries = "KID Entries";

            inline std::string none = "None";
            inline std::string filter = "Filter";
        }
    };

    const std::string translations_folder = "Data/SKSE/Plugins/InGamePatcher/translations/";

    const std::unordered_map<std::string, std::string> languageMap = {
        {"ENGLISH", "en"}, {"GERMAN", "de"},   {"SPANISH", "es"}, {"FRENCH", "fr"},
        {"ITALIAN", "it"}, {"JAPANESE", "ja"}, {"POLISH", "pl"},  {"PORTUGUESE", "pt"},
        {"RUSSIAN", "ru"}, {"CHINESE", "zh"},  {"KOREAN", "ko"},  {"TURKISH", "tr"}};

    const std::map<std::string, std::map<std::string, std::string*>> requiredTranslations = {
        {"MCP",
         {
             {"menu_title", &Strings::MCP::menu_title},
             {"BOS_file", &Strings::MCP::BOS_file},
             {"KID_file", &Strings::MCP::KID_file},
             {"log", &Strings::MCP::log},
             {"help", &Strings::MCP::help},

             {"general_options", &Strings::MCP::general_options},
             {"general_options_help_text", &Strings::MCP::general_options_help_text},

             {"patching_mode", &Strings::MCP::patching_mode},
             {"selected_ref", &Strings::MCP::selected_ref},
             {"no_selected_ref", &Strings::MCP::no_selected_ref},

             {"actor", &Strings::MCP::actor},
             {"KID_support", &Strings::MCP::KID_support},
             {"KID_support_help_text", &Strings::MCP::KID_support_help_text},
             {"game_keywords", &Strings::MCP::game_keywords},
             {"filter_text", &Strings::MCP::filter_text},
             {"custom_keyword", &Strings::MCP::custom_keyword},
             {"add_keyword", &Strings::MCP::add_keyword},
             {"indirect_KID", &Strings::MCP::indirect_KID},

             {"BOS_ignored", &Strings::MCP::BOS_ignored},
             {"BOS_support", &Strings::MCP::BOS_support},
             {"BOS_support_help_text", &Strings::MCP::BOS_support_help_text},

             {"step", &Strings::MCP::step},
             {"position", &Strings::MCP::position},
             {"rotation", &Strings::MCP::rotation},
             {"scale", &Strings::MCP::scale},

             {"unsaved_changes", &Strings::MCP::unsaved_changes},
             {"save_transform", &Strings::MCP::save_transform},
             {"remove_object", &Strings::MCP::remove_object},
             {"reset_transform", &Strings::MCP::reset_transform},

             {"BOS_editor", &Strings::MCP::BOS_editor},
             {"BOS_editor_help_text", &Strings::MCP::BOS_editor_help_text},

             {"current_file", &Strings::MCP::current_file},
             {"select_file", &Strings::MCP::select_file},
             {"create_new_file", &Strings::MCP::create_new_file},
             {"enter_file_name", &Strings::MCP::enter_file_name},
             {"new_file_name", &Strings::MCP::new_file_name},

             {"create", &Strings::MCP::create},
             {"cancel", &Strings::MCP::cancel},
             {"revert", &Strings::MCP::revert},
             {"remove", &Strings::MCP::remove},
             {"transform", &Strings::MCP::transform},

             {"save_BOS_text", &Strings::MCP::save_BOS_text},
             {"revert_BOS_text", &Strings::MCP::revert_BOS_text},
             {"remove_BOS_text", &Strings::MCP::remove_BOS_text},
             {"reset_BOS_transform", &Strings::MCP::reset_BOS_transform},
             {"auto_save_text", &Strings::MCP::auto_save_text},
             {"remove_KID_text", &Strings::MCP::remove_KID_text},

             {"removed_objects", &Strings::MCP::removed_objects},
             {"transformed_objects", &Strings::MCP::transformed_objects},

             {"KID_editor", &Strings::MCP::KID_editor},
             {"KID_editor_text", &Strings::MCP::KID_editor_text},
             {"KID_entries", &Strings::MCP::KID_entries},

             {"none", &Strings::MCP::none},
             {"filter", &Strings::MCP::filter},
         }},
        {"SkyPrompt",
         {
             {"move", &Strings::SkyPrompt::move},
             {"remove", &Strings::SkyPrompt::remove},
             {"save", &Strings::SkyPrompt::save},
             {"reset", &Strings::SkyPrompt::reset},
             {"enter", &Strings::SkyPrompt::enter},
             {"exit", &Strings::SkyPrompt::exit},
             {"accept", &Strings::SkyPrompt::accept},
             {"cancel", &Strings::SkyPrompt::cancel},
             {"zrotate", &Strings::SkyPrompt::zrotate},
             {"xyzrotate", &Strings::SkyPrompt::xyzrotate},
             {"zmove", &Strings::SkyPrompt::zmove},
             {"xyzmove", &Strings::SkyPrompt::xyzmove},
             {"basic", &Strings::SkyPrompt::basic},
             {"advanced", &Strings::SkyPrompt::advanced},
         }}};

    std::string GetValidLanguage();

    bool LoadTranslations(const std::string& lang);

};