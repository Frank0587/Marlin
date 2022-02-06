
/* Popup Config */

void CrealityDWINClass::Popup_Handler(PopupID popupid, bool option/*=false*/) {
  popup = last_popup = popupid;

  DEBUG_ECHOLNPGM("CrealityDWINClass::Popup_Handler (popupid=", popupid, ", option=", option, ")");

  switch (popupid) {
    case Pause:         Draw_Popup(F("Pause Print"), F(""), F(""), Popup); break;
    case Stop:          Draw_Popup(F("Stop Print"), F(""), F(""), Popup); break;
    case Resume:        Draw_Popup(F("Resume Print?"), F("Looks Like the last"), F("print was interrupted."), Popup); break;
    case ConfFilChange: Draw_Popup(F("Confirm Filament Change"), F(""), F(""), Popup); break;
    case PurgeMore:     Draw_Popup(F("Purge more filament?"), F("(Cancel to finish process)"), F(""), Popup); break;
    case SaveLevel:     Draw_Popup(F("Leveling Complete"), F("Save to EEPROM?"), F(""), Popup); break;
    case MeshSlot:      Draw_Popup(F("Mesh slot not selected"), F("(Confirm to select slot 0)"), F(""), Popup); break;
    case ETemp:         Draw_Popup(F("Nozzle is too cold"), F("Open Preheat Menu?"), F(""), Popup); break;
    case ManualProbing: Draw_Popup(F("Manual Probing"), F("(Confirm to probe)"), F("(cancel to exit)"), Popup); break;
    case Level:         Draw_Popup(F("Auto Bed Leveling"), F("Please wait until done."), F(""), Wait, ICON_AutoLeveling); break;
    case Home:          Draw_Popup(option ? F("Parking") : F("Homing"), F("Please wait until done."), F(""), Wait, ICON_BLTouch); break;
    case MoveWait:      Draw_Popup(F("Moving to Point"), F("Please wait until done."), F(""), Wait, ICON_BLTouch); break;
    case Heating:       Draw_Popup(F("Heating"), F("Please wait until done."), F(""), Wait, ICON_BLTouch); break;
    case FilLoad:       Draw_Popup(option ? F("Unloading Filament") : F("Loading Filament"), F("Please wait until done."), F(""), Wait, ICON_BLTouch); break;
    case FilChange:     Draw_Popup(F("Filament Change"), F("Please wait for prompt."), F(""), Wait, ICON_BLTouch); break;
    case TempWarn:      Draw_Popup(option ? F("Nozzle temp too low!") : F("Nozzle temp too high!"), F(""), F(""), Wait, option ? ICON_TempTooLow : ICON_TempTooHigh); break;
    case Runout:        Draw_Popup(F("Filament Runout"), F(""), F(""), Wait, ICON_BLTouch); break;
    case PIDWait:       Draw_Popup(F("PID Autotune"), F("in process"), F("Please wait until done."), Wait, ICON_BLTouch); break;
    case Resuming:      Draw_Popup(F("Resuming Print"), F("Please wait until done."), F(""), Wait, ICON_BLTouch); break;
    default: break;
  }
}

void CrealityDWINClass::Confirm_Handler(PopupID popupid) {

  DEBUG_ECHOLNPGM("CrealityDWINClass::Confirm_Handler (popupid=", popupid, ")");

  popup = popupid;
  switch (popupid) {
    case FilInsert:   Draw_Popup(F("Insert Filament"), F("Press to Continue"), F(""), Confirm); break;
    case HeaterTime:  Draw_Popup(F("Heater Timed Out"), F("Press to Reheat"), F(""), Confirm); break;
    case UserInput:   Draw_Popup(F("Waiting for Input"), F("Press to Continue"), F(""), Confirm); break;
    case LevelError:  Draw_Popup(F("Couldn't enable Leveling"), F("(Valid mesh must exist)"), F(""), Confirm); break;
    case InvalidMesh: Draw_Popup(F("Valid mesh must exist"), F("before tuning can be"), F("performed"), Confirm); break;
    default: break;
  }
}

/* Navigation and Control */

void CrealityDWINClass::Main_Menu_Control() {
  EncoderState encoder_diffState = Encoder_ReceiveAnalyze();
  if (encoder_diffState == ENCODER_DIFF_NO) return;
  if (encoder_diffState == ENCODER_DIFF_CW && selection < PAGE_COUNT - 1) {
    selection++; // Select Down
    Main_Menu_Icons();
  }
  else if (encoder_diffState == ENCODER_DIFF_CCW && selection > 0) {
    selection--; // Select Up
    Main_Menu_Icons();
  }
  else if (encoder_diffState == ENCODER_DIFF_ENTER)
    switch (selection) {
      case PAGE_PRINT: card.mount(); Draw_SD_List(); break;
      case PAGE_PREPARE: Draw_Menu(Prepare); break;
      case PAGE_CONTROL: Draw_Menu(Control); break;
      case PAGE_INFO_LEVELING: Draw_Menu(TERN(HAS_MESH, Leveling, InfoMain)); break;
    }
  DWIN_UpdateLCD();
}

void CrealityDWINClass::Menu_Control() {
  EncoderState encoder_diffState = Encoder_ReceiveAnalyze();
  if (encoder_diffState == ENCODER_DIFF_NO) return;
  if (encoder_diffState == ENCODER_DIFF_CW && selection < Get_Menu_Size(active_menu)) {
    DWIN_Draw_Rectangle(1, Color_Bg_Black, 0, MBASE(selection - scrollpos) - 18, 14, MBASE(selection - scrollpos) + 33);
    selection++; // Select Down
    if (selection > scrollpos+MROWS) {
      scrollpos++;
      DWIN_Frame_AreaMove(1, 2, MLINE, Color_Bg_Black, 0, 31, DWIN_WIDTH, 349);
      Menu_Item_Handler(active_menu, selection);
    }
    DWIN_Draw_Rectangle(1, GetColor(eeprom_settings.cursor_color, Rectangle_Color), 0, MBASE(selection - scrollpos) - 18, 14, MBASE(selection - scrollpos) + 33);
  }
  else if (encoder_diffState == ENCODER_DIFF_CCW && selection > 0) {
    DWIN_Draw_Rectangle(1, Color_Bg_Black, 0, MBASE(selection - scrollpos) - 18, 14, MBASE(selection - scrollpos) + 33);
    selection--; // Select Up
    if (selection < scrollpos) {
      scrollpos--;
      DWIN_Frame_AreaMove(1, 3, MLINE, Color_Bg_Black, 0, 31, DWIN_WIDTH, 349);
      Menu_Item_Handler(active_menu, selection);
    }
    DWIN_Draw_Rectangle(1, GetColor(eeprom_settings.cursor_color, Rectangle_Color), 0, MBASE(selection - scrollpos) - 18, 14, MBASE(selection - scrollpos) + 33);
  }
  else if (encoder_diffState == ENCODER_DIFF_ENTER)
    Menu_Item_Handler(active_menu, selection, false);
  DWIN_UpdateLCD();
}

void CrealityDWINClass::Value_Control() {
  EncoderState encoder_diffState = Encoder_ReceiveAnalyze();
  if (encoder_diffState == ENCODER_DIFF_NO) return;
  if (encoder_diffState == ENCODER_DIFF_CW)
    tempvalue += EncoderRate.encoderMoveValue;
  else if (encoder_diffState == ENCODER_DIFF_CCW)
    tempvalue -= EncoderRate.encoderMoveValue;
  else if (encoder_diffState == ENCODER_DIFF_ENTER) {
    process = Menu;
    EncoderRate.enabled = false;
    Draw_Float(tempvalue / valueunit, selection - scrollpos, false, valueunit);
    DWIN_UpdateLCD();
    if (active_menu == ZOffset && liveadjust) {
      planner.synchronize();
      current_position.z += (tempvalue / valueunit - zoffsetvalue);
      planner.buffer_line(current_position, homing_feedrate(Z_AXIS), active_extruder);
      current_position.z = 0;
      sync_plan_position();
    }
    else if (active_menu == Tune && selection == TUNE_ZOFFSET) {
      sprintf_P(cmd, PSTR("M290 Z%s"), dtostrf((tempvalue / valueunit - zoffsetvalue), 1, 3, str_1));
      gcode.process_subcommands_now(cmd);
    }
    if (TERN0(HAS_HOTEND, valuepointer == &thermalManager.temp_hotend[0].pid.Ki) || TERN0(HAS_HEATED_BED, valuepointer == &thermalManager.temp_bed.pid.Ki))
      tempvalue = scalePID_i(tempvalue);
    if (TERN0(HAS_HOTEND, valuepointer == &thermalManager.temp_hotend[0].pid.Kd) || TERN0(HAS_HEATED_BED, valuepointer == &thermalManager.temp_bed.pid.Kd))
      tempvalue = scalePID_d(tempvalue);
    switch (valuetype) {
      case 0: *(float*)valuepointer = tempvalue / valueunit; break;
      case 1: *(uint8_t*)valuepointer = tempvalue / valueunit; break;
      case 2: *(uint16_t*)valuepointer = tempvalue / valueunit; break;
      case 3: *(int16_t*)valuepointer = tempvalue / valueunit; break;
      case 4: *(uint32_t*)valuepointer = tempvalue / valueunit; break;
      case 5: *(int8_t*)valuepointer = tempvalue / valueunit; break;
    }
    switch (active_menu) {
      case Move:
        planner.synchronize();
        planner.buffer_line(current_position, manual_feedrate_mm_s[selection - 1], active_extruder);
        break;
      #if HAS_MESH
        case ManualMesh:
          planner.synchronize();
          planner.buffer_line(current_position, homing_feedrate(Z_AXIS), active_extruder);
          planner.synchronize();
          break;
        case UBLMesh:     mesh_conf.manual_move(true); break;
        case LevelManual: mesh_conf.manual_move(selection == LEVELING_M_OFFSET); break;
      #endif
    }
    if (valuepointer == &planner.flow_percentage[0])
      planner.refresh_e_factor(0);
    if (funcpointer) funcpointer();
    return;
  }
  NOLESS(tempvalue, (valuemin * valueunit));
  NOMORE(tempvalue, (valuemax * valueunit));
  Draw_Float(tempvalue / valueunit, selection - scrollpos, true, valueunit);
  DWIN_UpdateLCD();
  if (active_menu == Move && livemove) {
    *(float*)valuepointer = tempvalue / valueunit;
    planner.buffer_line(current_position, manual_feedrate_mm_s[selection - 1], active_extruder);
  }
}

void CrealityDWINClass::Option_Control() {
  EncoderState encoder_diffState = Encoder_ReceiveAnalyze();
  if (encoder_diffState == ENCODER_DIFF_NO) return;
  if (encoder_diffState == ENCODER_DIFF_CW)
    tempvalue += EncoderRate.encoderMoveValue;
  else if (encoder_diffState == ENCODER_DIFF_CCW)
    tempvalue -= EncoderRate.encoderMoveValue;
  else if (encoder_diffState == ENCODER_DIFF_ENTER) {
    process = Menu;
    EncoderRate.enabled = false;
    if (valuepointer == &color_names) {
      switch (selection) {
        case COLORSETTINGS_CURSOR: eeprom_settings.cursor_color = tempvalue; break;
        case COLORSETTINGS_SPLIT_LINE: eeprom_settings.menu_split_line = tempvalue; break;
        case COLORSETTINGS_MENU_TOP_BG: eeprom_settings.menu_top_bg = tempvalue; break;
        case COLORSETTINGS_MENU_TOP_TXT: eeprom_settings.menu_top_txt = tempvalue; break;
        case COLORSETTINGS_HIGHLIGHT_BORDER: eeprom_settings.highlight_box = tempvalue; break;
        case COLORSETTINGS_PROGRESS_PERCENT: eeprom_settings.progress_percent = tempvalue; break;
        case COLORSETTINGS_PROGRESS_TIME: eeprom_settings.progress_time = tempvalue; break;
        case COLORSETTINGS_PROGRESS_STATUS_BAR: eeprom_settings.status_bar_text = tempvalue; break;
        case COLORSETTINGS_PROGRESS_STATUS_AREA: eeprom_settings.status_area_text = tempvalue; break;
        case COLORSETTINGS_PROGRESS_COORDINATES: eeprom_settings.coordinates_text = tempvalue; break;
        case COLORSETTINGS_PROGRESS_COORDINATES_LINE: eeprom_settings.coordinates_split_line = tempvalue; break;
      }
      Redraw_Screen();
    }
    else if (valuepointer == &preheat_modes)
      preheatmode = tempvalue;

    Draw_Option(tempvalue, static_cast<const char * const *>(valuepointer), selection - scrollpos, false, (valuepointer == &color_names));
    DWIN_UpdateLCD();
    return;
  }
  NOLESS(tempvalue, valuemin);
  NOMORE(tempvalue, valuemax);
  Draw_Option(tempvalue, static_cast<const char * const *>(valuepointer), selection - scrollpos, true);
  DWIN_UpdateLCD();
}

void CrealityDWINClass::File_Control() {
  EncoderState encoder_diffState = Encoder_ReceiveAnalyze();
  static uint8_t filescrl = 0;
  if (encoder_diffState == ENCODER_DIFF_NO) {
    if (selection > 0) {
      card.getfilename_sorted(SD_ORDER(selection - 1, card.get_num_Files()));
      char * const filename = card.longest_filename();
      size_t len = strlen(filename);
      int8_t pos = len;
      if (!card.flag.filenameIsDir)
        while (pos && filename[pos] != '.') pos--;
      if (pos > MENU_CHAR_LIMIT) {
        static millis_t time = 0;
        if (PENDING(millis(), time)) return;
        time = millis() + 200;
        pos -= filescrl;
        len = _MIN((size_t)pos, (size_t)MENU_CHAR_LIMIT);
        char name[len + 1];
        if (pos >= 0) {
          LOOP_L_N(i, len) name[i] = filename[i + filescrl];
        }
        else {
          LOOP_L_N(i, MENU_CHAR_LIMIT + pos) name[i] = ' ';
          LOOP_S_L_N(i, MENU_CHAR_LIMIT + pos, MENU_CHAR_LIMIT) name[i] = filename[i - (MENU_CHAR_LIMIT + pos)];
        }
        name[len] = '\0';
        DWIN_Draw_Rectangle(1, Color_Bg_Black, LBLX, MBASE(selection - scrollpos) - 14, 271, MBASE(selection - scrollpos) + 28);
        Draw_Menu_Item(selection - scrollpos, card.flag.filenameIsDir ? ICON_More : ICON_File, name);
        if (-pos >= MENU_CHAR_LIMIT) filescrl = 0;
        filescrl++;
        DWIN_UpdateLCD();
      }
    }
    return;
  }
  if (encoder_diffState == ENCODER_DIFF_CW && selection < card.get_num_Files()) {
    DWIN_Draw_Rectangle(1, Color_Bg_Black, 0, MBASE(selection - scrollpos) - 18, 14, MBASE(selection - scrollpos) + 33);
    if (selection > 0) {
      DWIN_Draw_Rectangle(1, Color_Bg_Black, LBLX, MBASE(selection - scrollpos) - 14, 271, MBASE(selection - scrollpos) + 28);
      Draw_SD_Item(selection, selection - scrollpos);
    }
    filescrl = 0;
    selection++; // Select Down
    if (selection > scrollpos + MROWS) {
      scrollpos++;
      DWIN_Frame_AreaMove(1, 2, MLINE, Color_Bg_Black, 0, 31, DWIN_WIDTH, 349);
      Draw_SD_Item(selection, selection - scrollpos);
    }
    DWIN_Draw_Rectangle(1, GetColor(eeprom_settings.cursor_color, Rectangle_Color), 0, MBASE(selection - scrollpos) - 18, 14, MBASE(selection - scrollpos) + 33);
  }
  else if (encoder_diffState == ENCODER_DIFF_CCW && selection > 0) {
    DWIN_Draw_Rectangle(1, Color_Bg_Black, 0, MBASE(selection - scrollpos) - 18, 14, MBASE(selection - scrollpos) + 33);
    DWIN_Draw_Rectangle(1, Color_Bg_Black, LBLX, MBASE(selection - scrollpos) - 14, 271, MBASE(selection - scrollpos) + 28);
    Draw_SD_Item(selection, selection - scrollpos);
    filescrl = 0;
    selection--; // Select Up
    if (selection < scrollpos) {
      scrollpos--;
      DWIN_Frame_AreaMove(1, 3, MLINE, Color_Bg_Black, 0, 31, DWIN_WIDTH, 349);
      Draw_SD_Item(selection, selection - scrollpos);
    }
    DWIN_Draw_Rectangle(1, GetColor(eeprom_settings.cursor_color, Rectangle_Color), 0, MBASE(selection - scrollpos) - 18, 14, MBASE(selection - scrollpos) + 33);
  }
  else if (encoder_diffState == ENCODER_DIFF_ENTER) {
    if (selection == 0) {
      if (card.flag.workDirIsRoot) {
        process = Main;
        Draw_Main_Menu();
      }
      else {
        card.cdup();
        Draw_SD_List();
      }
    }
    else {
      card.getfilename_sorted(SD_ORDER(selection - 1, card.get_num_Files()));
      if (card.flag.filenameIsDir) {
        card.cd(card.filename);
        Draw_SD_List();
      }
      else {
        card.openAndPrintFile(card.filename);
      }
    }
  }
  DWIN_UpdateLCD();
}

void CrealityDWINClass::Print_Screen_Control() {
  EncoderState encoder_diffState = Encoder_ReceiveAnalyze();
  if (encoder_diffState == ENCODER_DIFF_NO) return;
  if (encoder_diffState == ENCODER_DIFF_CW && selection < PRINT_COUNT - 1) {
    selection++; // Select Down
    Print_Screen_Icons();
  }
  else if (encoder_diffState == ENCODER_DIFF_CCW && selection > 0) {
    selection--; // Select Up
    Print_Screen_Icons();
  }
  else if (encoder_diffState == ENCODER_DIFF_ENTER) {
    switch (selection) {
      case PRINT_SETUP:
        Draw_Menu(Tune);
        Update_Status_Bar(true);
        break;
      case PRINT_PAUSE_RESUME:
        if (paused) {
          if (sdprint) {
            wait_for_user = false;
            #if ENABLED(PARK_HEAD_ON_PAUSE)
              card.startOrResumeFilePrinting();
              TERN_(POWER_LOSS_RECOVERY, recovery.prepare());
            #else
              #if HAS_HEATED_BED
                cmd[sprintf_P(cmd, PSTR("M140 S%i"), pausebed)] = '\0';
                gcode.process_subcommands_now(cmd);
              #endif
              #if HAS_EXTRUDERS
                cmd[sprintf_P(cmd, PSTR("M109 S%i"), pausetemp)] = '\0';
                gcode.process_subcommands_now(cmd);
              #endif
              TERN_(HAS_FAN, thermalManager.fan_speed[0] = pausefan);
              planner.synchronize();
              TERN_(SDSUPPORT, queue.inject(F("M24")));
            #endif
          }
          else {
            TERN_(HOST_ACTION_COMMANDS, hostui.resume());
          }
          Draw_Print_Screen();
        }
        else
          Popup_Handler(Pause);
        break;
      case PRINT_STOP: Popup_Handler(Stop); break;
    }
  }
  DWIN_UpdateLCD();
}

void CrealityDWINClass::Popup_Control() {
  EncoderState encoder_diffState = Encoder_ReceiveAnalyze();
  if (encoder_diffState == ENCODER_DIFF_NO) return;
  if (encoder_diffState == ENCODER_DIFF_CW && selection < 1) {
    selection++;
    Popup_Select();
  }
  else if (encoder_diffState == ENCODER_DIFF_CCW && selection > 0) {
    selection--;
    Popup_Select();
  }
  else if (encoder_diffState == ENCODER_DIFF_ENTER) {
    switch (popup) {
      case Pause:
        if (selection == 0) {
          if (sdprint) {
            #if ENABLED(POWER_LOSS_RECOVERY)
              if (recovery.enabled) recovery.save(true);
            #endif
            #if ENABLED(PARK_HEAD_ON_PAUSE)
              Popup_Handler(Home, true);
              #if ENABLED(SDSUPPORT)
                if (IS_SD_PRINTING()) card.pauseSDPrint();
              #endif
              planner.synchronize();
              queue.inject(F("M125 P1"));
              planner.synchronize();
            #else
              queue.inject(F("M25"));
              TERN_(HAS_HOTEND, pausetemp = thermalManager.temp_hotend[0].target);
              TERN_(HAS_HEATED_BED, pausebed = thermalManager.temp_bed.target);
              TERN_(HAS_FAN, pausefan = thermalManager.fan_speed[0]);
              thermalManager.cooldown();
            #endif
          }
          else {
            TERN_(HOST_ACTION_COMMANDS, hostui.pause());
          }
        }
        Draw_Print_Screen();
        break;
      case Stop:
        if (selection == 0) {
          if (sdprint) {
            ui.abort_print();
            thermalManager.cooldown();
          }
          else {
            TERN_(HOST_ACTION_COMMANDS, hostui.cancel());
          }
        }
        else
          Draw_Print_Screen();
        break;
      case Resume:
        if (selection == 0)
            queue.inject( TERN(POWER_LOSS_RECOVERY, F("M1000"), F("M25")) );
        else {
            queue.inject( TERN(POWER_LOSS_RECOVERY, F("M1000 C"), F("M25")) );
          Draw_Main_Menu();
        }
        break;

      #if HAS_HOTEND
        case ETemp:
          if (selection == 0) {
//            thermalManager.setTargetHotend(EXTRUDE_MINTEMP, 0);
//            thermalManager.set_fan_speed(0, MAX_FAN_SPEED);
            Draw_Menu(PreheatHotend);
          }
          else
            Redraw_Menu(true, true, false);
          break;
      #endif

      #if HAS_BED_PROBE
        case ManualProbing:
          if (selection == 0) {
            char buf[80];
            const float dif = probe.probe_at_point(current_position.x, current_position.y, PROBE_PT_STOW, 0, false) - corner_avg;
            sprintf_P(buf, dif > 0 ? PSTR("Corner is %smm high") : PSTR("Corner is %smm low"), dtostrf(abs(dif), 1, 3, str_1));
            Update_Status(buf);
          }
          else {
            Redraw_Menu(true, true, false);
            Update_Status("");
          }
          break;
      #endif

      #if ENABLED(ADVANCED_PAUSE_FEATURE)
        case ConfFilChange:
          if (selection == 0) {
            if (thermalManager.temp_hotend[0].target < thermalManager.extrude_min_temp)
              Popup_Handler(ETemp);
            else {
              if (thermalManager.temp_hotend[0].celsius < thermalManager.temp_hotend[0].target - 2) {
                Popup_Handler(Heating);
                thermalManager.wait_for_hotend(0);
              }
              Popup_Handler(FilChange);
              sprintf_P(cmd, PSTR("M600 B1 R%i"), thermalManager.temp_hotend[0].target);
              gcode.process_subcommands_now(cmd);
            }
          }
          else
            Redraw_Menu(true, true, false);
          break;
        case PurgeMore:
          if (selection == 0) {
            pause_menu_response = PAUSE_RESPONSE_EXTRUDE_MORE;
            Popup_Handler(FilChange);
          }
          else {
            pause_menu_response = PAUSE_RESPONSE_RESUME_PRINT;
            if (printing) Popup_Handler(Resuming);
            else Redraw_Menu(true, true, (active_menu==PreheatHotend));
          }
          break;
      #endif // ADVANCED_PAUSE_FEATURE

      #if HAS_MESH
        case SaveLevel:
          if (selection == 0) {
            #if ENABLED(AUTO_BED_LEVELING_UBL)
              gcode.process_subcommands_now(F("G29 S"));
              planner.synchronize();
              AudioFeedback(true);
            #else
              AudioFeedback(settings.save());
            #endif
          }
          Draw_Menu(Leveling, LEVELING_GET_MESH);
          break;
      #endif

      #if ENABLED(AUTO_BED_LEVELING_UBL)
        case MeshSlot:
          if (selection == 0) ubl.storage_slot = 0;
          Redraw_Menu(true, true);
          break;
      #endif
      default: break;
    }
  }
  DWIN_UpdateLCD();
}

void CrealityDWINClass::Confirm_Control() {
  EncoderState encoder_diffState = Encoder_ReceiveAnalyze();
  if (encoder_diffState == ENCODER_DIFF_NO) return;
  if (encoder_diffState == ENCODER_DIFF_ENTER) {
    switch (popup) {
      case Complete:
        Draw_Main_Menu();
        break;
      case FilInsert:
        Popup_Handler(FilChange);
        wait_for_user = false;
        break;
      case HeaterTime:
        Popup_Handler(Heating);
        wait_for_user = false;
        break;
      default:
        Redraw_Menu(true, true, false);
        wait_for_user = false;
        break;
    }
  }
  DWIN_UpdateLCD();
}

void CrealityDWINClass::Keyboard_Control() {
  const uint8_t keyboard_size = 34;
  static uint8_t key_selection = 0, cursor = 0;
  static char string[31];
  static bool uppercase = false, locked = false;
  if (reset_keyboard) {
    if (strcmp(stringpointer, "-") == 0) stringpointer[0] = '\0';
    key_selection = 0, cursor = strlen(stringpointer);
    uppercase = false, locked = false;
    reset_keyboard = false;
    strcpy(string, stringpointer);
  }
  EncoderState encoder_diffState = Encoder_ReceiveAnalyze();
  if (encoder_diffState == ENCODER_DIFF_NO) return;
  if (encoder_diffState == ENCODER_DIFF_CW) {
    Draw_Keys(key_selection, false, uppercase, locked);
    key_selection++;
    if (key_selection > keyboard_size) key_selection = 0;
    Draw_Keys(key_selection, true, uppercase, locked);
  }
  else if (encoder_diffState == ENCODER_DIFF_CCW) {
    Draw_Keys(key_selection, false, uppercase, locked);
    if (key_selection == 0) key_selection = keyboard_size+1;
    key_selection--;
    Draw_Keys(key_selection, true, uppercase, locked);
  }
  else if (encoder_diffState == ENCODER_DIFF_ENTER) {
    if (key_selection < 28) {
      if (key_selection == 19) {
        if (!numeric_keyboard) {
          if (locked) {
            uppercase = false, locked = false;
            Draw_Keyboard(keyboard_restrict, false, key_selection, uppercase, locked);
          } else if (uppercase) {
            locked = true;
            Draw_Keyboard(keyboard_restrict, false, key_selection, uppercase, locked);
          }
          else {
            uppercase = true;
            Draw_Keyboard(keyboard_restrict, false, key_selection, uppercase, locked);
          }
        }
      }
      else if (key_selection == 27) {
        cursor--;
        string[cursor] = '\0';
      }
      else {
        uint8_t index = key_selection;
        if (index > 19) index--;
        if (index > 27) index--;
        const char *keys;
        if (numeric_keyboard) keys = "1234567890&<>(){}[]*\"\':;!?";
        else keys = (uppercase) ? "QWERTYUIOPASDFGHJKLZXCVBNM" : "qwertyuiopasdfghjklzxcvbnm";
        if (!(keyboard_restrict && numeric_keyboard && index > 9)) {
          string[cursor] = keys[index];
          cursor++;
          string[cursor] = '\0';
        }
        if (!locked && uppercase) {
          uppercase = false;
          Draw_Keyboard(keyboard_restrict, false, key_selection, uppercase, locked);
        }
      }
    }
    else {
      switch (key_selection) {
        case 28:
          if (!numeric_keyboard) uppercase = false, locked = false;
          Draw_Keyboard(keyboard_restrict, !numeric_keyboard, key_selection, uppercase, locked);
          break;
        case 29:
          string[cursor] = '-';
          cursor++;
          string[cursor] = '\0';
          break;
        case 30:
          string[cursor] = '_';
          cursor++;
          string[cursor] = '\0';
          break;
        case 31:
          if (!keyboard_restrict) {
            string[cursor] = ' ';
            cursor++;
            string[cursor] = '\0';
          }
          break;
        case 32:
          if (!keyboard_restrict) {
            string[cursor] = '.';
            cursor++;
            string[cursor] = '\0';
          }
          break;
        case 33:
          if (!keyboard_restrict) {
            string[cursor] = '/';
            cursor++;
            string[cursor] = '\0';
          }
          break;
        case 34:
          if (string[0] == '\0') strcpy(string, "-");
          strcpy(stringpointer, string);
          process = Menu;
          if (KEY_Y_START < STATUS_Y) DWIN_Draw_Rectangle(1, Color_Bg_Black, 0, KEY_Y_START, DWIN_WIDTH, STATUS_Y); // Keyboard is higher than StatusArea
          Draw_Status_Area(true);
          Update_Status_Bar(true);
          break;
      }
    }
    if (strlen(string) > maxstringlen) string[maxstringlen] = '\0', cursor = maxstringlen;
    Draw_String(string, selection, (process==Keyboard), (maxstringlen > 10));
  }
  DWIN_UpdateLCD();
}


/* In-Menu Value Modification */

void CrealityDWINClass::Setup_Value(float value, float min, float max, float unit, uint8_t type) {
  if (TERN0(HAS_HOTEND, valuepointer == &thermalManager.temp_hotend[0].pid.Ki) || TERN0(HAS_HEATED_BED, valuepointer == &thermalManager.temp_bed.pid.Ki))
    tempvalue = unscalePID_i(value) * unit;
  else if (TERN0(HAS_HOTEND, valuepointer == &thermalManager.temp_hotend[0].pid.Kd) || TERN0(HAS_HEATED_BED, valuepointer == &thermalManager.temp_bed.pid.Kd))
    tempvalue = unscalePID_d(value) * unit;
  else
    tempvalue = value * unit;
  valuemin = min;
  valuemax = max;
  valueunit = unit;
  valuetype = type;
  process = Value;
  EncoderRate.enabled = true;
  Draw_Float(tempvalue / unit, selection - scrollpos, true, valueunit);
}

void CrealityDWINClass::Modify_Value(float &value, float min, float max, float unit, void (*f)()/*=nullptr*/) {
  valuepointer = &value;
  funcpointer = f;
  Setup_Value((float)value, min, max, unit, 0);
}
void CrealityDWINClass::Modify_Value(uint8_t &value, float min, float max, float unit, void (*f)()/*=nullptr*/) {
  valuepointer = &value;
  funcpointer = f;
  Setup_Value((float)value, min, max, unit, 1);
}
void CrealityDWINClass::Modify_Value(uint16_t &value, float min, float max, float unit, void (*f)()/*=nullptr*/) {
  valuepointer = &value;
  funcpointer = f;
  Setup_Value((float)value, min, max, unit, 2);
}
void CrealityDWINClass::Modify_Value(int16_t &value, float min, float max, float unit, void (*f)()/*=nullptr*/) {
  valuepointer = &value;
  funcpointer = f;
  Setup_Value((float)value, min, max, unit, 3);
}
void CrealityDWINClass::Modify_Value(uint32_t &value, float min, float max, float unit, void (*f)()/*=nullptr*/) {
  valuepointer = &value;
  funcpointer = f;
  Setup_Value((float)value, min, max, unit, 4);
}
void CrealityDWINClass::Modify_Value(int8_t &value, float min, float max, float unit, void (*f)()/*=nullptr*/) {
  valuepointer = &value;
  funcpointer = f;
  Setup_Value((float)value, min, max, unit, 5);
}

void CrealityDWINClass::Modify_Option(uint8_t value, const char * const * options, uint8_t max) {
  tempvalue = value;
  valuepointer = const_cast<const char * *>(options);
  valuemin = 0;
  valuemax = max;
  process = Option;
  EncoderRate.enabled = true;
  Draw_Option(value, options, selection - scrollpos, true);
}

void CrealityDWINClass::Modify_String(char * string, uint8_t maxlength, bool restrict) {
  stringpointer = string;
  maxstringlen = maxlength;
  reset_keyboard = true;
  Draw_Keyboard(restrict, false);
  Draw_String(string, selection, true, (maxstringlen > 10));
}

/* Main Functions */

void CrealityDWINClass::Update_Status(const char * const text) {

  DEBUG_ECHOLNPGM("CrealityDWINClass::Update_Status (", text, ")");

  char header[4];
  LOOP_L_N(i, 3) header[i] = text[i];
  header[3] = '\0';
  if (strcmp_P(header, PSTR("<F>")) == 0) {
    LOOP_L_N(i, _MIN((size_t)LONG_FILENAME_LENGTH, strlen(text))) filename[i] = text[i + 3];
    filename[_MIN((size_t)LONG_FILENAME_LENGTH - 1, strlen(text))] = '\0';
    Draw_Print_Filename(true);
  }
  else {
    LOOP_L_N(i, _MIN((size_t)64, strlen(text))) statusmsg[i] = text[i];
    statusmsg[_MIN((size_t)64, strlen(text))] = '\0';
  }
}

void CrealityDWINClass::Start_Print(bool sd) {
  sdprint = sd;
  if (!printing) {
    printing = true;
    statusmsg[0] = '\0';
    if (sd) {
      #if ENABLED(POWER_LOSS_RECOVERY)
        if (recovery.valid()) {
          SdFile *diveDir = nullptr;
          const char * const fname = card.diveToFile(true, diveDir, recovery.info.sd_filename);
          card.selectFileByName(fname);
        }
      #endif
      strcpy_P(filename, card.longest_filename());
    }
    else
      strcpy_P(filename, "Host Print");
    TERN_(LCD_SET_PROGRESS_MANUALLY, ui.set_progress(0));
    TERN_(USE_M73_REMAINING_TIME, ui.set_remaining_time(0));
    Draw_Print_Screen();
  }
}

void CrealityDWINClass::Stop_Print() {
  printing = false;
  sdprint = false;
  thermalManager.cooldown();
  TERN_(LCD_SET_PROGRESS_MANUALLY, ui.set_progress(100 * (PROGRESS_SCALE)));
  TERN_(USE_M73_REMAINING_TIME, ui.set_remaining_time(0));
  Draw_Print_confirm();
}

void CrealityDWINClass::Update() {

//  DEBUG_SECTION(dwin, "CrealityDWINClass", true);
  static uint8_t prc = 99;
  static bool pri, pau, wfu;
  if (prc !=process || pri != printing || pau != paused || wfu != wait_for_user) {
    DEBUG_ECHOLNPGM("CrealityDWINClass::Update (process=", process, "/", last_process, ", printing=", printing, ", paused=", paused, ", wait_for_user=", wait_for_user, ")");
    prc=process; pri=printing; pau=paused; wfu=wait_for_user;
}

  State_Update();
  Screen_Update();
  switch (process) {
    case Main:      Main_Menu_Control();    break;
    case Menu:      Menu_Control();         break;
    case Value:     Value_Control();        break;
    case Option:    Option_Control();       break;
    case File:      File_Control();         break;
    case Print:     Print_Screen_Control(); break;
    case Popup:     Popup_Control();        break;
    case Confirm:   Confirm_Control();      break;
    case Keyboard:  Keyboard_Control();     break;
  }
}

void MarlinUI::update() { CrealityDWIN.Update(); }

#if HAS_LCD_BRIGHTNESS
  void MarlinUI::_set_brightness() { DWIN_LCD_Brightness(backlight ? brightness : 0); }
#endif

void CrealityDWINClass::State_Update() {
  if ((print_job_timer.isRunning() || print_job_timer.isPaused()) != printing) {
    if (!printing) Start_Print(card.isFileOpen() || TERN0(POWER_LOSS_RECOVERY, recovery.valid()));
    else Stop_Print();
  }
  if (print_job_timer.isPaused() != paused) {
    paused = print_job_timer.isPaused();
    if (process == Print) Print_Screen_Icons();
    if (process == Wait && !paused) Redraw_Menu(true, true);
  }
  if (wait_for_user && !(process == Confirm) && !print_job_timer.isPaused())
    Confirm_Handler(UserInput);
  #if ENABLED(ADVANCED_PAUSE_FEATURE)
    if (process == Popup && popup == PurgeMore) {
      if (pause_menu_response == PAUSE_RESPONSE_EXTRUDE_MORE)
        Popup_Handler(FilChange);
      else if (pause_menu_response == PAUSE_RESPONSE_RESUME_PRINT) {
        if (printing) Popup_Handler(Resuming);
        else Redraw_Menu(true, true, (active_menu==PreheatHotend));
      }
    }
  #endif
  #if ENABLED(FILAMENT_RUNOUT_SENSOR)
    static bool ranout = false;
    if (runout.filament_ran_out != ranout) {
      ranout = runout.filament_ran_out;
      if (ranout) Popup_Handler(Runout);
    }
  #endif
}

void CrealityDWINClass::Screen_Update() {
  const millis_t ms = millis();
  static millis_t scrltime = 0;
  if (ELAPSED(ms, scrltime)) {
    scrltime = ms + 200;
    if (process != Keyboard) Update_Status_Bar();
    if (process == Print) Draw_Print_Filename();
  }

  static millis_t statustime = 0;
  if (ELAPSED(ms, statustime) && process != Keyboard) {
    statustime = ms + 500;
    Draw_Status_Area();
  }

  static millis_t printtime = 0;
  if (ELAPSED(ms, printtime)) {
    printtime = ms + 1000;
    if (process == Print) {
      Draw_Print_ProgressBar();
      Draw_Print_ProgressElapsed();
      TERN_(USE_M73_REMAINING_TIME, Draw_Print_ProgressRemain());
    }
  }

  static bool mounted = card.isMounted();
  if (mounted != card.isMounted()) {
    mounted = card.isMounted();
    if (process == File)
      Draw_SD_List();
  }

  #if HAS_HOTEND
    static int16_t hotendtarget = -1;
  #endif
  #if HAS_HEATED_BED
    static int16_t bedtarget = -1;
  #endif
  #if HAS_FAN
    static int16_t fanspeed = -1;
  #endif

  #if HAS_ZOFFSET_ITEM
    static float lastzoffset = zoffsetvalue;
    if (zoffsetvalue != lastzoffset) {
      lastzoffset = zoffsetvalue;
      #if HAS_BED_PROBE
        probe.offset.z = zoffsetvalue;
      #else
        set_home_offset(Z_AXIS, -zoffsetvalue);
      #endif
    }

    #if HAS_BED_PROBE
      if (probe.offset.z != lastzoffset)
        zoffsetvalue = lastzoffset = probe.offset.z;
    #else
      if (-home_offset.z != lastzoffset)
        zoffsetvalue = lastzoffset = -home_offset.z;
    #endif
  #endif // HAS_ZOFFSET_ITEM

  if (process == Menu || process == Value) {
    switch (active_menu) {
      case TempMenu:
        #if HAS_HOTEND
          if (thermalManager.temp_hotend[0].target != hotendtarget) {
            hotendtarget = thermalManager.temp_hotend[0].target;
            if (scrollpos <= TEMP_HOTEND && TEMP_HOTEND <= scrollpos + MROWS) {
              if (process != Value || selection != TEMP_HOTEND - scrollpos)
                Draw_Float(thermalManager.temp_hotend[0].target, TEMP_HOTEND - scrollpos, false, 1);
            }
          }
        #endif
        #if HAS_HEATED_BED
          if (thermalManager.temp_bed.target != bedtarget) {
            bedtarget = thermalManager.temp_bed.target;
            if (scrollpos <= TEMP_BED && TEMP_BED <= scrollpos + MROWS) {
              if (process != Value || selection != TEMP_HOTEND - scrollpos)
                Draw_Float(thermalManager.temp_bed.target, TEMP_BED - scrollpos, false, 1);
            }
          }
        #endif
        #if HAS_FAN
          if (thermalManager.fan_speed[0] != fanspeed) {
            fanspeed = thermalManager.fan_speed[0];
            if (scrollpos <= TEMP_FAN && TEMP_FAN <= scrollpos + MROWS) {
              if (process != Value || selection != TEMP_HOTEND - scrollpos)
                Draw_Float(thermalManager.fan_speed[0], TEMP_FAN - scrollpos, false, 1);
            }
          }
        #endif
        break;
      case Tune:
        #if HAS_HOTEND
          if (thermalManager.temp_hotend[0].target != hotendtarget) {
            hotendtarget = thermalManager.temp_hotend[0].target;
            if (scrollpos <= TUNE_HOTEND && TUNE_HOTEND <= scrollpos + MROWS) {
              if (process != Value || selection != TEMP_HOTEND - scrollpos)
                Draw_Float(thermalManager.temp_hotend[0].target, TUNE_HOTEND - scrollpos, false, 1);
            }
          }
        #endif
        #if HAS_HEATED_BED
          if (thermalManager.temp_bed.target != bedtarget) {
            bedtarget = thermalManager.temp_bed.target;
            if (scrollpos <= TUNE_BED && TUNE_BED <= scrollpos + MROWS) {
              if (process != Value || selection != TEMP_HOTEND - scrollpos)
                Draw_Float(thermalManager.temp_bed.target, TUNE_BED - scrollpos, false, 1);
            }
          }
        #endif
        #if HAS_FAN
          if (thermalManager.fan_speed[0] != fanspeed) {
            fanspeed = thermalManager.fan_speed[0];
            if (scrollpos <= TUNE_FAN && TUNE_FAN <= scrollpos + MROWS) {
              if (process != Value || selection != TEMP_HOTEND - scrollpos)
                Draw_Float(thermalManager.fan_speed[0], TUNE_FAN - scrollpos, false, 1);
            }
          }
        #endif
        break;
    }
  }
}

void CrealityDWINClass::AudioFeedback(const bool success/*=true*/) {
  if (success) {
    if (ui.buzzer_enabled) {
      BUZZ(100, 659);
      BUZZ( 10,   0);
      BUZZ(100, 698);
    }
    else Update_Status("Success");
  }
  else if (ui.buzzer_enabled)
    BUZZ(40, 440);
  else
    Update_Status("Failed");
}

void CrealityDWINClass::Save_Settings(char *buff) {
  TERN_(AUTO_BED_LEVELING_UBL, eeprom_settings.tilt_grid_size = mesh_conf.tilt_grid - 1);
  eeprom_settings.corner_pos = corner_pos * 10;
  #if ENABLED(HOST_ACTION_COMMANDS)
    eeprom_settings.host_action_label_1 = Encode_String(action1);
    eeprom_settings.host_action_label_2 = Encode_String(action2);
    eeprom_settings.host_action_label_3 = Encode_String(action3);
  #endif
  
  TERN_(PREVENT_COLD_EXTRUSION, eeprom_settings.extrude_min_temp = _MIN(thermalManager.extrude_min_temp, 255));

  memcpy(buff, &eeprom_settings, _MIN(sizeof(eeprom_settings), eeprom_data_size));
}

void CrealityDWINClass::Load_Settings(const char *buff) {
  memcpy(&eeprom_settings, buff, _MIN(sizeof(eeprom_settings), eeprom_data_size));
  TERN_(AUTO_BED_LEVELING_UBL, mesh_conf.tilt_grid = eeprom_settings.tilt_grid_size + 1);
  if (eeprom_settings.corner_pos == 0) eeprom_settings.corner_pos = 325;
  corner_pos = eeprom_settings.corner_pos / 10.0f;
  #if ENABLED(HOST_ACTION_COMMANDS)
    Decode_String(eeprom_settings.host_action_label_1, action1);
    Decode_String(eeprom_settings.host_action_label_2, action2);
    Decode_String(eeprom_settings.host_action_label_3, action3);
  #endif
  
  TERN_(PREVENT_COLD_EXTRUSION, thermalManager.extrude_min_temp = eeprom_settings.extrude_min_temp);

  Redraw_Screen();
  #if ENABLED(POWER_LOSS_RECOVERY)
    static bool init = true;
    if (init) {
      init = false;
      queue.inject(F("M1000 S"));
    }
  #endif
}

void CrealityDWINClass::Reset_Settings() {
  eeprom_settings.time_format_textual = false;
  TERN_(AUTO_BED_LEVELING_UBL, eeprom_settings.tilt_grid_size = 0);
  eeprom_settings.corner_pos = 300;
  eeprom_settings.cursor_color = 0;
  eeprom_settings.menu_split_line = 0;
  eeprom_settings.menu_top_bg = 0;
  eeprom_settings.menu_top_txt = 0;
  eeprom_settings.highlight_box = 0;
  eeprom_settings.progress_percent = 0;
  eeprom_settings.progress_time = 0;
  eeprom_settings.status_bar_text = 0;
  eeprom_settings.status_area_text = 0;
  eeprom_settings.coordinates_text = 0;
  eeprom_settings.coordinates_split_line = 0;
  #if ENABLED(HOST_ACTION_COMMANDS)
    eeprom_settings.host_action_label_1 = 0;
    eeprom_settings.host_action_label_2 = 0;
    eeprom_settings.host_action_label_3 = 0;
    action1[0] = action2[0] = action3[0] = '-';
  #endif
  TERN_(PREVENT_COLD_EXTRUSION, thermalManager.extrude_min_temp = eeprom_settings.extrude_min_temp = EXTRUDE_MINTEMP);
  TERN_(AUTO_BED_LEVELING_UBL, mesh_conf.tilt_grid = eeprom_settings.tilt_grid_size + 1);
  corner_pos = eeprom_settings.corner_pos / 10.0f;
  TERN_(SOUND_MENU_ITEM, ui.buzzer_enabled = true);
  Redraw_Screen();
}

void MarlinUI::init_lcd() {
  delay(800);
  SERIAL_ECHOPGM("\nDWIN handshake ");
  if (DWIN_Handshake()) SERIAL_ECHOLNPGM("ok."); else SERIAL_ECHOLNPGM("error.");
  DWIN_Frame_SetDir(1); // Orientation 90°
  DWIN_UpdateLCD();     // Show bootscreen (first image)
  Encoder_Configuration();
  for (uint16_t t = 0; t <= 100; t += 2) {
    DWIN_ICON_Show(ICON, ICON_Bar, 15, 260);
    DWIN_Draw_Rectangle(1, Color_Bg_Black, 15 + t * 242 / 100, 260, 257, 280);
    DWIN_UpdateLCD();
    delay(20);
  }
  DWIN_JPG_CacheTo1(Language_English);
  CrealityDWIN.Redraw_Screen();
}

#if ENABLED(ADVANCED_PAUSE_FEATURE)
  void MarlinUI::pause_show_message(const PauseMessage message, const PauseMode mode/*=PAUSE_MODE_SAME*/, const uint8_t extruder/*=active_extruder*/) {

  DEBUG_ECHOLNPGM("MarlinUI::pause_show_message (message=", message, ", mode=", mode, ")");

    switch (message) {
      case PAUSE_MESSAGE_INSERT:  CrealityDWIN.Confirm_Handler(FilInsert);  break;
      case PAUSE_MESSAGE_PURGE:
      case PAUSE_MESSAGE_OPTION:
        pause_menu_response = PAUSE_RESPONSE_WAIT_FOR;
        CrealityDWIN.Popup_Handler(PurgeMore);
        break;
      case PAUSE_MESSAGE_HEAT:    CrealityDWIN.Confirm_Handler(HeaterTime); break;
      case PAUSE_MESSAGE_WAITING: CrealityDWIN.Update_Status(GET_TEXT(MSG_ADVANCED_PAUSE_WAITING));
                                  CrealityDWIN.Draw_Print_Screen();         
                                  break;
      case PAUSE_MESSAGE_PARKING: CrealityDWIN.Update_Status(GET_TEXT(MSG_PAUSE_PRINT_PARKING)); 
                                  CrealityDWIN.Popup_Handler(Home);   
                                  break;
      default: break;
    }
  }
#endif
